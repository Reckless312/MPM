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

    static unsigned int StaticLoad(const std::string &path, const std::string &directory);
private:
    GLuint id;

    std::string path;

    int width;
    int height;
    int nrChannels;

    static constexpr int formatRGBCode = 3;
    static constexpr int formatRGBACode = 4;
};


#endif