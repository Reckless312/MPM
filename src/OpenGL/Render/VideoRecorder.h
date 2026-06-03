#ifndef MPM_METHOD_VIDEO_RECORDER_H
#define MPM_METHOD_VIDEO_RECORDER_H

#include <cstdio>
#include <glad/glad.h>

class VideoRecorder
{
public:
    VideoRecorder(int width, int height);
    ~VideoRecorder();

    [[nodiscard]] bool IsDone() const;

    void EndFrame();
    void BeginFrame() const;
    void Open(const char* outputPath);

private:
    GLuint fbo{};
    GLuint pbos[2]{};
    GLuint fboDepth{};
    GLuint fboTexture{};

    FILE* ffmpegPipe{};

    int width{};
    int height{};
    int pboIndex{};
    int frameSize{};
    int frameCount{};
    int totalFrames{};
};

#endif
