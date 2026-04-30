#include "Snowball.h"

#include <cmath>
#include <cstring>
#include <random>
#include <glm/ext/quaternion_geometric.hpp>

#include "OpenGL/Program.h"

float Snowball::CalculateVolume() const
{
    return 4.0f / 3.0f * Program::pi * this->snowballRadius * this->snowballRadius * this->snowballRadius;
}

void Snowball::BuildInitialPositions()
{
    this->particleVolume = this->CalculateVolume() / static_cast<float>(this->particleCount);
    this->particleMass = this->snowDensity * particleVolume;

    std::random_device randomDevice;
    std::mt19937 randomEngine(randomDevice());
    std::uniform_real_distribution distribution(-this->snowballRadius, this->snowballRadius);

    try
    {
        this->initialPositions.reserve(this->particleCount);
    }
    catch (const std::exception& exception)
    {
        throw MPMException(exception.what(), Error::PositionMemoryAllocation);
    }

    while (static_cast<int>(this->initialPositions.size()) < this->particleCount)
    {
        // ReSharper disable once CppTooWideScopeInitStatement
        const glm::vec3 offset(distribution(randomEngine), distribution(randomEngine), distribution(randomEngine));

        if (glm::length(offset) <= snowballRadius)
        {
            constexpr float snowballCenterZ = 2.56f;
            constexpr float snowballCenterY = 3.0f;
            constexpr float snowballCenterX = 2.56f;

            initialPositions.emplace_back(snowballCenterX + offset.x, snowballCenterY + offset.y, snowballCenterZ + offset.z);
        }
    }
}

void Snowball::BuildParticleBlocks()
{
    const int blockCount = (this->particleCount + 31) / 32;

    try
    {
        this->initialBlocks.resize(blockCount);
    }
    catch (const std::exception& exception)
    {
        throw MPMException(exception.what(), Error::BlocksMemoryAllocation);
    }

    std::memset(this->initialBlocks.data(), 0, blockCount * sizeof(ParticleBlock));

    for (int particleIndex = 0; particleIndex < this->particleCount; particleIndex++)
    {
        const int blockIndex = particleIndex / 32;
        const int lane = particleIndex % 32;

        this->initialBlocks[blockIndex].positionX[lane] = this->initialPositions[particleIndex].x;
        this->initialBlocks[blockIndex].positionY[lane] = this->initialPositions[particleIndex].y;
        this->initialBlocks[blockIndex].positionZ[lane] = this->initialPositions[particleIndex].z;

        this->initialBlocks[blockIndex].mass[lane] = particleMass;
        this->initialBlocks[blockIndex].volume[lane] = this->particleVolume;
        this->initialBlocks[blockIndex].plasticVolume[lane] = 1.0f;

        this->initialBlocks[blockIndex].deformationGradient[0][lane] = 1.0f;
        this->initialBlocks[blockIndex].deformationGradient[4][lane] = 1.0f;
        this->initialBlocks[blockIndex].deformationGradient[8][lane] = 1.0f;
    }
}
