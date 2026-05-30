#ifndef MPM_METHOD_HASH_TABLE_H
#define MPM_METHOD_HASH_TABLE_H

#include <cstdint>

constexpr uint32_t BLOCK_NOT_FOUND = UINT32_MAX;
constexpr uint64_t EMPTY_CELL = UINT64_MAX;

struct HashTable {
    uint64_t* keys;
    uint32_t* values;
    uint32_t capacity;
};

#ifdef __CUDACC__
__device__ uint32_t HashCode(uint64_t blockCode, uint32_t capacity);
__device__ uint32_t Lookup(const HashTable& table, uint64_t blockCode);
__device__ void Insert(const HashTable& table, uint64_t blockCode, uint32_t* nextBlockIndex, uint64_t* blockCodes);
#endif

#endif
