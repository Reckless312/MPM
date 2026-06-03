#ifndef MPM_METHOD_MORTON_H
#define MPM_METHOD_MORTON_H

#include <cstdint>

constexpr uint64_t twentyOneBitMask = 0x1fffffull;
constexpr uint64_t separateHalvesMask = 0x1f00000000ffffull;
constexpr uint64_t separateBytesMask = 0x1f0000ff0000ffull;
constexpr uint64_t separateNibblesMask = 0x100f00f00f00f00full;
constexpr uint64_t separatePairsMask = 0x10c30c30c30c30c3ull;
constexpr uint64_t everyThirdBitMask = 0x1249249249249249ull;

#ifdef __CUDACC__
__device__ uint64_t ExpandBits(uint32_t value);
__device__ uint32_t CompactBits(uint64_t value);
__device__ uint64_t MortonEncode(int coordinateX, int coordinateY, int coordinateZ);
__device__ void MortonDecode(uint64_t code, int& coordinateX, int& coordinateY, int& coordinateZ);
#endif

#endif
