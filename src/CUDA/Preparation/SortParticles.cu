#include "SortParticles.h"
#include "RebuildMapping.h"
#include "../Structures/Morton.h"
#include "../CudaCheck.h"
#include <cuda_runtime.h>
#include <cub/cub.cuh>

__global__ void ComputeSortKeysKernel(const ParticleBlock* particleBlocks, const int particleCount, const HashTable& hashTable, uint64_t* sortKeys, const float cellSize)
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

    const int blockX = cellX / blockSize;
    const int blockY = cellY / blockSize;
    const int blockZ = cellZ / blockSize;

    const uint64_t blockCode = MortonEncode(blockX, blockY, blockZ);
    const uint32_t blockIndex = Lookup(hashTable, blockCode);

    if (blockIndex == UINT32_MAX)
    {
        printf("ComputeSortKeys miss: particle %d pos=(%.3f,%.3f,%.3f)\n",
               particleIndex, positionX, positionY, positionZ);
        sortKeys[particleIndex] = UINT64_MAX;
        return;
    }

    const auto localX = static_cast<uint32_t>(cellX % blockSize);
    const auto localY = static_cast<uint32_t>(cellY % blockSize);
    const auto localZ = static_cast<uint32_t>(cellZ % blockSize);

    const uint32_t cellCode = localX | (localY << 3) | (localZ << 6);

    sortKeys[particleIndex] = (static_cast<uint64_t>(blockIndex) << cellBits) | cellCode;
}

__global__ void ReorderParticlesKernel(const ParticleBlock* inputBlocks, ParticleBlock* outputBlocks, const uint32_t* sortedIndices, const int particleCount)
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
    outputBlocks[newParticleBlockIndex].plasticVolume[newParticleLane] = inputBlocks[oldParticleBlockIndex].plasticVolume[oldParticleLane];
}

__global__ void InitIndicesKernel(uint32_t* indices, const int particleCount)
{
    const int particleIndex = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);

    if (particleIndex >= particleCount)
    {
        return;
    }

    indices[particleIndex] = static_cast<uint32_t>(particleIndex);
}

