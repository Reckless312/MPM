#ifndef MPM_METHOD_SHADER_H
#define MPM_METHOD_SHADER_H
#include <string>
#include <GL/gl.h>

class Shader {
public:
    Shader(const char* vertexShaderPath, const char* fragmentShaderPath);
    ~Shader();

    void Load() const;
    void Use() const;

private:
    unsigned int id;

    const char* vertexShaderPath;
    const char* fragmentShaderPath;

    const int errorMessageSize = 512;

    static std::string ReadShader(const char* path);

    void CheckShaderError(unsigned int shaderId) const;
    void CheckShaderProgramError() const;
};


#endif