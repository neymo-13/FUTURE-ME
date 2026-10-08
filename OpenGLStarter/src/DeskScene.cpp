#include "DeskScene.h"

#include "SceneUtils.h"
#include "ProjectPaths.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>

// Scene faces -X (camera forward), right of the image is -Z. Units are metres, ground y = 0.
// Move DESK_POS to move the whole desk set; other positions are offsets from it.
const float TABLE_TOP_Y = 0.74f;
const glm::vec3 DESK_POS      = glm::vec3(4.6f, 0.0f, -6.9f);
const glm::vec3 TABLE_CENTER  = DESK_POS + glm::vec3( 0.0f,  TABLE_TOP_Y,  0.0f);
const glm::vec3 LAPTOP_POS    = DESK_POS + glm::vec3(-0.05f, TABLE_TOP_Y,  0.25f);
const glm::vec3 MUG_POS       = DESK_POS + glm::vec3( 0.0f,  TABLE_TOP_Y, -0.3f);
const glm::vec3 FLOWERPOT_POS = DESK_POS + glm::vec3(-0.35f, TABLE_TOP_Y,  0.6f);
const float LID_TILT_DEG = 12.0f;

static Mesh* g_Box = nullptr;

// nullptr = .obj missing, draw boxes instead
static Mesh* g_TableObj     = nullptr;
static Mesh* g_LaptopObj    = nullptr;
static Mesh* g_MugObj       = nullptr;
static Mesh* g_FlowerpotObj = nullptr;

static GLuint g_WoodTex  = 0;
static GLuint g_WhiteTex = 0;
static bool   g_HasWood  = false;

static bool HasFile(const char* dir, const char* name)
{
    return std::filesystem::exists(std::filesystem::u8path(dir) / name);
}

static Mesh* LoadIfExists(const char* name)
{
    return HasFile(OPENGL_STARTER_MODEL_DIR, name) ? LoadModelOrBox(name) : nullptr;
}

// 1x1 white texture, coloured per object with tintColor
static GLuint CreateWhiteTexture()
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    const unsigned char white[3] = { 255, 255, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    textureList.push_back(texture);
    return texture;
}

// Uniforms the main shader (Person A) needs for every object
static void SetSurface(Shader* shader, GLuint texture, glm::vec3 tint, float emissive)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(shader->GetUniformLocation("texture_albedo"), 0);
    glUniform1f(shader->GetUniformLocation("material_ao"), 1.0f);
    glUniform1f(shader->GetUniformLocation("emissiveStrength"), emissive);
    // tint only colours white/grey pixels, so a white texture becomes this colour
    glUniform3fv(shader->GetUniformLocation("tintColor"), 1, glm::value_ptr(tint));
}

static void Draw(Mesh* mesh, GLuint uniformModel, const glm::mat4& model)
{
    glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
    mesh->RenderMesh();
}

// Unit box scaled to size, centred at center
static void DrawBox(GLuint uniformModel, glm::mat4 base, glm::vec3 center, glm::vec3 size)
{
    glm::mat4 model = glm::translate(base, center);
    model = glm::scale(model, size);
    Draw(g_Box, uniformModel, model);
}

void CreateDeskScene()
{
    g_Box = CreateBox();

    g_TableObj     = LoadIfExists("table.obj");
    g_LaptopObj    = LoadIfExists("laptop.obj");
    g_MugObj       = LoadIfExists("mug.obj");
    g_FlowerpotObj = LoadIfExists("flowerpot.obj");

    g_HasWood  = HasFile(OPENGL_STARTER_TEXTURE_DIR, "wood.jpg");
    g_WoodTex  = LoadTexture("wood.jpg");
    g_WhiteTex = CreateWhiteTexture();
}

