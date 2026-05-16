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

Simulation::Simulation(const int particleCount, const ParticleBlock* initialParticleBlocks, const int particleBlockCount, const unsigned int vbo)
{
    this->particleCount = particleCount;

    CUDA_CHECK(cudaMalloc(&this->particleBlocks, particleBlockCount * sizeof(ParticleBlock)));
    CUDA_CHECK(cudaMalloc(&this->particleBlocksSortingBuffer, particleBlockCount * sizeof(ParticleBlock)));

    CUDA_CHECK(cudaMalloc(&this->gridBlocks, Program::maxBlocks * sizeof(GridBlock)));

    CUDA_CHECK(cudaMalloc(&this->blockCodeToIndex.keys, Program::maxBlocks * sizeof(uint64_t)));
    CUDA_CHECK(cudaMalloc(&this->blockCodeToIndex.values, Program::maxBlocks * sizeof(uint32_t)));
    this->blockCodeToIndex.capacity = static_cast<uint32_t>(Program::maxBlocks);

    CUDA_CHECK(cudaMalloc(&this->nextBlockIndex, sizeof(uint32_t)));
    CUDA_CHECK(cudaMalloc(&this->blockCodes, Program::maxBlocks * sizeof(uint64_t)));

    CUDA_CHECK(cudaMalloc(&this->particleSortKeys, particleCount * sizeof(uint64_t)));
    CUDA_CHECK(cudaMalloc(&this->particleSortKeysResult, particleCount * sizeof(uint64_t)));
    CUDA_CHECK(cudaMalloc(&this->particleIndices, particleCount * sizeof(uint32_t)));

    CUDA_CHECK(cudaMalloc(&this->sortedParticleIndices, particleCount * sizeof(uint32_t)));
    CUDA_CHECK(cudaMemcpy(this->particleBlocks, initialParticleBlocks, particleBlockCount * sizeof(ParticleBlock), cudaMemcpyHostToDevice));

    CUDA_CHECK(cudaMalloc(&this->particleHomeBlockCodes, particleCount * sizeof(uint64_t)));
    CUDA_CHECK(cudaMemset(this->particleHomeBlockCodes, 0xFF, particleCount * sizeof(uint64_t)));

    CUDA_CHECK(cudaMalloc(&this->rebuildFlag, sizeof(uint32_t)));
    constexpr uint32_t initialRebuildFlag = 1u;
    CUDA_CHECK(cudaMemcpy(this->rebuildFlag, &initialRebuildFlag, sizeof(uint32_t), cudaMemcpyHostToDevice));

    CUDA_CHECK(cub::DeviceRadixSort::SortPairs(nullptr, this->nvidiaCUBTemporaryStorageBytes, this->particleSortKeys, this->particleSortKeysResult, this->particleIndices, this->sortedParticleIndices, particleCount));

    CUDA_CHECK(cudaMalloc(&this->nvidiaCUBTemporaryStorage, this->nvidiaCUBTemporaryStorageBytes));

    CUDA_CHECK(cudaGraphicsGLRegisterBuffer(&this->vboResource, vbo, cudaGraphicsMapFlagsWriteDiscard));

    const int nodeCount = Program::cellCountPerAxis * Program::cellCountPerAxis * Program::cellCountPerAxis;
    CUDA_CHECK(cudaMalloc(&this->sdfDistances, nodeCount * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&this->sdfNormals, nodeCount * sizeof(glm::vec3)));
    CUDA_CHECK(cudaMemset(this->sdfDistances, 0x7F, nodeCount * sizeof(float)));
    CUDA_CHECK(cudaMemset(this->sdfNormals, 0, nodeCount * sizeof(glm::vec3)));

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
    cudaFree(this->particleBlocksSortingBuffer);
    cudaFree(this->gridBlocks);
    cudaFree(this->blockCodeToIndex.keys);
    cudaFree(this->blockCodeToIndex.values);
    cudaFree(this->nextBlockIndex);
    cudaFree(this->blockCodes);
    cudaFree(this->particleSortKeys);
    cudaFree(this->particleSortKeysResult);
    cudaFree(this->particleIndices);
    cudaFree(this->sortedParticleIndices);
    cudaFree(this->particleHomeBlockCodes);
    cudaFree(this->rebuildFlag);
    cudaFree(this->nvidiaCUBTemporaryStorage);
    cudaFree(this->sdfDistances);
    cudaFree(this->sdfNormals);
    cudaGraphicsUnregisterResource(this->vboResource);
}

