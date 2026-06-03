#ifndef MPM_METHOD_G2P_H
#define MPM_METHOD_G2P_H

#include "../Structures/ParticleBuffer.h"
#include "../Structures/GridBuffer.h"
#include "../Structures/HashTable.h"

#ifdef __CUDACC__
__global__ void G2PKernel(ParticleBlock* particleBlocks, const GridBlock* gridBlocks, int particleCount, const HashTable& blockCodeToIndex, const uint64_t* particleHomeBlockCodes, uint32_t* rebuildFlag);
#endif

#endif
