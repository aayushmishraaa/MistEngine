cmake_minimum_required(VERSION 3.10)
project(ExampleModule)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default visibility to hidden so only MIST_EXPORT symbols are exported
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# Source files
set(MODULE_SOURCES
    src/ExampleModule.cpp
)

# Only include dllmain.cpp on Windows
if(WIN32)
    list(APPEND MODULE_SOURCES dllmain.cpp)
endif()

# Build as shared library
add_library(${PROJECT_NAME} SHARED ${MODULE_SOURCES})

# Include directories — engine headers + module headers
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# Set output directory alongside the main executable
set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/modules
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/modules
)
