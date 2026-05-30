#ifndef MPM_METHOD_ERRORS_H
#define MPM_METHOD_ERRORS_H

enum class Error
{
    GLFWInitialization = 1,
    GLFWCreateWindow = 2,
    GladLoadLibrary = 3,
    LockCursor = 4,
    ShaderFileRead = 5,
    ShaderCompile = 6,
    ShaderLink = 7,
    TextureLoad = 8,
    GLFWLoadUserPointer = 9,
    ModelLoad = 10,
    PositionMemoryAllocation = 11,
    BlocksMemoryAllocation = 12,
    ModelMemoryAllocation = 13,
    WrongPlatform = 14,
    RecordingPipeFailure = 15,
};

#endif