cmake_minimum_required(VERSION 3.10)
project(METRID)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Prefer modern GLVND OpenGL (silences CMP0072 warning)
if(POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
endif()
set(OpenGL_GL_PREFERENCE GLVND)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(${PROJECT_NAME}
        src/main.c
        src/glad.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        OpenGL::GL
        glfw
        m
)

# Put the binary in a clean place
set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)