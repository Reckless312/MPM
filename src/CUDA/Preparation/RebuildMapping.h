#ifndef MPM_METHOD_REBUILD_MAPPING_H
#define MPM_METHOD_REBUILD_MAPPING_H

#include <cstdint>

#include "../Structures/ParticleBuffer.h"
#include "../Structures/HashTable.h"

constexpr int blockSize = 8;
constexpr int nodesPerBlock = blockSize * blockSize * blockSize;
constexpr float freeZoneShift = 0.5f;

#ifdef __CUDACC__
__global__ void RebuildMappingKernel(const ParticleBlock* particleBlocks, int particleCount, const HashTable &hashTable, uint32_t* nextBlockIndex, uint64_t* blockCodes, float cellSize);
__global__ void RecordHomeBlocksKernel(const ParticleBlock* particleBlocks, int particleCount, uint64_t* particleHomeBlockCodes, float cellSize);
__global__ void CheckRebuildKernel(const ParticleBlock* particleBlocks, int particleCount, const uint64_t* particleHomeBlockCodes, uint32_t* rebuildFlag, float cellSize);
#endif

#endif
