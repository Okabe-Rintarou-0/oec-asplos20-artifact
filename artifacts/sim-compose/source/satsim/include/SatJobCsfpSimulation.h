//
// Created by 92304 on 2022/7/23.
//

#ifndef CSFPBASE_SATJOBCSFPSIMULATION_H
#define CSFPBASE_SATJOBCSFPSIMULATION_H

#include <vector>
#include "SatJobSimulation.h"

namespace satsim {
    class Logger;
    class Job;
    class EHSatellite;

    class SatJobCsfpSimulation : public SatJobSimulation {
    public:
        ~SatJobCsfpSimulation();

        SatJobCsfpSimulation(size_t pipelineDepth, size_t tasksPerJob, size_t gtfCount, double orbitPeriodSec);

        void run() override;

        void update(double simSecs) override;

        bool running() override;

    private:
        std::unique_ptr<Logger> logger;
        std::vector<Job *> gtfs;
        std::vector<EHSatellite *> ehsatellites;
    };
}


#endif //CSFPBASE_SATJOBCSFPSIMULATION_H
