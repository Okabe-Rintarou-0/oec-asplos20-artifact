// EHSystem.cpp
// EHSystem class implementation file
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
#include <algorithm>    // max
#include <cmath>        // pow, sqrt
#include <string>       // string
#include <utility>      // move

// satsim
#include <Logger.hpp>   // Logger
#include <EHSystem.hpp> // EHSystem

namespace satsim {
  EHSystem::EHSystem(
   const EnergyHarvester& energyHarvester, const Capacitor& capacitor,
   Logger* logger
  ) : energyHarvester(energyHarvester.clone()), capacitor(capacitor),
      logger(logger), totalPower_W(0.0), simTime_sec(0.0) {
    updateNodeVoltage(); // initializes this->nodeVoltage_V
  }

  EHSystem::EHSystem(const EHSystem& ehsystem) :
   energyHarvester(ehsystem.getEnergyHarvester()),
   capacitor(ehsystem.getCapacitor()), totalPower_W(ehsystem.getTotalPower()),
   nodeVoltage_V(ehsystem.getNodeVoltage()), simTime_sec(ehsystem.getSimTime()),
   logger(ehsystem.getLogger()) {
    std::vector<const EnergyConsumer*> energyConsumers =
     ehsystem.getEnergyConsumers();
    for(size_t i=0; i<energyConsumers.size(); i++) {
      this->energyConsumers.push_back(energyConsumers.at(i)->clone());
      delete energyConsumers.at(i);
    }
  }

  EHSystem::EHSystem(EHSystem&& ehsystem) :
   energyHarvester(ehsystem.energyHarvester),
   capacitor(std::move(ehsystem.capacitor)),
   nodeVoltage_V(ehsystem.nodeVoltage_V), simTime_sec(ehsystem.simTime_sec),
   logger(ehsystem.logger) {
    ehsystem.energyHarvester = NULL;
    ehsystem.logger = NULL;
    for(size_t i=0; i<ehsystem.energyConsumers.size(); i++) {
      this->energyConsumers.push_back(ehsystem.energyConsumers.at(i));
      ehsystem.energyConsumers.at(i) = NULL;
    }
  }

  EHSystem::~EHSystem() {
    delete this->energyHarvester;
    for(size_t i=0; i<this->energyConsumers.size(); i++) {
      delete this->energyConsumers.at(i);
    }
  }

  EHSystem& EHSystem::operator=(const EHSystem& ehsystem) {
    EHSystem temp(ehsystem);
    *this = std::move(temp);
    return *this;
  }

  EHSystem& EHSystem::operator=(EHSystem&& ehsystem) {
    delete this->energyHarvester;
    this->energyHarvester = ehsystem.energyHarvester;
    ehsystem.energyHarvester = NULL;
    this->capacitor = std::move(ehsystem.capacitor);
    this->logger = ehsystem.logger;
    ehsystem.logger = NULL;
    for(size_t i=0; i<this->energyConsumers.size(); i++) {
      delete this->energyConsumers.at(i);
    }
    this->energyConsumers.clear();
    this->energyConsumers =
     std::vector<EnergyConsumer*>(ehsystem.energyConsumers.size());
    for(size_t i=0; i<ehsystem.energyConsumers.size(); i++) {
      this->energyConsumers.at(i) = ehsystem.energyConsumers.at(i);
      ehsystem.energyConsumers.at(i) = NULL;
    }
    return *this;
  }

  EnergyHarvester* EHSystem::getEnergyHarvester() const {
    return this->energyHarvester->clone();
  }

  Capacitor EHSystem::getCapacitor() const {
    return this->capacitor;
  }

  std::vector<const EnergyConsumer*> EHSystem::getEnergyConsumers() const {
    std::vector<const EnergyConsumer*> energyConsumers;
    for(size_t i=0; i<this->energyConsumers.size(); i++) {
      energyConsumers.push_back(this->energyConsumers.at(i)->clone());
    }
    return energyConsumers;
  }

  double EHSystem::getTotalPower() const {
    return this->totalPower_W;
  }

