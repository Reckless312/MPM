#ifndef MPM_METHOD_SORT_PARTICLES_H
#define MPM_METHOD_SORT_PARTICLES_H

#include <cstdint>
#include "../Structures/ParticleBuffer.h"
#include "../Structures/HashTable.h"

constexpr int cellBits = 9;

#ifdef __CUDACC__
__global__ void InitIndicesKernel(uint32_t* indices, int particleCount);
__global__ void ComputeSortKeysKernel(const ParticleBlock* particleBlocks, int particleCount, const HashTable& blockCodeToIndex, uint64_t* sortKeys);
__global__ void WarpSortKernel(ParticleBlock* particleBlocks, int particleCount, const HashTable& blockCodeToIndex, uint64_t* particleHomeBlockCodes);
__global__ void ReorderParticlesKernel(const ParticleBlock* inputBlocks, ParticleBlock* outputBlocks, const uint32_t* sortedIndices, int particleCount);
#endif

#endif
