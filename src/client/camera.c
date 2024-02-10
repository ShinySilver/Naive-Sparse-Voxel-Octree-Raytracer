#include "camera.h"
#include "context.h"
#include "common/log.h"

vec3 camera_pos = (vec3) {0, 0, 0};
vec3 camera_forward = (vec3) {0.5, 0.5, 0};

void camera_update(GLFWwindow *window, float deltaTime) {
    static float accum = 0;
    static bool has_recently_moved_keyboard = false, has_recently_moved_mouse = false;
    accum += deltaTime;

    float speed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? CAMERA_FAST_SPEED * CAMERA_SPEED_MULTIPLIER : CAMERA_BASE_SPEED * CAMERA_SPEED_MULTIPLIER;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        has_recently_moved_keyboard = true;
        camera_pos = add(camera_pos, mul(camera_forward, deltaTime * speed));
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        has_recently_moved_keyboard = true;
        camera_pos = add(camera_pos, mul(camera_forward, -deltaTime * speed));
    }

    vec3 right = normalize(cross(camera_forward, ((vec3) {0, 1, 0})));
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        has_recently_moved_keyboard = true;
        camera_pos = add(camera_pos, mul(right, -deltaTime * speed));
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        has_recently_moved_keyboard = true;
        camera_pos = add(camera_pos, mul(right, deltaTime * speed));
    }

    static double oldPosX = 0.0;
    static double oldPosY = 0.0;

    double posX = 0.0;
    double posY = 0.0;

    glfwGetCursorPos(window, &posX, &posY);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) || context_sticky_win) {
        double dX = posX - oldPosX;
        double dY = posY - oldPosY;

        if (dX != 0) {
            has_recently_moved_mouse = true;
            camera_forward = normalize(add(camera_forward, mul(right, (float) dX * CAMERA_MOUSE_SENSITIVITY / 1000.0f)));
        }

        if (dY != 0) {
            has_recently_moved_mouse = true;
            camera_forward = normalize(add(camera_forward, mul(((vec3) {0, 1, 0}), (float) -dY * CAMERA_MOUSE_SENSITIVITY *2 / 1000.0f)));
        }
    }

    if (accum > 1) {
        if (has_recently_moved_keyboard || has_recently_moved_mouse) {
            accum = 0;
            if (!has_recently_moved_keyboard) {
                INFO("Player is now looking at %.2f;%.2f;%.2f.", camera_forward.x, camera_forward.y, camera_forward.z);
            } else if (!has_recently_moved_mouse) {
                INFO("Player is now at %.1f;%.1f;%.1f.", camera_pos.x, camera_pos.y, camera_pos.z);
            } else {
                INFO("Player is now at %.1f;%.1f;%.1f and looking at %.2f;%.2f;%.2f.",
                     camera_pos.x, camera_pos.y, camera_pos.z,
                     camera_forward.x, camera_forward.y, camera_forward.z);
            }
            has_recently_moved_mouse = false;
            has_recently_moved_keyboard = false;
        }
    }

    oldPosX = posX;
    oldPosY = posY;
}