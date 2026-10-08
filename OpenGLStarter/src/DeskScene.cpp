#include "DeskScene.h"

#include "SceneUtils.h"
#include "ProjectPaths.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stb_image.h>   // implementation lives in SceneUtils.cpp

#include <filesystem>
#include <string>
#include <vector>

// Scene faces -X (camera forward), right of the image is -Z. Units are metres, ground y = 0.
// Move DESK_POS to move the whole desk set; other positions are offsets from it.
const float TABLE_TOP_Y = 0.78f;    // top surface of the table model
// Placed so the campfire is ahead-right and the RV ahead-left from the locked camera (main.cpp)
const glm::vec3 DESK_POS   = glm::vec3(7.8f, 0.0f, -8.25f);
const glm::vec3 LAPTOP_POS = DESK_POS + glm::vec3(-0.05f, TABLE_TOP_Y,  0.25f);
const glm::vec3 MUG_POS    = DESK_POS + glm::vec3(-0.15f, TABLE_TOP_Y, -0.2f);
const glm::vec3 FRAME_POS  = DESK_POS + glm::vec3(-0.3f,  TABLE_TOP_Y,  0.6f);

// Standing photo frame (metres): photo 12 x 16 cm (3:4), border, leaning back
const float FRAME_PHOTO_W = 0.12f;
const float FRAME_PHOTO_H = 0.16f;
const float FRAME_BORDER  = 0.012f;
const float FRAME_DEPTH   = 0.012f;
const float FRAME_LEAN    = 12.0f;   // degrees
const float FRAME_TURN    = 19.0f;   // degrees about Y, so it faces the seated camera
const glm::vec3 SNOWMAN_POS = glm::vec3(5.86f, 0.0f, -6.0f);  // on the snow, left of the desk

// RV is drawn by Person A at this position (CampScene.cpp, no rotation/scale); keep in sync
const glm::vec3 RV_POS = glm::vec3(0.0f, 0.0f, -3.0f);
const glm::vec3 RV_WINDOW_COLOUR = glm::vec3(1.0f, 0.78f, 0.45f);   // warm interior light

