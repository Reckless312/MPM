#ifndef MPM_METHOD_RENDER_OBJECT_H
#define MPM_METHOD_RENDER_OBJECT_H

#include <glad/glad.h>
#include "OpenGL/Shaders/Shader.h"

class RenderObject {
public:
    RenderObject() = default;
    virtual ~RenderObject();

    RenderObject(RenderObject&& other) noexcept;
    RenderObject& operator=(RenderObject&& other) noexcept;

    virtual void Draw(const Shader &shader) const = 0;

    [[nodiscard]] unsigned int GetVBO() const;

protected:
    unsigned int VAO{}, VBO{};
};

#endif
