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

#include "Editor/UndoStack.h"

// Forward declarations
class Coordinator;
struct TransformComponent;
struct RenderComponent;
struct PhysicsComponent;
class Scene;
class PhysicsSystem;
class GameExporter;
class Renderer;
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
    // Recursive helper — renders one entity row and walks its
    // HierarchyComponent::children. Kept public so editor modules could
    // hook it in the future, but today only DrawHierarchy calls it.
    void DrawHierarchyNode(Entity entity, const std::string& filterLower);
    // Cycle check for drag-drop reparenting: returns true if `candidate`
    // is `entity` itself or appears anywhere in its subtree. Attaching a
    // parent to its own descendant would create a loop.
    bool IsDescendantOf(Entity candidate, Entity entity) const;

    // Minimal entity snapshot for delete-undo. Not a full-fidelity scene
    // dump — for this cycle we restore Transform + Render (via the
    // shared mesh pointer which AssetRegistry keeps alive) + Hierarchy
    // parent link. Physics rigid bodies are NOT recreated on undo —
    // destroying a physics body through Bullet invalidates it; user
    // re-adds the component if they want it back.
    struct EntitySnapshot {
        bool        hasTransform = false;
        glm::vec3   position{0}, rotation{0}, scale{1};
        bool        hasRender    = false;
        void*       renderable   = nullptr;   // Renderable*; opaque here
        bool        visible      = true;
        std::string meshPath;                 // so respawn round-trips mesh identity
        std::string materialPath;
        bool        hasHierarchy = false;
        Entity      parent       = static_cast<Entity>(-1);
        std::string name;
    };
    EntitySnapshot SnapshotEntity(Entity e) const;
    Entity         RespawnFromSnapshot(const EntitySnapshot& snap);

    // Called when an ASSET_PATH payload is dropped onto the viewport.
    // Routes by file extension: mesh-ish → spawn entity, scene-ish →
    // load scene, anything else → toast a warning.
    void HandleAssetDrop(const std::string& path);
    // Helper: spawn a new entity with the mesh loaded from `path`.
    // Uses AssetRegistry's mesh cache so repeated drops of the same
    // file don't re-parse from disk. Returns the new entity id, or
    // (Entity)-1 on failure.
    Entity SpawnMeshEntity(const std::string& path);

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

    // References to engine systems
    void SetCoordinator(Coordinator* coordinator) { m_Coordinator = coordinator; }
    void SetScene(Scene* scene) { m_Scene = scene; }
    void SetPhysicsSystem(PhysicsSystem* physics) { m_PhysicsSystem = physics; }
    void SetRenderer(Renderer* renderer) { m_Renderer = renderer; }

    // Generic builders (used to be called from the old FPS game's toolbar
    // wrappers; now retained for module code / future tools).
    void CreateGameCube() { CreateCube(); }
    void CreateGamePlane() { CreatePlane(); }
    void CreateGameSphere() { CreateSphere(); }

    // Undo/Redo access — scene-local command history with merge
    // semantics. Replaces the earlier `UndoRedoManager` type.
    Mist::Editor::UndoStack& GetUndoStack() { return m_UndoStack; }

    // Scene save/load
    // Play mode. main() gates physics and script updates on
    // ShouldUpdateGame(); there was no accessor at all before, which is part
    // of why EditorState was never wired to anything.
    EditorState* GetEditorState() { return m_EditorState.get(); }

    // Drops selection and undo history. Required after anything that rebuilds
    // the world underneath the editor — play-mode Stop, for instance.
    void ClearSelectionAndHistory();

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
    bool m_ShowExportDialog;
    bool m_ShowProfiler;
    bool m_ShowEnvironment;
    bool m_ShowLightEditor;

    // Selected entity
    Entity m_SelectedEntity;
    bool m_HasSelectedEntity;

    // Entity creation helpers. All three delegate to CreatePrimitive, which
    // shares a cached `builtin://` mesh rather than leaking a `new Mesh` per
    // call and records `meshPath` so the scene serializer can round-trip it.
    void CreateCube();
    void CreateSphere();
    void CreatePlane();
    void CreatePrimitive(const char* meshPath, const char* displayName,
                         const glm::vec3& position, const glm::vec3& scale,
                         const struct PhysicsComponent& physicsProto);

    // Pushes an editor-authored transform onto the entity's Bullet body, if it
    // has one. Shared by the Inspector's numeric fields and the gizmo drag.
    void SyncPhysicsTransform(Entity entity, const TransformComponent& transform);

    // Inspector helpers
    void DrawTransformComponent(TransformComponent& transform);
    void DrawRenderComponent(RenderComponent& render);
    void DrawPhysicsComponent(PhysicsComponent& physics);

    // Export dialog
    void DrawExportDialog();

    // Crosshair overlay (kept — generic editor/runtime overlay).

    // Reflection-driven generic property editor. Draws ImGui widgets for
    // every field in `props` targeting `obj` as the base pointer. New
    // components register via MIST_REFLECT and get a functional inspector
    // block with zero UIManager edits.
    //
    // `const Mist::PropertyList&` is the typedef from Core/Reflection.h. We
    // use void* here so UIManager.h doesn't need that include in the public
    // header; the .cpp pulls in the real type.
    // `componentType` and `owner` are optional: when both are supplied and the
    // entity belongs to a prefab instance, an edit is recorded as an override.
    void DrawReflectedProperties(void* obj, const void* propertyListPtr,
                                 const char* componentType = nullptr,
                                 Entity owner = static_cast<Entity>(-1));

    // Editor panels (wired from EditorUI.cpp)
    void DrawProfilerWindow();
    // One reflected panel over Renderer::GetEnvironment(). Replaced
    // DrawPostProcessControls / DrawShadowControls / DrawSkyboxControls.
    void DrawEnvironmentPanel();

    // Shortcuts. DispatchShortcuts is the single site that turns a key event
    // into an editor command; RunShortcut maps a registry id to the action.
    void DispatchShortcuts();
    void RunShortcut(const std::string& id);
    static std::string ShortcutChord(const char* id);

    // Extracted from menu/context-menu bodies so a shortcut has something to
    // call. NewScene now actually destroys the previous scene's entities.
    void NewScene();
    void DuplicateEntity(Entity entity);

    // Latched, because "Save Scene As" opens a modal and a shortcut fires
    // outside the menu's ImGui ID stack.
    bool m_OpenSaveSceneAsPopup = false;

    // Prefabs. DestroyPrefabInstance removes every entity the instance
    // spawned, not just the root — destroying only the root orphans its
    // children in the Hierarchy.
    void DestroyPrefabInstance(Entity root);
    void SaveSelectionAsPrefab(const std::string& path);
    void DrawPrefabInstanceBlock(Entity sel);

    // Records an Inspector edit as a prefab override when `entity` belongs to
    // an instance. A no-op otherwise. Without it an edit looks like it worked
    // and silently reverts on the next scene load, because the scene stores a
    // reference plus overrides rather than the expanded subtree.
    void RecordOverrideIfPrefabMember(Entity entity, const char* componentType,
                                      const char* fieldName, const void* component);

    // "Save as Prefab..." is raised from a hierarchy context menu, which has
    // already been popped by the time the modal would open — so the request is
    // latched and the popup opened from the top-level draw.
    bool m_OpenSavePrefabPopup = false;
    char m_PrefabPathBuffer[260] = "prefabs/new.mistprefab";
    void DrawLightEditor();

    // Utility
    void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f);

    // Engine references
    Coordinator* m_Coordinator;
    Scene* m_Scene;
    PhysicsSystem* m_PhysicsSystem;
    Renderer* m_Renderer;

    // Editor subsystems
    Mist::Editor::UndoStack m_UndoStack;
    std::unique_ptr<AssetBrowser> m_AssetBrowser;
    std::unique_ptr<GizmoSystem> m_GizmoSystem;
    std::unique_ptr<EditorState> m_EditorState;
    std::unique_ptr<ConsoleSystem> m_ConsoleSystem;

    // Game export components
    std::unique_ptr<GameExporter> m_GameExporter;

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

    // Cached GLFW window — captured on Initialize so scene save/load
    // and the dirty-indicator title update don't need to thread the
    // pointer through every caller.
    GLFWwindow* m_Window = nullptr;
    bool        m_PrevDirty = false;  // tracks last frame's dirty state

    // Scene file dialog state
    char m_ScenePathBuffer[512];

    // File -> Import Model... popup path input.
    char m_ImportModelPathBuffer[512]{};

    // Assets -> New Material... popup path input.
    char m_NewMaterialPathBuffer[512]{};

    // File -> Export/Import Package... popup inputs.
    char m_PkgScenePathBuffer[512]{};
    char m_PkgOutPathBuffer[512]{};
    char m_PkgInPathBuffer[512]{};

    // Console REPL input line. Hit Enter to run as Lua.
    char m_ConsoleInputBuffer[512]{};

    // UI state
    std::vector<std::pair<Entity, std::string>> m_EntityList;
    std::vector<std::string> m_ConsoleMessages;

    // Entity counter for naming
    int m_EntityCounter;

    // Drag-drop hierarchy
    Entity m_DraggedEntity = 0;
    bool m_IsDragging = false;

    // Gizmo drag capture — tracks the pre-drag transform so we can
    // push a single undo command on mouse release instead of one
    // per frame. `active` flips on drag start (IsUsing transitions
    // from false to true) and off on drag end.
    struct GizmoCapture {
        bool      active = false;
        Entity    entity = 0;
        glm::vec3 position{};
        glm::vec3 rotation{};
        glm::vec3 scale{1.0f};
    };
    GizmoCapture m_GizmoCapture;

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

    int m_BottomTabIndex = 0; // 0=Console, 1=AssetBrowser, 2=Output

    // Hierarchy filter
    char m_HierarchyFilter[128] = "";
};

#endif // UIMANAGER_H