#include <cmath>
#include <iostream>
#include <ostream>

#include "Program.h"
#include "OpenGL/Scene/Camera.h"
#include "Scene/SnowSled.h"
#include "Scene/Snowfall.h"
#include "Simulation/SimulationConfig.h"

Program::Program()
{
    this->window = nullptr;
}

Program::~Program()
{
    glfwTerminate();
}

int Program::ReportErrorAndTerminate(const MPMException &exception)
{
    std::cout << exception.what() << std::endl;
    return static_cast<int>(exception.GetErrorType());
}

void Program::InitializeGLFW()
{
    glfwInitHint(GLFW_PLATFORM, Program::glfwPlatform);

    if (const int isInitialized = glfwInit(); isInitialized != GLFW_TRUE)
    {
        if (const int errorCode = glfwGetError(nullptr); errorCode == GLFW_PLATFORM_UNAVAILABLE)
        {
            throw MPMException("Platform specified is not correct! Please update with the right information.", Error::WrongPlatform);
        }

        throw MPMException("Failed to initialize GLFW.", Error::GLFWInitialization);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, Program::majorVersion);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, Program::minorVersion);
    glfwWindowHint(GLFW_OPENGL_PROFILE, Program::profile);
}

void Program::LoadGladLibrary()
{
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        throw MPMException("Failed to initialize GLAD.", Error::GladLoadLibrary);
    }
}

void Program::SetFloorUniforms(const Shader &shader)
{
    const glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(SimulationConfig::gridSize, 1.0f, SimulationConfig::gridSize));
    const glm::mat3 normal = glm::mat3(glm::transpose(glm::inverse(model)));

    constexpr glm::vec3 objectColor = glm::vec3(0.12f, 0.18f, 0.28f);

    shader.SetMat4("model", model);
    shader.SetMat3("normalMatrix", normal);
    shader.SetVec3("objectColor", objectColor);
}

void Program::ResizeWindow(GLFWwindow *window, const int width, const int height)
{
    glViewport(Program::viewportBottomLeftX, Program::viewportBottomLeftY, width, height);

    Program::currentWidth = width;
    Program::currentHeight = height;

    if (auto* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window)))
    {
        camera->UpdateProjectionMatrix();
    }
}

bool Program::IsKeyJustPressed(const int key)
{
    const bool current = glfwGetKey(this->window, key) == GLFW_PRESS;
    const bool pressed = current && !this->previousKeyStates[key];
    this->previousKeyStates[key] = current;

    return pressed;
}

bool Program::IsPaused() const
{
    return this->paused;
}

bool Program::IsInfoVisible() const
{
    return this->showInfo;
}

void Program::InitializeImGui() const
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().Fonts->AddFontDefault()->Scale = 1.5f;
    ImGui_ImplGlfw_InitForOpenGL(this->window, false);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void Program::ShutdownImGui() const
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Program::StartRecording()
{
    Program::recordingActive = true;

    SimulationConfig::simulationSubsteps = static_cast<int>(std::roundf(1.0f / (static_cast<float>(Program::recordingFrameRate) * SimulationConfig::physicsTimeStep)));
}

void Program::StopRecording() const
{
    Program::recordingActive = false;

    SimulationConfig::simulationSubsteps = SimulationConfig::defaultSimulationSubsteps;

    if (auto* camera = static_cast<Camera*>(glfwGetWindowUserPointer(this->window)))
    {
        camera->ResetMouseState();
    }
}

int Program::GetActiveScene() const
{
    return this->activeScene;
}

void Program::UpdateDeltaTime()
{
    const auto currentFrame = static_cast<float>(glfwGetTime());

    this->deltaTime = currentFrame - this->lastFrame;
    this->lastFrame = currentFrame;
}

void Program::LockCursor() const
{
    glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (const int errorCode = glfwGetError(nullptr); errorCode == GLFW_FEATURE_UNAVAILABLE)
    {
        std::cout << "Cursor is not available." << std::endl;
    }
}

