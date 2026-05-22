#ifndef MPM_METHOD_SIMULATION_H
#define MPM_METHOD_SIMULATION_H

#include <cstddef>
#include <vector>

#include <glm/vec3.hpp>

#include "Structures/ParticleBuffer.h"
#include "Structures/GridBuffer.h"
#include "Structures/HashTable.h"
#include "OpenGL/Simulation/MeshBoundary.h"

struct cudaGraphicsResource;
struct CUstream_st;
typedef CUstream_st* cudaStream_t;
struct CUgraphExec_st;
typedef CUgraphExec_st* cudaGraphExec_t;

class Simulation
{
public:
    Simulation(int maxParticleCount, int initialParticleCount, const ParticleBlock* initialParticleBlocks, int initialParticleBlockCount, unsigned int vbo);
    ~Simulation();
    void Step();
    void SyncPositionsToVBO();
    void UploadMeshBoundary(const MeshSDF& sdf) const;
    void ClearMeshBoundary() const;
    void SetBoundaryVelocity(glm::vec3 velocity) const;
    void ShiftSdfZ(int cells);
    void Reset(const ParticleBlock* initialBlocks, int blockCount, int newParticleCount);
    void AddParticles(const ParticleBlock* blocks, int blockCount, int additionalParticleCount);
    int Grow();
    void RebindVBO();

private:
    int particleCount;
    int allocatedParticleCount;

    ParticleBlock* particleBlocks{};
    ParticleBlock* particleBlocksSortingBuffer{};
    GridBlock* gridBlocks{};

    HashTable blockCodeToIndex{};
    uint32_t* nextBlockIndex{};
    uint64_t* blockCodes{};

    uint64_t* particleSortKeys{};
    uint64_t* particleSortKeysResult{};
    uint32_t* particleIndices{};
    uint32_t* sortedParticleIndices{};

    uint64_t* particleHomeBlockCodes{};
    uint32_t* rebuildFlag{};

    void* nvidiaCUBTemporaryStorage = nullptr;

    size_t nvidiaCUBTemporaryStorageBytes = 0;

    cudaGraphicsResource* vboResource{};
    unsigned int vboId{};
    float* sdfDistances{};
    glm::vec3* sdfNormals{};
    glm::vec3* boundaryVelocity{};

    cudaStream_t simulationStream{};
    cudaGraphExec_t simulationGraphExec{};
    bool graphValid = false;

    void AllocateParticleBuffers(int count);
    void FreeParticleBuffers() const;

    const int threadsPerBlock = 128;

    uint32_t activeBlockCount = 0;
};

#endif
