#include "OpenGL/Render/IceCrystalTexture.h"

#include <cmath>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

IceCrystalTexture::IceCrystalTexture()
{
    BuildShellTextures();
}

IceCrystalTexture::~IceCrystalTexture()
{
    glDeleteTextures(1, &colorTextureArray);
    glDeleteTextures(1, &specularTextureArray);
}

void IceCrystalTexture::Bind(const Shader& shader) const
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, colorTextureArray);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, specularTextureArray);
    shader.SetInt("shellColorTextures", 0);
    shader.SetInt("shellSpecularTextures", 1);
    shader.SetInt("totalShells", shellCount);
}

static glm::vec3 CellColor(int cellIndex)
{
    const float brightness = minCrystalBrightness + (1.0f - minCrystalBrightness)
        * std::fmod(static_cast<float>(cellIndex * 91 + 37) / 256.0f, 1.0f);
    const float tint = std::fmod(static_cast<float>(cellIndex * 127) / 256.0f, 1.0f);
    return glm::vec3(brightness, brightness + 0.04f * tint, brightness + 0.08f * tint);
}

static glm::vec3 CellNormal(int cellIndex)
{
    const float nx = std::fmod(static_cast<float>(cellIndex * 173 + 13) / 256.0f, 1.0f) * 2.0f - 1.0f;
    const float ny = std::fmod(static_cast<float>(cellIndex * 211 + 71) / 256.0f, 1.0f) * 2.0f - 1.0f;
    const float nz = std::fmod(static_cast<float>(cellIndex * 149 + 53) / 256.0f, 1.0f) * 2.0f - 1.0f;
    return glm::normalize(glm::vec3(nx, ny, nz));
}

void IceCrystalTexture::BuildShellTextures()
{
    const int pixelsPerLayer = textureResolution * textureResolution;

    std::vector<uint8_t> colorData(pixelsPerLayer * shellCount * 4, 0);
    std::vector<uint8_t> specularData(pixelsPerLayer * shellCount * 3, 0);

    for (int shell = 0; shell < shellCount; shell++)
    {
        const int shellOffset = shell * cellGridSize * cellGridSize;

        for (int v = 0; v < textureResolution; v++)
        {
            for (int u = 0; u < textureResolution; u++)
            {
                const int cellU = u * cellGridSize / textureResolution;
                const int cellV = v * cellGridSize / textureResolution;
                const int cellIndex = shellOffset + cellV * cellGridSize + cellU;

                const glm::vec3 color  = CellColor(cellIndex);
                const glm::vec3 normal = CellNormal(cellIndex);

                const int colorOffset    = (shell * pixelsPerLayer + v * textureResolution + u) * 4;
                const int specularOffset = (shell * pixelsPerLayer + v * textureResolution + u) * 3;

                colorData[colorOffset + 0] = static_cast<uint8_t>(color.r * 255.0f);
                colorData[colorOffset + 1] = static_cast<uint8_t>(color.g * 255.0f);
                colorData[colorOffset + 2] = static_cast<uint8_t>(color.b * 255.0f);
                colorData[colorOffset + 3] = 255;

                specularData[specularOffset + 0] = static_cast<uint8_t>((normal.x + 1.0f) * 0.5f * 255.0f);
                specularData[specularOffset + 1] = static_cast<uint8_t>((normal.y + 1.0f) * 0.5f * 255.0f);
                specularData[specularOffset + 2] = static_cast<uint8_t>((normal.z + 1.0f) * 0.5f * 255.0f);
            }
        }
    }

    glGenTextures(1, &colorTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, colorTextureArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
                 textureResolution, textureResolution, shellCount,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, colorData.data());
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    glGenTextures(1, &specularTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, specularTextureArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8,
                 textureResolution, textureResolution, shellCount,
                 0, GL_RGB, GL_UNSIGNED_BYTE, specularData.data());
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}