// Models in Models/ are split per colour and scaled to metres, origin at bottom centre.
struct Part
{
    Mesh* mesh;
    glm::vec3 colour;     // from the model's .mtl
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

// Adds the part only if its .obj exists; an empty list means "draw boxes instead"
static void AddPart(std::vector<Part>& parts, const char* name, glm::vec3 colour, float emissive = 0.0f)
{
    if (HasFile(OPENGL_STARTER_MODEL_DIR, name))
        parts.push_back({ LoadModelOrBox(name), colour, emissive });
}

// Loads an image as an sRGB texture: OpenGL converts its colours to linear when sampled.
// Needed for lit textures because the main shader gamma-corrects its output
// (pow(result, 1/2.2)); a plain RGB texture would be brightened twice and look washed out.
static GLuint LoadTextureSRGB(const char* name)
{
    const std::string path = (std::filesystem::u8path(OPENGL_STARTER_TEXTURE_DIR) / name).string();

    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 3);
    if (!data)
        return LoadTexture(name);   // prints the missing file and falls back to white

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

// Screen quad built from our own 4 vertices (P3 N3 UV2 = 8 floats each).
// Corners are given as seen by the viewer, so the image is upright on screen.
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

// Laptop screen, matching the screen of laptop_body.obj (laptop-local metres).
// The lid leans back, so the top edge sits further -X; it faces +X (the camera).
// Viewed from the camera, image left is +Z and image right is -Z.
static Mesh* CreateLaptopScreen()
{
    const float xBottom = -0.0945f, yBottom = 0.0185f;
    const float xTop    = -0.1128f, yTop    = 0.2140f;
    const float halfW   = 0.157f;

    return CreateQuad(glm::vec3(xBottom, yBottom,  halfW), glm::vec3(xBottom, yBottom, -halfW),
                      glm::vec3(xTop,    yTop,    -halfW), glm::vec3(xTop,    yTop,     halfW),
                      glm::vec3(0.9956f, 0.0940f, 0.0f));
}

// Photo inside the frame, standing upright in frame-local space (the lean is applied later).
// Frame front face is x = 0 and faces +X; the photo sits 1 mm in front of it, inside the border.
static Mesh* CreateFramePhoto()
{
    const float x      = 0.001f;
    const float bottom = FRAME_BORDER, top = FRAME_BORDER + FRAME_PHOTO_H;
    const float half   = FRAME_PHOTO_W / 2.0f;

    return CreateQuad(glm::vec3(x, bottom,  half), glm::vec3(x, bottom, -half),
                      glm::vec3(x, top,    -half), glm::vec3(x, top,     half),
                      glm::vec3(1.0f, 0.0f, 0.0f));
}

// Wood top laid on the table (the table model has no UVs, so it cannot take a texture itself).
// Desk-local metres, 2 mm above the top surface. The grain (image vertical) runs along the
// table's long side (Z, 1.71 m); image horizontal runs across its depth (X, 1.03 m).
static Mesh* CreateTableTop()
{
    const float y     = TABLE_TOP_Y + 0.002f;
    const float halfX = 0.51f, halfZ = 0.85f;

    return CreateQuad(glm::vec3( halfX, y,  halfZ), glm::vec3(-halfX, y,  halfZ),
                      glm::vec3(-halfX, y, -halfZ), glm::vec3( halfX, y, -halfZ),
                      glm::vec3(0.0f, 1.0f, 0.0f));
}

// Glowing panes for the 3 living-area windows on the RV's +X side (RV-local metres).
// Sized to the glass inside each window frame (measured from a straight side view)
// and placed just in front of that window's glass (x differs per window).
static void CreateRVWindows()
{
    struct Window { float x, zNear, zFar, yBottom, yTop; };   // zNear = image-left (+Z) edge
    const Window windows[] =
    {
        { 1.075f,  0.256f, -0.210f, 1.448f, 1.866f },   // middle window
        { 1.110f, -0.495f, -0.837f, 1.362f, 1.685f },   // small window
        { 1.110f, -1.417f, -2.453f, 1.267f, 1.942f },   // large rear window
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
    // Light wood (the .mtl's dark brown 0.19,0.09,0.06 reflects almost no blue screen light)
    AddPart(g_Table, "table_frame.obj", glm::vec3(0.55f, 0.42f, 0.32f));

    g_WoodQuad = CreateTableTop();
    g_WoodTex  = LoadTextureSRGB("wood.jpg");

    // The model's own black screen is replaced by g_ScreenQuad
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
        // Model's long side is along X; turn it to run across the view (Z)
        DrawParts(shader, uniformModel, g_Table, glm::rotate(base, glm::radians(90.0f), glm::vec3(0, 1, 0)));

        // Wood grain on top, lit normally (screen light shows on it)
        SetSurface(shader, g_WoodTex, glm::vec3(1.0f), 0.0f);
        Draw(g_WoodQuad, uniformModel, base);
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

        // Glowing code screen: emissive 1 shows the texture colour as-is, unaffected by lighting
        SetSurface(shader, g_ScreenTex, glm::vec3(1.0f), 1.0f);
        Draw(g_ScreenQuad, uniformModel, base);
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

// Standing photo frame: red panel + photo, leaning back on a stand behind it
static void RenderFrame(Shader* shader, GLuint uniformModel)
{
    const glm::vec3 red = glm::vec3(0.95f, 0.15f, 0.15f);
    const float lean = glm::radians(FRAME_LEAN);

    // base: on the table, turned to face the camera. Origin = bottom-front edge of the frame.
    glm::mat4 base = glm::translate(glm::mat4(1.0f), FRAME_POS);
    base = glm::rotate(base, glm::radians(FRAME_TURN), glm::vec3(0, 1, 0));

    // Rotating about Z by +lean tips the frame's top towards -X (away from the camera)
    glm::mat4 frame = glm::rotate(base, lean, glm::vec3(0, 0, 1));

    const float outerH = FRAME_PHOTO_H + 2.0f * FRAME_BORDER;
    const float outerW = FRAME_PHOTO_W + 2.0f * FRAME_BORDER;
    DrawBox(shader, uniformModel, frame, glm::vec3(-FRAME_DEPTH / 2.0f, outerH / 2.0f, 0.0f),
            glm::vec3(FRAME_DEPTH, outerH, outerW), red);

    // Photo: emissive 1 shows its own colours at full brightness so it reads at night
    SetSurface(shader, g_PhotoTex, glm::vec3(1.0f), 1.0f);
    Draw(g_PhotoQuad, uniformModel, frame);

    // Stand: from the frame's back (10 cm up) down to the table behind it
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

    // Model faces -Z; turn it to face the seated camera
    glm::mat4 base = glm::translate(glm::mat4(1.0f), SNOWMAN_POS);
    base = glm::rotate(base, glm::radians(-53.0f), glm::vec3(0, 1, 0));
    DrawParts(shader, uniformModel, g_Snowman, base);
}

// Blue glow from the laptop screen: a point light just in front of the screen centre.
// Attenuation 1 / (constant + linear*d + quadratic*d^2) fades it out within ~2.5 m,
// so it lights the desk, mug and photo frame but not the far scene.
void SetDeskLights(Shader* shader)
{
    // Screen centre in laptop-local space, pushed 15 cm out along the screen normal (+X)
    const glm::vec3 screenCentre = glm::vec3(-0.104f, 0.116f, 0.0f);
    const glm::vec3 lightPos = LAPTOP_POS + screenCentre + glm::vec3(0.15f, 0.0f, 0.0f);

    // The shader has no light intensity, so the colour is scaled up instead (blue x 1.5)
    const glm::vec3 screenBlue = glm::vec3(0.43f, 0.78f, 1.0f) * 1.5f;
    SetPointLight(shader, 2, lightPos, screenBlue, 1.0f, 0.35f, 0.44f);

    // Warm light spilling out of the RV windows onto the snow beside it (pointLights[3]).
    // There are no shadows, so it sits outside the wall rather than inside the RV,
    // far enough out that it spreads over the snow instead of a hot spot on the wall.
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

    // leave shared uniforms as Person A expects them
    glUniform3fv(shader->GetUniformLocation("tintColor"), 1, glm::value_ptr(glm::vec3(1.0f)));
    glUniform1f(shader->GetUniformLocation("emissiveStrength"), 0.0f);
}
