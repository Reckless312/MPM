#ifndef MPM_METHOD_SIM_PARAMS_H
#define MPM_METHOD_SIM_PARAMS_H

#include <glm/vec3.hpp>

struct SimParameters
{
    float cellSize;
    float deltaTime;
    float firstLameParameter;
    float secondLameParameter;
    float hardeningCoefficient;
    float criticalCompression;
    float criticalStretch;
    float boundaryFriction;
    int gridSizeInCells;
    int maxBlocks;
    glm::vec3 boundaryVelocity;
};

#ifdef __CUDACC__
extern __constant__ SimParameters simulationParameters;
#endif

#endif
