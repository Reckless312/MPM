#ifndef MPM_METHOD_G2P_H
#define MPM_METHOD_G2P_H

#include "../Structures/ParticleBuffer.h"
#include "../Structures/GridBuffer.h"
#include "../Structures/HashTable.h"

#ifdef __CUDACC__
__global__ void g2pKernel(ParticleBlock* particleBlocks, const GridBlock* gridBlocks, int particleCount, const HashTable& hashTable, float cellSize, float deltaTime, float criticalCompression, float criticalStretch);
#endif

#endif
