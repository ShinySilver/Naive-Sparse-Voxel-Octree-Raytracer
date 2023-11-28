#pragma once

#include <stdbool.h>
#include "glad/glad.h"
#include "GLFW/glfw3.h"

#define WIN_VSYNC true

extern bool context_heat_map_mode,
            context_depth_map_mode,
            context_is_fullscreen,
            context_imgui_enabled,
            context_sticky_win;

GLFWwindow *context_init(void);
void context_terminate(void);

void context_set_fullscreen(bool);
void context_set_vsync(bool);
void context_set_max_framerate(int);

int context_get_width(void);
int context_get_height(void);