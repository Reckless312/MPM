#include <algorithm>
#include <optional>
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
#include "OpenGL/Render/Box.h"
#include "OpenGL/Render/Particle.h"
#include "OpenGL/Render/VideoRecorder.h"
#include "OpenGL/Scene/Camera.h"
#include "OpenGL/Shaders/Shader.h"
#include "OpenGL/Simulation/Snowfall.h"
#include "OpenGL/Simulation/SnowLayer.h"

int main()
{
    Program program;

    try
    {
        Program::InitializeGLFW();
        if (Program::recordingMode)
        {
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        }
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

    try
    {
        sceneShader.Load();
        particleShader.Load();
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
        .firstLameParameter = 3.889e4f,
        .secondLameParameter = 5.833e4f,
        .hardeningCoefficient = 10.0f,
        .criticalCompression = 0.025f,
        .criticalStretch = 0.0075f
    };

    constexpr SceneParameters snowGroundSceneParameters = {
        .firstLameParameter = 1.5e4f,
        .secondLameParameter = 2.5e4f,
        .hardeningCoefficient = 3.0f,
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

    SnowLayer snowLayer;

    try
    {
        snowLayer.BuildInitialPositions();
        snowLayer.BuildParticleBlocks();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    constexpr glm::vec3 boxHalfExtents(0.2f, 0.2f, 0.2f);
    float boxCenterZ = 0.7f;

    Box box(boxHalfExtents);

    const int maxParticleCount = std::max(snowfall.particleCount, snowLayer.particleCount);

    std::vector maxPositions(maxParticleCount, glm::vec3(0.0f));
    std::ranges::copy(snowfall.initialPositions, maxPositions.begin());
    Particle snowfallParticles(maxPositions);

    Simulation simulation(maxParticleCount, snowfall.particleCount, snowfall.initialBlocks.data(), static_cast<int>(snowfall.initialBlocks.size()), snowfallParticles.GetVBO());

    constexpr glm::mat4 logoTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(1.3f, 1.2f, 2.56f));
    // Blender Moment
    const glm::mat4 logoRotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::mat4 logoModelMatrix = logoTranslation * logoRotation;

    std::vector<std::array<glm::vec3, 3>> logoTriangles = logoUBB.GetTriangles(logoModelMatrix);
    MeshSDF solidCells = MeshBoundary::Voxelize(logoTriangles, Program::cellCountPerAxis, Program::cellSize);

    simulation.UploadMeshBoundary(solidCells);

    camera.SetInitialOrientation(glm::vec3(2.56f, 1.5f, 7.5f), -90.0f, 0.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    int activeScene = 1;
    const int substepsPerFrame = Program::recordingMode ? Program::RecordingSubstepsPerFrame() : 5;

    int recordingScene = 1;
    std::optional<VideoRecorder> recorder;

    if (Program::recordingMode)
    {
        recorder.emplace(Program::currentWidth, Program::currentHeight, Program::recordingOutputPathScene1);
    }

    while (!glfwWindowShouldClose(program.window))
    {
        program.UpdateDeltaTime();

        if (program.WasFirstSceneSelected())
        {
            activeScene = 1;
            Program::ApplySceneParameters(snowfallSceneParameters);
            simulation.ClearBoxBoundary();
            simulation.UploadMeshBoundary(solidCells);
            simulation.Reset(snowfall.initialBlocks.data(), static_cast<int>(snowfall.initialBlocks.size()), snowfall.particleCount);
        }

        if (program.WasSecondSceneSelected())
        {
            activeScene = 2;
            Program::ApplySceneParameters(snowGroundSceneParameters);
            simulation.ClearMeshBoundary();
            simulation.Reset(snowLayer.initialBlocks.data(), static_cast<int>(snowLayer.initialBlocks.size()), snowLayer.particleCount);
            boxCenterZ = 0.7f;
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
            for (int step = 0; step < substepsPerFrame; step++)
            {
                if (activeScene == 2)
                {
                    constexpr float boxMaxZ = 3.8f;
                    constexpr float boxSpeed = 3.0f;

                    if (boxCenterZ < boxMaxZ)
                    {
                        boxCenterZ += boxSpeed * Program::physicsTimeStep;
                    }

                    const glm::vec3 boxCenter(2.56f, boxHalfExtents.y, boxCenterZ);
                    const glm::vec3 currentBoxVelocity = boxCenterZ < boxMaxZ ? glm::vec3(0.0f, 0.0f, boxSpeed) : glm::vec3(0.0f);
                    simulation.SetBoxBoundary(boxCenter, boxHalfExtents, currentBoxVelocity);
                }

                simulation.Step();
            }

            simulation.SyncPositionsToVBO();
        }

        if (recorder)
        {
            recorder->BeginFrame();
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sceneShader.Use();
        sceneShader.SetMat4("view", camera.GetViewMatrix());
        sceneShader.SetMat4("projection", camera.GetProjectionMatrix());
        sceneShader.SetVec3("viewPosition", camera.GetPosition());

        if (activeScene == 1)
        {
            sceneShader.SetMat4("model", logoModelMatrix);
            sceneShader.SetMat3("normalMatrix", glm::mat3(glm::transpose(glm::inverse(logoModelMatrix))));
            sceneShader.SetVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));

            logoUBB.Draw(sceneShader);
        }

        if (activeScene == 2)
        {
            const glm::vec3 boxCenter(2.56f, boxHalfExtents.y, boxCenterZ);
            const glm::mat4 boxModelMatrix = glm::translate(glm::mat4(1.0f), boxCenter);

            sceneShader.SetMat4("model", boxModelMatrix);
            sceneShader.SetMat3("normalMatrix", glm::mat3(1.0f));
            sceneShader.SetVec3("objectColor", glm::vec3(0.2f, 0.4f, 0.9f));

            box.Draw(sceneShader);
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

        if (!recorder)
        {
            glfwSwapBuffers(program.window);
            glfwPollEvents();
            continue;
        }

        recorder->EndFrame();

        if (!recorder->IsDone())
        {
            glfwPollEvents();
            continue;
        }

        if (recordingScene == 1)
        {
            recordingScene = 2;
            activeScene = 2;

            Program::ApplySceneParameters(snowGroundSceneParameters);

            simulation.ClearMeshBoundary();
            simulation.Reset(snowLayer.initialBlocks.data(), static_cast<int>(snowLayer.initialBlocks.size()), snowLayer.particleCount);

            boxCenterZ = 0.7f;
            recorder.emplace(Program::currentWidth, Program::currentHeight, Program::recordingOutputPathScene2);
        }
        else
        {
            break;
        }
    }

    return 0;
}
