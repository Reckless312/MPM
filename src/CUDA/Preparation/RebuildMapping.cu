#include "RebuildMapping.h"
#include "../Structures/Morton.h"
#include <cuda_runtime.h>

__device__ uint64_t ComputeParticleBlockCode(const float positionX, const float positionY, const float positionZ, const float cellSize)
{
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

    return MortonEncode(blockX, blockY, blockZ);
}

__global__ void RebuildMappingKernel(const ParticleBlock* particleBlocks, const int particleCount, const HashTable &hashTable, uint32_t* nextBlockIndex, uint64_t* blockCodes, const float cellSize)
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

    for (int offsetX = -1; offsetX <= 1; offsetX++)
    {
        for (int offsetY = -1; offsetY <= 1; offsetY++)
        {
            for (int offsetZ = -1; offsetZ <= 1; offsetZ++)
            {
                const uint64_t neighborBlockCode = MortonEncode(blockX + offsetX, blockY + offsetY, blockZ + offsetZ);
                Insert(hashTable, neighborBlockCode, nextBlockIndex, blockCodes);
            }
        }
    }
}

__global__ void RecordHomeBlocksKernel(const ParticleBlock* particleBlocks, const int particleCount, uint64_t* particleHomeBlockCodes, const float cellSize)
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

    particleHomeBlockCodes[particleIndex] = ComputeParticleBlockCode(positionX, positionY, positionZ, cellSize);
}

__global__ void CheckRebuildKernel(const ParticleBlock* particleBlocks, const int particleCount, const uint64_t* particleHomeBlockCodes, uint32_t* rebuildFlag, const float cellSize)
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

    int homeBlockX, homeBlockY, homeBlockZ;
    MortonDecode(particleHomeBlockCodes[particleIndex], homeBlockX, homeBlockY, homeBlockZ);

    const bool outsideX = cellX < (homeBlockX - 1) * blockSize || cellX > (homeBlockX + 2) * blockSize - 3;
    const bool outsideY = cellY < (homeBlockY - 1) * blockSize || cellY > (homeBlockY + 2) * blockSize - 3;
    // ReSharper disable once CppTooWideScopeInitStatement
    const bool outsideZ = cellZ < (homeBlockZ - 1) * blockSize || cellZ > (homeBlockZ + 2) * blockSize - 3;

    if (outsideX || outsideY || outsideZ)
    {
        atomicExch(rebuildFlag, 1u);
    }
}
