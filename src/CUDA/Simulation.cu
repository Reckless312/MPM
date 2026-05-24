#include "Simulation.h"
#include "CudaCheck.h"
#include <cstdio>
#include "Preparation/RebuildMapping.h"
#include "Preparation/SortParticles.h"
#include "MPM/P2G.h"
#include "MPM/UpdateGrid.h"
#include "MPM/G2P.h"
#include "WritePositionsKernel.h"
#include <cuda_runtime.h>
#include <glad/glad.h>
#include <cuda_gl_interop.h>
#include <cub/cub.cuh>
#include <utility>
#include "OpenGL/Program.h"
#include "OpenGL/SimulationConfig.h"

Simulation::Simulation(const SnowVolume& volume, const unsigned int vbo)
{
    const int initialParticleCount = volume.particleCount;
    const ParticleBlock* initialParticleBlocks = volume.initialBlocks.data();
    const int initialParticleBlockCount = static_cast<int>(volume.initialBlocks.size());

    this->particleCount = initialParticleCount;
    this->allocatedParticleCount = initialParticleCount;

    CUDA_CHECK(cudaMalloc(&this->particleBlocks, (initialParticleCount + 31) / 32 * sizeof(ParticleBlock)));
    CUDA_CHECK(cudaMemcpy(this->particleBlocks, initialParticleBlocks, initialParticleBlockCount * sizeof(ParticleBlock), cudaMemcpyHostToDevice));

    this->AllocateParticleBuffers(initialParticleCount);
    CUDA_CHECK(cudaMemset(this->particleHomeBlockCodes, 0xFF, initialParticleCount * sizeof(uint64_t)));

    CUDA_CHECK(cudaMalloc(&this->gridBlocks, SimulationConfig::maxBlocks * sizeof(GridBlock)));
    CUDA_CHECK(cudaMalloc(&this->blockCodeToIndex.keys, SimulationConfig::maxBlocks * sizeof(uint64_t)));
    CUDA_CHECK(cudaMalloc(&this->blockCodeToIndex.values, SimulationConfig::maxBlocks * sizeof(uint32_t)));
    this->blockCodeToIndex.capacity = static_cast<uint32_t>(SimulationConfig::maxBlocks);
    CUDA_CHECK(cudaMalloc(&this->nextBlockIndex, sizeof(uint32_t)));
    CUDA_CHECK(cudaMalloc(&this->blockCodes, SimulationConfig::maxBlocks * sizeof(uint64_t)));

    CUDA_CHECK(cudaMalloc(&this->rebuildFlag, sizeof(uint32_t)));
    constexpr uint32_t initialRebuildFlag = 1u;
    CUDA_CHECK(cudaMemcpy(this->rebuildFlag, &initialRebuildFlag, sizeof(uint32_t), cudaMemcpyHostToDevice));

    this->vboId = vbo;
    CUDA_CHECK(cudaGraphicsGLRegisterBuffer(&this->vboResource, vbo, cudaGraphicsMapFlagsWriteDiscard));

    const int nodeCount = SimulationConfig::cellCountPerAxis * SimulationConfig::cellCountPerAxis * SimulationConfig::cellCountPerAxis;
    CUDA_CHECK(cudaMalloc(&this->sdfDistances, nodeCount * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&this->sdfNormals, nodeCount * sizeof(glm::vec3)));
    CUDA_CHECK(cudaMemset(this->sdfDistances, 0x7F, nodeCount * sizeof(float)));
    CUDA_CHECK(cudaMemset(this->sdfNormals, 0, nodeCount * sizeof(glm::vec3)));

    CUDA_CHECK(cudaMalloc(&this->boundaryVelocity, sizeof(glm::vec3)));
    constexpr glm::vec3 zeroBoundaryVelocity(0.0f);
    CUDA_CHECK(cudaMemcpy(this->boundaryVelocity, &zeroBoundaryVelocity, sizeof(glm::vec3), cudaMemcpyHostToDevice));

    CUDA_CHECK(cudaStreamCreate(&this->simulationStream));
}

