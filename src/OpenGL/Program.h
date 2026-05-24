#ifndef MPM_METHOD_PROGRAM_H
#define MPM_METHOD_PROGRAM_H

#include <map>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Exceptions/MPMException.h"

class Program
{
public:
    GLFWwindow* window;

    /* Time between frames */
    float deltaTime = 0.0f;

    bool paused = false;

    inline static int currentWidth = 800;
    inline static int currentHeight = 600;

    /* Recording Settings */
    inline static bool recordingMode = false;
    inline static int recordingFrameRate = 144;
    inline static int recordingDurationSeconds = 5;
    inline static const char* firstRecordingOutputPathScene = "/home/cora/Videos/ubb_logo.mp4";
    inline static const char* secondRecordingOutputPathScene = "/home/cora/Videos/sled.mp4";

    /* Particle Rendering */
    inline static float particleShellRadius = 0.018f;
    inline static float shellInnerFraction = 0.85f;
    /* ---- */

    explicit Program();
    ~Program();

    void SwitchPause();
    void UpdateDeltaTime();
    void LockCursor() const;
    void ProcessInput() const;
    void CreateWindowAndAssignContext();
    void SetViewportAndResizeCallback() const;

    [[nodiscard]] bool IsKeyJustPressed(int key);

    static void InitializeGLFW();
    static void LoadGladLibrary();
    static void ResizeWindow(GLFWwindow* window, int width, int height);

    static int ReportErrorAndTerminate(const MPMException& exception);
private:
    /* OpenGL Settings */
    static constexpr int majorVersion = 3;
    static constexpr int minorVersion = 3;
    static constexpr int profile = GLFW_OPENGL_CORE_PROFILE;
    static constexpr int glfwPlatform = GLFW_PLATFORM_X11;

    static constexpr GLFWwindow* windowToShareResources = nullptr;

    static constexpr GLint viewportBottomLeftX = 0;
    static constexpr GLint viewportBottomLeftY = 0;

    inline static const char* windowTitle = "MPM Snow Simulation";
    /* ---- */

    std::map<int, bool> previousKeyStates;

    float lastFrame = 0.0f;
};


#endif