#ifndef MPM_METHOD_SORT_PARTICLES_H
#define MPM_METHOD_SORT_PARTICLES_H

#include <cstdint>
#include <cstddef>

#include "../Structures/ParticleBuffer.h"
#include "../Structures/HashTable.h"

constexpr int cellBits = 9;

#ifdef __CUDACC__
__global__ void ComputeSortKeysKernel(const ParticleBlock* particleBlocks, int particleCount, const HashTable& hashTable, uint64_t* sortKeys, float cellSize);
__global__ void InitIndicesKernel(uint32_t* indices, int particleCount);
__global__ void ReorderParticlesKernel(const ParticleBlock* inputBlocks, ParticleBlock* outputBlocks, const uint32_t* sortedIndices, int particleCount);
#endif

void SortParticles(const ParticleBlock* inputBlocks, ParticleBlock* outputBlocks, const HashTable& hashTable, int particleCount, float cellSize, uint64_t* sortKeys, uint64_t* sortKeysOut, uint32_t* indices, uint32_t* sortedIndices, void* tempStorage, size_t& tempStorageBytes);

#endif
