#ifndef MPM_METHOD_SIMULATION_H
#define MPM_METHOD_SIMULATION_H

#include <cstdint>
#include <cstddef>

#include "ParticleBuffer.h"
#include "GridBuffer.h"
#include "HashTable.h"

struct SimulationParameters
{
    int particleCount;
    int maxBlocks;
    int gridSizeInCells;
    float cellSize;
    float deltaTime;
    float gravity;
    float shearModulus;
    float firstLameParameter;
    float hardeningCoefficient;
    float criticalCompression;
    float criticalStretch;
};

class Simulation
{
public:
    Simulation(const SimulationParameters& parameters, const ParticleBlock* initialParticleBlocks, int particleBlockCount);
    ~Simulation();
    void step();
    void copyPositionsToHost(float* positionsX, float* positionsY, float* positionsZ) const;
    const ParticleBlock* getParticleBlocks() const;

private:
    SimulationParameters m_parameters;

    ParticleBlock* m_particleBlocks;
    ParticleBlock* m_particleBlocksBuffer;
    GridBlock* m_gridBlocks;

    HashTable m_hashTable;
    uint32_t* m_nextBlockIndex;
    uint64_t* m_blockCodes;

    uint64_t* m_sortKeys;
    uint64_t* m_sortKeysOut;
    uint32_t* m_indices;
    uint32_t* m_sortedIndices;
    void* m_cubTempStorage;
    size_t m_cubTempStorageBytes;

    ParticleBlock* m_hostParticleBlocks;
};

#endif
