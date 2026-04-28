#ifndef MPM_METHOD_GRID_BUFFER_H
#define MPM_METHOD_GRID_BUFFER_H

struct GridBlock {
    float mass[32];
    float vx[32], vy[32], vz[32];
};

#endif
