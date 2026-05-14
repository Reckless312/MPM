#ifndef MPM_METHOD_FULLSCREEN_QUAD_H
#define MPM_METHOD_FULLSCREEN_QUAD_H

#include <glad/glad.h>

class FullscreenQuad
{
public:
    FullscreenQuad();
    ~FullscreenQuad();

    void Draw() const;

private:
    GLuint vao{};
    GLuint vbo{};

    void Setup();
};

#endif
