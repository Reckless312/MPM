#include <glm/gtc/quaternion.hpp>

#include "Camera.h"
#include "Exceptions/MPMException.h"

Camera::Camera(GLFWwindow* window, GLsizei windowWidth, GLsizei windowHeight) : window(window)
{
    this->windowWidth = static_cast<float>(windowWidth);
    this->windowHeight = static_cast<float>(windowHeight);

    this->mouseXDirection = this->windowWidth / 2.0f;
    this->mouseYDirection = this->windowHeight / 2.0f;

    this->position = glm::vec3(0.0f, 0.0f, 3.0f);
    this->front = glm::vec3(0.0f, 0.0f, -1.0f);
    this->up = glm::vec3(0.0f, 1.0f, 0.0f);

    this->UpdateRightVector();
    this->UpdateViewMatrix();
    this->UpdateProjectionMatrix();
}

void Camera::UpdateViewMatrix()
{
    this->viewMatrix = glm::lookAt(this->position, this->position + this->front, this->up);
}

void Camera::UpdateRightVector()
{
    this->right = glm::normalize(glm::cross(this->front, this->up));
}

void Camera::UpdateSpeed(const float deltaTime)
{
    this->speed = this->speedMultiplier * deltaTime;
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

    // Negating to invert Up and Down (me not like it)
    this->UpdatePitch(-yDirectionOffset);
    this->UpdateDirection();
}

void Camera::UpdateProjectionMatrix()
{
    this->projectionMatrix = glm::perspective(glm::radians(this->fov),  this->windowWidth / this->windowHeight, this->nearPlane, this->farPlane);
}

void Camera::AssignUserPointerAndSetCallbacks()
{
    glfwSetWindowUserPointer(this->window, this);

    glfwSetCursorPosCallback(this->window, Camera::MouseCallback);
    glfwSetScrollCallback(this->window, Camera::ScrollCallback);
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