#ifndef MPM_METHOD_SNOWVOLUME_H
#define MPM_METHOD_SNOWVOLUME_H

#include <vector>

#include <glm/vec3.hpp>

#include "CUDA/Structures/ParticleBuffer.h"

class SnowVolume
{
public:
    std::vector<glm::vec3> initialPositions{};
    std::vector<ParticleBlock> initialBlocks{};

    int particleCount;

    SnowVolume(glm::vec3 lowerLeft, glm::vec3 upperRight, int particleCount);

    void BuildInitialPositions();
    void BuildParticleBlocks();

private:
    glm::vec3 lowerLeftBoxCorner{};
    glm::vec3 upperRightBoxCorner{};

    float snowDensity = 400.0f;
    float particleVolume = 0.0f;
    float particleMass = 0.0f;
};

#endif
