#include "UpdateGrid.h"
#include "../Structures/Morton.h"
#include "../Preparation/RebuildMapping.h"
#include "../SimParameters.h"

__global__ void UpdateGridKernel(GridBlock* gridBlocks, const uint64_t* blockCodes, const float* sdfDistances, const glm::vec3* sdfNormals)
{
    const int nodeIndex = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);

    if (nodeIndex >= simulationParameters.maxBlocks * nodesPerBlock)
    {
        return;
    }

    const int gridBlockIndex = nodeIndex / nodesPerBlock;
    const int nodeLane = nodeIndex % nodesPerBlock;

    const float nodeMass = gridBlocks[gridBlockIndex].mass[nodeLane];

    if (nodeMass == 0.0f)
    {
        return;
    }

    int blockX, blockY, blockZ;
    MortonDecode(blockCodes[gridBlockIndex], blockX, blockY, blockZ);

    const int localX = nodeLane % blockSize;
    const int localY = (nodeLane / blockSize) % blockSize;
    const int localZ = (nodeLane / (blockSize * blockSize)) % blockSize;

    const int nodeGridX = blockX * blockSize + localX;
    const int nodeGridY = blockY * blockSize + localY;
    const int nodeGridZ = blockZ * blockSize + localZ;

    float velocityX = gridBlocks[gridBlockIndex].velocityX[nodeLane] / nodeMass;
    float velocityY = gridBlocks[gridBlockIndex].velocityY[nodeLane] / nodeMass;
    float velocityZ = gridBlocks[gridBlockIndex].velocityZ[nodeLane] / nodeMass;

    constexpr float gravity = 9.8f;
    velocityY -= gravity * simulationParameters.deltaTime;

    const float tangentialScale = 1.0f - simulationParameters.boundaryFriction;

    if (nodeGridX < 2 && velocityX < 0.0f)
    {
        velocityX = 0.0f;
        velocityY *= tangentialScale;
        velocityZ *= tangentialScale;
    }
    else if (nodeGridX >= simulationParameters.gridSizeInCells - 2 && velocityX > 0.0f)
    {
        velocityX = 0.0f;
        velocityY *= tangentialScale;
        velocityZ *= tangentialScale;
    }

    if (nodeGridY < 2)
    {
        if (velocityY < 0.0f)
        {
            velocityX *= tangentialScale;
            velocityZ *= tangentialScale;
            velocityY = 0.0f;
        }
        else
        {
            velocityY *= tangentialScale;
        }
    }
    else if (nodeGridY >= simulationParameters.gridSizeInCells - 2)
    {
        if (velocityY > 0.0f)
        {
            velocityX *= tangentialScale;
            velocityZ *= tangentialScale;
            velocityY = 0.0f;
        }
        else
        {
            velocityY *= tangentialScale;
        }
    }

    if (nodeGridZ < 2 && velocityZ < 0.0f)
    {
        velocityZ = 0.0f;
        velocityX *= tangentialScale;
        velocityY *= tangentialScale;
    }
    else if (nodeGridZ >= simulationParameters.gridSizeInCells - 2 && velocityZ > 0.0f)
    {
        velocityZ = 0.0f;
        velocityX *= tangentialScale;
        velocityY *= tangentialScale;
    }

    const int sdfIndex = nodeGridZ * simulationParameters.gridSizeInCells * simulationParameters.gridSizeInCells + nodeGridY * simulationParameters.gridSizeInCells + nodeGridX;

    if (sdfDistances[sdfIndex] < simulationParameters.cellSize)
    {
        const glm::vec3 normal = sdfNormals[sdfIndex];
        const float relNormal = (velocityX - simulationParameters.boundaryVelocity.x) * normal.x + (velocityY - simulationParameters.boundaryVelocity.y) * normal.y + (velocityZ - simulationParameters.boundaryVelocity.z) * normal.z;

        if (relNormal < 0.0f)
        {
            velocityX -= relNormal * normal.x;
            velocityY -= relNormal * normal.y;
            velocityZ -= relNormal * normal.z;

            const float velDotNormal = velocityX * normal.x + velocityY * normal.y + velocityZ * normal.z;
            velocityX = velDotNormal * normal.x + tangentialScale * (velocityX - velDotNormal * normal.x);
            velocityY = velDotNormal * normal.y + tangentialScale * (velocityY - velDotNormal * normal.y);
            velocityZ = velDotNormal * normal.z + tangentialScale * (velocityZ - velDotNormal * normal.z);
        }
    }

    gridBlocks[gridBlockIndex].velocityX[nodeLane] = velocityX;
    gridBlocks[gridBlockIndex].velocityY[nodeLane] = velocityY;
    gridBlocks[gridBlockIndex].velocityZ[nodeLane] = velocityZ;
}
