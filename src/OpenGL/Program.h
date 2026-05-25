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

    float deltaTime = 0.0f;

    inline static int currentWidth = 800;
    inline static int currentHeight = 600;

    inline static bool recordingMode = false;
    inline static int recordingFrameRate = 144;
    inline static int recordingDurationSeconds = 5;
    inline static const char* recordingOutputPathScene1 = "/home/cora/Videos/logo_scene.mp4";
    inline static const char* recordingOutputPathScene2 = "/home/cora/Videos/sled_scene.mp4";

    explicit Program();
    ~Program();

    void SwitchPause();
    void ProcessInput() const;
    void UpdateDeltaTime();
    void LockCursor() const;
    void CreateWindowAndAssignContext();
    void SetViewportAndResizeCallback() const;

    [[nodiscard]] bool IsKeyJustPressed(int key);
    [[nodiscard]] bool IsPaused() const;

    static void InitializeGLFW();
    static void LoadGladLibrary();
    static void ResizeWindow(GLFWwindow* window, int width, int height);

    static int ReportErrorAndTerminate(const MPMException& exception);
private:
    std::map<int, bool> previousKeyStates;

    float lastFrame = 0.0f;

    bool paused = false;

    static constexpr int majorVersion = 3;
    static constexpr int minorVersion = 3;
    static constexpr int profile = GLFW_OPENGL_CORE_PROFILE;
    static constexpr int glfwPlatform = GLFW_PLATFORM_X11;

    static constexpr GLFWwindow* windowToShareResources = nullptr;

    static constexpr GLint viewportBottomLeftX = 0;
    static constexpr GLint viewportBottomLeftY = 0;

    inline static const char* windowTitle = "MPM Snow Simulation";
};


#endif