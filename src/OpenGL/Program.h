#ifndef MPM_METHOD_PROGRAM_H
#define MPM_METHOD_PROGRAM_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Exceptions/MPMException.h"

class Program
{
public:
    GLFWwindow* window;

    /* Time between frames */
    float deltaTime = 0.0f;

    /* Simulation Settings */
    inline static int currentWidth = 800;
    inline static int currentHeight = 600;

    inline static bool recordingMode = false;
    inline static int recordingFrameRate = 144;
    inline static int recordingDurationSeconds = 5;
    inline static const char* recordingOutputPathScene1 = "/home/cora/Videos/scene1.mp4";
    inline static const char* recordingOutputPathScene2 = "/home/cora/Videos/scene2.mp4";

    /* Particle Rendering */
    static constexpr float particleShellRadius = 0.018f;
    static constexpr float shellInnerFraction = 0.85f;
    /* ---- */

    explicit Program();
    ~Program();

    void SwitchPause();
    void ProcessInput();
    void UpdateKeyStates();
    void UpdateDeltaTime();
    void LockCursor() const;
    void UpdateFPSOnWindowTitle() const;
    void CreateWindowAndAssignContext();
    void SetViewportAndResizeCallback() const;

    [[nodiscard]] bool WasFirstSceneSelected() const;
    [[nodiscard]] bool WasSecondSceneSelected() const;
    [[nodiscard]] bool WasPauseKeyPressed() const;
    [[nodiscard]] bool IsPaused() const;

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

    float lastFrame = 0.0f;

    bool firstSceneKeyPressed = false;
    bool firstSceneKeyWasDown = false;
    bool secondSceneKeyPressed = false;
    bool secondSceneKeyWasDown = false;
    bool pauseKeyPressed = false;
    bool pauseKeyWasDown = false;
    bool paused = false;
};


#endif