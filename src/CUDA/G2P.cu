#include "G2P.h"
#include "P2G.h"
#include "RebuildMapping.h"
#include "Morton.h"
#include <cuda_runtime.h>
#include <svd3/svd3_cuda.h>

__global__ void g2pKernel(ParticleBlock* particleBlocks, const GridBlock* gridBlocks, const int particleCount, const HashTable& hashTable, const float cellSize, const float deltaTime, const float criticalCompression, const float criticalStretch)
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

    float deformationGradient[9];
    for (int componentIndex = 0; componentIndex < 9; componentIndex++)
    {
        deformationGradient[componentIndex] = particleBlocks[particleBlockIndex].deformationGradient[componentIndex][lane];
    }

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
    computeBSplineWeights(fractionalX, fractionalY, fractionalZ, weightsX, weightsY, weightsZ);

    float newVelocityX = 0.0f, newVelocityY = 0.0f, newVelocityZ = 0.0f;
    float bMatrix[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

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

                const int nodeBlockX = nodeX >> 3;
                const int nodeBlockY = nodeY >> 3;
                const int nodeBlockZ = nodeZ >> 3;

                const uint64_t blockCode = mortonEncode(nodeBlockX, nodeBlockY, nodeBlockZ);
                const uint32_t blockIndex = lookup(hashTable, blockCode);

                if (blockIndex == UINT32_MAX)
                {
                    continue;
                }

                const auto localX = static_cast<uint32_t>(nodeX & 7);
                const auto localY = static_cast<uint32_t>(nodeY & 7);
                const auto localZ = static_cast<uint32_t>(nodeZ & 7);
                const uint32_t nodeLane = localX | (localY << 3) | (localZ << 6);

                const float gridVelocityX = gridBlocks[blockIndex].velocityX[nodeLane];
                const float gridVelocityY = gridBlocks[blockIndex].velocityY[nodeLane];
                const float gridVelocityZ = gridBlocks[blockIndex].velocityZ[nodeLane];

                newVelocityX += weight * gridVelocityX;
                newVelocityY += weight * gridVelocityY;
                newVelocityZ += weight * gridVelocityZ;

                // B matrix: outer product of grid velocity and node-to-particle offset
                bMatrix[0] += weight * gridVelocityX * nodeToParticleOffsetX;
                bMatrix[1] += weight * gridVelocityX * nodeToParticleOffsetY;
                bMatrix[2] += weight * gridVelocityX * nodeToParticleOffsetZ;
                bMatrix[3] += weight * gridVelocityY * nodeToParticleOffsetX;
                bMatrix[4] += weight * gridVelocityY * nodeToParticleOffsetY;
                bMatrix[5] += weight * gridVelocityY * nodeToParticleOffsetZ;
                bMatrix[6] += weight * gridVelocityZ * nodeToParticleOffsetX;
                bMatrix[7] += weight * gridVelocityZ * nodeToParticleOffsetY;
                bMatrix[8] += weight * gridVelocityZ * nodeToParticleOffsetZ;
            }
        }
    }

    // Velocity gradient: C = 4/dx^2 * B
    const float velocityGradientScale = 4.0f * inverseCellSize * inverseCellSize;

    float velocityGradient[9];
    for (int componentIndex = 0; componentIndex < 9; componentIndex++)
    {
        velocityGradient[componentIndex] = velocityGradientScale * bMatrix[componentIndex];
    }

    // Deformation gradient update: F_new = (I + dt * C) * F_old
    float newDeformationGradient[9];
    for (int row = 0; row < 3; row++)
    {
        for (int col = 0; col < 3; col++)
        {
            float value = deformationGradient[row * 3 + col];

            for (int k = 0; k < 3; k++)
            {
                value += deltaTime * velocityGradient[row * 3 + k] * deformationGradient[k * 3 + col];
            }

            newDeformationGradient[row * 3 + col] = value;
        }
    }

    // Plasticity projection: SVD of F_new, clamp singular values, reconstruct F_E
    float u11, u12, u13, u21, u22, u23, u31, u32, u33;
    float s11, s22, s33;
    float v11, v12, v13, v21, v22, v23, v31, v32, v33;

    svd(newDeformationGradient[0], newDeformationGradient[1], newDeformationGradient[2],
        newDeformationGradient[3], newDeformationGradient[4], newDeformationGradient[5],
        newDeformationGradient[6], newDeformationGradient[7], newDeformationGradient[8],
        u11, u12, u13, u21, u22, u23, u31, u32, u33,
        s11, s22, s33,
        v11, v12, v13, v21, v22, v23, v31, v32, v33);

    const float clampedS11 = fmaxf(1.0f - criticalCompression, fminf(1.0f + criticalStretch, s11));
    const float clampedS22 = fmaxf(1.0f - criticalCompression, fminf(1.0f + criticalStretch, s22));
    const float clampedS33 = fmaxf(1.0f - criticalCompression, fminf(1.0f + criticalStretch, s33));

    // F_E = U * S_clamped * V^T
    const float elasticDeformationGradient00 = u11*clampedS11*v11 + u12*clampedS22*v12 + u13*clampedS33*v13;
    const float elasticDeformationGradient01 = u11*clampedS11*v21 + u12*clampedS22*v22 + u13*clampedS33*v23;
    const float elasticDeformationGradient02 = u11*clampedS11*v31 + u12*clampedS22*v32 + u13*clampedS33*v33;
    const float elasticDeformationGradient10 = u21*clampedS11*v11 + u22*clampedS22*v12 + u23*clampedS33*v13;
    const float elasticDeformationGradient11 = u21*clampedS11*v21 + u22*clampedS22*v22 + u23*clampedS33*v23;
    const float elasticDeformationGradient12 = u21*clampedS11*v31 + u22*clampedS22*v32 + u23*clampedS33*v33;
    const float elasticDeformationGradient20 = u31*clampedS11*v11 + u32*clampedS22*v12 + u33*clampedS33*v13;
    const float elasticDeformationGradient21 = u31*clampedS11*v21 + u32*clampedS22*v22 + u33*clampedS33*v23;
    const float elasticDeformationGradient22 = u31*clampedS11*v31 + u32*clampedS22*v32 + u33*clampedS33*v33;

    // J_P update: J_P_new = J_P_old * det(F_new) / det(F_E_new)
    const float oldPlasticVolume = particleBlocks[particleBlockIndex].plasticVolume[lane];
    const float detFNew = s11 * s22 * s33;
    const float detFElastic = clampedS11 * clampedS22 * clampedS33;
    const float newPlasticVolume = oldPlasticVolume * detFNew / detFElastic;

    // Write back
    particleBlocks[particleBlockIndex].positionX[lane] = positionX + deltaTime * newVelocityX;
    particleBlocks[particleBlockIndex].positionY[lane] = positionY + deltaTime * newVelocityY;
    particleBlocks[particleBlockIndex].positionZ[lane] = positionZ + deltaTime * newVelocityZ;

    particleBlocks[particleBlockIndex].velocityX[lane] = newVelocityX;
    particleBlocks[particleBlockIndex].velocityY[lane] = newVelocityY;
    particleBlocks[particleBlockIndex].velocityZ[lane] = newVelocityZ;

    for (int componentIndex = 0; componentIndex < 9; componentIndex++)
    {
        particleBlocks[particleBlockIndex].affineMomentumMatrix[componentIndex][lane] = velocityGradient[componentIndex];
    }

    particleBlocks[particleBlockIndex].deformationGradient[0][lane] = elasticDeformationGradient00;
    particleBlocks[particleBlockIndex].deformationGradient[1][lane] = elasticDeformationGradient01;
    particleBlocks[particleBlockIndex].deformationGradient[2][lane] = elasticDeformationGradient02;
    particleBlocks[particleBlockIndex].deformationGradient[3][lane] = elasticDeformationGradient10;
    particleBlocks[particleBlockIndex].deformationGradient[4][lane] = elasticDeformationGradient11;
    particleBlocks[particleBlockIndex].deformationGradient[5][lane] = elasticDeformationGradient12;
    particleBlocks[particleBlockIndex].deformationGradient[6][lane] = elasticDeformationGradient20;
    particleBlocks[particleBlockIndex].deformationGradient[7][lane] = elasticDeformationGradient21;
    particleBlocks[particleBlockIndex].deformationGradient[8][lane] = elasticDeformationGradient22;
    particleBlocks[particleBlockIndex].plasticVolume[lane] = newPlasticVolume;
}
