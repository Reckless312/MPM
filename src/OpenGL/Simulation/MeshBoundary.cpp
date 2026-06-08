#include "OpenGL/Simulation/MeshBoundary.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <mutex>
#include <thread>
#include <vector>

#include <glm/glm.hpp>

#include "SimulationConfig.h"

glm::vec3 MeshBoundary::ClosestPointOnTriangle(glm::vec3 p, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2)
{
    // I hate everything about this function and making it readable

    glm::vec3 e0 = v1 - v0;
    glm::vec3 e1 = v2 - v0;

    glm::vec3 v0ToP = p - v0;

    float d0 = glm::dot(e0, v0ToP);
    float d1 = glm::dot(e1, v0ToP);

    if (d0 <= 0.0f && d1 <= 0.0f)
    {
        return v0;
    }

    glm::vec3 v1ToP = p - v1;

    float d2 = glm::dot(e0, v1ToP);
    float d3 = glm::dot(e1, v1ToP);

    if (d2 >= 0.0f && d3 <= d2)
    {
        return v1;
    }

    glm::vec3 v2ToP = p - v2;

    float d4 = glm::dot(e0, v2ToP);
    float d5 = glm::dot(e1, v2ToP);

    if (d5 >= 0.0f && d4 <= d5)
    {
        return v2;
    }

    float o2 = d0 * d3 - d2 * d1;

    if (o2 <= 0.0f && d0 >= 0.0f && d2 <= 0.0f)
    {
        float t2 = d0 / (d0 - d2);
        return v0 + t2 * e0;
    }

    float o1 = d4 * d1 - d0 * d5;

    if (o1 <= 0.0f && d1 >= 0.0f && d5 <= 0.0f)
    {
        float t1 = d1 / (d1 - d5);
        return v0 + t1 * e1;
    }

    float o0 = d2 * d5 - d4 * d3;

    if (o0 <= 0.0f && (d3 - d2) >= 0.0f && (d4 - d5) >= 0.0f)
    {
        float t0 = (d3 - d2) / ((d3 - d2) + (d4 - d5));
        return v1 + t0 * (v2 - v1);
    }

    float s = 1.0f / (o0 + o1 + o2);

    float no1 = o1 * s;
    float no2 = o2 * s;

    return v0 + e0 * no1 + e1 * no2;
}

MeshSDF MeshBoundary::BuildSDF(const std::vector<std::vector<glm::vec3>>& triangles)
{
    constexpr float narrowBand = 4.0f * SimulationConfig::cellSize;

    MeshSDF sdf;
    sdf.distances.assign(SimulationConfig::nodeCount, std::numeric_limits<float>::max());
    sdf.normals.assign(SimulationConfig::nodeCount, glm::vec3(0.0f));

    constexpr int mutexPoolSize = 1024;
    std::array<std::mutex, mutexPoolSize> mutexPool;

    const int triangleCount = static_cast<int>(triangles.size());

    const unsigned int threadCount = std::thread::hardware_concurrency();
    const int chunkSize = (triangleCount + static_cast<int>(threadCount) - 1) / static_cast<int>(threadCount);

    auto processTriangles = [&](const int start, const int end)
    {
        for (int currentTriangle = start; currentTriangle < end; currentTriangle++)
        {
            const glm::vec3& v0 = triangles[currentTriangle][0];
            const glm::vec3& v1 = triangles[currentTriangle][1];
            const glm::vec3& v2 = triangles[currentTriangle][2];

            const glm::vec3 triangleNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

            const glm::vec3 boundsMin = glm::min(glm::min(v0, v1), v2);
            const glm::vec3 boundsMax = glm::max(glm::max(v0, v1), v2);

            const int cellXMin = std::max(0, static_cast<int>(std::floor((boundsMin.x - narrowBand) / SimulationConfig::cellSize)));
            const int cellYMin = std::max(0, static_cast<int>(std::floor((boundsMin.y - narrowBand) / SimulationConfig::cellSize)));
            const int cellZMin = std::max(0, static_cast<int>(std::floor((boundsMin.z - narrowBand) / SimulationConfig::cellSize)));

            const int cellXMax = std::min(SimulationConfig::cellCountPerAxis - 1, static_cast<int>(std::ceil((boundsMax.x + narrowBand) / SimulationConfig::cellSize)));
            const int cellYMax = std::min(SimulationConfig::cellCountPerAxis - 1, static_cast<int>(std::ceil((boundsMax.y + narrowBand) / SimulationConfig::cellSize)));
            const int cellZMax = std::min(SimulationConfig::cellCountPerAxis - 1, static_cast<int>(std::ceil((boundsMax.z + narrowBand) / SimulationConfig::cellSize)));

            for (int z = cellZMin; z <= cellZMax; z++)
            {
                for (int y = cellYMin; y <= cellYMax; y++)
                {
                    for (int x = cellXMin; x <= cellXMax; x++)
                    {
                        const glm::vec3 nodePosition(static_cast<float>(x) * SimulationConfig::cellSize, static_cast<float>(y) * SimulationConfig::cellSize, static_cast<float>(z) * SimulationConfig::cellSize);
                        const glm::vec3 closestPoint = ClosestPointOnTriangle(nodePosition, v0, v1, v2);
                        const glm::vec3 toNode = nodePosition - closestPoint;

                        const float distance = glm::length(toNode);

                        if (distance >= narrowBand)
                        {
                            continue;
                        }

                        const int index = z * SimulationConfig::cellCountPerAxis * SimulationConfig::cellCountPerAxis + y * SimulationConfig::cellCountPerAxis + x;

                        std::lock_guard lock(mutexPool[index % mutexPoolSize]);

                        if (distance >= std::abs(sdf.distances[index]))
                        {
                            continue;
                        }

                        if (glm::dot(toNode, triangleNormal) >= 0.0f)
                        {
                            sdf.distances[index] = distance;
                        }
                        else
                        {
                            sdf.distances[index] = -distance;
                        }

                        sdf.normals[index] = triangleNormal;
                    }
                }
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    for (unsigned int thread = 0; thread < threadCount; thread++)
    {
        const int start = static_cast<int>(thread) * chunkSize;
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
