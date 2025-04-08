cmake_minimum_required(VERSION 3.10)
project(MistEngine)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(assimp REQUIRED)

# Add source files
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*.c"
)

# Add header files
file(GLOB_RECURSE HEADERS
    "include/*.h"
    "include/*.hpp"
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPENGL_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${ASSIMP_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${OPENGL_LIBRARIES}
    glfw
    assimp
)

# Copy shader files to build directory
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})

# Copy texture files to build directory
file(COPY ${CMAKE_SOURCE_DIR}/textures DESTINATION ${CMAKE_BINARY_DIR})

# Copy model files to build directory
file(COPY ${CMAKE_SOURCE_DIR}/models DESTINATION ${CMAKE_BINARY_DIR})