#include "WritePositionsKernel.h"

__global__ void WritePositionsKernel(const ParticleBlock* particleBlocks, float* buffer, const int particleCount)
{
    const int particleIndex = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);

    if (particleIndex >= particleCount)
    {
        return;
    }

    const int particleBlockIndex = particleIndex / 32;
    const int lane = particleIndex % 32;

    buffer[particleIndex * 3 + 0] = particleBlocks[particleBlockIndex].positionX[lane];
    buffer[particleIndex * 3 + 1] = particleBlocks[particleBlockIndex].positionY[lane];
    buffer[particleIndex * 3 + 2] = particleBlocks[particleBlockIndex].positionZ[lane];
}
