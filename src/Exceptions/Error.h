#ifndef MPM_METHOD_ERRORS_H
#define MPM_METHOD_ERRORS_H

enum class Error
{
    GLFWInitialization,
    GLFWCreateWindow,
    GladLoadLibrary,
    LockCursor,
    ShaderFileRead,
    ShaderCompile,
    ShaderLink,
    TextureLoad,
    GLFWLoadUserPointer,
};

#endif