#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <random>

#include "OpenGL/Scene/Camera.h"
#include "OpenGL/Program.h"
#include "Exceptions/MPMException.h"
#include "OpenGL/Render/Model.h"
#include "OpenGL/Render/Particle.h"
#include "OpenGL/Shaders/Shader.h"
#include "OpenGL/Shaders/TextureLoader.h"

int main()
{
    Program program;

    try
    {
        Program::InitializeGLFW();
        program.CreateWindowAndAssignContext();

        Program::LoadGladLibrary();
        program.LockCursor();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    program.SetViewportAndResizeCallback();

    Camera camera(program.window, Program::windowWidth, Program::windowHeight);
    camera.AssignUserPointerAndSetCallbacks();

    Shader sceneShader("vertexShader.vs", "fragmentShader.fs");
    Shader particleShader("particleVertex.vs", "particleFragment.fs");

    try
    {
        sceneShader.Load();
        particleShader.Load();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    std::random_device randomDevice;
    std::mt19937 randomEngine(randomDevice());
    std::uniform_real_distribution distribution(-0.5f, 0.5f);

    std::vector<glm::vec3> snowballPositions;

    while (snowballPositions.size() < 300000)
    {
        if (glm::vec3 point(distribution(randomEngine), distribution(randomEngine), distribution(randomEngine)); glm::length(point) <= 0.5f)
        {
            snowballPositions.push_back(point);
        }
    }

    Particle snowball(snowballPositions);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    while (!glfwWindowShouldClose(program.window))
    {
        program.UpdateDeltaTime();
        camera.UpdateSpeed(program.deltaTime);

        program.ProcessInput();
        camera.ProcessInput();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        particleShader.Use();
        particleShader.SetMat4("view", camera.viewMatrix);
        particleShader.SetMat4("projection", camera.projectionMatrix);
        snowball.Draw(particleShader);

        glfwSwapBuffers(program.window);
        glfwPollEvents();
    }

    return 0;
}