Simulation::~Simulation()
{
    cudaStreamSynchronize(this->simulationStream);

    if (this->graphValid)
    {
        cudaGraphExecDestroy(this->simulationGraphExec);
    }

    cudaStreamDestroy(this->simulationStream);

    cudaFree(this->particleBlocks);
    this->FreeParticleBuffers();
    cudaFree(this->gridBlocks);
    cudaFree(this->blockCodeToIndex.keys);
    cudaFree(this->blockCodeToIndex.values);
    cudaFree(this->nextBlockIndex);
    cudaFree(this->blockCodes);
    cudaFree(this->rebuildFlag);
    cudaFree(this->sdfDistances);
    cudaFree(this->sdfNormals);
    cudaFree(this->boundaryVelocity);
    cudaGraphicsUnregisterResource(this->vboResource);
}

void Simulation::Step()
{
    const int launchBlocks = (this->particleCount + this->threadsPerBlock - 1) / this->threadsPerBlock;
    const size_t bSplineSharedMemoryBytes = this->threadsPerBlock * 9 * sizeof(float);
    const int gridLaunchBlocks = (SimulationConfig::maxBlocks * nodesPerBlock + this->threadsPerBlock - 1) / this->threadsPerBlock;

    uint32_t needsRebuild;
    CUDA_CHECK(cudaMemcpy(&needsRebuild, this->rebuildFlag, sizeof(uint32_t), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemsetAsync(this->rebuildFlag, 0, sizeof(uint32_t), this->simulationStream));

    if (needsRebuild)
    {
        if (this->graphValid)
        {
            CUDA_CHECK(cudaGraphExecDestroy(this->simulationGraphExec));
            this->graphValid = false;
        }

        CUDA_CHECK(cudaMemsetAsync(this->blockCodeToIndex.keys, 0xFF, SimulationConfig::maxBlocks * sizeof(uint64_t), this->simulationStream));
        CUDA_CHECK(cudaMemsetAsync(this->nextBlockIndex, 0, sizeof(uint32_t), this->simulationStream));

        RebuildMappingKernel<<<launchBlocks, this->threadsPerBlock, 0, this->simulationStream>>>(this->particleBlocks, this->particleCount, this->blockCodeToIndex, this->nextBlockIndex, this->blockCodes, SimulationConfig::cellSize);
        CUDA_CHECK(cudaGetLastError());

        SortParticles(this->particleBlocks, this->particleBlocksSortingBuffer, this->blockCodeToIndex, this->particleCount, SimulationConfig::cellSize, this->particleSortKeys, this->particleSortKeysResult, this->particleIndices, this->sortedParticleIndices, this->nvidiaCUBTemporaryStorage, this->nvidiaCUBTemporaryStorageBytes, this->simulationStream);
        CUDA_CHECK(cudaGetLastError());

        std::swap(this->particleBlocks, this->particleBlocksSortingBuffer);

        CUDA_CHECK(cudaMemcpy(&this->activeBlockCount, this->nextBlockIndex, sizeof(uint32_t), cudaMemcpyDeviceToHost));

        WarpSort(this->particleBlocks, this->particleCount, this->blockCodeToIndex, SimulationConfig::cellSize, this->particleHomeBlockCodes, this->simulationStream);
        CUDA_CHECK(cudaMemsetAsync(this->gridBlocks, 0, this->activeBlockCount * sizeof(GridBlock), this->simulationStream));
        P2GKernel<<<launchBlocks, this->threadsPerBlock, bSplineSharedMemoryBytes, this->simulationStream>>>(this->particleBlocks, this->gridBlocks, this->particleCount, this->blockCodeToIndex, SimulationConfig::cellSize, SimulationConfig::physicsTimeStep, SimulationConfig::secondLameParameter, SimulationConfig::firstLameParameter, SimulationConfig::hardeningCoefficient, true, this->particleHomeBlockCodes);
        CUDA_CHECK(cudaStreamSynchronize(this->simulationStream));
        UpdateGridKernel<<<gridLaunchBlocks, this->threadsPerBlock, 0, this->simulationStream>>>(this->gridBlocks, this->blockCodes, SimulationConfig::maxBlocks, SimulationConfig::physicsTimeStep, SimulationConfig::cellCountPerAxis, SimulationConfig::boundaryFriction, SimulationConfig::cellSize, this->sdfDistances, this->sdfNormals, this->boundaryVelocity);
        CUDA_CHECK(cudaStreamSynchronize(this->simulationStream));
        G2PKernel<<<launchBlocks, this->threadsPerBlock, bSplineSharedMemoryBytes, this->simulationStream>>>(this->particleBlocks, this->gridBlocks, this->particleCount, this->blockCodeToIndex, SimulationConfig::cellSize, SimulationConfig::physicsTimeStep, SimulationConfig::criticalCompression, SimulationConfig::criticalStretch, this->particleHomeBlockCodes, this->rebuildFlag);
        CUDA_CHECK(cudaStreamSynchronize(this->simulationStream));
        return;
    }

    if (!this->graphValid)
    {
        cudaGraph_t graph;
        CUDA_CHECK(cudaStreamBeginCapture(this->simulationStream, cudaStreamCaptureModeGlobal));

        WarpSort(this->particleBlocks, this->particleCount, this->blockCodeToIndex, SimulationConfig::cellSize, this->particleHomeBlockCodes, this->simulationStream);
        CUDA_CHECK(cudaMemsetAsync(this->gridBlocks, 0, this->activeBlockCount * sizeof(GridBlock), this->simulationStream));
        P2GKernel<<<launchBlocks, this->threadsPerBlock, bSplineSharedMemoryBytes, this->simulationStream>>>(this->particleBlocks, this->gridBlocks, this->particleCount, this->blockCodeToIndex, SimulationConfig::cellSize, SimulationConfig::physicsTimeStep, SimulationConfig::secondLameParameter, SimulationConfig::firstLameParameter, SimulationConfig::hardeningCoefficient, false, this->particleHomeBlockCodes);
        UpdateGridKernel<<<gridLaunchBlocks, this->threadsPerBlock, 0, this->simulationStream>>>(this->gridBlocks, this->blockCodes, SimulationConfig::maxBlocks, SimulationConfig::physicsTimeStep, SimulationConfig::cellCountPerAxis, SimulationConfig::boundaryFriction, SimulationConfig::cellSize, this->sdfDistances, this->sdfNormals, this->boundaryVelocity);
        G2PKernel<<<launchBlocks, this->threadsPerBlock, bSplineSharedMemoryBytes, this->simulationStream>>>(this->particleBlocks, this->gridBlocks, this->particleCount, this->blockCodeToIndex, SimulationConfig::cellSize, SimulationConfig::physicsTimeStep, SimulationConfig::criticalCompression, SimulationConfig::criticalStretch, this->particleHomeBlockCodes, this->rebuildFlag);

        CUDA_CHECK(cudaStreamEndCapture(this->simulationStream, &graph));
        CUDA_CHECK(cudaGraphInstantiate(&this->simulationGraphExec, graph, nullptr, nullptr, 0));
        CUDA_CHECK(cudaGraphDestroy(graph));
        this->graphValid = true;
    }

    CUDA_CHECK(cudaGraphLaunch(this->simulationGraphExec, this->simulationStream));
    CUDA_CHECK(cudaStreamSynchronize(this->simulationStream));
}

void Simulation::SyncPositionsToVBO()
{
    size_t bufferSize;
    float* deviceBuffer;

    CUDA_CHECK(cudaGraphicsMapResources(1, &this->vboResource));
    CUDA_CHECK(cudaGraphicsResourceGetMappedPointer(reinterpret_cast<void**>(&deviceBuffer), &bufferSize, this->vboResource));

    const int launchBlocks = (this->particleCount + this->threadsPerBlock - 1) / this->threadsPerBlock;
    WritePositionsKernel<<<launchBlocks, this->threadsPerBlock>>>(this->particleBlocks, deviceBuffer, this->particleCount);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaGraphicsUnmapResources(1, &this->vboResource));
}

