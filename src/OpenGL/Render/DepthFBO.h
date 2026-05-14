#ifndef MPM_METHOD_DEPTH_FBO_H
#define MPM_METHOD_DEPTH_FBO_H

#include <glad/glad.h>

class DepthFBO
{
public:
    DepthFBO() = default;
    ~DepthFBO();

    void Create(int width, int height);
    void Resize(int width, int height);

    void Bind() const;
    void Unbind() const;

    [[nodiscard]] GLuint GetDepthTexture() const;

private:
    GLuint fbo{};
    GLuint depthTexture{};
    GLuint depthRenderbuffer{};

    void Destroy();
    void Allocate(int width, int height);
};

#endif
