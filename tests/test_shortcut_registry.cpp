#include <catch2/catch_all.hpp>

#include "Editor/ShortcutRegistry.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

using Mist::Editor::Shortcut;
using Mist::Editor::ShortcutRegistry;

namespace {

// Build a disposable registry instance by manipulating the singleton.
// Tests mutate shared state, so each test re-registers fresh shortcuts
// under distinct ids to avoid leaking effects across cases.
ShortcutRegistry& clean() {
    return ShortcutRegistry::Instance();
}

} // namespace

TEST_CASE("ShortcutRegistry stores and retrieves by id", "[shortcut]") {
    auto& reg = clean();
    reg.Register({"test/a", "A", GLFW_KEY_A, GLFW_MOD_CONTROL});
    const Shortcut* s = reg.Find("test/a");
    REQUIRE(s != nullptr);
    REQUIRE(s->label == "A");
    REQUIRE(s->key == GLFW_KEY_A);
    REQUIRE(s->mods == GLFW_MOD_CONTROL);
}

TEST_CASE("ShortcutRegistry re-registration overwrites (last writer wins)", "[shortcut]") {
    auto& reg = clean();
    reg.Register({"test/rebind", "Orig", GLFW_KEY_Z, GLFW_MOD_CONTROL});
    reg.Register({"test/rebind", "New",  GLFW_KEY_Y, GLFW_MOD_CONTROL});

    const Shortcut* s = reg.Find("test/rebind");
    REQUIRE(s != nullptr);
    REQUIRE(s->label == "New");
    REQUIRE(s->key == GLFW_KEY_Y);
}

TEST_CASE("ShortcutRegistry Matches is strict on mods", "[shortcut]") {
    auto& reg = clean();
    reg.Register({"test/strict", "S", GLFW_KEY_S, GLFW_MOD_CONTROL});

    // Exact match fires.
    REQUIRE(reg.Matches("test/strict", GLFW_KEY_S, GLFW_MOD_CONTROL));
    // Ctrl+Shift+S must NOT fire a Ctrl+S binding — strictness is what
    // prevents Save-As (Ctrl+Shift+S) from also triggering Save.
    REQUIRE_FALSE(reg.Matches("test/strict",
                              GLFW_KEY_S,
                              GLFW_MOD_CONTROL | GLFW_MOD_SHIFT));
    // Different key is obvious no.
    REQUIRE_FALSE(reg.Matches("test/strict", GLFW_KEY_A, GLFW_MOD_CONTROL));
}

TEST_CASE("ShortcutRegistry FindByChord returns the matching id", "[shortcut]") {
    auto& reg = clean();
    reg.Register({"test/chord", "C", GLFW_KEY_X, GLFW_MOD_ALT});
    const Shortcut* hit = reg.FindByChord(GLFW_KEY_X, GLFW_MOD_ALT);
    REQUIRE(hit != nullptr);
    REQUIRE(hit->id == "test/chord");

    REQUIRE(reg.FindByChord(GLFW_KEY_X, 0) == nullptr);
}

TEST_CASE("Shortcut::DisplayText formats chord labels", "[shortcut]") {
    Shortcut s1{"x", "x", GLFW_KEY_S, GLFW_MOD_CONTROL};
    REQUIRE(s1.DisplayText() == "Ctrl+S");

    Shortcut s2{"x", "x", GLFW_KEY_S, GLFW_MOD_CONTROL | GLFW_MOD_SHIFT};
    REQUIRE(s2.DisplayText() == "Ctrl+Shift+S");

    Shortcut s3{"x", "x", GLFW_KEY_F5, 0};
    REQUIRE(s3.DisplayText() == "F5");
}
