#ifndef MPM_METHOD_SIMULATION_H
#define MPM_METHOD_SIMULATION_H

#include <cstdint>
#include <cstddef>
#include <vector>

#include "Structures/ParticleBuffer.h"
#include "Structures/GridBuffer.h"
#include "Structures/HashTable.h"

struct cudaGraphicsResource;

class Simulation
{
public:
    Simulation(int particleCount, const ParticleBlock* initialParticleBlocks, int particleBlockCount, unsigned int vbo);
    ~Simulation();
    void Step();
    void SyncPositionsToVBO();
    void UploadMeshBoundary(const std::vector<uint8_t>& solidCellsHost) const;
    void ClearMeshBoundary() const;
    void Reset(const ParticleBlock* initialBlocks, int blockCount) const;

private:
    int particleCount;

    ParticleBlock* particleBlocks{};
    ParticleBlock* particleBlocksSortingBuffer{};
    GridBlock* gridBlocks{};

    HashTable blockCodeToIndex{};
    uint32_t* nextBlockIndex{};
    uint64_t* blockCodes{};

    uint64_t* particleSortKeys{};
    uint64_t* particleSortKeysResult{};
    uint32_t* particleIndices{};
    uint32_t* sortedParticleIndices{};

    uint64_t* particleHomeBlockCodes{};
    uint32_t* rebuildFlag{};

    void* nvidiaCUBTemporaryStorage = nullptr;

    size_t nvidiaCUBTemporaryStorageBytes = 0;

    cudaGraphicsResource* vboResource{};
    uint8_t* solidCells{};

    const int threadsPerBlock = 128;

    uint32_t activeBlockCount = 0;
};

#endif