void Simulation::Step()
{
    const int launchBlocks = (this->particleCount + this->threadsPerBlock - 1) / this->threadsPerBlock;
    const size_t bSplineSharedMemoryBytes = this->threadsPerBlock * 9 * sizeof(float);
    const int gridLaunchBlocks = (Program::maxBlocks * nodesPerBlock + this->threadsPerBlock - 1) / this->threadsPerBlock;

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

        CUDA_CHECK(cudaMemsetAsync(this->blockCodeToIndex.keys, 0xFF, Program::maxBlocks * sizeof(uint64_t), this->simulationStream));
        CUDA_CHECK(cudaMemsetAsync(this->nextBlockIndex, 0, sizeof(uint32_t), this->simulationStream));

        RebuildMappingKernel<<<launchBlocks, this->threadsPerBlock, 0, this->simulationStream>>>(this->particleBlocks, this->particleCount, this->blockCodeToIndex, this->nextBlockIndex, this->blockCodes, Program::cellSize);
        CUDA_CHECK(cudaGetLastError());

        SortParticles(this->particleBlocks, this->particleBlocksSortingBuffer, this->blockCodeToIndex, this->particleCount, Program::cellSize, this->particleSortKeys, this->particleSortKeysResult, this->particleIndices, this->sortedParticleIndices, this->nvidiaCUBTemporaryStorage, this->nvidiaCUBTemporaryStorageBytes, this->simulationStream);
        CUDA_CHECK(cudaGetLastError());

        std::swap(this->particleBlocks, this->particleBlocksSortingBuffer);

        CUDA_CHECK(cudaMemcpy(&this->activeBlockCount, this->nextBlockIndex, sizeof(uint32_t), cudaMemcpyDeviceToHost));

        WarpSort(this->particleBlocks, this->particleCount, this->blockCodeToIndex, Program::cellSize, this->particleHomeBlockCodes, this->simulationStream);
        CUDA_CHECK(cudaMemsetAsync(this->gridBlocks, 0, this->activeBlockCount * sizeof(GridBlock), this->simulationStream));
        P2GKernel<<<launchBlocks, this->threadsPerBlock, bSplineSharedMemoryBytes, this->simulationStream>>>(this->particleBlocks, this->gridBlocks, this->particleCount, this->blockCodeToIndex, Program::cellSize, Program::physicsTimeStep, Program::secondLameParameter, Program::firstLameParameter, Program::hardeningCoefficient, true, this->particleHomeBlockCodes);
        CUDA_CHECK(cudaGetLastError());
        UpdateGridKernel<<<gridLaunchBlocks, this->threadsPerBlock, 0, this->simulationStream>>>(this->gridBlocks, this->blockCodes, Program::maxBlocks, Program::physicsTimeStep, Program::gravity, Program::cellCountPerAxis, Program::boundaryFriction, Program::cellSize, this->sdfDistances, this->sdfNormals);
        CUDA_CHECK(cudaGetLastError());
        G2PKernel<<<launchBlocks, this->threadsPerBlock, bSplineSharedMemoryBytes, this->simulationStream>>>(this->particleBlocks, this->gridBlocks, this->particleCount, this->blockCodeToIndex, Program::cellSize, Program::physicsTimeStep, Program::criticalCompression, Program::criticalStretch, this->particleHomeBlockCodes, this->rebuildFlag);
        CUDA_CHECK(cudaGetLastError());
        return;
    }

    if (!this->graphValid)
    {
        cudaGraph_t graph;
        CUDA_CHECK(cudaStreamBeginCapture(this->simulationStream, cudaStreamCaptureModeGlobal));

        WarpSort(this->particleBlocks, this->particleCount, this->blockCodeToIndex, Program::cellSize, this->particleHomeBlockCodes, this->simulationStream);
        CUDA_CHECK(cudaMemsetAsync(this->gridBlocks, 0, this->activeBlockCount * sizeof(GridBlock), this->simulationStream));
        P2GKernel<<<launchBlocks, this->threadsPerBlock, bSplineSharedMemoryBytes, this->simulationStream>>>(this->particleBlocks, this->gridBlocks, this->particleCount, this->blockCodeToIndex, Program::cellSize, Program::physicsTimeStep, Program::secondLameParameter, Program::firstLameParameter, Program::hardeningCoefficient, false, this->particleHomeBlockCodes);
        UpdateGridKernel<<<gridLaunchBlocks, this->threadsPerBlock, 0, this->simulationStream>>>(this->gridBlocks, this->blockCodes, Program::maxBlocks, Program::physicsTimeStep, Program::gravity, Program::cellCountPerAxis, Program::boundaryFriction, Program::cellSize, this->sdfDistances, this->sdfNormals);
        G2PKernel<<<launchBlocks, this->threadsPerBlock, bSplineSharedMemoryBytes, this->simulationStream>>>(this->particleBlocks, this->gridBlocks, this->particleCount, this->blockCodeToIndex, Program::cellSize, Program::physicsTimeStep, Program::criticalCompression, Program::criticalStretch, this->particleHomeBlockCodes, this->rebuildFlag);

        CUDA_CHECK(cudaStreamEndCapture(this->simulationStream, &graph));
        CUDA_CHECK(cudaGraphInstantiate(&this->simulationGraphExec, graph, nullptr, nullptr, 0));
        CUDA_CHECK(cudaGraphDestroy(graph));
        this->graphValid = true;
    }

    CUDA_CHECK(cudaGraphLaunch(this->simulationGraphExec, this->simulationStream));
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

