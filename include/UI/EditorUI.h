#ifndef EDITORUI_H
#define EDITORUI_H

#include "Config.h"
#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>

// Forward declarations
struct GLFWwindow;
class Coordinator;
class Scene;
class Camera;
class PhysicsSystem;
struct PhysicsRenderable;
using Entity = std::uint32_t;

class EditorUI {
public:
    EditorUI();
    ~EditorUI();

    bool Init(GLFWwindow* window);
    void Update(float deltaTime);
    void Render();
    void Shutdown();

#if defined(IMGUI_ENABLED) && IMGUI_ENABLED
    // Main UI panels
    void RenderMainMenuBar();
    void RenderSceneHierarchy();
    void RenderInspector();
    void RenderAssetBrowser();
    void RenderConsole();
    void RenderToolbar();
    void RenderViewport();
    void RenderStats();

    // Docking
    void SetupDockSpace();
#endif

    // Entity management
    void SetSelectedEntity(Entity entity);
    Entity GetSelectedEntity() const { return m_selectedEntity; }

    // External references
    void SetCoordinator(Coordinator* coordinator) { m_coordinator = coordinator; }
    void SetScene(Scene* scene) { m_scene = scene; }
    void SetCamera(Camera* camera) { m_camera = camera; }
    void SetPhysicsSystem(PhysicsSystem* physics) { m_physicsSystem = physics; }

    // UI State
    bool IsViewportFocused() const { return m_viewportFocused; }
    bool IsViewportHovered() const { return m_viewportHovered; }

private:
    // UI State
    bool m_showSceneHierarchy = true;
    bool m_showInspector = true;
    bool m_showAssetBrowser = true;
    bool m_showConsole = true;
    bool m_showStats = true;
    bool m_showDemoWindow = false;
    bool m_viewportFocused = false;
    bool m_viewportHovered = false;

    // Selected entity
    Entity m_selectedEntity = 0;

    // External references
    Coordinator* m_coordinator = nullptr;
    Scene* m_scene = nullptr;
    Camera* m_camera = nullptr;
    PhysicsSystem* m_physicsSystem = nullptr;

    // Console
    std::vector<std::string> m_consoleMessages;
    void AddConsoleMessage(const std::string& message);

#if defined(IMGUI_ENABLED) && IMGUI_ENABLED
    // Component rendering
    void RenderTransformComponent(Entity entity);
    void RenderRenderComponent(Entity entity);
    void RenderPhysicsComponent(Entity entity);

    // Entity operations
    void CreateEntity(const std::string& name = "New Entity");
    void DeleteEntity(Entity entity);
    void DuplicateEntity(Entity entity);

    // Asset creation
    void CreatePrimitive(const std::string& type);

    // Helper functions
    void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f);
    void DrawFloatControl(const std::string& label, float& value, float resetValue = 0.0f, float columnWidth = 100.0f);
    void DrawBoolControl(const std::string& label, bool& value, float columnWidth = 100.0f);

    // UI Style
    void SetupImGuiStyle();
    void PushStyleColor();
    void PopStyleColor();
#endif
};

#endif // EDITORUI_H