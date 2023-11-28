#pragma once

#include "common/terrain.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"

void render_init(GLFWwindow *window);
void render_draw_frame(Terrain *terrain);
