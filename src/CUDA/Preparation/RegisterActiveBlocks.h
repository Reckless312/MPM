#ifndef MPM_METHOD_REBUILD_MAPPING_H
#define MPM_METHOD_REBUILD_MAPPING_H

#include <cstdint>
#include "../Structures/ParticleBuffer.h"
#include "../Structures/HashTable.h"

constexpr float freeZoneShift = 0.5f;

constexpr int blockSize = 8;
constexpr int nodesPerBlock = blockSize * blockSize * blockSize;

#ifdef __CUDACC__
__global__ void RegisterActiveBlocks(const ParticleBlock* particleBlocks, int particleCount, const HashTable& blockCodeToIndex, uint32_t* nextBlockIndex, uint64_t* blockCodes);
#endif

#endif
