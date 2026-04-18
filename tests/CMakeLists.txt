# Catch2-based test suite. These tests are designed to run headless: no GL
# context, no window, no network. Tests that need a live GL context live
# behind MIST_TEST_GL which stays OFF by default (enabled in CI on runners
# with a software GL implementation).

option(MIST_TEST_GL "Build tests that require a live OpenGL context" OFF)

add_executable(MistEngineTests
    test_main.cpp
    test_ecs.cpp
    test_command_queue.cpp
    test_editor_plugin.cpp
    test_fixed_timestep.cpp
    test_hierarchy.cpp
    test_importer.cpp
    test_lua_script.cpp
    test_path_guard.cpp
    test_reflection.cpp
    test_resource_cache.cpp
    test_rid.cpp
    test_renderingdevice.cpp
    test_scene_serializer.cpp
    test_script_registry.cpp
    test_asset_drop.cpp
    test_shortcut_registry.cpp
    test_signal.cpp
    test_undo_integration.cpp
    test_undo_stack.cpp
    test_audio_clip.cpp
    test_version.cpp
)

target_link_libraries(MistEngineTests PRIVATE
    MistEngineLib
    Catch2::Catch2WithMain
)

target_compile_features(MistEngineTests PRIVATE cxx_std_17)

# Hand off to CTest.
include(Catch)
catch_discover_tests(MistEngineTests)
