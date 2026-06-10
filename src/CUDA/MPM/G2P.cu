#include "G2P.h"

#include "P2G.h"
#include "../Preparation/RegisterActiveBlocks.h"
#include "../Structures/Morton.h"
#include "../SimulationParameters.h"
#include <svd3/svd3_cuda.h>

__global__ void G2PKernel(ParticleBlock* particleBlocks, const GridBlock* gridBlocks, const int particleCount, const HashTable& blockCodeToIndex, const uint64_t* particleHomeBlockCodes, uint32_t* rebuildFlag)
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
    #pragma unroll
    for (int componentIndex = 0; componentIndex < 9; componentIndex++)
    {
        deformationGradient[componentIndex] = particleBlocks[particleBlockIndex].deformationGradient[componentIndex][lane];
    }

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

                if (nodeX < 0 || nodeY < 0 || nodeZ < 0)
                {
                    continue;
                }

                const float weight = sharedWeights[neighborX * weightStride + threadOffset] * sharedWeights[(3 + neighborY) * weightStride + threadOffset] * sharedWeights[(6 + neighborZ) * weightStride + threadOffset];

                const float particleToNodeOffsetX = (static_cast<float>(nodeX) - gridPositionX) * simulationParameters.cellSize;
                const float particleToNodeOffsetY = (static_cast<float>(nodeY) - gridPositionY) * simulationParameters.cellSize;
                const float particleToNodeOffsetZ = (static_cast<float>(nodeZ) - gridPositionZ) * simulationParameters.cellSize;

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

                const float gridVelocityX = gridBlocks[blockIndex].velocityX[nodeLane];
                const float gridVelocityY = gridBlocks[blockIndex].velocityY[nodeLane];
                const float gridVelocityZ = gridBlocks[blockIndex].velocityZ[nodeLane];

                newVelocityX += weight * gridVelocityX;
                newVelocityY += weight * gridVelocityY;
                newVelocityZ += weight * gridVelocityZ;

                // Eq. 176
                bMatrix[0] += weight * gridVelocityX * particleToNodeOffsetX;
                bMatrix[1] += weight * gridVelocityX * particleToNodeOffsetY;
                bMatrix[2] += weight * gridVelocityX * particleToNodeOffsetZ;
                bMatrix[3] += weight * gridVelocityY * particleToNodeOffsetX;
                bMatrix[4] += weight * gridVelocityY * particleToNodeOffsetY;
                bMatrix[5] += weight * gridVelocityY * particleToNodeOffsetZ;
                bMatrix[6] += weight * gridVelocityZ * particleToNodeOffsetX;
                bMatrix[7] += weight * gridVelocityZ * particleToNodeOffsetY;
                bMatrix[8] += weight * gridVelocityZ * particleToNodeOffsetZ;
            }
        }
    }

    // C = 4/dx^2 * B
    const float velocityGradientScale = 4.0f * inverseCellSize * inverseCellSize;

    float velocityGradient[9];

    #pragma unroll
    for (int componentIndex = 0; componentIndex < 9; componentIndex++)
    {
        velocityGradient[componentIndex] = velocityGradientScale * bMatrix[componentIndex];
    }

    float newDeformationGradient[9];
    #pragma unroll
    for (int row = 0; row < 3; row++)
    {
        #pragma unroll
        for (int column = 0; column < 3; column++)
        {
            float value = deformationGradient[row * 3 + column];

            #pragma unroll
            for (int contractionIndex = 0; contractionIndex < 3; contractionIndex++)
            {
                value += simulationParameters.deltaTime * velocityGradient[row * 3 + contractionIndex] * deformationGradient[contractionIndex * 3 + column];
            }

            newDeformationGradient[row * 3 + column] = value;
        }
    }

    float u[9], s[3], v[9];

    svd(newDeformationGradient[0], newDeformationGradient[1], newDeformationGradient[2],
        newDeformationGradient[3], newDeformationGradient[4], newDeformationGradient[5],
        newDeformationGradient[6], newDeformationGradient[7], newDeformationGradient[8],
        u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7], u[8],
        s[0], s[1], s[2],
        v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8]);

    float clampedS[3];
    #pragma unroll
    for (int i = 0; i < 3; i++)
    {
        clampedS[i] = fmaxf(1.0f - simulationParameters.criticalCompression, fminf(1.0f + simulationParameters.criticalStretch, s[i]));
    }

    float elasticDeformationGradient[9];
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
                value += u[row * 3 + contractionIndex] * clampedS[contractionIndex] * v[column * 3 + contractionIndex];
            }
            elasticDeformationGradient[row * 3 + column] = value;
        }
    }

    const float oldPlasticVolume = particleBlocks[particleBlockIndex].plasticVolume[lane];

    const float detFNew = s[0] * s[1] * s[2];
    const float detFElastic = clampedS[0] * clampedS[1] * clampedS[2];

    const float newPlasticVolume = oldPlasticVolume * detFNew / detFElastic;

    const float newPositionX = positionX + simulationParameters.deltaTime * newVelocityX;
    const float newPositionY = positionY + simulationParameters.deltaTime * newVelocityY;
    const float newPositionZ = positionZ + simulationParameters.deltaTime * newVelocityZ;

    const float newGridPositionX = newPositionX * inverseCellSize;
    const float newGridPositionY = newPositionY * inverseCellSize;
    const float newGridPositionZ = newPositionZ * inverseCellSize;

    const int newCellX = static_cast<int>(floorf(newGridPositionX - freeZoneShift));
    const int newCellY = static_cast<int>(floorf(newGridPositionY - freeZoneShift));
    const int newCellZ = static_cast<int>(floorf(newGridPositionZ - freeZoneShift));

    int homeBlockX, homeBlockY, homeBlockZ;
    MortonDecode(particleHomeBlockCodes[particleIndex], homeBlockX, homeBlockY, homeBlockZ);

    const bool outsideX = newCellX < (homeBlockX - 1) * blockSize || newCellX > (homeBlockX + 2) * blockSize - 3;
    const bool outsideY = newCellY < (homeBlockY - 1) * blockSize || newCellY > (homeBlockY + 2) * blockSize - 3;
    // ReSharper disable once CppTooWideScopeInitStatement
    const bool outsideZ = newCellZ < (homeBlockZ - 1) * blockSize || newCellZ > (homeBlockZ + 2) * blockSize - 3;

    if (outsideX || outsideY || outsideZ)
    {
        atomicExch(rebuildFlag, 1u);
    }

    const float gridMin = 2.0f * simulationParameters.cellSize;
    const float gridMax = static_cast<float>(simulationParameters.cellCountPerAxis - 2) * simulationParameters.cellSize;

    particleBlocks[particleBlockIndex].positionX[lane] = fmaxf(gridMin, fminf(gridMax, newPositionX));
    particleBlocks[particleBlockIndex].positionY[lane] = fmaxf(gridMin, fminf(gridMax, newPositionY));
    particleBlocks[particleBlockIndex].positionZ[lane] = fmaxf(gridMin, fminf(gridMax, newPositionZ));

    particleBlocks[particleBlockIndex].velocityX[lane] = newVelocityX;
    particleBlocks[particleBlockIndex].velocityY[lane] = newVelocityY;
    particleBlocks[particleBlockIndex].velocityZ[lane] = newVelocityZ;

    #pragma unroll
    for (int componentIndex = 0; componentIndex < 9; componentIndex++)
    {
        particleBlocks[particleBlockIndex].affineMomentumMatrix[componentIndex][lane] = velocityGradient[componentIndex];
    }

    #pragma unroll
    for (int componentIndex = 0; componentIndex < 9; componentIndex++)
    {
        particleBlocks[particleBlockIndex].deformationGradient[componentIndex][lane] = elasticDeformationGradient[componentIndex];
    }

    particleBlocks[particleBlockIndex].plasticVolume[lane] = newPlasticVolume;
}
