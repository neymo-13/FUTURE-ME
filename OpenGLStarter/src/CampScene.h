#ifndef CAMPSCENE____H
#define CAMPSCENE____H

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "Libs/Shader.h"

// Person A: sky, snow, RV, mountains, trees, campfire, chair
void CreateCampScene();
void RenderCampScene(Shader* shader, GLuint uniformModel);
void RenderAurora(Shader* auroraShader, const glm::mat4& view, const glm::mat4& projection);

#endif
