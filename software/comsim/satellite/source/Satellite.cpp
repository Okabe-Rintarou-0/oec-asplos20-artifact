// Satellite.cpp
// Satellite class implementation file
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
#include <array>         // array
#include <cmath>         // floor
#include <cstddef>       // size_t
#include <cstdint>       // uint32_t, int16_t, uint16_t, uint8_t
#include <fstream>       // ifstream
#include <string>        // string, getline, substr, stoi, stod
#include <tuple>         // tuple
#include <utility>       // move

// comsim
#include <constants.hpp> // constants
#include <DateTime.hpp>  // DateTime
#include <Log.hpp>       // Log
#include <Satellite.hpp> // Satellite
#include <utilities.hpp> // sgp4

namespace comsim {
  Satellite::Satellite(
   const std::string& tleFile, const DateTime* const globalTime, Log* const log
  ) : tleEpoch(*globalTime), localTime(*globalTime), globalTime(globalTime),
      log(log) {
    // Set up to parse TLE file
    std::ifstream tleHandle(tleFile);
    std::string line = "";
    std::getline(tleHandle,line);
    while(line.substr(0,2)!="1 " && std::getline(tleHandle,line)) {}
    // Parse catalog number
    this->catalogNumber = static_cast<uint32_t>(std::stoi(line.substr(2,5)));
    // Parse TLE
    std::tuple<int16_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint32_t> dt =
     util::getTleEpoch(tleFile);
    // Record TLE epoch
    // TLE 中记录了卫星发射的日期
    this->tleEpoch = DateTime(
     std::get<0>(dt),std::get<1>(dt),std::get<2>(dt),std::get<3>(dt),
     std::get<4>(dt),std::get<5>(dt),std::get<6>(dt)
    );
    // Parse TLE drag term
    // BSTAR拖调制系数，采用十进制小数，适用GP4一般摄动理论的情况下、BSTAR大气阻力这一项，除此之外为辐射压系数。 BSTAR拖调制系数的单位是1/(地球半径)。
    this->bstar =
     std::stof(line.substr(53,1)+"0."+line.substr(54,5)+"e"+line.substr(59,2));
    // TLE line 2
    std::getline(tleHandle,line);
    // Parse TLE inclination
    this->inclination = std::stof(line.substr(8,8))*cnst::STR3_RAD_PER_DEG;
    // Parse TLE right ascension of node
    this->raan = std::stof(line.substr(17,8))*cnst::STR3_RAD_PER_DEG;
    // Parse TLE eccentricity
    this->eccentricity = std::stof("0."+line.substr(26,7));
    // Parse TLE argument of perigee
    this->argOfPerigee = std::stof(line.substr(34,8))*cnst::STR3_RAD_PER_DEG;
    // Parse TLE mean anomaly
    this->meanAnomaly = std::stof(line.substr(43,8))*cnst::STR3_RAD_PER_DEG;
    // Parse TLE mean motion
    this->meanMotion =
     std::stof(line.substr(52,11))*cnst::STR3_RAD_PER_REV/
     cnst::STR3_MIN_PER_DAY;
    // Calculate initial eciPosn
    std::array<float,3> sgp4Posn = util::sgp4(
     this->bstar, this->inclination, this->raan, this->eccentricity,
     this->argOfPerigee, this->meanAnomaly, this->meanMotion,
     static_cast<float>(util::calcTdiffMin(
      this->localTime.getYear(), this->localTime.getMonth(),
      this->localTime.getDay(), this->localTime.getHour(),
      this->localTime.getMinute(), this->localTime.getSecond(),
      this->localTime.getNanosecond(),
      this->tleEpoch.getYear(), this->tleEpoch.getMonth(),
      this->tleEpoch.getDay(), this->tleEpoch.getHour(),
      this->tleEpoch.getMinute(), this->tleEpoch.getSecond(),
      this->tleEpoch.getNanosecond()
     ))
    );
    this->eciPosn = {
     static_cast<double>(sgp4Posn.at(0)),
     static_cast<double>(sgp4Posn.at(1)),
     static_cast<double>(sgp4Posn.at(2))
    };
  }

