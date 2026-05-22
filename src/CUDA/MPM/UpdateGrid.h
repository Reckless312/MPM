#ifndef MPM_METHOD_UPDATE_GRID_H
#define MPM_METHOD_UPDATE_GRID_H

#include <cstdint>

#include <glm/vec3.hpp>

#include "../Structures/GridBuffer.h"

#ifdef __CUDACC__
__global__ void UpdateGridKernel(GridBlock *gridBlocks, const uint64_t *blockCodes, int totalBlocks, float deltaTime, float gravity, int gridSizeInCells, float boundaryFriction, float cellSize, const float* sdfDistances, const glm::vec3* sdfNormals, const glm::vec3* boundaryVelocity);
#endif

#endif
