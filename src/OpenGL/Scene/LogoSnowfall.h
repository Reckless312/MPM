#ifndef MPM_METHOD_LOGOSNOWFALL_H
#define MPM_METHOD_LOGOSNOWFALL_H
#include <glm/vec3.hpp>

#include "OpenGL/Shaders/Shader.h"

namespace LogoSnowfall
{
    inline constexpr glm::vec3 boxLeftCorner(0.5f, 3.0f, 2.1f);
    inline constexpr glm::vec3 boxRightCorner(4.6f, 4.5f, 3.1f);
    inline constexpr glm::vec3 cameraPosition(2.56f, 1.5f, 7.5f);

    inline constexpr float cameraYaw = -90.0f;
    inline constexpr float cameraPitch = 0.0f;
    inline constexpr float snowDensity = 400.0f;

    inline constexpr int particleCount = 10000;

    inline glm::mat4 GetLogoModelMatrix()
    {
        constexpr glm::mat4 logoTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 1.2f, 2.56f));
        const glm::mat4 logoRotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        const glm::mat4 logoScale = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f / 2.5f));

        return logoTranslation * logoRotation * logoScale;
    }

    inline void SetLogoUniforms(const Shader &shader, const glm::mat4 &model)
    {
        const glm::mat3 normal = glm::mat3(glm::transpose(glm::inverse(model)));

        shader.SetMat4("model", model);
        shader.SetMat3("normalMatrix", normal);
    }
}

#endif
