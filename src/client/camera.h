#pragma once

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "cpmath.h"

#define CAMERA_BASE_SPEED (100)
#define CAMERA_FAST_SPEED (500)
#define CAMERA_SPEED_MULTIPLIER (10)
#define CAMERA_MOUSE_SENSITIVITY (5)

extern vec3 camera_pos;
extern vec3 camera_forward;

void camera_update(GLFWwindow *window, float deltaTime);