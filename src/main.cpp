#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera/Camera.h"
#include "Program/Program.h"
#include "Exceptions/MPMException.h"
#include "Shaders/Shader.h"
#include "TextureLoader/TextureLoader.h"

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
        Program::ReportErrorAndTerminate(exception);
    }

    Program::SetDefaultBackgroundToPurple();
    program.SetViewportAndResizeCallback();

    Camera camera(program.window, Program::windowWidth, Program::windowHeight);
    camera.AssignUserPointerAndSetCallbacks();

    const Shader shaderProgram("vertexShader.vs", "fragmentShader.fs");

    try
    {
        shaderProgram.Load();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    // ------------------------- Temp Code Start ------------------------- //

    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  0.0f, 1.0f
    };

    glm::vec3 cubePositions[] = {
        glm::vec3( 0.0f,  0.0f,  0.0f),
        glm::vec3( 2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3( 2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3( 1.3f, -2.0f, -2.5f),
        glm::vec3( 1.5f,  2.0f, -2.5f),
        glm::vec3( 1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };

    // unsigned int indices[] = {
    //     0, 1, 3,
    //     1, 2, 3
    // };

    constexpr int bufferObjectsCount = 1;

    unsigned int VAO, VBO, EBO;

    glGenBuffers(bufferObjectsCount, &VBO);
    // glGenBuffers(bufferObjectsCount, &EBO);
    glGenVertexArrays(bufferObjectsCount, &VAO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), static_cast<void *>(nullptr));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    // ------------------------- Temp Code End ------------------------- //

    TextureLoader textureLoader("skong.jpeg");

    try
    {
        textureLoader.Load();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(program.window))
    {
        program.UpdateDeltaTime();
        camera.UpdateSpeed(program.deltaTime);

        program.ProcessInput();
        camera.ProcessInput();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderProgram.Use();

        shaderProgram.SetMat4("view", camera.viewMatrix);
        shaderProgram.SetMat4("projection", camera.projectionMatrix);

        textureLoader.Bind(GL_TEXTURE0);

        glBindVertexArray(VAO);
        for(unsigned int i = 0; i < 10; i++)
        {
            auto model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            const float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            model = glm::rotate(model, glm::radians(static_cast<float>(glfwGetTime()) * 30.0f), glm::vec3(1.0f, 0.3f, 0.5f));
            shaderProgram.SetMat4("model", model);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(program.window);
        glfwPollEvents();
    }

    return 0;
}
