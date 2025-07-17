#include "UI/EditorUI.h"
#include "Config.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>

#if defined(IMGUI_ENABLED) && IMGUI_ENABLED
#include "ECS/Coordinator.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "Scene.h"
#include "Camera.h"
#include "PhysicsSystem.h"

extern Coordinator gCoordinator;
#endif

EditorUI::EditorUI() {
    m_consoleMessages.reserve(1000);
    AddConsoleMessage("Editor UI initialized");
}

EditorUI::~EditorUI() {
    Shutdown();
}

bool EditorUI::Init(GLFWwindow* window) {
#if defined(IMGUI_ENABLED) && IMGUI_ENABLED
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Dear ImGui style
    SetupImGuiStyle();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    AddConsoleMessage("ImGui initialized successfully");
#else
    AddConsoleMessage("ImGui not available - UI disabled");
#endif
    return true;
}

void EditorUI::Update(float deltaTime) {
#if defined(IMGUI_ENABLED) && IMGUI_ENABLED
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Enable docking
    SetupDockSpace();

    // Render all UI panels
    RenderMainMenuBar();
    RenderSceneHierarchy();
    RenderInspector();
    RenderAssetBrowser();
    RenderConsole();
    RenderViewport();
    RenderStats();

    if (m_showDemoWindow) {
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }
#endif
}

void EditorUI::Render() {
#if defined(IMGUI_ENABLED) && IMGUI_ENABLED
    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
#endif
}

void EditorUI::Shutdown() {
#if defined(IMGUI_ENABLED) && IMGUI_ENABLED
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
#endif
}

void EditorUI::SetSelectedEntity(Entity entity) {
    m_selectedEntity = entity;
    AddConsoleMessage("Selected entity: " + std::to_string(entity));
}

void EditorUI::AddConsoleMessage(const std::string& message) {
    std::stringstream ss;
    ss << "[" << std::fixed << std::setprecision(3) << 0.0f << "] " << message;
    m_consoleMessages.push_back(ss.str());
    
    // Keep only last 1000 messages
    if (m_consoleMessages.size() > 1000) {
        m_consoleMessages.erase(m_consoleMessages.begin());
    }
    
    // Also log to console
    std::cout << ss.str() << std::endl;
}

#if defined(IMGUI_ENABLED) && IMGUI_ENABLED

void EditorUI::SetupDockSpace() {
    static bool dockspaceOpen = true;
    static bool opt_fullscreen_persistant = true;
    bool opt_fullscreen = opt_fullscreen_persistant;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen) {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    ImGui::End();
}

void EditorUI::RenderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                AddConsoleMessage("New Scene created");
            }
            if (ImGui::MenuItem("Open Scene", "Ctrl+O")) {
                AddConsoleMessage("Open Scene dialog");
            }
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                AddConsoleMessage("Scene saved");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                // Handle exit
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                AddConsoleMessage("Undo action");
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                AddConsoleMessage("Redo action");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                AddConsoleMessage("Copy");
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                AddConsoleMessage("Paste");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("GameObject")) {
            if (ImGui::MenuItem("Create Empty")) {
                CreateEntity("Empty GameObject");
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("3D Object")) {
                if (ImGui::MenuItem("Cube")) {
                    CreatePrimitive("Cube");
                }
                if (ImGui::MenuItem("Sphere")) {
                    CreatePrimitive("Sphere");
                }
                if (ImGui::MenuItem("Plane")) {
                    CreatePrimitive("Plane");
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("Scene Hierarchy", nullptr, &m_showSceneHierarchy);
            ImGui::MenuItem("Inspector", nullptr, &m_showInspector);
            ImGui::MenuItem("Asset Browser", nullptr, &m_showAssetBrowser);
            ImGui::MenuItem("Console", nullptr, &m_showConsole);
            ImGui::MenuItem("Stats", nullptr, &m_showStats);
            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo", nullptr, &m_showDemoWindow);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void EditorUI::RenderSceneHierarchy() {
    if (!m_showSceneHierarchy) return;

    ImGui::Begin("Scene Hierarchy", &m_showSceneHierarchy);

    // Toolbar
    if (ImGui::Button("Create")) {
        CreateEntity("New Entity");
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete") && m_selectedEntity != 0) {
        DeleteEntity(m_selectedEntity);
    }
    ImGui::SameLine();
    if (ImGui::Button("Duplicate") && m_selectedEntity != 0) {
        DuplicateEntity(m_selectedEntity);
    }

    ImGui::Separator();

    // Scene tree
    if (m_coordinator && ImGui::TreeNode("Scene")) {
        // Get all entities (simplified - in a real implementation you'd have a proper scene graph)
        for (Entity entity = 1; entity < 100; ++entity) {
            try {
                // Try to get transform component to see if entity exists
                auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);
                
                std::string entityName = "Entity " + std::to_string(entity);
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                
                if (entity == m_selectedEntity) {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                ImGui::TreeNodeEx((void*)(intptr_t)entity, flags, entityName.c_str());
                
                if (ImGui::IsItemClicked()) {
                    SetSelectedEntity(entity);
                }

                // Context menu
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Delete")) {
                        DeleteEntity(entity);
                    }
                    if (ImGui::MenuItem("Duplicate")) {
                        DuplicateEntity(entity);
                    }
                    ImGui::EndPopup();
                }
            } catch (...) {
                // Entity doesn't exist, skip
                continue;
            }
        }
        ImGui::TreePop();
    }

    ImGui::End();
}

