cmake_minimum_required(VERSION 3.10)
project(MistEngine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(assimp REQUIRED)
find_package(Bullet REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(CURL REQUIRED)
find_package(Freetype REQUIRED)

# Collect all source files recursively
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*.c"
)

# Collect all header files recursively
file(GLOB_RECURSE HEADERS
    "include/*.h"
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${ASSIMP_INCLUDE_DIRS}
    ${BULLET_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    OpenGL::GL
    ${ASSIMP_LIBRARIES}
    ${BULLET_LIBRARIES}
    glfw
    CURL::libcurl
    Freetype::Freetype
    ${CMAKE_DL_LIBS}
)

# Copy shader files to build directory
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})

# Copy model files to build directory
file(COPY ${CMAKE_SOURCE_DIR}/models DESTINATION ${CMAKE_BINARY_DIR})

# Copy texture files to build directory
if(EXISTS ${CMAKE_SOURCE_DIR}/textures)
    file(COPY ${CMAKE_SOURCE_DIR}/textures DESTINATION ${CMAKE_BINARY_DIR})
endif()

# ExampleModule shared library (optional)
if(EXISTS ${CMAKE_SOURCE_DIR}/modules/ExampleModule/CMakeLists.txt)
    add_subdirectory(modules/ExampleModule)
endif()
