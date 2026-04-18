cmake_minimum_required(VERSION 3.20)
project(MistEngine VERSION 0.5.0 LANGUAGES CXX C)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(MIST_ENABLE_AUDIO "Enable audio system (requires miniaudio)" OFF)
option(MIST_ENABLE_TESTS "Build the Catch2 test suite"                      ON)
option(MIST_ASAN         "Enable AddressSanitizer + UBSan (GCC/Clang only)" OFF)
# G7 editor/runtime split. Editor ON ships UIManager + ImGui dock layout +
# AssetBrowser; OFF produces a headless runtime that loads a scene and runs
# without the editor chrome. Individual #if MIST_EDITOR guards land
# incrementally as subsystems are verified to work without the editor.
option(MIST_BUILD_EDITOR "Compile editor UI (UIManager, AssetBrowser, …)"   ON)
# G10 concrete: Lua 5.4 + sol2 via FetchContent. Gated so a minimal
# headless runtime build can skip the scripting VM entirely.
option(MIST_ENABLE_SCRIPTING "Compile Lua scripting backend (Lua 5.4 + sol2)" ON)

# ---------------------------------------------------------------------------
# Required dependencies
# ---------------------------------------------------------------------------
find_package(OpenGL REQUIRED)
find_package(assimp REQUIRED)
find_package(Bullet REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Freetype REQUIRED)

# nlohmann/json via FetchContent — used by SceneSerializer and any future
# save/load path. Promoted to top-level after the AI subsystem removal;
# previously gated behind MIST_ENABLE_AI which was a source-coupling
# accident.
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

# ImGui — docking branch. Replaces the previously-vendored master
# branch tree at include/imgui.h + src/imgui*.cpp. Docking enables
# DockSpaceOverViewport + ConfigFlags_DockingEnable + multi-viewport.
# Pinned to a fixed SHA for reproducible builds (upstream branch is a
# moving target by design). The docking branch is API-compatible
# with ImGuizmo's 1.89+ target.
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        329c5a6b3be75ebf54506d3ae94b836ffcf19fa0
)
FetchContent_MakeAvailable(imgui)

# ImGuizmo — viewport transform gizmos (translate/rotate/scale handles).
# Not a CMake project; we fetch the sources and add ImGuizmo.cpp to the
# main lib directly below. Uses ImGui headers pulled above.
FetchContent_Declare(
    imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
    # Pinned to master HEAD at plan time — upstream ships only 1.83 as
    # a tag but master tracks ImGui 1.89+ API changes we need. Pinned
    # SHA keeps builds reproducible.
    GIT_TAG        ba662b119d64f9ab700bb2cd7b2781f9044f5565
)
FetchContent_MakeAvailable(imguizmo)

if(MIST_ENABLE_SCRIPTING)
    # Lua 5.4 via walterschell's CMake fork — upstream Lua only ships a
    # Makefile, this fork is a thin CMakeLists that builds the same sources
    # into a static target called `lua_static`.
    include(FetchContent)
    FetchContent_Declare(
        lua_cmake
        GIT_REPOSITORY https://github.com/walterschell/Lua.git
        GIT_TAG        v5.4.5
    )
    FetchContent_MakeAvailable(lua_cmake)

    # sol2 — header-only Lua C++ wrapper. Latest stable tag.
    FetchContent_Declare(
        sol2
        GIT_REPOSITORY https://github.com/ThePhD/sol2.git
        GIT_TAG        v3.3.1
    )
    FetchContent_MakeAvailable(sol2)
endif()

# ---------------------------------------------------------------------------
# Source collection
# ---------------------------------------------------------------------------
file(GLOB_RECURSE LIB_SOURCES
    "src/*.cpp"
    "src/*.c"
)
file(GLOB_RECURSE LIB_HEADERS
    "include/*.h"
)

# Remove main entry point from library sources so we can link it into tests too.
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*MistEngine\\.cpp$")

# Gate Lua-dependent sources on MIST_ENABLE_SCRIPTING. The
# ScriptRegistry / IScriptLanguage contract stays compiled in either
# case — only the concrete Lua backend files come in/out.
if(NOT MIST_ENABLE_SCRIPTING)
    list(FILTER LIB_SOURCES EXCLUDE REGEX ".*/src/Script/Lua.*")
endif()

add_library(MistEngineLib STATIC ${LIB_SOURCES} ${LIB_HEADERS}
    # ImGui docking-branch sources, vendored via FetchContent.
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
)
target_include_directories(MistEngineLib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imguizmo_SOURCE_DIR})

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
    Freetype::Freetype
    ${CMAKE_DL_LIBS}
)

# pthread + stdc++fs are Linux-specific. On MSVC they're part of the runtime.
if(NOT WIN32)
    target_link_libraries(MistEngineLib PUBLIC pthread)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(MistEngineLib PUBLIC stdc++fs)
endif()

target_link_libraries(MistEngineLib PUBLIC nlohmann_json::nlohmann_json)

if(MIST_ENABLE_AUDIO)
    target_compile_definitions(MistEngineLib PUBLIC MIST_ENABLE_AUDIO=1)
endif()

if(MIST_BUILD_EDITOR)
    target_compile_definitions(MistEngineLib PUBLIC MIST_EDITOR=1)
endif()

if(MIST_ENABLE_SCRIPTING)
    target_link_libraries(MistEngineLib PUBLIC lua_static sol2::sol2)
    target_compile_definitions(MistEngineLib PUBLIC MIST_ENABLE_SCRIPTING=1)
endif()

# ---------------------------------------------------------------------------
# Warnings + sanitizers
# ---------------------------------------------------------------------------
if(MSVC)
    target_compile_options(MistEngineLib PRIVATE /W3)
else()
    target_compile_options(MistEngineLib PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

target_compile_definitions(MistEngineLib PUBLIC
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
    target_compile_options(MistEngineLib PRIVATE -g -O0)
elseif(NOT MSVC)
    target_compile_options(MistEngineLib PRIVATE -O2)
endif()

if(MIST_ASAN AND NOT MSVC)
    target_compile_options(MistEngineLib PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(MistEngineLib    PUBLIC -fsanitize=address,undefined)
endif()

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
add_executable(MistEngine src/MistEngine.cpp)
target_link_libraries(MistEngine PRIVATE MistEngineLib)
target_compile_features(MistEngine PRIVATE cxx_std_17)

# Copy runtime assets next to the executable.
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/models  DESTINATION ${CMAKE_BINARY_DIR})
if(EXISTS ${CMAKE_SOURCE_DIR}/textures)
    file(COPY ${CMAKE_SOURCE_DIR}/textures DESTINATION ${CMAKE_BINARY_DIR})
endif()
if(EXISTS ${CMAKE_SOURCE_DIR}/scripts)
    file(COPY ${CMAKE_SOURCE_DIR}/scripts DESTINATION ${CMAKE_BINARY_DIR})
endif()

# ExampleModule shared library (optional).
if(EXISTS ${CMAKE_SOURCE_DIR}/modules/ExampleModule/CMakeLists.txt)
    add_subdirectory(modules/ExampleModule)
endif()

# ---------------------------------------------------------------------------
# Tests (Catch2 via FetchContent so contributors don't need a system install)
# ---------------------------------------------------------------------------
if(MIST_ENABLE_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.3
    )
    FetchContent_MakeAvailable(Catch2)
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS MistEngine RUNTIME DESTINATION bin)
install(DIRECTORY shaders/ DESTINATION share/MistEngine/shaders)
install(DIRECTORY models/  DESTINATION share/MistEngine/models)
