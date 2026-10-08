#undef GLFW_DLL

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <vector>
#include <iostream>

#include "Libs/Mesh.h"
#include "Libs/Shader.h"
#include "Libs/Window.h"
#include "ProjectPaths.h"
#include "SceneUtils.h"
#include "CampScene.h"
#include "DeskScene.h"

const GLint WIDTH  = 800;
const GLint HEIGHT = 600;

const int NUM_POINT_LIGHTS = 2;

std::vector<Mesh*>   meshList;
std::vector<Shader*> shaderList;
std::vector<GLuint>  textureList;

const std::filesystem::path shaderDirectory =
    std::filesystem::u8path(OPENGL_STARTER_SHADER_DIR);

glm::vec3 lightColour = glm::vec3(1.0f, 1.0f, 1.0f);

// ------------------------------------------------------------
// Camera state
// ------------------------------------------------------------
glm::vec3 cameraPos   = glm::vec3(10.0f, 2.1f, -7.4f);
float     yaw         = 178.0f;
float     pitch       =  1.0f;
float     fov         = 60.0f;
bool      lookingBack = false;   // กด B toggle

// Mouse
double lastX = WIDTH / 2.0;
double lastY = HEIGHT / 2.0;
const float MOUSE_SENSITIVITY = 0.15f;

// Movement
const float MOVE_SPEED = 3.0f;

// ------------------------------------------------------------
// Callbacks
// ------------------------------------------------------------
void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    // กด B คือ หันหลัง / กลับหน้าปกติ
    if (key == GLFW_KEY_B && action == GLFW_PRESS)
        lookingBack = !lookingBack;
}

void MouseCallback(GLFWwindow*, double xpos, double ypos)
{
    float xoffset = (float)(xpos - lastX) * MOUSE_SENSITIVITY;
    float yoffset = (float)(lastY - ypos) * MOUSE_SENSITIVITY;
    lastX = xpos;
    lastY = ypos;

    yaw   += xoffset;
    pitch += yoffset;

    if (pitch >  89.0f) pitch =  89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
}

void ScrollCallback(GLFWwindow*, double, double yoffset)
{
    fov -= (float)yoffset * 2.0f;
    if (fov <  15.0f) fov =  15.0f;
    if (fov > 120.0f) fov = 120.0f;
}

// ------------------------------------------------------------
// Shaders
// ------------------------------------------------------------
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
    for (Mesh* mesh : meshList)        delete mesh;
    meshList.clear();

    for (Shader* shader : shaderList)  delete shader;
    shaderList.clear();

    glDeleteTextures((GLsizei)textureList.size(), textureList.data());
    textureList.clear();
}

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
int main()
{
    Window mainWindow(WIDTH, HEIGHT, 3, 3);
    if (mainWindow.initialise() != 0)
        return 1;

    GLFWwindow* win = mainWindow.getWindow();

    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetKeyCallback      (win, KeyCallback);
    glfwSetCursorPosCallback(win, MouseCallback);
    glfwSetScrollCallback   (win, ScrollCallback);

    CreateShaders();
    CreateCampScene();
    CreateDeskScene();

    const glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float lastTime = (float)glfwGetTime();

    while (!mainWindow.getShouldClose())
    {
        float currentTime = (float)glfwGetTime();
        float deltaTime   = currentTime - lastTime;
        lastTime = currentTime;

        glfwPollEvents();

        // ---------- ทิศทางกล้อง (รองรับ B หันหลัง) ----------
        float displayYaw = lookingBack ? (yaw + 180.0f) : yaw;

        glm::vec3 cameraDirection;
        cameraDirection.x = cos(glm::radians(displayYaw)) * cos(glm::radians(pitch));
        cameraDirection.y = sin(glm::radians(pitch));
        cameraDirection.z = sin(glm::radians(displayYaw)) * cos(glm::radians(pitch));
        cameraDirection = glm::normalize(cameraDirection);

        glm::vec3 cameraRight = glm::normalize(glm::cross(cameraDirection, worldUp));

        // ---------- WASD + Space/Shift ----------
        glm::vec3 flatForward = glm::normalize(
            glm::vec3(cameraDirection.x, 0.0f, cameraDirection.z));

        if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += flatForward * MOVE_SPEED * deltaTime;
        if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos -= flatForward * MOVE_SPEED * deltaTime;
        if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos -= cameraRight * MOVE_SPEED * deltaTime;
        if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += cameraRight * MOVE_SPEED * deltaTime;

        if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS)
            cameraPos.y += MOVE_SPEED * deltaTime;
        if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            cameraPos.y -= MOVE_SPEED * deltaTime;

        // ---------- Clear ----------
        // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClearColor(1.0f, 0.0f, 1.0f, 1.0f); // pink
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ---------- Shader + uniforms ----------
        Shader* shader = shaderList[0];
        shader->UseShader();

        // Directional light
        glUniform3fv(shader->GetUniformLocation("dirLight.direction"), 1,
                     glm::value_ptr(glm::vec3(-0.5f, -1.0f, -0.3f)));
        glUniform3fv(shader->GetUniformLocation("dirLight.colour"), 1,
                     glm::value_ptr(glm::vec3(1.0f, 0.98f, 0.95f)));
        glUniform1f (shader->GetUniformLocation("dirLight.intensity"), 0.8f);

        // Material
        glUniform1f(shader->GetUniformLocation("material.specularStrength"), 0.3f);
        glUniform1f(shader->GetUniformLocation("material.shininess"), 32.0f);

        // Emissive
        glUniform1f(shader->GetUniformLocation("emissiveStrength"), 0.0f);

        GLuint uniformModel      = shader->GetUniformLocation("model");
        GLuint uniformView       = shader->GetUniformLocation("view");
        GLuint uniformProjection = shader->GetUniformLocation("projection");

        glm::mat4 projection = glm::perspective(
            glm::radians(fov),
            (GLfloat)mainWindow.getBufferWidth() / (GLfloat)mainWindow.getBufferHeight(),
            0.1f, 200.0f);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraDirection, worldUp);

        glUniformMatrix4fv(uniformView,       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));

        // light
        glUniform3fv(shader->GetUniformLocation("lightColour"), 1, (GLfloat*)&lightColour);
        glUniform3fv(shader->GetUniformLocation("viewPos"),     1, (GLfloat*)&cameraPos);
        glUniform1i (shader->GetUniformLocation("numPointLights"), NUM_POINT_LIGHTS);

        // texture
        glUniform1i(shader->GetUniformLocation("texture2D"), 0);
        glActiveTexture(GL_TEXTURE0);

        // Objects
        RenderCampScene(shader, uniformModel);
        RenderDeskScene(shader, uniformModel);

        glUseProgram(0);
		static float logTimer = 0.0f;
		logTimer += deltaTime;
		if (logTimer >= 0.5f)
		{
			logTimer = 0.0f;
			std::cout << "Camera: ("
					<< cameraPos.x << ", "
					<< cameraPos.y << ", "
					<< cameraPos.z << ")"
					<< " | yaw=" << yaw
					<< " pitch=" << pitch
					<< std::endl;
		}
        mainWindow.swapBuffers();
    }

    Cleanup();
    return 0;
}