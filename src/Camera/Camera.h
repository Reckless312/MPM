#ifndef MPM_METHOD_CAMERA_H
#define MPM_METHOD_CAMERA_H

#include <glm/vec3.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>


class Camera {
public:
    Camera();

    void GoForward();
    void GoBackward();
    void GoLeft();
    void GoRight();

    void UpdateViewMatrix();
    void UpdateSpeed(float deltaTime);
    void UpdateDirection();

    [[nodiscard]] float GetLastXDirection() const;
    [[nodiscard]] float GetLastYDirection() const;

    void SetLastXDirection(float newXDirection);
    void SetLastYDirection(float newYDirection);

    void UpdateYaw(float xDirectionOffset);
    void UpdatePitch(float yDirectionOffset);

    glm::mat4 viewMatrix{};
private:
    glm::vec3 position{};
    glm::vec3 front{};
    glm::vec3 up{};
    glm::vec3 right{};
    glm::vec3 direction{};

    float speed = 2.5f;
    float lastXDirection{};
    float lastYDirection{};

    float yaw = -90.0f;
    float pitch = 0.0f;
};


#endif