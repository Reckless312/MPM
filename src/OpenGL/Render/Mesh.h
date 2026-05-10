#ifndef MPM_METHOD_MESH_H
#define MPM_METHOD_MESH_H

#include <array>
#include <string>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "OpenGL/Shaders/Shader.h"
#include "OpenGL/Render/RenderObject.h"

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TextureCoordinates;
};


struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

class Mesh : public RenderObject {
public:
    Mesh(const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices, const std::vector<Texture> &textures);

    void Draw(const Shader &shader) const override;

    [[nodiscard]] std::vector<std::array<glm::vec3, 3>> GetTriangles() const;
private:
    unsigned int EBO{};

    std::vector<Vertex> vertices;
    std::vector<Texture> textures;
    std::vector<unsigned int> indices;

    void setupMesh();
};


#endif