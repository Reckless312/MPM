#ifndef MPM_METHOD_PROGRAM_H
#define MPM_METHOD_PROGRAM_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Exceptions/MPMException.h"

class DepthFBO;

struct SceneParameters
{
    float firstLameParameter;
    float secondLameParameter;
    float hardeningCoefficient;
    float criticalCompression;
    float criticalStretch;
};

class Program
{
public:
    GLFWwindow* window;

    /* Time between frames */
    float deltaTime = 0.0f;

    /* Simulation Settings */
    inline static int currentWidth = 800;
    inline static int currentHeight = 600;
    inline static DepthFBO* depthFBO = nullptr;

    inline static int maxBlocks = 16384;
    inline static int cellCountPerAxis = 256;
    inline static float cellSize = 0.02f;
    inline static float physicsTimeStep = 2e-4f;
    inline static float gravity = 9.8f;
    inline static float firstLameParameter = 1.333e4f;
    inline static float secondLameParameter = 2.0e4f;
    inline static float hardeningCoefficient = 10.0f;
    inline static float criticalCompression = 0.025f;
    inline static float criticalStretch = 0.0075f;
    inline static float boundaryFriction = 0.8f;
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

    static void ApplySceneParameters(const SceneParameters& sceneParameters);
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

    static constexpr GLFWmonitor* fullscreenMonitor = nullptr;
    static constexpr GLFWwindow* windowToShareResources = nullptr;

    static constexpr GLint viewportBottomLeftX = 0;
    static constexpr GLint viewportBottomLeftY = 0;

    static constexpr GLsizei windowWidth = 800;
    static constexpr GLsizei windowHeight = 600;

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