#include "CampScene.h"
#include "SceneUtils.h"
#include "Libs/Mesh.h"
#include "Libs/Shader.h"

#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ------------------------------------------------------------
// Objects + Textures (ของจริง)
// ------------------------------------------------------------
static Mesh*  g_RV    = nullptr;
static GLuint g_RVTex = 0;

static Mesh*  g_Chair         = nullptr;
static GLuint g_ChairAlbedo   = 0;
static GLuint g_ChairMetallic = 0;
static GLuint g_ChairRough    = 0;
static GLuint g_ChairNormal   = 0;

// ------------------------------------------------------------
// Default 1x1 textures (สำหรับ RV ที่ไม่ใช่ PBR)
// ------------------------------------------------------------
static GLuint g_DefaultMetal = 0;   // (0,0,0)   = ไม่โลหะ
static GLuint g_DefaultRough = 0;   // (128,...) = หยาบกลาง
static GLuint g_DefaultNorm  = 0;   // (128,128,255) = normal ราบ

// สร้าง texture 1x1
static GLuint Make1x1Texture(unsigned char r, unsigned char g, unsigned char b)
{
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);

    unsigned char data[3] = { r, g, b };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
    return t;
}

// ------------------------------------------------------------
// Create
// ------------------------------------------------------------
void CreateCampScene()
{
    // --- Default 1x1 textures ---
    g_DefaultMetal = Make1x1Texture(  0,   0,   0);
    g_DefaultRough = Make1x1Texture(128, 128, 128);
    g_DefaultNorm  = Make1x1Texture(128, 128, 255);

    // --- RV ---
    g_RV    = LoadModelOrBox("rv3.obj");
    g_RVTex = LoadTexture("rv3.png");

    // --- เก้าอี้ PBR 4 texture ---
    g_Chair = LoadModelOrBox("chair.obj");   // ← ชื่อไฟล์ obj ของเธอ

    g_ChairAlbedo   = LoadTexture("chair/chair_geo_chair_BaseColor.png");
    g_ChairMetallic = LoadTexture("chair/chair_geo_chair_Metallic.png");
    g_ChairRough    = LoadTexture("chair/chair_geo_chair_Roughness.png");
    g_ChairNormal   = LoadTexture("chair/chair_geo_chair_Normal.png");

    std::cout << "[Camp] RV    mesh=" << g_RV << " tex=" << g_RVTex << "\n";
    std::cout << "[Camp] Chair albedo=" << g_ChairAlbedo
              << " metal="  << g_ChairMetallic
              << " rough="  << g_ChairRough
              << " normal=" << g_ChairNormal << std::endl;
}

// ------------------------------------------------------------
// Render
// ------------------------------------------------------------
void RenderCampScene(Shader* shader, GLuint uniformModel)
{
    // ============================================================
    // RV — ใช้ PBR เหมือนกัน แต่ metallic/rough/normal เป็น default 1x1
    // ============================================================
    if (g_RV && g_RVTex)
    {
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, g_RVTex);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, g_DefaultMetal);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, g_DefaultRough);
        glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, g_DefaultNorm);

        glUniform1i(shader->GetUniformLocation("texture_albedo"),    0);
        glUniform1i(shader->GetUniformLocation("texture_metallic"),  1);
        glUniform1i(shader->GetUniformLocation("texture_roughness"), 2);
        glUniform1i(shader->GetUniformLocation("texture_normal"),    3);
        glUniform1f(shader->GetUniformLocation("material_ao"), 1.0f);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -3.0f));
        glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));

        g_RV->RenderMesh();
    }

    // ============================================================
    // เก้าอี้ — ตัวที่ 1 (ซ้าย)
    // ============================================================
    if (g_Chair)
    {
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, g_ChairAlbedo);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, g_ChairMetallic);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, g_ChairRough);
        glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, g_ChairNormal);

        glUniform1i(shader->GetUniformLocation("texture_albedo"),    0);
        glUniform1i(shader->GetUniformLocation("texture_metallic"),  1);
        glUniform1i(shader->GetUniformLocation("texture_roughness"), 2);
        glUniform1i(shader->GetUniformLocation("texture_normal"),    3);
        glUniform1f(shader->GetUniformLocation("material_ao"), 1.0f);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3( 3.0f, 0.0f, -8.5f));
        model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0, 1, 0));
        model = glm::scale(model, glm::vec3(0.05f));

        glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
        g_Chair->RenderMesh();
    }

    // ============================================================
    // เก้าอี้ — ตัวที่ 2 (ขวา)
    // ============================================================
    if (g_Chair)
    {
        // texture ยังผูกอยู่จากตัวแรก ไม่ต้อง bind ซ้ำ
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(3.0f, 0.0f, -11.0f));
        model = glm::rotate(model, glm::radians(-45.0f), glm::vec3(0, 1, 0));
        model = glm::scale(model, glm::vec3(0.05f));

        glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
        g_Chair->RenderMesh();
    }
}