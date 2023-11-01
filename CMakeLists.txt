cmake_minimum_required(VERSION 3.21)
project (iVy C)
set(CMAKE_C_STANDARD 17)

include_directories(src)
#target_precompile_headers(iVy PUBLIC inc/pch.h)
set(dir ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${dir}/../build)

set(CMAKE_C_FLAGS "-std=c17 -mavx2 -mfma")
set(CMAKE_C_FLAGS_RELEASE "-O3 -static")
set(CMAKE_C_FLAGS_DEBUG
        "-g3  -O0 -Wall -Werror -Wshadow -Warray-bounds -Wcast-align=strict -Wstrict-prototypes -Wmissing-prototypes \
         -Wwrite-strings -Winit-self -Wold-style-definition -Wno-missing-braces -Wno-unused-function \
         -Wno-incompatible-pointer-types -Wno-uninitialized -Wno-format-security -Wno-unused-but-set-variable -Wno-unused-variable \
         -Wno-format-zero-length -Wno-unused-parameter -Wno-comment -fsanitize=address -static-libasan")

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c ${CMAKE_CURRENT_SOURCE_DIR}/includes/*.h)
add_executable(iVy ${SRC_FILES})
target_compile_options(iVy PRIVATE -fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}/=)

# IPO / LTO
if(CMAKE_BUILD_TYPE MATCHES RELEASE)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT supported OUTPUT error)
    cmake_policy(SET CMP0069 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
    if( supported )
        message(STATUS "IPO / LTO enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "IPO / LTO not supported: <${error}>")
    endif()
endif()

# fast noise
include_directories(libraries/FastNoise)
set_source_files_properties(/libraries/FastNoise/FastNoiseLite.h PROPERTIES COMPILE_FLAGS -w)

# stb include
include_directories(libraries/stb)
set_source_files_properties(/libraries/stb/stb_include.h PROPERTIES COMPILE_FLAGS -w)

# glad
include_directories(libraries/glad/include)
add_library(glad STATIC libraries/glad/src/glad.c)
target_link_libraries(iVy glad)

# GLFW
include_directories("libraries/glfw-3.3.2/include")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(libraries/glfw-3.3.2)
target_link_libraries(iVy glfw)

# cpmath
include_directories("libraries/cplib/")
set_source_files_properties(/libraries/cplib/cpmath.h PROPERTIES COMPILE_FLAGS -w)
