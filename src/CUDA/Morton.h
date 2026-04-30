#ifndef MPM_METHOD_MORTON_H
#define MPM_METHOD_MORTON_H

#include <cstdint>

#ifdef __CUDACC__
__device__ uint64_t expandBits(uint32_t value);
__device__ uint32_t compactBits(uint64_t value);
__device__ uint64_t mortonEncode(int coordinateX, int coordinateY, int coordinateZ);
__device__ void mortonDecode(uint64_t code, int& coordinateX, int& coordinateY, int& coordinateZ);
#endif

#endif
