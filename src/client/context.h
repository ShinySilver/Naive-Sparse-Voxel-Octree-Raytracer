#pragma once

#include <stdbool.h>
#include "glad/glad.h"
#include "GLFW/glfw3.h"

// VSync can be toggled on here.
// Note that vsync make frame timing untrustworthy.
#define WIN_VSYNC false

extern bool context_heat_map_mode,
            context_depth_map_mode,
            context_is_fullscreen,
            context_imgui_enabled,
            context_sticky_win;

GLFWwindow *context_init(void);
void context_terminate(void);