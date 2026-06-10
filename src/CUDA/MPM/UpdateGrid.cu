#include "UpdateGrid.h"

#include "../Structures/Morton.h"
#include "../Preparation/RegisterActiveBlocks.h"
#include "../SimulationParameters.h"

__global__ void UpdateGridKernel(GridBlock* gridBlocks, const uint64_t* blockCodes, const float* sdfDistances, const glm::vec3* sdfNormals)
{
    const int nodeIndex = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    const int gridBlockIndex = nodeIndex / NODES_PER_BLOCK;
    const int nodeLane = nodeIndex % NODES_PER_BLOCK;

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

    // ReSharper disable once CppTooWideScopeInitStatement
    const int sdfIndex = nodeGridZ * simulationParameters.cellCountPerAxis * simulationParameters.cellCountPerAxis + nodeGridY * simulationParameters.cellCountPerAxis + nodeGridX;

    if (sdfDistances[sdfIndex] < simulationParameters.cellSize)
    {
        const glm::vec3 normal = sdfNormals[sdfIndex];

        const float relativeVelocityX = velocityX - simulationParameters.boundaryVelocity.x;
        const float relativeVelocityY = velocityY - simulationParameters.boundaryVelocity.y;
        const float relativeVelocityZ = velocityZ - simulationParameters.boundaryVelocity.z;

        // ReSharper disable once CppTooWideScopeInitStatement
        const float relativeNormal = relativeVelocityX * normal.x + relativeVelocityY * normal.y + relativeVelocityZ * normal.z;

        if (relativeNormal < 0.0f)
        {
            const float tangentialVelocityX = (relativeVelocityX - relativeNormal * normal.x) * tangentialScale;
            const float tangentialVelocityY = (relativeVelocityY - relativeNormal * normal.y) * tangentialScale;
            const float tangentialVelocityZ = (relativeVelocityZ - relativeNormal * normal.z) * tangentialScale;

            velocityX = tangentialVelocityX + simulationParameters.boundaryVelocity.x;
            velocityY = tangentialVelocityY + simulationParameters.boundaryVelocity.y;
            velocityZ = tangentialVelocityZ + simulationParameters.boundaryVelocity.z;
        }
    }

    EnforceWallBoundary(velocityX, velocityY, velocityZ, nodeGridX, simulationParameters.cellCountPerAxis, tangentialScale);
    EnforceWallBoundary(velocityY, velocityX, velocityZ, nodeGridY, simulationParameters.cellCountPerAxis, tangentialScale);
    EnforceWallBoundary(velocityZ, velocityX, velocityY, nodeGridZ, simulationParameters.cellCountPerAxis, tangentialScale);

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
