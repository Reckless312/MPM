#include "OpenGL/Render/Mesh.h"

#include <glm/vec4.hpp>

Mesh::Mesh(const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices, const std::vector<Texture> &textures)
{
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;

    this->setupMesh();
}

Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &this->VAO);
    glDeleteBuffers(1, &this->VBO);
    glDeleteBuffers(1, &this->EBO);
}

Mesh::Mesh(Mesh&& other) noexcept
{
    *this = std::move(other);
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other)
    {
        glDeleteVertexArrays(1, &this->VAO);
        glDeleteBuffers(1, &this->VBO);
        glDeleteBuffers(1, &this->EBO);

        this->vertices = std::move(other.vertices);
        this->textures = std::move(other.textures);
        this->indices = std::move(other.indices);

        this->VAO = other.VAO;
        this->VBO = other.VBO;
        this->EBO = other.EBO;

        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
    }

    return *this;
}

std::vector<std::vector<glm::vec3>> Mesh::GetTriangles() const
{
    std::vector<std::vector<glm::vec3>> triangles;

    for (size_t currentIndex = 0; currentIndex < this->indices.size(); currentIndex += 3)
    {
        triangles.push_back({this->vertices[this->indices[currentIndex]].position, this->vertices[this->indices[currentIndex + 1]].position, this->vertices[this->indices[currentIndex + 2]].position});
    }

    return triangles;
}

void Mesh::Draw(const Shader &shader) const
{
    unsigned int diffuseNumber = 1;
    unsigned int specularNumber = 1;

    for (unsigned int textureId = 0; textureId < this->textures.size(); textureId++)
    {
        glActiveTexture(GL_TEXTURE0 + textureId);

        std::string number;
        std::string name = this->textures[textureId].type;

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
        glBindTexture(GL_TEXTURE_2D, this->textures[textureId].id);
    }

    glActiveTexture(GL_TEXTURE0);

    bool hasDiffuse = false;

    for (const auto& texture : this->textures)
    {
        if (texture.type == "texture_diffuse")
        {
            hasDiffuse = true;
            break;
        }
    }

    shader.SetBool("hasDiffuseTexture", hasDiffuse);

    glBindVertexArray(this->VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(this->indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Mesh::setupMesh()
{
    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &this->VBO);
    glGenBuffers(1, &this->EBO);

    glBindVertexArray(this->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(this->vertices.size() * sizeof(Vertex)), &this->vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(this->indices.size() * sizeof(unsigned int)), &this->indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), static_cast<void *>(nullptr));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, normal)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, textureCoordinates)));

    glBindVertexArray(0);
}