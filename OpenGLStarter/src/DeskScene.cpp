#include "DeskScene.h"

#include "SceneUtils.h"
#include "ProjectPaths.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stb_image.h>

#include <filesystem>
#include <string>
#include <vector>

// Camera looks -X, image right is -Z. Units in metres, ground y = 0.
const float TABLE_TOP_Y = 0.78f;
const glm::vec3 DESK_POS   = glm::vec3(7.8f, 0.0f, -8.25f);
const glm::vec3 LAPTOP_POS = DESK_POS + glm::vec3(-0.05f, TABLE_TOP_Y,  0.25f);
const glm::vec3 MUG_POS    = DESK_POS + glm::vec3(-0.15f, TABLE_TOP_Y, -0.2f);
const glm::vec3 FRAME_POS  = DESK_POS + glm::vec3(-0.3f,  TABLE_TOP_Y,  0.6f);

// Photo frame
const float FRAME_PHOTO_W = 0.12f;
const float FRAME_PHOTO_H = 0.16f;
const float FRAME_BORDER  = 0.012f;
const float FRAME_DEPTH   = 0.012f;
const float FRAME_LEAN    = 12.0f;   // degrees
const float FRAME_TURN    = 19.0f;   // degrees
const glm::vec3 SNOWMAN_POS = glm::vec3(5.86f, 0.0f, -6.0f);

// Same position as the RV in CampScene.cpp
const glm::vec3 RV_POS = glm::vec3(0.0f, 0.0f, -3.0f);
const glm::vec3 RV_WINDOW_COLOUR = glm::vec3(1.0f, 0.78f, 0.45f);

// One mesh per colour of a model
struct Part
{
    Mesh* mesh;
    glm::vec3 colour;
    float emissive;
};

static std::vector<Part> g_Table, g_Laptop, g_Mug, g_Snowman;

static Mesh*  g_Box      = nullptr;
static GLuint g_WhiteTex = 0;

static Mesh*  g_ScreenQuad = nullptr;
static GLuint g_ScreenTex  = 0;
static Mesh*  g_PhotoQuad  = nullptr;
static GLuint g_PhotoTex   = 0;

static std::vector<Mesh*> g_RVWindows;

static Mesh*  g_WoodQuad = nullptr;
static GLuint g_WoodTex  = 0;

static bool HasFile(const char* dir, const char* name)
{
    return std::filesystem::exists(std::filesystem::u8path(dir) / name);
}

// Empty list = model missing, draw boxes instead
static void AddPart(std::vector<Part>& parts, const char* name, glm::vec3 colour, float emissive = 0.0f)
{
    if (HasFile(OPENGL_STARTER_MODEL_DIR, name))
        parts.push_back({ LoadModelOrBox(name), colour, emissive });
}

// sRGB texture: avoids washed-out colours since the shader gamma-corrects its output
static GLuint LoadTextureSRGB(const char* name)
{
    const std::string path = (std::filesystem::u8path(OPENGL_STARTER_TEXTURE_DIR) / name).string();

    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 3);
    if (!data)
        return LoadTexture(name);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    textureList.push_back(texture);
    return texture;
}

// 1x1 white texture, coloured with tintColor
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

// Texture, colour and emissive for the next draw
static void SetSurface(Shader* shader, GLuint texture, glm::vec3 tint, float emissive)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(shader->GetUniformLocation("texture_albedo"), 0);
    glUniform1f(shader->GetUniformLocation("material_ao"), 1.0f);
    glUniform1f(shader->GetUniformLocation("emissiveStrength"), emissive);
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

static void DrawBox(Shader* shader, GLuint uniformModel, glm::mat4 base,
                    glm::vec3 center, glm::vec3 size, glm::vec3 colour)
{
    SetSurface(shader, g_WhiteTex, colour, 0.0f);
    glm::mat4 model = glm::translate(base, center);
    model = glm::scale(model, size);
    Draw(g_Box, uniformModel, model);
}

// Quad from 4 vertices (position, normal, uv)
static Mesh* CreateQuad(glm::vec3 bl, glm::vec3 br, glm::vec3 tr, glm::vec3 tl, glm::vec3 n)
{
    GLfloat vertices[] =
    {
        //  x     y     z       nx   ny   nz     u     v
        bl.x, bl.y, bl.z,    n.x, n.y, n.z,   0.0f, 0.0f,   // bottom-left
        br.x, br.y, br.z,    n.x, n.y, n.z,   1.0f, 0.0f,   // bottom-right
        tr.x, tr.y, tr.z,    n.x, n.y, n.z,   1.0f, 1.0f,   // top-right
        tl.x, tl.y, tl.z,    n.x, n.y, n.z,   0.0f, 1.0f,   // top-left
    };
    unsigned int indices[] = { 0, 1, 2,   0, 2, 3 };

    Mesh* quad = new Mesh();
    quad->CreateMesh(vertices, indices, 32, 6);
    meshList.push_back(quad);
    return quad;
}

