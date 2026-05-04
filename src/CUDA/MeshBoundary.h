#ifndef MPM_METHOD_MESH_BOUNDARY_H
#define MPM_METHOD_MESH_BOUNDARY_H

#include <array>
#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>

class MeshBoundary
{
public:
    static std::vector<uint8_t> Voxelize(
        const std::vector<std::array<glm::vec3, 3>>& triangles,
        int cellCountPerAxis,
        float cellSize
    );
};

#endif
