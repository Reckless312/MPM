#include "P2G.h"

#include "../Preparation/RegisterActiveBlocks.h"
#include "../Structures/Morton.h"
#include "../SimulationParameters.h"
#include <svd3/svd3_cuda.h>

__global__ void P2GKernel(const ParticleBlock* particleBlocks, GridBlock* gridBlocks, const int particleCount, const HashTable& blockCodeToIndex)
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

    const float velocityX = particleBlocks[particleBlockIndex].velocityX[lane];
    const float velocityY = particleBlocks[particleBlockIndex].velocityY[lane];
    const float velocityZ = particleBlocks[particleBlockIndex].velocityZ[lane];

    const float mass = particleBlocks[particleBlockIndex].mass[lane];
    const float volume = particleBlocks[particleBlockIndex].volume[lane];
    const float plasticVolume = particleBlocks[particleBlockIndex].plasticVolume[lane];

    float affineMomentumMatrix[9];
    float deformationGradient[9];

    #pragma unroll
    for (int componentIndex = 0; componentIndex < 9; componentIndex++)
    {
        affineMomentumMatrix[componentIndex] = particleBlocks[particleBlockIndex].affineMomentumMatrix[componentIndex][lane];
        deformationGradient[componentIndex] = particleBlocks[particleBlockIndex].deformationGradient[componentIndex][lane];
    }

    float u[9], s[3], v[9];

    svd(deformationGradient[0], deformationGradient[1], deformationGradient[2],
        deformationGradient[3], deformationGradient[4], deformationGradient[5],
        deformationGradient[6], deformationGradient[7], deformationGradient[8],
        u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7], u[8],
        s[0], s[1], s[2],
        v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8]);

    float r[9];
    #pragma unroll
    for (int row = 0; row < 3; row++)
    {
        #pragma unroll
        for (int column = 0; column < 3; column++)
        {
            float value = 0.0f;
            #pragma unroll
            for (int contractionIndex = 0; contractionIndex < 3; contractionIndex++)
            {
                value += u[row * 3 + contractionIndex] * v[column * 3 + contractionIndex];
            }
            r[row * 3 + column] = value;
        }
    }

    const float J = s[0] * s[1] * s[2];

    float a[9];
    #pragma unroll
    for (int componentIndex = 0; componentIndex < 9; componentIndex++)
    {
        a[componentIndex] = deformationGradient[componentIndex] - r[componentIndex];
    }

    const float hardening = expf(fmaxf(-10.0f, fminf(simulationParameters.hardeningCoefficient * (1.0f - plasticVolume), 10.0f)));
    const float effectiveShearModulus = simulationParameters.secondLameParameter * hardening;
    const float effectiveFirstLameParameter = simulationParameters.firstLameParameter * hardening;
    const float volumetricStress = effectiveFirstLameParameter * (J - 1.0f) * J;

    float kirchhoff[9];
    #pragma unroll
    for (int row = 0; row < 3; row++)
    {
        #pragma unroll
        for (int column = 0; column < 3; column++)
        {
            float value = 0.0f;
            #pragma unroll
            for (int contractionIndex = 0; contractionIndex < 3; contractionIndex++)
            {
                value += a[row * 3 + contractionIndex] * deformationGradient[column * 3 + contractionIndex];
            }
            kirchhoff[row * 3 + column] = 2.0f * effectiveShearModulus * value + (row == column ? volumetricStress : 0.0f);
        }
    }

    const float stressScale = -4.0f / (simulationParameters.cellSize * simulationParameters.cellSize) * volume * simulationParameters.deltaTime;

    const float inverseCellSize = 1.0f / simulationParameters.cellSize;

    const float gridPositionX = positionX * inverseCellSize;
    const float gridPositionY = positionY * inverseCellSize;
    const float gridPositionZ = positionZ * inverseCellSize;

    const int baseX = static_cast<int>(floorf(gridPositionX - freeZoneShift));
    const int baseY = static_cast<int>(floorf(gridPositionY - freeZoneShift));
    const int baseZ = static_cast<int>(floorf(gridPositionZ - freeZoneShift));

    const float fractionalX = gridPositionX - static_cast<float>(baseX);
    const float fractionalY = gridPositionY - static_cast<float>(baseY);
    const float fractionalZ = gridPositionZ - static_cast<float>(baseZ);

    extern __shared__ float sharedWeights[];

    const int weightStride = static_cast<int>(blockDim.x);
    const int threadOffset = static_cast<int>(threadIdx.x);

    float weightsX[3], weightsY[3], weightsZ[3];
    ComputeBSplineWeights(fractionalX, fractionalY, fractionalZ, weightsX, weightsY, weightsZ);

    sharedWeights[0 * weightStride + threadOffset] = weightsX[0];
    sharedWeights[1 * weightStride + threadOffset] = weightsX[1];
    sharedWeights[2 * weightStride + threadOffset] = weightsX[2];
    sharedWeights[3 * weightStride + threadOffset] = weightsY[0];
    sharedWeights[4 * weightStride + threadOffset] = weightsY[1];
    sharedWeights[5 * weightStride + threadOffset] = weightsY[2];
    sharedWeights[6 * weightStride + threadOffset] = weightsZ[0];
    sharedWeights[7 * weightStride + threadOffset] = weightsZ[1];
    sharedWeights[8 * weightStride + threadOffset] = weightsZ[2];

    for (int neighborX = 0; neighborX < 3; neighborX++)
    {
        for (int neighborY = 0; neighborY < 3; neighborY++)
        {
            for (int neighborZ = 0; neighborZ < 3; neighborZ++)
            {
                const int nodeX = baseX + neighborX;
                const int nodeY = baseY + neighborY;
                const int nodeZ = baseZ + neighborZ;

                if (nodeX < 0 || nodeY < 0 || nodeZ < 0)
                {
                    continue;
                }

                const float weight = sharedWeights[neighborX * weightStride + threadOffset] * sharedWeights[(3 + neighborY) * weightStride + threadOffset] * sharedWeights[(6 + neighborZ) * weightStride + threadOffset];

                const float particleToNodeOffsetX = (static_cast<float>(nodeX) - gridPositionX) * simulationParameters.cellSize;
                const float particleToNodeOffsetY = (static_cast<float>(nodeY) - gridPositionY) * simulationParameters.cellSize;
                const float particleToNodeOffsetZ = (static_cast<float>(nodeZ) - gridPositionZ) * simulationParameters.cellSize;

                const float stressForceX = kirchhoff[0] * particleToNodeOffsetX + kirchhoff[1] * particleToNodeOffsetY + kirchhoff[2] * particleToNodeOffsetZ;
                const float stressForceY = kirchhoff[3] * particleToNodeOffsetX + kirchhoff[4] * particleToNodeOffsetY + kirchhoff[5] * particleToNodeOffsetZ;
                const float stressForceZ = kirchhoff[6] * particleToNodeOffsetX + kirchhoff[7] * particleToNodeOffsetY + kirchhoff[8] * particleToNodeOffsetZ;

                const float momentumX = mass * (velocityX + affineMomentumMatrix[0] * particleToNodeOffsetX + affineMomentumMatrix[1] * particleToNodeOffsetY + affineMomentumMatrix[2] * particleToNodeOffsetZ) + stressScale * stressForceX;
                const float momentumY = mass * (velocityY + affineMomentumMatrix[3] * particleToNodeOffsetX + affineMomentumMatrix[4] * particleToNodeOffsetY + affineMomentumMatrix[5] * particleToNodeOffsetZ) + stressScale * stressForceY;
                const float momentumZ = mass * (velocityZ + affineMomentumMatrix[6] * particleToNodeOffsetX + affineMomentumMatrix[7] * particleToNodeOffsetY + affineMomentumMatrix[8] * particleToNodeOffsetZ) + stressScale * stressForceZ;

                const int nodeBlockX = nodeX / blockSize;
                const int nodeBlockY = nodeY / blockSize;
                const int nodeBlockZ = nodeZ / blockSize;

                const uint64_t blockCode = MortonEncode(nodeBlockX, nodeBlockY, nodeBlockZ);
                const uint32_t blockIndex = Lookup(blockCodeToIndex, blockCode);

                if (blockIndex == BLOCK_NOT_FOUND)
                {
                    continue;
                }

                const auto localX = static_cast<uint32_t>(nodeX % blockSize);
                const auto localY = static_cast<uint32_t>(nodeY % blockSize);
                const auto localZ = static_cast<uint32_t>(nodeZ % blockSize);

                const uint32_t nodeLane = localX + localY * blockSize + localZ * blockSize * blockSize;

                atomicAdd(&gridBlocks[blockIndex].mass[nodeLane], weight * mass);
                atomicAdd(&gridBlocks[blockIndex].velocityX[nodeLane], momentumX * weight);
                atomicAdd(&gridBlocks[blockIndex].velocityY[nodeLane], momentumY * weight);
                atomicAdd(&gridBlocks[blockIndex].velocityZ[nodeLane], momentumZ * weight);
            }
        }
    }
}

__device__ void ComputeBSplineWeights(const float fractionalX, const float fractionalY, const float fractionalZ, float weightsX[3], float weightsY[3], float weightsZ[3])
{
    weightsX[0] = 0.5f * (1.5f - fractionalX) * (1.5f - fractionalX);
    weightsX[1] = 0.75f - (fractionalX - 1.0f) * (fractionalX - 1.0f);
    weightsX[2] = 0.5f * (fractionalX - 0.5f) * (fractionalX - 0.5f);

    weightsY[0] = 0.5f * (1.5f - fractionalY) * (1.5f - fractionalY);
    weightsY[1] = 0.75f - (fractionalY - 1.0f) * (fractionalY - 1.0f);
    weightsY[2] = 0.5f * (fractionalY - 0.5f) * (fractionalY - 0.5f);

    weightsZ[0] = 0.5f * (1.5f - fractionalZ) * (1.5f - fractionalZ);
    weightsZ[1] = 0.75f - (fractionalZ - 1.0f) * (fractionalZ - 1.0f);
    weightsZ[2] = 0.5f * (fractionalZ - 0.5f) * (fractionalZ - 0.5f);
}