#include "Editor/BuiltinPlugins.h"
#include "Editor/EditorPlugin.h"

#include "ECS/Components/AnimationComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/LightComponent.h"
#include "ECS/Components/NameComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Components/PrefabComponents.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Coordinator.h"
#include "Core/Logger.h"

#include <imgui.h>

#include <memory>
#include <string>

extern Coordinator gCoordinator;

namespace Mist::Editor::Plugins {

// The first real in-tree editor plugin.
//
// EditorPluginRegistry has been complete since it was written — OnEnable /
// OnDisable, dock and menu registration, thread-safe snapshots — and its only
// caller was tests/test_editor_plugin.cpp. A registry with no plugins and a
// UIManager that never iterated it meant the extension seam could not be shown
// to work, only asserted to compile.
//
// So this exists to exercise the seam end to end rather than to be
// indispensable: it registers one dock and one menu item, and it deliberately
// uses nothing but the public plugin API. If a contributor wants to know what a
// plugin looks like, this is the smallest honest answer.
class SceneStatsPlugin : public IEditorPlugin {
public:
    std::string GetName() const override { return "Scene Stats"; }

    void OnEnable(EditorContext& ctx) override {
        ctx.RegisterDock("Scene Stats", [] { Draw(); });
        ctx.RegisterMenuItem("Plugins/Log Scene Stats", [] {
            LOG_INFO("Scene stats: ", gCoordinator.GetLivingEntities().size(), " entities");
        });
    }

private:
    static void Draw() {
        const auto& living = gCoordinator.GetLivingEntities();

        int named = 0, renderable = 0, lights = 0, physics = 0;
        int cameras = 0, animated = 0, prefabInstances = 0, prefabMembers = 0;

        for (Entity e : living) {
            if (gCoordinator.HasComponent<NameComponent>(e))            ++named;
            if (gCoordinator.HasComponent<RenderComponent>(e))          ++renderable;
            if (gCoordinator.HasComponent<LightComponent>(e))           ++lights;
            if (gCoordinator.HasComponent<PhysicsComponent>(e))         ++physics;
            if (gCoordinator.HasComponent<CameraComponent>(e))          ++cameras;
            if (gCoordinator.HasComponent<AnimationComponent>(e))       ++animated;
            if (gCoordinator.HasComponent<PrefabInstanceComponent>(e))  ++prefabInstances;
            if (gCoordinator.HasComponent<PrefabMemberComponent>(e))    ++prefabMembers;
        }

        ImGui::Text("Entities:        %zu", living.size());
        ImGui::Separator();
        ImGui::Text("Named:           %d", named);
        ImGui::Text("Renderable:      %d", renderable);
        ImGui::Text("Lights:          %d", lights);
        ImGui::Text("Physics bodies:  %d", physics);
        ImGui::Text("Cameras:         %d", cameras);
        ImGui::Text("Animated:        %d", animated);
        ImGui::Separator();
        ImGui::Text("Prefab instances: %d", prefabInstances);
        ImGui::Text("Prefab members:   %d", prefabMembers);
        if (prefabInstances > 0) {
            ImGui::TextDisabled("Members are stored by reference, not in the scene file.");
        }
    }
};

void RegisterBuiltinPlugins() {
    EditorPluginRegistry::Instance().Register(std::make_shared<SceneStatsPlugin>());
}

} // namespace Mist::Editor::Plugins
