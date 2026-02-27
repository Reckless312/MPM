#ifndef MPM_METHOD_SHADER_H
#define MPM_METHOD_SHADER_H
#include <string>
#include <GL/gl.h>
#include <glm/fwd.hpp>

class Shader {
public:
    Shader(const char* vertexShaderPath, const char* fragmentShaderPath);
    ~Shader();

    void Load() const;
    void Use() const;

    void SetBool(const std::string& name, bool value) const;
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetMat4(const std::string& name, const glm::mat4& matrix) const;

private:
    unsigned int id;

    std::string vertexShaderPath;
    std::string fragmentShaderPath;

    const int errorMessageSize = 512;

    static std::string ReadShader(const std::string& path);

    void CheckShaderError(unsigned int shaderId) const;
    void CheckShaderProgramError() const;
};


#endif