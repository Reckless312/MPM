#ifndef MPM_METHOD_PROGRAM_H
#define MPM_METHOD_PROGRAM_H
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Camera;

class Program {
public:
    explicit Program(Camera &camera);

    GLFWwindow* window;

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    void CreateWindowAndAssignContext();
    void SetViewportAndCallbacks() const;
    void ProcessInput() const;
    void UpdateDeltaTime();
    void LockCursor() const;
    void UpdateMousePosition(float currentXDirection, float currentYDirection);

    static void MouseCallback(GLFWwindow* window, double xPosition, double yPosition);
    static void SetDefaultBackgroundToPurple();
    static void InitializeGLFW();
    static void LoadGladLibrary();
    static void ResizeWindow(GLFWwindow* window, int width, int height);

    static int ReportErrorAndTerminate(const char* errorMessage);

    static constexpr GLsizei windowWidth = 800;
    static constexpr GLsizei windowHeight = 600;
private:
    Camera* camera;

    static constexpr int majorVersion = 3;
    static constexpr int minorVersion = 3;

    static constexpr int profile = GLFW_OPENGL_CORE_PROFILE;

    inline static const char* windowTitle = "MPM Snow Simulation";

    static constexpr GLFWmonitor* fullscreenMonitor = nullptr;
    static constexpr GLFWwindow* windowToShareResources = nullptr;

    static constexpr int failureCode = 1;

    static constexpr GLint viewportBottomLeftX = 0;
    static constexpr GLint viewportBottomLeftY = 0;

    bool firstMouse = true;
};


#endif