// Folding table: top board + 4 metal legs
static void RenderTable(Shader* shader, GLuint uniformModel)
{
    glm::mat4 base = glm::translate(glm::mat4(1.0f), TABLE_CENTER);

    if (g_TableObj)
    {
        SetSurface(shader, g_WoodTex, glm::vec3(1.0f), 0.0f);
        Draw(g_TableObj, uniformModel, glm::translate(glm::mat4(1.0f), DESK_POS));
        return;
    }

    glm::vec3 woodTint = g_HasWood ? glm::vec3(1.0f) : glm::vec3(0.55f, 0.38f, 0.22f);
    SetSurface(shader, g_WoodTex, woodTint, 0.0f);
    DrawBox(uniformModel, base, glm::vec3(0.0f, -0.02f, 0.0f), glm::vec3(1.0f, 0.04f, 1.7f));

    SetSurface(shader, g_WhiteTex, glm::vec3(0.25f, 0.25f, 0.27f), 0.0f);
    for (float x : { -0.44f, 0.44f })
        for (float z : { -0.79f, 0.79f })
            DrawBox(uniformModel, base, glm::vec3(x, -TABLE_TOP_Y / 2.0f, z),
                    glm::vec3(0.04f, TABLE_TOP_Y - 0.04f, 0.04f));
}

// Laptop: base + lid hinged at the back edge, tilted back away from the camera
static void RenderLaptop(Shader* shader, GLuint uniformModel)
{
    glm::mat4 base = glm::translate(glm::mat4(1.0f), LAPTOP_POS);

    if (g_LaptopObj)
    {
        SetSurface(shader, g_WhiteTex, glm::vec3(0.8f), 0.0f);
        Draw(g_LaptopObj, uniformModel, base);
        return;
    }

    SetSurface(shader, g_WhiteTex, glm::vec3(0.18f, 0.18f, 0.2f), 0.0f);
    DrawBox(uniformModel, base, glm::vec3(0.0f, 0.009f, 0.0f), glm::vec3(0.23f, 0.018f, 0.33f));

    // Rotating about Z by +angle tips +Y towards -X (away from the camera)
    glm::mat4 lid = glm::translate(base, glm::vec3(-0.115f, 0.018f, 0.0f));
    lid = glm::rotate(lid, glm::radians(LID_TILT_DEG), glm::vec3(0.0f, 0.0f, 1.0f));
    DrawBox(uniformModel, lid, glm::vec3(-0.004f, 0.11f, 0.0f), glm::vec3(0.008f, 0.22f, 0.33f));
}

static void RenderMug(Shader* shader, GLuint uniformModel)
{
    glm::mat4 base = glm::translate(glm::mat4(1.0f), MUG_POS);
    SetSurface(shader, g_WhiteTex, glm::vec3(0.75f, 0.2f, 0.18f), 0.0f);

    if (g_MugObj)
        Draw(g_MugObj, uniformModel, base);
    else
        DrawBox(uniformModel, base, glm::vec3(0.0f, 0.05f, 0.0f), glm::vec3(0.08f, 0.10f, 0.08f));
}

static void RenderFlowerpot(Shader* shader, GLuint uniformModel)
{
    glm::mat4 base = glm::translate(glm::mat4(1.0f), FLOWERPOT_POS);

    if (g_FlowerpotObj)
    {
        SetSurface(shader, g_WhiteTex, glm::vec3(0.6f, 0.32f, 0.2f), 0.0f);
        Draw(g_FlowerpotObj, uniformModel, base);
        return;
    }

    SetSurface(shader, g_WhiteTex, glm::vec3(0.6f, 0.32f, 0.2f), 0.0f);
    DrawBox(uniformModel, base, glm::vec3(0.0f, 0.065f, 0.0f), glm::vec3(0.14f, 0.13f, 0.14f));

    SetSurface(shader, g_WhiteTex, glm::vec3(0.25f, 0.55f, 0.25f), 0.0f);
    DrawBox(uniformModel, base, glm::vec3(0.0f, 0.2f, 0.0f), glm::vec3(0.12f, 0.14f, 0.12f));
}

void RenderDeskScene(Shader* shader, GLuint uniformModel)
{
    RenderTable(shader, uniformModel);
    RenderLaptop(shader, uniformModel);
    RenderMug(shader, uniformModel);
    RenderFlowerpot(shader, uniformModel);

    // leave shared uniforms as Person A expects them
    glUniform3fv(shader->GetUniformLocation("tintColor"), 1, glm::value_ptr(glm::vec3(1.0f)));
    glUniform1f(shader->GetUniformLocation("emissiveStrength"), 0.0f);
}
