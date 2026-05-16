#ifndef MPM_METHOD_MESH_BOUNDARY_H
#define MPM_METHOD_MESH_BOUNDARY_H

#include <array>
#include <vector>

#include <glm/vec3.hpp>

struct MeshSDF
{
    std::vector<float> distances;
    std::vector<glm::vec3> normals;
};

class MeshBoundary
{
public:
    static MeshSDF Voxelize(const std::vector<std::array<glm::vec3, 3>>& triangles, int cellCountPerAxis, float cellSize);

private:
    static glm::vec3 ClosestPointOnTriangle(glm::vec3 point, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2);
};

#endif
