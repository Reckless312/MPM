#include "Simulation.h"
#include "CudaCheck.h"
#include <cstdio>
#include "Preparation/RebuildMapping.h"
#include "Preparation/SortParticles.h"
#include "MPM/P2G.h"
#include "MPM/UpdateGrid.h"
#include "MPM/G2P.h"
#include <cuda_runtime.h>
#include <glad/glad.h>
#include <cuda_gl_interop.h>
#include <cub/cub.cuh>
#include <utility>

__global__ void WritePositionsKernel(const ParticleBlock* particleBlocks, float* buffer, const int particleCount)
{
    const int particleIndex = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);

    if (particleIndex >= particleCount)
    {
        return;
    }

    const int particleBlockIndex = particleIndex / 32;
    const int lane = particleIndex % 32;

    buffer[particleIndex * 3 + 0] = particleBlocks[particleBlockIndex].positionX[lane];
    buffer[particleIndex * 3 + 1] = particleBlocks[particleBlockIndex].positionY[lane];
    buffer[particleIndex * 3 + 2] = particleBlocks[particleBlockIndex].positionZ[lane];
}

Simulation::Simulation(const int particleCount, const ParticleBlock* initialParticleBlocks, const int particleBlockCount, const unsigned int vbo)
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

    CUDA_CHECK(cudaMalloc(&particleHomeBlockCodes, particleCount * sizeof(uint64_t)));
    CUDA_CHECK(cudaMemset(particleHomeBlockCodes, 0xFF, particleCount * sizeof(uint64_t)));

    CUDA_CHECK(cudaMalloc(&rebuildFlag, sizeof(uint32_t)));
    constexpr uint32_t initialRebuildFlag = 1u;
    CUDA_CHECK(cudaMemcpy(rebuildFlag, &initialRebuildFlag, sizeof(uint32_t), cudaMemcpyHostToDevice));

    CUDA_CHECK(cub::DeviceRadixSort::SortPairs(nullptr, nvidiaCUBTemporaryStorageBytes, particleSortKeys, particleSortKeysResult, particleIndices, sortedParticleIndices, particleCount));

    CUDA_CHECK(cudaMalloc(&nvidiaCUBTemporaryStorage, nvidiaCUBTemporaryStorageBytes));

    CUDA_CHECK(cudaGraphicsGLRegisterBuffer(&vboResource, vbo, cudaGraphicsMapFlagsWriteDiscard));

    const size_t solidCellsSize = configuration.cellCountPerAxis * configuration.cellCountPerAxis * configuration.cellCountPerAxis * sizeof(uint8_t);
    CUDA_CHECK(cudaMalloc(&solidCells, solidCellsSize));
    CUDA_CHECK(cudaMemset(solidCells, 0, solidCellsSize));
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
    cudaFree(particleHomeBlockCodes);
    cudaFree(rebuildFlag);
    cudaFree(nvidiaCUBTemporaryStorage);
    cudaFree(solidCells);
    cudaGraphicsUnregisterResource(vboResource);
}

