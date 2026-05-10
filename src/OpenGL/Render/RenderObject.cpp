#include "OpenGL/Render/RenderObject.h"

RenderObject::RenderObject(RenderObject&& other) noexcept
{
    this->VAO = other.VAO;
    this->VBO = other.VBO;

    other.VAO = 0;
    other.VBO = 0;
}

RenderObject& RenderObject::operator=(RenderObject&& other) noexcept
{
    if (this != &other)
    {
        glDeleteVertexArrays(1, &this->VAO);
        glDeleteBuffers(1, &this->VBO);

        this->VAO = other.VAO;
        this->VBO = other.VBO;

        other.VAO = 0;
        other.VBO = 0;
    }
    return *this;
}

RenderObject::~RenderObject()
{
    glDeleteVertexArrays(1, &this->VAO);
    glDeleteBuffers(1, &this->VBO);
}

unsigned int RenderObject::GetVBO() const
{
    return this->VBO;
}
