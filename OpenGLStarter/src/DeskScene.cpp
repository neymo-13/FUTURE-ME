#include "DeskScene.h"

#include "SceneUtils.h"
#include "ProjectPaths.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <vector>

// Scene faces -X (camera forward), right of the image is -Z. Units are metres, ground y = 0.
// Move DESK_POS to move the whole desk set; other positions are offsets from it.
const float TABLE_TOP_Y = 0.78f;    // top surface of the table model
const glm::vec3 DESK_POS   = glm::vec3(4.6f, 0.0f, -6.9f);
const glm::vec3 LAPTOP_POS = DESK_POS + glm::vec3(-0.05f, TABLE_TOP_Y,  0.25f);
const glm::vec3 MUG_POS    = DESK_POS + glm::vec3( 0.0f,  TABLE_TOP_Y, -0.3f);
const glm::vec3 TABLET_POS = DESK_POS + glm::vec3(-0.3f,  TABLE_TOP_Y,  0.6f);

// Models in Models/ are split per colour and scaled to metres, origin at bottom centre.
struct Part
{
    Mesh* mesh;
    glm::vec3 colour;     // from the model's .mtl
    float emissive;
};

static std::vector<Part> g_Table, g_Laptop, g_Mug, g_Tablet;

static Mesh*  g_Box      = nullptr;
static GLuint g_WhiteTex = 0;

static bool HasFile(const char* dir, const char* name)
{
    return std::filesystem::exists(std::filesystem::u8path(dir) / name);
}

// Adds the part only if its .obj exists; an empty list means "draw boxes instead"
static void AddPart(std::vector<Part>& parts, const char* name, glm::vec3 colour, float emissive = 0.0f)
{
    if (HasFile(OPENGL_STARTER_MODEL_DIR, name))
        parts.push_back({ LoadModelOrBox(name), colour, emissive });
}

// 1x1 white texture, coloured per part with tintColor
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

static void DrawParts(Shader* shader, GLuint uniformModel, const std::vector<Part>& parts, const glm::mat4& model)
{
    for (const Part& p : parts)
    {
        SetSurface(shader, g_WhiteTex, p.colour, p.emissive);
        Draw(p.mesh, uniformModel, model);
    }
}

// Unit box scaled to size, centred at center (placeholder when a model is missing)
static void DrawBox(Shader* shader, GLuint uniformModel, glm::mat4 base,
                    glm::vec3 center, glm::vec3 size, glm::vec3 colour)
{
    SetSurface(shader, g_WhiteTex, colour, 0.0f);
    glm::mat4 model = glm::translate(base, center);
    model = glm::scale(model, size);
    Draw(g_Box, uniformModel, model);
}

void CreateDeskScene()
{
    g_Box = CreateBox();
    g_WhiteTex = CreateWhiteTexture();

    AddPart(g_Table, "table_top.obj",   glm::vec3(0.63f, 0.70f, 0.72f));
    AddPart(g_Table, "table_frame.obj", glm::vec3(0.19f, 0.09f, 0.06f));

    // laptop_screen.obj is drawn for now; the code-screen quad replaces it in the next step
    AddPart(g_Laptop, "laptop_body.obj",   glm::vec3(0.184f));
    AddPart(g_Laptop, "laptop_keys.obj",   glm::vec3(0.04f));
    AddPart(g_Laptop, "laptop_screen.obj", glm::vec3(0.008f));

    AddPart(g_Mug, "mug_outer.obj",  glm::vec3(0.698f, 0.106f, 0.106f));
    AddPart(g_Mug, "mug_inner.obj",  glm::vec3(0.588f));
    AddPart(g_Mug, "mug_coffee.obj", glm::vec3(0.071f, 0.012f, 0.0f));

    AddPart(g_Tablet, "tablet_body.obj",   glm::vec3(0.886f));
    AddPart(g_Tablet, "tablet_back.obj",   glm::vec3(0.02f));
    AddPart(g_Tablet, "tablet_screen.obj", glm::vec3(0.435f, 0.969f, 0.988f), 0.5f);
}

