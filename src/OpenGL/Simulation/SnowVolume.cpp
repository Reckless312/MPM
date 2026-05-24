#include "SnowVolume.h"

#include <cstring>
#include <random>

#include "Exceptions/Error.h"
#include "Exceptions/MPMException.h"

SnowVolume::SnowVolume(const glm::vec3 lowerLeft, const glm::vec3 upperRight, const int particleCount)
{
    this->lowerLeftBoxCorner = lowerLeft;
    this->upperRightBoxCorner = upperRight;
    this->particleCount = particleCount;
}

void SnowVolume::BuildInitialPositions()
{
    const glm::vec3 boxDiagonal = upperRightBoxCorner - lowerLeftBoxCorner;
    const float boxVolume = boxDiagonal.x * boxDiagonal.y * boxDiagonal.z;

    this->particleVolume = boxVolume / static_cast<float>(this->particleCount);
    this->particleMass = this->snowDensity * this->particleVolume;

    std::random_device randomDevice;
    std::mt19937 randomEngine(randomDevice());

    std::uniform_real_distribution distributionX(lowerLeftBoxCorner.x, upperRightBoxCorner.x);
    std::uniform_real_distribution distributionY(lowerLeftBoxCorner.y, upperRightBoxCorner.y);
    std::uniform_real_distribution distributionZ(lowerLeftBoxCorner.z, upperRightBoxCorner.z);

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
        initialPositions.emplace_back(distributionX(randomEngine), distributionY(randomEngine), distributionZ(randomEngine));
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
