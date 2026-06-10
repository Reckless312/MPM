#ifndef MPM_METHOD_PARTICLE_H
#define MPM_METHOD_PARTICLE_H

#include <vector>
#include <glm/vec3.hpp>
#include <glad/glad.h>
#include "OpenGL/Shaders/Shader.h"

class Particle
{
public:
    explicit Particle(const std::vector<glm::vec3> &positions);
    ~Particle();

    [[nodiscard]] unsigned int GetVBO() const;

    void Draw() const;
    void ResizeVBO(int newCount);
    void SetCount(int newCount);

private:
    unsigned int VAO{}, VBO{};

    int count{};
};

#endif
