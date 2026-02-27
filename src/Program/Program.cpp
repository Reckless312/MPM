#include "Program.h"
#include "../Exceptions/ProgramException.h"

#include <iostream>
#include <ostream>

#include "Camera/Camera.h"

Program::Program(Camera &camera) : window(nullptr), camera(&camera) {
    camera.SetLastXDirection(Program::windowWidth / 2.0f);
    camera.SetLastYDirection(Program::windowHeight / 2.0f);
}

void Program::InitializeGLFW() {
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, Program::majorVersion);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, Program::minorVersion);

    glfwWindowHint(GLFW_OPENGL_PROFILE, Program::profile);
}

void Program::CreateWindowAndAssignContext() {
    this->window = glfwCreateWindow(Program::windowWidth, Program::windowHeight, Program::windowTitle, Program::fullscreenMonitor, Program::windowToShareResources);

    if (this->window == nullptr)
    {
        throw ProgramException("Failed to create GLFW window.");
    }

    glfwSetWindowUserPointer(this->window, this);
    glfwMakeContextCurrent(this->window);
}

void Program::LoadGladLibrary() {
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        throw ProgramException("Failed to initialize GLAD.");
    }
}

void Program::SetViewportAndCallbacks() const {
    glViewport(Program::viewportBottomLeftX, Program::viewportBottomLeftY, Program::windowWidth, Program::windowHeight);

    glfwSetFramebufferSizeCallback(this->window, Program::ResizeWindow);
    glfwSetCursorPosCallback(this->window, Program::MouseCallback);
}

void Program::ResizeWindow(GLFWwindow *window, const int width, const int height) {
    glViewport(Program::viewportBottomLeftX, Program::viewportBottomLeftY, width, height);
}

void Program::MouseCallback(GLFWwindow *window, const double xPosition, const double yPosition) {
    if (auto* program = static_cast<Program*>(glfwGetWindowUserPointer(window))) {
        program->UpdateMousePosition(static_cast<float>(xPosition), static_cast<float>(yPosition));
    }
    else {
        throw ProgramException("Failed to get GLFW window user pointer.");
    }
}

void Program::ProcessInput() const {
    if (glfwGetKey(this->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(this->window, true);
    }

    if (glfwGetKey(this->window, GLFW_KEY_W) == GLFW_PRESS) {
        camera->GoForward();
    }
    if (glfwGetKey(this->window, GLFW_KEY_S) == GLFW_PRESS) {
        camera->GoBackward();
    }
    if (glfwGetKey(this->window, GLFW_KEY_A) == GLFW_PRESS) {
        camera->GoLeft();
    }
    if (glfwGetKey(this->window, GLFW_KEY_D) == GLFW_PRESS) {
        camera->GoRight();
    }
}

void Program::UpdateDeltaTime() {
    const auto currentFrame = static_cast<float>(glfwGetTime());

    this->deltaTime = currentFrame - this->lastFrame;
    this->lastFrame = currentFrame;
}

void Program::LockCursor() const {
    glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Program::UpdateMousePosition(const float currentXDirection, const float currentYDirection) {
    if (this->firstMouse) {
        camera->SetLastXDirection(currentXDirection);
        camera->SetLastYDirection(currentYDirection);
        this->firstMouse = false;
    }

    float xDirectionOffset = currentXDirection - camera->GetLastXDirection();
    float yDirectionOffset = currentYDirection - camera->GetLastYDirection();

    camera->SetLastXDirection(currentXDirection);
    camera->SetLastYDirection(currentYDirection);

    constexpr float sensitivity = 0.1f;
    xDirectionOffset *= sensitivity;
    yDirectionOffset *= sensitivity;

    camera->UpdateYaw(xDirectionOffset);
    camera->UpdatePitch(-yDirectionOffset);

    camera->UpdateDirection();
}

void Program::SetDefaultBackgroundToPurple() {
    constexpr float red = 0.239f, green = 0.122f, blue = 0.361f, alpha = 1.0f;

    glClearColor(red, green, blue, alpha);
}

int Program::ReportErrorAndTerminate(const char *errorMessage) {
    std::cout << errorMessage << std::endl;
    glfwTerminate();
    return Program::failureCode;
}
