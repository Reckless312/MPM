#ifndef MPM_METHOD_GRID_BUFFER_H
#define MPM_METHOD_GRID_BUFFER_H

#include "../Preparation/RegisterActiveBlocks.h"

struct GridBlock {
    float mass[NODES_PER_BLOCK];
    float velocityX[NODES_PER_BLOCK], velocityY[NODES_PER_BLOCK], velocityZ[NODES_PER_BLOCK];
};

#endif
