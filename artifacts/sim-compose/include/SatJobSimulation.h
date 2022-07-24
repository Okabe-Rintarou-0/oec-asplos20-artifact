//
// Created by 92304 on 2022/7/23.
//

#ifndef CLOSESPACED_SATJOBSIMULATION_H
#define CLOSESPACED_SATJOBSIMULATION_H

#include <cstddef>

namespace satsim {
    class SatJobSimulation {
    public:
        SatJobSimulation(size_t pipelineDepth, size_t tasksPerJob, size_t gtfCount, double orbitPeriodSec);

        virtual ~SatJobSimulation() = default;

        virtual void run() = 0;

        virtual void update(double simSecs) = 0;

        virtual bool running() = 0;

    protected:
        size_t pipelineDepth;
        size_t tasksPerJob;
        size_t gtfCount;
        double periodSec;
        bool simulating;
    };
}


#endif //CLOSESPACED_SATJOBSIMULATION_H
