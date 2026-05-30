#include "Morton.h"

__device__ uint64_t ExpandBits(const uint32_t value)
{
    uint64_t expanded = value & twentyOneBitMask;

    expanded = (expanded | expanded << 32) & separateHalvesMask;
    expanded = (expanded | expanded << 16) & separateBytesMask;
    expanded = (expanded | expanded << 8)  & separateNibblesMask;
    expanded = (expanded | expanded << 4)  & separatePairsMask;
    expanded = (expanded | expanded << 2)  & everyThirdBitMask;

    return expanded;
}

__device__ uint32_t CompactBits(uint64_t value)
{
    value &= everyThirdBitMask;

    value = (value | value >> 2) & separatePairsMask;
    value = (value | value >> 4) & separateNibblesMask;
    value = (value | value >> 8) & separateBytesMask;
    value = (value | value >> 16) & separateHalvesMask;
    value = (value | value >> 32) & twentyOneBitMask;

    return static_cast<uint32_t>(value);
}

__device__ uint64_t MortonEncode(const int coordinateX, const int coordinateY, const int coordinateZ)
{
    return ExpandBits(static_cast<uint32_t>(coordinateX)) | (ExpandBits(static_cast<uint32_t>(coordinateY)) << 1) | (ExpandBits(static_cast<uint32_t>(coordinateZ)) << 2);
}

__device__ void MortonDecode(const uint64_t code, int& coordinateX, int& coordinateY, int& coordinateZ)
{
    coordinateX = static_cast<int>(CompactBits(code));
    coordinateY = static_cast<int>(CompactBits(code >> 1));
    coordinateZ = static_cast<int>(CompactBits(code >> 2));
}
