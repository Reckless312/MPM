#include <cstdio>

#include "OpenGL/Render/VideoRecorder.h"
#include "OpenGL/Program.h"

VideoRecorder::VideoRecorder(const int width, const int height)
{
    this->totalFrames = Program::recordingFrameRate * Program::recordingDurationSeconds;

    this->width = width;
    this->height = height;
    this->frameSize = width * height * 4;

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

    for (const unsigned int pbo : this->pbos)
    {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
        glBufferData(GL_PIXEL_PACK_BUFFER, this->frameSize, nullptr, GL_STREAM_READ);
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

VideoRecorder::~VideoRecorder()
{
    if (this->ffmpegPipe != nullptr)
    {
        pclose(this->ffmpegPipe);
    }

    glDeleteFramebuffers(1, &this->fbo);
    glDeleteTextures(1, &this->fboTexture);
    glDeleteRenderbuffers(1, &this->fboDepth);

    glDeleteBuffers(2, this->pbos);
}

bool VideoRecorder::IsDone() const
{
    return this->frameCount >= this->totalFrames;
}

void VideoRecorder::EndFrame()
{
    glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pbos[this->pboIndex]);
    glReadPixels(0, 0, this->width, this->height, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    const int readIndex = (this->pboIndex + 1) % 2;
    glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pbos[readIndex]);

    if (const void* data = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY))
    {
        const size_t returnedCount = fwrite(data, 1, this->frameSize, this->ffmpegPipe);
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

        if (returnedCount < static_cast<size_t>(this->frameSize))
        {
            throw MPMException("Failed to write to recording pipe.", Error::RecordingPipeFailure);
        }
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    this->pboIndex = readIndex;
    this->frameCount++;

    printf("Frame %d / %d\n", this->frameCount, this->totalFrames);
}

void VideoRecorder::BeginFrame() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
}

void VideoRecorder::Open(const char* outputPath)
{
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "ffmpeg -y -f rawvideo -pixel_format rgba -video_size %dx%d -framerate %d -i pipe:0 -c:v libx264 -pix_fmt yuv420p -crf 18 -vf vflip %s", this->width, this->height, Program::recordingFrameRate, outputPath);

    this->ffmpegPipe = popen(cmd, "w");

    if (this->ffmpegPipe == nullptr)
    {
        throw MPMException("Failed to open a recording pipe.", Error::RecordingPipeFailure);
    }
}