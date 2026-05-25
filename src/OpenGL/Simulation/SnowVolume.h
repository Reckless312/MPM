#ifndef MPM_METHOD_SNOW_VOLUME_H
#define MPM_METHOD_SNOW_VOLUME_H

#include <vector>

#include <glm/vec3.hpp>

#include "CUDA/Structures/ParticleBuffer.h"

class SnowVolume
{
public:
    std::vector<glm::vec3> initialPositions{};
    std::vector<ParticleBlock> initialBlocks{};

    int particleCount;

    SnowVolume(glm::vec3 lowerLeftBoxCorner, glm::vec3 upperRightBoxCorner, int particleCount);

    void BuildInitialPositions();
    void BuildParticleBlocks();

private:
    glm::vec3 lowerLeftBoxCorner;
    glm::vec3 upperRightBoxCorner;

    float snowDensity = 400.0f;
    float particleVolume = 0.0f;
    float particleMass = 0.0f;
public:
    float GetParticleVolume() const { return particleVolume; }
    float GetParticleMass() const { return particleMass; }
};

#endif
