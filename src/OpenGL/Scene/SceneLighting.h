#ifndef MPM_METHOD_SCENELIGHTING_H
#define MPM_METHOD_SCENELIGHTING_H
#include <glm/vec3.hpp>
#include "OpenGL/Shaders/Shader.h"

namespace SceneLighting
{
    inline constexpr glm::vec3 directionLight(-0.2f, -1.0f, -0.3f);
    inline constexpr glm::vec3 ambientLight(0.5f, 0.5f, 0.5f);
    inline constexpr glm::vec3 diffuseLight(0.8f, 0.8f, 0.8f);
    inline constexpr glm::vec3 specularLight(1.0f, 1.0f, 1.0f);
    inline constexpr float shininess = 32.0f;

    inline void Set(const Shader& sceneShader)
    {
        sceneShader.Use();

        sceneShader.SetVec3("directionalLight.direction", directionLight);
        sceneShader.SetVec3("directionalLight.ambient", ambientLight);
        sceneShader.SetVec3("directionalLight.diffuse", diffuseLight);
        sceneShader.SetVec3("directionalLight.specular", specularLight);
        sceneShader.SetFloat("material.shininess", shininess);
    }
}

#endif
