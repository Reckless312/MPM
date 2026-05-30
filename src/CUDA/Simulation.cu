#include "Simulation.h"

#include <glad/glad.h>
#include <cuda_gl_interop.h>
#include <utility>
#include <cub/cub.cuh>
#include "CudaCheck.h"
#include "WritePositionsKernel.h"
#include "MPM/G2P.h"
#include "MPM/MoveSDF.h"
#include "MPM/P2G.h"
#include "MPM/UpdateGrid.h"
#include "OpenGL/Simulation/SimulationConfig.h"
#include "Preparation/ComputeHomeBlocks.h"
#include "Preparation/RebuildMapping.h"
#include "Preparation/SortParticles.h"


__constant__ SimulationParameters simulationParameters;

Simulation::Simulation(const SnowVolume& snowVolume, const unsigned int vbo)
{
    this->particleCount = snowVolume.GetParticleCount();
    this->allocatedParticleCount = max(this->particleCount * 4, this->minimumAllocation);

    CUDA_CHECK(cudaMalloc(&this->particleBlocks,  Simulation::ParticlesToBlocks(this->allocatedParticleCount) * sizeof(ParticleBlock)));
    CUDA_CHECK(cudaMemcpy(this->particleBlocks, snowVolume.GetInitialBlocks().data(), snowVolume.GetInitialBlocks().size() * sizeof(ParticleBlock), cudaMemcpyHostToDevice));

    this->AllocateParticleBuffers(this->allocatedParticleCount);

    CUDA_CHECK(cudaMemset(this->particleHomeBlockCodes, 0xFF, this->particleCount * sizeof(uint64_t)));

    CUDA_CHECK(cudaMalloc(&this->gridBlocks, SimulationConfig::maxBlocks * sizeof(GridBlock)));
    CUDA_CHECK(cudaMemset(this->gridBlocks, 0, SimulationConfig::maxBlocks * sizeof(GridBlock)));
    CUDA_CHECK(cudaMalloc(&this->blockCodeToIndex.keys, SimulationConfig::maxBlocks * sizeof(uint64_t)));
    CUDA_CHECK(cudaMalloc(&this->blockCodeToIndex.values, SimulationConfig::maxBlocks * sizeof(uint32_t)));

    this->blockCodeToIndex.capacity = static_cast<uint32_t>(SimulationConfig::maxBlocks);

    CUDA_CHECK(cudaMalloc(&this->nextBlockIndex, sizeof(uint32_t)));
    CUDA_CHECK(cudaMalloc(&this->blockCodes, SimulationConfig::maxBlocks * sizeof(uint64_t)));

    CUDA_CHECK(cudaMalloc(&this->rebuildFlag, sizeof(uint32_t)));
    this->SetRebuildFlag();

    this->vboId = vbo;
    CUDA_CHECK(cudaGraphicsGLRegisterBuffer(&this->vboResource, vbo, cudaGraphicsMapFlagsWriteDiscard));

    CUDA_CHECK(cudaMalloc(&this->sdfDistances, SimulationConfig::nodeCount * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&this->sdfNormals, SimulationConfig::nodeCount * sizeof(glm::vec3)));

    this->ClearMeshBoundary();

    CUDA_CHECK(cudaStreamCreate(&this->simulationStream));

    this->hostSimulationParameters = {
        SimulationConfig::cellSize,
        SimulationConfig::physicsTimeStep,
        SimulationConfig::firstLameParameter,
        SimulationConfig::secondLameParameter,
        SimulationConfig::hardeningCoefficient,
        SimulationConfig::criticalCompression,
        SimulationConfig::criticalStretch,
        SimulationConfig::boundaryFriction,
        SimulationConfig::cellCountPerAxis,
        SimulationConfig::maxBlocks,
        glm::vec3(0.0f)
    };

    this->UploadSimParams();
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

    cudaGraphicsUnregisterResource(this->vboResource);
}

