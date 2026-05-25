#include "OpenGL/Simulation/MeshBoundary.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <mutex>
#include <thread>
#include <vector>

#include <glm/glm.hpp>

glm::vec3 MeshBoundary::ClosestPointOnTriangle(glm::vec3 point, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2)
{
    // 7 Voronoi Regions

    // Vertices
    glm::vec3 e0 = v1 - v0;
    glm::vec3 e1 = v2 - v0;
    glm::vec3 v0ToPoint = point - v0;

    float v0ToPointProjectionE0 = glm::dot(e0, v0ToPoint);
    float v0ToPointProjectionE1 = glm::dot(e1, v0ToPoint);

    if (v0ToPointProjectionE0 <= 0.0f && v0ToPointProjectionE1 <= 0.0f)
    {
        return v0;
    }

    glm::vec3 v1ToPoint = point - v1;

    float v1ToPointProjectionE0 = glm::dot(e0, v1ToPoint);
    float v1ToPointProjectionE1 = glm::dot(e1, v1ToPoint);

    if (v1ToPointProjectionE0 >= 0.0f && v1ToPointProjectionE1 <= v1ToPointProjectionE0)
    {
        return v1;
    }

    glm::vec3 v2ToPoint = point - v2;

    float v2ToPointProjectionE0 = glm::dot(e0, v2ToPoint);
    float v2ToPointProjectionE1 = glm::dot(e1, v2ToPoint);

    if (v2ToPointProjectionE1 >= 0.0f && v2ToPointProjectionE0 <= v2ToPointProjectionE1)
    {
        return v2;
    }

    // Edges
    float unnormalizedWeightV2 = v0ToPointProjectionE0 * v1ToPointProjectionE1 - v1ToPointProjectionE0 * v0ToPointProjectionE1;

    if (unnormalizedWeightV2 <= 0.0f && v0ToPointProjectionE0 >= 0.0f && v1ToPointProjectionE0 <= 0.0f)
    {
        float fractionAlongE0 = v0ToPointProjectionE0 / (v0ToPointProjectionE0 - v1ToPointProjectionE0);
        return v0 + fractionAlongE0 * e0;
    }

    float unnormalizedWeightV1 = v2ToPointProjectionE0 * v0ToPointProjectionE1 - v0ToPointProjectionE0 * v2ToPointProjectionE1;

    if (unnormalizedWeightV1 <= 0.0f && v0ToPointProjectionE1 >= 0.0f && v2ToPointProjectionE1 <= 0.0f)
    {
        float fractionAlongE1 = v0ToPointProjectionE1 / (v0ToPointProjectionE1 - v2ToPointProjectionE1);
        return v0 + fractionAlongE1 * e1;
    }

    float unnormalizedWeightV0 = v1ToPointProjectionE0 * v2ToPointProjectionE1 - v2ToPointProjectionE0 * v1ToPointProjectionE1;

    if (unnormalizedWeightV0 <= 0.0f && (v1ToPointProjectionE1 - v1ToPointProjectionE0) >= 0.0f && (v2ToPointProjectionE0 - v2ToPointProjectionE1) >= 0.0f)
    {
        float fractionAlongV1ToV2 = (v1ToPointProjectionE1 - v1ToPointProjectionE0) / ((v1ToPointProjectionE1 - v1ToPointProjectionE0) + (v2ToPointProjectionE0 - v2ToPointProjectionE1));
        return v1 + fractionAlongV1ToV2 * (v2 - v1);
    }

    // Inside the triangle
    float inverseBarycentricSum = 1.0f / (unnormalizedWeightV0 + unnormalizedWeightV1 + unnormalizedWeightV2);

    float normalizedWeightV1 = unnormalizedWeightV1 * inverseBarycentricSum;
    float normalizedWeightV2 = unnormalizedWeightV2 * inverseBarycentricSum;

    return v0 + e0 * normalizedWeightV1 + e1 * normalizedWeightV2;
}

MeshSDF MeshBoundary::Voxelize(const std::vector<std::array<glm::vec3, 3>>& triangles, const int cellCountPerAxis, const float cellSize)
{
    const int nodeCount = cellCountPerAxis * cellCountPerAxis * cellCountPerAxis;
    const float narrowBand = 4.0f * cellSize;

    MeshSDF sdf;
    sdf.distances.assign(nodeCount, std::numeric_limits<float>::max());
    sdf.normals.assign(nodeCount, glm::vec3(0.0f));

    constexpr int mutexPoolSize = 1024;
    std::array<std::mutex, mutexPoolSize> mutexPool;

    const int triangleCount = static_cast<int>(triangles.size());
    const unsigned int threadCount = std::thread::hardware_concurrency();
    const int chunkSize = (triangleCount + static_cast<int>(threadCount) - 1) / static_cast<int>(threadCount);

    auto processTriangles = [&](const int start, const int end)
    {
        for (int t = start; t < end; t++)
        {
            const glm::vec3& v0 = triangles[t][0];
            const glm::vec3& v1 = triangles[t][1];
            const glm::vec3& v2 = triangles[t][2];

            const glm::vec3 triangleNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

            const glm::vec3 boundsMin = glm::min(glm::min(v0, v1), v2);
            const glm::vec3 boundsMax = glm::max(glm::max(v0, v1), v2);

            const int cellXMin = std::max(0, static_cast<int>(std::floor((boundsMin.x - narrowBand) / cellSize)));
            const int cellYMin = std::max(0, static_cast<int>(std::floor((boundsMin.y - narrowBand) / cellSize)));
            const int cellZMin = std::max(0, static_cast<int>(std::floor((boundsMin.z - narrowBand) / cellSize)));

            const int cellXMax = std::min(cellCountPerAxis - 1, static_cast<int>(std::ceil((boundsMax.x + narrowBand) / cellSize)));
            const int cellYMax = std::min(cellCountPerAxis - 1, static_cast<int>(std::ceil((boundsMax.y + narrowBand) / cellSize)));
            const int cellZMax = std::min(cellCountPerAxis - 1, static_cast<int>(std::ceil((boundsMax.z + narrowBand) / cellSize)));

            for (int z = cellZMin; z <= cellZMax; z++)
            {
                for (int y = cellYMin; y <= cellYMax; y++)
                {
                    for (int x = cellXMin; x <= cellXMax; x++)
                    {
                        const glm::vec3 nodePosition(static_cast<float>(x) * cellSize, static_cast<float>(y) * cellSize, static_cast<float>(z) * cellSize);
                        const glm::vec3 closestPoint = ClosestPointOnTriangle(nodePosition, v0, v1, v2);
                        const glm::vec3 toNode = nodePosition - closestPoint;
                        const float distance = glm::length(toNode);

                        if (distance >= narrowBand)
                        {
                            continue;
                        }

                        const int index = z * cellCountPerAxis * cellCountPerAxis + y * cellCountPerAxis + x;

                        std::lock_guard<std::mutex> lock(mutexPool[index % mutexPoolSize]);

                        if (distance < std::abs(sdf.distances[index]))
                        {
                            const float sign = glm::dot(toNode, triangleNormal) >= 0.0f ? 1.0f : -1.0f;
                            sdf.distances[index] = sign * distance;
                            sdf.normals[index] = triangleNormal;
                        }
                    }
                }
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    for (unsigned int i = 0; i < threadCount; i++)
    {
        const int start = static_cast<int>(i) * chunkSize;
        const int end = std::min(start + chunkSize, triangleCount);

        if (start < triangleCount)
        {
            threads.emplace_back(processTriangles, start, end);
        }
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    return sdf;
}
