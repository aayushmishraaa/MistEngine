#include "UIManager.h"
#include "Core/Reflection.h"
#include "GameExporter.h"
#include "ECS/Coordinator.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Components/HierarchyComponent.h"
#include "ECS/Systems/HierarchySystem.h"
#include "Scene.h"
#include "PhysicsSystem.h"
#include "Mesh.h"
#include "Texture.h"
#include "ShapeGenerator.h"
#include "Version.h"
#include "Renderer.h"
#include "Editor/UndoStack.h"
#include "Editor/AssetBrowser.h"
#include "Editor/GizmoSystem.h"
#include "ImGuizmo.h"
#include "Editor/EditorState.h"
#include "Editor/MistTheme.h"
#include "Editor/ShortcutRegistry.h"
#include "Editor/Toaster.h"
#include "Core/Logger.h"
#include "Resources/AssetRegistry.h"
#include "Resources/Ref.h"
#include "imgui_internal.h"  // DockBuilder* APIs
#include "Debug/ConsoleSystem.h"
#include "Debug/Profiler.h"
#include "PostProcessStack.h"
#include "ShadowSystem.h"
#include "LightManager.h"
#include "SkyboxRenderer.h"
#include "Scene/SceneSerializer.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <set>
#include <algorithm>

#define MAX_ENTITIES 1000  // Reasonable limit for entity iteration

extern Coordinator gCoordinator;

UIManager::UIManager()
    : m_ShowDemo(false)
    , m_ShowHierarchy(true)
    , m_ShowInspector(true)
    , m_ShowSceneView(true)
    , m_ShowAssetBrowser(true)
    , m_ShowConsole(true)
    , m_ShowExportDialog(false)
    , m_ShowProfiler(false)
    , m_ShowPostProcess(false)
    , m_ShowShadowControls(false)
    , m_ShowLightEditor(false)
    , m_ShowSkyboxControls(false)
    , m_SelectedEntity(0)
    , m_HasSelectedEntity(false)
    , m_Coordinator(nullptr)
    , m_Scene(nullptr)
    , m_PhysicsSystem(nullptr)
    , m_Renderer(nullptr)
    , m_EntityCounter(0)
{
    // Initialize editor subsystems — m_UndoStack is a plain member,
    // default-constructed in the initializer list; no heap alloc needed.
    m_AssetBrowser = std::make_unique<AssetBrowser>();
    m_GizmoSystem = std::make_unique<GizmoSystem>();
    m_EditorState = std::make_unique<EditorState>();
    m_ConsoleSystem = std::make_unique<ConsoleSystem>();
    m_ConsoleSystem->RegisterBuiltins();

    m_GameExporter = std::make_unique<GameExporter>();

    // Initialize dialog buffers
    memset(m_GameNameBuffer, 0, sizeof(m_GameNameBuffer));
    memset(m_OutputPathBuffer, 0, sizeof(m_OutputPathBuffer));
    memset(m_ScenePathBuffer, 0, sizeof(m_ScenePathBuffer));

    // Initialize export settings with defaults
    strncpy(m_GameNameBuffer, "MistFPS", sizeof(m_GameNameBuffer) - 1); m_GameNameBuffer[sizeof(m_GameNameBuffer) - 1] = '\0';
    strncpy(m_OutputPathBuffer, "exports", sizeof(m_OutputPathBuffer) - 1); m_OutputPathBuffer[sizeof(m_OutputPathBuffer) - 1] = '\0';
    strncpy(m_ScenePathBuffer, "scene.json", sizeof(m_ScenePathBuffer) - 1); m_ScenePathBuffer[sizeof(m_ScenePathBuffer) - 1] = '\0';
    m_NumLevels = 5;
    m_EnemiesPerLevel = 10;
    m_IncludeAssets = true;
    m_CompressAssets = true;
}

UIManager::~UIManager() {
    Shutdown();
}

bool UIManager::Initialize(GLFWwindow* window) {
    m_Window = window;  // cache for title updates + future GLFW calls

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // Enable keyboard controls and clipboard
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    // Docking turns every panel into a draggable/rearrangeable dock;
    // layout persists via io.IniFilename below. The viewport flag
    // would enable undocking into OS-native windows, but that adds
    // multi-context complexity — revisit in a later cycle.
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Enable layout persistence (saves panel positions/sizes across sessions)
    io.IniFilename = "imgui_layout.ini";
    
    // Explicitly set clipboard functions to ensure copy/paste works
    io.SetClipboardTextFn = [](void* user_data, const char* text) {
        glfwSetClipboardString((GLFWwindow*)user_data, text);
    };
    io.GetClipboardTextFn = [](void* user_data) -> const char* {
        return glfwGetClipboardString((GLFWwindow*)user_data);
    };
    io.ClipboardUserData = window;

    // Theme derived from base+accent+contrast — see Editor/MistTheme.h.
    // Swap presets at runtime via the View → Theme menu.
    Mist::Editor::MistTheme::Apply(Mist::Editor::MistTheme::MistDark());

    // Wire the engine logger to auto-toast WARN and ERROR messages.
    // Info stays in the console to avoid notification fatigue — if the
    // user cares, they'll look. The editor only surfaces things that
    // need attention.
    Logger::Instance().SetSink([](LogLevel lv, const std::string& msg) {
        using Mist::Editor::Toaster;
        using Mist::Editor::ToastLevel;
        if (lv == LogLevel::WARN)  Toaster::Instance().Push(ToastLevel::Warn,  msg);
        if (lv == LogLevel::ERR)   Toaster::Instance().Push(ToastLevel::Error, msg);
        if (lv == LogLevel::FATAL) Toaster::Instance().Push(ToastLevel::Error, msg, 10.0f);
    });

    // Register keyboard shortcuts. Menu items below pull their chord
    // labels from this registry so rebinding flows through one place.
    {
        using Mist::Editor::ShortcutRegistry;
        using Mist::Editor::Shortcut;
        auto& reg = ShortcutRegistry::Instance();
        reg.Register({"editor/new_scene",    "New Scene",   GLFW_KEY_N,      GLFW_MOD_CONTROL});
        reg.Register({"editor/open_scene",   "Open Scene",  GLFW_KEY_O,      GLFW_MOD_CONTROL});
        reg.Register({"editor/save_scene",   "Save Scene",  GLFW_KEY_S,      GLFW_MOD_CONTROL});
        reg.Register({"editor/save_as",      "Save As",     GLFW_KEY_S,      GLFW_MOD_CONTROL | GLFW_MOD_SHIFT});
        reg.Register({"editor/undo",         "Undo",        GLFW_KEY_Z,      GLFW_MOD_CONTROL});
        reg.Register({"editor/redo",         "Redo",        GLFW_KEY_Y,      GLFW_MOD_CONTROL});
        reg.Register({"editor/duplicate",    "Duplicate",   GLFW_KEY_D,      GLFW_MOD_CONTROL});
        reg.Register({"editor/delete",       "Delete",      GLFW_KEY_DELETE, 0});
        reg.Register({"editor/play",         "Play",        GLFW_KEY_F5,     0});
        reg.Register({"editor/stop",         "Stop",        GLFW_KEY_F8,     0});
        // Gizmo mode shortcuts — input dispatch lands in Phase 3 when
        // ImGuizmo is wired up. Registering here so menu labels are
        // correct from day one.
        reg.Register({"editor/gizmo_select",    "Select",    GLFW_KEY_Q, 0});
        reg.Register({"editor/gizmo_translate", "Translate", GLFW_KEY_W, 0});
        reg.Register({"editor/gizmo_rotate",    "Rotate",    GLFW_KEY_E, 0});
        reg.Register({"editor/gizmo_scale",     "Scale",     GLFW_KEY_R, 0});
        reg.Register({"editor/gizmo_toggle_space", "Toggle Local/World", GLFW_KEY_X, 0});
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    // Verify clipboard functions are set
    if (io.GetClipboardTextFn && io.SetClipboardTextFn) {
        m_ConsoleMessages.push_back("Clipboard functions initialized successfully");
    } else {
        m_ConsoleMessages.push_back("Warning: Clipboard functions not properly initialized");
    }

    // Set up asset browser root directory
    m_AssetBrowser->SetRootDirectory(".");

    // Register console commands
    m_ConsoleSystem->RegisterCommand("save", [this](const std::vector<std::string>& args) {
        std::string path = args.empty() ? "scene.json" : args[0];
        SaveScene(path);
        return "Scene saved to " + path;
    }, "Save scene to file");

    m_ConsoleSystem->RegisterCommand("load", [this](const std::vector<std::string>& args) {
        std::string path = args.empty() ? "scene.json" : args[0];
        LoadScene(path);
        return "Scene loaded from " + path;
    }, "Load scene from file");

    // Add some initial console messages
    m_ConsoleMessages.push_back("MistEngine UI initialized successfully");
    m_ConsoleMessages.push_back("Press F1 to toggle ImGui demo window");
    m_ConsoleMessages.push_back("Ctrl+Z/Y for Undo/Redo");
    m_ConsoleMessages.push_back("Layout persists across sessions (imgui_layout.ini)");

    return true;
}

void UIManager::Shutdown() {
    if (!ImGui::GetCurrentContext()) return; // Already shut down

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::NewFrame() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    // ImGuizmo tracks per-frame state separately from ImGui — this
    // resets hit-test caches so gizmo interactions don't bleed across
    // frames (mis-detecting a hover from last frame, etc.).
    ImGuizmo::BeginFrame();

    {
        // Editor-only frame now — the FPS game mode was removed, so there's
        // no runtime "game active" branch anymore. Kept the outer brace so the
        // diff vs the old `else` block is trivial.

        // Process Ctrl+Z / Ctrl+Y for undo/redo
        ImGuiIO& undoIO = ImGui::GetIO();
        if (undoIO.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) && m_UndoStack.CanUndo()) {
            std::string label = m_UndoStack.TopUndoLabel();
            m_UndoStack.Undo();
            m_ConsoleMessages.push_back("Undo: " + label);
        }
        if (undoIO.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y) && m_UndoStack.CanRedo()) {
            std::string label = m_UndoStack.TopRedoLabel();
            m_UndoStack.Redo();
            m_ConsoleMessages.push_back("Redo: " + label);
        }

        // Gizmo mode shortcuts. Gate on `!WantCaptureKeyboard` so they
        // don't hijack single-letter typing in an InputText (e.g.
        // renaming an entity with "e" in the name). ImGuizmo itself
        // also respects its own input-capture logic.
        if (m_GizmoSystem && !undoIO.WantCaptureKeyboard) {
            if (ImGui::IsKeyPressed(ImGuiKey_W, false)) m_GizmoSystem->SetMode(GizmoMode::Translate);
            if (ImGui::IsKeyPressed(ImGuiKey_E, false)) m_GizmoSystem->SetMode(GizmoMode::Rotate);
            if (ImGui::IsKeyPressed(ImGuiKey_R, false)) m_GizmoSystem->SetMode(GizmoMode::Scale);
            if (ImGui::IsKeyPressed(ImGuiKey_X, false)) m_GizmoSystem->ToggleSpace();
        }

        // Godot-like fixed layout
        DrawMainMenuBar();
        DrawToolbar();
        DrawEditorLayout();

        // Floating windows render on top
        if (m_ShowProfiler) DrawProfilerWindow();
        if (m_ShowPostProcess) DrawPostProcessControls();
        if (m_ShowShadowControls) DrawShadowControls();
        if (m_ShowLightEditor) DrawLightEditor();
        if (m_ShowSkyboxControls) DrawSkyboxControls();
        if (m_ShowDemo) ImGui::ShowDemoWindow(&m_ShowDemo);

        // Draw Export Game dialog
        if (m_ShowExportDialog) {
            DrawExportDialog();
        }

        // Toast notifications render last (top-most) so they overlay
        // any floating panels. DeltaTime comes from ImGui's io since
        // UIManager doesn't track its own frame time.
        Mist::Editor::Toaster::Instance().Draw(ImGui::GetIO().DeltaTime);

        // Update window title when dirty state flips. Don't call
        // glfwSetWindowTitle every frame — it's a syscall on most
        // platforms and will thrash the compositor on Wayland/X11.
        bool dirty = m_UndoStack.IsDirty();
        if (m_Window && dirty != m_PrevDirty) {
            std::string title = std::string("MistEngine - ")
                              + (m_ScenePathBuffer[0] ? m_ScenePathBuffer : "untitled");
            if (dirty) title += " *";
            glfwSetWindowTitle(m_Window, title.c_str());
            m_PrevDirty = dirty;
        }
    }
}