void Simulation::Step()
{
    const int launchBlocks = this->ParticleLaunchBlocks();
    const size_t bSplineSharedMemoryBytes = this->threadsPerBlock * 9 * sizeof(float);

    uint32_t needsRebuild;
    CUDA_CHECK(cudaMemcpy(&needsRebuild, this->rebuildFlag, sizeof(uint32_t), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemsetAsync(this->rebuildFlag, 0, sizeof(uint32_t), this->simulationStream));

    if (needsRebuild)
    {
        this->DestroyGraphIfValid();

        CUDA_CHECK(cudaMemsetAsync(this->blockCodeToIndex.keys, 0xFF, SimulationConfig::maxBlocks * sizeof(uint64_t), this->simulationStream));
        CUDA_CHECK(cudaMemsetAsync(this->nextBlockIndex, 0, sizeof(uint32_t), this->simulationStream));

        RebuildMappingKernel<<<launchBlocks, this->threadsPerBlock, 0, this->simulationStream>>>(this->particleBlocks, this->particleCount, this->blockCodeToIndex, this->nextBlockIndex, this->blockCodes);
        CUDA_CHECK(cudaGetLastError());

        this->SortParticles();
        CUDA_CHECK(cudaGetLastError());

        std::swap(this->particleBlocks, this->particleBlocksSortingBuffer);

        CUDA_CHECK(cudaMemcpy(&this->activeBlockCount, this->nextBlockIndex, sizeof(uint32_t), cudaMemcpyDeviceToHost));

        this->ComputeHomeBlocks();
        CUDA_CHECK(cudaGetLastError());
    }

    if (!this->graphValid)
    {
        const uint32_t gridLaunchBlocks = (this->activeBlockCount * nodesPerBlock + this->threadsPerBlock - 1) / this->threadsPerBlock;

        cudaGraph_t graph;
        CUDA_CHECK(cudaStreamBeginCapture(this->simulationStream, cudaStreamCaptureModeGlobal));

        this->WarpSort();
        CUDA_CHECK(cudaMemsetAsync(this->gridBlocks, 0, this->activeBlockCount * sizeof(GridBlock), this->simulationStream));

        P2GKernel<<<launchBlocks, this->threadsPerBlock, bSplineSharedMemoryBytes, this->simulationStream>>>(this->particleBlocks, this->gridBlocks, this->particleCount, this->blockCodeToIndex);
        UpdateGridKernel<<<gridLaunchBlocks, this->threadsPerBlock, 0, this->simulationStream>>>(this->gridBlocks, this->blockCodes, this->sdfDistances, this->sdfNormals);
        G2PKernel<<<launchBlocks, this->threadsPerBlock, bSplineSharedMemoryBytes, this->simulationStream>>>(this->particleBlocks, this->gridBlocks, this->particleCount, this->blockCodeToIndex, this->particleHomeBlockCodes, this->rebuildFlag);

        CUDA_CHECK(cudaStreamEndCapture(this->simulationStream, &graph));
        CUDA_CHECK(cudaGraphInstantiate(&this->simulationGraphExec, graph, nullptr, nullptr, 0));
        CUDA_CHECK(cudaGraphDestroy(graph));

        this->graphValid = true;
    }

    CUDA_CHECK(cudaGraphLaunch(this->simulationGraphExec, this->simulationStream));
    CUDA_CHECK(cudaStreamSynchronize(this->simulationStream));
}

void Simulation::Grow()
{
    const int newAllocatedCount = this->allocatedParticleCount * 4;

    CUDA_CHECK(cudaStreamSynchronize(this->simulationStream));
    this->DestroyGraphIfValid();

    const int oldBlockCount = Simulation::ParticlesToBlocks(this->particleCount);
    ParticleBlock* oldParticleBlocks = this->particleBlocks;

    this->FreeParticleBuffers();

    CUDA_CHECK(cudaMalloc(&this->particleBlocks, Simulation::ParticlesToBlocks(newAllocatedCount) * sizeof(ParticleBlock)));
    CUDA_CHECK(cudaMemcpy(this->particleBlocks, oldParticleBlocks, oldBlockCount * sizeof(ParticleBlock), cudaMemcpyDeviceToDevice));
    CUDA_CHECK(cudaFree(oldParticleBlocks));

    this->nvidiaCUBTemporaryStorageBytes = 0;
    this->AllocateParticleBuffers(newAllocatedCount);
    CUDA_CHECK(cudaMemset(this->particleHomeBlockCodes, 0xFF, newAllocatedCount * sizeof(uint64_t)));

    this->allocatedParticleCount = newAllocatedCount;
}

void Simulation::SyncPositionsToVBO()
{
    size_t bufferSize;
    float* deviceBuffer;

    CUDA_CHECK(cudaGraphicsMapResources(1, &this->vboResource));
    CUDA_CHECK(cudaGraphicsResourceGetMappedPointer(reinterpret_cast<void**>(&deviceBuffer), &bufferSize, this->vboResource));

    const int launchBlocks = this->ParticleLaunchBlocks();

    WritePositionsKernel<<<launchBlocks, this->threadsPerBlock>>>(this->particleBlocks, deviceBuffer, this->particleCount);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaGraphicsUnmapResources(1, &this->vboResource));
}

