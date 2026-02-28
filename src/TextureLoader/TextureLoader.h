#ifndef MPM_METHOD_TEXTURELOADER_H
#define MPM_METHOD_TEXTURELOADER_H

#include <string>

#include "glad/glad.h"

class TextureLoader
{
public:
    explicit TextureLoader(const char* path);

    void Load();
    void Bind(GLenum textureUnit) const;
private:
    GLuint id;

    std::string path;

    int width;
    int height;
    int nrChannels;

    const int formatRGBCode = 3;
    const int formatRGBACode = 4;
};


#endif