void EditorUI::RenderInspector() {
    if (!m_showInspector) return;

    ImGui::Begin("Inspector", &m_showInspector);

    if (m_selectedEntity != 0 && m_coordinator) {
        ImGui::Text("Entity: %u", m_selectedEntity);
        ImGui::Separator();

        // Transform Component
        try {
            auto& transform = m_coordinator->GetComponent<TransformComponent>(m_selectedEntity);
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                RenderTransformComponent(m_selectedEntity);
            }
        } catch (...) {
            if (ImGui::Button("Add Transform Component")) {
                m_coordinator->AddComponent(m_selectedEntity, TransformComponent{});
                AddConsoleMessage("Added Transform component to entity " + std::to_string(m_selectedEntity));
            }
        }

        // Render Component
        try {
            auto& render = m_coordinator->GetComponent<RenderComponent>(m_selectedEntity);
            if (ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_DefaultOpen)) {
                RenderRenderComponent(m_selectedEntity);
            }
        } catch (...) {
            if (ImGui::Button("Add Render Component")) {
                m_coordinator->AddComponent(m_selectedEntity, RenderComponent{});
                AddConsoleMessage("Added Render component to entity " + std::to_string(m_selectedEntity));
            }
        }

        // Physics Component
        try {
            auto& physics = m_coordinator->GetComponent<PhysicsComponent>(m_selectedEntity);
            if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
                RenderPhysicsComponent(m_selectedEntity);
            }
        } catch (...) {
            if (ImGui::Button("Add Physics Component")) {
                m_coordinator->AddComponent(m_selectedEntity, PhysicsComponent{});
                AddConsoleMessage("Added Physics component to entity " + std::to_string(m_selectedEntity));
            }
        }

        ImGui::Separator();
        if (ImGui::Button("Remove Entity")) {
            DeleteEntity(m_selectedEntity);
        }
    } else {
        ImGui::Text("No entity selected");
    }

    ImGui::End();
}

void EditorUI::RenderAssetBrowser() {
    if (!m_showAssetBrowser) return;

    ImGui::Begin("Asset Browser", &m_showAssetBrowser);

    // Toolbar
    if (ImGui::Button("Import")) {
        AddConsoleMessage("Import asset dialog");
    }
    ImGui::SameLine();
    if (ImGui::Button("Create")) {
        AddConsoleMessage("Create asset menu");
    }
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        AddConsoleMessage("Refreshing assets...");
    }

    ImGui::Separator();

    // Asset folders
    if (ImGui::TreeNode("Assets")) {
        if (ImGui::TreeNode("Models")) {
            ImGui::Text("backpack.obj");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Textures")) {
            ImGui::Text("container.jpg");
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Shaders")) {
            ImGui::Text("vertex.glsl");
            ImGui::Text("fragment.glsl");
            ImGui::TreePop();
        }
        ImGui::TreePop();
    }

    ImGui::End();
}

void EditorUI::RenderConsole() {
    if (!m_showConsole) return;

    ImGui::Begin("Console", &m_showConsole);

    // Toolbar
    if (ImGui::Button("Clear")) {
        m_consoleMessages.clear();
    }
    ImGui::SameLine();
    static bool autoScroll = true;
    ImGui::Checkbox("Auto-scroll", &autoScroll);

    ImGui::Separator();

    // Console output
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    
    for (const auto& message : m_consoleMessages) {
        ImGui::TextUnformatted(message.c_str());
    }

    if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();
}

void EditorUI::RenderViewport() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Check if viewport is focused and hovered
    m_viewportFocused = ImGui::IsWindowFocused();
    m_viewportHovered = ImGui::IsWindowHovered();

    // Get viewport size
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    
    // Display viewport info
    ImGui::Text("Viewport Size: %.0f x %.0f", viewportSize.x, viewportSize.y);
    ImGui::Text("Focused: %s | Hovered: %s", 
                m_viewportFocused ? "Yes" : "No", 
                m_viewportHovered ? "Yes" : "No");

    // Instructions
    if (!m_viewportFocused) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Click here to focus viewport");
    }
    if (m_viewportFocused) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Viewport focused - WASD to move camera");
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Press TAB to toggle cursor mode");
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void EditorUI::RenderStats() {
    if (!m_showStats) return;

    ImGui::Begin("Stats", &m_showStats);

    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::Text("Entities: %d", 0); // TODO: Get actual entity count
    ImGui::Text("Draw Calls: %d", 0); // TODO: Get actual draw calls
    ImGui::Text("Vertices: %d", 0); // TODO: Get actual vertex count

    ImGui::End();
}

void EditorUI::RenderTransformComponent(Entity entity) {
    if (!m_coordinator) return;

    try {
        auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);
        
        DrawVec3Control("Position", transform.position);
        DrawVec3Control("Rotation", transform.rotation);
        DrawVec3Control("Scale", transform.scale, 1.0f);
        
    } catch (...) {
        ImGui::Text("Transform component not found");
    }
}

