#include "UpdateGrid.h"
#include "../Structures/Morton.h"
#include "../Preparation/RebuildMapping.h"
#include <cuda_runtime.h>

__global__ void UpdateGridKernel(GridBlock *gridBlocks, const uint64_t *blockCodes, const int totalBlocks, const float deltaTime, const float gravity, const int gridSizeInCells)
{
    const int nodeIndex = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);

    if (nodeIndex >= totalBlocks * nodesPerBlock)
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

    velocityY -= gravity * deltaTime;

    if (nodeGridX < 2 || nodeGridX >= gridSizeInCells - 2)
    {
        velocityX = 0.0f;
    }

    if (nodeGridY < 2 || nodeGridY >= gridSizeInCells - 2)
    {
        velocityY = 0.0f;
    }

    if (nodeGridZ < 2 || nodeGridZ >= gridSizeInCells - 2)
    {
        velocityZ = 0.0f;
    }

    gridBlocks[gridBlockIndex].velocityX[nodeLane] = velocityX;
    gridBlocks[gridBlockIndex].velocityY[nodeLane] = velocityY;
    gridBlocks[gridBlockIndex].velocityZ[nodeLane] = velocityZ;
}
