#include <iostream>
#include "SatJobCsfpSimulation.h"


// tasksPerJob = 3072
// gtfCount = 1642
// orbitPeriodSec = 5580.0
int main(int argc, char **argv) {
    size_t pipelineDepth = 1;
    size_t tasksPerJob = 3072;
    size_t gtfCount = 1642;
    double orbitPeriodSec = 5580.0;
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

    std::cout << "pipeline depth = " << pipelineDepth << std::endl;

    satsim::SatJobCsfpSimulation simulation(
            pipelineDepth,
            tasksPerJob,
            gtfCount,
            orbitPeriodSec
    );

    simulation.run();

    while(simulation.running()) {
        simulation.update(2e-5);
    }
}