void UIManager::Render() {
    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    GLFWwindow* backup_current_context = glfwGetCurrentContext();
    glfwMakeContextCurrent(backup_current_context);
}

void UIManager::DrawMainMenuBar() {
    // Pull chord display text straight from the registry so menu labels
    // stay in sync with any future rebinding without touching this code.
    auto chord = [](const char* id) -> std::string {
        if (auto* s = Mist::Editor::ShortcutRegistry::Instance().Find(id)) {
            return s->DisplayText();
        }
        return "";
    };

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", chord("editor/new_scene").c_str())) {
                m_ConsoleMessages.push_back("New scene created");
                m_EntityList.clear();
                m_HasSelectedEntity = false;
            }
            if (ImGui::MenuItem("Save Scene", chord("editor/save_scene").c_str())) {
                SaveScene(m_ScenePathBuffer);
            }
            if (ImGui::MenuItem("Save Scene As...", chord("editor/save_as").c_str())) {
                ImGui::OpenPopup("SaveSceneAs");
            }
            if (ImGui::MenuItem("Load Scene", chord("editor/open_scene").c_str())) {
                LoadScene(m_ScenePathBuffer);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Export Game...")) {
                m_ShowExportDialog = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                // Will be handled by GLFW window close
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            bool canUndo = m_UndoStack.CanUndo();
            bool canRedo = m_UndoStack.CanRedo();
            std::string undoLabel = canUndo ? ("Undo " + m_UndoStack.TopUndoLabel()) : "Undo";
            std::string redoLabel = canRedo ? ("Redo " + m_UndoStack.TopRedoLabel()) : "Redo";
            if (ImGui::MenuItem(undoLabel.c_str(), chord("editor/undo").c_str(), false, canUndo)) {
                m_UndoStack.Undo();
            }
            if (ImGui::MenuItem(redoLabel.c_str(), chord("editor/redo").c_str(), false, canRedo)) {
                m_UndoStack.Redo();
            }
            ImGui::Separator();
            ImGui::Text("History: %zu undo, %zu redo",
                m_UndoStack.UndoCount(), m_UndoStack.RedoCount());
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("GameObject")) {
            if (ImGui::MenuItem("Create Empty")) {
                CreateEntity("Empty Entity");
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("3D Object")) {
                if (ImGui::MenuItem("Cube")) {
                    CreateCube();
                }
                if (ImGui::MenuItem("Sphere")) {
                    CreateSphere();
                }
                if (ImGui::MenuItem("Plane")) {
                    CreatePlane();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Hierarchy", nullptr, &m_Layout.leftPanelVisible);
            ImGui::MenuItem("Inspector", nullptr, &m_Layout.rightPanelVisible);
            ImGui::MenuItem("Bottom Panel", nullptr, &m_Layout.bottomPanelVisible);
            ImGui::Separator();
            ImGui::MenuItem("Profiler", nullptr, &m_ShowProfiler);
            ImGui::MenuItem("Post-Processing", nullptr, &m_ShowPostProcess);
            ImGui::MenuItem("Shadow Controls", nullptr, &m_ShowShadowControls);
            ImGui::MenuItem("Light Editor", nullptr, &m_ShowLightEditor);
            ImGui::MenuItem("Skybox Controls", nullptr, &m_ShowSkyboxControls);
            ImGui::Separator();
            if (ImGui::BeginMenu("Theme")) {
                if (ImGui::MenuItem("Mist Dark")) {
                    Mist::Editor::MistTheme::Apply(Mist::Editor::MistTheme::MistDark());
                }
                if (ImGui::MenuItem("Godot Blue")) {
                    Mist::Editor::MistTheme::Apply(Mist::Editor::MistTheme::GodotBlue());
                }
                if (ImGui::MenuItem("Monochrome")) {
                    Mist::Editor::MistTheme::Apply(Mist::Editor::MistTheme::Monochrome());
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Layout")) {
                m_Layout = EditorLayout();
            }
            ImGui::MenuItem("Demo Window", nullptr, &m_ShowDemo);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Project")) {
            if (ImGui::MenuItem("Project Settings...")) {
                m_ConsoleMessages.push_back("Project Settings (stub)");
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

namespace {
// Lowercase ASCII — good enough for file-extension matching. UTF-8
// extensions don't exist in practice for assets we care about.
std::string ascii_lower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}
} // namespace

Entity UIManager::SpawnMeshEntity(const std::string& path) {
    if (!m_Coordinator) return static_cast<Entity>(-1);

    auto& registry = Mist::Assets::AssetRegistry::Instance();
    auto ref = LoadRef(registry.meshes(), path);
    if (!ref) {
        LOG_ERROR("SpawnMeshEntity: failed to load mesh: ", path);
        return static_cast<Entity>(-1);
    }

    Entity e = m_Coordinator->CreateEntity();
    TransformComponent t;
    m_Coordinator->AddComponent(e, t);

    RenderComponent r;
    r.renderable = ref.get();   // non-owning; registry keeps it alive
    r.visible    = true;
    m_Coordinator->AddComponent(e, r);

    m_Coordinator->AddComponent(e, HierarchyComponent{});

    // Name = basename, without extension. Hierarchy + Inspector show
    // that instead of "Entity N".
    auto slash = path.find_last_of("/\\");
    auto dot   = path.find_last_of('.');
    std::string base = (slash == std::string::npos) ? path : path.substr(slash + 1);
    if (dot != std::string::npos && dot > slash) {
        base = (slash == std::string::npos) ? path.substr(0, dot)
                                            : path.substr(slash + 1, dot - slash - 1);
    }
    m_EntityNames[e] = base;
    SelectEntity(e);

    // Push undo. Undo = destroy; redo = re-spawn via snapshot.
    EntitySnapshot snap = SnapshotEntity(e);
    auto idRef = std::make_shared<Entity>(e);
    Mist::Editor::Command c;
    c.label = "Drop asset " + base;
    c.merge_key = 0;
    c.redo = [this, snap, idRef]() { *idRef = RespawnFromSnapshot(snap); };
    c.undo = [this, idRef]() {
        if (m_Coordinator && m_Coordinator->GetLivingEntities().count(*idRef)) {
            m_Coordinator->DestroyEntity(*idRef);
            m_EntityNames.erase(*idRef);
            if (m_HasSelectedEntity && m_SelectedEntity == *idRef) {
                m_HasSelectedEntity = false;
            }
        }
    };
    m_UndoStack.Push(std::move(c));
    return e;
}

void UIManager::HandleAssetDrop(const std::string& path) {
    auto dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        Mist::Editor::Toaster::Instance().Push(
            Mist::Editor::ToastLevel::Warn,
            "Asset has no extension: " + path);
        return;
    }
    std::string ext = ascii_lower(path.substr(dot));

    if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb") {
        SpawnMeshEntity(path);
    } else if (ext == ".scene" || ext == ".json" || ext == ".mscene") {
        LoadScene(path);
    } else {
        Mist::Editor::Toaster::Instance().Push(
            Mist::Editor::ToastLevel::Warn,
            "Unsupported asset type: " + ext);
    }
}

UIManager::EntitySnapshot UIManager::SnapshotEntity(Entity e) const {
    EntitySnapshot s;
    if (!m_Coordinator) return s;
    if (m_Coordinator->HasComponent<TransformComponent>(e)) {
        const auto& t = m_Coordinator->GetComponent<TransformComponent>(e);
        s.hasTransform = true;
        s.position = t.position; s.rotation = t.rotation; s.scale = t.scale;
    }
    if (m_Coordinator->HasComponent<RenderComponent>(e)) {
        const auto& r = m_Coordinator->GetComponent<RenderComponent>(e);
        s.hasRender = true;
        s.renderable = r.renderable;
        s.visible    = r.visible;
    }
    if (m_Coordinator->HasComponent<HierarchyComponent>(e)) {
        const auto& h = m_Coordinator->GetComponent<HierarchyComponent>(e);
        s.hasHierarchy = true;
        s.parent = h.parent;
    }
    auto it = m_EntityNames.find(e);
    if (it != m_EntityNames.end()) s.name = it->second;
    return s;
}

Entity UIManager::RespawnFromSnapshot(const EntitySnapshot& snap) {
    if (!m_Coordinator) return static_cast<Entity>(-1);
    Entity e = m_Coordinator->CreateEntity();
    if (snap.hasTransform) {
        TransformComponent t;
        t.position = snap.position; t.rotation = snap.rotation; t.scale = snap.scale;
        m_Coordinator->AddComponent(e, t);
    }
    if (snap.hasRender) {
        RenderComponent r;
        r.renderable = static_cast<Renderable*>(snap.renderable);
        r.visible    = snap.visible;
        m_Coordinator->AddComponent(e, r);
    }
    if (snap.hasHierarchy) {
        m_Coordinator->AddComponent(e, HierarchyComponent{});
        if (snap.parent != HierarchyComponent::kNoParent
            && m_Coordinator->HasComponent<HierarchyComponent>(snap.parent)) {
            HierarchySystem::Attach(*m_Coordinator, snap.parent, e);
        }
    }
    if (!snap.name.empty()) m_EntityNames[e] = snap.name;
    return e;
}

bool UIManager::IsDescendantOf(Entity candidate, Entity entity) const {
    if (!m_Coordinator) return false;
    if (candidate == entity) return true;
    if (!m_Coordinator->HasComponent<HierarchyComponent>(entity)) return false;
    const auto& h = m_Coordinator->GetComponent<HierarchyComponent>(entity);
    for (Entity child : h.children) {
        if (IsDescendantOf(candidate, child)) return true;
    }
    return false;
}

// Recursive hierarchy row renderer. Walks HierarchyComponent::children
// post-parent so the tree mirrors the scene graph exactly. Used only by
// DrawHierarchy below; kept at translation-unit scope to keep the
// member function flat.
namespace {
std::string HierarchyEntityName(const std::unordered_map<Entity, std::string>& names,
                                Entity e) {
    auto it = names.find(e);
    if (it != names.end()) return it->second;
    return "Entity " + std::to_string(e);
}

bool HierarchyPassesFilter(const std::string& name, const std::string& filterLower) {
    if (filterLower.empty()) return true;
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower.find(filterLower) != std::string::npos;
}
} // namespace

void UIManager::DrawHierarchyNode(Entity entity, const std::string& filterLower) {
    if (!m_Coordinator) return;

    std::string name = HierarchyEntityName(m_EntityNames, entity);

    // Collect children before building flags — a leaf vs. open-on-arrow
    // node needs different flag combinations.
    const std::vector<Entity>* children = nullptr;
    if (m_Coordinator->HasComponent<HierarchyComponent>(entity)) {
        children = &m_Coordinator->GetComponent<HierarchyComponent>(entity).children;
    }

    // Apply filter: if this entity doesn't match BUT a descendant does,
    // we still want to render it as a passthrough so the user sees the
    // path. For now: drop the whole subtree if this row fails. Can
    // revisit once we have fuzzy/path search.
    if (!HierarchyPassesFilter(name, filterLower)) return;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
                             | ImGuiTreeNodeFlags_OpenOnDoubleClick
                             | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (!children || children->empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (m_HasSelectedEntity && m_SelectedEntity == entity) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    ImGui::PushID((int)entity);
    bool opened = ImGui::TreeNodeEx("##node", flags, "%s", name.c_str());
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        SelectEntity(entity);
    }

    // Drag source — carries the entity id in a typed payload. Matches
    // Godot's "unified drag dictionary" pattern at the C++/ImGui level:
    // every drop site checks for payload type MIST_ENTITY (singular for
    // now; the plan upgrades this to a vector once multi-select lands).
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        ImGui::SetDragDropPayload("MIST_ENTITY", &entity, sizeof(Entity));
        ImGui::Text("Reparent %s", name.c_str());
        ImGui::EndDragDropSource();
    }

    // Drop target — if we accept an entity here, wire it as a child via
    // HierarchySystem::Attach. Cycle detection inside the helper:
    // reject if `newParent` is a descendant of the dragged child.
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("MIST_ENTITY")) {
            Entity child = *static_cast<const Entity*>(p->Data);
            if (child != entity && !IsDescendantOf(entity, child)
                && m_Coordinator->HasComponent<HierarchyComponent>(child)) {
                Entity oldParent = m_Coordinator->GetComponent<HierarchyComponent>(child).parent;
                Entity newParent = entity;

                // Apply the move.
                HierarchySystem::Detach(*m_Coordinator, child);
                HierarchySystem::Attach(*m_Coordinator, newParent, child);

                // Record undo.
                Coordinator* coord = m_Coordinator;
                Mist::Editor::Command c;
                c.label = "Reparent";
                c.merge_key = 0;
                c.redo = [coord, child, newParent]() {
                    HierarchySystem::Detach(*coord, child);
                    HierarchySystem::Attach(*coord, newParent, child);
                };
                c.undo = [coord, child, oldParent]() {
                    HierarchySystem::Detach(*coord, child);
                    if (oldParent != HierarchyComponent::kNoParent
                        && coord->HasComponent<HierarchyComponent>(oldParent)) {
                        HierarchySystem::Attach(*coord, oldParent, child);
                    }
                };
                m_UndoStack.Push(std::move(c));
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopupContextItem("EntityContext")) {
        if (ImGui::MenuItem("Rename")) { SelectEntity(entity); }
        if (ImGui::MenuItem("Duplicate")) {
            Entity newEntity = m_Coordinator->CreateEntity();
            if (m_Coordinator->HasComponent<TransformComponent>(entity)) {
                TransformComponent t = m_Coordinator->GetComponent<TransformComponent>(entity);
                t.position.x += 1.0f;
                m_Coordinator->AddComponent(newEntity, t);
            }
            if (m_Coordinator->HasComponent<RenderComponent>(entity)) {
                RenderComponent r = m_Coordinator->GetComponent<RenderComponent>(entity);
                m_Coordinator->AddComponent(newEntity, r);
            }
            // Always add HierarchyComponent so the duplicate is first-class
            // in the scene graph (parent defaults to kNoParent = root).
            m_Coordinator->AddComponent(newEntity, HierarchyComponent{});
            m_EntityNames[newEntity] = name + " (copy)";
            m_ConsoleMessages.push_back("Duplicated: " + m_EntityNames[newEntity]);
            SelectEntity(newEntity);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Delete")) {
            DeleteEntity(entity);
            m_EntityNames.erase(entity);
        }
        ImGui::EndPopup();
    }

    if (opened && children && !children->empty()) {
        // Copy the child list before recursing — context-menu Duplicate
        // and similar can mutate the children vector mid-iteration.
        std::vector<Entity> snapshot = *children;
        for (Entity child : snapshot) {
            DrawHierarchyNode(child, filterLower);
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void UIManager::DrawHierarchy() {
    // "+" button with popup menu
    if (ImGui::Button("+")) {
        ImGui::OpenPopup("AddEntityPopup");
    }
    if (ImGui::BeginPopup("AddEntityPopup")) {
        if (ImGui::MenuItem("Empty")) CreateEntity("Empty");
        if (ImGui::MenuItem("Cube")) CreateCube();
        if (ImGui::MenuItem("Sphere")) CreateSphere();
        if (ImGui::MenuItem("Plane")) CreatePlane();
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##HierFilter", "Search...", m_HierarchyFilter, sizeof(m_HierarchyFilter));
    ImGui::Separator();

    if (!m_Coordinator) return;

    std::string filterLower(m_HierarchyFilter);
    std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);

    // Iterate the authoritative living-entity set from EntityManager —
    // every entity shows up regardless of which path created it (UI, Lua
    // spawn_cube, scene load). Roots are entities with no HierarchyComponent
    // *or* with parent == kNoParent. Everything else is reached by
    // recursing into HierarchyComponent::children below.
    std::vector<Entity> roots;
    const auto& living = m_Coordinator->GetLivingEntities();
    roots.reserve(living.size());
    for (Entity e : living) {
        // Skip entities with no UI-relevant components (matches the prior
        // "did this entity get any ECS plumbing" check).
        bool hasAny = m_Coordinator->HasComponent<TransformComponent>(e)
                   || m_Coordinator->HasComponent<RenderComponent>(e)
                   || m_Coordinator->HasComponent<PhysicsComponent>(e);
        if (!hasAny) continue;

        if (m_Coordinator->HasComponent<HierarchyComponent>(e)) {
            const auto& h = m_Coordinator->GetComponent<HierarchyComponent>(e);
            if (h.parent == HierarchyComponent::kNoParent) roots.push_back(e);
        } else {
            // Entity without HierarchyComponent is treated as a root leaf.
            roots.push_back(e);
        }
    }

    // Stable order by entity id so scene layout doesn't shuffle every
    // frame when an unordered_set iteration changes order.
    std::sort(roots.begin(), roots.end());

    // Invisible "unparent to root" drop band. Dropping an entity here
    // detaches it from its current parent so it becomes a root.
    ImGui::InvisibleButton("##root_drop", ImVec2(-1, 4));
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("MIST_ENTITY")) {
            Entity child = *static_cast<const Entity*>(p->Data);
            if (m_Coordinator->HasComponent<HierarchyComponent>(child)) {
                Entity oldParent = m_Coordinator->GetComponent<HierarchyComponent>(child).parent;
                HierarchySystem::Detach(*m_Coordinator, child);

                Coordinator* coord = m_Coordinator;
                Mist::Editor::Command c;
                c.label = "Unparent";
                c.merge_key = 0;
                c.redo = [coord, child]() { HierarchySystem::Detach(*coord, child); };
                c.undo = [coord, child, oldParent]() {
                    if (oldParent != HierarchyComponent::kNoParent
                        && coord->HasComponent<HierarchyComponent>(oldParent)) {
                        HierarchySystem::Attach(*coord, oldParent, child);
                    }
                };
                m_UndoStack.Push(std::move(c));
            }
        }
        ImGui::EndDragDropTarget();
    }

    for (Entity e : roots) {
        DrawHierarchyNode(e, filterLower);
    }

    if (roots.empty()) {
        ImGui::TextDisabled("No entities in scene");
        ImGui::TextDisabled("Use + or GameObject menu");
    }
}

void UIManager::DrawInspector() {
    if (m_HasSelectedEntity && m_Coordinator) {
        // Editable entity name at top
        auto nameIt = m_EntityNames.find(m_SelectedEntity);
        std::string currentName = (nameIt != m_EntityNames.end()) ? nameIt->second : ("Entity " + std::to_string(m_SelectedEntity));
        char nameBuf[128];
        strncpy(nameBuf, currentName.c_str(), sizeof(nameBuf) - 1); nameBuf[sizeof(nameBuf) - 1] = '\0';
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##EntityName", nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
            m_EntityNames[m_SelectedEntity] = nameBuf;
        }
        ImGui::TextDisabled("ID: %d", m_SelectedEntity);
        ImGui::Separator();
        
        // Per-component header with inline Remove button. The closure
        // renders `[X]` right-aligned next to the header label; click
        // removes the component + pushes an undo command that re-adds
        // the snapshotted state. `bodyFn` draws the component's
        // property widgets inside the collapsing header body.
        auto drawComponent = [&](const char* label, auto hasFn,
                                 auto removeFn, auto addBackFn,
                                 auto bodyFn) {
            if (!hasFn()) return;
            // Row layout: header label + right-aligned X button.
            bool open = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen);
            ImGui::SameLine(ImGui::GetWindowWidth() - 30.0f);
            ImGui::PushID(label);
            if (ImGui::SmallButton("X")) {
                Mist::Editor::Command c;
                c.label = std::string("Remove ") + label;
                c.merge_key = 0;
                // addBackFn captures the current component value so
                // undo restores the same state (not a default).
                c.redo = removeFn;
                c.undo = addBackFn;
                removeFn();  // apply now
                m_UndoStack.Push(std::move(c));
                ImGui::PopID();
                return;  // don't draw body — component is gone
            }
            ImGui::PopID();
            if (open) bodyFn();
        };

        Entity sel = m_SelectedEntity;
        Coordinator* coord = m_Coordinator;

        drawComponent("Transform",
            [&] { return coord->HasComponent<TransformComponent>(sel); },
            [=] { coord->RemoveComponent<TransformComponent>(sel); },
            [=, snap = SnapshotEntity(sel)] {
                TransformComponent t;
                if (snap.hasTransform) {
                    t.position = snap.position; t.rotation = snap.rotation; t.scale = snap.scale;
                }
                coord->AddComponent(sel, t);
            },
            [&] { DrawTransformComponent(coord->GetComponent<TransformComponent>(sel)); });

        drawComponent("Render",
            [&] { return coord->HasComponent<RenderComponent>(sel); },
            [=] { coord->RemoveComponent<RenderComponent>(sel); },
            [=, snap = SnapshotEntity(sel)] {
                RenderComponent r;
                r.renderable = static_cast<Renderable*>(snap.renderable);
                r.visible    = snap.visible;
                coord->AddComponent(sel, r);
            },
            [&] { DrawRenderComponent(coord->GetComponent<RenderComponent>(sel)); });

        drawComponent("Physics",
            [&] { return coord->HasComponent<PhysicsComponent>(sel); },
            [=] { coord->RemoveComponent<PhysicsComponent>(sel); },
            // Physics undo: we don't recreate the Bullet rigid body
            // (see EntitySnapshot comment). Restores an empty Physics
            // component slot; user re-adds body through gameplay code.
            [=] { PhysicsComponent p; coord->AddComponent(sel, p); },
            [&] { DrawPhysicsComponent(coord->GetComponent<PhysicsComponent>(sel)); });

        ImGui::Separator();

        // Add Component button — HasComponent replaces the old
        // try/catch dance; duplicate-add warns via toast instead of
        // silently no-op'ing.
        if (ImGui::Button("Add Component")) {
            ImGui::OpenPopup("AddComponentPopup");
        }

        auto tryAdd = [&](const char* name, auto hasFn, auto addFn) {
            if (ImGui::MenuItem(name)) {
                if (hasFn()) {
                    Mist::Editor::Toaster::Instance().Push(
                        Mist::Editor::ToastLevel::Info,
                        std::string(name) + " already present on this entity");
                } else {
                    addFn();
                }
            }
        };

        if (ImGui::BeginPopup("AddComponentPopup")) {
            tryAdd("Transform",
                   [&] { return coord->HasComponent<TransformComponent>(sel); },
                   [&] { coord->AddComponent(sel, TransformComponent{}); });
            tryAdd("Render",
                   [&] { return coord->HasComponent<RenderComponent>(sel); },
                   [&] {
                       RenderComponent r; r.renderable = nullptr; r.visible = true;
                       coord->AddComponent(sel, r);
                   });
            tryAdd("Physics",
                   [&] { return coord->HasComponent<PhysicsComponent>(sel); },
                   [&] {
                       PhysicsComponent p; p.rigidBody = nullptr; p.syncTransform = true;
                       coord->AddComponent(sel, p);
                   });
            ImGui::EndPopup();
        }
    } else {
        ImGui::TextDisabled("No entity selected");
        ImGui::TextDisabled("Click an entity in Hierarchy");
    }
}

