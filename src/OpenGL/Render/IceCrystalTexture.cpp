#include "OpenGL/Render/IceCrystalTexture.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

#include <glm/glm.hpp>

IceCrystalTexture::IceCrystalTexture()
{
    BuildCrystalVolume();
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

int IceCrystalTexture::VoxelIndex(int x, int y, int z) const
{
    return z * volumeSize * volumeSize + y * volumeSize + x;
}

bool IceCrystalTexture::IsInBounds(int x, int y, int z) const
{
    return x >= 0 && x < volumeSize
        && y >= 0 && y < volumeSize
        && z >= 0 && z < volumeSize;
}

void IceCrystalTexture::BuildCrystalVolume()
{
    const int totalVoxels = volumeSize * volumeSize * volumeSize;
    volume.assign(totalVoxels, CrystalVoxel{});

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> seedDist(-seedRadius, seedRadius);
    std::uniform_real_distribution<float> colorDist(minCrystalBrightness, 1.0f);
    std::uniform_real_distribution<float> normalDist(-1.0f, 1.0f);

    const std::array<glm::ivec3, 6> neighborDirections = {
        glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0),
        glm::ivec3(0, 1, 0), glm::ivec3(0, -1, 0),
        glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1)
    };

    for (int crystal = 0; crystal < crystalCount; crystal++)
    {
        glm::vec3 seedPos;

        do
        {
            seedPos = glm::vec3(seedDist(rng), seedDist(rng), seedDist(rng));
        }
        while (glm::dot(seedPos, seedPos) > seedRadius * seedRadius);

        const glm::ivec3 seedVoxel = glm::ivec3(
            static_cast<int>(std::floor((seedPos.x + 1.0f) * 0.5f * volumeSize)),
            static_cast<int>(std::floor((seedPos.y + 1.0f) * 0.5f * volumeSize)),
            static_cast<int>(std::floor((seedPos.z + 1.0f) * 0.5f * volumeSize))
        );

        if (!IsInBounds(seedVoxel.x, seedVoxel.y, seedVoxel.z))
        {
            continue;
        }

        auto& seedCell = volume[VoxelIndex(seedVoxel.x, seedVoxel.y, seedVoxel.z)];
        seedCell.occupied = true;
        seedCell.color = glm::vec3(colorDist(rng), colorDist(rng), colorDist(rng));

        glm::vec3 randomNormal;
        do
        {
            randomNormal = glm::vec3(normalDist(rng), normalDist(rng), normalDist(rng));
        }
        while (glm::dot(randomNormal, randomNormal) > 1.0f);
        seedCell.reflectionNormal = glm::normalize(randomNormal);

        std::vector<glm::ivec3> occupiedPositions = { seedVoxel };
        int grown = 0;

        while (grown < voxelsPerCrystal && !occupiedPositions.empty())
        {
            std::uniform_int_distribution<int> pickDist(0, static_cast<int>(occupiedPositions.size()) - 1);
            const glm::ivec3 current = occupiedPositions[pickDist(rng)];

            std::array<int, 6> dirOrder = { 0, 1, 2, 3, 4, 5 };
            std::shuffle(dirOrder.begin(), dirOrder.end(), rng);

            bool expanded = false;

            for (int d : dirOrder)
            {
                const glm::ivec3 neighbor = current + neighborDirections[d];

                if (!IsInBounds(neighbor.x, neighbor.y, neighbor.z))
                {
                    continue;
                }

                auto& neighborCell = volume[VoxelIndex(neighbor.x, neighbor.y, neighbor.z)];

                if (neighborCell.occupied)
                {
                    continue;
                }

                neighborCell.occupied = true;
                neighborCell.color = glm::vec3(colorDist(rng), colorDist(rng), colorDist(rng));

                glm::vec3 normal;
                do
                {
                    normal = glm::vec3(normalDist(rng), normalDist(rng), normalDist(rng));
                }
                while (glm::dot(normal, normal) > 1.0f);
                neighborCell.reflectionNormal = glm::normalize(normal);

                occupiedPositions.push_back(neighbor);
                grown++;
                expanded = true;
                break;
            }

            if (!expanded)
            {
                occupiedPositions.erase(
                    std::remove(occupiedPositions.begin(), occupiedPositions.end(), current),
                    occupiedPositions.end()
                );
            }
        }
    }
}

void IceCrystalTexture::BuildShellTextures()
{
    const int pixelsPerLayer = textureResolution * textureResolution;
    const int colorBytesPerTexel = 4;
    const int specularBytesPerTexel = 3;

    std::vector<uint8_t> colorData(pixelsPerLayer * shellCount * colorBytesPerTexel, 0);
    std::vector<uint8_t> specularData(pixelsPerLayer * shellCount * specularBytesPerTexel, 0);

    for (int shell = 0; shell < shellCount; shell++)
    {
        const float t = static_cast<float>(shell) / static_cast<float>(shellCount - 1);
        const float shellRadius = innerShellFraction + (outerShellFraction - innerShellFraction) * t;

        for (int v = 0; v < textureResolution; v++)
        {
            for (int u = 0; u < textureResolution; u++)
            {
                const float theta = (static_cast<float>(u) + 0.5f) / textureResolution * 2.0f * pi;
                const float phi   = (static_cast<float>(v) + 0.5f) / textureResolution * pi;

                const float sinPhi = std::sin(phi);
                const glm::vec3 samplePoint = glm::vec3(
                    shellRadius * sinPhi * std::cos(theta),
                    shellRadius * std::cos(phi),
                    shellRadius * sinPhi * std::sin(theta)
                );

                const int vx = static_cast<int>(std::floor((samplePoint.x + 1.0f) * 0.5f * volumeSize));
                const int vy = static_cast<int>(std::floor((samplePoint.y + 1.0f) * 0.5f * volumeSize));
                const int vz = static_cast<int>(std::floor((samplePoint.z + 1.0f) * 0.5f * volumeSize));

                const int colorOffset    = (shell * pixelsPerLayer + v * textureResolution + u) * colorBytesPerTexel;
                const int specularOffset = (shell * pixelsPerLayer + v * textureResolution + u) * specularBytesPerTexel;

                if (!IsInBounds(vx, vy, vz))
                {
                    continue;
                }

                const CrystalVoxel& voxel = volume[VoxelIndex(vx, vy, vz)];

                if (!voxel.occupied)
                {
                    continue;
                }

                colorData[colorOffset + 0] = static_cast<uint8_t>(voxel.color.r * 255.0f);
                colorData[colorOffset + 1] = static_cast<uint8_t>(voxel.color.g * 255.0f);
                colorData[colorOffset + 2] = static_cast<uint8_t>(voxel.color.b * 255.0f);
                colorData[colorOffset + 3] = 255;

                specularData[specularOffset + 0] = static_cast<uint8_t>((voxel.reflectionNormal.x + 1.0f) * 0.5f * 255.0f);
                specularData[specularOffset + 1] = static_cast<uint8_t>((voxel.reflectionNormal.y + 1.0f) * 0.5f * 255.0f);
                specularData[specularOffset + 2] = static_cast<uint8_t>((voxel.reflectionNormal.z + 1.0f) * 0.5f * 255.0f);
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
