#ifndef MPM_METHOD_ICE_CRYSTAL_TEXTURE_H
#define MPM_METHOD_ICE_CRYSTAL_TEXTURE_H

#include <vector>

#include <glad/glad.h>
#include <glm/vec3.hpp>

#include "OpenGL/Shaders/Shader.h"

class IceCrystalTexture
{
public:
    static constexpr int shellCount = 8;

    IceCrystalTexture();
    ~IceCrystalTexture();

    void Bind(const Shader& shader) const;

private:
    static constexpr int volumeSize = 16;
    static constexpr int crystalCount = 30;
    static constexpr int voxelsPerCrystal = 48;
    static constexpr int textureResolution = 64;
    static constexpr float innerShellFraction = 0.85f;
    static constexpr float outerShellFraction = 1.0f;
    static constexpr float seedRadius = 0.7f;
    static constexpr float minCrystalBrightness = 0.85f;
    static constexpr float pi = 3.14159265358979f;

    struct CrystalVoxel
    {
        bool occupied = false;
        glm::vec3 color{};
        glm::vec3 reflectionNormal{};
    };

    GLuint colorTextureArray{};
    GLuint specularTextureArray{};

    std::vector<CrystalVoxel> volume;

    void BuildCrystalVolume();
    void BuildShellTextures();

    [[nodiscard]] int VoxelIndex(int x, int y, int z) const;
    [[nodiscard]] bool IsInBounds(int x, int y, int z) const;
};

#endif
