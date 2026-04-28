#ifndef MPM_METHOD_HASH_TABLE_H
#define MPM_METHOD_HASH_TABLE_H

#include <cstdint>

constexpr uint64_t EMPTY_CELL = UINT64_MAX;

struct HashTable {
    uint64_t* keys;
    uint32_t* values;
    uint32_t  capacity;
};

#ifdef __CUDACC__
__device__ uint32_t hashCode(uint64_t blockCode, uint32_t capacity);
__device__ void insert(const HashTable& table, uint64_t blockCode, uint32_t* nextBlockIndex);
__device__ uint32_t lookup(const HashTable& table, uint64_t blockCode);
#endif

#endif
