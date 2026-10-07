#include "CampScene.h"
#include "SceneUtils.h"
#include "Mesh.h"
#include "Shader.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// เก็บของฉากนี้ไว้เป็น static (เฉพาะไฟล์นี้)
static Mesh*  g_RV    = nullptr;
static GLuint g_RVTex = 0;

// Person A: load models and textures here
void CreateCampScene()
{
    // โหลด OBJ (ถ้าหาไม่เจอจะได้ box แทน ไม่ crash)
    g_RV = LoadModelOrBox("rv3.obj");

    // โหลด texture (รับแค่ชื่อไฟล์)
    g_RVTex = LoadTexture("rv3.png");
}

// Person A: set dirLight and pointLights[1] (campfire), then draw objects here
void RenderCampScene(Shader* shader, GLuint uniformModel)
{
    if (!g_RV || g_RVTex == 0) return;

    // ตั้ง material (specular, shininess, emissive)
    SetMaterial(shader, 0.5f, 32.0f, 0.0f);

    // ผูก texture ที่ unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_RVTex);

    // model matrix
    glm::mat4 model = glm::mat4(1.0f);
    // model = glm::translate(model, glm::vec3(0.0f, 0.0f, -3.0f));
	// model = glm::translate(model, glm::vec3(0.0f, 0.0f, -2.5f));   // ขยับไปข้างหน้า 2.5 หน่วย
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0,1,0)); // ถ้าอยากหมุนด้านข้าง
    // model = glm::scale(model, glm::vec3(0.5f));
    glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));

    g_RV->RenderMesh();
}