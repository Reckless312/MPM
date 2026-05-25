#ifndef MPM_METHOD_SHELL_TEXTURE_H
#define MPM_METHOD_SHELL_TEXTURE_H

#include <glad/glad.h>

#include "OpenGL/Render/RenderObject.h"
#include "OpenGL/Shaders/Shader.h"

class ShellTexture : public RenderObject
{
public:
    int shellCount;

    ShellTexture(const char* colorPath, const char* specularPath, int shellCount);
    ~ShellTexture() override;

    void Bind(const Shader& shader) const;
    void Draw(const Shader& shader) const override;

private:
    GLuint colorTextureArray{};
    GLuint specularTextureArray{};
};

#endif