void UIManager::DrawSceneView() {
    // Keep the Renderer informed about whether this panel is currently
    // visible. When closed, Renderer falls back to blitting the viewport's
    // output directly to the default framebuffer instead of handing it to
    // this panel — without that fallback, closing the panel would leave the
    // user staring at ImGui's background ("blue screen" bug).
    if (m_Renderer) {
        m_Renderer->SetFullscreenPresent(!m_ShowSceneView);
    }

    if (!m_ShowSceneView) {
        return;
    }

    ImGui::Begin("Scene View", &m_ShowSceneView);

    if (m_ViewportTexture != 0) {
        // Display the rendered scene as an ImGui image
        ImVec2 available = ImGui::GetContentRegionAvail();
        float aspect = (m_ViewportHeight > 0) ? (float)m_ViewportWidth / (float)m_ViewportHeight : 16.0f / 9.0f;
        float displayW = available.x;
        float displayH = displayW / aspect;
        if (displayH > available.y) {
            displayH = available.y;
            displayW = displayH * aspect;
        }

        // Flip UV vertically for OpenGL (texture origin is bottom-left)
        ImGui::Image((ImTextureID)(intptr_t)m_ViewportTexture,
            ImVec2(displayW, displayH),
            ImVec2(0, 1), ImVec2(1, 0));

        // Gizmo controls below viewport
        if (m_GizmoSystem) {
            ImGui::Separator();
            ImGui::Text("Gizmo: ");
            ImGui::SameLine();
            if (ImGui::RadioButton("Translate", m_GizmoSystem->GetMode() == GizmoMode::Translate))
                m_GizmoSystem->SetMode(GizmoMode::Translate);
            ImGui::SameLine();
            if (ImGui::RadioButton("Rotate", m_GizmoSystem->GetMode() == GizmoMode::Rotate))
                m_GizmoSystem->SetMode(GizmoMode::Rotate);
            ImGui::SameLine();
            if (ImGui::RadioButton("Scale", m_GizmoSystem->GetMode() == GizmoMode::Scale))
                m_GizmoSystem->SetMode(GizmoMode::Scale);
        }
    } else {
        ImGui::Text("No viewport texture bound.");
        ImGui::Text("Set a Renderer reference to enable scene view.");
    }

    ImGui::End();
}

