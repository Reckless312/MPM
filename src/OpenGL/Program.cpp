#include <cmath>
#include <iostream>
#include <ostream>

#include "Program.h"
#include "OpenGL/Scene/Camera.h"
#include "Scene/SnowSled.h"
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

    if (Program::recordingMode)
    {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }
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

int Program::GetActiveScene() const
{
    return this->activeScene;
}

int Program::GetRecordingScene() const
{
    return this->recordingScene;
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

void Program::SetRecordingScene(const int scene)
{
    this->recordingScene = scene;
}

void Program::CreateWindowAndAssignContext()
{
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);

    Program::currentWidth = videoMode->width;
    Program::currentHeight = videoMode->height;

    GLFWmonitor* monitor = primaryMonitor;

    if (Program::recordingMode)
    {
        monitor = nullptr;
    }

    this->window = glfwCreateWindow(Program::currentWidth, Program::currentHeight, Program::windowTitle, monitor, Program::windowToShareResources);

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

void Program::ChangeScene(const int scene, Simulation& simulation, const MeshSDF& boundarySDF, Particle& particles, const SnowVolume& snowVolume)
{
    simulation.UnregisterVBO();

    this->activeScene = scene;

    SimulationConfig::SwitchScenesParameters(this->activeScene);

    constexpr glm::vec3 nullifyVelocity = glm::vec3(0.0f);

    simulation.UpdatePhysicsParams();
    simulation.UploadMeshBoundary(boundarySDF);
    simulation.SetBoundaryVelocity(nullifyVelocity);
    particles.ResizeVBO(snowVolume.GetParticleCount());
    simulation.Reset(snowVolume);

    simulation.RebindVBO(particles.GetVBO());

    Camera::ChangeOrientationOnScene(this->window, this->activeScene);
}

void Program::ProcessInput(Simulation*& activeSimulation, Particle*& activeParticles, const std::array<Simulation*, 3> &simulations, const std::array<const MeshSDF*, 3> &boundarySDFs, const std::array<Particle*, 3> &particles, const std::array<const SnowVolume*, 3> &snowVolumes)
{
    if (glfwGetKey(this->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(this->window, true);
    }

    if (this->IsKeyJustPressed(GLFW_KEY_SPACE))
    {
        this->paused = !this->paused;
    }

    if (this->IsKeyJustPressed(GLFW_KEY_1))
    {
        this->ChangeScene(1, *simulations[0], *boundarySDFs[0], *particles[0], *snowVolumes[0]);
        activeSimulation = simulations[0];
        activeParticles = particles[0];
    }

    if (this->IsKeyJustPressed(GLFW_KEY_2))
    {
        this->ChangeScene(2, *simulations[1], *boundarySDFs[1], *particles[1], *snowVolumes[1]);
        activeSimulation = simulations[1];
        activeParticles = particles[1];

        SnowSled::sledCenterZ = SnowSled::sledInitialZ;
        SnowSled::sledAccumulatedZ = 0.0f;
    }

    if (this->IsKeyJustPressed(GLFW_KEY_3))
    {
        this->ChangeScene(3, *simulations[2], *boundarySDFs[2], *particles[2], *snowVolumes[2]);
        activeSimulation = simulations[2];
        activeParticles = particles[2];
    }
}
