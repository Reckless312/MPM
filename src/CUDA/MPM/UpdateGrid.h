#ifndef MPM_METHOD_UPDATE_GRID_H
#define MPM_METHOD_UPDATE_GRID_H

#include <cstdint>

#include <glm/vec3.hpp>

#include "../Structures/GridBuffer.h"

#ifdef __CUDACC__
__global__ void UpdateGridKernel(GridBlock* gridBlocks, const uint64_t* blockCodes, const float* sdfDistances, const glm::vec3* sdfNormals);
#endif

#endif