// Laptop screen, tilted back with the lid
static Mesh* CreateLaptopScreen()
{
    const float xBottom = -0.0945f, yBottom = 0.0185f;
    const float xTop    = -0.1128f, yTop    = 0.2140f;
    const float halfW   = 0.157f;

    return CreateQuad(glm::vec3(xBottom, yBottom,  halfW), glm::vec3(xBottom, yBottom, -halfW),
                      glm::vec3(xTop,    yTop,    -halfW), glm::vec3(xTop,    yTop,     halfW),
                      glm::vec3(0.9956f, 0.0940f, 0.0f));
}

// Photo inside the frame border
static Mesh* CreateFramePhoto()
{
    const float x      = 0.001f;
    const float bottom = FRAME_BORDER, top = FRAME_BORDER + FRAME_PHOTO_H;
    const float half   = FRAME_PHOTO_W / 2.0f;

    return CreateQuad(glm::vec3(x, bottom,  half), glm::vec3(x, bottom, -half),
                      glm::vec3(x, top,    -half), glm::vec3(x, top,     half),
                      glm::vec3(1.0f, 0.0f, 0.0f));
}

// Wood top (table model has no UVs)
static Mesh* CreateTableTop()
{
    const float y     = TABLE_TOP_Y + 0.002f;
    const float halfX = 0.51f, halfZ = 0.85f;

    return CreateQuad(glm::vec3( halfX, y,  halfZ), glm::vec3(-halfX, y,  halfZ),
                      glm::vec3(-halfX, y, -halfZ), glm::vec3( halfX, y, -halfZ),
                      glm::vec3(0.0f, 1.0f, 0.0f));
}

// Glowing RV windows
static void CreateRVWindows()
{
    struct Window { float x, zNear, zFar, yBottom, yTop; };
    const Window windows[] =
    {
        { 1.075f,  0.256f, -0.210f, 1.448f, 1.866f },   // middle
        { 1.110f, -0.495f, -0.837f, 1.362f, 1.685f },   // small
        { 1.110f, -1.417f, -2.453f, 1.267f, 1.942f },   // large
    };

    for (const Window& w : windows)
        g_RVWindows.push_back(CreateQuad(glm::vec3(w.x, w.yBottom, w.zNear), glm::vec3(w.x, w.yBottom, w.zFar),
                                         glm::vec3(w.x, w.yTop,    w.zFar),  glm::vec3(w.x, w.yTop,    w.zNear),
                                         glm::vec3(1.0f, 0.0f, 0.0f)));
}

void CreateDeskScene()
{
    g_Box = CreateBox();
    g_WhiteTex = CreateWhiteTexture();

    AddPart(g_Table, "table_top.obj",   glm::vec3(0.63f, 0.70f, 0.72f));
    AddPart(g_Table, "table_frame.obj", glm::vec3(0.55f, 0.42f, 0.32f));

    g_WoodQuad = CreateTableTop();
    g_WoodTex  = LoadTextureSRGB("wood.jpg");

    AddPart(g_Laptop, "laptop_body.obj", glm::vec3(0.184f));
    AddPart(g_Laptop, "laptop_keys.obj", glm::vec3(0.04f));

    g_ScreenQuad = CreateLaptopScreen();
    g_ScreenTex  = LoadTexture("screen_code.png");

    AddPart(g_Mug, "mug_outer.obj",  glm::vec3(0.698f, 0.106f, 0.106f));
    AddPart(g_Mug, "mug_inner.obj",  glm::vec3(0.588f));
    AddPart(g_Mug, "mug_coffee.obj", glm::vec3(0.071f, 0.012f, 0.0f));

    g_PhotoQuad = CreateFramePhoto();
    g_PhotoTex  = LoadTexture("frame_photo.jpg");

    CreateRVWindows();

    AddPart(g_Snowman, "snowman_body.obj",   glm::vec3(1.0f));
    AddPart(g_Snowman, "snowman_dark.obj",   glm::vec3(0.06f, 0.10f, 0.13f));
    AddPart(g_Snowman, "snowman_red.obj",    glm::vec3(1.0f, 0.09f, 0.01f));
    AddPart(g_Snowman, "snowman_orange.obj", glm::vec3(1.0f, 0.4f, 0.05f));
    AddPart(g_Snowman, "snowman_brown.obj",  glm::vec3(0.19f, 0.09f, 0.06f));
}

