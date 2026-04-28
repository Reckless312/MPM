#ifndef MPM_METHOD_REBUILD_MAPPING_H
#define MPM_METHOD_REBUILD_MAPPING_H

#include <cstdint>

#include "ParticleBuffer.h"
#include "HashTable.h"

constexpr int blockSize = 8;

#ifdef __CUDACC__
__global__ void rebuildMappingKernel(const ParticleBlock* particleBlocks, const int particleCount, const HashTable &hashTable, uint32_t* nextBlockIndex, const float cellSize);
#endif

#endif
