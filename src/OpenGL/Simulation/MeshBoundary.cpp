#include "OpenGL/Simulation/MeshBoundary.h"

#include <algorithm>
#include <execution>
#include <limits>
#include <numeric>

#include <glm/glm.hpp>

glm::vec3 MeshBoundary::ClosestPointOnTriangle(glm::vec3 point, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2)
{
    // 7 Voronoi Regions

    // Vertices
    const glm::vec3 e0 = v1 - v0;
    const glm::vec3 e1 = v2 - v0;
    const glm::vec3 v0ToPoint = point - v0;

    const float v0ToPointProjectionE0 = glm::dot(e0, v0ToPoint);
    const float v0ToPointProjectionE1 = glm::dot(e1, v0ToPoint);

    if (v0ToPointProjectionE0 <= 0.0f && v0ToPointProjectionE1 <= 0.0f)
    {
        return v0;
    }

    const glm::vec3 v1ToPoint = point - v1;

    const float v1ToPointProjectionE0 = glm::dot(e0, v1ToPoint);
    const float v1ToPointProjectionE1 = glm::dot(e1, v1ToPoint);

    if (v1ToPointProjectionE0 >= 0.0f && v1ToPointProjectionE1 <= v1ToPointProjectionE0)
    {
        return v1;
    }

    const glm::vec3 v2ToPoint = point - v2;

    const float v2ToPointProjectionE0 = glm::dot(e0, v2ToPoint);
    const float v2ToPointProjectionE1 = glm::dot(e1, v2ToPoint);

    if (v2ToPointProjectionE1 >= 0.0f && v2ToPointProjectionE0 <= v2ToPointProjectionE1)
    {
        return v2;
    }

    // Edges
    const float unnormalizedWeightV2 = v0ToPointProjectionE0 * v1ToPointProjectionE1 - v1ToPointProjectionE0 * v0ToPointProjectionE1;

    if (unnormalizedWeightV2 <= 0.0f && v0ToPointProjectionE0 >= 0.0f && v1ToPointProjectionE0 <= 0.0f)
    {
        return v0 + (v0ToPointProjectionE0 / (v0ToPointProjectionE0 - v1ToPointProjectionE0)) * e0;
    }

    const float unnormalizedWeightV1 = v2ToPointProjectionE0 * v0ToPointProjectionE1 - v0ToPointProjectionE0 * v2ToPointProjectionE1;

    if (unnormalizedWeightV1 <= 0.0f && v0ToPointProjectionE1 >= 0.0f && v2ToPointProjectionE1 <= 0.0f)
    {
        return v0 + (v0ToPointProjectionE1 / (v0ToPointProjectionE1 - v2ToPointProjectionE1)) * e1;
    }

    const float unnormalizedWeightV0 = v1ToPointProjectionE0 * v2ToPointProjectionE1 - v2ToPointProjectionE0 * v1ToPointProjectionE1;

    if (unnormalizedWeightV0 <= 0.0f && (v1ToPointProjectionE1 - v1ToPointProjectionE0) >= 0.0f && (v2ToPointProjectionE0 - v2ToPointProjectionE1) >= 0.0f)
    {
        return v1 + ((v1ToPointProjectionE1 - v1ToPointProjectionE0) / ((v1ToPointProjectionE1 - v1ToPointProjectionE0) + (v2ToPointProjectionE0 - v2ToPointProjectionE1))) * (v2 - v1);
    }

    // Inside the triangle
    const float inverseBarycentricSum = 1.0f / (unnormalizedWeightV0 + unnormalizedWeightV1 + unnormalizedWeightV2);

    return v0 + e0 * (unnormalizedWeightV1 * inverseBarycentricSum) + e1 * (unnormalizedWeightV2 * inverseBarycentricSum);
}

MeshSDF MeshBoundary::Voxelize(const std::vector<std::array<glm::vec3, 3>>& triangles, const int cellCountPerAxis, const float cellSize)
{
    const int cellsPerLayer = cellCountPerAxis * cellCountPerAxis;
    const int nodeCount = cellsPerLayer * cellCountPerAxis;
    const float narrowBand = 4.0f * cellSize;

    MeshSDF sdf;
    sdf.distances.assign(nodeCount, std::numeric_limits<float>::max());
    sdf.normals.assign(nodeCount, glm::vec3(0.0f));

    std::vector<int> nodeIndices(nodeCount);
    std::iota(nodeIndices.begin(), nodeIndices.end(), 0);

    std::for_each(std::execution::par_unseq, nodeIndices.begin(), nodeIndices.end(), [&](const int index)
    {
        const int z = index / cellsPerLayer;
        const int y = (index / cellCountPerAxis) % cellCountPerAxis;
        const int x = index % cellCountPerAxis;

        const glm::vec3 nodePosition(static_cast<float>(x) * cellSize, static_cast<float>(y) * cellSize, static_cast<float>(z) * cellSize);

        float bestAbsDist = std::numeric_limits<float>::max();

        for (const auto& triangle : triangles)
        {
            const glm::vec3& v0 = triangle[0];
            const glm::vec3& v1 = triangle[1];
            const glm::vec3& v2 = triangle[2];

            const glm::vec3 boundsMin = glm::min(glm::min(v0, v1), v2) - narrowBand;
            // ReSharper disable once CppTooWideScopeInitStatement
            const glm::vec3 boundsMax = glm::max(glm::max(v0, v1), v2) + narrowBand;

            if (glm::any(glm::lessThan(nodePosition, boundsMin) || glm::greaterThan(nodePosition, boundsMax)))
            {
                continue;
            }

            const glm::vec3 closestPoint = ClosestPointOnTriangle(nodePosition, v0, v1, v2);
            const glm::vec3 toNode = nodePosition - closestPoint;

            const float distance = glm::length(toNode);

            if (distance >= narrowBand || distance >= bestAbsDist)
            {
                continue;
            }

            bestAbsDist = distance;
            const glm::vec3 triangleNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
            const float sign = glm::dot(toNode, triangleNormal) >= 0.0f ? 1.0f : -1.0f;

            sdf.distances[index] = sign * distance;
            sdf.normals[index] = triangleNormal;
        }
    });

    return sdf;
}
