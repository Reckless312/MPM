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

    const Shader regularCubeShader("vertexShader.vs", "fragmentShader.fs");

    try
    {
        regularCubeShader.Load();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    const Shader lightCubeShader("vertexShader.vs", "lightFragmentShader.fs");

    try
    {
        lightCubeShader.Load();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    // ------------------------- Temp Code Start ------------------------- //

    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
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

    unsigned int VAO, VBO, lightVAO; //, EBO;

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

    glGenVertexArrays(bufferObjectsCount, &lightVAO);
    glBindVertexArray(lightVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), static_cast<void *>(nullptr));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glm::vec3 pointLightPositions[] = {
        glm::vec3( 0.7f,  0.2f,  2.0f),
        glm::vec3( 2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3( 0.0f,  0.0f, -3.0f)
    };

    glm::mat4 pointLightsModels[] = {
        glm::mat4(1.0f),
        glm::mat4(1.0f),
        glm::mat4(1.0f),
        glm::mat4(1.0f),
    };

    for (int i = 0; i < 4; i++)
    {
        pointLightsModels[i] = glm::translate(pointLightsModels[i], pointLightPositions[i]);
        pointLightsModels[i] = glm::scale(pointLightsModels[i], glm::vec3(0.2f));
    }

    // ------------------------- Temp Code End ------------------------- //

    TextureLoader textureLoader("container2.png");

    try
    {
        textureLoader.Load();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    TextureLoader textureLoader2("container2_specular.png");

    try
    {
        textureLoader2.Load();
    }
    catch (const MPMException& exception)
    {
        return Program::ReportErrorAndTerminate(exception);
    }

    TextureLoader textureLoader3("green-lines.png");

    try
    {
        textureLoader3.Load();
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

        regularCubeShader.Use();

        regularCubeShader.SetVec3("objectColor", glm::vec3(1.0f, 0.5f, 0.32f));
        regularCubeShader.SetVec3("viewPosition", camera.position);

        regularCubeShader.SetVec3("directionalLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
        regularCubeShader.SetVec3("directionalLight.ambient",  glm::vec3(0.05f, 0.05f, 0.05f));
        regularCubeShader.SetVec3("directionalLight.diffuse",  glm::vec3(0.4f, 0.4f, 0.4f));
        regularCubeShader.SetVec3("directionalLight.specular", glm::vec3(0.5f, 0.5f, 0.5f));

        for (int i = 0; i < 4; i++)
        {
            std::string name = "pointLights[" + std::to_string(i) + "]";
            regularCubeShader.SetVec3(name + ".position", pointLightPositions[i]);
            regularCubeShader.SetVec3(name + ".ambient",  glm::vec3(0.05f, 0.05f, 0.05f));
            regularCubeShader.SetVec3(name + ".diffuse",  glm::vec3(0.8f, 0.8f, 0.8f));
            regularCubeShader.SetVec3(name + ".specular", glm::vec3(1.0f, 1.0f, 1.0f));
            regularCubeShader.SetFloat(name + ".constant",  1.0f);
            regularCubeShader.SetFloat(name + ".linear",    0.09f);
            regularCubeShader.SetFloat(name + ".quadratic", 0.032f);
        }

        regularCubeShader.SetInt("material.diffuse", 0);
        regularCubeShader.SetInt("material.specular", 1);
        regularCubeShader.SetInt("material.emission", 2);

        regularCubeShader.SetFloat("material.shininess", 32.0f);

        regularCubeShader.SetFloat("spotLight.constant",  1.0f);
        regularCubeShader.SetFloat("spotLight.linear",    0.09f);
        regularCubeShader.SetFloat("spotLight.quadratic", 0.032f);
        regularCubeShader.SetFloat("spotLight.cutOff",   glm::cos(glm::radians(12.5f)));
        regularCubeShader.SetFloat("spotLight.outerCutOff", glm::cos(glm::radians(17.5f)));

        regularCubeShader.SetVec3("spotLight.ambient",  glm::vec3(0.2f, 0.2f, 0.2f));
        regularCubeShader.SetVec3("spotLight.diffuse",  glm::vec3(0.5f, 0.5f, 0.5f));
        regularCubeShader.SetVec3("spotLight.specular", glm::vec3(1.0f, 1.0f, 1.0f));
        regularCubeShader.SetVec3("spotLight.position",  camera.position);
        regularCubeShader.SetVec3("spotLight.direction", camera.front);

        regularCubeShader.SetMat4("view", camera.viewMatrix);
        regularCubeShader.SetMat4("projection", camera.projectionMatrix);

        textureLoader.Bind(GL_TEXTURE0);
        textureLoader2.Bind(GL_TEXTURE1);
        textureLoader3.Bind(GL_TEXTURE2);

        glBindVertexArray(VAO);
        for(unsigned int i = 0; i < 10; i++)
        {
            const float angle = 20.0f * i;

            auto model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            model = glm::rotate(model, glm::radians(static_cast<float>(glfwGetTime()) * 30.0f), glm::vec3(1.0f, 0.3f, 0.5f));

            auto normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));

            regularCubeShader.SetMat4("model", model);
            regularCubeShader.SetMat3("normalMatrix", normalMatrix);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glBindVertexArray(0);

        lightCubeShader.Use();

        glBindVertexArray(lightVAO);

        lightCubeShader.SetMat4("view", camera.viewMatrix);
        lightCubeShader.SetMat4("projection", camera.projectionMatrix);

        for (const auto & pointLightsModel : pointLightsModels)
        {
            lightCubeShader.SetMat4("model", pointLightsModel);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(program.window);
        glfwPollEvents();
    }

    return 0;
}
