#include "RebuildMapping.h"
#include "../Structures/Morton.h"
#include <cuda_runtime.h>

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
