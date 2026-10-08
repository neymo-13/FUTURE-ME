#include "CampScene.h"
#include "SceneUtils.h"
#include "Libs/Mesh.h"
#include "Libs/Shader.h"

#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ------------------------------------------------------------
// Objects + Textures
// ------------------------------------------------------------
static Mesh*  g_RV    = nullptr;
static GLuint g_RVTex = 0;

static Mesh*  g_Chair         = nullptr;
static GLuint g_ChairAlbedo   = 0;
static GLuint g_ChairMetallic = 0;
static GLuint g_ChairRough    = 0;
static GLuint g_ChairNormal   = 0;

static Mesh*  g_Fire    = nullptr;
static GLuint g_FireTex = 0;

// --- ภูเขา 2 ชิ้น ---
static Mesh*  g_MountainRock = nullptr;
static Mesh*  g_MountainSnow = nullptr;
static GLuint g_MountainRockColor = 0;
static GLuint g_MountainSnowColor = 0;

// --- พื้นหิมะ ---
static Mesh*  g_SnowGround  = nullptr;
static GLuint g_SnowDiffuse = 0;
static GLuint g_SnowRough   = 0;
static GLuint g_SnowNormal  = 0;

// ------------------------------------------------------------
// Default 1x1 textures
// ------------------------------------------------------------
static GLuint g_DefaultMetal = 0;
static GLuint g_DefaultRough = 0;
static GLuint g_DefaultNorm  = 0;

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
    // --- Default 1x1 ---
    g_DefaultMetal = Make1x1Texture(  0,   0,   0);
    g_DefaultRough = Make1x1Texture(128, 128, 128);
    g_DefaultNorm  = Make1x1Texture(128, 128, 255);

    // --- สีภูเขา ---
    g_MountainRockColor = Make1x1Texture(32, 31, 29);
    g_MountainSnowColor = Make1x1Texture(240, 240, 245);

    // --- RV ---
    g_RV    = LoadModelOrBox("rv3.obj");
    g_RVTex = LoadTexture("rv3.png");

    // --- เก้าอี้ ---
    g_Chair         = LoadModelOrBox("chair.obj");
    g_ChairAlbedo   = LoadTexture("chair/chair_geo_chair_BaseColor.png");
    g_ChairMetallic = LoadTexture("chair/chair_geo_chair_Metallic.png");
    g_ChairRough    = LoadTexture("chair/chair_geo_chair_Roughness.png");
    g_ChairNormal   = LoadTexture("chair/chair_geo_chair_Normal.png");

    // --- กองไฟ ---
    g_Fire    = LoadModelOrBox("fire.obj");
    g_FireTex = LoadTexture("fire/gltf_embedded_0.png");

    // --- ภูเขา 2 ชิ้น ---
    g_MountainRock = LoadModelOrBox("mountain_rock.obj");
    g_MountainSnow = LoadModelOrBox("mountain_snow.obj");

    // --- พื้นหิมะ ---
    g_SnowGround  = CreateBox();
    g_SnowDiffuse = LoadTexture("snow/snow01_diffuse_4k.jpg");
    g_SnowRough   = LoadTexture("snow/snow01_roughness_4k.jpg");
    g_SnowNormal  = LoadTexture("snow/snow01_normal_4k.jpg");

    std::cout << "[Camp] RV    mesh=" << g_RV    << " tex=" << g_RVTex << "\n";
    std::cout << "[Camp] Chair albedo=" << g_ChairAlbedo
              << " metal="  << g_ChairMetallic
              << " rough="  << g_ChairRough
              << " normal=" << g_ChairNormal << "\n";
    std::cout << "[Camp] Mountain rock=" << g_MountainRock
              << " snow=" << g_MountainSnow << std::endl;
}

