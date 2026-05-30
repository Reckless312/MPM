#ifndef MPM_METHOD_MOVE_SDF_H
#define MPM_METHOD_MOVE_SDF_H

#include <glm/vec3.hpp>

#ifdef __CUDACC__
__global__ void MoveSDFInward(float* sdfDistances, glm::vec3* sdfNormals, int shiftCells);
#endif

#endif