void Simulation::Reset(const ParticleBlock* initialBlocks, const int blockCount) const
{
    CUDA_CHECK(cudaMemcpy(this->particleBlocks, initialBlocks, blockCount * sizeof(ParticleBlock), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemset(this->blockCodeToIndex.keys, 0xFF, Program::maxBlocks * sizeof(uint64_t)));
    CUDA_CHECK(cudaMemset(this->nextBlockIndex, 0, sizeof(uint32_t)));
    CUDA_CHECK(cudaMemset(this->gridBlocks, 0, Program::maxBlocks * sizeof(GridBlock)));
    CUDA_CHECK(cudaMemset(this->particleHomeBlockCodes, 0xFF, this->particleCount * sizeof(uint64_t)));

    constexpr uint32_t forceRebuild = 1u;
    CUDA_CHECK(cudaMemcpy(this->rebuildFlag, &forceRebuild, sizeof(uint32_t), cudaMemcpyHostToDevice));
}

void Simulation::UploadMeshBoundary(const MeshSDF& sdf) const
{
    const int nodeCount = Program::cellCountPerAxis * Program::cellCountPerAxis * Program::cellCountPerAxis;
    CUDA_CHECK(cudaMemcpy(this->sdfDistances, sdf.distances.data(), nodeCount * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(this->sdfNormals, sdf.normals.data(), nodeCount * sizeof(glm::vec3), cudaMemcpyHostToDevice));
}

void Simulation::ClearMeshBoundary() const
{
    const int nodeCount = Program::cellCountPerAxis * Program::cellCountPerAxis * Program::cellCountPerAxis;
    CUDA_CHECK(cudaMemset(this->sdfDistances, 0x7F, nodeCount * sizeof(float)));
    CUDA_CHECK(cudaMemset(this->sdfNormals, 0, nodeCount * sizeof(glm::vec3)));
}
