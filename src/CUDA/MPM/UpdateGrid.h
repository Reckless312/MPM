#ifndef MPM_METHOD_UPDATE_GRID_H
#define MPM_METHOD_UPDATE_GRID_H

#include <cstdint>

#include "../Structures/GridBuffer.h"

#ifdef __CUDACC__
__global__ void updateGridKernel(GridBlock* gridBlocks, const uint64_t* blockCodes, int totalBlocks, float deltaTime, float gravity, float cellSize, int gridSizeInCells);
#endif

#endif
