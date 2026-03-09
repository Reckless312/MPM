#ifndef MPM_METHOD_MESH_H
#define MPM_METHOD_MESH_H

#include <string>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "OpenGL/Shaders/Shader.h"

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

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    Mesh(const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices, const std::vector<Texture> &textures);

    void Draw(const Shader &shader) const;
private:
    unsigned int VAO{}, VBO{}, EBO{};

    void setupMesh();
};


#endif