void UIManager::DrawAssetBrowser() {
    ImGui::Begin("Asset Browser", &m_ShowAssetBrowser);

    if (m_AssetBrowser) {
        // Navigation
        if (m_AssetBrowser->CanNavigateUp()) {
            if (ImGui::Button("<- Back")) m_AssetBrowser->NavigateUp();
            ImGui::SameLine();
        }
        if (ImGui::Button("Refresh")) m_AssetBrowser->Refresh();
        ImGui::SameLine();
        ImGui::Text("Path: %s", m_AssetBrowser->GetRelativePath().c_str());
        ImGui::Separator();

        // Filter
        static char filterBuf[128] = "";
        ImGui::SetNextItemWidth(200);
        ImGui::InputText("Filter", filterBuf, sizeof(filterBuf));
        m_AssetBrowser->SetFilter(filterBuf);
        ImGui::Separator();

        // File listing with icons
        auto entries = m_AssetBrowser->GetFilteredEntries();
        int columns = std::max(1, (int)(ImGui::GetContentRegionAvail().x / 100.0f));
        if (ImGui::BeginTable("AssetGrid", columns)) {
            int col = 0;
            for (auto& entry : entries) {
                if (col == 0) ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(col);

                ImGui::PushID(entry.fullPath.c_str());

                if (entry.isDirectory) {
                    if (ImGui::Button(("[DIR] " + entry.name).c_str(), ImVec2(90, 40))) {
                        m_AssetBrowser->NavigateTo(entry.fullPath);
                        ImGui::PopID();
                        break;
                    }
                } else {
                    // Color-code by extension
                    ImVec4 color(0.8f, 0.8f, 0.8f, 1.0f);
                    if (entry.extension == ".glsl" || entry.extension == ".frag" || entry.extension == ".vert" || entry.extension == ".comp")
                        color = ImVec4(0.4f, 0.8f, 0.4f, 1.0f); // Green for shaders
                    else if (entry.extension == ".jpg" || entry.extension == ".png" || entry.extension == ".hdr")
                        color = ImVec4(0.8f, 0.6f, 0.2f, 1.0f); // Orange for textures
                    else if (entry.extension == ".obj" || entry.extension == ".fbx" || entry.extension == ".gltf")
                        color = ImVec4(0.2f, 0.6f, 0.8f, 1.0f); // Blue for models
                    else if (entry.extension == ".json")
                        color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f); // Yellow for data

                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    ImGui::Selectable(entry.name.c_str(), false, 0, ImVec2(90, 20));
                    ImGui::PopStyleColor();

                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s\nSize: %lu bytes", entry.fullPath.c_str(), (unsigned long)entry.fileSize);
                    }

                    // Drag source for assets
                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                        ImGui::SetDragDropPayload("ASSET_PATH", entry.fullPath.c_str(), entry.fullPath.size() + 1);
                        ImGui::Text("Drag: %s", entry.name.c_str());
                        ImGui::EndDragDropSource();
                    }
                }

                ImGui::PopID();
                col = (col + 1) % columns;
            }
            ImGui::EndTable();
        }

        ImGui::Separator();
        ImGui::Text("%zu items", entries.size());
    }

    ImGui::End();
}

void UIManager::DrawConsole() {
    ImGui::Begin("Console", &m_ShowConsole);
    
    // Display console messages
    for (const auto& message : m_ConsoleMessages) {
        ImGui::TextUnformatted(message.c_str());
    }
    
    // Auto-scroll to bottom
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::End();
}

void UIManager::CreateEntity(const std::string& name) {
    if (!m_Coordinator) return;

    Entity entity = m_Coordinator->CreateEntity();
    m_EntityCounter = std::max(m_EntityCounter, (int)entity + 1);

    TransformComponent transform;
    m_Coordinator->AddComponent(entity, transform);

    // Hierarchy participation — matches the Lua spawn_cube path so
    // the scene tree shows UI-created entities identically to
    // script-created ones.
    m_Coordinator->AddComponent(entity, HierarchyComponent{});

    m_EntityNames[entity] = name;
    m_ConsoleMessages.push_back("Created entity: " + name);

    SelectEntity(entity);

    // Undo for create = destroy; redo respawns via snapshot of what
    // we just built. Captured snapshot covers name + root-level empty
    // entity shape.
    EntitySnapshot snap = SnapshotEntity(entity);
    auto idRef = std::make_shared<Entity>(entity);
    Mist::Editor::Command c;
    c.label = "Create " + name;
    c.merge_key = 0;
    c.redo = [this, snap, idRef]() {
        *idRef = RespawnFromSnapshot(snap);
    };
    c.undo = [this, idRef]() {
        if (m_Coordinator && m_Coordinator->GetLivingEntities().count(*idRef)) {
            m_Coordinator->DestroyEntity(*idRef);
            m_EntityNames.erase(*idRef);
            if (m_HasSelectedEntity && m_SelectedEntity == *idRef) {
                m_HasSelectedEntity = false;
            }
        }
    };
    m_UndoStack.Push(std::move(c));
}

void UIManager::DeleteEntity(Entity entity) {
    if (!m_Coordinator) return;

    // Snapshot before destroy so undo can respawn the same components.
    // Physics bodies are not captured (Bullet-owned pointers don't
    // survive DestroyEntity); the undo comment in UIManager.h
    // documents this trade-off.
    EntitySnapshot snap = SnapshotEntity(entity);

    m_Coordinator->DestroyEntity(entity);
    m_EntityNames.erase(entity);
    if (m_HasSelectedEntity && m_SelectedEntity == entity) {
        m_HasSelectedEntity = false;
        m_SelectedEntity = 0;
    }
    m_ConsoleMessages.push_back("Deleted entity: " + std::to_string(entity));

    // Record undo. Redo re-destroys the entity-id-of-the-moment; undo
    // respawns from the snapshot. Entity ids *may* change across the
    // cycle — undo_id below is mutable state the lambda captures by
    // shared_ptr so redo/undo can rebind after a respawn.
    auto respawnedId = std::make_shared<Entity>(entity);
    Mist::Editor::Command c;
    c.label = "Delete entity";
    c.merge_key = 0;  // never merge deletes
    c.undo = [this, snap, respawnedId]() {
        *respawnedId = RespawnFromSnapshot(snap);
    };
    c.redo = [this, respawnedId]() {
        if (m_Coordinator && m_Coordinator->GetLivingEntities().count(*respawnedId)) {
            m_Coordinator->DestroyEntity(*respawnedId);
            m_EntityNames.erase(*respawnedId);
            if (m_HasSelectedEntity && m_SelectedEntity == *respawnedId) {
                m_HasSelectedEntity = false;
            }
        }
    };
    m_UndoStack.Push(std::move(c));
}

void UIManager::SelectEntity(Entity entity) {
    m_SelectedEntity = entity;
    m_HasSelectedEntity = true;
}

void UIManager::CreateCube() {
    if (m_Coordinator && m_PhysicsSystem) {
        Entity entity = m_Coordinator->CreateEntity();
        m_EntityCounter = std::max(m_EntityCounter, (int)entity + 1);
        
        m_ConsoleMessages.push_back("Creating cube entity " + std::to_string(entity));
        
        // Transform
        TransformComponent transform;
        transform.position = glm::vec3(0.0f, 2.0f, 0.0f);  // Spawn higher up
        transform.scale = glm::vec3(1.0f);
        m_Coordinator->AddComponent(entity, transform);
        m_ConsoleMessages.push_back("Added transform component");
        
        // Create mesh
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        generateCubeMesh(vertices, indices);
        std::vector<Texture> textures;
        Mesh* mesh = new Mesh(vertices, indices, textures);
        
        // Render
        RenderComponent render;
        render.renderable = mesh;
        render.visible = true;
        m_Coordinator->AddComponent(entity, render);
        m_ConsoleMessages.push_back("Added render component");
        
        // Physics
        btRigidBody* body = m_PhysicsSystem->CreateCube(transform.position, 1.0f);
        PhysicsComponent physics;
        physics.rigidBody = body;
        physics.syncTransform = true;
        m_Coordinator->AddComponent(entity, physics);
        m_ConsoleMessages.push_back("Added physics component");
        
        m_Coordinator->AddComponent(entity, HierarchyComponent{});

        m_EntityNames[entity] = "Cube";
        m_ConsoleMessages.push_back("Cube entity created successfully with " + std::to_string(vertices.size()) + " vertices");
        SelectEntity(entity);
    } else {
        m_ConsoleMessages.push_back("ERROR: Cannot create cube - missing coordinator or physics system");
    }
}

void UIManager::CreateSphere() {
    if (m_Coordinator && m_PhysicsSystem) {
        Entity entity = m_Coordinator->CreateEntity();
        m_EntityCounter = std::max(m_EntityCounter, (int)entity + 1);
        
        m_ConsoleMessages.push_back("Creating sphere entity " + std::to_string(entity));
        
        // Transform
        TransformComponent transform;
        transform.position = glm::vec3(2.0f, 3.0f, 0.0f);  // Spawn to the side and higher up
        transform.scale = glm::vec3(1.0f);
        m_Coordinator->AddComponent(entity, transform);
        m_ConsoleMessages.push_back("Added transform component");
        
        // Create sphere mesh
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        generateSphereMesh(vertices, indices, 1.0f, 36, 18); // radius, sectors, stacks
        std::vector<Texture> textures;
        Mesh* mesh = new Mesh(vertices, indices, textures);
        
        // Render
        RenderComponent render;
        render.renderable = mesh;
        render.visible = true;
        m_Coordinator->AddComponent(entity, render);
        m_ConsoleMessages.push_back("Added render component");
        
        // Physics - create sphere physics body
        btRigidBody* body = m_PhysicsSystem->CreateSphere(transform.position, 1.0f, 1.0f); // position, radius, mass
        PhysicsComponent physics;
        physics.rigidBody = body;
        physics.syncTransform = true;
        m_Coordinator->AddComponent(entity, physics);
        m_ConsoleMessages.push_back("Added physics component");
        
        m_Coordinator->AddComponent(entity, HierarchyComponent{});

        m_EntityNames[entity] = "Sphere";
        m_ConsoleMessages.push_back("Sphere entity created successfully with " + std::to_string(vertices.size()) + " vertices");
        SelectEntity(entity);
    } else {
        m_ConsoleMessages.push_back("ERROR: Cannot create sphere - missing coordinator or physics system");
    }
}

