#include "SceneUtils.h"

#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#include <stb_image.h>

#include "ProjectPaths.h"

const std::filesystem::path textureDirectory =
    std::filesystem::u8path(OPENGL_STARTER_TEXTURE_DIR);
const std::filesystem::path modelDirectory =
    std::filesystem::u8path(OPENGL_STARTER_MODEL_DIR);

// Placeholder box for missing .obj files
Mesh* CreateBox()
{
    struct Face { glm::vec3 n, u, v; };
    const Face faces[6] =
    {
        { { 1, 0, 0}, { 0, 0,-1}, {0, 1, 0} },
        { {-1, 0, 0}, { 0, 0, 1}, {0, 1, 0} },
        { { 0, 1, 0}, { 1, 0, 0}, {0, 0,-1} },
        { { 0,-1, 0}, { 1, 0, 0}, {0, 0, 1} },
        { { 0, 0, 1}, { 1, 0, 0}, {0, 1, 0} },
        { { 0, 0,-1}, {-1, 0, 0}, {0, 1, 0} },
    };
    const glm::vec2 corners[4] = { {0, 0}, {1, 0}, {1, 1}, {0, 1} };

    std::vector<VertexPNU> vertices;
    std::vector<unsigned int> indices;
    for (const Face& f : faces)
    {
        unsigned int base = (unsigned int)vertices.size();
        for (const glm::vec2& c : corners)
        {
            VertexPNU v;
            v.position = f.n * 0.5f + (c.x - 0.5f) * f.u + (c.y - 0.5f) * f.v;
            v.normal = f.n;
            v.uv = c;
            vertices.push_back(v);
        }
        indices.insert(indices.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
    }

    Mesh* box = new Mesh();
    box->CreateMesh(vertices, indices);
    meshList.push_back(box);
    return box;
}

Mesh* LoadModelOrBox(const std::string& fileName)
{
    Mesh* mesh = new Mesh();
    if (mesh->CreateMeshFromOBJ((modelDirectory / fileName).string()))
    {
        meshList.push_back(mesh);
        return mesh;
    }

    delete mesh;
    std::cout << "Missing model Models/" << fileName << " -> using a box instead" << std::endl;
    return CreateBox();
}

// Falls back to white if the image is missing
GLuint LoadTexture(const std::string& fileName)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);

    const std::string path = (textureDirectory / fileName).string();
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (!data)
    {
        std::cout << "Missing texture Textures/" << fileName << " -> using white instead" << std::endl;
        const unsigned char white[3] = { 255, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, white);
    }
    else
    {
        GLenum format = GL_RGB;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }
    glGenerateMipmap(GL_TEXTURE_2D);

    textureList.push_back(texture);
    return texture;
}

void SetMaterial(Shader* shader, float specularStrength, float shininess, float emissiveStrength)
{
    glUniform1f(shader->GetUniformLocation("material.specularStrength"), specularStrength);
    glUniform1f(shader->GetUniformLocation("material.shininess"), shininess);
    glUniform1f(shader->GetUniformLocation("emissiveStrength"), emissiveStrength);
}

void SetPointLight(Shader* shader, int index, glm::vec3 position, glm::vec3 colour,
                   float constant, float linear, float quadratic)
{
    const std::string name = "pointLights[" + std::to_string(index) + "].";
    glUniform3fv(shader->GetUniformLocation((name + "position").c_str()), 1, glm::value_ptr(position));
    glUniform3fv(shader->GetUniformLocation((name + "colour").c_str()), 1, glm::value_ptr(colour));
    glUniform1f(shader->GetUniformLocation((name + "constant").c_str()), constant);
    glUniform1f(shader->GetUniformLocation((name + "linear").c_str()), linear);
    glUniform1f(shader->GetUniformLocation((name + "quadratic").c_str()), quadratic);
}
