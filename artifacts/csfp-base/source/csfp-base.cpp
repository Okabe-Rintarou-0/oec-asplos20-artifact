// csfp-base.cpp
// Close-spaced, frame-parallel base
//
// Copyright 2019 Bradley Denby
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at <http://www.apache.org/licenses/LICENSE-2.0>.
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

// Standard library
#include <algorithm>           // max
#include <cmath>               // floor
#include <cstddef>             // size_t
#include <cstdint>             // exit, EXIT_SUCCESS, EXIT_FAILURE
#include <cstdlib>             // atoi
#include <iomanip>             // setw, setprecision, setfill
#include <ios>                 // fixed
#include <iostream>            // cout
#include <ostream>             // endl
#include <sstream>             // ostringstream

// satsim
#include <Capacitor.hpp>       // Capacitor
#include <ChameleonImager.hpp> // ChameleonImager
#include <EHSatellite.hpp>     // EHSatellite
#include <EHSystem.hpp>        // EHSystem
#include <EnergyConsumer.hpp>  // EnergyConsumer
#include <EnergyHarvester.hpp> // EnergyHarvester
#include <JetsonTX2.hpp>       // JetsonTX2
#include <Job.hpp>             // Job
#include <Logger.hpp>          // Logger
#include <MAIAdacs.hpp>        // MAIAdacs
#include <Orbit.hpp>           // Orbit
#include <Satellite.hpp>       // Satellite
#include <SimpleSolarCell.hpp> // SimpleSolarCell

