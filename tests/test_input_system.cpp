#include <catch2/catch_all.hpp>

#include "Input/InputContext.h"
#include "Input/InputMapSerializer.h"

#include <filesystem>
#include <fstream>
#include <string>

// InputSystem's action map.
//
// This subsystem had NO test at all before, not just no callers — it was
// written, shipped, and never executed. The action-query paths need a live
// GLFWwindow, so what is covered here is everything that does not: the context
// and binding model, and the binding-table persistence that makes
// RebindAction's effects outlive the process.

TEST_CASE("The built-in editor context binds the camera-fly actions",
          "[input]") {
    const InputContextMap ctx = CreateEditorContext();
    REQUIRE(ctx.name == "Editor");

    for (const char* action : {"MoveForward", "MoveBackward", "MoveLeft",
                               "MoveRight", "MoveUp", "MoveDown"}) {
        INFO("action " << action);
        REQUIRE(ctx.actions.count(action) == 1);
        REQUIRE_FALSE(ctx.actions.at(action).bindings.empty());
    }

    // The keys behind the W/E collision. Both are camera bindings AND gizmo
    // chords, which is why viewport navigation had to become modal (the fly
    // context is only pushed while RMB is held).
    REQUIRE(ctx.actions.at("MoveForward").bindings[0].code == 87);  // W
    REQUIRE(ctx.actions.at("MoveUp").bindings[0].code     == 69);  // E
}

TEST_CASE("The gameplay context supports multiple bindings per action",
          "[input]") {
    // Keyboard plus gamepad on the same action is the feature that made this
    // system worth wiring over InputManager's hardcoded glfwGetKey calls.
    const InputContextMap ctx = CreateGameplayContext();
    REQUIRE(ctx.name == "Gameplay");
    REQUIRE(ctx.actions.count("MoveForward") == 1);
    REQUIRE(ctx.actions.at("MoveForward").bindings.size() >= 2);

    bool sawKeyboard = false, sawGamepad = false;
    for (const auto& b : ctx.actions.at("MoveForward").bindings) {
        if (b.device == InputDevice::Keyboard)                              sawKeyboard = true;
        if (b.device == InputDevice::GamepadButton
            || b.device == InputDevice::GamepadAxis)                        sawGamepad  = true;
    }
    REQUIRE(sawKeyboard);
    REQUIRE(sawGamepad);
}

TEST_CASE("AddAction replaces an existing action by name", "[input]") {
    // This is the mechanism behind both RebindAction and the saved-map merge.
    InputContextMap ctx = CreateEditorContext();
    REQUIRE(ctx.actions.at("MoveForward").bindings[0].code == 87);

    ctx.AddAction({"MoveForward", InputActionType::Button,
                   {{InputDevice::Keyboard, 38 /* up arrow */}}});

    REQUIRE(ctx.actions.at("MoveForward").bindings.size() == 1);
    REQUIRE(ctx.actions.at("MoveForward").bindings[0].code == 38);
}

TEST_CASE("FindAction returns null for an unknown action", "[input]") {
    InputContextMap ctx = CreateEditorContext();
    REQUIRE(ctx.FindAction("MoveForward") != nullptr);
    REQUIRE(ctx.FindAction("NoSuchAction") == nullptr);
}

namespace {
struct ScopedFile {
    std::filesystem::path path;
    explicit ScopedFile(const std::string& rel)
        : path(std::filesystem::current_path() / rel) {}
    ~ScopedFile() { std::error_code ec; std::filesystem::remove(path, ec); }
    std::string rel() const {
        return std::filesystem::relative(path, std::filesystem::current_path()).generic_string();
    }
};
} // namespace

TEST_CASE("An input map round-trips through a file", "[input][serializer]") {
    ScopedFile file("test_input_map.json");

    InputContextMap saved = CreateEditorContext();
    saved.AddAction({"MoveForward", InputActionType::Button,
                     {{InputDevice::Keyboard, 265 /* up arrow */}}});
    REQUIRE(Mist::Input::SaveInputMap(saved, file.rel()));

    // Start from defaults and merge the file over them.
    InputContextMap loaded = CreateEditorContext();
    REQUIRE(loaded.actions.at("MoveForward").bindings[0].code == 87);
    REQUIRE(Mist::Input::LoadInputMap(loaded, file.rel()));
    REQUIRE(loaded.actions.at("MoveForward").bindings[0].code == 265);
}

TEST_CASE("Loading an input map merges rather than replaces", "[input][serializer]") {
    // Version skew: a map saved by an older build must not leave actions the
    // new build added unbound.
    ScopedFile file("test_input_map_partial.json");

    InputContextMap partial;
    partial.name = "Editor";
    partial.AddAction({"MoveForward", InputActionType::Button,
                       {{InputDevice::Keyboard, 265}}});
    REQUIRE(Mist::Input::SaveInputMap(partial, file.rel()));

    InputContextMap loaded = CreateEditorContext();
    REQUIRE(Mist::Input::LoadInputMap(loaded, file.rel()));

    REQUIRE(loaded.actions.at("MoveForward").bindings[0].code == 265);  // from the file
    REQUIRE(loaded.actions.count("MoveBackward") == 1);                 // kept its default
    REQUIRE(loaded.actions.at("MoveBackward").bindings[0].code == 83);
}

TEST_CASE("A deliberate unbind survives the round trip", "[input][serializer]") {
    // An action stored with zero bindings is a user removing a default, not a
    // corrupt entry — skipping it would make a default impossible to remove.
    ScopedFile file("test_input_map_unbound.json");

    InputContextMap saved = CreateEditorContext();
    saved.AddAction({"MoveUp", InputActionType::Button, {}});
    REQUIRE(Mist::Input::SaveInputMap(saved, file.rel()));

    InputContextMap loaded = CreateEditorContext();
    REQUIRE(Mist::Input::LoadInputMap(loaded, file.rel()));
    REQUIRE(loaded.actions.count("MoveUp") == 1);
    REQUIRE(loaded.actions.at("MoveUp").bindings.empty());
}

TEST_CASE("A missing input map is not an error", "[input][serializer]") {
    // First run: there is no saved map, and the built-in defaults stand.
    InputContextMap ctx = CreateEditorContext();
    REQUIRE_FALSE(Mist::Input::LoadInputMap(ctx, "no_such_input_map.json"));
    REQUIRE(ctx.actions.at("MoveForward").bindings[0].code == 87);
}

TEST_CASE("A malformed input map keeps the built-in bindings", "[input][serializer]") {
    ScopedFile file("test_input_map_broken.json");
    { std::ofstream out(file.path); out << "{ not json"; }

    InputContextMap ctx = CreateEditorContext();
    REQUIRE_FALSE(Mist::Input::LoadInputMap(ctx, file.rel()));
    REQUIRE(ctx.actions.at("MoveForward").bindings[0].code == 87);
}

TEST_CASE("Input map paths are guarded against escaping the project root",
          "[input][serializer][security]") {
    const InputContextMap ctx = CreateEditorContext();
    REQUIRE_FALSE(Mist::Input::SaveInputMap(ctx, "../../escaped_input_map.json"));
    REQUIRE_FALSE(Mist::Input::SaveInputMap(ctx, "/tmp/escaped_input_map.json"));
}
