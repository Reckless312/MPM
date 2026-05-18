#ifndef MPM_METHOD_VIDEO_RECORDER_H
#define MPM_METHOD_VIDEO_RECORDER_H

#include <cstdio>
#include <glad/glad.h>

class VideoRecorder
{
public:
    VideoRecorder(int width, int height);
    ~VideoRecorder();

    void BeginFrame() const;
    void EndFrame();
    [[nodiscard]] bool IsDone() const;

private:
    GLuint fbo{};
    GLuint fboTexture{};
    GLuint fboDepth{};
    GLuint pbos[2]{};
    FILE* ffmpegPipe{};
    int pboIndex{};
    int frameCount{};
    int totalFrames{};
    int width{};
    int height{};
};

#endif
