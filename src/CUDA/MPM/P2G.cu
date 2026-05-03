#include "P2G.h"
#include "../Preparation/RebuildMapping.h"
#include "../Structures/Morton.h"
#include <cuda_runtime.h>
#include <svd3/svd3_cuda.h>

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

__global__ void P2GKernel(const ParticleBlock* particleBlocks, GridBlock* gridBlocks, const int particleCount, const HashTable& hashTable, const float cellSize, const float deltaTime, const float shearModulus, const float firstLameParameter, const float hardeningCoefficient, const bool shouldRecordHomeBlocks, uint64_t* particleHomeBlockCodes)
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

    if (shouldRecordHomeBlocks)
    {
        particleHomeBlockCodes[particleIndex] = ComputeParticleBlockCode(positionX, positionY, positionZ, cellSize);
    }

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

    float u11, u12, u13, u21, u22, u23, u31, u32, u33;
    float s11, s22, s33;
    float v11, v12, v13, v21, v22, v23, v31, v32, v33;

    svd(deformationGradient[0], deformationGradient[1], deformationGradient[2],
        deformationGradient[3], deformationGradient[4], deformationGradient[5],
        deformationGradient[6], deformationGradient[7], deformationGradient[8],
        u11, u12, u13, u21, u22, u23, u31, u32, u33,
        s11, s22, s33,
        v11, v12, v13, v21, v22, v23, v31, v32, v33);

    const float U[9] = { u11, u12, u13, u21, u22, u23, u31, u32, u33 };
    const float V[9] = { v11, v12, v13, v21, v22, v23, v31, v32, v33 };
    const float J = s11 * s22 * s33;

    float rotation[9];
    #pragma unroll
    for (int row = 0; row < 3; row++)
    {
        #pragma unroll
        for (int col = 0; col < 3; col++)
        {
            float sum = 0.0f;
            #pragma unroll
            for (int k = 0; k < 3; k++)
            {
                sum += U[row * 3 + k] * V[col * 3 + k];
            }
            rotation[row * 3 + col] = sum;
        }
    }

    float fMinusRotation[9];
    #pragma unroll
    for (int i = 0; i < 9; i++)
    {
        fMinusRotation[i] = deformationGradient[i] - rotation[i];
    }

    const float hardening = expf(hardeningCoefficient * (1.0f - plasticVolume));
    const float effectiveShearModulus = shearModulus * hardening;
    const float effectiveFirstLameParameter = firstLameParameter * hardening;
    const float volumetricStress = effectiveFirstLameParameter * (J - 1.0f) * J;

    float kirchhoffStress[9];
    #pragma unroll
    for (int row = 0; row < 3; row++)
    {
        #pragma unroll
        for (int col = 0; col < 3; col++)
        {
            float sum = 0.0f;
            #pragma unroll
            for (int k = 0; k < 3; k++)
            {
                sum += fMinusRotation[row * 3 + k] * deformationGradient[col * 3 + k];
            }
            kirchhoffStress[row * 3 + col] = 2.0f * effectiveShearModulus * sum + (row == col ? volumetricStress : 0.0f);
        }
    }

    const float stressScale = -4.0f / (cellSize * cellSize) * volume * deltaTime;

    const float inverseCellSize = 1.0f / cellSize;

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

    #pragma unroll
    for (int neighborX = 0; neighborX < 3; neighborX++)
    {
        #pragma unroll
        for (int neighborY = 0; neighborY < 3; neighborY++)
        {
            #pragma unroll
            for (int neighborZ = 0; neighborZ < 3; neighborZ++)
            {
                const int nodeX = baseX + neighborX;
                const int nodeY = baseY + neighborY;
                const int nodeZ = baseZ + neighborZ;

                const float weight = sharedWeights[neighborX * weightStride + threadOffset] *
                                     sharedWeights[(3 + neighborY) * weightStride + threadOffset] *
                                     sharedWeights[(6 + neighborZ) * weightStride + threadOffset];

                const float particleToNodeOffsetX = (static_cast<float>(nodeX) - gridPositionX) * cellSize;
                const float particleToNodeOffsetY = (static_cast<float>(nodeY) - gridPositionY) * cellSize;
                const float particleToNodeOffsetZ = (static_cast<float>(nodeZ) - gridPositionZ) * cellSize;

                const float stressForceX = kirchhoffStress[0] * particleToNodeOffsetX + kirchhoffStress[1] * particleToNodeOffsetY + kirchhoffStress[2] * particleToNodeOffsetZ;
                const float stressForceY = kirchhoffStress[3] * particleToNodeOffsetX + kirchhoffStress[4] * particleToNodeOffsetY + kirchhoffStress[5] * particleToNodeOffsetZ;
                const float stressForceZ = kirchhoffStress[6] * particleToNodeOffsetX + kirchhoffStress[7] * particleToNodeOffsetY + kirchhoffStress[8] * particleToNodeOffsetZ;

                const float momentumX = mass * (velocityX + affineMomentumMatrix[0] * particleToNodeOffsetX + affineMomentumMatrix[1] * particleToNodeOffsetY + affineMomentumMatrix[2] * particleToNodeOffsetZ) + stressScale * stressForceX;
                const float momentumY = mass * (velocityY + affineMomentumMatrix[3] * particleToNodeOffsetX + affineMomentumMatrix[4] * particleToNodeOffsetY + affineMomentumMatrix[5] * particleToNodeOffsetZ) + stressScale * stressForceY;
                const float momentumZ = mass * (velocityZ + affineMomentumMatrix[6] * particleToNodeOffsetX + affineMomentumMatrix[7] * particleToNodeOffsetY + affineMomentumMatrix[8] * particleToNodeOffsetZ) + stressScale * stressForceZ;

                const int nodeBlockX = nodeX / blockSize;
                const int nodeBlockY = nodeY / blockSize;
                const int nodeBlockZ = nodeZ / blockSize;

                const uint64_t blockCode = MortonEncode(nodeBlockX, nodeBlockY, nodeBlockZ);
                const uint32_t blockIndex = Lookup(hashTable, blockCode);

                const auto localX = static_cast<uint32_t>(nodeX % blockSize);
                const auto localY = static_cast<uint32_t>(nodeY % blockSize);
                const auto localZ = static_cast<uint32_t>(nodeZ % blockSize);

                const uint32_t nodeLane = localX | (localY << 3) | (localZ << 6);

                atomicAdd(&gridBlocks[blockIndex].mass[nodeLane], weight * mass);
                atomicAdd(&gridBlocks[blockIndex].velocityX[nodeLane], momentumX * weight);
                atomicAdd(&gridBlocks[blockIndex].velocityY[nodeLane], momentumY * weight);
                atomicAdd(&gridBlocks[blockIndex].velocityZ[nodeLane], momentumZ * weight);
            }
        }
    }
}