int main(int argc, char **argv) {
    size_t tasksPerJob = 3072;
    size_t pipelineDepth = 1;
    // Parse command line argument(s)
    if (argc != 2) {
        std::cout << "Usage: ./" << argv[0] << " int"
                  << std::endl
                  << "  int: pipeline depth (minimum 1)"
                  << std::endl;
        std::exit(EXIT_FAILURE);
    } else {
        pipelineDepth = std::max(1, std::atoi(argv[1]));
    }
    // Set up logger
    satsim::Logger logger("s");
    // Set up jobs
    size_t gtfCount = 1642;
    std::vector<satsim::Job *> gtfs;
    for (size_t i = 0; i < gtfCount; i++) {
        gtfs.push_back(new satsim::Job(i, tasksPerJob));
    }
    // Build each satellite in the pipeline
    std::vector<satsim::EHSatellite *> ehsatellites;
    // 注意到，这边的i会在下面被用作workerId。workerId会在job中被用于映射到分配的任务
    for (size_t i = 0; i < pipelineDepth; i++) {
        // Orbit: 400 km altitude polar orbit (93 min period = 5580.0 sec)
        // For CSFP, all satellites are at the same radial position (0.0)
        satsim::Orbit orbit(5580.0, 0.0);
        // Energy harvester
        // Azur Space 3G30A 2.5E14; three in series of two cells in parallel (6 ct.)
        double sscVmp_V = 7.0290;
        double sscCmp_A = 1.0034;
        double nodeVoltage_V = sscVmp_V; // Start at maximum node voltage
        satsim::EnergyHarvester *simpleSolarCell =
                new satsim::SimpleSolarCell(sscVmp_V, sscCmp_A, nodeVoltage_V, &logger);
        simpleSolarCell->setWorkerId(i);
        // Energy storage
        // AVX SuperCapacitor SCMR22L105M; five in parallel
        // Cap_V: nodeVoltage_V-sscCmp_A*esr_Ohm is max valid voltage for this model
        // Assuming sim starts with this Cap_V, charge is Cap_V*capacity_F
        double capacity_F = 5.0;
        double esr_Ohm = 0.168;
        double charge_C = (nodeVoltage_V - sscCmp_A * esr_Ohm) * capacity_F; //Q = CU
        satsim::Capacitor capacitor(capacity_F, esr_Ohm, charge_C, sscCmp_A, &logger);
        // Minimal energy harvesting system
        satsim::EHSystem ehsystem(*simpleSolarCell, capacitor, &logger);
        // Clean up energy harvester
        delete simpleSolarCell;
        // Energy consumer: Jetson TX2
        satsim::JetsonTX2 jetsonTX2(
                nodeVoltage_V, satsim::JetsonTX2::PowerState::IDLE, &logger
        );
        jetsonTX2.setWorkerId(i);
        jetsonTX2.logEvent(
                "jetson-" + std::to_string(jetsonTX2.getWorkerId()) + "-idle-start", 0.0
        );
        ehsystem.addEnergyConsumer(jetsonTX2);
        // Energy consumer: Chameleon imager
        satsim::ChameleonImager chameleonImager(
                nodeVoltage_V, satsim::ChameleonImager::PowerState::IDLE, &logger
        );
        chameleonImager.setWorkerId(i);
        //chameleonImager.logEvent(
        // "chamgr-"+std::to_string(chameleonImager.getWorkerId())+"-idle-start",0.0
        //);
        ehsystem.addEnergyConsumer(chameleonImager);
        // Energy consumer: MAI ADACS
        satsim::MAIAdacs maiadacs(
                nodeVoltage_V, satsim::MAIAdacs::PowerState::NADIR, &logger
        );
        ehsystem.addEnergyConsumer(maiadacs);
        // Push back new energy harvesting satellite
        ehsatellites.push_back(new satsim::EHSatellite(orbit, ehsystem, &logger));
    }
    // Run sim-compose
    const double PI = satsim::Orbit::TAU / 2.0;
    const double radPerGtf = PI / gtfs.size();
    //const double STEP = 0.00314159265358979323846;
    //double thresh     = 0.00314159265358979323846;
    //double percent    = 0.0;
    bool simulate = true;
    while (simulate) {
        // Prove that the sim-compose should still run
        simulate = false;
        // Run sim-compose for each satellite
        // 枚举所有的卫星（卫星的数量等于pipeline的深度），下面的jiPtr和ciPtr都是对应于当前卫星的Jetson和相机
        for (size_t i = 0; i < ehsatellites.size(); i++) {
            satsim::EHSatellite *ehsPtr = ehsatellites.at(i);
            double ehsPosn = ehsPtr->getOrbit().getPosn();
            // if posn < PI check to see if new jobs are added
            if (ehsPosn < PI) {
                simulate = true;
                std::vector<satsim::EnergyConsumer *> ecs = ehsPtr->getEnergyConsumers();
                satsim::JetsonTX2 *jtPtr =
                        dynamic_cast<satsim::JetsonTX2 *>(ecs.at(0));
                satsim::ChameleonImager *ciPtr =
                        dynamic_cast<satsim::ChameleonImager *>(ecs.at(1));
                satsim::Job *jobPtr = gtfs.at(std::floor(ehsPosn / radPerGtf));
                // Push job onto ChameleonImager if IDLE and readyImages is empty and
                // the ground track frame has unclaimed tasks
                if (ciPtr->isIdle() && !ciPtr->hasImage() &&
                    jobPtr->getUnclaimedTaskCount() > 0) {
                    // For CSFP, all tasks are claimed
                    // 分配至多tasksPerJob个任务给imager。
                    jobPtr->claimTasks(ciPtr->getWorkerId(), tasksPerJob);
                    ciPtr->addClaimedJob(jobPtr);
                }
                // Transfer job from ChameleonImager to Jetson if
                // ChameleonImager has completed work and Jetson is low on work
                if (ciPtr->hasImage() && jtPtr->getClaimedJobCount() == 0) {
                    while (ciPtr->hasImage()) {
                        // 把chameleonImager中的任务移交给jetson
                        jtPtr->addClaimedJob(ciPtr->dequeImage());
                    }
                }
                // Clean up
                jtPtr = NULL;
                ciPtr = NULL;
                jobPtr = NULL;
                for (size_t j = 0; j < ecs.size(); j++) {
                    ecs.at(j) = NULL;
                }
                ecs.clear();
                // Update
                ehsPtr->update(2.0e-5);
            }
                // otherwise, continue updating if Jetson is not idle
            else {
                std::vector<satsim::EnergyConsumer *> ecs = ehsPtr->getEnergyConsumers();
                satsim::JetsonTX2 *jtPtr = dynamic_cast<satsim::JetsonTX2 *>(ecs.at(0));
                // 只要此时jetson还在处于工作状态，那么就继续进行仿真，直到jetson完成工作。
                // 只要有一个jetson处于工作状态那么仿真就会继续
                if (!jtPtr->isIdle()) {
                    simulate = true;
                    // Update
                    ehsPtr->update(2.0e-5);
                }
                // Clean up
                jtPtr = NULL;
                for (size_t j = 0; j < ecs.size(); j++) {
                    ecs.at(j) = NULL;
                }
                ecs.clear();
            }
            // Clean up
            ehsPtr = NULL;
        }
        // Progress report
        //if(ehsatellites.back()->getOrbit().getPosn() >= thresh) {
        //  thresh += STEP;
        //  percent += 0.1;
        //  std::cout << std::setw(5) << std::fixed << std::setprecision(1)
        //            << percent << "% complete"
        //            << std::endl;
        //}
    }
    // 仿真结束，此时所有的jetson都已经完成了工作从而进入了IDLE的状态。
    // Simulation is over, all Jetsons are in idle mode
    for (size_t i = 0; i < ehsatellites.size(); i++) {
        satsim::EHSatellite *ehsPtr = ehsatellites.at(i);
        std::vector<satsim::EnergyConsumer *> ecs = ehsPtr->getEnergyConsumers();
        satsim::JetsonTX2 *jtPtr = dynamic_cast<satsim::JetsonTX2 *>(ecs.at(0));
        jtPtr->logEvent(
                "jetson-" + std::to_string(jtPtr->getWorkerId()) + "-idle-stop",
                jtPtr->getSimTime()
        );
        // Clean up
        jtPtr = NULL;
        for (size_t j = 0; j < ecs.size(); j++) {
            ecs.at(j) = NULL;
        }
        ecs.clear();
        ehsPtr = NULL;
    }
    // Write out logs
    std::ostringstream oss;
    oss << "../logs/" << std::setfill('0') << std::setw(3)
        << std::atoi(argv[1]);
    logger.exportCsvs(oss.str());
    // Clean up each satellite in the pipeline
    for (size_t i = 0; i < ehsatellites.size(); i++) {
        delete ehsatellites.at(i);
    }
    // Clean up jobs
    for (size_t i = 0; i < gtfs.size(); i++) {
        delete gtfs.at(i);
    }
    std::exit(EXIT_SUCCESS);
}
