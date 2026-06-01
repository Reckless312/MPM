#include "UpdateGrid.h"

#include "../Structures/Morton.h"
#include "../Preparation/RegisterActiveBlocks.h"
#include "../SimulationParameters.h"

__global__ void UpdateGridKernel(GridBlock* gridBlocks, const uint64_t* blockCodes, const float* sdfDistances, const glm::vec3* sdfNormals)
{
    const int nodeIndex = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
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

    EnforceWallBoundary(velocityX, velocityY, velocityZ, nodeGridX, simulationParameters.gridSizeInCells, tangentialScale);
    if (nodeGridY < 2 || nodeGridY >= simulationParameters.gridSizeInCells - 2)
    {
        velocityY *= tangentialScale;
        EnforceWallBoundary(velocityY, velocityX, velocityZ, nodeGridY, simulationParameters.gridSizeInCells, tangentialScale);
    }
    EnforceWallBoundary(velocityZ, velocityX, velocityY, nodeGridZ, simulationParameters.gridSizeInCells, tangentialScale);

    // ReSharper disable once CppTooWideScopeInitStatement
    const int sdfIndex = nodeGridZ * simulationParameters.gridSizeInCells * simulationParameters.gridSizeInCells + nodeGridY * simulationParameters.gridSizeInCells + nodeGridX;

    if (sdfDistances[sdfIndex] < simulationParameters.cellSize)
    {
        const glm::vec3 normal = sdfNormals[sdfIndex];
        // ReSharper disable once CppTooWideScopeInitStatement
        const float relativeNormal = (velocityX - simulationParameters.boundaryVelocity.x) * normal.x + (velocityY - simulationParameters.boundaryVelocity.y) * normal.y + (velocityZ - simulationParameters.boundaryVelocity.z) * normal.z;

        if (relativeNormal < 0.0f)
        {
            velocityX -= relativeNormal * normal.x;
            velocityY -= relativeNormal * normal.y;
            velocityZ -= relativeNormal * normal.z;

            const float normalVelocityComponent = velocityX * normal.x + velocityY * normal.y + velocityZ * normal.z;
            velocityX = normalVelocityComponent * normal.x + tangentialScale * (velocityX - normalVelocityComponent * normal.x);
            velocityY = normalVelocityComponent * normal.y + tangentialScale * (velocityY - normalVelocityComponent * normal.y);
            velocityZ = normalVelocityComponent * normal.z + tangentialScale * (velocityZ - normalVelocityComponent * normal.z);
        }
    }

    gridBlocks[gridBlockIndex].velocityX[nodeLane] = velocityX;
    gridBlocks[gridBlockIndex].velocityY[nodeLane] = velocityY;
    gridBlocks[gridBlockIndex].velocityZ[nodeLane] = velocityZ;
}

__device__ void EnforceWallBoundary(float& normalVelocity, float& tangentialVelocityA, float& tangentialVelocityB, const int nodeCoordinate, const int gridSizeInCells, const float tangentialScale)
{
    if ((nodeCoordinate < 2 && normalVelocity < 0.0f) || (nodeCoordinate >= gridSizeInCells - 2 && normalVelocity > 0.0f))
    {
        normalVelocity = 0.0f;
        tangentialVelocityA *= tangentialScale;
        tangentialVelocityB *= tangentialScale;
    }
}
