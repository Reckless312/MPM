#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <random>
#include <cstring>

#include "OpenGL/Scene/Camera.h"
#include "OpenGL/Program.h"
#include "Exceptions/MPMException.h"
#include "OpenGL/Render/Model.h"
#include "OpenGL/Render/Particle.h"
#include "OpenGL/Shaders/Shader.h"
#include "OpenGL/Shaders/TextureLoader.h"
#include "CUDA/Simulation.h"

static std::vector<ParticleBlock> buildParticleBlocks(const std::vector<glm::vec3>& positions, const float particleMass, const float particleVolume)
{
    const int particleCount = static_cast<int>(positions.size());
    const int blockCount = (particleCount + 31) / 32;
    std::vector<ParticleBlock> blocks(blockCount);
    std::memset(blocks.data(), 0, blockCount * sizeof(ParticleBlock));

    for (int particleIndex = 0; particleIndex < particleCount; particleIndex++)
    {
        const int blockIndex = particleIndex / 32;
        const int lane = particleIndex % 32;
        blocks[blockIndex].positionX[lane] = positions[particleIndex].x;
        blocks[blockIndex].positionY[lane] = positions[particleIndex].y;
        blocks[blockIndex].positionZ[lane] = positions[particleIndex].z;
        blocks[blockIndex].mass[lane] = particleMass;
        blocks[blockIndex].volume[lane] = particleVolume;
        blocks[blockIndex].plasticVolume[lane] = 1.0f;
        blocks[blockIndex].deformationGradient[0][lane] = 1.0f;
        blocks[blockIndex].deformationGradient[4][lane] = 1.0f;
        blocks[blockIndex].deformationGradient[8][lane] = 1.0f;
    }
    return blocks;
}

int main()
{
    Program program;

    try
    {
        Program::InitializeGLFW();
        program.CreateWindowAndAssignContext();
        Program::LoadGladLibrary();
        program.LockCursor();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    program.SetViewportAndResizeCallback();

    Camera camera(program.window, Program::windowWidth, Program::windowHeight);
    camera.AssignUserPointerAndSetCallbacks();

    Shader sceneShader("vertexShader.vs", "fragmentShader.fs");
    Shader particleShader("particleVertex.vs", "particleFragment.fs");

    try
    {
        sceneShader.Load();
        particleShader.Load();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    constexpr int particleCount = 100000;

    constexpr float cellSize = 0.02f;
    constexpr int cellCountPerAxis = 256;

    constexpr float snowballRadius = 0.5f;

    constexpr float snowDensity = 400.0f;
    constexpr float snowballVolume = 4.0f / 3.0f * 3.14159265f * snowballRadius * snowballRadius * snowballRadius;

    constexpr float particleVolume = snowballVolume / static_cast<float>(particleCount);
    constexpr float particleMass = snowDensity * particleVolume;

    std::random_device randomDevice;
    std::mt19937 randomEngine(randomDevice());
    std::uniform_real_distribution distribution(-snowballRadius, snowballRadius);

    std::vector<glm::vec3> initialPositions;
    initialPositions.reserve(particleCount);

    while (static_cast<int>(initialPositions.size()) < particleCount)
    {
        // ReSharper disable once CppTooWideScopeInitStatement
        const glm::vec3 offset(distribution(randomEngine), distribution(randomEngine), distribution(randomEngine));

        if (glm::length(offset) <= snowballRadius)
        {
            constexpr float snowballCenterZ = 2.56f;
            constexpr float snowballCenterY = 3.0f;
            constexpr float snowballCenterX = 2.56f;

            initialPositions.emplace_back(snowballCenterX + offset.x, snowballCenterY + offset.y, snowballCenterZ + offset.z);
        }
    }

    const std::vector<ParticleBlock> initialBlocks = buildParticleBlocks(initialPositions, particleMass, particleVolume);

    SimulationParameters parameters;
    parameters.particleCount = particleCount;
    parameters.maxBlocks = 4096;
    parameters.gridSizeInCells = cellCountPerAxis;
    parameters.cellSize = cellSize;
    parameters.deltaTime = 2e-4f;
    parameters.gravity = 9.8f;
    parameters.shearModulus = 1.4e5f;
    parameters.firstLameParameter = 4.0e4f;
    parameters.hardeningCoefficient = 10.0f;
    parameters.criticalCompression = 0.025f;
    parameters.criticalStretch = 0.0075f;

    Simulation simulation(parameters, initialBlocks.data(), static_cast<int>(initialBlocks.size()));

    Particle snowball(initialPositions);

    glEnable(GL_DEPTH_TEST);

    std::vector<float> positionsX(particleCount);
    std::vector<float> positionsY(particleCount);
    std::vector<float> positionsZ(particleCount);

    std::vector<glm::vec3> positions(particleCount);

    while (!glfwWindowShouldClose(program.window))
    {
        program.UpdateDeltaTime();
        camera.UpdateSpeed(program.deltaTime);

        program.ProcessInput();
        camera.ProcessInput();

        simulation.step();
        simulation.copyPositionsToHost(positionsX.data(), positionsY.data(), positionsZ.data());

        for (int particleIndex = 0; particleIndex < particleCount; particleIndex++)
        {
            positions[particleIndex] = glm::vec3(positionsX[particleIndex], positionsY[particleIndex], positionsZ[particleIndex]);
        }

        snowball.Update(positions);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        particleShader.Use();
        particleShader.SetMat4("view", camera.viewMatrix);
        particleShader.SetMat4("projection", camera.projectionMatrix);
        snowball.Draw(particleShader);

        glfwSwapBuffers(program.window);
        glfwPollEvents();
    }

    return 0;
}
