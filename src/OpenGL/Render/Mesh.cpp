#include <array>
#include <glm/vec4.hpp>
#include "OpenGL/Render/Mesh.h"

Mesh::Mesh(const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices, const std::vector<Texture> &textures)
{
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;

    this->setupMesh();
}

void Mesh::setupMesh()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), &indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), static_cast<void *>(nullptr));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, Normal)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, TextureCoordinates)));

    glBindVertexArray(0);
}

void Mesh::Draw(const Shader &shader) const
{
    unsigned int diffuseNumber = 1;
    unsigned int specularNumber = 1;

    for (unsigned int textureId = 0; textureId < textures.size(); textureId++)
    {
        glActiveTexture(GL_TEXTURE0 + textureId);

        std::string number;
        std::string name = textures[textureId].type;

        if(name == "texture_diffuse")
        {
            number = std::to_string(diffuseNumber++);
        }
        else if(name == "texture_specular")
        {
            number = std::to_string(specularNumber++);
        }

        std::string materialName = "material.";
        materialName.append(name);
        materialName.append(number);

        shader.SetInt(materialName, static_cast<int>(textureId));
        glBindTexture(GL_TEXTURE_2D, textures[textureId].id);
    }

    glActiveTexture(GL_TEXTURE0);

    bool hasDiffuse = false;

    for (const auto& texture : textures)
    {
        if (texture.type == "texture_diffuse")
        {
            hasDiffuse = true;
            break;
        }
    }

    shader.SetBool("hasDiffuseTexture", hasDiffuse);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

std::vector<std::array<glm::vec3, 3>> Mesh::GetTriangles() const
{
    std::vector<std::array<glm::vec3, 3>> triangles;

    for (size_t currentIndex = 0; currentIndex < indices.size(); currentIndex += 3)
    {
        triangles.push_back({vertices[indices[currentIndex]].Position, vertices[indices[currentIndex + 1]].Position, vertices[indices[currentIndex + 2]].Position});
    }

    return triangles;
}