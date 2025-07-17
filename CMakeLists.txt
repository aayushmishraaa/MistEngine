cmake_minimum_required(VERSION 3.16)
project(MistEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set vcpkg toolchain if available
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

# Find packages
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/MistEngine.cpp
    src/Renderer.cpp
    src/Camera.cpp
    src/Shader.cpp
    src/Texture.cpp
    src/Mesh.cpp
    src/Model.cpp
    src/Orb.cpp
    src/Scene.cpp
    src/PhysicsSystem.cpp
    src/ShapeGenerator.cpp
    src/glad.c
    src/ECS/Systems/RenderSystem.cpp
    src/ECS/Systems/ECSPhysicsSystem.cpp
    src/UI/EditorUI.cpp
)

# Header files
set(HEADERS
    include/Renderer.h
    include/Camera.h
    include/Shader.h
    include/Texture.h
    include/Mesh.h
    include/Model.h
    include/Orb.h
    include/Scene.h
    include/PhysicsSystem.h
    include/ShapeGenerator.h
    include/Renderable.h
    include/ECS/Entity.h
    include/ECS/Component.h
    include/ECS/ComponentArray.h
    include/ECS/ComponentManager.h
    include/ECS/EntityManager.h
    include/ECS/System.h
    include/ECS/SystemManager.h
    include/ECS/Coordinator.h
    include/ECS/Components/TransformComponent.h
    include/ECS/Components/RenderComponent.h
    include/ECS/Components/PhysicsComponent.h
    include/ECS/Systems/RenderSystem.h
    include/ECS/Systems/ECSPhysicsSystem.h
    include/UI/EditorUI.h
)

# Create executable
add_executable(MistEngine ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(MistEngine PRIVATE
    glfw
    glad::glad
    glm::glm
    ${BULLET_LIBRARIES}
    assimp::assimp
    imgui::imgui
)

# Additional includes for Bullet
target_include_directories(MistEngine PRIVATE ${BULLET_INCLUDE_DIRS})

# Copy shaders and assets
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/models DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/textures DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Set working directory for Visual Studio
set_target_properties(MistEngine PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)