  double EHSystem::getNodeVoltage() const {
    return this->nodeVoltage_V;
  }

  double EHSystem::getSimTime() const {
    return this->simTime_sec;
  }

  // Send the actual pointers to allow for energy consumer state updates
  std::vector<EnergyConsumer*> EHSystem::getEnergyConsumers() {
    return this->energyConsumers;
  }

  Logger* EHSystem::getLogger() const {
    return this->logger;
  }

  void EHSystem::addEnergyConsumer(const EnergyConsumer& energyConsumer) {
    this->energyConsumers.push_back(energyConsumer.clone());
  }

  void EHSystem::logEvent(const std::string& name, const double& time) {
    if(this->logger!=NULL) {
      this->logger->logEvent(name, time);
    }
  }

  void EHSystem::logMeasurement(
   const std::string& name, const double& time, const double& measurement
  ) {
    if(this->logger!=NULL) {
      this->logger->logMeasurement(name, time, measurement);
    }
  }

  void EHSystem::update(const double& seconds) {
    // Sanitize input
    double sanitizedSeconds = std::max(0.0,seconds);
    // Update this->nodeVoltage_V (no time dep, required for all other updates)
    updateNodeVoltage();
    // Update energy harvester and get the current
    this->energyHarvester->setVoltage(this->nodeVoltage_V);
    energyHarvester->update(sanitizedSeconds);
    double harvestCurrent_A = energyHarvester->getCurrent();
    // Update each energy consuming device and get the total current draw
    double deviceCurrent_A = 0.0;
    for(size_t i=0; i<this->energyConsumers.size(); i++) {
      // 设备是并联的。
      energyConsumers.at(i)->setVoltage(this->nodeVoltage_V);
      energyConsumers.at(i)->update(sanitizedSeconds);
      deviceCurrent_A += energyConsumers.at(i)->getCurrent();
    }
    updateTotalPower(); // call this anytime energy consumers may have changed
    // Update capacitor
    // harvestCurrent_A 和 deviceCurrent_A 方向相反，一个产生电流一个消耗电流
    capacitor.setCurrent(harvestCurrent_A-deviceCurrent_A);
    capacitor.update(sanitizedSeconds);
    // Update sim-compose time
    this->simTime_sec += sanitizedSeconds;
  }

  double EHSystem::calculateDiscriminant() const {
    // 这边的discriminant即判别式，也就是后面用到的一元二次方程的判别式
    // E = IR + Q/C
    // E^2 - 4RP，在下一个函数会解释为什么是4RP
    return
     std::pow(
      energyHarvester->getCurrent()*capacitor.getEsr() +
      capacitor.getCharge()/capacitor.getCapacity(), 2.0
     ) - 4.0*capacitor.getEsr()*this->totalPower_W;
  }

  double EHSystem::calculateNodeVoltage() const {
    // 为了获取最大的输出功率，这边认为设备的内阻和电容的内阻相同
    // 我们假设设备的电压为V，则根据基尔霍夫定律，可得V + P/V*R = E
    // 由此可得一元二次方程：V^2 - EV + PR = 0，于是得到上面提及的判别式△=E^2 - 4PR
    // 同时，方程的解为1/2(E + √(E^2 - 4PR))
    return
     (
      energyHarvester->getCurrent()*capacitor.getEsr() +
      capacitor.getCharge()/capacitor.getCapacity() +
      std::sqrt(calculateDiscriminant())
     )*0.5;
  }

  void EHSystem::updateTotalPower() {
    double tally_W = 0.0;
    for(size_t i=0; i<this->energyConsumers.size(); i++) {
      tally_W += energyConsumers.at(i)->getPower();
    }
    this->totalPower_W = tally_W;
  }

  void EHSystem::updateNodeVoltage() {
    if(calculateDiscriminant()<0.0) {
      for(size_t i=0; i<this->energyConsumers.size(); i++) {
        energyConsumers.at(i)->setOff();
      }
      updateTotalPower();
      logEvent("ehs-blackout", this->simTime_sec);
    }
    this->nodeVoltage_V = calculateNodeVoltage();
  }
}
