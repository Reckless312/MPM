#include <algorithm>
#include <memory>
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
#include "OpenGL/Simulation/SimulationConfig.h"
#include "OpenGL/Render/Model.h"
#include "OpenGL/Render/ShellTexture.h"
#include "OpenGL/Render/Particle.h"
#include "OpenGL/Render/VideoRecorder.h"
#include "OpenGL/Scene/Camera.h"
#include "OpenGL/Scene/LogoSnowfall.h"
#include "OpenGL/Scene/SceneLighting.h"
#include "OpenGL/Scene/Snowfall.h"
#include "OpenGL/Scene/SnowSled.h"
#include "OpenGL/Shaders/Shader.h"
#include "OpenGL/Simulation/SnowVolume.h"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().Fonts->AddFontDefault()->Scale = 1.5f;
    ImGui_ImplGlfw_InitForOpenGL(program.window, false);
    ImGui_ImplOpenGL3_Init("#version 330");

    program.LockCursor();
    program.SetViewportAndResizeCallback();

    Camera camera(program.window);
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

    SceneLighting::Set(sceneShader);

    Model logoUBB("/Logo/ubb_logo.obj");
    Model sled("/Sled/sled.obj");
    Model doom("/Doom/doom.obj");
    Model floorModel("/Floor/floor.obj");

    try
    {
        logoUBB.loadModel();
        sled.loadModel();
        doom.loadModel();
        floorModel.loadModel();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    ShellTexture shellTexture("/Snow/crystal_color.png", "/Snow/crystal_specular.png");

    try
    {
        shellTexture.Load();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    SnowVolume logoSnowfall(LogoSnowfall::boxLeftCorner, LogoSnowfall::boxRightCorner, LogoSnowfall::particleCount, LogoSnowfall::snowDensity);
    SnowVolume snowSledLayer(SnowSled::boxLeftCorner, SnowSled::boxRightCorner, SnowSled::particleCount, SnowSled::snowDensity);
    SnowVolume snowfall(Snowfall::boxLeftCorner, Snowfall::boxRightCorner, Snowfall::particleBatchCount, Snowfall::snowDensity);

    try
    {
        logoSnowfall.BuildInitialPositions();
        logoSnowfall.BuildParticleBlocks();

        snowSledLayer.BuildInitialPositions();
        snowSledLayer.BuildParticleBlocks();

        snowfall.BuildInitialPositions();
        snowfall.BuildParticleBlocks();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    const glm::mat4 logoModelMatrix = LogoSnowfall::GetLogoModelMatrix();
    MeshSDF logoSDF = MeshBoundary::BuildSDF(logoUBB.GetTriangles(logoModelMatrix));

    const glm::mat4 sledModelMatrix = SnowSled::GetSledModelMatrix();
    MeshSDF sledSDF = MeshBoundary::BuildSDF(sled.GetTriangles(sledModelMatrix));

    MeshSDF doomSDF = MeshBoundary::BuildSDF(doom.GetTriangles(Snowfall::GetDoomModelMatrix()));

    Particle particles(logoSnowfall.GetInitialPositions());

    Simulation simulation(logoSnowfall, particles.GetVBO(), logoSnowfall.GetParticleCount());
    simulation.UploadMeshBoundary(logoSDF);

    camera.SetInitialOrientation(LogoSnowfall::cameraPosition, LogoSnowfall::cameraYaw, LogoSnowfall::cameraPitch);

    std::unique_ptr<VideoRecorder> recorder = std::make_unique<VideoRecorder>(Program::currentWidth, Program::currentHeight);

    if (Program::recordingMode)
    {
        try
        {
            recorder->Open(Program::logoSceneOutputPath);
        }
        catch (const MPMException& exception)
        {
            return Program::ReportErrorAndTerminate(exception);
        }
    }

    std::array<const SnowVolume*, 3> snowVolumes = { &logoSnowfall, &snowSledLayer, &snowfall };
    std::array<const MeshSDF*, 3> boundarySDFs = { &logoSDF, &sledSDF, &doomSDF };
    std::array maxCapacities = { LogoSnowfall::particleCount, SnowSled::particleCount, Snowfall::maxParticleCount };

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    glm::vec4 backgroundColor(0.04f, 0.07f, 0.15f, 1.0f);

    glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);

    while (!glfwWindowShouldClose(program.window))
    {
        program.UpdateDeltaTime();
        program.ProcessInput(simulation, particles, boundarySDFs, snowVolumes, maxCapacities);

        camera.UpdateSpeed(program.deltaTime);
        camera.ProcessInput();

        if (!program.IsPaused())
        {
            if (program.GetActiveScene() == 3 && simulation.GetParticleCount() < Snowfall::maxParticleCount)
            {
                try
                {
                    Snowfall::SpawnIfPossible(&simulation, &particles);
                }
                catch (const MPMException& exception)
                {
                    return Program::ReportErrorAndTerminate(exception);
                }
            }

            for (int step = 0; step < SimulationConfig::simulationSubsteps; step++)
            {
                if (program.GetActiveScene() == 2)
                {
                    SnowSled::Move(&simulation);
                }

                simulation.Step();
            }
        }

        simulation.SyncPositionsToVBO();

        if (Program::recordingMode)
        {
            recorder->BeginFrame();
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sceneShader.Use();
        program.SetSceneUniforms(sceneShader);

        Program::SetFloorUniforms(sceneShader);
        floorModel.Draw(sceneShader);

        if (program.GetActiveScene() == 1)
        {
            LogoSnowfall::SetLogoUniforms(sceneShader, logoModelMatrix);
            logoUBB.Draw(sceneShader);
        }

        if (program.GetActiveScene() == 2)
        {
            SnowSled::SetLogoUniforms(sceneShader);
            sled.Draw(sceneShader);
        }

        if (program.GetActiveScene() == 3)
        {
            Snowfall::SetDoomUniforms(sceneShader);
            doom.Draw(sceneShader);
        }

        shellShader.Use();
        ShellTexture::SetShellCameraUniforms(shellShader, camera);

        shellTexture.Bind(shellShader);
        shellTexture.DrawParticles(shellShader, particles);

        if (!Program::recordingMode)
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if (program.IsInfoVisible())
            {
                ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Always);
                ImGui::Begin("Info");
                ImGui::Text("FPS: %.1f", 1.0f / program.deltaTime);
                ImGui::Separator();
                ImGui::Text("1: Logo Scene");
                ImGui::Text("2: Sled Scene");
                ImGui::Text("3: Snowfall Scene");
                ImGui::Text("Space: Pause");
                ImGui::Text("Tab: Hide");
                ImGui::Text("Esc: Quit");
                ImGui::End();
            }

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(program.window);
            glfwPollEvents();
            continue;
        }

        try
        {
            recorder->EndFrame();
        }
        catch (const MPMException& exception)
        {
            return Program::ReportErrorAndTerminate(exception);
        }

        if (!recorder->IsDone())
        {
            glfwPollEvents();
            continue;
        }

        if (program.GetRecordingScene() < 3)
        {
            try
            {
                program.AdvanceRecordingScene(simulation, particles, boundarySDFs, snowVolumes, maxCapacities, recorder);
            }
            catch (const MPMException& exception)
            {
                return Program::ReportErrorAndTerminate(exception);
            }

            if (program.GetRecordingScene() == 2)
            {
                SnowSled::sledCenterZ = SnowSled::sledInitialZ;
                SnowSled::sledAccumulatedZ = 0.0f;
            }
        }
        else
        {
            break;
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return 0;
}