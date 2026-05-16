#include "OpenGL/Render/Box.h"

Box::Box(glm::vec3 center, glm::vec3 halfExtents)
    : mesh(BuildMesh(center, halfExtents))
{
}

void Box::Draw(const Shader& shader) const
{
    mesh.Draw(shader);
}

Mesh Box::BuildMesh(glm::vec3 center, glm::vec3 halfExtents)
{
    const glm::vec3 c = center;
    const glm::vec3 h = halfExtents;

    // 24 vertices (4 per face, 6 faces) with per-face normals
    std::vector<Vertex> vertices = {
        // +X
        {{c.x+h.x, c.y-h.y, c.z-h.z}, {1,0,0}, {0,0}},
        {{c.x+h.x, c.y+h.y, c.z-h.z}, {1,0,0}, {0,1}},
        {{c.x+h.x, c.y+h.y, c.z+h.z}, {1,0,0}, {1,1}},
        {{c.x+h.x, c.y-h.y, c.z+h.z}, {1,0,0}, {1,0}},
        // -X
        {{c.x-h.x, c.y-h.y, c.z+h.z}, {-1,0,0}, {0,0}},
        {{c.x-h.x, c.y+h.y, c.z+h.z}, {-1,0,0}, {0,1}},
        {{c.x-h.x, c.y+h.y, c.z-h.z}, {-1,0,0}, {1,1}},
        {{c.x-h.x, c.y-h.y, c.z-h.z}, {-1,0,0}, {1,0}},
        // +Y
        {{c.x-h.x, c.y+h.y, c.z-h.z}, {0,1,0}, {0,0}},
        {{c.x-h.x, c.y+h.y, c.z+h.z}, {0,1,0}, {0,1}},
        {{c.x+h.x, c.y+h.y, c.z+h.z}, {0,1,0}, {1,1}},
        {{c.x+h.x, c.y+h.y, c.z-h.z}, {0,1,0}, {1,0}},
        // -Y
        {{c.x-h.x, c.y-h.y, c.z+h.z}, {0,-1,0}, {0,0}},
        {{c.x-h.x, c.y-h.y, c.z-h.z}, {0,-1,0}, {0,1}},
        {{c.x+h.x, c.y-h.y, c.z-h.z}, {0,-1,0}, {1,1}},
        {{c.x+h.x, c.y-h.y, c.z+h.z}, {0,-1,0}, {1,0}},
        // +Z
        {{c.x-h.x, c.y-h.y, c.z+h.z}, {0,0,1}, {0,0}},
        {{c.x+h.x, c.y-h.y, c.z+h.z}, {0,0,1}, {0,1}},
        {{c.x+h.x, c.y+h.y, c.z+h.z}, {0,0,1}, {1,1}},
        {{c.x-h.x, c.y+h.y, c.z+h.z}, {0,0,1}, {1,0}},
        // -Z
        {{c.x+h.x, c.y-h.y, c.z-h.z}, {0,0,-1}, {0,0}},
        {{c.x-h.x, c.y-h.y, c.z-h.z}, {0,0,-1}, {0,1}},
        {{c.x-h.x, c.y+h.y, c.z-h.z}, {0,0,-1}, {1,1}},
        {{c.x+h.x, c.y+h.y, c.z-h.z}, {0,0,-1}, {1,0}},
    };

    std::vector<unsigned int> indices;
    indices.reserve(36);

    for (unsigned int face = 0; face < 6; face++)
    {
        unsigned int base = face * 4;
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    return Mesh(vertices, indices, {});
}
