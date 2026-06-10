#ifndef MPM_METHOD_SNOWFALL_H
#define MPM_METHOD_SNOWFALL_H
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "CUDA/Simulation.h"
#include "OpenGL/Render/Particle.h"
#include "OpenGL/Shaders/Shader.h"
#include "OpenGL/Simulation/SnowVolume.h"

namespace Snowfall
{
    inline constexpr glm::vec3 boxLeftCorner(1.0f, 4.0f, 1.0f);
    inline constexpr glm::vec3 boxRightCorner(4.1f, 4.02f, 4.1f);
    inline constexpr glm::vec3 doomPosition(2.56f, 1.0f, 2.56f);
    inline constexpr glm::vec3 doomScale(4.0f, 4.0f, 4.0f);

    inline constexpr glm::vec3 cameraPosition(2.64f, 2.83f, 7.46f);
    inline constexpr float cameraYaw = -90.2f;
    inline constexpr float cameraPitch = -17.7f;

    inline constexpr int maxParticleCount = 64000;
    inline constexpr int particleBatchCount = 320;
    inline constexpr int spawnIntervalFrames = 30;
    
    inline constexpr float snowDensity = 100.0f;

    inline int spawnFrameCounter = 0;

    inline glm::mat4 GetDoomModelMatrix()
    {
        return glm::scale(glm::translate(glm::mat4(1.0f), doomPosition), doomScale);
    }

    inline void SetDoomUniforms(const Shader& shader)
    {
        const glm::mat4 model = GetDoomModelMatrix();
        const glm::mat3 normal = glm::mat3(glm::transpose(glm::inverse(model)));

        shader.SetMat4("model", model);
        shader.SetMat3("normalMatrix", normal);
    }

    inline void SpawnIfPossible(Simulation* simulation, Particle* particles)
    {
        if (spawnFrameCounter == 0)
        {
            SnowVolume batch(boxLeftCorner, boxRightCorner, particleBatchCount, snowDensity);

            batch.BuildInitialPositions();
            batch.BuildParticleBlocks();

            simulation->AddParticles(batch);
        }

        spawnFrameCounter = (spawnFrameCounter + 1) % spawnIntervalFrames;
        particles->SetCount(simulation->GetParticleCount());
    }
}

#endif
