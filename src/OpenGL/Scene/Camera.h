#ifndef MPM_METHOD_CAMERA_H
#define MPM_METHOD_CAMERA_H

#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

class Camera
{
public:
    explicit Camera(GLFWwindow* window);

    void ProcessInput();
    void UpdateSpeed(float deltaTime);
    void AssignUserPointerAndSetCallbacks();
    void SetInitialOrientation(glm::vec3 desiredPosition, float desiredYaw, float desiredPitch);
    void UpdateProjectionMatrix();

    [[nodiscard]] glm::vec3 GetPosition() const;

    [[nodiscard]] glm::mat4 GetViewMatrix() const;
    [[nodiscard]] glm::mat4 GetProjectionMatrix() const;

    static void MouseCallback(GLFWwindow* window, double xPosition, double yPosition);
    static void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset);
private:
    GLFWwindow* window;

    glm::vec3 up{};
    glm::vec3 right{};
    glm::vec3 direction{};
    glm::vec3 position{};
    glm::vec3 front{};

    glm::mat4 viewMatrix{};
    glm::mat4 projectionMatrix{};

    float mouseXDirection;
    float mouseYDirection;

    bool firstMouse = true;

    float speed = 5.0f;
    float yaw = -90.0f;
    float pitch = 0.0f;
    float fov = 45.0f;

    const float speedMultiplier = 5.0f;
    const float sensitivity = 0.1f;
    const float nearPlane = 0.1f;
    const float farPlane = 700.0f;
    const float maxPitch = 89.0f;
    const float minPitch = -89.0f;
    const float maxFov = 90.0f;
    const float minFov = 10.0f;

    void UpdateDirection();
    void UpdateViewMatrix();
    void UpdateRightVector();
    void UpdateFov(float yOffset);
    void UpdatePitch(float yDirectionOffset);
    void UpdateMousePosition(float currentXDirection, float currentYDirection);
};


#endif