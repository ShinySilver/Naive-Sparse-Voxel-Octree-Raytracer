#pragma once

#include <stdio.h>
#include <stdlib.h>

#define INFO(message, ...) printf("[INFO] %s:%d ", __FILE__, __LINE__); printf(message, ##__VA_ARGS__); printf("\n")
/*static inline void info(const char *message, const char *file, int line) {
    printf("[INFO] %s:%d %s\n", file, line, message);
}*/

#define WARN(message) warn(message, __FILE__, __LINE__)
static inline void warn(const char *message, const char *file, int line) {
    printf("[WARN] %s:%d %s\n", file, line, message);
}

#define ERROR(message) error(message, __FILE__, __LINE__)
static inline void error(const char *message, const char *file, int line) {
    printf("[ERROR] %s:%d %s\n", file, line, message);
}

#define FATAL(message) fatal(message, __FILE__, __LINE__)
static inline void fatal(const char *message, const char *file, int line) {
    printf("[FATAL] %s:%d %s\n", file, line, message);
    exit(1);
}