#ifndef DESKSCENE____H
#define DESKSCENE____H

#include <GL/glew.h>

#include "Libs/Shader.h"

// Person B: folding table, laptop + screen, mug, flower pot
void CreateDeskScene();
void RenderDeskScene(Shader* shader, GLuint uniformModel);

#endif
