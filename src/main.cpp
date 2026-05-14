#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "OpenGL/Simulation/MeshBoundary.h"
#include "CUDA/Simulation.h"
#include "Exceptions/MPMException.h"
#include "OpenGL/Program.h"
#include "OpenGL/Render/Model.h"
#include "OpenGL/Render/Particle.h"
#include "OpenGL/Render/DepthFBO.h"
#include "OpenGL/Render/FullscreenQuad.h"
#include "OpenGL/Scene/Camera.h"
#include "OpenGL/Shaders/Shader.h"
#include "OpenGL/Simulation/Snowfall.h"

int main()
{
    Program program;

    try
    {
        Program::InitializeGLFW();
        program.CreateWindowAndAssignContext();
        Program::LoadGladLibrary();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    program.LockCursor();
    program.SetViewportAndResizeCallback();

    Camera camera(program.window);
    camera.AssignUserPointerAndSetCallbacks();

    Shader sceneShader("vertexShader.vs", "fragmentShader.fs");
    Shader particleShader("particleVertex.vs", "particleFragment.fs");
    Shader particleDepthShader("particleDepthVertex.vs", "particleDepthFragment.fs");
    Shader debugDepthShader("debugDepthVertex.vs", "debugDepthFragment.fs");
    Shader normalReconstructShader("debugDepthVertex.vs", "normalReconstructFragment.fs");

    try
    {
        sceneShader.Load();
        particleShader.Load();
        particleDepthShader.Load();
        debugDepthShader.Load();
        normalReconstructShader.Load();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    sceneShader.Use();

    glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    glm::vec3 ambient = glm::vec3(0.5f, 0.5f, 0.5f);
    glm::vec3 diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 specular = glm::vec3(1.0f, 1.0f, 1.0f);

    float shininess = 32.0f;

    sceneShader.SetVec3("directionalLight.direction", direction);
    sceneShader.SetVec3("directionalLight.ambient", ambient);
    sceneShader.SetVec3("directionalLight.diffuse", diffuse);
    sceneShader.SetVec3("directionalLight.specular", specular);
    sceneShader.SetFloat("material.shininess", shininess);

    Model logoUBB(std::string(ASSETS_PATH) + "/ubb_logo.obj");

    try
    {
        logoUBB.loadModel();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    constexpr SceneParameters snowfallSceneParameters = {
        .firstLameParameter = 1.333e4f,
        .secondLameParameter = 2.0e4f,
        .hardeningCoefficient = 10.0f,
        .criticalCompression = 0.025f,
        .criticalStretch = 0.0075f
    };

    constexpr SceneParameters crawlingSceneParameters = {
        .firstLameParameter = 0.889e4f,
        .secondLameParameter = 1.333e4f,
        .hardeningCoefficient = 10.0f,
        .criticalCompression = 0.025f,
        .criticalStretch = 0.0075f
    };

    Program::ApplySceneParameters(snowfallSceneParameters);

    Snowfall snowfall;

    try
    {
        snowfall.BuildInitialPositions();
        snowfall.BuildParticleBlocks();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    Particle snowfallParticles(snowfall.initialPositions);

    Simulation simulation(snowfall.particleCount, snowfall.initialBlocks.data(), static_cast<int>(snowfall.initialBlocks.size()), snowfallParticles.GetVBO());

    constexpr glm::mat4 logoTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(1.3f, 1.2f, 2.56f));
    // Blender Moment
    const glm::mat4 logoRotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::mat4 logoModelMatrix = logoTranslation * logoRotation;

    std::vector<std::array<glm::vec3, 3>> logoTriangles = logoUBB.GetTriangles(logoModelMatrix);
    std::vector<uint8_t> solidCells = MeshBoundary::Voxelize(logoTriangles, Program::cellCountPerAxis, Program::cellSize);

    simulation.UploadMeshBoundary(solidCells);

    camera.SetInitialOrientation(glm::vec3(2.56f, 1.5f, 7.5f), -90.0f, 0.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    DepthFBO depthFBO;
    depthFBO.Create(Program::currentWidth, Program::currentHeight);
    Program::depthFBO = &depthFBO;

    FullscreenQuad fullscreenQuad;

    int activeScene = 1;

    while (!glfwWindowShouldClose(program.window))
    {
        program.UpdateDeltaTime();

        if (program.WasFirstSceneSelected())
        {
            activeScene = 1;
            Program::ApplySceneParameters(snowfallSceneParameters);
            simulation.UploadMeshBoundary(solidCells);
            simulation.Reset(snowfall.initialBlocks.data(), static_cast<int>(snowfall.initialBlocks.size()));
        }

        if (program.WasSecondSceneSelected())
        {
            activeScene = 2;
            Program::ApplySceneParameters(crawlingSceneParameters);
            simulation.ClearMeshBoundary();
            simulation.Reset(snowfall.initialBlocks.data(), static_cast<int>(snowfall.initialBlocks.size()));
        }

        if (program.WasPauseKeyPressed())
        {
            program.SwitchPause();
        }

        program.UpdateKeyStates();
        program.UpdateFPSOnWindowTitle();

        camera.UpdateSpeed(program.deltaTime);

        program.ProcessInput();
        camera.ProcessInput();

        if (!program.IsPaused())
        {
            for (int step = 0; step < 6; step++)
            {
                simulation.Step();
            }

            simulation.SyncPositionsToVBO();
        }

        depthFBO.Bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        particleDepthShader.Use();
        particleDepthShader.SetMat4("view", camera.GetViewMatrix());
        particleDepthShader.SetMat4("projection", camera.GetProjectionMatrix());
        particleDepthShader.SetFloat("sphereRadius", 0.012f);
        particleDepthShader.SetFloat("viewportHeight", static_cast<float>(Program::currentHeight));
        snowfallParticles.Draw(particleDepthShader);

        depthFBO.Unbind();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        normalReconstructShader.Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthFBO.GetDepthTexture());
        normalReconstructShader.SetInt("depthTexture", 0);
        normalReconstructShader.SetVec2("resolution", glm::vec2(static_cast<float>(Program::currentWidth), static_cast<float>(Program::currentHeight)));
        normalReconstructShader.SetMat4("projection", camera.GetProjectionMatrix());
        fullscreenQuad.Draw();

        if (activeScene == 1)
        {
            sceneShader.Use();

            sceneShader.SetMat4("view", camera.GetViewMatrix());
            sceneShader.SetMat4("projection", camera.GetProjectionMatrix());
            sceneShader.SetMat4("model", logoModelMatrix);
            sceneShader.SetMat3("normalMatrix", glm::mat3(glm::transpose(glm::inverse(logoModelMatrix))));
            sceneShader.SetVec3("viewPosition", camera.GetPosition());

            logoUBB.Draw(sceneShader);
        }

        particleShader.Use();

        glm::mat4 viewMatrix = camera.GetViewMatrix();
        glm::mat4 projMatrix = camera.GetProjectionMatrix();
        glm::vec3 lightDirEye = glm::normalize(glm::vec3(viewMatrix * glm::vec4(direction, 0.0f)));

        particleShader.SetMat4("view", viewMatrix);
        particleShader.SetMat4("projection", projMatrix);
        particleShader.SetFloat("sphereRadius", 0.012f);
        particleShader.SetFloat("viewportHeight", static_cast<float>(Program::currentHeight));
        particleShader.SetVec3("lightDirEye", lightDirEye);

        snowfallParticles.Draw(particleShader);

        glfwSwapBuffers(program.window);
        glfwPollEvents();
    }

    return 0;
}
