#ifndef MPM_METHOD_SNOWBALL_H
#define MPM_METHOD_SNOWBALL_H
#include <vector>
#include <glm/vec3.hpp>

#include "CUDA/Structures/ParticleBuffer.h"

class Snowball
{
public:
    std::vector<glm::vec3> initialPositions{};
    std::vector<ParticleBlock> initialBlocks{};

    int particleCount = 200000;

    void BuildInitialPositions();
    void BuildParticleBlocks();

private:
    float snowballRadius = 0.5f;
    float snowDensity = 400.0f;

    float particleVolume = 0.0f;
    float particleMass = 0.0f;

    [[nodiscard]] float CalculateVolume() const;
};

#endif