#include "SortParticles.h"
#include "RebuildMapping.h"
#include "Morton.h"
#include <cuda_runtime.h>
#include <cub/cub.cuh>

__global__ void computeSortKeysKernel(const ParticleBlock* particleBlocks, const int particleCount, const HashTable& hashTable, uint64_t* sortKeys, const float cellSize)
{
    const int particleIndex = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);

    if (particleIndex >= particleCount)
    {
        return;
    }

    const int particleBlockIndex = particleIndex / 32;
    const int lane = particleIndex % 32;

    const float positionX = particleBlocks[particleBlockIndex].positionX[lane];
    const float positionY = particleBlocks[particleBlockIndex].positionY[lane];
    const float positionZ = particleBlocks[particleBlockIndex].positionZ[lane];

    const float inverseCellSize = 1.0f / cellSize;

    const float gridPositionX = positionX * inverseCellSize;
    const float gridPositionY = positionY * inverseCellSize;
    const float gridPositionZ = positionZ * inverseCellSize;

    const int cellX = static_cast<int>(floorf(gridPositionX - freeZoneShift));
    const int cellY = static_cast<int>(floorf(gridPositionY - freeZoneShift));
    const int cellZ = static_cast<int>(floorf(gridPositionZ - freeZoneShift));

    const int blockX = cellX >> 3;
    const int blockY = cellY >> 3;
    const int blockZ = cellZ >> 3;

    const uint64_t blockCode = mortonEncode(blockX, blockY, blockZ);
    const uint32_t blockIndex = lookup(hashTable, blockCode);

    const auto localX = static_cast<uint32_t>(cellX & 7);
    const auto localY = static_cast<uint32_t>(cellY & 7);
    const auto localZ = static_cast<uint32_t>(cellZ & 7);

    const uint32_t cellCode = localX | (localY << 3) | (localZ << 6);

    sortKeys[particleIndex] = (static_cast<uint64_t>(blockIndex) << cellBits) | cellCode;
}

__global__ void reorderParticlesKernel(const ParticleBlock* inputBlocks, ParticleBlock* outputBlocks, const uint32_t* sortedIndices, const int particleCount)
{
    const int newIndex = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);

    if (newIndex >= particleCount)
    {
        return;
    }

    const int oldIndex = static_cast<int>(sortedIndices[newIndex]);

    const int oldParticleBlockIndex = oldIndex / 32;
    const int oldParticleLane = oldIndex % 32;
    const int newParticleBlockIndex = newIndex / 32;
    const int newParticleLane = newIndex % 32;

    outputBlocks[newParticleBlockIndex].positionX[newParticleLane] = inputBlocks[oldParticleBlockIndex].positionX[oldParticleLane];
    outputBlocks[newParticleBlockIndex].positionY[newParticleLane] = inputBlocks[oldParticleBlockIndex].positionY[oldParticleLane];
    outputBlocks[newParticleBlockIndex].positionZ[newParticleLane] = inputBlocks[oldParticleBlockIndex].positionZ[oldParticleLane];

    outputBlocks[newParticleBlockIndex].velocityX[newParticleLane] = inputBlocks[oldParticleBlockIndex].velocityX[oldParticleLane];
    outputBlocks[newParticleBlockIndex].velocityY[newParticleLane] = inputBlocks[oldParticleBlockIndex].velocityY[oldParticleLane];
    outputBlocks[newParticleBlockIndex].velocityZ[newParticleLane] = inputBlocks[oldParticleBlockIndex].velocityZ[oldParticleLane];

    for (int componentIndex = 0; componentIndex < 9; componentIndex++)
    {
        outputBlocks[newParticleBlockIndex].deformationGradient[componentIndex][newParticleLane] = inputBlocks[oldParticleBlockIndex].deformationGradient[componentIndex][oldParticleLane];
        outputBlocks[newParticleBlockIndex].affineMomentumMatrix[componentIndex][newParticleLane] = inputBlocks[oldParticleBlockIndex].affineMomentumMatrix[componentIndex][oldParticleLane];
    }

    outputBlocks[newParticleBlockIndex].mass[newParticleLane] = inputBlocks[oldParticleBlockIndex].mass[oldParticleLane];
    outputBlocks[newParticleBlockIndex].volume[newParticleLane] = inputBlocks[oldParticleBlockIndex].volume[oldParticleLane];
}

__global__ void initIndicesKernel(uint32_t* indices, const int particleCount)
{
    const int particleIndex = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);

    if (particleIndex >= particleCount)
    {
        return;
    }

    indices[particleIndex] = static_cast<uint32_t>(particleIndex);
}

void sortParticles(const ParticleBlock* inputBlocks, ParticleBlock* outputBlocks, const HashTable& hashTable, const int particleCount, const float cellSize, uint64_t* sortKeys, uint64_t* sortKeysOut, uint32_t* indices, uint32_t* sortedIndices, void* tempStorage, size_t& tempStorageBytes)
{
    constexpr int threadsPerBlock = 256;
    const int threadBlocks = (particleCount + threadsPerBlock - 1) / threadsPerBlock;

    initIndicesKernel<<<threadBlocks, threadsPerBlock>>>(indices, particleCount);
    computeSortKeysKernel<<<threadBlocks, threadsPerBlock>>>(inputBlocks, particleCount, hashTable, sortKeys, cellSize);

    cub::DeviceRadixSort::SortPairs(tempStorage, tempStorageBytes, sortKeys, sortKeysOut, indices, sortedIndices, particleCount);

    reorderParticlesKernel<<<threadBlocks, threadsPerBlock>>>(inputBlocks, outputBlocks, sortedIndices, particleCount);
}
