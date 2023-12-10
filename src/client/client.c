#include <unistd.h>
#include "client/client.h"
#include "client/context.h"
#include "client/camera.h"
#include "common/log.h"
#include "common/terrain.h"
#include "cptime.h"
#include "render.h"

#define UCLOCKS_PER_SECONDS (1e6)


void client_start(void) {
    /**
     * Generating world data
     * Assuming a node width of 2 and a chunk size of 8, a depth 8 means a 2048x2048x2048 world.
     */
    INFO("Generating terrain.");
    Terrain terrain;
    terrain_init(&terrain, 8);
    camera_pos = (vec3){-0.5*terrain.width,1.5*terrain.width, -0.5*terrain.width};
    camera_forward = (vec3) {0.5, -0.5, 0.5};

    /**
     * Creating context
     */
    INFO("Creating context.");
    GLFWwindow *window = NULL;
    if (!(window = context_init())) FATAL("Could not initialize context!");

    /**
     * Initializing renderer, passing it the context so it initialize the framebuffer at the right size
     */
    render_init(window);

    /**
     * Setup some stats in order to compute framerate
     */
    u32 frametime = 0, accum = 0, count = 0, time = uclock();
    char win_title[64];

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

        /**
         * Do the actual rendering
         */
        render_draw_frame(&terrain);
        glfwSwapBuffers(window);

        /**
         * Punctually print the average frame time
         */
        frametime = uclock() - time;
        time = uclock();
        accum += frametime;
        count++;
        if (accum / UCLOCKS_PER_SECONDS >= 1) {
            float frame_time = (accum / (float) count / UCLOCKS_PER_SECONDS * 1000.0f);
            snprintf(win_title, 64, "iVy - %0.2fms - %0.2fFPS", frame_time, 1e3/frame_time);
            glfwSetWindowTitle(window, win_title);
            accum = 0;
            count = 0;
        }
    }
    INFO("Client exiting.");

    /**
     * Closing all opened buffers and destroying context since all the GL stuff is above
     */
    render_terminate();
    context_terminate();

    /**
     * Freeing the world data
     */
    terrain_destroy(&terrain);
}