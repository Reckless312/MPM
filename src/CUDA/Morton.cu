#include "Morton.h"
#include <cuda_runtime.h>

constexpr uint64_t twentyOneBitMask    = 0x1fffffull;
constexpr uint64_t separateHalvesMask  = 0x1f00000000ffffull;
constexpr uint64_t separateBytesMask   = 0x1f0000ff0000ffull;
constexpr uint64_t separateNibblesMask = 0x100f00f00f00f00full;
constexpr uint64_t separatePairsMask   = 0x10c30c30c30c30c3ull;
constexpr uint64_t everyThirdBitMask   = 0x1249249249249249ull;

__device__ uint64_t expandBits(const uint32_t value)
{
    uint64_t expanded = value & twentyOneBitMask;

    expanded = (expanded | expanded << 32) & separateHalvesMask;
    expanded = (expanded | expanded << 16) & separateBytesMask;
    expanded = (expanded | expanded << 8)  & separateNibblesMask;
    expanded = (expanded | expanded << 4)  & separatePairsMask;
    expanded = (expanded | expanded << 2)  & everyThirdBitMask;

    return expanded;
}

__device__ uint64_t mortonEncode(const int coordinateX, const int coordinateY, const int coordinateZ)
{
    return expandBits(static_cast<uint32_t>(coordinateX)) | (expandBits(static_cast<uint32_t>(coordinateY)) << 1) | (expandBits(static_cast<uint32_t>(coordinateZ)) << 2);
}
