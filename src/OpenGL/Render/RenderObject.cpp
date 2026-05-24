#include "OpenGL/Render/RenderObject.h"


RenderObject::~RenderObject()
{
    glDeleteVertexArrays(1, &this->VAO);
    glDeleteBuffers(1, &this->VBO);
}

unsigned int RenderObject::GetVBO() const
{
    return this->VBO;
}