  Satellite::Satellite(const Satellite& satellite) :
   catalogNumber(satellite.getCatalogNumber()),
   tleEpoch(satellite.getTLEEpoch()), bstar(satellite.getBstar()),
   inclination(satellite.getInclination()), raan(satellite.getRAAN()),
   eccentricity(satellite.getEccentricity()),
   argOfPerigee(satellite.getArgOfPerigee()),
   meanAnomaly(satellite.getMeanAnomaly()),
   meanMotion(satellite.getMeanMotion()), eciPosn(satellite.getECIPosn()),
   localTime(satellite.getLocalTime()), globalTime(satellite.getGlobalTime()),
   log(satellite.getLog()) {}

  Satellite::Satellite(Satellite&& satellite) :
   catalogNumber(satellite.catalogNumber), tleEpoch(satellite.tleEpoch),
   bstar(satellite.bstar), inclination(satellite.inclination),
   raan(satellite.raan), eccentricity(satellite.eccentricity),
   argOfPerigee(satellite.argOfPerigee), meanAnomaly(satellite.meanAnomaly),
   meanMotion(satellite.meanMotion), eciPosn(satellite.eciPosn),
   localTime(satellite.localTime), globalTime(satellite.globalTime),
   log(satellite.log) {
    satellite.globalTime = NULL;
    satellite.log = NULL;
  }

  Satellite::~Satellite() {
    this->globalTime = NULL;
    this->log = NULL;
  }

  Satellite& Satellite::operator=(const Satellite& satellite) {
    Satellite temp(satellite);
    *this = std::move(temp);
    return *this;
  }

  Satellite& Satellite::operator=(Satellite&& satellite) {
    this->catalogNumber = satellite.catalogNumber;
    this->tleEpoch = satellite.tleEpoch;
    this->bstar = satellite.bstar;
    this->inclination = satellite.inclination;
    this->raan = satellite.raan;
    this->eccentricity = satellite.eccentricity;
    this->argOfPerigee = satellite.argOfPerigee;
    this->meanAnomaly = satellite.meanAnomaly;
    this->meanMotion = satellite.meanMotion;
    this->eciPosn = satellite.eciPosn;
    this->localTime = satellite.localTime;
    this->globalTime = satellite.globalTime;
    this->log = satellite.log;
    satellite.globalTime = NULL;
    satellite.log = NULL;
    return *this;
  }

  Satellite* Satellite::clone() const {
    return new Satellite(*this);
  }

  uint32_t Satellite::getCatalogNumber() const {
    return this->catalogNumber;
  }

  DateTime Satellite::getTLEEpoch() const {
    return this->tleEpoch;
  }

  float Satellite::getBstar() const {
    return this->bstar;
  }

  float Satellite::getInclination() const {
    return this->inclination;
  }

  float Satellite::getRAAN() const {
    return this->raan;
  }

  float Satellite::getEccentricity() const {
    return this->eccentricity;
  }

  float Satellite::getArgOfPerigee() const {
    return this->argOfPerigee;
  }

  float Satellite::getMeanAnomaly() const {
    return this->meanAnomaly;
  }

  float Satellite::getMeanMotion() const {
    return this->meanMotion;
  }

  std::array<double,3> Satellite::getECIPosn() const {
    return this->eciPosn;
  }

  DateTime Satellite::getLocalTime() const {
    return this->localTime;
  }

  const DateTime* Satellite::getGlobalTime() const {
    return this->globalTime;
  }

  Log* Satellite::getLog() const {
    return this->log;
  }

  void Satellite::update(const uint32_t& nanosecond) {
    // **It is expected that this->globalTime has already been updated**
    // Perform (possibly custom) update for localTime
    this->localTime.update(nanosecond);
    // calculate eciPosn using SGP4
    std::array<float,3> sgp4Posn = util::sgp4(
     this->bstar, this->inclination, this->raan, this->eccentricity,
     this->argOfPerigee, this->meanAnomaly, this->meanMotion,
     util::calcTdiffMin(
      this->localTime.getYear(), this->localTime.getMonth(),
      this->localTime.getDay(), this->localTime.getHour(),
      this->localTime.getMinute(), this->localTime.getSecond(),
      this->localTime.getNanosecond(),
      this->tleEpoch.getYear(), this->tleEpoch.getMonth(),
      this->tleEpoch.getDay(), this->tleEpoch.getHour(),
      this->tleEpoch.getMinute(), this->tleEpoch.getSecond(),
      this->tleEpoch.getNanosecond()
     )
    );
    this->eciPosn = {
     static_cast<double>(sgp4Posn.at(0)),
     static_cast<double>(sgp4Posn.at(1)),
     static_cast<double>(sgp4Posn.at(2))
    };
  }

