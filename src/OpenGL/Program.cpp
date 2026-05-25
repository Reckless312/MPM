#include <cmath>
#include <iostream>
#include <ostream>

#include "Program.h"
#include "OpenGL/Scene/Camera.h"
#include "SimulationConfig.h"

Program::Program()
{
    this->window = nullptr;
}

Program::~Program()
{
    glfwTerminate();
}

void Program::SwitchPause()
{
    this->paused = !this->paused;
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

void Program::CreateWindowAndAssignContext()
{
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);

    Program::currentWidth = videoMode->width;
    Program::currentHeight = videoMode->height;

    GLFWmonitor* monitor = Program::recordingMode ? nullptr : primaryMonitor;
    this->window = glfwCreateWindow(Program::currentWidth, Program::currentHeight, Program::windowTitle, monitor, Program::windowToShareResources);

    if (this->window == nullptr)
    {
        throw MPMException("Failed to create GLFW window.", Error::GLFWCreateWindow);
    }

    glfwMakeContextCurrent(this->window);
}

void Program::LoadGladLibrary()
{
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        throw MPMException("Failed to initialize GLAD.", Error::GladLoadLibrary);
    }
}

void Program::SetViewportAndResizeCallback() const
{
    glViewport(Program::viewportBottomLeftX, Program::viewportBottomLeftY, Program::currentWidth, Program::currentHeight);

    glfwSetFramebufferSizeCallback(this->window, Program::ResizeWindow);
}

bool Program::WasFirstSceneSelected() const
{
    return this->firstSceneKeyPressed && !this->firstSceneKeyWasDown;
}

bool Program::WasSecondSceneSelected() const
{
    return this->secondSceneKeyPressed && !this->secondSceneKeyWasDown;
}

bool Program::WasPauseKeyPressed() const
{
    return this->pauseKeyPressed && !this->pauseKeyWasDown;
}

bool Program::IsPaused() const
{
    return this->paused;
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

int Program::ReportErrorAndTerminate(const MPMException &exception)
{
    std::cout << exception.what() << std::endl;
    return static_cast<int>(exception.GetErrorType());
}

void Program::ProcessInput()
{
    if (glfwGetKey(this->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(this->window, true);
    }

    this->firstSceneKeyPressed = glfwGetKey(this->window, GLFW_KEY_1) == GLFW_PRESS;
    this->secondSceneKeyPressed = glfwGetKey(this->window, GLFW_KEY_2) == GLFW_PRESS;
    this->pauseKeyPressed = glfwGetKey(this->window, GLFW_KEY_SPACE) == GLFW_PRESS;
}

void Program::UpdateKeyStates()
{
    this->firstSceneKeyWasDown = this->firstSceneKeyPressed;
    this->secondSceneKeyWasDown = this->secondSceneKeyPressed;
    this->pauseKeyWasDown = this->pauseKeyPressed;
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

void Program::UpdateFPSOnWindowTitle() const
{
    if (this->deltaTime > 0.0f)
    {
        const int fps = static_cast<int>(1.0f / this->deltaTime);
        const std::string title = std::string(Program::windowTitle) + " | FPS: " + std::to_string(fps);
        glfwSetWindowTitle(this->window, title.c_str());
    }
}