static void RenderTable(Shader* shader, GLuint uniformModel)
{
    glm::mat4 base = glm::translate(glm::mat4(1.0f), DESK_POS);

    if (!g_Table.empty())
    {
        DrawParts(shader, uniformModel, g_Table, glm::rotate(base, glm::radians(90.0f), glm::vec3(0, 1, 0)));

        SetSurface(shader, g_WoodTex, glm::vec3(1.0f), 0.0f);
        Draw(g_WoodQuad, uniformModel, base);
        return;
    }

    // Placeholder: top + 4 legs
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

    if (!g_Laptop.empty())
    {
        DrawParts(shader, uniformModel, g_Laptop, base);

        // Glowing screen
        SetSurface(shader, g_ScreenTex, glm::vec3(1.0f), 1.0f);
        Draw(g_ScreenQuad, uniformModel, base);
        return;
    }

    // Placeholder: base + tilted lid
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
        DrawParts(shader, uniformModel, g_Mug, glm::rotate(base, glm::radians(-120.0f), glm::vec3(0, 1, 0)));
        return;
    }

    DrawBox(shader, uniformModel, base, glm::vec3(0.0f, 0.05f, 0.0f),
            glm::vec3(0.08f, 0.10f, 0.08f), glm::vec3(0.7f, 0.1f, 0.1f));
}

// Photo frame: panel + photo + stand
static void RenderFrame(Shader* shader, GLuint uniformModel)
{
    const glm::vec3 red = glm::vec3(0.95f, 0.15f, 0.15f);
    const float lean = glm::radians(FRAME_LEAN);

    glm::mat4 base = glm::translate(glm::mat4(1.0f), FRAME_POS);
    base = glm::rotate(base, glm::radians(FRAME_TURN), glm::vec3(0, 1, 0));

    // Lean back
    glm::mat4 frame = glm::rotate(base, lean, glm::vec3(0, 0, 1));

    const float outerH = FRAME_PHOTO_H + 2.0f * FRAME_BORDER;
    const float outerW = FRAME_PHOTO_W + 2.0f * FRAME_BORDER;
    DrawBox(shader, uniformModel, frame, glm::vec3(-FRAME_DEPTH / 2.0f, outerH / 2.0f, 0.0f),
            glm::vec3(FRAME_DEPTH, outerH, outerW), red);

    SetSurface(shader, g_PhotoTex, glm::vec3(1.0f), 1.0f);
    Draw(g_PhotoQuad, uniformModel, frame);

    // Stand from the back of the frame to the table
    glm::vec2 top(-FRAME_DEPTH * cosf(lean) - 0.10f * sinf(lean),
                  -FRAME_DEPTH * sinf(lean) + 0.10f * cosf(lean));
    glm::vec2 foot(-0.085f, 0.0f);
    glm::vec2 d = top - foot;
    glm::mat4 leg = glm::translate(base, glm::vec3((top + foot) / 2.0f, 0.0f));
    leg = glm::rotate(leg, atan2f(-d.x, d.y), glm::vec3(0, 0, 1));
    DrawBox(shader, uniformModel, leg, glm::vec3(0.0f), glm::vec3(0.006f, glm::length(d), 0.03f), red);
}

static void RenderSnowman(Shader* shader, GLuint uniformModel)
{
    if (g_Snowman.empty()) return;

    glm::mat4 base = glm::translate(glm::mat4(1.0f), SNOWMAN_POS);
    base = glm::rotate(base, glm::radians(-53.0f), glm::vec3(0, 1, 0));
    DrawParts(shader, uniformModel, g_Snowman, base);
}

// Point lights: laptop screen (2) and RV windows (3)
void SetDeskLights(Shader* shader)
{
    // In front of the screen
    const glm::vec3 screenCentre = glm::vec3(-0.104f, 0.116f, 0.0f);
    const glm::vec3 lightPos = LAPTOP_POS + screenCentre + glm::vec3(0.15f, 0.0f, 0.0f);

    // No intensity uniform, so the colour is scaled
    const glm::vec3 screenBlue = glm::vec3(0.43f, 0.78f, 1.0f) * 1.5f;
    SetPointLight(shader, 2, lightPos, screenBlue, 1.0f, 0.35f, 0.44f);

    // Outside the RV wall (no shadows)
    const glm::vec3 rvLightPos = RV_POS + glm::vec3(2.3f, 1.2f, -1.0f);
    SetPointLight(shader, 3, rvLightPos, RV_WINDOW_COLOUR * 1.5f, 1.0f, 0.35f, 0.44f);
}

static void RenderRVWindows(Shader* shader, GLuint uniformModel)
{
    SetSurface(shader, g_WhiteTex, RV_WINDOW_COLOUR, 1.0f);
    glm::mat4 base = glm::translate(glm::mat4(1.0f), RV_POS);
    for (Mesh* window : g_RVWindows)
        Draw(window, uniformModel, base);
}

void RenderDeskScene(Shader* shader, GLuint uniformModel)
{
    RenderTable(shader, uniformModel);
    RenderLaptop(shader, uniformModel);
    RenderMug(shader, uniformModel);
    RenderFrame(shader, uniformModel);
    RenderSnowman(shader, uniformModel);
    RenderRVWindows(shader, uniformModel);

    // Reset shared uniforms
    glUniform3fv(shader->GetUniformLocation("tintColor"), 1, glm::value_ptr(glm::vec3(1.0f)));
    glUniform1f(shader->GetUniformLocation("emissiveStrength"), 0.0f);
}
