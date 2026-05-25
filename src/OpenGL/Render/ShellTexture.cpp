#include "OpenGL/Render/ShellTexture.h"

#include "OpenGL/Shaders/TextureLoader.h"

ShellTexture::ShellTexture(const char* colorPath, const char* specularPath, const int shellCount)
    : shellCount(shellCount)
{
    colorTextureArray = TextureLoader::LoadTextureArray(colorPath, shellCount);
    specularTextureArray = TextureLoader::LoadTextureArray(specularPath, shellCount);
}

ShellTexture::~ShellTexture()
{
    glDeleteTextures(1, &colorTextureArray);
    glDeleteTextures(1, &specularTextureArray);
}

void ShellTexture::Bind(const Shader& shader) const
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, colorTextureArray);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, specularTextureArray);
    shader.SetInt("shellColorTextures", 0);
    shader.SetInt("shellSpecularTextures", 1);
    shader.SetInt("totalShells", shellCount);
}

void ShellTexture::Draw(const Shader& shader) const
{
    Bind(shader);
}
