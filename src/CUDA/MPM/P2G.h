#ifndef MPM_METHOD_P2G_H
#define MPM_METHOD_P2G_H

#include "../Structures/ParticleBuffer.h"
#include "../Structures/GridBuffer.h"
#include "../Structures/HashTable.h"

#ifdef __CUDACC__
__global__ void P2GKernel(const ParticleBlock* particleBlocks, GridBlock* gridBlocks, int particleCount, const HashTable& blockCodeToIndex);
__device__ void ComputeBSplineWeights(float fractionalX, float fractionalY, float fractionalZ, float weightsX[3], float weightsY[3], float weightsZ[3]);
#endif

#endif
