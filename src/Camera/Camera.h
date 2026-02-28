#ifndef MPM_METHOD_CAMERA_H
#define MPM_METHOD_CAMERA_H

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    glm::mat4 viewMatrix{};
    glm::mat4 projectionMatrix{};

    explicit Camera(GLFWwindow* window, GLsizei windowWidth, GLsizei windowHeight);

    void UpdateSpeed(float deltaTime);

    void AssignUserPointerAndSetCallbacks();
    void ProcessInput();

    static void MouseCallback(GLFWwindow* window, double xPosition, double yPosition);
    static void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset);
private:
    GLFWwindow* window;

    glm::vec3 position{};
    glm::vec3 front{};
    glm::vec3 up{};
    glm::vec3 right{};
    glm::vec3 direction{};

    float windowWidth;
    float windowHeight;
    float mouseXDirection;
    float mouseYDirection;

    float speed = 2.5f;

    float yaw = -90.0f;
    float pitch = 0.0f;
    float fov = 45.0f;

    bool firstMouse = true;

    const float speedMultiplier = 2.5f;
    const float sensitivity = 0.1f;

    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;

    const float maxPitch = 89.0f;
    const float minPitch = -89.0f;
    const float maxFov = 90.0f;
    const float minFov = 10.0f;

    void UpdateViewMatrix();
    void UpdateRightVector();
    void UpdateDirection();
    void UpdateProjectionMatrix();

    void UpdatePitch(float yDirectionOffset);
    void UpdateFov(float yOffset);
    void UpdateMousePosition(float currentXDirection, float currentYDirection);
};


#endif