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
#include "OpenGL/Render/IceCrystalTexture.h"
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
    Shader shellShader("shellVertex.vs", "shellFragment.fs");

    try
    {
        sceneShader.Load();
        particleShader.Load();
        shellShader.Load();
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

    Model sledModel(std::string(ASSETS_PATH) + "/sled.obj");

    try
    {
        sledModel.loadModel();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    Model floorModel(std::string(ASSETS_PATH) + "/floor.obj");

    try
    {
        floorModel.loadModel();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

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

    constexpr float sledScale = 1.5f;
    constexpr float sledCenterX = 2.1f;
    constexpr float sledInitialZ = 0.5f;
    constexpr float sledEndZ = 4.5f;
    constexpr float sledSpeed = 3.0f;
    float sledCenterZ = sledInitialZ;
    float sledAccumulatedZ = 0.0f;

    // 180° Y rotation so runner nose faces +Z (direction of travel); scale baked in once
    const glm::mat4 sledLocalMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(sledScale)) * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 sledStartMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(sledCenterX, 0.04f, sledInitialZ)) * sledLocalMatrix;
    MeshSDF initialSledSDF = MeshBoundary::Voxelize(sledModel.GetTriangles(sledStartMatrix), Program::cellCountPerAxis, Program::cellSize);

    camera.SetInitialOrientation(glm::vec3(2.56f, 1.5f, 7.5f), -90.0f, 0.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glClearColor(0.04f, 0.07f, 0.15f, 1.0f);

    IceCrystalTexture iceCrystalTexture;

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
            simulation.SetBoundaryVelocity(glm::vec3(0.0f));
            simulation.UploadMeshBoundary(solidCells);
            simulation.Reset(snowfall.initialBlocks.data(), static_cast<int>(snowfall.initialBlocks.size()), snowfall.particleCount);
            camera.SetInitialOrientation(glm::vec3(2.56f, 1.5f, 7.5f), -90.0f, 0.0f);
        }

        if (program.WasSecondSceneSelected())
        {
            activeScene = 2;
            Program::ApplySceneParameters(snowGroundSceneParameters);
            simulation.UploadMeshBoundary(initialSledSDF);
            simulation.Reset(snowLayer.initialBlocks.data(), static_cast<int>(snowLayer.initialBlocks.size()), snowLayer.particleCount);
            sledCenterZ = sledInitialZ;
            sledAccumulatedZ = 0.0f;
            camera.SetInitialOrientation(glm::vec3(-0.400f, 2.218f, 3.051f), -0.40f, -35.00f);
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
                    const bool sledMoving = sledCenterZ < sledEndZ;
                    simulation.SetBoundaryVelocity(sledMoving ? glm::vec3(0.0f, 0.0f, sledSpeed) : glm::vec3(0.0f));

                    if (sledMoving)
                    {
                        sledCenterZ += sledSpeed * Program::physicsTimeStep;
                        sledAccumulatedZ += sledSpeed * Program::physicsTimeStep;

                        if (sledAccumulatedZ >= Program::cellSize)
                        {
                            const int shiftCells = static_cast<int>(sledAccumulatedZ / Program::cellSize);
                            simulation.ShiftSdfZ(shiftCells);
                            sledAccumulatedZ -= static_cast<float>(shiftCells) * Program::cellSize;
                        }
                    }
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

        const float gridSize = Program::cellCountPerAxis * Program::cellSize;
        sceneShader.SetMat4("model", glm::scale(glm::mat4(1.0f), glm::vec3(gridSize, 1.0f, gridSize)));
        sceneShader.SetMat3("normalMatrix", glm::mat3(1.0f));
        sceneShader.SetVec3("objectColor", glm::vec3(0.12f, 0.18f, 0.28f));
        floorModel.Draw(sceneShader);

        if (activeScene == 1)
        {
            sceneShader.SetMat4("model", logoModelMatrix);
            sceneShader.SetMat3("normalMatrix", glm::mat3(glm::transpose(glm::inverse(logoModelMatrix))));
            sceneShader.SetVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));

            logoUBB.Draw(sceneShader);
        }

        if (activeScene == 2)
        {
            const glm::mat4 sledMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(sledCenterX, 0.04f, sledCenterZ)) * sledLocalMatrix;
            sceneShader.SetMat4("model", sledMatrix);
            sceneShader.SetMat3("normalMatrix", glm::mat3(glm::transpose(glm::inverse(sledMatrix))));
            sledModel.Draw(sceneShader);
        }

        shellShader.Use();

        const glm::mat4 viewMatrix = camera.GetViewMatrix();
        const glm::mat4 projMatrix = camera.GetProjectionMatrix();
        const glm::vec3 lightDirEye = glm::normalize(glm::vec3(viewMatrix * glm::vec4(direction, 0.0f)));

        shellShader.SetMat4("view", viewMatrix);
        shellShader.SetMat4("projection", projMatrix);
        shellShader.SetFloat("viewportHeight", static_cast<float>(Program::currentHeight));
        shellShader.SetVec3("lightDirEye", lightDirEye);

        iceCrystalTexture.Bind(shellShader);

        for (int shell = IceCrystalTexture::shellCount - 1; shell >= 0; shell--)
        {
            const float t = static_cast<float>(shell) / static_cast<float>(IceCrystalTexture::shellCount - 1);
            const float shellFraction = Program::shellInnerFraction + (1.0f - Program::shellInnerFraction) * t;
            shellShader.SetFloat("sphereRadius", Program::particleShellRadius * shellFraction);
            shellShader.SetInt("currentShell", shell);
            snowfallParticles.Draw(shellShader);
        }

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
            camera.SetInitialOrientation(glm::vec3(-0.400f, 2.218f, 3.051f), -0.40f, -35.00f);

            simulation.UploadMeshBoundary(initialSledSDF);
            simulation.Reset(snowLayer.initialBlocks.data(), static_cast<int>(snowLayer.initialBlocks.size()), snowLayer.particleCount);

            sledCenterZ = sledInitialZ;
            sledAccumulatedZ = 0.0f;
            recorder.emplace(Program::currentWidth, Program::currentHeight, Program::recordingOutputPathScene2);
        }
        else
        {
            break;
        }
    }

    return 0;
}
