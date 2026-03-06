cmake_minimum_required(VERSION 3.20)
project(MistEngine VERSION 0.4.0 LANGUAGES CXX C)

# Options
option(MIST_ENABLE_AUDIO "Enable audio system (requires miniaudio)" OFF)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(assimp REQUIRED)
find_package(Bullet REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(CURL REQUIRED)
find_package(Freetype REQUIRED)

# Collect source files
file(GLOB_RECURSE LIB_SOURCES
    "src/*.cpp"
    "src/*.c"
)
file(GLOB_RECURSE LIB_HEADERS
    "include/*.h"
)

# Remove main entry point from library sources
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*MistEngine\\.cpp$")

# Static library target
add_library(MistEngineLib STATIC ${LIB_SOURCES} ${LIB_HEADERS})

target_compile_features(MistEngineLib PUBLIC cxx_std_17)
set_target_properties(MistEngineLib PROPERTIES CXX_EXTENSIONS OFF)

target_include_directories(MistEngineLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${ASSIMP_INCLUDE_DIRS}
    ${BULLET_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(MistEngineLib PUBLIC
    OpenGL::GL
    ${ASSIMP_LIBRARIES}
    ${BULLET_LIBRARIES}
    glfw
    CURL::libcurl
    Freetype::Freetype
    ${CMAKE_DL_LIBS}
    pthread
    stdc++fs
)

# Audio support
if(MIST_ENABLE_AUDIO)
    target_compile_definitions(MistEngineLib PUBLIC MIST_ENABLE_AUDIO=1)
endif()

# Debug/Release configs
target_compile_definitions(MistEngineLib PUBLIC
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(MistEngineLib PRIVATE -g -O0 -Wall -Wextra)
else()
    target_compile_options(MistEngineLib PRIVATE -O2)
endif()

# Executable target
add_executable(MistEngine src/MistEngine.cpp)
target_link_libraries(MistEngine PRIVATE MistEngineLib)
target_compile_features(MistEngine PRIVATE cxx_std_17)

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

# Install targets
install(TARGETS MistEngine RUNTIME DESTINATION bin)
install(DIRECTORY shaders/ DESTINATION share/MistEngine/shaders)
install(DIRECTORY models/ DESTINATION share/MistEngine/models)
