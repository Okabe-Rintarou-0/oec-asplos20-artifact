//
// Created by 92304 on 2022/7/23.
//

#include "SatJobSimulation.h"


satsim::SatJobSimulation::SatJobSimulation(size_t pipelineDepth,
                                           size_t tasksPerJob,
                                           size_t gtfCount,
                                           double periodSec) :
        pipelineDepth(pipelineDepth), tasksPerJob(tasksPerJob), gtfCount(gtfCount), periodSec(periodSec),
        simulating(false) {}
