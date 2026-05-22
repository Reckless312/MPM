#ifndef MPM_METHOD_PARTICLE_H
#define MPM_METHOD_PARTICLE_H

#include <vector>
#include <glm/vec3.hpp>

#include "OpenGL/Render/RenderObject.h"

class Particle : public RenderObject {
public:
    explicit Particle(const std::vector<glm::vec3> &positions);

    void Update(const std::vector<glm::vec3> &positions);
    void ResizeVBO(int newCount);
    void Draw(const Shader &shader) const override;

private:
    int count{};

    void setupParticle();
};

#endif
