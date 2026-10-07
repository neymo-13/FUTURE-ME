#ifndef SCENEUTILS____H
#define SCENEUTILS____H

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

#include "Libs/Mesh.h"
#include "Libs/Shader.h"

// Defined in main.cpp; everything created below is freed in Cleanup()
extern std::vector<Mesh*> meshList;
extern std::vector<GLuint> textureList;

Mesh* CreateBox();
Mesh* LoadModelOrBox(const std::string& fileName);
GLuint LoadTexture(const std::string& fileName);

void SetMaterial(Shader* shader, float specularStrength, float shininess, float emissiveStrength);
void SetPointLight(Shader* shader, int index, glm::vec3 position, glm::vec3 colour,
                   float constant, float linear, float quadratic);

#endif
