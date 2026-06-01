#ifndef MPM_METHOD_GRID_BUFFER_H
#define MPM_METHOD_GRID_BUFFER_H

#include "../Preparation/RegisterActiveBlocks.h"

struct GridBlock {
    float mass[nodesPerBlock];
    float velocityX[nodesPerBlock], velocityY[nodesPerBlock], velocityZ[nodesPerBlock];
};

#endif
