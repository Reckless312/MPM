#ifndef MPM_METHOD_BOX_H
#define MPM_METHOD_BOX_H

#include <glm/vec3.hpp>

#include "OpenGL/Render/Mesh.h"
#include "OpenGL/Shaders/Shader.h"

class Box
{
public:
    Box(glm::vec3 center, glm::vec3 halfExtents);

    void Draw(const Shader& shader) const;

private:
    Mesh mesh;

    static Mesh BuildMesh(glm::vec3 center, glm::vec3 halfExtents);
};

#endif
