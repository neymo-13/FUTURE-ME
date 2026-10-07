#undef GLFW_DLL

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <vector>

#include "Libs/Mesh.h"
#include "Libs/Shader.h"
#include "Libs/Window.h"
#include "ProjectPaths.h"
#include "SceneUtils.h"
#include "CampScene.h"
#include "DeskScene.h"

const GLint WIDTH = 800;
const GLint HEIGHT = 600;

// pointLights[0] = laptop screen (B), pointLights[1] = campfire (A)
const int NUM_POINT_LIGHTS = 2;

std::vector<Mesh*> meshList;
std::vector<Shader*> shaderList;
std::vector<GLuint> textureList;

const std::filesystem::path shaderDirectory =
    std::filesystem::u8path(OPENGL_STARTER_SHADER_DIR);

// Ambient colour
glm::vec3 lightColour = glm::vec3(1.0f, 1.0f, 1.0f);

void CreateShaders()
{
    Shader* shader1 = new Shader();
    shader1->CreateFromFiles(
        shaderDirectory / "shader.vert",
        shaderDirectory / "shader.frag"
    );
    shaderList.push_back(shader1);
}

void Cleanup()
{
    for (Mesh* mesh : meshList)
        delete mesh;
    meshList.clear();

    for (Shader* shader : shaderList)
        delete shader;
    shaderList.clear();

    glDeleteTextures((GLsizei)textureList.size(), textureList.data());
    textureList.clear();
}

int main()
{
    Window mainWindow(WIDTH, HEIGHT, 3, 3);
    if (mainWindow.initialise() != 0)
        return 1;

    CreateShaders();
    CreateCampScene();
    CreateDeskScene();

    glm::mat4 projection = glm::perspective(glm::radians(60.0f),
        (GLfloat)mainWindow.getBufferWidth() / (GLfloat)mainWindow.getBufferHeight(), 0.1f, 200.0f);

    glm::vec3 cameraPos = glm::vec3(0.0f, 1.22f, 0.85f);
    glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    while (!mainWindow.getShouldClose())
    {
        glfwPollEvents();

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Shader* shader = shaderList[0];
        shader->UseShader();
        GLuint uniformModel = shader->GetUniformLocation("model");
        GLuint uniformView = shader->GetUniformLocation("view");
        GLuint uniformProjection = shader->GetUniformLocation("projection");

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraDirection, up);
        glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));

        // light
        glUniform3fv(shader->GetUniformLocation("lightColour"), 1, (GLfloat*)&lightColour);
        glUniform3fv(shader->GetUniformLocation("viewPos"), 1, (GLfloat*)&cameraPos);
        glUniform1i(shader->GetUniformLocation("numPointLights"), NUM_POINT_LIGHTS);

        // texture
        glUniform1i(shader->GetUniformLocation("texture2D"), 0);
        glActiveTexture(GL_TEXTURE0);

        //Object
        RenderCampScene(shader, uniformModel);
        RenderDeskScene(shader, uniformModel);

        glUseProgram(0);

        mainWindow.swapBuffers();
    }

    // Delete OpenGL objects before the Window destructor destroys the context.
    Cleanup();
    return 0;
}
