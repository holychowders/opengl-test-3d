cmake_minimum_required(VERSION 3.20)
project(gl3d LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(gl3d src/main.cpp)
target_include_directories(gl3d PRIVATE src)
target_compile_options(gl3d PRIVATE
    -pedantic -O0 -g -fvisibility=hidden
    -Wall -Wextra -Weverything
    -Wvla -Werror=vla -Wno-padded -Wno-old-style-cast -Wno-unsafe-buffer-usage $<$<COMPILE_LANGUAGE:CXX>:-Wno-zero-as-null-pointer-constant>
    -Wno-gnu-anonymous-struct -Wno-nested-anon-types
    -Wno-missing-braces
    -Wno-declaration-after-statement -Wno-unreachable-code-break -Wno-c++98-compat -Wno-zero-as-null-pointer-constant #-Wno-pre-c++20-compat-pedantic 
    -Wno-unused-parameter -Wno-unused-function -Wno-writable-strings -Wno-unsafe-buffer-usage-in-libc-call
    -Wno-shadow-field-in-constructor
)

target_compile_definitions(gl3d PRIVATE GLEW_STATIC _CRT_SECURE_NO_WARNINGS)
target_include_directories(gl3d SYSTEM PRIVATE vendor/stb-master vendor/imgui-1.92.7-src vendor/glm-1.0.2)
target_link_directories(gl3d PRIVATE vendor/glfw-3.4.bin.WIN64/lib-vc2022)
target_link_libraries(gl3d PRIVATE imgui glew glfw3dll opengl32)

# IMGUI
add_library(imgui STATIC
    vendor/imgui-1.92.7-src/imgui.cpp
    vendor/imgui-1.92.7-src/imgui_draw.cpp
    vendor/imgui-1.92.7-src/imgui_tables.cpp
    vendor/imgui-1.92.7-src/imgui_widgets.cpp
    vendor/imgui-1.92.7-src/imgui_demo.cpp
    vendor/imgui-1.92.7-src/backends/imgui_impl_opengl3.cpp
    vendor/imgui-1.92.7-src/backends/imgui_impl_glfw.cpp
)
target_include_directories(imgui SYSTEM PUBLIC vendor/imgui-1.92.7-src vendor/imgui-1.92.7-src/backends vendor/glfw-3.4.bin.WIN64/include)
target_compile_definitions(imgui PUBLIC GLEW_STATIC)

# GLEW
add_library(glew STATIC vendor/glew-2.2.0-src-release/src/glew.c)
target_include_directories(glew SYSTEM PUBLIC vendor/glew-2.2.0-src-release/include)
target_compile_definitions(glew PUBLIC GLEW_STATIC)
target_link_libraries(glew PUBLIC opengl32)

# Copy glfw3.dll to build dir at configure time
file(COPY "vendor/glfw-3.4.bin.WIN64/lib-vc2022/glfw3.dll"
     DESTINATION "${CMAKE_BINARY_DIR}")

if(WIN32)
    target_compile_definitions(gl3d PRIVATE
        _CRT_SECURE_NO_WARNINGS  # suppress MSVC CRT deprecation warnings for standard C functions
        WIN32_LEAN_AND_MEAN      # exclude rarely-used APIs from windows.h
        NOMINMAX                 # prevent windows.h from defining min/max macros
        UNICODE _UNICODE         # use Unicode versions of Windows API functions
    )
endif()


