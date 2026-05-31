#ifndef MPM_METHOD_GRID_BUFFER_H
#define MPM_METHOD_GRID_BUFFER_H

#include "../Preparation/RebuildMapping.h"

struct GridBlock {
    float mass[nodesPerBlock];
    float velocityX[nodesPerBlock], velocityY[nodesPerBlock], velocityZ[nodesPerBlock];
};

#endif
