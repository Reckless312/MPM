#ifndef MPM_METHOD_P2G_H
#define MPM_METHOD_P2G_H

#include "ParticleBuffer.h"
#include "GridBuffer.h"
#include "HashTable.h"

#ifdef __CUDACC__
__device__ void computeBSplineWeights(float fractionalX, float fractionalY, float fractionalZ, float weightsX[3], float weightsY[3], float weightsZ[3]);
__global__ void p2gKernel(const ParticleBlock* particleBlocks, GridBlock* gridBlocks, int particleCount, const HashTable& hashTable, float cellSize, float deltaTime, float shearModulus, float firstLameParameter);
#endif

#endif
