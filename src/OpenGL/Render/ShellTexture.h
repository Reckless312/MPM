#ifndef MPM_METHOD_SHELL_TEXTURE_H
#define MPM_METHOD_SHELL_TEXTURE_H

#include <glad/glad.h>

#include "Particle.h"
#include "OpenGL/Scene/Camera.h"
#include "OpenGL/Shaders/Shader.h"

class ShellTexture
{
public:
    ShellTexture(const char* colorPath, const char* specularPath);
    ~ShellTexture();

    void Load();

    static void SetShellCameraUniforms(const Shader &shader, const Camera& camera);
    void Bind(const Shader& shader) const;
    void DrawParticles(const Shader& shader, const Particle& particles) const;
private:
    GLuint colorTextureArray{};
    GLuint specularTextureArray{};

    std::string fullColorPath{};
    std::string fullSpecularPath{};

    float shellInnerFraction = 0.85f;
    float particleShellRadius = 0.018f;

    int shellCount = 8;
};

#endif
