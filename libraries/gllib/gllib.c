#include "glad/glad.h"
#include <stdlib.h>
#include "gllib.h"
#include "cplog.h"
#define STB_INCLUDE_IMPLEMENTATION
#define STB_INCLUDE_LINE_NONE
#include "stb_include.h"
#include "cpmath.h"
#include "common/log.h"

static u32 makeShader(const char* path, GLenum shaderType);

u32 gllib_makePipeline(const char *vertPath, const char *fragPath)
{
    u32 vertShader = makeShader(vertPath, GL_VERTEX_SHADER);
    u32 fragShader = makeShader(fragPath, GL_FRAGMENT_SHADER);

    u32 shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertShader);
    glAttachShader(shaderProgram, fragShader);
    glLinkProgram(shaderProgram);

    GLint isLinked = 0;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE)
    {
    	GLint maxLength = 0;
    	glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &maxLength);
    	char errorLog[maxLength];
	    glGetProgramInfoLog(shaderProgram, maxLength, &maxLength, errorLog);
    	glDeleteProgram(shaderProgram);
        FATAL("ERROR COMPILING SHADER: %s", errorLog);
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    return shaderProgram;
}

u32 gllib_makeCompute(const char *shaderPath)
{
    u32 shader = makeShader(shaderPath, GL_COMPUTE_SHADER);

    u32 shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, shader);
    glLinkProgram(shaderProgram);

    GLint isLinked = 0;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &maxLength);
        char errorLog[maxLength];
        glGetProgramInfoLog(shaderProgram, maxLength, &maxLength, errorLog);
        glDeleteProgram(shaderProgram);
        FATAL("ERROR COMPILING SHADER: %s", errorLog);
    }

    glDeleteShader(shader);

    return shaderProgram;
}

Texture *gllib_makeDefaultTexture(u32 width, u32 height, u32 glInternalFormat, u32 glFilter)
{
    Texture *out = (Texture *)malloc(sizeof(Texture));
    if(!out) FATAL("Could not allocate memory.");
    memset(out, 0, sizeof(Texture));
    out->internalFormat = glInternalFormat;
    glCreateTextures(GL_TEXTURE_2D, 1, &out->handle);
    glTextureParameteri(out->handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(out->handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (glFilter)
    {
        glTextureParameteri(out->handle, GL_TEXTURE_MAG_FILTER, glFilter);
        glTextureParameteri(out->handle, GL_TEXTURE_MIN_FILTER, glFilter);
    }
    glTextureStorage2D(out->handle, 1, glInternalFormat, width, height);
    return out;
}

void gllib_destroyTexture(Texture *texture)
{
    glDeleteTextures(1, &texture->handle);
    free(texture);
}

void gllib_bindTexture(const Texture *texture, u32 idx, u32 glUsage)
{
    glBindImageTexture(idx, texture->handle, 0, GL_FALSE, 0, glUsage, texture->internalFormat);
}

static u32 makeShader(const char* path, GLenum shaderType)
{
    char error[256];
    char* code = stb_include_file((char*) path, (char*) "", (char*) "res/shaders/inc", error);
    if(!code)
    {
        FATAL("Error Parsing Shader: %s", error);
    }

    u32 shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, (const GLchar* const*) &code, NULL);
    glCompileShader(shader);

    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if(isCompiled == GL_FALSE)
    {
    	GLint maxLength = 0;
    	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
    	char errorLog[maxLength];
    	glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog);
    	glDeleteShader(shader); // Don't leak the shader.
        FATAL("ERROR COMPILING SHADER: %s", errorLog);
    }

    free(code);
    return shader;
}

void APIENTRY gllib_debug_callback(GLenum source, GLenum type, unsigned int id, GLenum severity,
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