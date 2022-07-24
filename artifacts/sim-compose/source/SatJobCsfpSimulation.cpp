#include <Logger.hpp>
#include <iostream>
#include <Job.hpp>
#include <EHSatellite.hpp>
#include <SimpleSolarCell.hpp>
#include <JetsonTX2.hpp>
#include <ChameleonImager.hpp>
#include <MAIAdacs.hpp>
#include <cmath>
#include <sstream>
#include <iomanip>
#include "../include/SatJobCsfpSimulation.h"

// tasksPerJob = 3072
// gtfCount = 1642
// orbitPeriodSec = 5580.0
satsim::SatJobCsfpSimulation::SatJobCsfpSimulation(size_t pipelineDepth,
                                                   size_t tasksPerJob,
                                                   size_t gtfCount,
                                                   double orbitPeriodSec) :
        logger("s"),
        SatJobSimulation(pipelineDepth, tasksPerJob, gtfCount, orbitPeriodSec) {}

void satsim::SatJobCsfpSimulation::run() {
//    size_t tasksPerJob = 3072;
    size_t tasksPerJob = this->tasksPerJob;
    size_t pipelineDepth = this->pipelineDepth;
    // Set up jobs
//    size_t gtfCount = 1642;
    size_t gtfCount = this->gtfCount;

    for (size_t i = 0; i < gtfCount; i++) {
        this->gtfs.push_back(new satsim::Job(i, tasksPerJob));
    }
    // Build each satellite in the pipeline
    // 注意到，这边的i会在下面被用作workerId。workerId会在job中被用于映射到分配的任务
    for (size_t i = 0; i < pipelineDepth; i++) {
        // Orbit: 400 km altitude polar orbit (93 min period = 5580.0 sec)
        // For CSFP, all satellites are at the same radial position (0.0)
//        satsim::Orbit orbit(5580.0, 0.0);
        satsim::Orbit orbit(this->periodSec, 0.0);
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
        this->ehsatellites.push_back(new satsim::EHSatellite(orbit, ehsystem, &logger));
    }

    this->simulating = true;
}

// simSecs = 2e-5
void satsim::SatJobCsfpSimulation::update(double simSecs) {
    static const double PI = satsim::Orbit::TAU / 2.0;
    static const double radPerGtf = PI / this->gtfs.size();
    if (!this->simulating) {
        std::cout << "Simulating has ended!" << std::endl;
    }

    this->simulating = false;
    // Run sim-compose for each satellite
    // 枚举所有的卫星（卫星的数量等于pipeline的深度），下面的jiPtr和ciPtr都是对应于当前卫星的Jetson和相机
    for (auto ehsPtr : this->ehsatellites) {
        double ehsPosn = ehsPtr->getOrbit().getPosn();
        // if posn < PI check to see if new jobs are added
        if (ehsPosn < PI) {
            this->simulating = true;
            std::vector<satsim::EnergyConsumer *> ecs = ehsPtr->getEnergyConsumers();
            auto jtPtr = dynamic_cast<satsim::JetsonTX2 *>(ecs.at(0));
            auto ciPtr = dynamic_cast<satsim::ChameleonImager *>(ecs.at(1));
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
            for (auto &ec : ecs) {
                ec = nullptr;
            }
            ecs.clear();
            // Update
            ehsPtr->update(simSecs);
        } else { // otherwise, continue updating if Jetson is not idle
            std::vector<satsim::EnergyConsumer *> ecs = ehsPtr->getEnergyConsumers();
            auto jtPtr = dynamic_cast<satsim::JetsonTX2 *>(ecs.at(0));
            // 只要此时jetson还在处于工作状态，那么就继续进行仿真，直到jetson完成工作。
            // 只要有一个jetson处于工作状态那么仿真就会继续
            if (!jtPtr->isIdle()) {
                // Update
                this->simulating = true;
                ehsPtr->update(simSecs);
            }
            // Clean up
            for (auto &ec : ecs) {
                ec = nullptr;
            }
            ecs.clear();
        }
    }

    // It means it has just ended the simulation
    if (!this->simulating) {
        // Simulation is over, all Jetsons are in idle mode
        for (auto ehsPtr : this->ehsatellites) {
            std::vector<satsim::EnergyConsumer *> ecs = ehsPtr->getEnergyConsumers();
            auto jtPtr = dynamic_cast<satsim::JetsonTX2 *>(ecs.at(0));
            jtPtr->logEvent(
                    "jetson-" + std::to_string(jtPtr->getWorkerId()) + "-idle-stop",
                    jtPtr->getSimTime()
            );
            // Clean up
            for (auto &ec : ecs) {
                ec = nullptr;
            }
            ecs.clear();
        }
        // Write out logs
        std::ostringstream oss;
        oss << "../logs/" << std::setfill('0') << std::setw(3)
            << this->pipelineDepth;
        logger.exportCsvs(oss.str());
    }
}

bool satsim::SatJobCsfpSimulation::running() {
    return this->simulating;
}

satsim::SatJobCsfpSimulation::~SatJobCsfpSimulation() {
    // RAII style

    // Clean up each satellite in the pipeline
    for (auto & ehsatellite : this->ehsatellites) {
        delete ehsatellite;
    }

    // Clean up jobs
    for (auto & gtf : gtfs) {
        delete gtf;
    }
}
