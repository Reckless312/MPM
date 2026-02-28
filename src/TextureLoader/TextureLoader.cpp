#include "TextureLoader.h"

#include "Exceptions/MPMException.h"
#include "glad/glad.h"
#include "stb image/stb_image.h"

TextureLoader::TextureLoader(const char *path) : id(0), width(0), height(0), nrChannels(0)
{
    stbi_set_flip_vertically_on_load(true);

    this->path = std::string(ASSETS_PATH) + path;
}

void TextureLoader::Load()
{
    constexpr int desiredChannels = 0;
    unsigned char *data = stbi_load(this->path.c_str(), &width, &height, &nrChannels, desiredChannels);

    if (data == nullptr)
    {
        throw MPMException("Failed to load texture", Error::TextureLoad);
    }

    GLint internalFormat = GL_RED;
    GLenum format = GL_RED;

    if (nrChannels == this->formatRGBCode)
    {
        internalFormat = GL_RGB;
        format = GL_RGB;
    }
    else if (nrChannels == this->formatRGBACode)
    {
        internalFormat = GL_RGBA;
        format = GL_RGBA;
    }

    glGenTextures(1, &this->id);

    glBindTexture(GL_TEXTURE_2D, this->id);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextureLoader::Bind(const GLenum textureUnit) const
{
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, this->id);
}
