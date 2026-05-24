#ifndef MPM_METHOD_ICE_CRYSTAL_TEXTURE_H
#define MPM_METHOD_ICE_CRYSTAL_TEXTURE_H

#include <glad/glad.h>

#include "OpenGL/Shaders/Shader.h"

class IceCrystalTexture
{
public:
    static constexpr int shellCount = 8;

    IceCrystalTexture();
    ~IceCrystalTexture();

    void Bind(const Shader& shader) const;

private:
    static constexpr int textureResolution = 64;
    static constexpr int cellGridSize = 6;
    static constexpr float minCrystalBrightness = 0.85f;
    static constexpr float pi = 3.14159265358979f;

    GLuint colorTextureArray{};
    GLuint specularTextureArray{};

    void BuildShellTextures();
};

#endif
