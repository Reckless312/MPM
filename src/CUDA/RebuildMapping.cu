#include "RebuildMapping.h"
#include <cuda_runtime.h>

constexpr uint64_t twentyOneBitMask    = 0x1fffffull;
constexpr uint64_t separateHalvesMask  = 0x1f00000000ffffull;
constexpr uint64_t separateBytesMask   = 0x1f0000ff0000ffull;
constexpr uint64_t separateNibblesMask = 0x100f00f00f00f00full;
constexpr uint64_t separatePairsMask   = 0x10c30c30c30c30c3ull;
constexpr uint64_t everyThirdBitMask   = 0x1249249249249249ull;

__device__ static uint64_t expandBits(const uint32_t value)
{
    uint64_t expanded = value & twentyOneBitMask;

    expanded = (expanded | expanded << 32) & separateHalvesMask;
    expanded = (expanded | expanded << 16) & separateBytesMask;
    expanded = (expanded | expanded << 8)  & separateNibblesMask;
    expanded = (expanded | expanded << 4)  & separatePairsMask;
    expanded = (expanded | expanded << 2)  & everyThirdBitMask;

    return expanded;
}

__device__ static uint64_t mortonEncode(const int x, const int y, const int z)
{
    return expandBits(static_cast<uint32_t>(x)) | (expandBits(static_cast<uint32_t>(y)) << 1) | (expandBits(static_cast<uint32_t>(z)) << 2);
}

__global__ void rebuildMappingKernel(const ParticleBlock* particleBlocks, const int particleCount, const HashTable &hashTable, uint32_t* nextBlockIndex, const float cellSize)
{
    const int particleIndex = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);

    if (particleIndex >= particleCount)
    {
        return;
    }

    const int particleBlockIndex = particleIndex / 32;
    const int lane = particleIndex % 32;

    const float positionX = particleBlocks[particleBlockIndex].x[lane];
    const float positionY = particleBlocks[particleBlockIndex].y[lane];
    const float positionZ = particleBlocks[particleBlockIndex].z[lane];

    const float inverseCellSize = 1.0f / cellSize;

    const int cellX = static_cast<int>(floorf(positionX * inverseCellSize - 0.5f));
    const int cellY = static_cast<int>(floorf(positionY * inverseCellSize - 0.5f));
    const int cellZ = static_cast<int>(floorf(positionZ * inverseCellSize - 0.5f));

    const int blockX = cellX >> 3;
    const int blockY = cellY >> 3;
    const int blockZ = cellZ >> 3;

    for (int offsetX = -1; offsetX <= 1; offsetX++)
    {
        for (int offsetY = -1; offsetY <= 1; offsetY++)
        {
            for (int offsetZ = -1; offsetZ <= 1; offsetZ++)
            {
                const uint64_t neighborBlockCode = mortonEncode(blockX + offsetX, blockY + offsetY, blockZ + offsetZ);
                insert(hashTable, neighborBlockCode, nextBlockIndex);
            }
        }
    }
}
