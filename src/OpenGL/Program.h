#ifndef MPM_METHOD_PROGRAM_H
#define MPM_METHOD_PROGRAM_H

#include <array>
#include <map>
#include <memory>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "CUDA/Simulation.h"
#include "Exceptions/MPMException.h"
#include "Render/Particle.h"
#include "Simulation/MeshBoundary.h"
#include "Simulation/SnowVolume.h"
#include "Render/VideoRecorder.h"

class Program
{
public:
    explicit Program();
    ~Program();

    GLFWwindow* window;

    inline static const char* logoSceneOutputPath = "/home/cora/Videos/logo_scene.mp4";
    inline static const char* sledSceneOutputPath = "/home/cora/Videos/sled_scene.mp4";
    inline static const char* snowfallSceneOutputPath = "/home/cora/Videos/snowfall_scene.mp4";

    inline static const std::array<const char*, 3> sceneOutputPaths = { logoSceneOutputPath, sledSceneOutputPath, snowfallSceneOutputPath };

    inline static bool recordingMode = false;

    inline static int currentWidth = 800;
    inline static int currentHeight = 600;
    inline static int recordingFrameRate = 144;
    inline static int recordingDurationSeconds = 10;

    float deltaTime = 0.0f;

    static int ReportErrorAndTerminate(const MPMException& exception);

    static void InitializeGLFW();
    static void LoadGladLibrary();
    static void SetFloorUniforms(const Shader& shader);
    static void ResizeWindow(GLFWwindow* window, int width, int height);

    [[nodiscard]] bool IsKeyJustPressed(int key);
    [[nodiscard]] bool IsPaused() const;
    [[nodiscard]] bool IsInfoVisible() const;

    [[nodiscard]] int GetActiveScene() const;
    [[nodiscard]] int GetRecordingScene() const;

    void UpdateDeltaTime();
    void LockCursor() const;
    void SetRecordingScene(int scene);
    void CreateWindowAndAssignContext();
    void SetViewportAndResizeCallback() const;
    void SetSceneUniforms(const Shader& shader) const;
    void ChangeScene(int scene, Simulation& simulation, const MeshSDF& boundarySDF, Particle& particles, const SnowVolume& snowVolume, int maxCapacity);
    void AdvanceRecordingScene(Simulation& simulation, Particle& particles, const std::array<const MeshSDF*, 3>& boundarySDFs, const std::array<const SnowVolume*, 3>& snowVolumes, const std::array<int, 3>& maxCapacities, std::unique_ptr<VideoRecorder>& recorder);
    void ProcessInput(Simulation& simulation, Particle& particles, const std::array<const MeshSDF*, 3>& boundarySDFs, const std::array<const SnowVolume*, 3>& snowVolumes, const std::array<int, 3>& maxCapacities);
private:
    static constexpr GLFWwindow* windowToShareResources = nullptr;

    static constexpr GLint viewportBottomLeftX = 0;
    static constexpr GLint viewportBottomLeftY = 0;

    inline static const char* windowTitle = "MPM Snow Simulation";

    std::map<int, bool> previousKeyStates;

    static constexpr int majorVersion = 3;
    static constexpr int minorVersion = 3;
    static constexpr int glfwPlatform = GLFW_PLATFORM_X11;
    static constexpr int profile = GLFW_OPENGL_CORE_PROFILE;

    bool paused = false;
    bool showInfo = true;

    float lastFrame = 0.0f;

    int activeScene = 1;
    int recordingScene = 1;
};


#endif