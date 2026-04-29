#ifndef MPM_METHOD_GRID_BUFFER_H
#define MPM_METHOD_GRID_BUFFER_H

struct GridBlock {
    float mass[512];
    float velocityX[512], velocityY[512], velocityZ[512];
};

#endif