void EditorUI::RenderRenderComponent(Entity entity) {
    if (!m_coordinator) return;

    try {
        auto& render = m_coordinator->GetComponent<RenderComponent>(entity);
        
        DrawBoolControl("Visible", render.visible);
        
        // Display renderable info
        if (render.renderable) {
            ImGui::Text("Renderable: Present");
        } else {
            ImGui::Text("Renderable: None");
        }
        
    } catch (...) {
        ImGui::Text("Render component not found");
    }
}

void EditorUI::RenderPhysicsComponent(Entity entity) {
    if (!m_coordinator) return;

    try {
        auto& physics = m_coordinator->GetComponent<PhysicsComponent>(entity);
        
        DrawBoolControl("Sync Transform", physics.syncTransform);
        
        // Display physics body info
        if (physics.rigidBody) {
            ImGui::Text("Rigid Body: Present");
            float mass = physics.rigidBody->getMass();
            ImGui::Text("Mass: %.3f", mass);
        } else {
            ImGui::Text("Rigid Body: None");
        }
        
    } catch (...) {
        ImGui::Text("Physics component not found");
    }
}

void EditorUI::CreateEntity(const std::string& name) {
    if (!m_coordinator) return;

    Entity entity = m_coordinator->CreateEntity();
    m_coordinator->AddComponent(entity, TransformComponent{});
    
    AddConsoleMessage("Created entity: " + name + " (ID: " + std::to_string(entity) + ")");
    SetSelectedEntity(entity);
}

void EditorUI::DeleteEntity(Entity entity) {
    if (!m_coordinator) return;

    m_coordinator->DestroyEntity(entity);
    AddConsoleMessage("Deleted entity: " + std::to_string(entity));
    
    if (m_selectedEntity == entity) {
        m_selectedEntity = 0;
    }
}

void EditorUI::DuplicateEntity(Entity entity) {
    if (!m_coordinator) return;

    Entity newEntity = m_coordinator->CreateEntity();
    
    // Copy components
    try {
        auto& transform = m_coordinator->GetComponent<TransformComponent>(entity);
        m_coordinator->AddComponent(newEntity, transform);
    } catch (...) {}
    
    try {
        auto& render = m_coordinator->GetComponent<RenderComponent>(entity);
        m_coordinator->AddComponent(newEntity, render);
    } catch (...) {}
    
    try {
        auto& physics = m_coordinator->GetComponent<PhysicsComponent>(entity);
        // Note: Can't easily duplicate physics bodies, so create new component without body
        m_coordinator->AddComponent(newEntity, PhysicsComponent{});
    } catch (...) {}
    
    AddConsoleMessage("Duplicated entity " + std::to_string(entity) + " to " + std::to_string(newEntity));
    SetSelectedEntity(newEntity);
}

void EditorUI::CreatePrimitive(const std::string& type) {
    CreateEntity(type);
    AddConsoleMessage("Created primitive: " + type);
}

void EditorUI::DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float columnWidth) {
    ImGuiIO& io = ImGui::GetIO();
    auto boldFont = io.Fonts->Fonts[0];

    ImGui::PushID(label.c_str());

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text(label.c_str());
    ImGui::NextColumn();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

    float lineHeight = ImGui::GetTextLineHeight() + ImGui::GetStyle().FramePadding.y * 2.0f;
    ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

    // X component
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
    ImGui::PushFont(boldFont);
    if (ImGui::Button("X", buttonSize))
        values.x = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::SameLine();

    // Y component
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Y", buttonSize))
        values.y = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::SameLine();

    // Z component
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Z", buttonSize))
        values.z = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");

    ImGui::PopStyleVar();

    ImGui::Columns(1);

    ImGui::PopID();
}

void EditorUI::DrawFloatControl(const std::string& label, float& value, float resetValue, float columnWidth) {
    ImGui::PushID(label.c_str());

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text(label.c_str());
    ImGui::NextColumn();

    ImGui::DragFloat("##value", &value, 0.1f, 0.0f, 0.0f, "%.2f");

    ImGui::Columns(1);
    ImGui::PopID();
}

void EditorUI::DrawBoolControl(const std::string& label, bool& value, float columnWidth) {
    ImGui::PushID(label.c_str());

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text(label.c_str());
    ImGui::NextColumn();

    ImGui::Checkbox("##value", &value);

    ImGui::Columns(1);
    ImGui::PopID();
}

void EditorUI::SetupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Modern dark theme colors
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.90f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.90f, 0.35f);

    style.WindowPadding = ImVec2(15, 15);
    style.WindowRounding = 5.0f;
    style.FramePadding = ImVec2(5, 5);
    style.FrameRounding = 4.0f;
    style.ItemSpacing = ImVec2(12, 8);
    style.ItemInnerSpacing = ImVec2(8, 6);
    style.IndentSpacing = 25.0f;
    style.ScrollbarSize = 15.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabMinSize = 5.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 4.0f;
    style.ChildRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
}

#endif // IMGUI_ENABLED