#ifndef MPM_METHOD_MESH_BOUNDARY_H
#define MPM_METHOD_MESH_BOUNDARY_H

#include <array>
#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>

class MeshBoundary
{
public:
    static std::vector<uint8_t> Voxelize(const std::vector<std::array<glm::vec3, 3>>& triangles, int cellCountPerAxis, float cellSize);

private:
    static float PointTriangleDistance(glm::vec3 point, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2);
};

#endif