void Program::CreateWindowAndAssignContext()
{
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);

    Program::currentWidth = videoMode->width;
    Program::currentHeight = videoMode->height;

    this->window = glfwCreateWindow(Program::currentWidth, Program::currentHeight, Program::windowTitle, primaryMonitor, Program::windowToShareResources);

    if (this->window == nullptr)
    {
        throw MPMException("Failed to create GLFW window.", Error::GLFWCreateWindow);
    }

    glfwMakeContextCurrent(this->window);
}

void Program::SetViewportAndResizeCallback() const
{
    glViewport(Program::viewportBottomLeftX, Program::viewportBottomLeftY, Program::currentWidth, Program::currentHeight);

    glfwSetFramebufferSizeCallback(this->window, Program::ResizeWindow);
}

void Program::SetSceneUniforms(const Shader& shader) const
{
    if (const auto* camera = static_cast<Camera*>(glfwGetWindowUserPointer(this->window)))
    {
        shader.SetMat4("view", camera->GetViewMatrix());
        shader.SetMat4("projection", camera->GetProjectionMatrix());
        shader.SetVec3("viewPosition", camera->GetPosition());
    }
}

void Program::ChangeScene(const int scene, Simulation& simulation, const MeshSDF& boundarySDF, Particle& particles, const SnowVolume& snowVolume, const int maxCapacity)
{
    simulation.UnregisterVBO();

    this->activeScene = scene;

    SimulationConfig::SwitchScenesParameters(this->activeScene);

    constexpr glm::vec3 nullifyVelocity = glm::vec3(0.0f);

    simulation.UpdatePhysicsParams();
    simulation.UploadMeshBoundary(boundarySDF);
    simulation.SetBoundaryVelocity(nullifyVelocity);
    particles.ResizeVBO(maxCapacity);

    simulation.Reset(snowVolume, maxCapacity);
    simulation.RebindVBO(particles.GetVBO());

    particles.SetCount(simulation.GetParticleCount());

    simulation.SyncPositionsToVBO();

    Camera::ChangeOrientationOnScene(this->window, this->activeScene);
}

void Program::ProcessInput(Simulation& simulation, Particle& particles, const std::array<const MeshSDF*, 3>& boundarySDFs, const std::array<const SnowVolume*, 3>& snowVolumes, const std::array<int, 3>& maxCapacities)
{
    if (glfwGetKey(this->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(this->window, true);
    }

    if (this->IsKeyJustPressed(GLFW_KEY_SPACE))
    {
        this->paused = !this->paused;
    }

    if (this->IsKeyJustPressed(GLFW_KEY_TAB))
    {
        this->showInfo = !this->showInfo;
    }

    if (this->IsKeyJustPressed(GLFW_KEY_R) && !Program::recordingActive)
    {
        this->ChangeScene(this->activeScene, simulation, *boundarySDFs[this->activeScene - 1], particles, *snowVolumes[this->activeScene - 1], maxCapacities[this->activeScene - 1]);

        if (this->activeScene == 2)
        {
            SnowSled::sledCenterZ = SnowSled::sledInitialZ;
            SnowSled::sledAccumulatedZ = 0.0f;
        }

        if (this->activeScene == 3)
        {
            Snowfall::spawnFrameCounter = 0;
        }

        this->recorder = std::make_unique<VideoRecorder>(Program::currentWidth, Program::currentHeight, Program::recordingDurationSeconds[this->activeScene - 1]);
        this->recorder->Open(Program::sceneOutputPaths[this->activeScene - 1]);

        Program::StartRecording();
        this->showInfo = true;
    }

    if (this->IsKeyJustPressed(GLFW_KEY_1))
    {
        this->ChangeScene(1, simulation, *boundarySDFs[0], particles, *snowVolumes[0], maxCapacities[0]);
    }

    if (this->IsKeyJustPressed(GLFW_KEY_2))
    {
        this->ChangeScene(2, simulation, *boundarySDFs[1], particles, *snowVolumes[1], maxCapacities[1]);

        SnowSled::sledCenterZ = SnowSled::sledInitialZ;
        SnowSled::sledAccumulatedZ = 0.0f;
    }

    if (this->IsKeyJustPressed(GLFW_KEY_3))
    {
        this->ChangeScene(3, simulation, *boundarySDFs[2], particles, *snowVolumes[2], maxCapacities[2]);

        Snowfall::spawnFrameCounter = 0;
    }
}
