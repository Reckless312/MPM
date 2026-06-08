#ifndef MPM_METHOD_TEXTURELOADER_H
#define MPM_METHOD_TEXTURELOADER_H

#include <string>

#include "Exceptions/MPMException.h"
#include "glad/glad.h"
#include "stb image/stb_image.h"

namespace TextureLoader
{
    constexpr int formatRGBCode = 3;
    constexpr int formatRGBACode = 4;

    inline GLuint Load(const std::string& path, const std::string& directory)
    {
        int width, height, nrChannels;
        const std::string fullPath = directory + "/" + path;
        unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &nrChannels, 0);

        if (data == nullptr)
        {
            throw MPMException("Failed to load texture", Error::TextureLoad);
        }

        GLint internalFormat = GL_RED;
        GLenum format = GL_RED;

        if (nrChannels == formatRGBCode)
        {
            internalFormat = GL_RGB;
            format = GL_RGB;
        }
        else if (nrChannels == formatRGBACode)
        {
            internalFormat = GL_RGBA;
            format = GL_RGBA;
        }

        GLuint textureId;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);

        return textureId;
    }

    inline GLuint LoadArray(const char* path, const int layerCount)
    {
        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(false);

        unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

        if (data == nullptr)
        {
            throw MPMException("Failed to load texture array", Error::TextureLoad);
        }

        GLint internalFormat = GL_RGB8;
        GLenum format = GL_RGB;

        if (nrChannels == formatRGBACode)
        {
            internalFormat = GL_RGBA8;
            format = GL_RGBA;
        }

        const int layerHeight = height / layerCount;

        GLuint textureId;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textureId);

        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, width, layerHeight, layerCount, 0, format, GL_UNSIGNED_BYTE, data);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

        stbi_image_free(data);

        return textureId;
    }
}

#endif