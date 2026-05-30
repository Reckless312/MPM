#ifndef MPM_METHOD_CAMERA_H
#define MPM_METHOD_CAMERA_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

class Camera
{
public:
    explicit Camera(GLFWwindow* window);

    [[nodiscard]] glm::mat4 GetViewMatrix() const;
    [[nodiscard]] glm::mat4 GetProjectionMatrix() const;

    [[nodiscard]] glm::vec3 GetPosition() const;

    static void ChangeOrientationOnScene(GLFWwindow *window, int scene);
    static void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset);
    static void MouseCallback(GLFWwindow* window, double xPosition, double yPosition);

    void ProcessInput();
    void UpdateProjectionMatrix();
    void UpdateSpeed(float deltaTime);
    void AssignUserPointerAndSetCallbacks();
    void SetInitialOrientation(glm::vec3 desiredPosition, float desiredYaw, float desiredPitch);

private:
    GLFWwindow* window;

    glm::mat4 viewMatrix{};
    glm::mat4 projectionMatrix{};

    glm::vec3 up{};
    glm::vec3 right{};
    glm::vec3 front{};
    glm::vec3 position{};
    glm::vec3 direction{};

    const float speedMultiplier = 5.0f;
    const float sensitivity = 0.1f;
    const float nearPlane = 0.1f;
    const float farPlane = 700.0f;
    const float maxPitch = 89.0f;
    const float minPitch = -89.0f;
    const float maxFov = 90.0f;
    const float minFov = 10.0f;

    float speed = 5.0f;
    float yaw = -90.0f;
    float pitch = 0.0f;
    float fov = 45.0f;
    float mouseXDirection;
    float mouseYDirection;

    bool firstMouse = true;

    void UpdateDirection();
    void UpdateViewMatrix();
    void UpdateRightVector();
    void UpdateFov(float yOffset);
    void UpdatePitch(float yDirectionOffset);
    void UpdateMousePosition(float currentXDirection, float currentYDirection);
};


#endif