#pragma once

#include <stdio.h>
#include <stdlib.h>

#define INFO(_message, ...) {printf("[INFO] %s:%d ", __FILE__, __LINE__); printf(_message, ##__VA_ARGS__); printf("\n");}
#define WARN(_message, ...) {printf("[WARN] %s:%d ", __FILE__, __LINE__); printf(_message, ##__VA_ARGS__); printf("\n");}
#define ERROR(_message, ...) {fprintf(stderr, "[ERROR] %s:%d ", __FILE__, __LINE__); fprintf(stderr, _message, ##__VA_ARGS__); fprintf(stderr, "\n");}
#define FATAL(_message, ...) {fprintf(stderr, "[FATAL] %s:%d ", __FILE__, __LINE__); fprintf(stderr, _message, ##__VA_ARGS__); fprintf(stderr, "\n"); exit(0);}