#include "MoveSDF.h"

#include "OpenGL/Simulation/SimulationConfig.h"

__global__ void MoveSDFInward(float* sdfDistances, glm::vec3* sdfNormals, const int shiftCells)
{
    constexpr int gridSize = SimulationConfig::cellCountPerAxis;

    const int gridX = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    const int gridY = static_cast<int>(blockIdx.y * blockDim.y + threadIdx.y);

    if (gridX >= gridSize || gridY >= gridSize)
    {
        return;
    }

    for (int gridZ = gridSize - 1; gridZ >= 0; gridZ--)
    {
        const int index = gridZ * gridSize * gridSize + gridY * gridSize + gridX;

        if (gridZ >= shiftCells)
        {
            const int srcIndex = (gridZ - shiftCells) * gridSize * gridSize + gridY * gridSize + gridX;
            sdfDistances[index] = sdfDistances[srcIndex];
            sdfNormals[index] = sdfNormals[srcIndex];
        }
        else
        {
            sdfDistances[index] = __int_as_float(0x7F7F7F7F);
            sdfNormals[index] = glm::vec3(0.0f);
        }
    }
}
