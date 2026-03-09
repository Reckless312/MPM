#ifndef MPM_METHOD_PROGRAM_H
#define MPM_METHOD_PROGRAM_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Exceptions/MPMException.h"

/**
 * Responsible for managing libraries and the main window
 */
class Program
{
public:
    GLFWwindow* window;

    /**
     * Time between two frames
     */
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    static constexpr GLsizei windowWidth = 800;
    static constexpr GLsizei windowHeight = 600;

    explicit Program();
    ~Program();

    void CreateWindowAndAssignContext();
    void SetViewportAndResizeCallback() const;
    void LockCursor() const;

    void ProcessInput() const;
    void UpdateDeltaTime();

    static void InitializeGLFW();
    static void LoadGladLibrary();
    static void ResizeWindow(GLFWwindow* window, int width, int height);

    static int ReportErrorAndTerminate(const MPMException& exception);
private:
    static constexpr int majorVersion = 3;
    static constexpr int minorVersion = 3;
    static constexpr int profile = GLFW_OPENGL_CORE_PROFILE;

    static constexpr int failureCode = 1;

    inline static const char* windowTitle = "MPM Snow Simulation";

    static constexpr GLFWmonitor* fullscreenMonitor = nullptr;
    static constexpr GLFWwindow* windowToShareResources = nullptr;

    static constexpr GLint viewportBottomLeftX = 0;
    static constexpr GLint viewportBottomLeftY = 0;
};


#endif