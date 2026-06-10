#include "Camera.h"

#include <glm/gtc/quaternion.hpp>

#include "LogoSnowfall.h"
#include "Snowfall.h"
#include "SnowSled.h"
#include "Exceptions/MPMException.h"

Camera::Camera(GLFWwindow* window)
{
    this->window = window;

    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(this->window, &framebufferWidth, &framebufferHeight);

    this->mouseXDirection = static_cast<float>(framebufferWidth) / 2.0f;
    this->mouseYDirection = static_cast<float>(framebufferHeight) / 2.0f;

    this->position = glm::vec3(0.0f, 0.0f, 3.0f);
    this->front = glm::vec3(0.0f, 0.0f, -1.0f);
    this->up = glm::vec3(0.0f, 1.0f, 0.0f);

    this->UpdateRightVector();
    this->UpdateViewMatrix();
    this->UpdateProjectionMatrix();
}

glm::mat4 Camera::GetViewMatrix() const
{
    return this->viewMatrix;
}

glm::mat4 Camera::GetProjectionMatrix() const
{
    return this->projectionMatrix;
}

glm::vec3 Camera::GetPosition() const
{
    return this->position;
}

void Camera::ChangeOrientationOnScene(GLFWwindow *window, const int scene)
{
    if (auto* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window)))
    {
        if (scene == 1)
        {
            camera->SetInitialOrientation(LogoSnowfall::cameraPosition, LogoSnowfall::cameraYaw, LogoSnowfall::cameraPitch);
        }
        else if (scene == 2)
        {
            camera->SetInitialOrientation(SnowSled::cameraPosition, SnowSled::cameraYaw, SnowSled::cameraPitch);
        }
        else if (scene == 3)
        {
            camera->SetInitialOrientation(Snowfall::cameraPosition, Snowfall::cameraYaw, Snowfall::cameraPitch);
        }
    }
    else
    {
        throw MPMException("Failed to get GLFW window user pointer.", Error::GLFWLoadUserPointer);
    }
}

void Camera::ScrollCallback(GLFWwindow *window, double xOffset, const double yOffset)
{
    if (auto* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window)))
    {
        camera->UpdateFov(static_cast<float>(yOffset));
        camera->UpdateProjectionMatrix();
    }
    else
    {
        throw MPMException("Failed to get GLFW window user pointer.", Error::GLFWLoadUserPointer);
    }
}

void Camera::MouseCallback(GLFWwindow *window, const double xPosition, const double yPosition)
{
    if (auto* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window)))
    {
        camera->UpdateMousePosition(static_cast<float>(xPosition), static_cast<float>(yPosition));
    }
    else
    {
        throw MPMException("Failed to get GLFW window user pointer.", Error::GLFWLoadUserPointer);
    }
}

void Camera::ProcessInput()
{
    if (glfwGetKey(this->window, GLFW_KEY_W) == GLFW_PRESS)
    {
        this->position += this->speed * this->front;
        this->UpdateViewMatrix();
    }
    if (glfwGetKey(this->window, GLFW_KEY_S) == GLFW_PRESS)
    {
        this->position -= this->speed * this->front;
        this->UpdateViewMatrix();
    }
    if (glfwGetKey(this->window, GLFW_KEY_A) == GLFW_PRESS)
    {
        this->position -= this->right * this->speed;
        this->UpdateViewMatrix();
    }
    if (glfwGetKey(this->window, GLFW_KEY_D) == GLFW_PRESS)
    {
        this->position += this->right * this->speed;
        this->UpdateViewMatrix();
    }
}

void Camera::UpdateProjectionMatrix()
{
    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(this->window, &framebufferWidth, &framebufferHeight);

    const float aspectRatio = static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight);
    this->projectionMatrix = glm::perspective(glm::radians(this->fov), aspectRatio, this->nearPlane, this->farPlane);
}

void Camera::UpdateSpeed(const float deltaTime)
{
    this->speed = this->speedMultiplier * deltaTime;
}

void Camera::AssignUserPointerAndSetCallbacks()
{
    glfwSetWindowUserPointer(this->window, this);

    glfwSetCursorPosCallback(this->window, Camera::MouseCallback);
    glfwSetScrollCallback(this->window, Camera::ScrollCallback);
}

void Camera::SetInitialOrientation(const glm::vec3 desiredPosition, const float desiredYaw, const float desiredPitch)
{
    this->position = desiredPosition;
    this->yaw = desiredYaw;
    this->pitch = desiredPitch;

    this->UpdateDirection();
}

void Camera::UpdateDirection()
{
    this->direction.x = static_cast<float>(cos(glm::radians(yaw)) * cos(glm::radians(pitch)));
    this->direction.z = static_cast<float>(sin(glm::radians(yaw)) * cos(glm::radians(pitch)));
    this->direction.y = static_cast<float>(sin(glm::radians(pitch)));

    this->front = glm::normalize(this->direction);

    this->UpdateRightVector();
    this->UpdateViewMatrix();
}

void Camera::UpdateViewMatrix()
{
    this->viewMatrix = glm::lookAt(this->position, this->position + this->front, this->up);
}

void Camera::UpdateRightVector()
{
    this->right = glm::normalize(glm::cross(this->front, this->up));
}

void Camera::UpdateFov(const float yOffset)
{
    this->fov -= yOffset;

    if (fov < this->minFov)
    {
        fov = this->minFov;
    }

    if (fov > this->maxFov)
    {
        fov = this->maxFov;
    }
}

void Camera::UpdatePitch(const float yDirectionOffset)
{
    this->pitch += yDirectionOffset;

    if (this->pitch > this->maxPitch)
    {
        this->pitch = this->maxPitch;
    }

    if (this->pitch < this->minPitch)
    {
        this->pitch = this->minPitch;
    }
}

void Camera::UpdateMousePosition(const float currentXDirection, const float currentYDirection)
{
    if (this->firstMouse)
    {
        this->mouseXDirection = currentXDirection;
        this->mouseYDirection = currentYDirection;
        this->firstMouse = false;
    }

    float xDirectionOffset = currentXDirection - this->mouseXDirection;
    float yDirectionOffset = currentYDirection - this->mouseYDirection;

    this->mouseXDirection = currentXDirection;
    this->mouseYDirection = currentYDirection;

    xDirectionOffset *= this->sensitivity;
    yDirectionOffset *= this->sensitivity;

    this->yaw += xDirectionOffset;

    this->UpdatePitch(-yDirectionOffset);
    this->UpdateDirection();
}