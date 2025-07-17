#ifndef UIMANAGER_H
#define UIMANAGER_H

// Include OpenGL headers before GLFW to prevent conflicts
#include <glad/glad.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>

// Forward declarations
class Coordinator;
struct TransformComponent;
struct RenderComponent;
struct PhysicsComponent;
class Scene;
class PhysicsSystem;

typedef uint32_t Entity;

class UIManager {
public:
    UIManager();
    ~UIManager();

    bool Initialize(GLFWwindow* window);
    void Shutdown();
    void NewFrame();
    void Render();

    void DrawMainMenuBar();
    void DrawHierarchy();
    void DrawInspector();
    void DrawSceneView();
    void DrawAssetBrowser();
    void DrawConsole();

    // Entity management
    void CreateEntity(const std::string& name);
    void DeleteEntity(Entity entity);
    void SelectEntity(Entity entity);
    
    // UI State
    void SetShowDemo(bool show) { m_ShowDemo = show; }
    bool IsShowingDemo() const { return m_ShowDemo; }
    
    // References to engine systems
    void SetCoordinator(Coordinator* coordinator) { m_Coordinator = coordinator; }
    void SetScene(Scene* scene) { m_Scene = scene; }
    void SetPhysicsSystem(PhysicsSystem* physics) { m_PhysicsSystem = physics; }

private:
    bool m_ShowDemo;
    bool m_ShowHierarchy;
    bool m_ShowInspector;
    bool m_ShowSceneView;
    bool m_ShowAssetBrowser;
    bool m_ShowConsole;
    
    // Selected entity
    Entity m_SelectedEntity;
    bool m_HasSelectedEntity;
    
    // Entity creation helpers
    void CreateCube();
    void CreateSphere();
    void CreatePlane();
    void CreateModel();
    
    // Inspector helpers
    void DrawTransformComponent(TransformComponent& transform);
    void DrawRenderComponent(RenderComponent& render);
    void DrawPhysicsComponent(PhysicsComponent& physics);
    
    // Utility
    void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f);
    
    // Engine references
    Coordinator* m_Coordinator;
    Scene* m_Scene;
    PhysicsSystem* m_PhysicsSystem;
    
    // UI state
    std::vector<std::pair<Entity, std::string>> m_EntityList;
    std::vector<std::string> m_ConsoleMessages;
    
    // Entity counter for naming
    int m_EntityCounter;
};

#endif // UIMANAGER_H