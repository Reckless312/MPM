#include "MeshBoundary.h"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>

static float PointTriangleDistance(glm::vec3 point, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2)
{
    glm::vec3 edge01 = v1 - v0;
    glm::vec3 edge02 = v2 - v0;
    glm::vec3 v0ToPoint = point - v0;

    float proj01 = glm::dot(edge01, v0ToPoint);
    float proj02 = glm::dot(edge02, v0ToPoint);

    if (proj01 <= 0.0f && proj02 <= 0.0f)
        return glm::length(point - v0);

    glm::vec3 v1ToPoint = point - v1;
    float proj11 = glm::dot(edge01, v1ToPoint);
    float proj12 = glm::dot(edge02, v1ToPoint);

    if (proj11 >= 0.0f && proj12 <= proj11)
        return glm::length(point - v1);

    glm::vec3 v2ToPoint = point - v2;
    float proj21 = glm::dot(edge01, v2ToPoint);
    float proj22 = glm::dot(edge02, v2ToPoint);

    if (proj22 >= 0.0f && proj21 <= proj22)
        return glm::length(point - v2);

    float baryV2 = proj01 * proj12 - proj11 * proj02;
    if (baryV2 <= 0.0f && proj01 >= 0.0f && proj11 <= 0.0f)
    {
        float t = proj01 / (proj01 - proj11);
        return glm::length(point - (v0 + t * edge01));
    }

    float baryV1 = proj21 * proj02 - proj01 * proj22;
    if (baryV1 <= 0.0f && proj02 >= 0.0f && proj22 <= 0.0f)
    {
        float t = proj02 / (proj02 - proj22);
        return glm::length(point - (v0 + t * edge02));
    }

    float baryV0 = proj11 * proj22 - proj21 * proj12;
    if (baryV0 <= 0.0f && (proj12 - proj11) >= 0.0f && (proj21 - proj22) >= 0.0f)
    {
        float t = (proj12 - proj11) / ((proj12 - proj11) + (proj21 - proj22));
        return glm::length(point - (v1 + t * (v2 - v1)));
    }

    float totalBary = 1.0f / (baryV0 + baryV1 + baryV2);
    float bary1 = baryV1 * totalBary;
    float bary2 = baryV2 * totalBary;
    return glm::length(point - (v0 + edge01 * bary1 + edge02 * bary2));
}

std::vector<uint8_t> MeshBoundary::Voxelize(
    const std::vector<std::array<glm::vec3, 3>>& triangles,
    const int cellCountPerAxis,
    const float cellSize)
{
    const int N = cellCountPerAxis;
    std::vector<uint8_t> solidCells(N * N * N, 0);

    for (const auto& triangle : triangles)
    {
        const glm::vec3& v0 = triangle[0];
        const glm::vec3& v1 = triangle[1];
        const glm::vec3& v2 = triangle[2];

        glm::vec3 boundsMin = glm::min(glm::min(v0, v1), v2);
        glm::vec3 boundsMax = glm::max(glm::max(v0, v1), v2);

        int xMin = std::max(0, static_cast<int>(std::floor(boundsMin.x / cellSize)) - 1);
        int yMin = std::max(0, static_cast<int>(std::floor(boundsMin.y / cellSize)) - 1);
        int zMin = std::max(0, static_cast<int>(std::floor(boundsMin.z / cellSize)) - 1);
        int xMax = std::min(N - 1, static_cast<int>(std::ceil(boundsMax.x / cellSize)) + 1);
        int yMax = std::min(N - 1, static_cast<int>(std::ceil(boundsMax.y / cellSize)) + 1);
        int zMax = std::min(N - 1, static_cast<int>(std::ceil(boundsMax.z / cellSize)) + 1);

        for (int z = zMin; z <= zMax; z++)
        {
            for (int y = yMin; y <= yMax; y++)
            {
                for (int x = xMin; x <= xMax; x++)
                {
                    glm::vec3 nodeWorldPosition(x * cellSize, y * cellSize, z * cellSize);

                    if (PointTriangleDistance(nodeWorldPosition, v0, v1, v2) < cellSize)
                    {
                        solidCells[z * N * N + y * N + x] = 1;
                    }
                }
            }
        }
    }

    return solidCells;
}
