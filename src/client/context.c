#include "context.h"
#include "client.h"
#include "common/log.h"
#include <string.h>

int win_width, win_height, win_x, win_y;
bool context_heat_map_mode, context_depth_map_mode, context_is_fullscreen, context_imgui_enabled, context_sticky_win;

static int prev_win_width = CLIENT_WIN_WIDTH, prev_win_height = CLIENT_WIN_HEIGHT;
static GLFWwindow *window = NULL;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
static void APIENTRY gl_debug_output(GLenum source, GLenum type, unsigned int id, GLenum severity,
                                     GLsizei length, const char *message, const void *userParam);

GLFWwindow *context_init(void) {

    /**
     * Ensuring the context has not been already initialized
     */
    if (window != NULL) {
        FATAL("Tried to init an already existing context!");
    }

    /**
     * Initializing GLFW and giving all the window hints
     */
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

    /**
     * Creating the window...
     */
    window = glfwCreateWindow(CLIENT_WIN_WIDTH, CLIENT_WIN_HEIGHT, "iVy_", NULL, NULL);
    if (window == NULL) {
        glfwTerminate();
        FATAL("Failed to create GLFW window");
    }
    glfwMakeContextCurrent(window);

    /**
     * Had we been using key pooling, we would have uncommented the lines below
     */
    //glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    //glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GL_TRUE);

#if WIN_VSYNC
    glfwSwapInterval(0);
#endif

    /**
     * Moving the mouse to the window
     */
    glfwPollEvents();
    glfwSetCursorPos(window, CLIENT_WIN_WIDTH / 2, CLIENT_WIN_HEIGHT / 2);

    /**
     * Initializing GLAD
     */
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        FATAL("Failed to initialize GLAD");
    }

    /**
     * Initializing the OpenGL debug context
     */
    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(gl_debug_output, NULL);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    } else {
        WARN("Could not create OpenGL debug context, we are running blind!\n");
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    /**
     * Registering our callbacks into the window
     */
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    return window;
}

void context_terminate(void) {
    window = NULL;
    glfwTerminate();
}

void context_set_fullscreen(bool b) {
    if (b) {
        GLFWmonitor *monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        glfwGetWindowSize(window, &prev_win_width, &prev_win_height);
        glfwGetWindowPos(window, &win_x, &win_y);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height,
                             mode->refreshRate);
    } else {
        glfwSetWindowMonitor(window, NULL, 0, 0, prev_win_width,
                             prev_win_height,
                             CLIENT_WIN_DEFAULT_FRAMERATE);
        glfwSetWindowPos(window, win_x, win_y);

    }
}

int context_get_width(void) {
    glfwGetWindowSize(window, &win_width, &win_height);
    return win_width;
}

int context_get_height(void) {
    glfwGetWindowSize(window, &win_width, &win_height);
    return win_height;
}

static void APIENTRY gl_debug_output(GLenum source, GLenum type, unsigned int id, GLenum severity,
                                      GLsizei length, const char *message, const void *userParam) {
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
    WARN("----------------------------");

    char header_str[256];
    snprintf(header_str, 256, "OpenGL debug message (%u): %s", id, message);
    WARN(header_str);

    switch (source) {
        case GL_DEBUG_SOURCE_API: WARN("Source: API");
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: WARN("Source: Window System");
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: WARN("Source: Shader Compiler");
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: WARN("Source: Third Party");
            break;
        case GL_DEBUG_SOURCE_APPLICATION: WARN("Source: Application");
            break;
        case GL_DEBUG_SOURCE_OTHER: WARN("Source: Other");
            break;
    }

    switch (type) {
        case GL_DEBUG_TYPE_ERROR: WARN("Type: Error");
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: WARN("Type: Deprecated Behaviour");
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: WARN("Type: Undefined Behaviour");
            break;
        case GL_DEBUG_TYPE_PORTABILITY: WARN("Type: Portability");
            break;
        case GL_DEBUG_TYPE_PERFORMANCE: WARN("Type: Performance");
            break;
        case GL_DEBUG_TYPE_MARKER: WARN("Type: Marker");
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP: WARN("Type: Push Group");
            break;
        case GL_DEBUG_TYPE_POP_GROUP: WARN("Type: Pop Group");
            break;
        case GL_DEBUG_TYPE_OTHER: WARN("Type: Other");
            break;
    }

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH: WARN("Severity: high");
            break;
        case GL_DEBUG_SEVERITY_MEDIUM: WARN("Severity: medium");
            break;
        case GL_DEBUG_SEVERITY_LOW: WARN("Severity: low");
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: WARN("Severity: notification");
            break;
    }
}

static void key_callback(GLFWwindow *_window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_F1:
                context_depth_map_mode = !context_depth_map_mode;
                INFO(context_depth_map_mode ? "Enabling depth map" : "Disabling depth map");
                break;
            case GLFW_KEY_F2:
                context_heat_map_mode = !context_heat_map_mode;
                INFO(context_heat_map_mode ? "Enabling computation time heat map"
                                           : "Disabling computation time heat map");
                break;
            case GLFW_KEY_F3:
                context_imgui_enabled = !context_imgui_enabled;
                INFO(context_imgui_enabled ? "Enabling Imgui" : "Disabling Imgui");
                break;
            case GLFW_KEY_F11:
                context_is_fullscreen = !context_is_fullscreen;
                context_set_fullscreen(context_is_fullscreen);
                break;
            case GLFW_KEY_ESCAPE: INFO("User pressed Escape. Client will now exit");
                glfwSetWindowShouldClose(window, true);
                break;
            case GLFW_KEY_LEFT_ALT:
                context_sticky_win = !context_sticky_win;
                glfwSetInputMode(window, GLFW_CURSOR, context_sticky_win ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
                break;
            default:
                break;
        }
    }
}

static void mouse_button_callback(GLFWwindow *_window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                if (!context_sticky_win) {
                    context_sticky_win = true;
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
                break;
            default:
                break;
        }
    }
}