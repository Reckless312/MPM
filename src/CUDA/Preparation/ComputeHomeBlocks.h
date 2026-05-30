#ifndef MPM_METHOD_COMPUTE_HOME_BLOCKS_H
#define MPM_METHOD_COMPUTE_HOME_BLOCKS_H

#include <cstdint>
#include "../Structures/ParticleBuffer.h"
#ifdef __CUDACC__
__global__ void ComputeHomeBlocksKernel(const ParticleBlock* particleBlocks, int particleCount, uint64_t* particleHomeBlockCodes);
#endif

#endif
