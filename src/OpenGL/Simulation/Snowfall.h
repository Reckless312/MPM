#ifndef MPM_METHOD_SNOWFALL_H
#define MPM_METHOD_SNOWFALL_H

#include <vector>

#include <glm/vec3.hpp>

#include "CUDA/Structures/ParticleBuffer.h"

class Snowfall
{
public:
    std::vector<glm::vec3> initialPositions{};
    std::vector<ParticleBlock> initialBlocks{};

    int particleCount = 100000;

    void BuildInitialPositions();
    void BuildParticleBlocks();

private:
    glm::vec3 lowerLeftBoxCorner = glm::vec3(0.5f, 3.0f, 2.1f);
    glm::vec3 upperRightBoxCorner = glm::vec3(4.6f, 4.5f, 3.1f);

    float snowDensity = 400.0f;
    float particleVolume = 0.0f;
    float particleMass = 0.0f;
};

#endif
