#include "OpenGL/Render/DepthFBO.h"

DepthFBO::~DepthFBO()
{
    Destroy();
}

void DepthFBO::Create(int width, int height)
{
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &depthTexture);
    glGenRenderbuffers(1, &depthRenderbuffer);

    Allocate(width, height);
}

void DepthFBO::Resize(int width, int height)
{
    Destroy();
    Create(width, height);
}

void DepthFBO::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void DepthFBO::Unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint DepthFBO::GetDepthTexture() const
{
    return depthTexture;
}

void DepthFBO::Destroy()
{
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &depthTexture);
    glDeleteRenderbuffers(1, &depthRenderbuffer);
}

void DepthFBO::Allocate(int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, depthTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
