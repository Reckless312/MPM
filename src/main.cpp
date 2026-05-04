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
#include "OpenGL/Simulation/Snowfall.h"
#include "CUDA/MeshBoundary.h"

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

    Model untitled(std::string(ASSETS_PATH) + "/ubb_logo.obj");

    try
    {
        untitled.loadModel();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    sceneShader.Use();
    sceneShader.SetVec3("directionalLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
    sceneShader.SetVec3("directionalLight.ambient",   glm::vec3(0.5f, 0.5f, 0.5f));
    sceneShader.SetVec3("directionalLight.diffuse",   glm::vec3(0.8f, 0.8f, 0.8f));
    sceneShader.SetVec3("directionalLight.specular",  glm::vec3(1.0f, 1.0f, 1.0f));
    sceneShader.SetFloat("material.shininess", 32.0f);

    Snowfall snowfallInformation;
    Snowball snowballInformation;

    try
    {
        snowfallInformation.BuildInitialPositions();
        snowfallInformation.BuildParticleBlocks();
        snowballInformation.BuildInitialPositions();
        snowballInformation.BuildParticleBlocks();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    Particle snowfall(snowfallInformation.initialPositions);

    Simulation simulation(snowfallInformation.particleCount, snowfallInformation.initialBlocks.data(), static_cast<int>(snowfallInformation.initialBlocks.size()), snowfall.GetVBO());

    const glm::mat4 logoModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(1.3f, 1.2f, 2.56f))
                                    * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    auto logoTriangles = untitled.GetTriangles(logoModelMatrix);
    auto solidCells = MeshBoundary::Voxelize(logoTriangles, 256, 0.02f);
    simulation.UploadMeshBoundary(solidCells);

    camera.SetInitialOrientation(glm::vec3(2.56f, 1.5f, 7.5f), -90.0f, 0.0f);

    glEnable(GL_DEPTH_TEST);

    bool showLogo = true;
    bool paused = false;

    bool key1WasDown = false;
    bool key2WasDown = false;
    bool spaceWasDown = false;

    while (!glfwWindowShouldClose(program.window))
    {
        program.UpdateDeltaTime();

        const bool key1Down = glfwGetKey(program.window, GLFW_KEY_1) == GLFW_PRESS;
        const bool key2Down = glfwGetKey(program.window, GLFW_KEY_2) == GLFW_PRESS;

        const bool spaceDown = glfwGetKey(program.window, GLFW_KEY_SPACE) == GLFW_PRESS;

        if (key1Down && !key1WasDown)
        {
            simulation.Reset(snowfallInformation.initialBlocks.data(), static_cast<int>(snowfallInformation.initialBlocks.size()));
            showLogo = true;
        }

        if (key2Down && !key2WasDown)
        {
            simulation.Reset(snowballInformation.initialBlocks.data(), static_cast<int>(snowballInformation.initialBlocks.size()));
            camera.SetInitialOrientation(glm::vec3(2.56f, 1.5f, 4.0f), -90.0f, 0.0f);
            showLogo = false;
        }

        if (spaceDown && !spaceWasDown)
        {
            paused = !paused;
        }

        key1WasDown = key1Down;
        key2WasDown = key2Down;
        spaceWasDown = spaceDown;

        if (program.deltaTime > 0.0f)
        {
            const int fps = static_cast<int>(1.0f / program.deltaTime);
            const std::string title = std::string(Program::windowTitle) + " | FPS: " + std::to_string(fps);
            glfwSetWindowTitle(program.window, title.c_str());
        }

        camera.UpdateSpeed(program.deltaTime);

        program.ProcessInput();
        camera.ProcessInput();

        if (!paused)
        {
            for (int step = 0; step < 5; step++)
            {
                simulation.Step();
            }

            simulation.SyncPositionsToVBO();
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (showLogo)
        {
            sceneShader.Use();
            sceneShader.SetMat4("view", camera.viewMatrix);
            sceneShader.SetMat4("projection", camera.projectionMatrix);
            sceneShader.SetMat4("model", logoModelMatrix);
            sceneShader.SetMat3("normalMatrix", glm::mat3(glm::transpose(glm::inverse(logoModelMatrix))));
            sceneShader.SetVec3("viewPosition", camera.position);
            untitled.Draw(sceneShader);
        }

        particleShader.Use();
        particleShader.SetMat4("view", camera.viewMatrix);
        particleShader.SetMat4("projection", camera.projectionMatrix);
        snowfall.Draw(particleShader);

        glfwSwapBuffers(program.window);
        glfwPollEvents();
    }

    return 0;
}
