#ifndef MPM_METHOD_SNOWSLED_H
#define MPM_METHOD_SNOWSLED_H

#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "CUDA/Simulation.h"
#include "OpenGL/Shaders/Shader.h"
#include "OpenGL/Simulation/SimulationConfig.h"

namespace SnowSled
{
    inline constexpr glm::vec3 boxLeftCorner(1.5f, 0.04f, 1.5f);
    inline constexpr glm::vec3 boxRightCorner(3.6f, 0.2f, 3.6f);
    inline constexpr glm::vec3 cameraPosition(-0.4f, 2.2f, 3.0f);

    inline constexpr float zMargin = 1.0f;
    inline constexpr float sledSpeed = 3.0f;
    inline constexpr float sledCenterX = 2.1f;
    inline constexpr float cameraYaw = -0.40f;
    inline constexpr float snowDensity = 400.0f;
    inline constexpr float cameraPitch = -35.00f;
    inline constexpr float sledEndZ = boxRightCorner.z + zMargin;
    inline constexpr float sledInitialZ = boxLeftCorner.z - zMargin;

    inline constexpr int particleCount = 60000;

    inline float sledAccumulatedZ = 0.0f;
    inline float sledCenterZ = sledInitialZ;

    inline glm::mat4 GetSledModelMatrix()
    {
        return glm::translate(glm::mat4(1.0f), glm::vec3(2.1f, 0.04f, sledCenterZ));
    }

    inline void SetLogoUniforms(const Shader &shader)
    {
        const glm::mat4 sledMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(sledCenterX, 0.04f, sledCenterZ));
        const glm::mat3 normal = glm::mat3(glm::transpose(glm::inverse(sledMatrix)));

        shader.SetMat4("model", sledMatrix);
        shader.SetMat3("normalMatrix", normal);
    }

    inline void Move(Simulation* activeSim)
    {
        if (!(sledCenterZ < sledEndZ))
        {
            activeSim->SetBoundaryVelocity(glm::vec3(0.0f));
        }
        else
        {
            activeSim->SetBoundaryVelocity(glm::vec3(0.0f, 0.0f, sledSpeed));

            sledCenterZ += sledSpeed * SimulationConfig::physicsTimeStep;
            sledAccumulatedZ += sledSpeed * SimulationConfig::physicsTimeStep;

            if (sledAccumulatedZ >= SimulationConfig::cellSize)
            {
                const int shiftCells = static_cast<int>(sledAccumulatedZ / SimulationConfig::cellSize);
                activeSim->MoveSledSDF(shiftCells);
                sledAccumulatedZ -= static_cast<float>(shiftCells) * SimulationConfig::cellSize;
            }
        }
    }
}

#endif