void Simulation::Reset(const ParticleBlock* initialBlocks, const int blockCount, const int newParticleCount)
{
    if (this->graphValid)
    {
        CUDA_CHECK(cudaStreamSynchronize(this->simulationStream));
        CUDA_CHECK(cudaGraphExecDestroy(this->simulationGraphExec));
        this->graphValid = false;
    }

    this->particleCount = newParticleCount;

    CUDA_CHECK(cudaMemcpy(this->particleBlocks, initialBlocks, blockCount * sizeof(ParticleBlock), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemset(this->blockCodeToIndex.keys, 0xFF, SimulationConfig::maxBlocks * sizeof(uint64_t)));
    CUDA_CHECK(cudaMemset(this->nextBlockIndex, 0, sizeof(uint32_t)));
    CUDA_CHECK(cudaMemset(this->gridBlocks, 0, SimulationConfig::maxBlocks * sizeof(GridBlock)));
    CUDA_CHECK(cudaMemset(this->particleHomeBlockCodes, 0xFF, newParticleCount * sizeof(uint64_t)));

    constexpr uint32_t forceRebuild = 1u;
    CUDA_CHECK(cudaMemcpy(this->rebuildFlag, &forceRebuild, sizeof(uint32_t), cudaMemcpyHostToDevice));
}

void Simulation::AddParticles(const ParticleBlock* blocks, const int blockCount, const int additionalParticleCount)
{
    const int existingBlockCount = (this->particleCount + 31) / 32;
    CUDA_CHECK(cudaMemcpy(this->particleBlocks + existingBlockCount, blocks, blockCount * sizeof(ParticleBlock), cudaMemcpyHostToDevice));

    this->particleCount += additionalParticleCount;

    constexpr uint32_t forceRebuild = 1u;
    CUDA_CHECK(cudaMemcpy(this->rebuildFlag, &forceRebuild, sizeof(uint32_t), cudaMemcpyHostToDevice));
}

void Simulation::UploadMeshBoundary(const MeshSDF& sdf) const
{
    const int nodeCount = SimulationConfig::cellCountPerAxis * SimulationConfig::cellCountPerAxis * SimulationConfig::cellCountPerAxis;
    CUDA_CHECK(cudaMemcpy(this->sdfDistances, sdf.distances.data(), nodeCount * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(this->sdfNormals, sdf.normals.data(), nodeCount * sizeof(glm::vec3), cudaMemcpyHostToDevice));
}

void Simulation::ClearMeshBoundary() const
{
    const int nodeCount = SimulationConfig::cellCountPerAxis * SimulationConfig::cellCountPerAxis * SimulationConfig::cellCountPerAxis;
    CUDA_CHECK(cudaMemset(this->sdfDistances, 0x7F, nodeCount * sizeof(float)));
    CUDA_CHECK(cudaMemset(this->sdfNormals, 0, nodeCount * sizeof(glm::vec3)));
}

__global__ void ShiftSdfZKernel(float* sdfDistances, glm::vec3* sdfNormals, const int gridSize, const int shiftCells)
{
    const int gridX = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    const int gridY = static_cast<int>(blockIdx.y * blockDim.y + threadIdx.y);

    if (gridX >= gridSize || gridY >= gridSize)
    {
        return;
    }

    for (int gridZ = gridSize - 1; gridZ >= 0; gridZ--)
    {
        const int index = gridZ * gridSize * gridSize + gridY * gridSize + gridX;

        if (gridZ >= shiftCells)
        {
            const int srcIndex = (gridZ - shiftCells) * gridSize * gridSize + gridY * gridSize + gridX;
            sdfDistances[index] = sdfDistances[srcIndex];
            sdfNormals[index] = sdfNormals[srcIndex];
        }
        else
        {
            sdfDistances[index] = __int_as_float(0x7F7F7F7F);
            sdfNormals[index] = glm::vec3(0.0f);
        }
    }
}

void Simulation::SetBoundaryVelocity(const glm::vec3 velocity) const
{
    CUDA_CHECK(cudaMemcpy(this->boundaryVelocity, &velocity, sizeof(glm::vec3), cudaMemcpyHostToDevice));
}

void Simulation::ShiftSdfZ(const int cells)
{
    const int gridSize = SimulationConfig::cellCountPerAxis;
    const dim3 threads(16, 16);
    const dim3 blocks((gridSize + 15) / 16, (gridSize + 15) / 16);
    ShiftSdfZKernel<<<blocks, threads>>>(this->sdfDistances, this->sdfNormals, gridSize, cells);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
}

void Simulation::AllocateParticleBuffers(const int count)
{
    const int blockCount = (count + 31) / 32;
    CUDA_CHECK(cudaMalloc(&this->particleBlocksSortingBuffer, blockCount * sizeof(ParticleBlock)));
    CUDA_CHECK(cudaMalloc(&this->particleSortKeys, count * sizeof(uint64_t)));
    CUDA_CHECK(cudaMalloc(&this->particleSortKeysResult, count * sizeof(uint64_t)));
    CUDA_CHECK(cudaMalloc(&this->particleIndices, count * sizeof(uint32_t)));
    CUDA_CHECK(cudaMalloc(&this->sortedParticleIndices, count * sizeof(uint32_t)));
    CUDA_CHECK(cudaMalloc(&this->particleHomeBlockCodes, count * sizeof(uint64_t)));
    CUDA_CHECK(cub::DeviceRadixSort::SortPairs(nullptr, this->nvidiaCUBTemporaryStorageBytes, this->particleSortKeys, this->particleSortKeysResult, this->particleIndices, this->sortedParticleIndices, count));
    CUDA_CHECK(cudaMalloc(&this->nvidiaCUBTemporaryStorage, this->nvidiaCUBTemporaryStorageBytes));
}

void Simulation::FreeParticleBuffers() const
{
    cudaFree(this->particleBlocksSortingBuffer);
    cudaFree(this->particleSortKeys);
    cudaFree(this->particleSortKeysResult);
    cudaFree(this->particleIndices);
    cudaFree(this->sortedParticleIndices);
    cudaFree(this->particleHomeBlockCodes);
    cudaFree(this->nvidiaCUBTemporaryStorage);
}

int Simulation::Grow()
{
    const int newAllocatedCount = this->allocatedParticleCount * 4;

    CUDA_CHECK(cudaStreamSynchronize(this->simulationStream));

    if (this->graphValid)
    {
        CUDA_CHECK(cudaGraphExecDestroy(this->simulationGraphExec));
        this->graphValid = false;
    }

    CUDA_CHECK(cudaGraphicsUnregisterResource(this->vboResource));

    const int oldBlockCount = (this->particleCount + 31) / 32;
    ParticleBlock* oldParticleBlocks = this->particleBlocks;

    this->FreeParticleBuffers();

    CUDA_CHECK(cudaMalloc(&this->particleBlocks, (newAllocatedCount + 31) / 32 * sizeof(ParticleBlock)));
    CUDA_CHECK(cudaMemcpy(this->particleBlocks, oldParticleBlocks, oldBlockCount * sizeof(ParticleBlock), cudaMemcpyDeviceToDevice));
    CUDA_CHECK(cudaFree(oldParticleBlocks));

    this->nvidiaCUBTemporaryStorageBytes = 0;
    this->AllocateParticleBuffers(newAllocatedCount);
    CUDA_CHECK(cudaMemset(this->particleHomeBlockCodes, 0xFF, newAllocatedCount * sizeof(uint64_t)));

    this->allocatedParticleCount = newAllocatedCount;

    return newAllocatedCount;
}

void Simulation::RebindVBO()
{
    CUDA_CHECK(cudaGraphicsGLRegisterBuffer(&this->vboResource, this->vboId, cudaGraphicsMapFlagsWriteDiscard));
}