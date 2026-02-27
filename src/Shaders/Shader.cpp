#include <glad/glad.h>

#include "Shader.h"

#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

#include "../Exceptions/ShaderException.h"

Shader::Shader(const char* vertexShaderPath, const char* fragmentShaderPath) {
    this->id = glCreateProgram();
    this->vertexShaderPath = std::string(SHADERS_PATH) + vertexShaderPath;
    this->fragmentShaderPath = std::string(SHADERS_PATH) + fragmentShaderPath;
}

Shader::~Shader() {
    glDeleteProgram(this->id);
}

void Shader::Load() const {
    const std::string vertexStringCode = Shader::ReadShader(this->vertexShaderPath);
    const std::string fragmentStringCode = Shader::ReadShader(this->fragmentShaderPath);

    const char* vertexCode = vertexStringCode.c_str();
    const char* fragmentCode = fragmentStringCode.c_str();

    const unsigned int vertexId = glCreateShader(GL_VERTEX_SHADER);
    const unsigned int fragmentId = glCreateShader(GL_FRAGMENT_SHADER);
    constexpr GLsizei shaderArrayLength = 1;

    glShaderSource(vertexId, shaderArrayLength, &vertexCode, nullptr);
    glCompileShader(vertexId);
    this->CheckShaderError(vertexId);

    glShaderSource(fragmentId, shaderArrayLength, &fragmentCode, nullptr);
    glCompileShader(fragmentId);
    this->CheckShaderError(fragmentId);

    glAttachShader(this->id, vertexId);
    glAttachShader(this->id, fragmentId);
    glLinkProgram(this->id);
    this->CheckShaderProgramError();

    glDeleteShader(vertexId);
    glDeleteShader(fragmentId);
}

std::string Shader::ReadShader(const std::string& path) {
    std::string shaderCode;

    std::ifstream shaderFile;

    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        shaderFile.open(path);

        std::stringstream shaderStream;
        shaderStream << shaderFile.rdbuf();

        shaderFile.close();

        shaderCode = shaderStream.str();
    } catch (std::ifstream::failure& e) {
        throw ShaderException(e.what());
    }

    return shaderCode;
}

void Shader::Use() const {
    glUseProgram(this->id);
}

void Shader::SetBool(const std::string &name, const bool value) const {
    glUniform1i(glGetUniformLocation(this->id, name.c_str()), static_cast<int>(value));
}

void Shader::SetInt(const std::string &name, const int value) const {
    glUniform1i(glGetUniformLocation(this->id, name.c_str()), value);
}

void Shader::SetFloat(const std::string &name, const float value) const {
    glUniform1f(glGetUniformLocation(this->id, name.c_str()), value);
}

void Shader::SetMat4(const std::string &name, const glm::mat4 &matrix) const {
    glUniformMatrix4fv(glGetUniformLocation(this->id, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::CheckShaderError(const unsigned int shaderId) const {
    char infoLog[Shader::errorMessageSize];
    GLint success;

    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(shaderId, Shader::errorMessageSize, nullptr, infoLog);
        throw ShaderException(infoLog);
    }
}

void Shader::CheckShaderProgramError() const {
    char infoLog[Shader::errorMessageSize];
    GLint success;

    glGetProgramiv(this->id, GL_LINK_STATUS, &success);

    if (!success) {
        glGetProgramInfoLog(this->id, Shader::errorMessageSize, nullptr, infoLog);
        throw ShaderException(infoLog);
    }
}