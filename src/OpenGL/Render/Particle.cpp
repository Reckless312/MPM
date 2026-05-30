#include "OpenGL/Render/Particle.h"

Particle::Particle(const std::vector<glm::vec3> &positions)
{
    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &this->VBO);

    this->count = static_cast<int>(positions.size());

    glBindVertexArray(this->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);

    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(positions.size() * sizeof(glm::vec3)), positions.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), static_cast<void *>(nullptr));

    glBindVertexArray(0);
}

Particle::~Particle()
{
    glDeleteVertexArrays(1, &this->VAO);
    glDeleteBuffers(1, &this->VBO);
}

unsigned int Particle::GetVBO() const
{
    return this->VBO;
}

void Particle::Draw() const
{
    glBindVertexArray(this->VAO);
    glDrawArrays(GL_POINTS, 0, this->count);

    glBindVertexArray(0);
}

void Particle::ResizeVBO(const int newCount)
{
    this->count = newCount;

    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, newCount * static_cast<GLsizeiptr>(sizeof(glm::vec3)), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