// ------------------------------------------------------------
// Render
// ------------------------------------------------------------
void RenderCampScene(Shader* shader, GLuint uniformModel)
{
    // ============================================================
    // พื้นหิมะ — วาดบนสุด (ครั้งเดียว ไม่ซ้ำในลูป)
    // ============================================================
    if (g_SnowGround)
    {
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, g_SnowDiffuse);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, g_DefaultMetal);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, g_SnowRough);
        glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, g_SnowNormal);

        glUniform1i(shader->GetUniformLocation("texture_albedo"),    0);
        glUniform1i(shader->GetUniformLocation("texture_metallic"),  1);
        glUniform1i(shader->GetUniformLocation("texture_roughness"), 2);
        glUniform1i(shader->GetUniformLocation("texture_normal"),    3);
        glUniform1f(shader->GetUniformLocation("material_ao"), 1.0f);
        glUniform1f(shader->GetUniformLocation("emissiveStrength"), 0.0f);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(200.0f, 0.1f, 200.0f));

        glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
        g_SnowGround->RenderMesh();
    }

    // ============================================================
    // เทือกเขา 4 ลูก
    // ============================================================
    struct MountainSpot { glm::vec3 pos; float scale; float rotY; };
    MountainSpot mountains[] = {
        { glm::vec3(-17.0f, 0.0f, -30.0f), 6.0f, 180.0f },
        { glm::vec3(-17.0f, 0.0f, -20.0f), 6.5f, 160.0f },
        { glm::vec3(-17.0f, 0.0f, -10.0f), 6.2f, 200.0f },
        { glm::vec3(-17.0f, 0.0f,   0.0f), 6.0f, 180.0f },
    };

    for (const auto& m : mountains)
    {
        // ---- หิน ----
        if (g_MountainRock)
        {
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, g_MountainRockColor);
            glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, g_DefaultMetal);
            glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, g_DefaultRough);
            glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, g_DefaultNorm);

            glUniform1i(shader->GetUniformLocation("texture_albedo"),    0);
            glUniform1i(shader->GetUniformLocation("texture_metallic"),  1);
            glUniform1i(shader->GetUniformLocation("texture_roughness"), 2);
            glUniform1i(shader->GetUniformLocation("texture_normal"),    3);
            glUniform1f(shader->GetUniformLocation("material_ao"), 1.0f);
            glUniform1f(shader->GetUniformLocation("emissiveStrength"), 0.0f);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, m.pos);
            model = glm::rotate(model, glm::radians(m.rotY), glm::vec3(0, 1, 0));
            model = glm::scale(model, glm::vec3(m.scale));

            glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
            g_MountainRock->RenderMesh();
        }

        // ---- หิมะ (ตำแหน่งเดียวกับหิน) ----
        if (g_MountainSnow)
        {
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, g_MountainSnowColor);
            glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, g_DefaultMetal);
            glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, g_DefaultRough);
            glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, g_DefaultNorm);

            glUniform1i(shader->GetUniformLocation("texture_albedo"),    0);
            glUniform1i(shader->GetUniformLocation("texture_metallic"),  1);
            glUniform1i(shader->GetUniformLocation("texture_roughness"), 2);
            glUniform1i(shader->GetUniformLocation("texture_normal"),    3);
            glUniform1f(shader->GetUniformLocation("material_ao"), 1.0f);
            glUniform1f(shader->GetUniformLocation("emissiveStrength"), 0.0f);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, m.pos);
            model = glm::rotate(model, glm::radians(m.rotY), glm::vec3(0, 1, 0));
            model = glm::scale(model, glm::vec3(m.scale));

            glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
            g_MountainSnow->RenderMesh();
        }
    }

    // ============ RV ============
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
        glUniform1f(shader->GetUniformLocation("emissiveStrength"), 0.0f);

        // ⭐ tint สีรถ — เปลี่ยนสีได้ตรงนี้
        glm::vec3 rvTint = glm::vec3(1.0f, 0.3f, 0.3f);   // ขาว = ไม่เปลี่ยนสี
        glUniform3fv(shader->GetUniformLocation("tintColor"), 1, glm::value_ptr(rvTint));

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -3.0f));
        glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));

        g_RV->RenderMesh();

        // reset tint
        glUniform3fv(shader->GetUniformLocation("tintColor"), 1,
                     glm::value_ptr(glm::vec3(1.0f)));
    }

    // ============ เก้าอี้ (2 ตัว) ============
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
        glUniform1f(shader->GetUniformLocation("emissiveStrength"), 0.0f);

        struct ChairSpot { glm::vec3 pos; float rotY; };
        ChairSpot chairs[] = {
            { glm::vec3( 3.0f, 0.0f,  -8.5f),  45.0f },
            { glm::vec3( 3.0f, 0.0f, -11.0f), -45.0f },
        };

        for (const auto& c : chairs)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, c.pos);
            model = glm::rotate(model, glm::radians(c.rotY), glm::vec3(0, 1, 0));
            model = glm::scale(model, glm::vec3(0.05f));

            glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
            g_Chair->RenderMesh();
        }
    }

    // ============ กองไฟ ============
    if (g_Fire)
    {
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, g_FireTex);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, g_DefaultMetal);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, g_DefaultRough);
        glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, g_DefaultNorm);

        glUniform1i(shader->GetUniformLocation("texture_albedo"),    0);
        glUniform1i(shader->GetUniformLocation("texture_metallic"),  1);
        glUniform1i(shader->GetUniformLocation("texture_roughness"), 2);
        glUniform1i(shader->GetUniformLocation("texture_normal"),    3);
        glUniform1f(shader->GetUniformLocation("material_ao"), 1.0f);

        glUniform1f(shader->GetUniformLocation("emissiveStrength"), 0.8f);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(4.5f, 0.0f, -9.8f));
        model = glm::scale(model, glm::vec3(0.01f));

        glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
        g_Fire->RenderMesh();

        glUniform1f(shader->GetUniformLocation("emissiveStrength"), 0.0f);
    }
}
