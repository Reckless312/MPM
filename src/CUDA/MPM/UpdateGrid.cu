#include "UpdateGrid.h"
#include "../Structures/Morton.h"
#include "../Preparation/RebuildMapping.h"

__global__ void UpdateGridKernel(GridBlock *gridBlocks, const uint64_t *blockCodes, const int totalBlocks, const float deltaTime, const float gravity, const int gridSizeInCells, const float boundaryFriction, const float cellSize, const float* sdfDistances, const glm::vec3* sdfNormals, const glm::vec3* boxCenter, const glm::vec3* boxHalfExtents, const glm::vec3* boxVelocity)
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

    const float tangentialScale = 1.0f - boundaryFriction;

    if (nodeGridX < 2 && velocityX < 0.0f)
    {
        velocityX = 0.0f;
        velocityY *= tangentialScale;
        velocityZ *= tangentialScale;
    }
    else if (nodeGridX >= gridSizeInCells - 2 && velocityX > 0.0f)
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
    else if (nodeGridY >= gridSizeInCells - 2)
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
    else if (nodeGridZ >= gridSizeInCells - 2 && velocityZ > 0.0f)
    {
        velocityZ = 0.0f;
        velocityX *= tangentialScale;
        velocityY *= tangentialScale;
    }

    const int sdfIndex = nodeGridZ * gridSizeInCells * gridSizeInCells + nodeGridY * gridSizeInCells + nodeGridX;

    if (sdfDistances[sdfIndex] < cellSize)
    {
        const glm::vec3 normal = sdfNormals[sdfIndex];
        const float normalComponent = velocityX * normal.x + velocityY * normal.y + velocityZ * normal.z;

        if (normalComponent < 0.0f)
        {
            velocityX -= normalComponent * normal.x;
            velocityY -= normalComponent * normal.y;
            velocityZ -= normalComponent * normal.z;
            velocityX *= tangentialScale;
            velocityY *= tangentialScale;
            velocityZ *= tangentialScale;
        }
    }

    const glm::vec3 nodePos(nodeGridX * cellSize, nodeGridY * cellSize, nodeGridZ * cellSize);

    const float dx = fabsf(nodePos.x - boxCenter->x) - boxHalfExtents->x;
    const float dy = fabsf(nodePos.y - boxCenter->y) - boxHalfExtents->y;
    const float dz = fabsf(nodePos.z - boxCenter->z) - boxHalfExtents->z;

    const float dxPos = fmaxf(dx, 0.0f);
    const float dyPos = fmaxf(dy, 0.0f);
    const float dzPos = fmaxf(dz, 0.0f);
    const float outsideDist = sqrtf(dxPos * dxPos + dyPos * dyPos + dzPos * dzPos);
    const float boxSdf = outsideDist + fminf(fmaxf(dx, fmaxf(dy, dz)), 0.0f);

    if (boxSdf < 2.0f * cellSize)
    {
        glm::vec3 n;

        if (outsideDist > 0.0f)
        {
            n = glm::vec3(dxPos, dyPos, dzPos) * (1.0f / outsideDist);
        }
        else if (dx >= dy && dx >= dz)
        {
            n = glm::vec3(nodePos.x > boxCenter->x ? 1.0f : -1.0f, 0.0f, 0.0f);
        }
        else if (dy >= dz)
        {
            n = glm::vec3(0.0f, nodePos.y > boxCenter->y ? 1.0f : -1.0f, 0.0f);
        }
        else
        {
            n = glm::vec3(0.0f, 0.0f, nodePos.z > boxCenter->z ? 1.0f : -1.0f);
        }

        const float relNormal = (velocityX - boxVelocity->x) * n.x + (velocityY - boxVelocity->y) * n.y + (velocityZ - boxVelocity->z) * n.z;

        if (relNormal < 0.0f)
        {
            velocityX -= relNormal * n.x;
            velocityY -= relNormal * n.y;
            velocityZ -= relNormal * n.z;
        }
    }

    gridBlocks[gridBlockIndex].velocityX[nodeLane] = velocityX;
    gridBlocks[gridBlockIndex].velocityY[nodeLane] = velocityY;
    gridBlocks[gridBlockIndex].velocityZ[nodeLane] = velocityZ;
}
