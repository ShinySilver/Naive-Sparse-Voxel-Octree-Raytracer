#include "render.h"
#include "cpmath.h"
#include "common/terrain.h"
#include "gllib.h"
#include "client/camera.h"

static u32 terrainChunkPoolSSBO;
static u32 currentChunkBufferSize = 0;

static u32 terrainNodePoolSSBO;
static u32 currentNodeBufferSize = 0;

static u32 svo_tracer_shader;
static Texture *svoTexture;

static void render_framebuffer_size_callback(GLFWwindow *_window, int width, int height);

void render_init(GLFWwindow *window) {
    glfwSetFramebufferSizeCallback(window, render_framebuffer_size_callback);
}


void render_draw_frame(Terrain *terrain) {

    // If the terrain has not been uploaded yet, or if it has been modified
    if (terrain->dirty) {

        // (Re)Uploading the SVO
        // Naive implementation. Must be improved
        if (terrain->chunkPool.maxSize != currentChunkBufferSize) {
            glNamedBufferData(terrainChunkPoolSSBO, terrain->chunkPool.maxSize * terrain->chunkPool.unitSize,
                              terrain->chunkPool.memory, GL_STATIC_DRAW);
        } else {
            glNamedBufferSubData(terrainChunkPoolSSBO, 0, terrain->chunkPool.maxSize * terrain->chunkPool.unitSize,
                                 terrain->chunkPool.memory);
        }
        if (terrain->nodePool.maxSize != currentNodeBufferSize) {
            glNamedBufferData(terrainNodePoolSSBO, terrain->nodePool.maxSize * terrain->nodePool.unitSize,
                              terrain->nodePool.memory, GL_STATIC_DRAW);
        } else {
            glNamedBufferSubData(terrainNodePoolSSBO, 0, terrain->nodePool.maxSize * terrain->nodePool.unitSize,
                                 terrain->nodePool.memory);
        }
    }

    // Doing the actual render
    glUseProgram(svo_tracer_shader);

    // Binding the SVO
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, terrainChunkPoolSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, terrainNodePoolSSBO);


    // Binding the uniforms
    gllib_bindTexture(&svoTexture, 0, GL_WRITE_ONLY);

    glUniform2ui(glGetUniformLocation(svo_tracer_shader, "screenSize"), resX, resY);
    glUniform3ui(glGetUniformLocation(svo_tracer_shader, "terrainSize"), terrain->width, terrain->height, terrain->width);
    glUniform3f(glGetUniformLocation(svo_tracer_shader, "camPos"), camera_pos.x, camera_pos.y, camera_pos.z);
    glUniformMatrix4fv(glGetUniformLocation(svo_tracer_shader, "viewMat"), 1, GL_FALSE, viewMat.arr);
    glUniformMatrix4fv(glGetUniformLocation(svo_tracer_shader, "projMat"), 1, GL_FALSE, projMat.arr);

    // Dispatching the compute-shader and pushing the result to the framebuffer
    glDispatchCompute(ceilf(resX / 8.0f), ceilf(resY / 8.0f), 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glBlitNamedFramebuffer(svoTexture, 0,
                           0, 0, resX, resY,
                           0, 0, resX, resY,
                           GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

static void render_framebuffer_size_callback(GLFWwindow *_window, int width, int height){

}