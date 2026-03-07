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
#include <unordered_map>

// Forward declarations
class Coordinator;
struct TransformComponent;
struct RenderComponent;
struct PhysicsComponent;
class Scene;
class PhysicsSystem;
class AIManager;
class AIWindow;
struct ChatMessage;
class GameExporter;
class FPSGameManager;
class Renderer;
class UndoRedoManager;
class AssetBrowser;
class GizmoSystem;
class EditorState;
class ConsoleSystem;
class Profiler;

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
    void SetEntityName(Entity entity, const std::string& name);

    // Editor layout
    void DrawToolbar();
    void DrawEditorLayout();

    // UI State
    void SetShowDemo(bool show) { m_ShowDemo = show; }
    bool IsShowingDemo() const { return m_ShowDemo; }

    // AI functionality
    void InitializeAI(const std::string& apiKey, const std::string& provider = "OpenAI", const std::string& endpoint = "");
    void SetShowAI(bool show) { m_ShowAI = show; }
    bool IsShowingAI() const { return m_ShowAI; }
    void ShowAPIKeyDialog();

    // References to engine systems
    void SetCoordinator(Coordinator* coordinator) { m_Coordinator = coordinator; }
    void SetScene(Scene* scene) { m_Scene = scene; }
    void SetPhysicsSystem(PhysicsSystem* physics) { m_PhysicsSystem = physics; }
    void SetRenderer(Renderer* renderer) { m_Renderer = renderer; }

    // FPS Game Manager reference
    void SetFPSGameManager(FPSGameManager* fpsManager) { m_FPSGameManager = fpsManager; }

    // Public methods for FPS game to create objects
    void CreateGameCube() { CreateCube(); }
    void CreateGamePlane() { CreatePlane(); }
    void CreateGameSphere() { CreateSphere(); }

    // Undo/Redo access
    UndoRedoManager& GetUndoRedo();

    // Scene save/load
    void SaveScene(const std::string& path);
    void LoadScene(const std::string& path);

    // Viewport FBO texture (set by renderer for scene view panel)
    void SetViewportTexture(GLuint tex, int w, int h) { m_ViewportTexture = tex; m_ViewportWidth = w; m_ViewportHeight = h; }

    // Selection queries for editor camera focus
    bool HasSelectedEntity() const { return m_HasSelectedEntity; }
    Entity GetSelectedEntity() const { return m_SelectedEntity; }

private:
    bool m_ShowDemo;
    bool m_ShowHierarchy;
    bool m_ShowInspector;
    bool m_ShowSceneView;
    bool m_ShowAssetBrowser;
    bool m_ShowConsole;
    bool m_ShowAI;
    bool m_ShowAPIKeyDialog;
    bool m_ShowExportDialog;
    bool m_ShowProfiler;
    bool m_ShowPostProcess;
    bool m_ShowShadowControls;
    bool m_ShowLightEditor;
    bool m_ShowSkyboxControls;

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

    // AI helpers
    void DrawAPIKeyDialog();

    // Export dialog
    void DrawExportDialog();

    // FPS Game Launcher
    void DrawFPSGameLauncher();

    // FPS Game HUD
    void DrawFPSGameHUD();
    void DrawCrosshair();
    void DrawGameOverScreen();

    // Editor panels (wired from EditorUI.cpp)
    void DrawProfilerWindow();
    void DrawPostProcessControls();
    void DrawShadowControls();
    void DrawLightEditor();
    void DrawSkyboxControls();

    // Utility
    void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f);

    // Engine references
    Coordinator* m_Coordinator;
    Scene* m_Scene;
    PhysicsSystem* m_PhysicsSystem;
    Renderer* m_Renderer;

    // FPS Game Manager reference
    FPSGameManager* m_FPSGameManager;

    // Editor subsystems
    std::unique_ptr<UndoRedoManager> m_UndoRedo;
    std::unique_ptr<AssetBrowser> m_AssetBrowser;
    std::unique_ptr<GizmoSystem> m_GizmoSystem;
    std::unique_ptr<EditorState> m_EditorState;
    std::unique_ptr<ConsoleSystem> m_ConsoleSystem;

    // AI components
    std::unique_ptr<AIManager> m_AIManager;
    std::unique_ptr<AIWindow> m_AIWindow;

    // Game export components
    std::unique_ptr<GameExporter> m_GameExporter;

    // API Key dialog state
    char m_APIKeyBuffer[512];
    char m_EndpointBuffer[512];
    int m_SelectedProvider;

    // Export dialog state
    char m_GameNameBuffer[256];
    char m_OutputPathBuffer[512];
    int m_NumLevels;
    int m_EnemiesPerLevel;
    bool m_IncludeAssets;
    bool m_CompressAssets;

    // Viewport panel state
    GLuint m_ViewportTexture = 0;
    int m_ViewportWidth = 0;
    int m_ViewportHeight = 0;

    // Scene file dialog state
    char m_ScenePathBuffer[512];

    // UI state
    std::vector<std::pair<Entity, std::string>> m_EntityList;
    std::vector<std::string> m_ConsoleMessages;

    // Entity counter for naming
    int m_EntityCounter;

    // Drag-drop hierarchy
    Entity m_DraggedEntity = 0;
    bool m_IsDragging = false;

    // Godot-like fixed layout
    struct EditorLayout {
        float menuBarHeight = 22.0f;
        float toolbarHeight = 34.0f;
        float leftPanelWidth = 250.0f;
        float rightPanelWidth = 300.0f;
        float bottomPanelHeight = 200.0f;
        bool leftPanelVisible = true;
        bool rightPanelVisible = true;
        bool bottomPanelVisible = true;
    };
    EditorLayout m_Layout;

    // Entity naming
    std::unordered_map<Entity, std::string> m_EntityNames;
    int m_BottomTabIndex = 0; // 0=Console, 1=AssetBrowser, 2=Output

    // Hierarchy filter
    char m_HierarchyFilter[128] = "";
};

#endif // UIMANAGER_H