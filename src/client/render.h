#pragma once

#include "common/terrain.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"

extern int render_resolution_x, render_resolution_y;

void render_init(GLFWwindow *window);
void render_terminate(void);
void render_draw_frame(Terrain *terrain);