  void Satellite::setCatalogNumber(const uint32_t& catalogNumber) {
    this->catalogNumber = catalogNumber;
  }

  void Satellite::setLocalTime(const DateTime& localTime) {
    this->localTime = localTime;
  }

  void Satellite::update(const uint8_t& second, const uint32_t& nanosecond) {
    // **It is expected that this->globalTime has already been updated**
    // Perform (possibly custom) update for localTime
    this->localTime.update(second,nanosecond);
    // calculate eciPosn using SGP4
    std::array<float,3> sgp4Posn = util::sgp4(
     this->bstar, this->inclination, this->raan, this->eccentricity,
     this->argOfPerigee, this->meanAnomaly, this->meanMotion,
     util::calcTdiffMin(
      this->localTime.getYear(), this->localTime.getMonth(),
      this->localTime.getDay(), this->localTime.getHour(),
      this->localTime.getMinute(), this->localTime.getSecond(),
      this->localTime.getNanosecond(),
      this->tleEpoch.getYear(), this->tleEpoch.getMonth(),
      this->tleEpoch.getDay(), this->tleEpoch.getHour(),
      this->tleEpoch.getMinute(), this->tleEpoch.getSecond(),
      this->tleEpoch.getNanosecond()
     )
    );
    this->eciPosn = {
     static_cast<double>(sgp4Posn.at(0)),
     static_cast<double>(sgp4Posn.at(1)),
     static_cast<double>(sgp4Posn.at(2))
    };
  }

  void Satellite::update(
   const uint8_t& minute, const uint8_t& second, const uint32_t& nanosecond
  ) {
    // **It is expected that this->globalTime has already been updated**
    // Perform (possibly custom) update for localTime
    this->localTime.update(minute,second,nanosecond);
    // calculate eciPosn using SGP4
    std::array<float,3> sgp4Posn = util::sgp4(
     this->bstar, this->inclination, this->raan, this->eccentricity,
     this->argOfPerigee, this->meanAnomaly, this->meanMotion,
     util::calcTdiffMin(
      this->localTime.getYear(), this->localTime.getMonth(),
      this->localTime.getDay(), this->localTime.getHour(),
      this->localTime.getMinute(), this->localTime.getSecond(),
      this->localTime.getNanosecond(),
      this->tleEpoch.getYear(), this->tleEpoch.getMonth(),
      this->tleEpoch.getDay(), this->tleEpoch.getHour(),
      this->tleEpoch.getMinute(), this->tleEpoch.getSecond(),
      this->tleEpoch.getNanosecond()
     )
    );
    this->eciPosn = {
     static_cast<double>(sgp4Posn.at(0)),
     static_cast<double>(sgp4Posn.at(1)),
     static_cast<double>(sgp4Posn.at(2))
    };
  }

  void Satellite::update(
   const uint8_t& hour, const uint8_t& minute, const uint8_t& second,
   const uint32_t& nanosecond
  ) {
    // **It is expected that this->globalTime has already been updated**
    // Perform (possibly custom) update for localTime
    this->localTime.update(hour,minute,second,nanosecond);
    // calculate eciPosn using SGP4
    std::array<float,3> sgp4Posn = util::sgp4(
     this->bstar, this->inclination, this->raan, this->eccentricity,
     this->argOfPerigee, this->meanAnomaly, this->meanMotion,
     util::calcTdiffMin(
      this->localTime.getYear(), this->localTime.getMonth(),
      this->localTime.getDay(), this->localTime.getHour(),
      this->localTime.getMinute(), this->localTime.getSecond(),
      this->localTime.getNanosecond(),
      this->tleEpoch.getYear(), this->tleEpoch.getMonth(),
      this->tleEpoch.getDay(), this->tleEpoch.getHour(),
      this->tleEpoch.getMinute(), this->tleEpoch.getSecond(),
      this->tleEpoch.getNanosecond()
     )
    );
    this->eciPosn = {
     static_cast<double>(sgp4Posn.at(0)),
     static_cast<double>(sgp4Posn.at(1)),
     static_cast<double>(sgp4Posn.at(2))
    };
  }
}
