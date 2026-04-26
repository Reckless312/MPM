#include "OpenGL/Render/Particle.h"

Particle::Particle(const std::vector<glm::vec3> &positions)
{
    this->count = static_cast<int>(positions.size());
    this->setupParticle();
    this->Update(positions);
}

void Particle::Update(const std::vector<glm::vec3> &positions)
{
    this->count = static_cast<int>(positions.size());

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(positions.size() * sizeof(glm::vec3)), positions.data(), GL_DYNAMIC_DRAW);
}

void Particle::Draw(const Shader &shader) const
{
    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, this->count);
    glBindVertexArray(0);
}

void Particle::setupParticle()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), static_cast<void *>(nullptr));

    glBindVertexArray(0);
}
