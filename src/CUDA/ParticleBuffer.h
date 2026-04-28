#ifndef MPM_METHOD_PARTICLE_BUFFER_H
#define MPM_METHOD_PARTICLE_BUFFER_H

struct ParticleBlock {
    float x[32], y[32], z[32];
    float vx[32], vy[32], vz[32];
    float F[9][32];
    float Cp[9][32];
    float mass[32];
    float vol[32];
};

#endif
