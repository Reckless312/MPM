#include "SnowVolume.h"

#include <cstring>
#include <random>

#include "Exceptions/Error.h"
#include "Exceptions/MPMException.h"
#include "SimulationConfig.h"

SnowVolume::SnowVolume(const glm::vec3 lowerLeftBoxCorner, const glm::vec3 upperRightBoxCorner, const int particleCount, const float snowDensity)
{
    this->lowerLeftBoxCorner = lowerLeftBoxCorner;
    this->upperRightBoxCorner = upperRightBoxCorner;
    this->particleCount = particleCount;
    this->snowDensity = snowDensity;
}

std::vector<glm::vec3> SnowVolume::GetInitialPositions() const
{
    return this->initialPositions;
}

std::vector<ParticleBlock> SnowVolume::GetInitialBlocks() const
{
    return this->initialBlocks;
}

int SnowVolume::GetParticleCount() const
{
    return this->particleCount;
}

int SnowVolume::PadToBlockSize(const int count)
{
    return ((count + 31) / 32) * 32;
}

void SnowVolume::BuildInitialPositions()
{
    this->particleCount = SnowVolume::PadToBlockSize(this->particleCount);

    this->particleVolume = SimulationConfig::cellSize * SimulationConfig::cellSize * SimulationConfig::cellSize;
    this->particleMass = this->snowDensity * this->particleVolume;

    std::random_device randomDevice;
    std::mt19937 randomEngine(randomDevice());

    std::uniform_real_distribution distributionX(this->lowerLeftBoxCorner.x, this->upperRightBoxCorner.x);
    std::uniform_real_distribution distributionY(this->lowerLeftBoxCorner.y, this->upperRightBoxCorner.y);
    std::uniform_real_distribution distributionZ(this->lowerLeftBoxCorner.z, this->upperRightBoxCorner.z);

    try
    {
        this->initialPositions.reserve(this->particleCount);
    }
    catch (const std::exception& exception)
    {
        throw MPMException(exception.what(), Error::PositionMemoryAllocation);
    }

    for (int particle = 0; particle < this->particleCount; particle++)
    {
        this->initialPositions.emplace_back(distributionX(randomEngine), distributionY(randomEngine), distributionZ(randomEngine));
    }
}

void SnowVolume::BuildParticleBlocks()
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

        this->initialBlocks[blockIndex].mass[lane] = this->particleMass;
        this->initialBlocks[blockIndex].volume[lane] = this->particleVolume;
        this->initialBlocks[blockIndex].plasticVolume[lane] = 1.0f;

        this->initialBlocks[blockIndex].deformationGradient[0][lane] = 1.0f;
        this->initialBlocks[blockIndex].deformationGradient[4][lane] = 1.0f;
        this->initialBlocks[blockIndex].deformationGradient[8][lane] = 1.0f;
    }
}