#include "render.h"
#include "cpmath.h"
#include "common/terrain.h"
#include "client/camera.h"
#include "gllib.h"
#include "stb_include.h"

static u32 terrainChunkPoolSSBO;
static u32 currentChunkBufferSize = 0;

static u32 terrainNodePoolSSBO;
static u32 currentNodeBufferSize = 0;

static u32 svo_tracer_shader;
static u32 svo_framebuffer;
static Texture *svoTexture = 0;

int render_resolution_x;
int render_resolution_y;

static void render_framebuffer_size_callback(GLFWwindow *_window, int width, int height);

void render_init(GLFWwindow *window) {
    glCreateBuffers(1, &terrainChunkPoolSSBO);
    glCreateBuffers(1, &terrainNodePoolSSBO);
    glCreateFramebuffers(1, &svo_framebuffer);

    svo_tracer_shader = gllib_makeCompute("resources/shaders/compute/svo_tracer.glsl");

    glfwSetFramebufferSizeCallback(window, render_framebuffer_size_callback);
    glfwGetWindowSize(window, &render_resolution_x, &render_resolution_y);
    render_framebuffer_size_callback(window, render_resolution_x, render_resolution_y);
}

void render_terminate(void) {
    glDeleteBuffers(1, &terrainChunkPoolSSBO);
    glDeleteBuffers(1, &terrainNodePoolSSBO);
    glDeleteFramebuffers(1, &svo_framebuffer);

    glDeleteProgram(svo_tracer_shader);
}

void render_draw_frame(Terrain *terrain) {
    // Computing the view and projection matrices
    mat4 view_matrix = worldToCamMatrix(camera_pos, camera_forward, (vec3) {0, 1, 0});
    mat4 projection_matrix = perspectiveProjectionMatrix(radians(70.0f), render_resolution_x / (float) render_resolution_y, 0.01, 1000);

    // If the terrain has not been uploaded yet, or if it has been modified
    if (terrain->dirty) {
        terrain->dirty = false;

        // (Re)Uploading the SVO
        // Naive implementation. Must be improved
        if (terrain->chunkPool.size != currentChunkBufferSize) {
            glNamedBufferData(terrainChunkPoolSSBO, terrain->chunkPool.size * terrain->chunkPool.unitSize,
                              terrain->chunkPool.memory, GL_STATIC_DRAW);
        } else {
            glNamedBufferSubData(terrainChunkPoolSSBO, 0, terrain->chunkPool.size * terrain->chunkPool.unitSize,
                                 terrain->chunkPool.memory);
        }
        if (terrain->nodePool.size != currentNodeBufferSize) {
            glNamedBufferData(terrainNodePoolSSBO, terrain->nodePool.size * terrain->nodePool.unitSize,
                              terrain->nodePool.memory, GL_STATIC_DRAW);
        } else {
            glNamedBufferSubData(terrainNodePoolSSBO, 0, terrain->nodePool.size * terrain->nodePool.unitSize,
                                 terrain->nodePool.memory);
        }
    }

    // Doing the actual render
    glUseProgram(svo_tracer_shader);

    // Binding the SVO
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, terrainNodePoolSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, terrainChunkPoolSSBO);

    // Binding the uniforms
    gllib_bindTexture(svoTexture, 0, GL_WRITE_ONLY);

    glUniform2ui(glGetUniformLocation(svo_tracer_shader, "screenSize"), render_resolution_x, render_resolution_y);
    glUniform3ui(glGetUniformLocation(svo_tracer_shader, "terrainSize"), terrain->width, terrain->width, terrain->width);
    glUniform1ui(glGetUniformLocation(svo_tracer_shader, "treeDepth"), terrain->depth);
    glUniform3f(glGetUniformLocation(svo_tracer_shader, "camPos"), camera_pos.x, camera_pos.y, camera_pos.z);
    glUniformMatrix4fv(glGetUniformLocation(svo_tracer_shader, "viewMat"), 1, GL_FALSE, view_matrix.arr);
    glUniformMatrix4fv(glGetUniformLocation(svo_tracer_shader, "projMat"), 1, GL_FALSE, projection_matrix.arr);

    // Dispatching the compute-shader and pushing the result to the framebuffer
    glDispatchCompute(ceilf(render_resolution_x / 8.0f), ceilf(render_resolution_y / 8.0f), 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glBlitNamedFramebuffer(svo_framebuffer, 0,
                           0, 0, render_resolution_x, render_resolution_y,
                           0, 0, render_resolution_x, render_resolution_y,
                           GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

static void render_framebuffer_size_callback(GLFWwindow *_window, int width, int height) {
    if (svoTexture) gllib_destroyTexture(svoTexture);
    glViewport(0, 0, width, height);
    render_resolution_x = max(1, width);
    render_resolution_y = max(1, height);
    svoTexture = gllib_makeDefaultTexture(render_resolution_x, render_resolution_y, GL_RGBA8, GL_NEAREST);
    glNamedFramebufferTexture(svo_framebuffer, GL_COLOR_ATTACHMENT0, svoTexture->handle, 0);
}