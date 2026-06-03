#include "RegisterParticlesHomeBlocks.h"

#include "RegisterActiveBlocks.h"
#include "../Structures/Morton.h"
#include "../SimulationParameters.h"

__global__ void RegisterParticlesHomeBlocks(const ParticleBlock* particleBlocks, const int particleCount, uint64_t* particleHomeBlockCodes)
{
    const int particleIndex = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);

    if (particleIndex >= particleCount)
    {
        return;
    }

    const int particleBlockIndex = particleIndex / 32;
    const int lane = particleIndex % 32;

    const float inverseCellSize = 1.0f / simulationParameters.cellSize;

    const float gridPositionX = particleBlocks[particleBlockIndex].positionX[lane] * inverseCellSize;
    const float gridPositionY = particleBlocks[particleBlockIndex].positionY[lane] * inverseCellSize;
    const float gridPositionZ = particleBlocks[particleBlockIndex].positionZ[lane] * inverseCellSize;

    const int blockX = static_cast<int>(floorf(gridPositionX - freeZoneShift)) / blockSize;
    const int blockY = static_cast<int>(floorf(gridPositionY - freeZoneShift)) / blockSize;
    const int blockZ = static_cast<int>(floorf(gridPositionZ - freeZoneShift)) / blockSize;

    particleHomeBlockCodes[particleIndex] = MortonEncode(blockX, blockY, blockZ);
}

