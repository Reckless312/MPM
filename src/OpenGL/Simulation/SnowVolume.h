#ifndef MPM_METHOD_SNOW_VOLUME_H
#define MPM_METHOD_SNOW_VOLUME_H

#include <vector>

#include <glm/vec3.hpp>

#include "CUDA/Structures/ParticleBuffer.h"

class SnowVolume
{
public:
    SnowVolume(glm::vec3 lowerLeftBoxCorner, glm::vec3 upperRightBoxCorner, int particleCount, float snowDensity);

    [[nodiscard]] std::vector<glm::vec3> GetInitialPositions() const;
    [[nodiscard]] std::vector<ParticleBlock> GetInitialBlocks() const;

    [[nodiscard]] int GetParticleCount() const;

    [[nodiscard]] static int PadToBlockSize(int count);

    void BuildInitialPositions();
    void BuildParticleBlocks();

private:
    glm::vec3 lowerLeftBoxCorner{};
    glm::vec3 upperRightBoxCorner{};

    std::vector<glm::vec3> initialPositions{};
    std::vector<ParticleBlock> initialBlocks{};

    float particleVolume = 0.0f;
    float particleMass = 0.0f;
    float snowDensity;

    int particleCount;
};

#endif
