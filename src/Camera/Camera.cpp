#include "Camera.h"

#include <glm/gtc/quaternion.hpp>

Camera::Camera() {
    this->position = glm::vec3(0.0f, 0.0f, 3.0f);
    this->front = glm::vec3(0.0f, 0.0f, -1.0f);
    this->up = glm::vec3(0.0f, 1.0f, 0.0f);

    this->right = glm::normalize(glm::cross(this->front, this->up));

    this->UpdateViewMatrix();
}

void Camera::GoForward() {
    this->position += this->speed * this->front;
    this->UpdateViewMatrix();
}

void Camera::GoBackward() {
    this->position -= this->speed * this->front;
    this->UpdateViewMatrix();
}

void Camera::GoLeft() {
    this->position -= this->right * this->speed;
    this->UpdateViewMatrix();
}

void Camera::GoRight() {
    this->position += this->right * this->speed;
    this->UpdateViewMatrix();
}

void Camera::UpdateViewMatrix() {
    this->viewMatrix = glm::lookAt(this->position, this->position + this->front, this->up);
}

void Camera::UpdateSpeed(const float deltaTime) {
    this->speed = 2.5f * deltaTime;
}

void Camera::UpdateDirection() {
    this->direction.x = static_cast<float>(cos(glm::radians(yaw)) * cos(glm::radians(pitch)));
    this->direction.z = static_cast<float>(sin(glm::radians(yaw)) * cos(glm::radians(pitch)));
    this->direction.y = static_cast<float>(sin(glm::radians(pitch)));

    this->front = glm::normalize(this->direction);
    this->right = glm::normalize(glm::cross(this->front, this->up));

    this->UpdateViewMatrix();
}

float Camera::GetLastXDirection() const {
    return this->lastXDirection;
}

float Camera::GetLastYDirection() const {
    return this->lastYDirection;
}

void Camera::SetLastXDirection(const float newXDirection) {
    this->lastXDirection = newXDirection;
}

void Camera::SetLastYDirection(const float newYDirection) {
    this->lastYDirection = newYDirection;
}

void Camera::UpdateYaw(const float xDirectionOffset) {
    this->yaw += xDirectionOffset;
}

void Camera::UpdatePitch(const float yDirectionOffset) {
    this->pitch += yDirectionOffset;

    if (this->pitch > 89.0f) {
        this->pitch = 89.0f;
    }

    if (this->pitch < -89.0f) {
        this->pitch = -89.0f;
    }
}
