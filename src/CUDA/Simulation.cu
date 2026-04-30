#include "Simulation.h"
#include "RebuildMapping.h"
#include "SortParticles.h"
#include "P2G.h"
#include "UpdateGrid.h"
#include "G2P.h"
#include <cuda_runtime.h>
#include <cub/cub.cuh>
#include <utility>

constexpr int threadsPerBlock = 256;

Simulation::Simulation(const SimulationParameters& parameters, const ParticleBlock* initialParticleBlocks, const int particleBlockCount) : m_parameters(parameters), m_cubTempStorage(nullptr), m_cubTempStorageBytes(0)
{
    const int particleCount = parameters.particleCount;
    const int maxBlocks = parameters.maxBlocks;

    cudaMalloc(&m_particleBlocks, particleBlockCount * sizeof(ParticleBlock));
    cudaMalloc(&m_particleBlocksBuffer, particleBlockCount * sizeof(ParticleBlock));
    cudaMalloc(&m_gridBlocks, maxBlocks * sizeof(GridBlock));

    cudaMalloc(&m_hashTable.keys, maxBlocks * sizeof(uint64_t));
    cudaMalloc(&m_hashTable.values, maxBlocks * sizeof(uint32_t));
    m_hashTable.capacity = static_cast<uint32_t>(maxBlocks);

    cudaMalloc(&m_nextBlockIndex, sizeof(uint32_t));
    cudaMalloc(&m_blockCodes, maxBlocks * sizeof(uint64_t));

    cudaMalloc(&m_sortKeys, particleCount * sizeof(uint64_t));
    cudaMalloc(&m_sortKeysOut, particleCount * sizeof(uint64_t));
    cudaMalloc(&m_indices, particleCount * sizeof(uint32_t));
    cudaMalloc(&m_sortedIndices, particleCount * sizeof(uint32_t));

    cudaMemcpy(m_particleBlocks, initialParticleBlocks, particleBlockCount * sizeof(ParticleBlock), cudaMemcpyHostToDevice);

    cub::DeviceRadixSort::SortPairs(nullptr, m_cubTempStorageBytes,
        m_sortKeys, m_sortKeysOut, m_indices, m_sortedIndices, particleCount);
    cudaMalloc(&m_cubTempStorage, m_cubTempStorageBytes);

    const int blockCount = (particleCount + 31) / 32;
    cudaMallocHost(&m_hostParticleBlocks, blockCount * sizeof(ParticleBlock));
}

Simulation::~Simulation()
{
    cudaFree(m_particleBlocks);
    cudaFree(m_particleBlocksBuffer);
    cudaFree(m_gridBlocks);
    cudaFree(m_hashTable.keys);
    cudaFree(m_hashTable.values);
    cudaFree(m_nextBlockIndex);
    cudaFree(m_blockCodes);
    cudaFree(m_sortKeys);
    cudaFree(m_sortKeysOut);
    cudaFree(m_indices);
    cudaFree(m_sortedIndices);
    cudaFree(m_cubTempStorage);
    cudaFreeHost(m_hostParticleBlocks);
}

void Simulation::step()
{
    const int particleCount = m_parameters.particleCount;
    const int maxBlocks = m_parameters.maxBlocks;
    const int launchBlocks = (particleCount + threadsPerBlock - 1) / threadsPerBlock;

    cudaMemset(m_hashTable.keys, 0xFF, maxBlocks * sizeof(uint64_t));
    cudaMemset(m_nextBlockIndex, 0, sizeof(uint32_t));

    rebuildMappingKernel<<<launchBlocks, threadsPerBlock>>>(m_particleBlocks, particleCount, m_hashTable, m_nextBlockIndex, m_blockCodes, m_parameters.cellSize);

    cudaMemset(m_gridBlocks, 0, maxBlocks * sizeof(GridBlock));

    sortParticles(m_particleBlocks, m_particleBlocksBuffer, m_hashTable, particleCount, m_parameters.cellSize, m_sortKeys, m_sortKeysOut, m_indices, m_sortedIndices, m_cubTempStorage, m_cubTempStorageBytes);
    std::swap(m_particleBlocks, m_particleBlocksBuffer);

    p2gKernel<<<launchBlocks, threadsPerBlock>>>(m_particleBlocks, m_gridBlocks, particleCount, m_hashTable, m_parameters.cellSize, m_parameters.deltaTime, m_parameters.shearModulus, m_parameters.firstLameParameter, m_parameters.hardeningCoefficient);

    const int totalNodes = maxBlocks * nodesPerBlock;
    const int gridLaunchBlocks = (totalNodes + threadsPerBlock - 1) / threadsPerBlock;
    updateGridKernel<<<gridLaunchBlocks, threadsPerBlock>>>(m_gridBlocks, m_blockCodes, maxBlocks, m_parameters.deltaTime, m_parameters.gravity, m_parameters.cellSize, m_parameters.gridSizeInCells);

    g2pKernel<<<launchBlocks, threadsPerBlock>>>(m_particleBlocks, m_gridBlocks, particleCount, m_hashTable, m_parameters.cellSize, m_parameters.deltaTime, m_parameters.criticalCompression, m_parameters.criticalStretch);
}

void Simulation::copyPositionsToHost(float* positionsX, float* positionsY, float* positionsZ) const
{
    const int particleCount = m_parameters.particleCount;
    const int blockCount = (particleCount + 31) / 32;
    cudaMemcpy(m_hostParticleBlocks, m_particleBlocks, blockCount * sizeof(ParticleBlock), cudaMemcpyDeviceToHost);

    for (int particleIndex = 0; particleIndex < particleCount; particleIndex++)
    {
        const int blockIndex = particleIndex / 32;
        const int lane = particleIndex % 32;
        positionsX[particleIndex] = m_hostParticleBlocks[blockIndex].positionX[lane];
        positionsY[particleIndex] = m_hostParticleBlocks[blockIndex].positionY[lane];
        positionsZ[particleIndex] = m_hostParticleBlocks[blockIndex].positionZ[lane];
    }
}

const ParticleBlock* Simulation::getParticleBlocks() const
{
    return m_particleBlocks;
}