void UIManager::CreatePlane() {
    if (m_Coordinator && m_PhysicsSystem) {
        Entity entity = m_Coordinator->CreateEntity();
        m_EntityCounter = std::max(m_EntityCounter, (int)entity + 1);
        
        m_ConsoleMessages.push_back("Creating plane entity " + std::to_string(entity));
        
        // Transform
        TransformComponent transform;
        transform.position = glm::vec3(0.0f, -1.0f, 0.0f);  // Place below origin
        transform.scale = glm::vec3(10.0f, 1.0f, 10.0f);
        m_Coordinator->AddComponent(entity, transform);
        m_ConsoleMessages.push_back("Added transform component");
        
        // Create mesh
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        generatePlaneMesh(vertices, indices);
        std::vector<Texture> textures;
        Mesh* mesh = new Mesh(vertices, indices, textures);
        
        // Render
        RenderComponent render;
        render.renderable = mesh;
        render.visible = true;
        m_Coordinator->AddComponent(entity, render);
        m_ConsoleMessages.push_back("Added render component");
        
        // Physics
        btRigidBody* body = m_PhysicsSystem->CreateGroundPlane(transform.position);
        PhysicsComponent physics;
        physics.rigidBody = body;
        physics.syncTransform = true;
        m_Coordinator->AddComponent(entity, physics);
        m_ConsoleMessages.push_back("Added physics component");
        
        m_Coordinator->AddComponent(entity, HierarchyComponent{});

        m_EntityNames[entity] = "Plane";
        m_ConsoleMessages.push_back("Plane entity created successfully with " + std::to_string(vertices.size()) + " vertices");
        SelectEntity(entity);
    } else {
        m_ConsoleMessages.push_back("ERROR: Cannot create plane - missing coordinator or physics system");
    }
}

void UIManager::DrawTransformComponent(TransformComponent& transform) {
    // Store original values to detect changes
    glm::vec3 originalPos = transform.position;
    glm::vec3 originalRot = transform.rotation;
    glm::vec3 originalScale = transform.scale;

    DrawVec3Control("Position", transform.position);
    DrawVec3Control("Rotation", transform.rotation);
    DrawVec3Control("Scale", transform.scale, 1.0f);

    // If transform was modified, record undo command and sync physics
    if (m_HasSelectedEntity && m_Coordinator) {
        bool transformChanged = (transform.position != originalPos ||
                                transform.rotation != originalRot ||
                                transform.scale != originalScale);

        if (transformChanged) {
            // Capture for undo via the merge-aware stack. Using one key
            // per entity transform ("entity/transform") means successive
            // drags within 500ms collapse into a single undo step —
            // matches Godot's slider-drag behaviour.
            Entity entity = m_SelectedEntity;
            glm::vec3 newPos = transform.position;
            glm::vec3 newRot = transform.rotation;
            glm::vec3 newScale = transform.scale;
            Coordinator* coord = m_Coordinator;

            Mist::Editor::Command c;
            c.label = "Transform";
            c.merge_key = (static_cast<std::uint64_t>(entity) << 8) | 0x01; // 0x01 = transform
            c.redo = [coord, entity, newPos, newRot, newScale]() {
                if (coord->HasComponent<TransformComponent>(entity)) {
                    auto& t = coord->GetComponent<TransformComponent>(entity);
                    t.position = newPos; t.rotation = newRot; t.scale = newScale;
                    t.dirty = true;
                }
            };
            c.undo = [coord, entity, originalPos, originalRot, originalScale]() {
                if (coord->HasComponent<TransformComponent>(entity)) {
                    auto& t = coord->GetComponent<TransformComponent>(entity);
                    t.position = originalPos; t.rotation = originalRot; t.scale = originalScale;
                    t.dirty = true;
                }
            };
            m_UndoStack.Push(std::move(c));
        }

        if (transformChanged) {
            try {
                auto& physics = m_Coordinator->GetComponent<PhysicsComponent>(m_SelectedEntity);
                if (physics.rigidBody && physics.syncTransform) {
                    // Update physics body position to match transform
                    btTransform physicsTransform;
                    physicsTransform.setOrigin(btVector3(transform.position.x, transform.position.y, transform.position.z));
                    
                    // Convert rotation from degrees to radians for physics
                    btQuaternion rotation;
                    rotation.setEulerZYX(glm::radians(transform.rotation.y), 
                                       glm::radians(transform.rotation.x), 
                                       glm::radians(transform.rotation.z));
                    physicsTransform.setRotation(rotation);
                    
                    physics.rigidBody->setWorldTransform(physicsTransform);
                    physics.rigidBody->getMotionState()->setWorldTransform(physicsTransform);
                    physics.rigidBody->activate(true); // Wake up the physics body
                }
            } catch (...) {
                // No physics component, that's okay
            }
        }
    }
}

void UIManager::DrawRenderComponent(RenderComponent& render) {
    // Reflection-driven block for reflected fields (currently: `visible`).
    // Add MIST_FIELD lines in RenderComponent.h and they appear here for free.
    if (const auto* props = Mist::TypeRegistry::Instance().Get("RenderComponent")) {
        DrawReflectedProperties(&render, props);
    }

    // Post-hook: surface non-reflected runtime info (the concrete Mesh pointer).
    if (render.renderable) {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Renderable: Active");
        Mesh* mesh = dynamic_cast<Mesh*>(render.renderable);
        if (mesh) {
            ImGui::Text("Vertices: %zu", mesh->vertices.size());
            ImGui::Text("Indices: %zu", mesh->indices.size());
        }
    } else {
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), "Renderable: None");
    }
}

void UIManager::DrawPhysicsComponent(PhysicsComponent& physics) {
    // Reflection-driven block for reflected fields (currently: `syncTransform`).
    if (const auto* props = Mist::TypeRegistry::Instance().Get("PhysicsComponent")) {
        DrawReflectedProperties(&physics, props);
    }
    
    if (physics.rigidBody) {
        ImGui::Text("Rigid Body: Valid");
        
        // Show physics info
        btVector3 velocity = physics.rigidBody->getLinearVelocity();
        ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", velocity.x(), velocity.y(), velocity.z());
        
        float mass = physics.rigidBody->getMass();
        ImGui::Text("Mass: %.2f", mass);
    } else {
        ImGui::Text("Rigid Body: None");
    }
}

void UIManager::DrawReflectedProperties(void* obj, const void* propertyListPtr) {
    // Dispatches on (PropertyType, PropertyHint) to the right ImGui widget.
    // This is the generic spine the component inspectors delegate to —
    // adding a new field in a MIST_REFLECT block is enough to see it here
    // without touching UIManager. Anything unknown falls through to a
    // greyed-out label so a registration mistake is visible rather than
    // silent.
    auto* props = static_cast<const Mist::PropertyList*>(propertyListPtr);
    if (!obj || !props) return;

    auto* base = static_cast<char*>(obj);

    for (const auto& p : *props) {
        void* field = base + p.offset;

        switch (p.type) {
            case Mist::PropertyType::Bool:
                ImGui::Checkbox(p.name, reinterpret_cast<bool*>(field));
                break;

            case Mist::PropertyType::Int:
                ImGui::DragInt(p.name, reinterpret_cast<int*>(field));
                break;

            case Mist::PropertyType::Float: {
                float* f = reinterpret_cast<float*>(field);
                if (p.hint == Mist::PropertyHint::Range) {
                    float lo = 0, hi = 1, step = 0.01f;
                    if (Mist::parse_range_hint(p.hintString, lo, hi, step)) {
                        ImGui::SliderFloat(p.name, f, lo, hi);
                        break;
                    }
                }
                ImGui::DragFloat(p.name, f, 0.01f);
                break;
            }

            case Mist::PropertyType::Vec2:
                ImGui::DragFloat2(p.name, reinterpret_cast<float*>(field), 0.01f);
                break;

            case Mist::PropertyType::Vec3:
                if (p.hint == Mist::PropertyHint::Color) {
                    ImGui::ColorEdit3(p.name, reinterpret_cast<float*>(field));
                } else {
                    ImGui::DragFloat3(p.name, reinterpret_cast<float*>(field), 0.01f);
                }
                break;

            case Mist::PropertyType::Vec4:
                if (p.hint == Mist::PropertyHint::Color) {
                    ImGui::ColorEdit4(p.name, reinterpret_cast<float*>(field));
                } else {
                    ImGui::DragFloat4(p.name, reinterpret_cast<float*>(field), 0.01f);
                }
                break;

            case Mist::PropertyType::String: {
                auto* s = reinterpret_cast<std::string*>(field);
                char buf[256];
                std::strncpy(buf, s->c_str(), sizeof(buf) - 1);
                buf[sizeof(buf) - 1] = '\0';
                const ImGuiInputTextFlags flags =
                    (p.hint == Mist::PropertyHint::Multiline) ? ImGuiInputTextFlags_AllowTabInput
                                                               : 0;
                if (ImGui::InputText(p.name, buf, sizeof(buf), flags)) {
                    *s = buf;
                }
                break;
            }

            case Mist::PropertyType::Unknown:
            default:
                ImGui::TextDisabled("%s (unreflected type)", p.name);
                break;
        }
    }
}

void UIManager::DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float columnWidth) {
    ImGui::PushID(label.c_str());
    
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text("%s", label.c_str());
    ImGui::NextColumn();
    
    ImGui::PushItemWidth(-1);
    float lineHeight = ImGui::GetTextLineHeight();
    ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};
    
    float widthEach = (ImGui::CalcItemWidth() - 6.0f) / 3.0f;
    
    // X Component
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
    if (ImGui::Button("X", buttonSize))
        values.x = resetValue;
    ImGui::PopStyleColor(3);
    
    ImGui::SameLine();
    ImGui::SetNextItemWidth(widthEach);
    ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::SameLine();
    
    // Y Component
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
    if (ImGui::Button("Y", buttonSize))
        values.y = resetValue;
    ImGui::PopStyleColor(3);
    
    ImGui::SameLine();
    ImGui::SetNextItemWidth(widthEach);
    ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::SameLine();
    
    // Z Component
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
    if (ImGui::Button("Z", buttonSize))
        values.z = resetValue;
    ImGui::PopStyleColor(3);
    
    ImGui::SameLine();
    ImGui::SetNextItemWidth(widthEach);
    ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
    
    ImGui::PopItemWidth();
    ImGui::Columns(1);
    ImGui::PopID();
}

