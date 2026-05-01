#include "P2G.h"
#include "../Preparation/RebuildMapping.h"
#include "../Structures/Morton.h"
#include <cuda_runtime.h>
#include <svd3/svd3_cuda.h>

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

__global__ void P2GKernel(const ParticleBlock* particleBlocks, GridBlock* gridBlocks, const int particleCount, const HashTable& hashTable, const float cellSize, const float deltaTime, const float shearModulus, const float firstLameParameter, const float hardeningCoefficient)
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

    const float r00 = u11*v11 + u12*v12 + u13*v13;
    const float r01 = u11*v21 + u12*v22 + u13*v23;
    const float r02 = u11*v31 + u12*v32 + u13*v33;
    const float r10 = u21*v11 + u22*v12 + u23*v13;
    const float r11 = u21*v21 + u22*v22 + u23*v23;
    const float r12 = u21*v31 + u22*v32 + u23*v33;
    const float r20 = u31*v11 + u32*v12 + u33*v13;
    const float r21 = u31*v21 + u32*v22 + u33*v23;
    const float r22 = u31*v31 + u32*v32 + u33*v33;

    const float J = s11 * s22 * s33;

    const float a00 = deformationGradient[0] - r00;
    const float a01 = deformationGradient[1] - r01;
    const float a02 = deformationGradient[2] - r02;
    const float a10 = deformationGradient[3] - r10;
    const float a11 = deformationGradient[4] - r11;
    const float a12 = deformationGradient[5] - r12;
    const float a20 = deformationGradient[6] - r20;
    const float a21 = deformationGradient[7] - r21;
    const float a22 = deformationGradient[8] - r22;

    const float hardening = expf(hardeningCoefficient * (1.0f - plasticVolume));
    const float effectiveShearModulus = shearModulus * hardening;
    const float effectiveFirstLameParameter = firstLameParameter * hardening;

    const float volumetricStress = effectiveFirstLameParameter * (J - 1.0f) * J;

    const float kirchhoff00 = 2.0f * effectiveShearModulus * (a00*deformationGradient[0] + a01*deformationGradient[1] + a02*deformationGradient[2]) + volumetricStress;
    const float kirchhoff01 = 2.0f * effectiveShearModulus * (a00*deformationGradient[3] + a01*deformationGradient[4] + a02*deformationGradient[5]);
    const float kirchhoff02 = 2.0f * effectiveShearModulus * (a00*deformationGradient[6] + a01*deformationGradient[7] + a02*deformationGradient[8]);
    const float kirchhoff10 = 2.0f * effectiveShearModulus * (a10*deformationGradient[0] + a11*deformationGradient[1] + a12*deformationGradient[2]);
    const float kirchhoff11 = 2.0f * effectiveShearModulus * (a10*deformationGradient[3] + a11*deformationGradient[4] + a12*deformationGradient[5]) + volumetricStress;
    const float kirchhoff12 = 2.0f * effectiveShearModulus * (a10*deformationGradient[6] + a11*deformationGradient[7] + a12*deformationGradient[8]);
    const float kirchhoff20 = 2.0f * effectiveShearModulus * (a20*deformationGradient[0] + a21*deformationGradient[1] + a22*deformationGradient[2]);
    const float kirchhoff21 = 2.0f * effectiveShearModulus * (a20*deformationGradient[3] + a21*deformationGradient[4] + a22*deformationGradient[5]);
    const float kirchhoff22 = 2.0f * effectiveShearModulus * (a20*deformationGradient[6] + a21*deformationGradient[7] + a22*deformationGradient[8]) + volumetricStress;

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

    float weightsX[3], weightsY[3], weightsZ[3];
    ComputeBSplineWeights(fractionalX, fractionalY, fractionalZ, weightsX, weightsY, weightsZ);

    for (int neighborX = 0; neighborX < 3; neighborX++)
    {
        for (int neighborY = 0; neighborY < 3; neighborY++)
        {
            for (int neighborZ = 0; neighborZ < 3; neighborZ++)
            {
                const int nodeX = baseX + neighborX;
                const int nodeY = baseY + neighborY;
                const int nodeZ = baseZ + neighborZ;

                const float weight = weightsX[neighborX] * weightsY[neighborY] * weightsZ[neighborZ];

                const float nodeToParticleOffsetX = (static_cast<float>(nodeX) - gridPositionX) * cellSize;
                const float nodeToParticleOffsetY = (static_cast<float>(nodeY) - gridPositionY) * cellSize;
                const float nodeToParticleOffsetZ = (static_cast<float>(nodeZ) - gridPositionZ) * cellSize;

                const float stressForceX = kirchhoff00 * nodeToParticleOffsetX + kirchhoff01 * nodeToParticleOffsetY + kirchhoff02 * nodeToParticleOffsetZ;
                const float stressForceY = kirchhoff10 * nodeToParticleOffsetX + kirchhoff11 * nodeToParticleOffsetY + kirchhoff12 * nodeToParticleOffsetZ;
                const float stressForceZ = kirchhoff20 * nodeToParticleOffsetX + kirchhoff21 * nodeToParticleOffsetY + kirchhoff22 * nodeToParticleOffsetZ;

                const float momentumX = mass * (velocityX + affineMomentumMatrix[0] * nodeToParticleOffsetX + affineMomentumMatrix[1] * nodeToParticleOffsetY + affineMomentumMatrix[2] * nodeToParticleOffsetZ) + stressScale * stressForceX;
                const float momentumY = mass * (velocityY + affineMomentumMatrix[3] * nodeToParticleOffsetX + affineMomentumMatrix[4] * nodeToParticleOffsetY + affineMomentumMatrix[5] * nodeToParticleOffsetZ) + stressScale * stressForceY;
                const float momentumZ = mass * (velocityZ + affineMomentumMatrix[6] * nodeToParticleOffsetX + affineMomentumMatrix[7] * nodeToParticleOffsetY + affineMomentumMatrix[8] * nodeToParticleOffsetZ) + stressScale * stressForceZ;

                const int nodeBlockX = nodeX / blockSize;
                const int nodeBlockY = nodeY / blockSize;
                const int nodeBlockZ = nodeZ / blockSize;

                const uint64_t blockCode = MortonEncode(nodeBlockX, nodeBlockY, nodeBlockZ);
                const uint32_t blockIndex = Lookup(hashTable, blockCode);

                if (blockIndex == UINT32_MAX)
                {
                    continue;
                }

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
