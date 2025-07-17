cmake_minimum_required(VERSION 3.15)
project(MistEngine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glad REQUIRED)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)
find_package(Bullet REQUIRED)
find_package(stb REQUIRED)
find_package(imgui REQUIRED)

# Collect all source files
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*.c"
    "src/**/*.cpp"
    "src/**/*.c"
)

# Collect all header files
file(GLOB_RECURSE HEADERS
    "include/*.h"
    "include/*.hpp"
    "include/**/*.h"
    "include/**/*.hpp"
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    glfw
    glad::glad
    glm::glm
    assimp::assimp
    ${BULLET_LIBRARIES}
    imgui::imgui
    stb::stb
)

# Bullet Physics include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${BULLET_INCLUDE_DIRS}
)

# Copy assets to build directory
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/models DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/textures DESTINATION ${CMAKE_BINARY_DIR})

# Set working directory for debugging
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# Enable folders in IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)