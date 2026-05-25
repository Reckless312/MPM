#ifndef MPM_METHOD_P2G_H
#define MPM_METHOD_P2G_H

#include "../Structures/ParticleBuffer.h"
#include "../Structures/GridBuffer.h"
#include "../Structures/HashTable.h"

#ifdef __CUDACC__
__device__ void ComputeBSplineWeights(float fractionalX, float fractionalY, float fractionalZ, float weightsX[3], float weightsY[3], float weightsZ[3]);
__device__ uint64_t ComputeParticleBlockCode(float positionX, float positionY, float positionZ, float cellSize);
__global__ void P2GKernel(const ParticleBlock* particleBlocks, GridBlock* gridBlocks, int particleCount, const HashTable& blockCodeToIndex, bool shouldRecordHomeBlocks, uint64_t* particleHomeBlockCodes);
#endif

#endif
