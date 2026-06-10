#ifndef MPM_METHOD_SIM_PARAMS_H
#define MPM_METHOD_SIM_PARAMS_H

#include <glm/vec3.hpp>

struct SimulationParameters
{
    float cellSize;
    float deltaTime;
    float firstLameParameter;
    float secondLameParameter;
    float hardeningCoefficient;
    float criticalCompression;
    float criticalStretch;
    float boundaryFriction;
    int cellCountPerAxis;
    int maxBlocks;
    int cellBits;
    glm::vec3 boundaryVelocity;
};

#ifdef __CUDACC__
extern __constant__ SimulationParameters simulationParameters;
#endif

#endif
