#ifndef MPM_METHOD_SIMULATION_H
#define MPM_METHOD_SIMULATION_H

#include <cstddef>
#include <vector>
#include <glm/vec3.hpp>
#include <cuda_runtime.h>
#include "SimulationParameters.h"
#include "OpenGL/Simulation/MeshBoundary.h"
#include "OpenGL/Simulation/SnowVolume.h"
#include "Structures/GridBuffer.h"
#include "Structures/HashTable.h"
#include "Structures/ParticleBuffer.h"

class Simulation
{
public:
    Simulation(const SnowVolume& snowVolume, unsigned int vbo, int maxCapacity);
    ~Simulation();

    void Step();
    void SyncPositionsToVBO();
    void UpdatePhysicsParams();
    void ClearMeshBoundary() const;
    void MoveSledSDF(int movedCells) const;
    void Reset(const SnowVolume& snowVolume, int newMaxCapacity);
    void AddParticles(const SnowVolume& snowVolume);
    [[nodiscard]] int GetParticleCount() const;
    void SetBoundaryVelocity(glm::vec3 velocity);
    void UploadMeshBoundary(const MeshSDF& sdf) const;
    void UnregisterVBO();
    void RebindVBO(unsigned int vbo);

private:
    ParticleBlock* particleBlocks{};
    ParticleBlock* particleBlocksSortingBuffer{};

    GridBlock* gridBlocks{};

    SimulationParameters hostSimulationParameters{};

    HashTable blockCodeToIndex{};

    cudaGraphicsResource* vboResource{};
    cudaStream_t simulationStream{};
    cudaGraphExec_t simulationGraphExec{};

    glm::vec3* sdfNormals{};

    uint64_t* blockCodes{};
    uint64_t* particleSortKeys{};
    uint64_t* particleSortKeysResult{};
    uint64_t* particleHomeBlockCodes{};

    uint32_t* nextBlockIndex{};
    uint32_t* particleIndices{};
    uint32_t* sortedParticleIndices{};
    uint32_t* rebuildFlag{};

    uint32_t activeBlockCount = 0;
    size_t nvidiaCUBTemporaryStorageBytes = 0;

    float* sdfDistances{};
    void* nvidiaCUBTemporaryStorage = nullptr;

    unsigned int vboId{};
    const int threadsPerBlock = 128;

    bool graphValid = false;

    int particleCount;
    int maxCapacity;

    void WarpSort();
    void SortParticles();
    void SetRebuildFlag() const;
    void UploadSimParams() const;
    void DestroyGraphIfValid();
    void ComputeHomeBlocks() const;
    void FreeParticleBuffers() const;
    void AllocateParticleBuffers(int count);

    [[nodiscard]] int ParticleLaunchBlocks() const;
    [[nodiscard]] static int ParticlesToBlocks(int count);
};

#endif
