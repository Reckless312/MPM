#ifndef MPM_METHOD_SIMULATION_H
#define MPM_METHOD_SIMULATION_H

#include <cstdint>
#include <cstddef>

#include "ParticleBuffer.h"
#include "GridBuffer.h"
#include "HashTable.h"
#include "OpenGL/Simulation/Configuration.h"

class Simulation
{
public:
    Simulation(int particleCount, const Configuration& configuration, const ParticleBlock* initialParticleBlocks, int particleBlockCount);
    ~Simulation();
    void step();
    void copyPositionsToHost(float* positionsX, float* positionsY, float* positionsZ) const;
    const ParticleBlock* getParticleBlocks() const;

private:
    int particleCount;

    Configuration configuration;

    ParticleBlock* particleBlocks;
    ParticleBlock* particleBlocksSortingBuffer;
    GridBlock* gridBlocks;

    HashTable blockCodeToIndex;
    uint32_t* nextBlockIndex;
    uint64_t* blockCodes;

    uint64_t* particleSortKeys;
    uint64_t* particleSortKeysResult;
    uint32_t* particleIndices;
    uint32_t* sortedParticleIndices;
    void* nvidiaCUBTemporaryStorage;
    size_t nvidiaCUBTemporaryStorageBytes;

    ParticleBlock* hostParticleBlocks;
};

#endif
