#ifndef MPM_METHOD_RENDER_OBJECT_H
#define MPM_METHOD_RENDER_OBJECT_H

#include <glad/glad.h>
#include "OpenGL/Shaders/Shader.h"

class RenderObject {
public:
    virtual ~RenderObject()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    virtual void Draw(const Shader &shader) const = 0;

protected:
    unsigned int VAO{}, VBO{};
};

#endif
