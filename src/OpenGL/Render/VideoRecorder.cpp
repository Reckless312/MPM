#include <cstdio>

#include "OpenGL/Render/VideoRecorder.h"
#include "OpenGL/Program.h"

VideoRecorder::VideoRecorder(const int width, const int height)
    : totalFrames(Program::recordingFrameRate * Program::recordingDurationSeconds),
      width(width),
      height(height)
{
    glGenFramebuffers(1, &this->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);

    glGenTextures(1, &this->fboTexture);
    glBindTexture(GL_TEXTURE_2D, this->fboTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->fboTexture, 0);

    glGenRenderbuffers(1, &this->fboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, this->fboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, this->fboDepth);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenBuffers(2, this->pbos);
    for (int i = 0; i < 2; i++)
    {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pbos[i]);
        glBufferData(GL_PIXEL_PACK_BUFFER, static_cast<GLsizeiptr>(width * height * 4), nullptr, GL_STREAM_READ);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "ffmpeg -y -f rawvideo -pixel_format rgba -video_size %dx%d -framerate %d -i pipe:0 -c:v libx264 -pix_fmt yuv420p -crf 18 -vf vflip %s",
        width, height, Program::recordingFrameRate, Program::recordingOutputPath);
    this->ffmpegPipe = popen(cmd, "w");
}

VideoRecorder::~VideoRecorder()
{
    pclose(this->ffmpegPipe);
    glDeleteFramebuffers(1, &this->fbo);
    glDeleteTextures(1, &this->fboTexture);
    glDeleteRenderbuffers(1, &this->fboDepth);
    glDeleteBuffers(2, this->pbos);
}

void VideoRecorder::BeginFrame() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
}

void VideoRecorder::EndFrame()
{
    glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pbos[this->pboIndex]);
    glReadPixels(0, 0, this->width, this->height, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    const int readIndex = 1 - this->pboIndex;
    glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pbos[readIndex]);
    void* data = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
    if (data)
    {
        fwrite(data, 1, static_cast<size_t>(this->width * this->height * 4), this->ffmpegPipe);
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    this->pboIndex = 1 - this->pboIndex;
    this->frameCount++;

    printf("Frame %d / %d\n", this->frameCount, this->totalFrames);
}

bool VideoRecorder::IsDone() const
{
    return this->frameCount >= this->totalFrames;
}
