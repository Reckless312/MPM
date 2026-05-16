#include "OpenGL/Render/Box.h"

Box::Box(glm::vec3 halfExtents)
    : mesh(BuildMesh(halfExtents))
{
}

void Box::Draw(const Shader& shader) const
{
    mesh.Draw(shader);
}

Mesh Box::BuildMesh(glm::vec3 halfExtents)
{
    const glm::vec3 h = halfExtents;

    std::vector<Vertex> vertices = {
        // +X
        {{h.x, -h.y, -h.z}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{h.x,  h.y, -h.z}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{h.x,  h.y,  h.z}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{h.x, -h.y,  h.z}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        // -X
        {{-h.x, -h.y,  h.z}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-h.x,  h.y,  h.z}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{-h.x,  h.y, -h.z}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{-h.x, -h.y, -h.z}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        // +Y
        {{-h.x, h.y, -h.z}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{-h.x, h.y,  h.z}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ h.x, h.y,  h.z}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ h.x, h.y, -h.z}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        // -Y
        {{-h.x, -h.y,  h.z}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{-h.x, -h.y, -h.z}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ h.x, -h.y, -h.z}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ h.x, -h.y,  h.z}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
        // +Z
        {{-h.x, -h.y, h.z}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ h.x, -h.y, h.z}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{ h.x,  h.y, h.z}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-h.x,  h.y, h.z}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        // -Z
        {{ h.x, -h.y, -h.z}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{-h.x, -h.y, -h.z}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
        {{-h.x,  h.y, -h.z}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
        {{ h.x,  h.y, -h.z}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
    };

    std::vector<unsigned int> indices;
    indices.reserve(36);

    for (unsigned int face = 0; face < 6; face++)
    {
        const unsigned int base = face * 4;
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    return Mesh(vertices, indices, {});
}
