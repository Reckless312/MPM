#ifndef MPM_METHOD_SNOW_LAYER_H
#define MPM_METHOD_SNOW_LAYER_H

#include <vector>

#include <glm/vec3.hpp>

#include "CUDA/Structures/ParticleBuffer.h"

class SnowLayer
{
public:
    std::vector<glm::vec3> initialPositions{};
    std::vector<ParticleBlock> initialBlocks{};

    int particleCount = 100000;

    void BuildInitialPositions();
    void BuildParticleBlocks();

private:
    glm::vec3 lowerLeftBoxCorner = glm::vec3(1.5f, 0.04f, 1.5f);
    glm::vec3 upperRightBoxCorner = glm::vec3(3.6f, 0.2f, 3.6f);

    float snowDensity = 400.0f;
    float particleVolume = 0.0f;
    float particleMass = 0.0f;
};

#endif
