#undef GLFW_DLL

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <vector>

#include "Libs/Mesh.h"
#include "Libs/Shader.h"
#include "Libs/Window.h"
#include "ProjectPaths.h"

const GLint WIDTH = 800;
const GLint HEIGHT = 600;

std::vector<Mesh*> meshList;
std::vector<Shader*> shaderList;

const std::filesystem::path shaderDirectory =
    std::filesystem::u8path(OPENGL_STARTER_SHADER_DIR);

void CreateTriangle()
{
    GLfloat vertices[] =
    {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         0.0f,  1.0f, 0.0f
    };

    unsigned int indices[] =
    {
        0, 1, 2
    };

    Mesh* obj1 = new Mesh();
    obj1->CreateMesh(vertices, indices, 9, 3);
    meshList.push_back(obj1);
}

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
}

int main()
{
    Window mainWindow(WIDTH, HEIGHT, 3, 3);
    if (mainWindow.initialise() != 0)
        return 1;

    CreateTriangle();
    CreateShaders();

    while (!mainWindow.getShouldClose())
    {
        glfwPollEvents();

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderList[0]->UseShader();
        meshList[0]->RenderMesh();
        glUseProgram(0);

        mainWindow.swapBuffers();
    }

    // Delete OpenGL objects before the Window destructor destroys the context.
    Cleanup();
    return 0;
}
