#ifndef MPM_METHOD_MESH_BOUNDARY_H
#define MPM_METHOD_MESH_BOUNDARY_H

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
    static MeshSDF BuildSDF(const std::vector<std::vector<glm::vec3>>& triangles);

private:
    static glm::vec3 ClosestPointOnTriangle(glm::vec3 p, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2);
};

#endif
