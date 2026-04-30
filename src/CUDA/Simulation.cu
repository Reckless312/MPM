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

Simulation::Simulation(const int particleCount, const Configuration& configuration, const ParticleBlock* initialParticleBlocks, const int particleBlockCount) : particleCount(particleCount), configuration(configuration), nvidiaCUBTemporaryStorage(nullptr), nvidiaCUBTemporaryStorageBytes(0)
{
    cudaMalloc(&particleBlocks, particleBlockCount * sizeof(ParticleBlock));
    cudaMalloc(&particleBlocksSortingBuffer, particleBlockCount * sizeof(ParticleBlock));
    cudaMalloc(&gridBlocks,  configuration.maxBlocks * sizeof(GridBlock));

    cudaMalloc(&blockCodeToIndex.keys, configuration.maxBlocks * sizeof(uint64_t));
    cudaMalloc(&blockCodeToIndex.values, configuration.maxBlocks * sizeof(uint32_t));
    blockCodeToIndex.capacity = static_cast<uint32_t>(configuration.maxBlocks);

    cudaMalloc(&nextBlockIndex, sizeof(uint32_t));
    cudaMalloc(&blockCodes, configuration.maxBlocks * sizeof(uint64_t));

    cudaMalloc(&particleSortKeys, particleCount * sizeof(uint64_t));
    cudaMalloc(&particleSortKeysResult, particleCount * sizeof(uint64_t));
    cudaMalloc(&particleIndices, particleCount * sizeof(uint32_t));
    cudaMalloc(&sortedParticleIndices, particleCount * sizeof(uint32_t));

    cudaMemcpy(particleBlocks, initialParticleBlocks, particleBlockCount * sizeof(ParticleBlock), cudaMemcpyHostToDevice);

    cub::DeviceRadixSort::SortPairs(nullptr, nvidiaCUBTemporaryStorageBytes, particleSortKeys, particleSortKeysResult, particleIndices, sortedParticleIndices, particleCount);

    cudaMalloc(&nvidiaCUBTemporaryStorage, nvidiaCUBTemporaryStorageBytes);

    const int blockCount = (particleCount + 31) / 32;
    cudaMallocHost(&hostParticleBlocks, blockCount * sizeof(ParticleBlock));
}

Simulation::~Simulation()
{
    cudaFree(particleBlocks);
    cudaFree(particleBlocksSortingBuffer);
    cudaFree(gridBlocks);
    cudaFree(blockCodeToIndex.keys);
    cudaFree(blockCodeToIndex.values);
    cudaFree(nextBlockIndex);
    cudaFree(blockCodes);
    cudaFree(particleSortKeys);
    cudaFree(particleSortKeysResult);
    cudaFree(particleIndices);
    cudaFree(sortedParticleIndices);
    cudaFree(nvidiaCUBTemporaryStorage);
    cudaFreeHost(hostParticleBlocks);
}

void Simulation::step()
{
    const int particleCount = this->particleCount;
    const int maxBlocks = configuration.maxBlocks;
    const int launchBlocks = (particleCount + threadsPerBlock - 1) / threadsPerBlock;

    cudaMemset(blockCodeToIndex.keys, 0xFF, maxBlocks * sizeof(uint64_t));
    cudaMemset(nextBlockIndex, 0, sizeof(uint32_t));

    rebuildMappingKernel<<<launchBlocks, threadsPerBlock>>>(particleBlocks, particleCount, blockCodeToIndex, nextBlockIndex, blockCodes, configuration.cellSize);

    cudaMemset(gridBlocks, 0, maxBlocks * sizeof(GridBlock));

    sortParticles(particleBlocks, particleBlocksSortingBuffer, blockCodeToIndex, particleCount, configuration.cellSize, particleSortKeys, particleSortKeysResult, particleIndices, sortedParticleIndices, nvidiaCUBTemporaryStorage, nvidiaCUBTemporaryStorageBytes);
    std::swap(particleBlocks, particleBlocksSortingBuffer);

    p2gKernel<<<launchBlocks, threadsPerBlock>>>(particleBlocks, gridBlocks, particleCount, blockCodeToIndex, configuration.cellSize, configuration.deltaTime, configuration.secondLameParameter, configuration.firstLameParameter, configuration.hardeningCoefficient);

    const int totalNodes = maxBlocks * nodesPerBlock;
    const int gridLaunchBlocks = (totalNodes + threadsPerBlock - 1) / threadsPerBlock;
    updateGridKernel<<<gridLaunchBlocks, threadsPerBlock>>>(gridBlocks, blockCodes, maxBlocks, configuration.deltaTime, configuration.gravity, configuration.cellSize, configuration.cellCountPerAxis);

    g2pKernel<<<launchBlocks, threadsPerBlock>>>(particleBlocks, gridBlocks, particleCount, blockCodeToIndex, configuration.cellSize, configuration.deltaTime, configuration.criticalCompression, configuration.criticalStretch);
}

void Simulation::copyPositionsToHost(float* positionsX, float* positionsY, float* positionsZ) const
{
    const int particleCount = this->particleCount;
    const int blockCount = (particleCount + 31) / 32;
    cudaMemcpy(hostParticleBlocks, particleBlocks, blockCount * sizeof(ParticleBlock), cudaMemcpyDeviceToHost);

    for (int particleIndex = 0; particleIndex < particleCount; particleIndex++)
    {
        const int blockIndex = particleIndex / 32;
        const int lane = particleIndex % 32;
        positionsX[particleIndex] = hostParticleBlocks[blockIndex].positionX[lane];
        positionsY[particleIndex] = hostParticleBlocks[blockIndex].positionY[lane];
        positionsZ[particleIndex] = hostParticleBlocks[blockIndex].positionZ[lane];
    }
}

const ParticleBlock* Simulation::getParticleBlocks() const
{
    return particleBlocks;
}