void UIManager::DrawExportDialog() {
    if (m_ShowExportDialog) {
        ImGui::OpenPopup("Export FPS Game");
    }
    
    // Always draw the popup modal
    if (ImGui::BeginPopupModal("Export FPS Game", &m_ShowExportDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Export your FPS game as a standalone playable!");
        ImGui::Separator();
        
        // Game Settings
        if (ImGui::CollapsingHeader("Game Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Game Name:");
            ImGui::SetNextItemWidth(300);
            ImGui::InputText("##GameName", m_GameNameBuffer, sizeof(m_GameNameBuffer));
            
            ImGui::Text("Output Directory:");
            ImGui::SetNextItemWidth(300);
            ImGui::InputText("##OutputPath", m_OutputPathBuffer, sizeof(m_OutputPathBuffer));
            ImGui::SameLine();
            if (ImGui::Button("Browse...")) {
                // TODO: Could implement file browser here
                m_ConsoleMessages.push_back("File browser not implemented, manually enter path");
            }
        }
        
        // Level Settings
        if (ImGui::CollapsingHeader("Level Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderInt("Number of Levels", &m_NumLevels, 1, 20);
            ImGui::SliderInt("Enemies per Level", &m_EnemiesPerLevel, 5, 50);
            
            ImGui::Spacing();
            ImGui::Text("Each level will have %d + (level-1)*2 enemies", m_EnemiesPerLevel);
            ImGui::Text("Total enemies across all levels: %d", 
                m_NumLevels * m_EnemiesPerLevel + (m_NumLevels * (m_NumLevels - 1)));
        }
        
        // Export Options
        if (ImGui::CollapsingHeader("Export Options", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Include Assets", &m_IncludeAssets);
            if (m_IncludeAssets) {
                ImGui::Checkbox("Compress Assets", &m_CompressAssets);
            }
            
            ImGui::Spacing();
            ImGui::Text("Export will include:");
            ImGui::BulletText("Game executable (placeholder)");
            ImGui::BulletText("Configuration files");
            ImGui::BulletText("Level data for %d levels", m_NumLevels);
            if (m_IncludeAssets) {
                ImGui::BulletText("Asset files %s", m_CompressAssets ? "(compressed)" : "(uncompressed)");
            }
            ImGui::BulletText("Launcher script");
            ImGui::BulletText("README with instructions");
        }
        
        // Export Progress
        if (m_GameExporter && m_GameExporter->IsExporting()) {
            ImGui::Separator();
            ImGui::Text("Export Progress:");
            ImGui::ProgressBar(m_GameExporter->GetExportProgress() / 100.0f);
            ImGui::Text("Status: %s", m_GameExporter->GetExportStatus().c_str());
        }
        
        ImGui::Separator();
        
        // Action Buttons
        bool canExport = !m_GameExporter->IsExporting() && strlen(m_GameNameBuffer) > 0;
        
        if (!canExport) {
            ImGui::BeginDisabled();
        }
        
        if (ImGui::Button("Export Game", ImVec2(120, 0))) {
            // Prepare export settings
            ExportSettings settings;
            settings.gameName = m_GameNameBuffer;
            settings.outputDirectory = m_OutputPathBuffer;
            settings.numberOfLevels = m_NumLevels;
            settings.enemiesPerLevel = m_EnemiesPerLevel;
            settings.includeAssets = m_IncludeAssets;
            settings.compressAssets = m_CompressAssets;
            settings.version = MIST_ENGINE_VERSION_STRING;  // Use actual MistEngine version
            settings.weaponTypes = {"Pistol", "Rifle", "Shotgun", "Sniper"};
            
            // Start export
            m_ConsoleMessages.push_back("Starting game export...");
            m_ConsoleMessages.push_back("Game: " + std::string(m_GameNameBuffer));
            m_ConsoleMessages.push_back("Output: " + std::string(m_OutputPathBuffer));
            
            bool success = m_GameExporter->ExportGame(settings);
            if (success) {
                m_ConsoleMessages.push_back("Export completed successfully!");
            } else {
                m_ConsoleMessages.push_back("Export failed!");
            }
        }
        
        if (!canExport) {
            ImGui::EndDisabled();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_ShowExportDialog = false;
        }
        
        // Help text
        if (strlen(m_GameNameBuffer) == 0) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Please enter a game name to enable export");
        }
        
        ImGui::Spacing();
        if (ImGui::CollapsingHeader("Help")) {
            ImGui::TextWrapped("This will create a complete standalone game package including:");
            ImGui::BulletText("Game executable (currently creates placeholder)");
            ImGui::BulletText("All necessary configuration files");
            ImGui::BulletText("Level data with enemy spawn points");
            ImGui::BulletText("Asset files (textures, models, sounds)");
            ImGui::BulletText("Launcher script for easy starting");
            ImGui::BulletText("Complete README with instructions");
            
            ImGui::Spacing();
            ImGui::TextWrapped("In a production version, this would:");
            ImGui::BulletText("Copy the actual compiled game executable");
            ImGui::BulletText("Package real assets and resources");
            ImGui::BulletText("Create proper installers if requested");
            ImGui::BulletText("Handle dependency distribution");
        }
        
        ImGui::EndPopup();
    }
}

// FPS-specific UI methods deleted. The blocks below are stubbed out so that
// any stale include-chain still compiles; real removal of their declarations
// happens in UIManager.h in the same commit.
#if 0
void UIManager::DrawFPSGameLauncher() {
    // Create a prominent window for FPS game controls
    ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;
    
    if (ImGui::Begin("?? FPS Game Controller", nullptr, window_flags)) {
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.4f, 1.0f)); // Bright green
        ImGui::Text("MistEngine FPS Game Mode");
        ImGui::PopStyleColor();
        
        ImGui::Separator();
        
        if (m_FPSGameManager) {
            bool isGameActive = m_FPSGameManager->IsGameActive();
            bool isGamePaused = m_FPSGameManager->IsGamePaused();
            
            if (!isGameActive) {
                // Game not started - show start button
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.8f, 0.0f, 1.0f)); // Green
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.6f, 0.0f, 1.0f));
                
                if (ImGui::Button("?? START FPS GAME", ImVec2(200, 50))) {
                    m_ConsoleMessages.push_back("?? STARTING FPS GAME FROM UI BUTTON!");
                    m_FPSGameManager->StartNewGame();
                    
                    // Note: Scene clearing would be implemented here in a full version
                    m_ConsoleMessages.push_back("FPS Game starting - clearing editor scene");
                }
                ImGui::PopStyleColor(3);
                
                ImGui::Spacing();
                ImGui::Text("Click the button above to start your FPS adventure!");
                ImGui::Text("Game Features:");
                ImGui::BulletText("9 Enemy AI opponents");
                ImGui::BulletText("Multiple weapon types");
                ImGui::BulletText("Physics-based combat");
                ImGui::BulletText("Score and health system");
                
            } else if (isGamePaused) {
                // Game is paused - show resume button
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.6f, 0.8f, 1.0f)); // Blue
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.8f, 1.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.4f, 0.6f, 1.0f));
                
                if (ImGui::Button("?? RESUME GAME", ImVec2(150, 40))) {
                    m_FPSGameManager->ResumeGame();
                    m_ConsoleMessages.push_back("Game resumed from UI");
                }
                ImGui::PopStyleColor(3);
                
                ImGui::SameLine();
                
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.0f, 1.0f)); // Orange
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.3f, 0.0f, 1.0f));
                
                if (ImGui::Button("?? RESTART", ImVec2(100, 40))) {
                    m_FPSGameManager->RestartGame();
                    m_ConsoleMessages.push_back("Game restarted from UI");
                }
                ImGui::PopStyleColor(3);
                
                ImGui::Text("?? Game is PAUSED");
                
            } else {
                // Game is active - show pause and quit buttons
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.8f, 0.0f, 1.0f)); // Yellow
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.0f, 1.0f));
                
                if (ImGui::Button("?? PAUSE GAME", ImVec2(120, 40))) {
                    m_FPSGameManager->PauseGame();
                    m_ConsoleMessages.push_back("Game paused from UI");
                }
                ImGui::PopStyleColor(3);
                
                ImGui::SameLine();
                
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.0f, 0.0f, 1.0f)); // Red
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
                
                if (ImGui::Button("?? QUIT GAME", ImVec2(100, 40))) {
                    m_FPSGameManager->QuitGame();
                    m_ConsoleMessages.push_back("Game quit from UI");
                }
                ImGui::PopStyleColor(3);
                
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green
                ImGui::Text("?? GAME IS RUNNING!");
                ImGui::PopStyleColor();
                
                ImGui::Text("Controls:");
                ImGui::BulletText("WASD - Move player");
                ImGui::BulletText("Mouse - Look around");
                ImGui::BulletText("Left Click - Shoot");
                ImGui::BulletText("R - Reload weapon");
                ImGui::BulletText("1/2 - Switch weapons");
            }
            
            ImGui::Separator();
            
            // Game stats if available
            if (isGameActive && m_FPSGameManager->m_enemySystem) {
                int aliveEnemies = m_FPSGameManager->m_enemySystem->GetAliveEnemyCount();
                ImGui::Text("?? Enemies Remaining: %d", aliveEnemies);
                
                if (aliveEnemies == 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow
                    ImGui::Text("?? VICTORY! All enemies defeated!");
                    ImGui::PopStyleColor();
                }
            }
            
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red
            ImGui::Text("? FPS Game Manager not available");
            ImGui::Text("The FPS system is not properly initialized.");
            ImGui::PopStyleColor();
        }
        
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Press F3 to toggle Scene Editor mode");
    }
    
    ImGui::End();
}

void UIManager::DrawFPSGameHUD() {
    // Get screen size
    ImGuiIO& io = ImGui::GetIO();
    float screenWidth = io.DisplaySize.x;
    float screenHeight = io.DisplaySize.y;
    
    // Health bar (top-left)
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200, 60), ImGuiCond_Always);
    ImGui::Begin("Health", nullptr, 
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBackground);
    
    // Mock player health - in real implementation, get from PlayerComponent
    float playerHealth = 100.0f; // TODO: Get from actual player
    float maxHealth = 100.0f;
    
    ImGui::Text("HEALTH");
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // Red
    ImGui::ProgressBar(playerHealth / maxHealth, ImVec2(180, 20), "");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::Text("%.0f", playerHealth);
    
    ImGui::End();
    
    // Ammo counter (top-right)
    ImGui::SetNextWindowPos(ImVec2(screenWidth - 210, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200, 60), ImGuiCond_Always);
    ImGui::Begin("Ammo", nullptr, 
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBackground);
    
    // Mock ammo count - in real implementation, get from WeaponComponent  
    int currentAmmo = 30;
    int maxAmmo = 30;
    
    ImGui::Text("AMMO");
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.8f, 0.2f, 1.0f)); // Green
    ImGui::ProgressBar((float)currentAmmo / (float)maxAmmo, ImVec2(180, 20), "");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::Text("%d/%d", currentAmmo, maxAmmo);
    
    ImGui::End();
    
    // Score (bottom-left)
    ImGui::SetNextWindowPos(ImVec2(10, screenHeight - 70), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200, 60), ImGuiCond_Always);
    ImGui::Begin("Score", nullptr, 
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBackground);
    
    int playerScore = 0; // TODO: Get from actual player
    int enemiesKilled = 0;
    
    ImGui::Text("SCORE: %d", playerScore);
    ImGui::Text("KILLS: %d", enemiesKilled);
    
    ImGui::End();
}
#endif // FPS-specific UI methods

void UIManager::DrawCrosshair() {
    ImGuiIO& io = ImGui::GetIO();
    float screenWidth = io.DisplaySize.x;
    float screenHeight = io.DisplaySize.y;
    
    // Center of screen
    float centerX = screenWidth * 0.5f;
    float centerY = screenHeight * 0.5f;
    
    // Draw crosshair using ImGui draw list
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    
    float crosshairSize = 20.0f;
    float crosshairThickness = 2.0f;
    ImU32 crosshairColor = IM_COL32(255, 255, 255, 200); // White with transparency
    
    // Horizontal line
    drawList->AddLine(
        ImVec2(centerX - crosshairSize, centerY),
        ImVec2(centerX + crosshairSize, centerY),
        crosshairColor, crosshairThickness
    );
    
    // Vertical line
    drawList->AddLine(
        ImVec2(centerX, centerY - crosshairSize),
        ImVec2(centerX, centerY + crosshairSize),
        crosshairColor, crosshairThickness
    );
    
    // Center dot
    drawList->AddCircleFilled(
        ImVec2(centerX, centerY),
        2.0f,
        crosshairColor
    );
}

