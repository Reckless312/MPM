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
#include "OpenGL/SimulationConfig.h"
#include "OpenGL/Render/Model.h"
#include "OpenGL/Render/IceCrystalTexture.h"
#include "OpenGL/Render/Particle.h"
#include "OpenGL/Render/VideoRecorder.h"
#include "OpenGL/Scene/Camera.h"
#include "OpenGL/Shaders/Shader.h"
#include "OpenGL/Simulation/SnowVolume.h"

void SetSceneLighting(const Shader& sceneShader);

glm::mat4 GetLogoLocalMatrix();
glm::mat4 GetSledLocalMatrix();

MeshSDF GetMeshSDF(const Model& model, const glm::mat4& worldMatrix);

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

    Camera camera(program.window, Program::currentWidth, Program::currentHeight);
    camera.AssignUserPointerAndSetCallbacks();

    Shader sceneShader("vertexShader.vs", "fragmentShader.fs");
    Shader shellShader("shellVertex.vs", "shellFragment.fs");

    try
    {
        sceneShader.Load();
        shellShader.Load();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    SetSceneLighting(sceneShader);

    Model logoUBB(std::string(ASSETS_PATH) + "/ubb_logo.obj");
    Model sledModel(std::string(ASSETS_PATH) + "/sled.obj");
    Model floorModel(std::string(ASSETS_PATH) + "/floor.obj");

    try
    {
        logoUBB.loadModel();
        sledModel.loadModel();
        floorModel.loadModel();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    constexpr glm::vec3 snowfallLowerLeft(0.5f, 3.0f, 2.1f);
    constexpr glm::vec3 snowfallUpperRight(4.6f, 4.5f, 3.1f);
    constexpr int snowfallParticleCount = 100000;

    constexpr glm::vec3 snowLayerLowerLeft(1.5f, 0.04f, 1.5f);
    constexpr glm::vec3 snowLayerUpperRight(3.6f, 0.2f, 3.6f);
    constexpr int snowLayerParticleCount = 100000;

    SnowVolume snowfall(snowfallLowerLeft, snowfallUpperRight, snowfallParticleCount);
    SnowVolume snowLayer(snowLayerLowerLeft, snowLayerUpperRight, snowLayerParticleCount);

    try
    {
        snowfall.BuildInitialPositions();
        snowfall.BuildParticleBlocks();

        snowLayer.BuildInitialPositions();
        snowLayer.BuildParticleBlocks();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    Particle snowfallParticles(snowfall.initialPositions);
    Particle snowLayerParticles(snowLayer.initialPositions);

    Simulation snowfallSimulation(snowfall, snowfallParticles.GetVBO());
    Simulation snowLayerSimulation(snowLayer, snowLayerParticles.GetVBO());

    glm::mat4 logoModelMatrix = GetLogoLocalMatrix();
    MeshSDF logoSDF = GetMeshSDF(logoUBB, logoModelMatrix);
    snowfallSimulation.UploadMeshBoundary(logoSDF);

    glm::mat4 sledModelMatrix = GetSledLocalMatrix();
    MeshSDF sledSDF = GetMeshSDF(sledModel, sledModelMatrix);
    snowLayerSimulation.UploadMeshBoundary(sledSDF);

    camera.SetInitialOrientation(glm::vec3(2.56f, 1.5f, 7.5f), -90.0f, 0.0f);

    IceCrystalTexture iceCrystalTexture;

    int activeScene = 1;
    const int substepsPerFrame = Program::recordingMode ? Program::RecordingSubstepsPerFrame() : 5;

    int recordingScene = 1;
    std::optional<VideoRecorder> recorder;

    if (Program::recordingMode)
    {
        recorder.emplace(Program::currentWidth, Program::currentHeight, Program::firstRecordingOutputPathScene);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    constexpr glm::vec4 backgroundColor(0.04f, 0.07f, 0.15f, 1.0f);
    glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);

    while (!glfwWindowShouldClose(program.window))
    {
        program.UpdateDeltaTime();

        if (program.IsKeyJustPressed(GLFW_KEY_1))
        {
            activeScene = 1;
            Program::ApplySceneParameters(snowfallSceneParameters);
            simulation.SetBoundaryVelocity(glm::vec3(0.0f));
            simulation.UploadMeshBoundary(solidCells);
            simulation.Reset(snowfall.initialBlocks.data(), static_cast<int>(snowfall.initialBlocks.size()), snowfall.particleCount);
            camera.SetInitialOrientation(glm::vec3(2.56f, 1.5f, 7.5f), -90.0f, 0.0f);
        }

        if (program.IsKeyJustPressed(GLFW_KEY_2))
        {
            activeScene = 2;
            Program::ApplySceneParameters(snowGroundSceneParameters);
            simulation.UploadMeshBoundary(initialSledSDF);
            simulation.Reset(snowLayer.initialBlocks.data(), static_cast<int>(snowLayer.initialBlocks.size()), snowLayer.particleCount);
            sledCenterZ = sledInitialZ;
            sledAccumulatedZ = 0.0f;
            camera.SetInitialOrientation(glm::vec3(-0.400f, 2.218f, 3.051f), -0.40f, -35.00f);
        }

        if (program.IsKeyJustPressed(GLFW_KEY_SPACE))
        {
            program.SwitchPause();
        }

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
                    constexpr float sledEndZ = 4.5f;
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
            const glm::mat4 sledMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, sledCenterZ)) * sledLocalMatrix;
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
            recorder.emplace(Program::currentWidth, Program::currentHeight, Program::secondRecordingOutputPathScene);
        }
        else
        {
            break;
        }
    }

    return 0;
}

void SetSceneLighting(const Shader& sceneShader)
{
    sceneShader.Use();

    sceneShader.SetVec3("directionalLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
    sceneShader.SetVec3("directionalLight.ambient", glm::vec3(0.5f, 0.5f, 0.5f));
    sceneShader.SetVec3("directionalLight.diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
    sceneShader.SetVec3("directionalLight.specular", glm::vec3(1.0f, 1.0f, 1.0f));
    sceneShader.SetFloat("material.shininess", 32.0f);
}

glm::mat4 GetLogoLocalMatrix()
{
    constexpr glm::mat4 logoTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(1.3f, 1.2f, 2.56f));
    // Absolute Blender Moment
    const glm::mat4 logoRotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    return logoTranslation * logoRotation;
}

glm::mat4 GetSledLocalMatrix()
{
    constexpr glm::mat4 sledTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(2.56f, 0.04f, 0.0f));
    const glm::mat4 sledScale = glm::scale(glm::mat4(1.0f), glm::vec3(0.8f));
    const glm::mat4 sledRotation = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 1, 0));

    return sledTranslation * sledScale * sledRotation;
}

MeshSDF GetMeshSDF(const Model& model, const glm::mat4& worldMatrix)
{
    const std::vector<std::array<glm::vec3, 3>> triangles = model.GetTriangles(worldMatrix);

    return MeshBoundary::Voxelize(triangles, SimulationConfig::cellCountPerAxis, SimulationConfig::cellSize);
}
