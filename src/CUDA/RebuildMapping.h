#ifndef MPM_METHOD_REBUILD_MAPPING_H
#define MPM_METHOD_REBUILD_MAPPING_H

#include <cstdint>

#include "ParticleBuffer.h"
#include "HashTable.h"

constexpr int blockSize = 8;
constexpr int nodesPerBlock = blockSize * blockSize * blockSize;
constexpr float freeZoneShift = 0.5f;

#ifdef __CUDACC__
__global__ void rebuildMappingKernel(const ParticleBlock* particleBlocks, int particleCount, const HashTable &hashTable, uint32_t* nextBlockIndex, uint64_t* blockCodes, float cellSize);
#endif

#endif
