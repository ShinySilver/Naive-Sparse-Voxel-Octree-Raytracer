#pragma once

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "cpmath.h"

extern vec3 camera_pos;
extern vec3 camera_forward;

void camera_update(GLFWwindow *window, float deltaTime);