#include "OpenGL/Simulation/MeshBoundary.h"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>

float MeshBoundary::PointTriangleDistance(glm::vec3 point, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2)
{
    // v - vertex
    // e - edge
    // I hate names on this function

    // 7 Voronoi Regions

    // Vertices
    glm::vec3 e0 = v1 - v0;
    glm::vec3 e1 = v2 - v0;
    glm::vec3 v0ToPoint = point - v0;

    float v0ToPointProjectionE0 = glm::dot(e0, v0ToPoint);
    float v0ToPointProjectionE1 = glm::dot(e1, v0ToPoint);

    if (v0ToPointProjectionE0 <= 0.0f && v0ToPointProjectionE1 <= 0.0f)
    {
        return glm::length(point - v0);
    }

    glm::vec3 v1ToPoint = point - v1;

    float v1ToPointProjectionE0 = glm::dot(e0, v1ToPoint);
    float v1ToPointProjectionE1 = glm::dot(e1, v1ToPoint);

    if (v1ToPointProjectionE0 >= 0.0f && v1ToPointProjectionE1 <= v1ToPointProjectionE0)
    {
        return glm::length(point - v1);
    }

    glm::vec3 v2ToPoint = point - v2;

    float v2ToPointProjectionE0 = glm::dot(e0, v2ToPoint);
    float v2ToPointProjectionE1 = glm::dot(e1, v2ToPoint);

    if (v2ToPointProjectionE1 >= 0.0f && v2ToPointProjectionE0 <= v2ToPointProjectionE1)
    {
        return glm::length(point - v2);
    }

    // Edges
    float unnormalizedWeightV2 = v0ToPointProjectionE0 * v1ToPointProjectionE1 - v1ToPointProjectionE0 * v0ToPointProjectionE1;

    if (unnormalizedWeightV2 <= 0.0f && v0ToPointProjectionE0 >= 0.0f && v1ToPointProjectionE0 <= 0.0f)
    {
        float fractionAlongE0 = v0ToPointProjectionE0 / (v0ToPointProjectionE0 - v1ToPointProjectionE0);
        return glm::length(point - (v0 + fractionAlongE0 * e0));
    }

    float unnormalizedWeightV1 = v2ToPointProjectionE0 * v0ToPointProjectionE1 - v0ToPointProjectionE0 * v2ToPointProjectionE1;

    if (unnormalizedWeightV1 <= 0.0f && v0ToPointProjectionE1 >= 0.0f && v2ToPointProjectionE1 <= 0.0f)
    {
        float fractionAlongE1 = v0ToPointProjectionE1 / (v0ToPointProjectionE1 - v2ToPointProjectionE1);
        return glm::length(point - (v0 + fractionAlongE1 * e1));
    }

    float unnormalizedWeightV0 = v1ToPointProjectionE0 * v2ToPointProjectionE1 - v2ToPointProjectionE0 * v1ToPointProjectionE1;

    if (unnormalizedWeightV0 <= 0.0f && (v1ToPointProjectionE1 - v1ToPointProjectionE0) >= 0.0f && (v2ToPointProjectionE0 - v2ToPointProjectionE1) >= 0.0f)
    {
        float fractionAlongV1ToV2 = (v1ToPointProjectionE1 - v1ToPointProjectionE0) / ((v1ToPointProjectionE1 - v1ToPointProjectionE0) + (v2ToPointProjectionE0 - v2ToPointProjectionE1));
        return glm::length(point - (v1 + fractionAlongV1ToV2 * (v2 - v1)));
    }

    // Inside the triangle
    float inverseBarycentricSum = 1.0f / (unnormalizedWeightV0 + unnormalizedWeightV1 + unnormalizedWeightV2);

    float normalizedWeightV1 = unnormalizedWeightV1 * inverseBarycentricSum;
    float normalizedWeightV2 = unnormalizedWeightV2 * inverseBarycentricSum;

    return glm::length(point - (v0 + e0 * normalizedWeightV1 + e1 * normalizedWeightV2));
}

std::vector<uint8_t> MeshBoundary::Voxelize(const std::vector<std::array<glm::vec3, 3>>& triangles, const int cellCountPerAxis, const float cellSize)
{
    std::vector<uint8_t> solidCells(cellCountPerAxis * cellCountPerAxis * cellCountPerAxis, false);

    for (const auto& triangle : triangles)
    {
        const glm::vec3& v0 = triangle[0];
        const glm::vec3& v1 = triangle[1];
        const glm::vec3& v2 = triangle[2];

        const glm::vec3 boundsMin = glm::min(glm::min(v0, v1), v2);
        const glm::vec3 boundsMax = glm::max(glm::max(v0, v1), v2);

        const int cellXMin = std::max(0, static_cast<int>(std::floor(boundsMin.x / cellSize)) - 1);
        const int cellYMin = std::max(0, static_cast<int>(std::floor(boundsMin.y / cellSize)) - 1);
        const int cellZMin = std::max(0, static_cast<int>(std::floor(boundsMin.z / cellSize)) - 1);

        const int cellXMax = std::min(cellCountPerAxis - 1, static_cast<int>(std::ceil(boundsMax.x / cellSize)) + 1);
        const int cellYMax = std::min(cellCountPerAxis - 1, static_cast<int>(std::ceil(boundsMax.y / cellSize)) + 1);
        const int cellZMax = std::min(cellCountPerAxis - 1, static_cast<int>(std::ceil(boundsMax.z / cellSize)) + 1);

        for (int z = cellZMin; z <= cellZMax; z++)
        {
            for (int y = cellYMin; y <= cellYMax; y++)
            {
                for (int x = cellXMin; x <= cellXMax; x++)
                {
                    // ReSharper disable once CppTooWideScopeInitStatement
                    const glm::vec3 nodeWorldPosition(static_cast<float>(x) * cellSize, static_cast<float>(y) * cellSize, static_cast<float>(z) * cellSize);

                    if (PointTriangleDistance(nodeWorldPosition, v0, v1, v2) < cellSize)
                    {
                        solidCells[z * cellCountPerAxis * cellCountPerAxis + y * cellCountPerAxis + x] = 1;
                    }
                }
            }
        }
    }

    return solidCells;
}
