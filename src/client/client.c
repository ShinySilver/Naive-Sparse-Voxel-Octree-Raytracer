#include <unistd.h>
#include "client/client.h"
#include "client/context.h"
#include "client/camera.h"
#include "common/log.h"
#include "common/terrain.h"
#include "cptime.h"

#define UCLOCKS_PER_SECONDS (1e6)


void client_start(void) {
    /**
     * Generating world data
     * Depths: 1 is 64 blocks, 2 is 256, 3 is 1024, 4 is 4096, 5 is 16384, 6 is 65536, 7 is 262144.
     */
    INFO("Generating terrain.");
    Terrain terrain;
    terrain_init(&terrain, 4);

    /**
     * Creating context
     */
    INFO("Creating context.");
    GLFWwindow *window = NULL;
    if (!(window = context_init())) FATAL("Could not initialize context!");

    /**
     * Registering our callbacks into the window
     */
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    /**
     * Some stats in order to compute framerate
     */
    u32 frametime = 0, accum = 0, count = 0, time = uclock();

    /**
     * Render loop!
     */
    INFO("Client ticking!");
    while (!glfwWindowShouldClose(window)) {
        /**
         * Handle camera movements
         */
        glfwPollEvents();
        camera_update(window, frametime / UCLOCKS_PER_SECONDS);

        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
        glfwSwapBuffers(window);

        /**
         * Punctually print the average frame time
         */
        frametime = uclock() - time;
        time = uclock();
        accum += frametime;
        count++;
        if (accum / UCLOCKS_PER_SECONDS >= 2) {
            INFO("Frame Time: %.2fms", (accum / (float) count / UCLOCKS_PER_SECONDS * 1000.0f));
            accum = 0;
            count = 0;
        }
    }
    INFO("Client exiting.");

    /**
     * Closing all opened buffers and destroying context since all the GL stuff is above
     */
    context_terminate();

    /**
     * Freeing the world data
     */
    terrain_destroy(&terrain);
}