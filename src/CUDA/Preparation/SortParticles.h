#ifndef MPM_METHOD_SORT_PARTICLES_H
#define MPM_METHOD_SORT_PARTICLES_H

#include <cstdint>
#include <cstddef>

#include "../Structures/ParticleBuffer.h"
#include "../Structures/HashTable.h"

constexpr int cellBits = 9;

#ifdef __CUDACC__
__global__ void ComputeSortKeysKernel(const ParticleBlock* particleBlocks, int particleCount, const HashTable& blockCodeToIndex, uint64_t* sortKeys, float cellSize);
__global__ void InitIndicesKernel(uint32_t* indices, int particleCount);
__global__ void ReorderParticlesKernel(const ParticleBlock* inputBlocks, ParticleBlock* outputBlocks, const uint32_t* sortedIndices, int particleCount);
__global__ void WarpSortKernel(ParticleBlock* particleBlocks, int particleCount, const HashTable &blockCodeToIndex, float cellSize, uint64_t* particleHomeBlockCodes);
#endif

#ifdef __CUDACC__
#include <cuda_runtime_api.h>
#else
struct CUstream_st;
typedef CUstream_st* cudaStream_t;
#endif

void SortParticles(const ParticleBlock* inputBlocks, ParticleBlock* outputBlocks, const HashTable& blockCodeToIndex, int particleCount, float cellSize, uint64_t* sortKeys, uint64_t* sortKeysOut, uint32_t* indices, uint32_t* sortedIndices, void* tempStorage, size_t& tempStorageBytes, cudaStream_t stream);
void WarpSort(ParticleBlock* particleBlocks, int particleCount, const HashTable& blockCodeToIndex, float cellSize, uint64_t* particleHomeBlockCodes, cudaStream_t stream);

#endif
