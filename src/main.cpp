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
#include "OpenGL/Simulation/Configuration.h"
#include "OpenGL/Simulation/Snowball.h"

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

    Snowball snowballInformation;

    try
    {
        snowballInformation.BuildInitialPositions();
        snowballInformation.BuildParticleBlocks();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    Simulation simulation(snowballInformation.particleCount, snowballInformation.initialBlocks.data(), static_cast<int>(snowballInformation.initialBlocks.size()));

    Particle snowball(snowballInformation.initialPositions);

    glEnable(GL_DEPTH_TEST);

    std::vector<float> positionsX(snowballInformation.particleCount);
    std::vector<float> positionsY(snowballInformation.particleCount);
    std::vector<float> positionsZ(snowballInformation.particleCount);

    std::vector<glm::vec3> positions(snowballInformation.particleCount);

    while (!glfwWindowShouldClose(program.window))
    {
        program.UpdateDeltaTime();

        if (program.deltaTime > 0.0f)
        {
            const int fps = static_cast<int>(1.0f / program.deltaTime);
            const std::string title = std::string(Program::windowTitle) + " | FPS: " + std::to_string(fps);
            glfwSetWindowTitle(program.window, title.c_str());
        }

        camera.UpdateSpeed(program.deltaTime);

        program.ProcessInput();
        camera.ProcessInput();

        for (int step = 0; step < 5; step++)
        {
            simulation.Step();
        }

        simulation.CopyPositionsToHost(positionsX.data(), positionsY.data(), positionsZ.data());

        for (int particleIndex = 0; particleIndex < snowballInformation.particleCount; particleIndex++)
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
