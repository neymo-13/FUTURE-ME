#ifndef CAMPSCENE____H
#define CAMPSCENE____H

#include <GL/glew.h>

#include "Libs/Shader.h"

// Person A: sky, snow, RV, mountains, trees, campfire, chair
void CreateCampScene();
void RenderCampScene(Shader* shader, GLuint uniformModel);

#endif