void Simulation::UpdatePhysicsParams()
{
    this->hostSimulationParameters.deltaTime = SimulationConfig::physicsTimeStep;
    this->hostSimulationParameters.firstLameParameter = SimulationConfig::firstLameParameter;
    this->hostSimulationParameters.secondLameParameter = SimulationConfig::secondLameParameter;
    this->hostSimulationParameters.hardeningCoefficient = SimulationConfig::hardeningCoefficient;
    this->hostSimulationParameters.criticalCompression = SimulationConfig::criticalCompression;
    this->hostSimulationParameters.criticalStretch = SimulationConfig::criticalStretch;
    this->hostSimulationParameters.boundaryFriction = SimulationConfig::boundaryFriction;

    this->UploadSimParams();
}

void Simulation::ClearMeshBoundary() const
{
    CUDA_CHECK(cudaMemset(this->sdfDistances, 0x7F, SimulationConfig::nodeCount * sizeof(float)));
    CUDA_CHECK(cudaMemset(this->sdfNormals, 0, SimulationConfig::nodeCount * sizeof(glm::vec3)));
}

void Simulation::MoveSledSDF(const int movedCells) const
{
    constexpr dim3 threads(16, 16);
    constexpr dim3 blocks(SimulationConfig::cellCountPerAxis / 16, SimulationConfig::cellCountPerAxis / 16);

    MoveSDFInward<<<blocks, threads>>>(this->sdfDistances, this->sdfNormals, movedCells);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
}

