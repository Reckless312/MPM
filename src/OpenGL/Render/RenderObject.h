#ifndef MPM_METHOD_RENDER_OBJECT_H
#define MPM_METHOD_RENDER_OBJECT_H

#include <glad/glad.h>
#include "OpenGL/Shaders/Shader.h"

class RenderObject {
public:
    RenderObject() = default;

    RenderObject(RenderObject&& other) noexcept : VAO(other.VAO), VBO(other.VBO)
    {
        other.VAO = 0;
        other.VBO = 0;
    }

    RenderObject& operator=(RenderObject&& other) noexcept
    {
        if (this != &other)
        {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            VAO = other.VAO;
            VBO = other.VBO;
            other.VAO = 0;
            other.VBO = 0;
        }
        return *this;
    }

    RenderObject(const RenderObject&) = delete;
    RenderObject& operator=(const RenderObject&) = delete;

    virtual ~RenderObject()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    unsigned int GetVBO() const { return VBO; }

    virtual void Draw(const Shader &shader) const = 0;

protected:
    unsigned int VAO{}, VBO{};
};

#endif
