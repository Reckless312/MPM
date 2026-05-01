#include "Simulation.h"
#include "CudaCheck.h"
#include "Preparation/RebuildMapping.h"
#include "Preparation/SortParticles.h"
#include "MPM/P2G.h"
#include "MPM/UpdateGrid.h"
#include "MPM/G2P.h"
#include <cuda_runtime.h>
#include <cub/cub.cuh>
#include <utility>

Simulation::Simulation(const int particleCount, const ParticleBlock* initialParticleBlocks, const int particleBlockCount)
{
    this->particleCount = particleCount;

    CUDA_CHECK(cudaMalloc(&particleBlocks, particleBlockCount * sizeof(ParticleBlock)));
    CUDA_CHECK(cudaMalloc(&particleBlocksSortingBuffer, particleBlockCount * sizeof(ParticleBlock)));

    CUDA_CHECK(cudaMalloc(&gridBlocks, configuration.maxBlocks * sizeof(GridBlock)));

    CUDA_CHECK(cudaMalloc(&blockCodeToIndex.keys, configuration.maxBlocks * sizeof(uint64_t)));
    CUDA_CHECK(cudaMalloc(&blockCodeToIndex.values, configuration.maxBlocks * sizeof(uint32_t)));
    blockCodeToIndex.capacity = static_cast<uint32_t>(configuration.maxBlocks);

    CUDA_CHECK(cudaMalloc(&nextBlockIndex, sizeof(uint32_t)));
    CUDA_CHECK(cudaMalloc(&blockCodes, configuration.maxBlocks * sizeof(uint64_t)));

    CUDA_CHECK(cudaMalloc(&particleSortKeys, particleCount * sizeof(uint64_t)));
    CUDA_CHECK(cudaMalloc(&particleSortKeysResult, particleCount * sizeof(uint64_t)));
    CUDA_CHECK(cudaMalloc(&particleIndices, particleCount * sizeof(uint32_t)));

    CUDA_CHECK(cudaMalloc(&sortedParticleIndices, particleCount * sizeof(uint32_t)));
    CUDA_CHECK(cudaMemcpy(particleBlocks, initialParticleBlocks, particleBlockCount * sizeof(ParticleBlock), cudaMemcpyHostToDevice));

    CUDA_CHECK(cub::DeviceRadixSort::SortPairs(nullptr, nvidiaCUBTemporaryStorageBytes, particleSortKeys, particleSortKeysResult, particleIndices, sortedParticleIndices, particleCount));

    CUDA_CHECK(cudaMalloc(&nvidiaCUBTemporaryStorage, nvidiaCUBTemporaryStorageBytes));

    const int blockCount = (particleCount + 31) / 32;
    CUDA_CHECK(cudaMallocHost(&hostParticleBlocks, blockCount * sizeof(ParticleBlock)));
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

void Simulation::Step()
{
    const int launchBlocks = (this->particleCount + threadsPerBlock - 1) / threadsPerBlock;

    CUDA_CHECK(cudaMemset(blockCodeToIndex.keys, 0xFF, configuration.maxBlocks * sizeof(uint64_t)));
    CUDA_CHECK(cudaMemset(nextBlockIndex, 0, sizeof(uint32_t)));

    RebuildMappingKernel<<<launchBlocks, threadsPerBlock>>>(particleBlocks, particleCount, blockCodeToIndex, nextBlockIndex, blockCodes, configuration.cellSize);

    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaMemset(gridBlocks, 0, configuration.maxBlocks * sizeof(GridBlock)));

    SortParticles(particleBlocks, particleBlocksSortingBuffer, blockCodeToIndex, particleCount, configuration.cellSize, particleSortKeys, particleSortKeysResult, particleIndices, sortedParticleIndices, nvidiaCUBTemporaryStorage, nvidiaCUBTemporaryStorageBytes);

    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    std::swap(particleBlocks, particleBlocksSortingBuffer);

    P2GKernel<<<launchBlocks, threadsPerBlock>>>(particleBlocks, gridBlocks, particleCount, blockCodeToIndex, configuration.cellSize, configuration.deltaTime, configuration.secondLameParameter, configuration.firstLameParameter, configuration.hardeningCoefficient);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    const int totalNodes = configuration.maxBlocks * nodesPerBlock;
    const int gridLaunchBlocks = (totalNodes + threadsPerBlock - 1) / threadsPerBlock;
    updateGridKernel<<<gridLaunchBlocks, threadsPerBlock>>>(gridBlocks, blockCodes, configuration.maxBlocks, configuration.deltaTime, configuration.gravity, configuration.cellSize, configuration.cellCountPerAxis);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    g2pKernel<<<launchBlocks, threadsPerBlock>>>(particleBlocks, gridBlocks, particleCount, blockCodeToIndex, configuration.cellSize, configuration.deltaTime, configuration.criticalCompression, configuration.criticalStretch);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
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
