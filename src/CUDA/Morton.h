#ifndef MPM_METHOD_MORTON_H
#define MPM_METHOD_MORTON_H

#include <cstdint>

#ifdef __CUDACC__
__device__ uint64_t expandBits(uint32_t value);
__device__ uint64_t mortonEncode(int coordinateX, int coordinateY, int coordinateZ);
#endif

#endif
