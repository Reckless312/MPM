#include "Snowfall.h"

#include <cstring>
#include <random>

#include "Exceptions/Error.h"
#include "Exceptions/MPMException.h"

void Snowfall::BuildInitialPositions()
{
    glm::vec3 spawnSize = spawnMax - spawnMin;
    float spawnVolume = spawnSize.x * spawnSize.y * spawnSize.z;

    this->particleVolume = spawnVolume / static_cast<float>(this->particleCount);
    this->particleMass = this->snowDensity * this->particleVolume;

    std::random_device randomDevice;
    std::mt19937 randomEngine(randomDevice());

    std::uniform_real_distribution distributionX(spawnMin.x, spawnMax.x);
    std::uniform_real_distribution distributionY(spawnMin.y, spawnMax.y);
    std::uniform_real_distribution distributionZ(spawnMin.z, spawnMax.z);

    try
    {
        this->initialPositions.reserve(this->particleCount);
    }
    catch (const std::exception& exception)
    {
        throw MPMException(exception.what(), Error::PositionMemoryAllocation);
    }

    for (int i = 0; i < this->particleCount; i++)
    {
        initialPositions.emplace_back(distributionX(randomEngine), distributionY(randomEngine), distributionZ(randomEngine));
    }
}

void Snowfall::BuildParticleBlocks()
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