__global__ void WarpSortKernel(ParticleBlock* particleBlocks, const int particleCount, const HashTable &hashTable, const float cellSize, uint64_t* particleHomeBlockCodes)
{
    const int particleBlockIndex = static_cast<int>(blockIdx.x);
    const int lane = static_cast<int>(threadIdx.x);

    const int particleIndex = particleBlockIndex * 32 + lane;

    const float inverseCellSize = 1.0f / cellSize;

    uint32_t key;

    if (particleIndex < particleCount)
    {
        const float positionX = particleBlocks[particleBlockIndex].positionX[lane];
        const float positionY = particleBlocks[particleBlockIndex].positionY[lane];
        const float positionZ = particleBlocks[particleBlockIndex].positionZ[lane];

        const float gridPositionX = positionX * inverseCellSize;
        const float gridPositionY = positionY * inverseCellSize;
        const float gridPositionZ = positionZ * inverseCellSize;

        const int cellX = static_cast<int>(floorf(gridPositionX - freeZoneShift));
        const int cellY = static_cast<int>(floorf(gridPositionY - freeZoneShift));
        const int cellZ = static_cast<int>(floorf(gridPositionZ - freeZoneShift));

        const int blockX = cellX / blockSize;
        const int blockY = cellY / blockSize;
        const int blockZ = cellZ / blockSize;

        const uint64_t blockCode = MortonEncode(blockX, blockY, blockZ);
        const uint32_t blockIndex = Lookup(hashTable, blockCode);

        if (blockIndex == UINT32_MAX)
        {
            printf("WarpSort miss: particle %d pos=(%.3f,%.3f,%.3f)\n",
                   particleIndex, positionX, positionY, positionZ);
            key = UINT32_MAX;
        }
        else
        {
            const auto localX = static_cast<uint32_t>(cellX % blockSize);
            const auto localY = static_cast<uint32_t>(cellY % blockSize);
            const auto localZ = static_cast<uint32_t>(cellZ % blockSize);

            const uint32_t cellCode = localX | (localY << 3) | (localZ << 6);

            key = (blockIndex << cellBits) | cellCode;
        }
    }
    else
    {
        key = UINT32_MAX;
    }

    auto sourceLane = static_cast<uint32_t>(lane);

    for (int k = 2; k <= 32; k <<= 1)
    {
        for (int j = k >> 1; j > 0; j >>= 1)
        {
            const uint32_t partnerKey = __shfl_xor_sync(0xFFFFFFFF, key, j);
            const uint32_t partnerSourceLane = __shfl_xor_sync(0xFFFFFFFF, sourceLane, j);

            const bool isLower = (lane & j) == 0;
            const bool ascending = (lane & k) == 0;
            const bool wantMin = (isLower == ascending);
            // ReSharper disable once CppTooWideScope
            const bool shouldSwap = wantMin ? (key > partnerKey) : (key < partnerKey);

            if (shouldSwap)
            {
                key = partnerKey;
                sourceLane = partnerSourceLane;
            }
        }
    }

    const float positionX = particleBlocks[particleBlockIndex].positionX[lane];
    const float positionY = particleBlocks[particleBlockIndex].positionY[lane];
    const float positionZ = particleBlocks[particleBlockIndex].positionZ[lane];
    const float velocityX = particleBlocks[particleBlockIndex].velocityX[lane];
    const float velocityY = particleBlocks[particleBlockIndex].velocityY[lane];
    const float velocityZ = particleBlocks[particleBlockIndex].velocityZ[lane];

    float deformationGradient[9];
    float affineMomentumMatrix[9];

    for (int componentIndex = 0; componentIndex < 9; componentIndex++)
    {
        deformationGradient[componentIndex] = particleBlocks[particleBlockIndex].deformationGradient[componentIndex][lane];
        affineMomentumMatrix[componentIndex] = particleBlocks[particleBlockIndex].affineMomentumMatrix[componentIndex][lane];
    }

    const float mass = particleBlocks[particleBlockIndex].mass[lane];
    const float volume = particleBlocks[particleBlockIndex].volume[lane];
    const float plasticVolume = particleBlocks[particleBlockIndex].plasticVolume[lane];

    const uint64_t homeBlockCode = particleHomeBlockCodes[particleIndex];

    const auto homeBlockCodeLow = static_cast<uint32_t>(homeBlockCode);
    const auto homeBlockCodeHigh = static_cast<uint32_t>(homeBlockCode >> 32);

    particleBlocks[particleBlockIndex].positionX[lane] = __shfl_sync(0xFFFFFFFF, positionX, static_cast<int>(sourceLane));
    particleBlocks[particleBlockIndex].positionY[lane] = __shfl_sync(0xFFFFFFFF, positionY, static_cast<int>(sourceLane));
    particleBlocks[particleBlockIndex].positionZ[lane] = __shfl_sync(0xFFFFFFFF, positionZ, static_cast<int>(sourceLane));
    particleBlocks[particleBlockIndex].velocityX[lane] = __shfl_sync(0xFFFFFFFF, velocityX, static_cast<int>(sourceLane));
    particleBlocks[particleBlockIndex].velocityY[lane] = __shfl_sync(0xFFFFFFFF, velocityY, static_cast<int>(sourceLane));
    particleBlocks[particleBlockIndex].velocityZ[lane] = __shfl_sync(0xFFFFFFFF, velocityZ, static_cast<int>(sourceLane));

    for (int componentIndex = 0; componentIndex < 9; componentIndex++)
    {
        particleBlocks[particleBlockIndex].deformationGradient[componentIndex][lane] = __shfl_sync(0xFFFFFFFF, deformationGradient[componentIndex], static_cast<int>(sourceLane));
        particleBlocks[particleBlockIndex].affineMomentumMatrix[componentIndex][lane] = __shfl_sync(0xFFFFFFFF, affineMomentumMatrix[componentIndex], static_cast<int>(sourceLane));
    }

    particleBlocks[particleBlockIndex].mass[lane] = __shfl_sync(0xFFFFFFFF, mass, static_cast<int>(sourceLane));
    particleBlocks[particleBlockIndex].volume[lane] = __shfl_sync(0xFFFFFFFF, volume, static_cast<int>(sourceLane));
    particleBlocks[particleBlockIndex].plasticVolume[lane] = __shfl_sync(0xFFFFFFFF, plasticVolume, static_cast<int>(sourceLane));

    const uint32_t sortedHomeBlockCodeLow = __shfl_sync(0xFFFFFFFF, homeBlockCodeLow, static_cast<int>(sourceLane));
    const uint32_t sortedHomeBlockCodeHigh = __shfl_sync(0xFFFFFFFF, homeBlockCodeHigh, static_cast<int>(sourceLane));
    particleHomeBlockCodes[particleIndex] = (static_cast<uint64_t>(sortedHomeBlockCodeHigh) << 32) | sortedHomeBlockCodeLow;
}

void WarpSort(ParticleBlock* particleBlocks, const int particleCount, const HashTable& hashTable, const float cellSize, uint64_t* particleHomeBlockCodes)
{
    const int particleBlockCount = (particleCount + 31) / 32;
    WarpSortKernel<<<particleBlockCount, 32>>>(particleBlocks, particleCount, hashTable, cellSize, particleHomeBlockCodes);
}

void SortParticles(const ParticleBlock* inputBlocks, ParticleBlock* outputBlocks, const HashTable& hashTable, const int particleCount, const float cellSize, uint64_t* sortKeys, uint64_t* sortKeysOut, uint32_t* indices, uint32_t* sortedIndices, void* tempStorage, size_t& tempStorageBytes)
{
    constexpr int threadsPerBlock = 256;
    const int threadBlocks = (particleCount + threadsPerBlock - 1) / threadsPerBlock;

    InitIndicesKernel<<<threadBlocks, threadsPerBlock>>>(indices, particleCount);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    ComputeSortKeysKernel<<<threadBlocks, threadsPerBlock>>>(inputBlocks, particleCount, hashTable, sortKeys, cellSize);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cub::DeviceRadixSort::SortPairs(tempStorage, tempStorageBytes, sortKeys, sortKeysOut, indices, sortedIndices, particleCount));

    ReorderParticlesKernel<<<threadBlocks, threadsPerBlock>>>(inputBlocks, outputBlocks, sortedIndices, particleCount);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
}
