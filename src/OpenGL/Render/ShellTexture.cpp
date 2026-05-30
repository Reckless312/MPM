#include "OpenGL/Render/ShellTexture.h"

#include "OpenGL/Program.h"
#include "OpenGL/Scene/Camera.h"
#include "OpenGL/Scene/SceneLighting.h"
#include "OpenGL/Shaders/TextureLoader.h"

ShellTexture::ShellTexture(const char* colorPath, const char* specularPath)
{
    this->fullColorPath = std::string(ASSETS_PATH) + colorPath;
    this->fullSpecularPath = std::string(ASSETS_PATH) + specularPath;
}

ShellTexture::~ShellTexture()
{
    glDeleteTextures(1, &this->colorTextureArray);
    glDeleteTextures(1, &this->specularTextureArray);
}

void ShellTexture::Load()
{
    this->colorTextureArray = TextureLoader::LoadArray(fullColorPath.c_str(), shellCount);
    this->specularTextureArray = TextureLoader::LoadArray(fullSpecularPath.c_str(), shellCount);
}

void ShellTexture::SetShellCameraUniforms(const Shader &shader, const Camera& camera)
{
    const glm::vec3 lightDirectionView = glm::normalize(glm::vec3(camera.GetViewMatrix() * glm::vec4(SceneLighting::directionLight, 0.0f)));

    shader.SetMat4("view", camera.GetViewMatrix());
    shader.SetMat4("projection", camera.GetProjectionMatrix());
    shader.SetFloat("viewportHeight", static_cast<float>(Program::currentHeight));
    shader.SetVec3("lightDirectionView", lightDirectionView);
}

void ShellTexture::Bind(const Shader& shader) const
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, this->colorTextureArray);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, this->specularTextureArray);

    shader.SetInt("shellColorTextures", 0);
    shader.SetInt("shellSpecularTextures", 1);
    shader.SetInt("totalShells", this->shellCount);
}

void ShellTexture::DrawParticles(const Shader &shader, const Particle &particles) const
{
    for (int shell = this->shellCount - 1; shell >= 0; shell--)
    {
        const float shellDepth = static_cast<float>(shell) / static_cast<float>(this->shellCount - 1);
        const float shellFraction = this->shellInnerFraction + (1.0f - shellInnerFraction) * shellDepth;

        shader.SetFloat("sphereRadius", particleShellRadius * shellFraction);
        shader.SetInt("currentShell", shell);

        particles.Draw();
    }
}