void Simulation::Step()
{
    const int launchBlocks = (this->particleCount + threadsPerBlock - 1) / threadsPerBlock;

    uint32_t needsRebuild;
    CUDA_CHECK(cudaMemcpy(&needsRebuild, rebuildFlag, sizeof(uint32_t), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemset(rebuildFlag, 0, sizeof(uint32_t)));

    if (needsRebuild)
    {
        printf("Rebuild triggered after %d skipped steps\n", stepsSinceLastRebuild);
        stepsSinceLastRebuild = 0;
        CUDA_CHECK(cudaMemset(blockCodeToIndex.keys, 0xFF, configuration.maxBlocks * sizeof(uint64_t)));
        CUDA_CHECK(cudaMemset(nextBlockIndex, 0, sizeof(uint32_t)));

        RebuildMappingKernel<<<launchBlocks, threadsPerBlock>>>(particleBlocks, particleCount, blockCodeToIndex, nextBlockIndex, blockCodes, configuration.cellSize);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());

        SortParticles(particleBlocks, particleBlocksSortingBuffer, blockCodeToIndex, particleCount, configuration.cellSize, particleSortKeys, particleSortKeysResult, particleIndices, sortedParticleIndices, nvidiaCUBTemporaryStorage, nvidiaCUBTemporaryStorageBytes);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());

        std::swap(particleBlocks, particleBlocksSortingBuffer);

    }
    else
    {
        stepsSinceLastRebuild++;
    }

    WarpSort(particleBlocks, particleCount, blockCodeToIndex, configuration.cellSize, particleHomeBlockCodes);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaMemset(gridBlocks, 0, configuration.maxBlocks * sizeof(GridBlock)));

    const size_t bSplineSharedMemoryBytes = threadsPerBlock * 9 * sizeof(float);

    P2GKernel<<<launchBlocks, threadsPerBlock, bSplineSharedMemoryBytes>>>(particleBlocks, gridBlocks, particleCount, blockCodeToIndex, configuration.cellSize, configuration.deltaTime, configuration.secondLameParameter, configuration.firstLameParameter, configuration.hardeningCoefficient, needsRebuild, particleHomeBlockCodes);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    const int totalNodes = configuration.maxBlocks * nodesPerBlock;
    const int gridLaunchBlocks = (totalNodes + threadsPerBlock - 1) / threadsPerBlock;

    UpdateGridKernel<<<gridLaunchBlocks, threadsPerBlock>>>(gridBlocks, blockCodes, configuration.maxBlocks, configuration.deltaTime, configuration.gravity, configuration.cellCountPerAxis, configuration.boundaryFriction, solidCells);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    G2PKernel<<<launchBlocks, threadsPerBlock, bSplineSharedMemoryBytes>>>(particleBlocks, gridBlocks, particleCount, blockCodeToIndex, configuration.cellSize, configuration.deltaTime, configuration.criticalCompression, configuration.criticalStretch, particleHomeBlockCodes, rebuildFlag);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
}

void Simulation::SyncPositionsToVBO()
{
    size_t bufferSize;
    float* deviceBuffer;

    CUDA_CHECK(cudaGraphicsMapResources(1, &vboResource));
    CUDA_CHECK(cudaGraphicsResourceGetMappedPointer(reinterpret_cast<void**>(&deviceBuffer), &bufferSize, vboResource));

    const int launchBlocks = (particleCount + threadsPerBlock - 1) / threadsPerBlock;
    WritePositionsKernel<<<launchBlocks, threadsPerBlock>>>(particleBlocks, deviceBuffer, particleCount);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaGraphicsUnmapResources(1, &vboResource));
}

void Simulation::Reset(const ParticleBlock* initialBlocks, const int blockCount)
{
    CUDA_CHECK(cudaMemcpy(particleBlocks, initialBlocks, blockCount * sizeof(ParticleBlock), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemset(blockCodeToIndex.keys, 0xFF, configuration.maxBlocks * sizeof(uint64_t)));
    CUDA_CHECK(cudaMemset(nextBlockIndex, 0, sizeof(uint32_t)));
    CUDA_CHECK(cudaMemset(gridBlocks, 0, configuration.maxBlocks * sizeof(GridBlock)));
    CUDA_CHECK(cudaMemset(particleHomeBlockCodes, 0xFF, particleCount * sizeof(uint64_t)));

    constexpr uint32_t forceRebuild = 1u;
    CUDA_CHECK(cudaMemcpy(rebuildFlag, &forceRebuild, sizeof(uint32_t), cudaMemcpyHostToDevice));

    stepsSinceLastRebuild = 0;
}

void Simulation::UploadMeshBoundary(const std::vector<uint8_t>& solidCellsHost)
{
    const size_t solidCellsSize = configuration.cellCountPerAxis * configuration.cellCountPerAxis * configuration.cellCountPerAxis * sizeof(uint8_t);
    CUDA_CHECK(cudaMemcpy(solidCells, solidCellsHost.data(), solidCellsSize, cudaMemcpyHostToDevice));
}
