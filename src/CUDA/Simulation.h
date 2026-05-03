#ifndef MPM_METHOD_SIMULATION_H
#define MPM_METHOD_SIMULATION_H

#include <cstdint>
#include <cstddef>

#include "Structures/ParticleBuffer.h"
#include "Structures/GridBuffer.h"
#include "Structures/HashTable.h"
#include "OpenGL/Simulation/Configuration.h"

class Simulation
{
public:
    Simulation(int particleCount, const ParticleBlock* initialParticleBlocks, int particleBlockCount);
    ~Simulation();
    void Step();
    void CopyPositionsToHost(float* positionsX, float* positionsY, float* positionsZ) const;

private:
    int particleCount;

    Configuration configuration;

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

    ParticleBlock* hostParticleBlocks{};

    const int threadsPerBlock = 256;

    int stepsSinceLastRebuild = 0;
};

#endif
