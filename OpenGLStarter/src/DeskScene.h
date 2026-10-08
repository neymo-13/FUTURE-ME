#ifndef DESKSCENE____H
#define DESKSCENE____H

#include <GL/glew.h>

#include "Libs/Shader.h"

// Person B: folding table, laptop + screen, mug, photo frame, snowman
void CreateDeskScene();
void RenderDeskScene(Shader* shader, GLuint uniformModel);

// Person B's point lights (pointLights[2] = laptop screen). Call before drawing any scene.
void SetDeskLights(Shader* shader);

#endif
