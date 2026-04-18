#include <catch2/catch_all.hpp>

#include "Editor/EditorPlugin.h"

#include <memory>
#include <string>

namespace {
struct DemoPlugin : Mist::Editor::IEditorPlugin {
    int enables  = 0;
    int disables = 0;
    std::string GetName() const override { return "Demo"; }

    void OnEnable(Mist::Editor::EditorContext& ctx) override {
        ++enables;
        ctx.RegisterDock("DemoDock", [] { /* ImGui calls go here */ });
        ctx.RegisterMenuItem("Tools/Demo", [] {});
    }
    void OnDisable(Mist::Editor::EditorContext&) override {
        ++disables;
    }
};
} // namespace

TEST_CASE("EditorPluginRegistry enables plugins and exposes their docks", "[editor_plugin]") {
    auto plugin = std::make_shared<DemoPlugin>();
    Mist::Editor::EditorPluginRegistry::Instance().Register(plugin);

    Mist::Editor::EditorPluginRegistry::Instance().EnableAll();
    REQUIRE(plugin->enables == 1);

    auto docks = Mist::Editor::EditorPluginRegistry::Instance().SnapshotDocks();
    auto menus = Mist::Editor::EditorPluginRegistry::Instance().SnapshotMenus();

    bool foundDock = false;
    for (const auto& d : docks) if (d.title == "DemoDock") foundDock = true;
    REQUIRE(foundDock);

    bool foundMenu = false;
    for (const auto& m : menus) if (m.path == "Tools/Demo") foundMenu = true;
    REQUIRE(foundMenu);

    Mist::Editor::EditorPluginRegistry::Instance().DisableAll();
    REQUIRE(plugin->disables == 1);

    // After disabling, snapshots should no longer surface the plugin's
    // registrations — the registry filters by enabled state.
    auto docksAfter = Mist::Editor::EditorPluginRegistry::Instance().SnapshotDocks();
    for (const auto& d : docksAfter) REQUIRE(d.title != "DemoDock");
}
