#include "HashTable.h"

__device__ uint32_t HashCode(const uint64_t blockCode, const uint32_t capacity)
{
    const uint64_t hashCode = (blockCode ^ (blockCode >> 30)) * 0xbf58476d1ce4e5b9ULL;
    return static_cast<uint32_t>(hashCode % capacity);
}

__device__ uint32_t Lookup(const HashTable& table, const uint64_t blockCode)
{
    const uint32_t startSlot = HashCode(blockCode, table.capacity);
    uint32_t slot = startSlot;

    do
    {
        if (table.keys[slot] == blockCode)
        {
            return table.values[slot];
        }

        if (table.keys[slot] == EMPTY_CELL)
        {
            return BLOCK_NOT_FOUND;
        }

        slot = (slot + 1) % table.capacity;
    }
    while (slot != startSlot);

    return BLOCK_NOT_FOUND;
}

__device__ void Insert(const HashTable& table, const uint64_t blockCode, uint32_t* nextBlockIndex, uint64_t* blockCodes)
{
    const uint32_t startSlot = HashCode(blockCode, table.capacity);
    uint32_t slot = startSlot;

    do
    {
        // ReSharper disable once CppTooWideScopeInitStatement
        const uint64_t previousValue = atomicCAS(reinterpret_cast<unsigned long long *>(&table.keys[slot]), EMPTY_CELL, blockCode);

        if (previousValue == EMPTY_CELL)
        {
            const uint32_t blockIndex = atomicAdd(nextBlockIndex, 1u);

            if (blockIndex >= table.capacity)
            {
                return;
            }

            table.values[slot] = blockIndex;
            blockCodes[blockIndex] = blockCode;
            return;
        }

        if (previousValue == blockCode)
        {
            return;
        }

        slot = (slot + 1) % table.capacity;
    }
    while (slot != startSlot);
}