static void RenderTable(Shader* shader, GLuint uniformModel)
{
    glm::mat4 base = glm::translate(glm::mat4(1.0f), DESK_POS);

    if (!g_Table.empty())
    {
        // Model's long side is along X; turn it to run across the view (Z)
        DrawParts(shader, uniformModel, g_Table, glm::rotate(base, glm::radians(90.0f), glm::vec3(0, 1, 0)));
        return;
    }

    // Placeholder: top board + 4 legs
    DrawBox(shader, uniformModel, base, glm::vec3(0.0f, TABLE_TOP_Y - 0.02f, 0.0f),
            glm::vec3(1.0f, 0.04f, 1.7f), glm::vec3(0.55f, 0.38f, 0.22f));
    for (float x : { -0.44f, 0.44f })
        for (float z : { -0.79f, 0.79f })
            DrawBox(shader, uniformModel, base, glm::vec3(x, (TABLE_TOP_Y - 0.04f) / 2.0f, z),
                    glm::vec3(0.04f, TABLE_TOP_Y - 0.04f, 0.04f), glm::vec3(0.25f, 0.25f, 0.27f));
}

static void RenderLaptop(Shader* shader, GLuint uniformModel)
{
    glm::mat4 base = glm::translate(glm::mat4(1.0f), LAPTOP_POS);

    // Model's screen already faces +X (towards the camera)
    if (!g_Laptop.empty())
    {
        DrawParts(shader, uniformModel, g_Laptop, base);
        return;
    }

    // Placeholder: base + lid tilted back 12 degrees
    DrawBox(shader, uniformModel, base, glm::vec3(0.0f, 0.009f, 0.0f),
            glm::vec3(0.23f, 0.018f, 0.33f), glm::vec3(0.18f, 0.18f, 0.2f));
    glm::mat4 lid = glm::translate(base, glm::vec3(-0.115f, 0.018f, 0.0f));
    lid = glm::rotate(lid, glm::radians(12.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    DrawBox(shader, uniformModel, lid, glm::vec3(-0.004f, 0.11f, 0.0f),
            glm::vec3(0.008f, 0.22f, 0.33f), glm::vec3(0.18f, 0.18f, 0.2f));
}

static void RenderMug(Shader* shader, GLuint uniformModel)
{
    glm::mat4 base = glm::translate(glm::mat4(1.0f), MUG_POS);

    if (!g_Mug.empty())
    {
        // Turn the handle (model -X) to the right and slightly towards the camera
        DrawParts(shader, uniformModel, g_Mug, glm::rotate(base, glm::radians(-120.0f), glm::vec3(0, 1, 0)));
        return;
    }

    DrawBox(shader, uniformModel, base, glm::vec3(0.0f, 0.05f, 0.0f),
            glm::vec3(0.08f, 0.10f, 0.08f), glm::vec3(0.7f, 0.1f, 0.1f));
}

static void RenderTablet(Shader* shader, GLuint uniformModel)
{
    glm::mat4 base = glm::translate(glm::mat4(1.0f), TABLET_POS);
    base = glm::rotate(base, glm::radians(20.0f), glm::vec3(0, 1, 0));

    if (!g_Tablet.empty())
    {
        DrawParts(shader, uniformModel, g_Tablet, base);
        return;
    }

    DrawBox(shader, uniformModel, base, glm::vec3(0.0f, 0.005f, 0.0f),
            glm::vec3(0.215f, 0.01f, 0.25f), glm::vec3(0.1f));
}

void RenderDeskScene(Shader* shader, GLuint uniformModel)
{
    RenderTable(shader, uniformModel);
    RenderLaptop(shader, uniformModel);
    RenderMug(shader, uniformModel);
    RenderTablet(shader, uniformModel);

    // leave shared uniforms as Person A expects them
    glUniform3fv(shader->GetUniformLocation("tintColor"), 1, glm::value_ptr(glm::vec3(1.0f)));
    glUniform1f(shader->GetUniformLocation("emissiveStrength"), 0.0f);
}