void Simulation::Reset(const SnowVolume& snowVolume)
{
    CUDA_CHECK(cudaStreamSynchronize(this->simulationStream));

    this->DestroyGraphIfValid();
    this->particleCount = snowVolume.GetParticleCount();

    CUDA_CHECK(cudaMemcpy(this->particleBlocks, snowVolume.GetInitialBlocks().data(), snowVolume.GetInitialBlocks().size() * sizeof(ParticleBlock), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemset(this->blockCodeToIndex.keys, 0xFF, SimulationConfig::maxBlocks * sizeof(uint64_t)));
    CUDA_CHECK(cudaMemset(this->nextBlockIndex, 0, sizeof(uint32_t)));
    CUDA_CHECK(cudaMemset(this->gridBlocks, 0, SimulationConfig::maxBlocks * sizeof(GridBlock)));
    CUDA_CHECK(cudaMemset(this->particleHomeBlockCodes, 0xFF, snowVolume.GetParticleCount() * sizeof(uint64_t)));

    this->SetRebuildFlag();
}

void Simulation::UnregisterVBO()
{
    CUDA_CHECK(cudaGraphicsUnregisterResource(this->vboResource));
    this->vboResource = nullptr;
}

void Simulation::RebindVBO(const unsigned int vbo)
{
    this->vboId = vbo;
    CUDA_CHECK(cudaGraphicsGLRegisterBuffer(&this->vboResource, vbo, cudaGraphicsMapFlagsWriteDiscard));
}

void Simulation::SetBoundaryVelocity(const glm::vec3 velocity)
{
    this->hostSimulationParameters.boundaryVelocity = velocity;
    this->UploadSimParams();
}

void Simulation::UploadMeshBoundary(const MeshSDF& sdf) const
{
    CUDA_CHECK(cudaMemcpy(this->sdfDistances, sdf.distances.data(), SimulationConfig::nodeCount * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(this->sdfNormals, sdf.normals.data(), SimulationConfig::nodeCount * sizeof(glm::vec3), cudaMemcpyHostToDevice));
}

void Simulation::AddParticles(const ParticleBlock* blocks, const int additionalParticleCount)
{
    if (this->particleCount + additionalParticleCount > this->allocatedParticleCount)
    {
        //TODO: Register back in scene 3 / where is it used
        CUDA_CHECK(cudaGraphicsUnregisterResource(this->vboResource));
    }

    while (this->particleCount + additionalParticleCount > this->allocatedParticleCount)
    {
        this->Grow();
    }

    const int existingBlockCount = Simulation::ParticlesToBlocks(this->particleCount);
    const int newBlockCount = Simulation::ParticlesToBlocks(additionalParticleCount);
    CUDA_CHECK(cudaMemcpy(this->particleBlocks + existingBlockCount, blocks, newBlockCount * sizeof(ParticleBlock), cudaMemcpyHostToDevice));

    this->particleCount += additionalParticleCount;

    this->SetRebuildFlag();
}

void Simulation::WarpSort()
{
    const int particleBlockCount = Simulation::ParticlesToBlocks(this->particleCount);
    WarpSortKernel<<<particleBlockCount, 32, 0, this->simulationStream>>>(this->particleBlocks, this->particleCount, this->blockCodeToIndex, this->particleHomeBlockCodes);
}

void Simulation::SortParticles()
{
    const int launchBlocks = this->ParticleLaunchBlocks();

    InitIndicesKernel<<<launchBlocks, this->threadsPerBlock, 0, this->simulationStream>>>(this->particleIndices, this->particleCount);
    CUDA_CHECK(cudaGetLastError());

    ComputeSortKeysKernel<<<launchBlocks, this->threadsPerBlock, 0, this->simulationStream>>>(this->particleBlocks, this->particleCount, this->blockCodeToIndex, this->particleSortKeys);
    CUDA_CHECK(cudaGetLastError());

    CUDA_CHECK(cub::DeviceRadixSort::SortPairs(this->nvidiaCUBTemporaryStorage, this->nvidiaCUBTemporaryStorageBytes, this->particleSortKeys, this->particleSortKeysResult, this->particleIndices, this->sortedParticleIndices, this->particleCount, 0, sizeof(uint64_t) * 8, this->simulationStream));

    ReorderParticlesKernel<<<launchBlocks, this->threadsPerBlock, 0, this->simulationStream>>>(this->particleBlocks, this->particleBlocksSortingBuffer, this->sortedParticleIndices, this->particleCount);
    CUDA_CHECK(cudaGetLastError());
}

void Simulation::SetRebuildFlag() const
{
    constexpr uint32_t value = 1u;
    CUDA_CHECK(cudaMemcpy(this->rebuildFlag, &value, sizeof(uint32_t), cudaMemcpyHostToDevice));
}

void Simulation::UploadSimParams() const
{
    CUDA_CHECK(cudaMemcpyToSymbol(simulationParameters, &this->hostSimulationParameters, sizeof(SimulationParameters)));
}

void Simulation::DestroyGraphIfValid()
{
    if (this->graphValid)
    {
        CUDA_CHECK(cudaGraphExecDestroy(this->simulationGraphExec));
        this->graphValid = false;
    }
}

void Simulation::ComputeHomeBlocks() const
{
    const int launchBlocks = this->ParticleLaunchBlocks();
    ComputeHomeBlocksKernel<<<launchBlocks, this->threadsPerBlock, 0, this->simulationStream>>>(this->particleBlocks, this->particleCount, this->particleHomeBlockCodes);
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

void Simulation::AllocateParticleBuffers(const int count)
{
    const int blockCount = Simulation::ParticlesToBlocks(count);

    CUDA_CHECK(cudaMalloc(&this->particleBlocksSortingBuffer, blockCount * sizeof(ParticleBlock)));
    CUDA_CHECK(cudaMalloc(&this->particleSortKeys, count * sizeof(uint64_t)));
    CUDA_CHECK(cudaMalloc(&this->particleSortKeysResult, count * sizeof(uint64_t)));
    CUDA_CHECK(cudaMalloc(&this->particleIndices, count * sizeof(uint32_t)));
    CUDA_CHECK(cudaMalloc(&this->sortedParticleIndices, count * sizeof(uint32_t)));
    CUDA_CHECK(cudaMalloc(&this->particleHomeBlockCodes, count * sizeof(uint64_t)));

    CUDA_CHECK(cub::DeviceRadixSort::SortPairs(nullptr, this->nvidiaCUBTemporaryStorageBytes, this->particleSortKeys, this->particleSortKeysResult, this->particleIndices, this->sortedParticleIndices, count));
    CUDA_CHECK(cudaMalloc(&this->nvidiaCUBTemporaryStorage, this->nvidiaCUBTemporaryStorageBytes));
}

int Simulation::ParticleLaunchBlocks() const
{
    return (this->particleCount + this->threadsPerBlock - 1) / this->threadsPerBlock;
}

int Simulation::ParticlesToBlocks(const int count)
{
    return (count + 31) / 32;
}