#if 0
void UIManager::DrawGameOverScreen() {
    if (!m_FPSGameManager) return;

    ImGuiIO& io = ImGui::GetIO();
    float screenWidth = io.DisplaySize.x;
    float screenHeight = io.DisplaySize.y;

    // Full screen overlay
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(screenWidth, screenHeight), ImGuiCond_Always);
    ImGui::Begin("GameOver", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar);

    // Dark overlay background
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        ImVec2(0, 0), ImVec2(screenWidth, screenHeight),
        IM_COL32(0, 0, 0, 180)
    );

    // Center the GAME OVER text
    ImGui::SetCursorPos(ImVec2(screenWidth * 0.5f - 150, screenHeight * 0.4f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
    ImGui::SetWindowFontScale(3.0f);
    ImGui::Text("GAME OVER");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // Restart button
    ImGui::SetCursorPos(ImVec2(screenWidth * 0.5f - 75, screenHeight * 0.6f));
    if (ImGui::Button("RESTART", ImVec2(150, 50))) {
    }

    ImGui::End();
}
#endif // DrawGameOverScreen

// --- Wired Editor Panels (from EditorUI.cpp functionality) ---

void UIManager::DrawProfilerWindow() {
    if (!m_Renderer) return;
    Profiler& profiler = m_Renderer->GetProfiler();

    ImGui::Begin("Profiler", &m_ShowProfiler);

    ImGui::Text("FPS: %.1f (%.2f ms)", profiler.GetFPS(), profiler.GetFrameTimeMs());
    ImGui::Text("Draw Calls: %d", profiler.GetDrawCalls());
    ImGui::Text("Triangles: %d", profiler.GetTriangles());

    ImGui::PlotLines("FPS", profiler.GetFPSHistory(), profiler.GetFPSHistorySize(),
        profiler.GetFPSHistoryOffset(), nullptr, 0.0f, 120.0f, ImVec2(0, 60));

    ImGui::Separator();
    if (ImGui::BeginTable("Sections", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Section");
        ImGui::TableSetupColumn("CPU (ms)");
        ImGui::TableSetupColumn("GPU (ms)");
        ImGui::TableHeadersRow();

        for (auto& s : profiler.GetSections()) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%s", s.name.c_str());
            ImGui::TableNextColumn(); ImGui::Text("%.2f", s.cpuTimeMs);
            ImGui::TableNextColumn(); ImGui::Text("%.2f", s.gpuTimeMs);
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void UIManager::DrawPostProcessControls() {
    if (!m_Renderer) return;
    PostProcessStack& postProcess = m_Renderer->GetPostProcess();
    float exposure = m_Renderer->GetExposure();

    ImGui::Begin("Post-Processing", &m_ShowPostProcess);

    if (ImGui::SliderFloat("Exposure", &exposure, 0.1f, 10.0f)) {
        m_Renderer->SetExposure(exposure);
    }

    ImGui::Separator();
    ImGui::Checkbox("Bloom", &postProcess.enableBloom);
    if (postProcess.enableBloom) {
        ImGui::SliderFloat("Bloom Threshold", &postProcess.bloom.threshold, 0.0f, 5.0f);
        ImGui::SliderFloat("Bloom Intensity", &postProcess.bloom.intensity, 0.0f, 3.0f);
    }

    ImGui::Separator();
    ImGui::Checkbox("SSAO", &postProcess.enableSSAO);
    if (postProcess.enableSSAO) {
        ImGui::SliderFloat("SSAO Radius", &postProcess.ssao.radius, 0.1f, 5.0f);
        ImGui::SliderFloat("SSAO Bias", &postProcess.ssao.bias, 0.001f, 0.1f);
    }

    ImGui::Separator();
    ImGui::Checkbox("TAA (Temporal AA)", &postProcess.enableTAA);
    if (postProcess.enableTAA) {
        postProcess.taa.enabled = true;
        postProcess.enableFXAA = false; // TAA replaces FXAA
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "TAA active (FXAA disabled)");
    } else {
        postProcess.taa.enabled = false;
        ImGui::Checkbox("FXAA", &postProcess.enableFXAA);
    }

    ImGui::Separator();
    ImGui::Checkbox("SSGI (Global Illumination)", &postProcess.enableSSGI);
    if (postProcess.enableSSGI) {
        postProcess.ssgi.enabled = true;
        ImGui::SliderFloat("GI Radius", &postProcess.ssgi.radius, 0.5f, 10.0f);
        ImGui::SliderFloat("GI Intensity", &postProcess.ssgi.intensity, 0.0f, 3.0f);
    } else {
        postProcess.ssgi.enabled = false;
    }

    ImGui::End();
}

void UIManager::DrawShadowControls() {
    if (!m_Renderer) return;
    ShadowSystem& shadows = m_Renderer->GetShadowSystem();

    ImGui::Begin("Shadow Controls", &m_ShowShadowControls);

    ImGui::Checkbox("Debug Cascade Colors", &shadows.showCascadeColors);

    auto& splits = shadows.GetCascadeSplits();
    ImGui::Text("Cascade splits:");
    for (int i = 0; i < ShadowSystem::NUM_CASCADES; i++) {
        ImGui::Text("  Cascade %d: %.2f", i, splits[i]);
    }

    ImGui::End();
}

void UIManager::DrawLightEditor() {
    if (!m_Renderer) return;
    LightManager& lights = m_Renderer->GetLightManager();

    ImGui::Begin("Light Editor", &m_ShowLightEditor);

    ImGui::Text("Active lights: %d / %d", lights.GetLightCount(), LightManager::MAX_LIGHTS);

    if (ImGui::Button("Add Point Light")) {
        Light light;
        light.position = glm::vec4(0, 5, 0, 1.0f);
        light.color = glm::vec4(1, 1, 1, 5.0f);
        light.params = glm::vec4(20.0f, 0, -1, 0);
        lights.AddLight(light);
        m_ConsoleMessages.push_back("Added point light");
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Spot Light")) {
        Light light;
        light.position = glm::vec4(0, 5, 0, 2.0f);
        light.direction = glm::vec4(0, -1, 0, glm::cos(glm::radians(25.0f)));
        light.color = glm::vec4(1, 1, 1, 10.0f);
        light.params = glm::vec4(30.0f, glm::cos(glm::radians(35.0f)), -1, 0);
        lights.AddLight(light);
        m_ConsoleMessages.push_back("Added spot light");
    }

    ImGui::Separator();

    for (int i = 0; i < lights.GetLightCount(); i++) {
        ImGui::PushID(i);
        Light& l = lights.GetLight(i);
        if (ImGui::TreeNode("Light", "Light %d", i)) {
            ImGui::DragFloat3("Position", glm::value_ptr(l.position), 0.1f);
            ImGui::ColorEdit3("Color", glm::value_ptr(l.color));
            ImGui::DragFloat("Intensity", &l.color.w, 0.1f, 0.0f, 100.0f);
            ImGui::DragFloat("Range", &l.params.x, 0.1f, 0.1f, 200.0f);

            if (ImGui::Button("Remove")) {
                lights.RemoveLight(i);
                ImGui::TreePop();
                ImGui::PopID();
                break;
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    ImGui::End();
}

void UIManager::DrawSkyboxControls() {
    if (!m_Renderer) return;
    SkyboxRenderer& skybox = m_Renderer->GetSkybox();

    ImGui::Begin("Skybox Controls", &m_ShowSkyboxControls);

    const char* modes[] = {"Procedural", "HDR Cubemap", "Atmospheric"};
    int currentMode = static_cast<int>(skybox.GetMode());
    if (ImGui::Combo("Mode", &currentMode, modes, 3)) {
        skybox.SetMode(static_cast<SkyboxMode>(currentMode));
    }

    if (skybox.GetMode() == SkyboxMode::Atmospheric) {
        ImGui::DragFloat3("Sun Direction", glm::value_ptr(skybox.sunDirection), 0.01f, -1.0f, 1.0f);
        skybox.sunDirection = glm::normalize(skybox.sunDirection);
        ImGui::SliderFloat("Turbidity", &skybox.turbidity, 1.0f, 10.0f);
        ImGui::SliderFloat("Rayleigh", &skybox.rayleighStrength, 0.0f, 5.0f);
        ImGui::SliderFloat("Mie", &skybox.mieStrength, 0.0f, 0.1f);
    }

    ImGui::End();
}

void UIManager::SetEntityName(Entity entity, const std::string& name) {
    m_EntityNames[entity] = name;
    m_EntityCounter = std::max(m_EntityCounter, (int)entity + 1);
}

// --- Toolbar ---

void UIManager::DrawToolbar() {
    ImGuiIO& io = ImGui::GetIO();
    float menuBarH = m_Layout.menuBarHeight;
    float toolbarH = m_Layout.toolbarHeight;

    ImGui::SetNextWindowPos(ImVec2(0, menuBarH));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, toolbarH));
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 4));
    ImGui::Begin("##Toolbar", nullptr, flags);

    // Transform tools
    auto activeBtn = [](const char* label, bool active) {
        if (active) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.55f, 0.75f, 1.0f));
        }
        bool clicked = ImGui::Button(label, ImVec2(0, 24));
        if (active) ImGui::PopStyleColor(2);
        return clicked;
    };

    GizmoMode currentMode = m_GizmoSystem ? m_GizmoSystem->GetMode() : GizmoMode::Translate;
    if (activeBtn("Move (W)", currentMode == GizmoMode::Translate) && m_GizmoSystem)
        m_GizmoSystem->SetMode(GizmoMode::Translate);
    ImGui::SameLine();
    if (activeBtn("Rotate (E)", currentMode == GizmoMode::Rotate) && m_GizmoSystem)
        m_GizmoSystem->SetMode(GizmoMode::Rotate);
    ImGui::SameLine();
    if (activeBtn("Scale (R)", currentMode == GizmoMode::Scale) && m_GizmoSystem)
        m_GizmoSystem->SetMode(GizmoMode::Scale);

    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();

    // Play / Pause / Stop using EditorState
    EditorPlayState playState = m_EditorState ? m_EditorState->GetState() : EditorPlayState::Edit;

    ImGui::PushStyleColor(ImGuiCol_Button, playState == EditorPlayState::Playing
        ? ImVec4(0.2f, 0.6f, 0.2f, 1.0f) : ImVec4(0.24f, 0.26f, 0.30f, 1.0f));
    if (ImGui::Button("Play", ImVec2(0, 24)) && m_EditorState) {
        m_EditorState->Play();
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, playState == EditorPlayState::Paused
        ? ImVec4(0.6f, 0.6f, 0.2f, 1.0f) : ImVec4(0.24f, 0.26f, 0.30f, 1.0f));
    if (ImGui::Button("Pause", ImVec2(0, 24)) && m_EditorState) {
        m_EditorState->Pause();
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    if (ImGui::Button("Stop", ImVec2(0, 24)) && m_EditorState) m_EditorState->Stop();

    // FPS counter on right side
    ImGui::SameLine(io.DisplaySize.x - 120);
    if (m_Renderer) {
        ImGui::TextDisabled("%.1f FPS", m_Renderer->GetProfiler().GetFPS());
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

// --- Dockable Layout ---
//
// All panels below are plain ImGui windows. ImGui's dock system
// (enabled via `ImGuiConfigFlags_DockingEnable` in Initialize)
// manages their positions based on the DockSpaceOverViewport set up
// by BuildDockspace below. The first-run layout is programmatic; on
// subsequent launches ImGui restores from `imgui_layout.ini`.

void UIManager::DrawEditorLayout() {
    // Host dockspace occupies the entire main viewport's work area
    // (below the menu bar). Each panel below calls Begin/End with a
    // stable name — the dock system binds them by name to the slots
    // we built in BuildDockspace() on first run.
    ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(
        0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    static bool builtOnce = false;
    if (!builtOnce) {
        builtOnce = true;
        // Only build the default layout if no prior layout exists —
        // otherwise we'd stomp the user's saved arrangement every
        // restart.
        if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr
            || ImGui::DockBuilderGetNode(dockspaceId)->ChildNodes[0] == nullptr) {
            ImGui::DockBuilderRemoveNode(dockspaceId);
            ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);

            ImGuiID left    = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left,  0.18f, nullptr, &dockspaceId);
            ImGuiID right   = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Right, 0.22f / (1.0f - 0.18f), nullptr, &dockspaceId);
            ImGuiID bottom  = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Down,  0.28f, nullptr, &dockspaceId);

            ImGui::DockBuilderDockWindow("Hierarchy",      left);
            ImGui::DockBuilderDockWindow("Inspector",      right);
            ImGui::DockBuilderDockWindow("##BottomPanel",  bottom);
            ImGui::DockBuilderDockWindow("Viewport",       dockspaceId);
            ImGui::DockBuilderFinish(dockspaceId);
        }
    }

    // === LEFT PANEL (Hierarchy) — docks to the left slot ===
    if (m_Layout.leftPanelVisible) {
        ImGui::Begin("Hierarchy", &m_Layout.leftPanelVisible);
        DrawHierarchy();
        ImGui::End();
    }

    // === CENTER PANEL (Viewport) ===
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoCollapse);

        if (m_ViewportTexture != 0) {
            ImVec2 available = ImGui::GetContentRegionAvail();
            float aspect = (m_ViewportHeight > 0) ? (float)m_ViewportWidth / (float)m_ViewportHeight : 16.0f / 9.0f;
            float displayW2 = available.x;
            float displayH2 = displayW2 / aspect;
            if (displayH2 > available.y) {
                displayH2 = available.y;
                displayW2 = displayH2 * aspect;
            }
            // Center the image
            float offsetX = (available.x - displayW2) * 0.5f;
            float offsetY = (available.y - displayH2) * 0.5f;
            ImVec2 imgCursor(ImGui::GetCursorPosX() + offsetX, ImGui::GetCursorPosY() + offsetY);
            ImGui::SetCursorPos(imgCursor);
            // Screen-space rectangle of the image, used by ImGuizmo
            // to hit-test and draw into the viewport overlay.
            ImVec2 imgScreenPos = ImGui::GetCursorScreenPos();
            ImGui::Image((ImTextureID)(intptr_t)m_ViewportTexture,
                ImVec2(displayW2, displayH2),
                ImVec2(0, 1), ImVec2(1, 0));

            // Asset → viewport drop target. Accepts the same
            // `ASSET_PATH` payload that the asset browser panels
            // emit, routes by extension inside HandleAssetDrop.
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* p =
                        ImGui::AcceptDragDropPayload("ASSET_PATH")) {
                    std::string dropped(
                        static_cast<const char*>(p->Data),
                        p->DataSize ? static_cast<size_t>(p->DataSize) - 1 : 0);
                    HandleAssetDrop(dropped);
                }
                ImGui::EndDragDropTarget();
            }

            // Gizmo overlay — renders the translate/rotate/scale
            // handles on top of the viewport image. No-ops when no
            // entity is selected or the entity has no transform.
            if (m_HasSelectedEntity && m_Coordinator && m_Renderer
                && m_Coordinator->HasComponent<TransformComponent>(m_SelectedEntity)
                && m_GizmoSystem) {
                GizmoSystem::BeginFrame(imgScreenPos.x, imgScreenPos.y, displayW2, displayH2);

                auto& cam  = m_Renderer->GetCamera();
                auto& t    = m_Coordinator->GetComponent<TransformComponent>(m_SelectedEntity);
                float camAspect = displayH2 > 0 ? displayW2 / displayH2 : 16.0f / 9.0f;
                glm::mat4 view  = cam.GetViewMatrix();
                glm::mat4 proj  = cam.GetProjectionMatrix(camAspect);
                glm::mat4 model = t.GetModelMatrix();

                // Gizmo drag undo: capture pre-drag state at the
                // transition from "not using" to "using". The check
                // runs BEFORE Manipulate so the capture is the true
                // pre-drag transform, not the first-frame-applied
                // one. One undo step per drag regardless of frame count.
                bool wasUsing    = m_GizmoCapture.active;
                bool isUsingNow  = GizmoSystem::IsUsing();
                if (!wasUsing && isUsingNow) {
                    m_GizmoCapture.active   = true;
                    m_GizmoCapture.entity   = m_SelectedEntity;
                    m_GizmoCapture.position = t.position;
                    m_GizmoCapture.rotation = t.rotation;
                    m_GizmoCapture.scale    = t.scale;
                }

                if (m_GizmoSystem->Manipulate(view, proj, model)) {
                    float tr[3], rot[3], sc[3];
                    ImGuizmo::DecomposeMatrixToComponents(
                        glm::value_ptr(model), tr, rot, sc);
                    t.position = {tr[0],  tr[1],  tr[2]};
                    t.rotation = {rot[0], rot[1], rot[2]};
                    t.scale    = {sc[0],  sc[1],  sc[2]};
                    t.dirty    = true;
                }

                // Drag ended this frame — push the undo command.
                if (wasUsing && !GizmoSystem::IsUsing() && m_GizmoCapture.active) {
                    Coordinator* coord = m_Coordinator;
                    Entity ent         = m_GizmoCapture.entity;
                    glm::vec3 oldPos   = m_GizmoCapture.position;
                    glm::vec3 oldRot   = m_GizmoCapture.rotation;
                    glm::vec3 oldScale = m_GizmoCapture.scale;
                    glm::vec3 newPos   = t.position;
                    glm::vec3 newRot   = t.rotation;
                    glm::vec3 newScale = t.scale;

                    Mist::Editor::Command c;
                    switch (m_GizmoSystem->GetMode()) {
                        case GizmoMode::Translate: c.label = "Translate"; break;
                        case GizmoMode::Rotate:    c.label = "Rotate";    break;
                        case GizmoMode::Scale:     c.label = "Scale";     break;
                    }
                    c.merge_key = (static_cast<std::uint64_t>(ent) << 8)
                                | (static_cast<std::uint64_t>(m_GizmoSystem->GetMode()) + 0x10);
                    c.redo = [coord, ent, newPos, newRot, newScale]() {
                        if (coord->HasComponent<TransformComponent>(ent)) {
                            auto& tt = coord->GetComponent<TransformComponent>(ent);
                            tt.position = newPos; tt.rotation = newRot; tt.scale = newScale;
                            tt.dirty = true;
                        }
                    };
                    c.undo = [coord, ent, oldPos, oldRot, oldScale]() {
                        if (coord->HasComponent<TransformComponent>(ent)) {
                            auto& tt = coord->GetComponent<TransformComponent>(ent);
                            tt.position = oldPos; tt.rotation = oldRot; tt.scale = oldScale;
                            tt.dirty = true;
                        }
                    };
                    m_UndoStack.Push(std::move(c));
                    m_GizmoCapture.active = false;
                }
            } else if (m_GizmoCapture.active && !GizmoSystem::IsUsing()) {
                // Selection lost mid-drag (or entity deleted) — drop
                // the capture so a stale entity doesn't get undo'd.
                m_GizmoCapture.active = false;
            }
        } else {
            ImGui::Text("No viewport texture");
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    // === RIGHT PANEL (Inspector) ===
    if (m_Layout.rightPanelVisible) {
        ImGui::Begin("Inspector", &m_Layout.rightPanelVisible);
        DrawInspector();
        ImGui::End();
    }

    // === BOTTOM PANEL (Console / Asset Browser / Output tabs) ===
    if (m_Layout.bottomPanelVisible) {
        ImGui::Begin("##BottomPanel", &m_Layout.bottomPanelVisible);

        if (ImGui::BeginTabBar("BottomTabs")) {
            if (ImGui::BeginTabItem("Console")) {
                m_BottomTabIndex = 0;
                // Embedded console content
                ImGui::BeginChild("ConsoleScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
                for (const auto& msg : m_ConsoleMessages) {
                    ImGui::TextUnformatted(msg.c_str());
                }
                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                    ImGui::SetScrollHereY(1.0f);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Asset Browser")) {
                m_BottomTabIndex = 1;
                // Embedded asset browser
                if (m_AssetBrowser) {
                    if (m_AssetBrowser->CanNavigateUp()) {
                        if (ImGui::Button("<- Back")) m_AssetBrowser->NavigateUp();
                        ImGui::SameLine();
                    }
                    if (ImGui::Button("Refresh")) m_AssetBrowser->Refresh();
                    ImGui::SameLine();
                    ImGui::Text("Path: %s", m_AssetBrowser->GetRelativePath().c_str());
                    ImGui::Separator();

                    static char filterBuf[128] = "";
                    ImGui::SetNextItemWidth(200);
                    ImGui::InputText("Filter", filterBuf, sizeof(filterBuf));
                    m_AssetBrowser->SetFilter(filterBuf);
                    ImGui::Separator();

                    auto entries = m_AssetBrowser->GetFilteredEntries();
                    int columns = std::max(1, (int)(ImGui::GetContentRegionAvail().x / 100.0f));
                    if (ImGui::BeginTable("AssetGrid", columns)) {
                        int col = 0;
                        for (auto& entry : entries) {
                            if (col == 0) ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(col);
                            ImGui::PushID(entry.fullPath.c_str());
                            if (entry.isDirectory) {
                                if (ImGui::Button(("[DIR] " + entry.name).c_str(), ImVec2(90, 40))) {
                                    m_AssetBrowser->NavigateTo(entry.fullPath);
                                    ImGui::PopID();
                                    break;
                                }
                            } else {
                                ImVec4 color(0.8f, 0.8f, 0.8f, 1.0f);
                                if (entry.extension == ".glsl" || entry.extension == ".frag" || entry.extension == ".vert" || entry.extension == ".comp")
                                    color = ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
                                else if (entry.extension == ".jpg" || entry.extension == ".png" || entry.extension == ".hdr")
                                    color = ImVec4(0.8f, 0.6f, 0.2f, 1.0f);
                                else if (entry.extension == ".obj" || entry.extension == ".fbx" || entry.extension == ".gltf")
                                    color = ImVec4(0.2f, 0.6f, 0.8f, 1.0f);
                                else if (entry.extension == ".json")
                                    color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);

                                ImGui::PushStyleColor(ImGuiCol_Text, color);
                                ImGui::Selectable(entry.name.c_str(), false, 0, ImVec2(90, 20));
                                ImGui::PopStyleColor();
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip("%s\nSize: %lu bytes", entry.fullPath.c_str(), (unsigned long)entry.fileSize);
                                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                                    ImGui::SetDragDropPayload("ASSET_PATH", entry.fullPath.c_str(), entry.fullPath.size() + 1);
                                    ImGui::Text("Drag: %s", entry.name.c_str());
                                    ImGui::EndDragDropSource();
                                }
                            }
                            ImGui::PopID();
                            col = (col + 1) % columns;
                        }
                        ImGui::EndTable();
                    }
                    ImGui::Text("%zu items", entries.size());
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Output")) {
                m_BottomTabIndex = 2;
                if (m_Renderer) {
                    Profiler& profiler = m_Renderer->GetProfiler();
                    ImGui::Text("FPS: %.1f  Frame: %.2f ms  Draw Calls: %d",
                        profiler.GetFPS(), profiler.GetFrameTimeMs(), profiler.GetDrawCalls());
                    ImGui::Separator();
                    for (auto& s : profiler.GetSections()) {
                        ImGui::Text("  %s: CPU %.2fms GPU %.2fms", s.name.c_str(), s.cpuTimeMs, s.gpuTimeMs);
                    }
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
    }

}

// --- Scene Serialization ---

void UIManager::SaveScene(const std::string& path) {
    if (!m_Coordinator) {
        m_ConsoleMessages.push_back("Cannot save: no coordinator");
        return;
    }

    extern Coordinator gCoordinator;
    if (SceneSerializer::Save(path, gCoordinator, m_EntityCounter)) {
        m_ConsoleMessages.push_back("Scene saved to: " + path);
        // Clean the dirty marker so the title's `*` clears on next
        // frame. Load does the same via Clear() below.
        m_UndoStack.MarkSaved();
    } else {
        m_ConsoleMessages.push_back("Failed to save scene to: " + path);
    }
}

void UIManager::LoadScene(const std::string& path) {
    if (!m_Coordinator) {
        m_ConsoleMessages.push_back("Cannot load: no coordinator");
        return;
    }

    extern Coordinator gCoordinator;
    int entityCount = 0;
    if (SceneSerializer::Load(path, gCoordinator, entityCount)) {
        m_EntityCounter = entityCount;
        m_HasSelectedEntity = false;
        m_ConsoleMessages.push_back("Scene loaded from: " + path);
        // New scene = fresh history. Undoing back into the previous
        // scene's entities would produce zombie ids.
        m_UndoStack.Clear();
    } else {
        m_ConsoleMessages.push_back("Failed to load scene from: " + path);
    }
}

