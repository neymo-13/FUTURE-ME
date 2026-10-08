#ifndef DESKSCENE____H
#define DESKSCENE____H

#include <GL/glew.h>

#include "Libs/Shader.h"

// Table, laptop, mug, photo frame, snowman, RV windows
void CreateDeskScene();
void RenderDeskScene(Shader* shader, GLuint uniformModel);

// Point lights 2-3, call before drawing
void SetDeskLights(Shader* shader);

#endif
