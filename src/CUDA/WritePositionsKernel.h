#ifndef MPM_METHOD_WRITE_POSITIONS_KERNEL_H
#define MPM_METHOD_WRITE_POSITIONS_KERNEL_H

#include "Structures/ParticleBuffer.h"

#ifdef __CUDACC__
__global__ void WritePositionsKernel(const ParticleBlock* particleBlocks, float* buffer, int particleCount);
#endif

#endif
