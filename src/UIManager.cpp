#include "UIManager.h"
#include "ECS/Coordinator.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "Scene.h"
#include "PhysicsSystem.h"
#include "Mesh.h"
#include "Texture.h"
#include "ShapeGenerator.h"
#include "AI/AIManager.h"
#include "AI/AIWindow.h"
#include "AI/AIConfig.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>

extern Coordinator gCoordinator;

UIManager::UIManager() 
    : m_ShowDemo(false)
    , m_ShowHierarchy(true)
    , m_ShowInspector(true)
    , m_ShowSceneView(false)
    , m_ShowAssetBrowser(false)
    , m_ShowConsole(false)
    , m_ShowAI(false)
    , m_ShowAPIKeyDialog(false)
    , m_SelectedEntity(0)
    , m_HasSelectedEntity(false)
    , m_Coordinator(nullptr)
    , m_Scene(nullptr)
    , m_PhysicsSystem(nullptr)
    , m_SelectedProvider(0)
    , m_EntityCounter(0)
{
    // Initialize AI components
    m_AIManager = std::make_unique<AIManager>();
    m_AIWindow = std::make_unique<AIWindow>();
    m_AIWindow->SetAIManager(m_AIManager.get());
    
    // Initialize dialog buffers
    memset(m_APIKeyBuffer, 0, sizeof(m_APIKeyBuffer));
    memset(m_EndpointBuffer, 0, sizeof(m_EndpointBuffer));
}

UIManager::~UIManager() {
    Shutdown();
}

bool UIManager::Initialize(GLFWwindow* window) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // Enable keyboard controls and clipboard
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    
    // Explicitly set clipboard functions to ensure copy/paste works
    io.SetClipboardTextFn = [](void* user_data, const char* text) {
        glfwSetClipboardString((GLFWwindow*)user_data, text);
    };
    io.GetClipboardTextFn = [](void* user_data) -> const char* {
        return glfwGetClipboardString((GLFWwindow*)user_data);
    };
    io.ClipboardUserData = window;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    // Verify clipboard functions are set
    if (io.GetClipboardTextFn && io.SetClipboardTextFn) {
        m_ConsoleMessages.push_back("Clipboard functions initialized successfully");
    } else {
        m_ConsoleMessages.push_back("Warning: Clipboard functions not properly initialized");
    }

    // Load AI configuration
    AIConfig::Instance().LoadFromFile();
    
    // Try to auto-initialize AI if API key is available
    if (AIConfig::Instance().HasAPIKey("Gemini")) {
        std::string apiKey = AIConfig::Instance().GetAPIKey("Gemini");
        InitializeAI(apiKey, "Gemini", "");
    }

    // Add some initial console messages
    m_ConsoleMessages.push_back("MistEngine UI initialized successfully");
    m_ConsoleMessages.push_back("Press F1 to toggle ImGui demo window");
    m_ConsoleMessages.push_back("Press F2 to open AI assistant");
    m_ConsoleMessages.push_back("Use Window > Ask AI to open AI assistant");
    m_ConsoleMessages.push_back("Clipboard support: Ctrl+C to copy, Ctrl+V to paste");

    return true;
}

void UIManager::Shutdown() {
    // Save AI configuration
    AIConfig::Instance().SaveToFile();
    
    m_AIWindow.reset();
    m_AIManager.reset();
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::NewFrame() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Main menu bar
    DrawMainMenuBar();

    // Draw windows
    if (m_ShowHierarchy) DrawHierarchy();
    if (m_ShowInspector) DrawInspector();
    if (m_ShowSceneView) DrawSceneView();
    if (m_ShowAssetBrowser) DrawAssetBrowser();
    if (m_ShowConsole) DrawConsole();
    if (m_ShowDemo) ImGui::ShowDemoWindow(&m_ShowDemo);
    
    // Draw AI window
    if (m_ShowAI && m_AIWindow) {
        m_AIWindow->SetVisible(true);
        m_AIWindow->Draw();
        m_ShowAI = m_AIWindow->IsVisible(); // Sync state if window was closed
    }
    
    // Draw API Key dialog
    if (m_ShowAPIKeyDialog) {
        DrawAPIKeyDialog();
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

void UIManager::InitializeAI(const std::string& apiKey, const std::string& provider, const std::string& endpoint) {
    if (m_AIManager) {
        bool success = m_AIManager->InitializeProvider(provider, apiKey, endpoint);
        if (success) {
            m_ConsoleMessages.push_back("AI provider initialized: " + provider);
            
            // Save the configuration immediately
            AIConfig::Instance().SetAPIKey(provider, apiKey);
            if (!endpoint.empty()) {
                AIConfig::Instance().SetEndpoint(provider, endpoint);
            }
            
            // Force save to ensure persistence
            bool saved = AIConfig::Instance().SaveToFile();
            if (saved) {
                m_ConsoleMessages.push_back("Configuration saved successfully");
            } else {
                m_ConsoleMessages.push_back("Warning: Failed to save configuration");
            }
            
            if (m_AIWindow) {
                m_AIWindow->SetAIManager(m_AIManager.get());
            }
        } else {
            m_ConsoleMessages.push_back("Failed to initialize AI provider: " + provider);
        }
    }
}

void UIManager::DrawMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene")) {
                // TODO: Implement new scene
            }
            if (ImGui::MenuItem("Open Scene")) {
                // TODO: Implement open scene
            }
            if (ImGui::MenuItem("Save Scene")) {
                // TODO: Implement save scene
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                // TODO: Implement exit
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {
                // TODO: Implement undo
            }
            if (ImGui::MenuItem("Redo", "CTRL+Y")) {
                // TODO: Implement redo
            }
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
        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("Hierarchy", nullptr, &m_ShowHierarchy);
            ImGui::MenuItem("Inspector", nullptr, &m_ShowInspector);
            ImGui::MenuItem("Scene View", nullptr, &m_ShowSceneView);
            ImGui::MenuItem("Asset Browser", nullptr, &m_ShowAssetBrowser);
            ImGui::MenuItem("Console", nullptr, &m_ShowConsole);
            ImGui::Separator();
            ImGui::MenuItem("Ask AI", "F2", &m_ShowAI);
            ImGui::Separator();
            ImGui::MenuItem("Demo Window", nullptr, &m_ShowDemo);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("AI")) {
            if (ImGui::MenuItem("Open AI Assistant", "F2")) {
                m_ShowAI = true;
            }
            ImGui::Separator();
            
            // Show AI provider status
            std::string statusText = "Configure API Key";
            if (m_AIManager && m_AIManager->HasActiveProvider()) {
                statusText = "Reconfigure API Key (" + m_AIManager->GetActiveProviderName() + " Connected)";
            }
            
            if (ImGui::MenuItem(statusText.c_str())) {
                ShowAPIKeyDialog();
            }
            
            if (ImGui::BeginMenu("Quick Actions")) {
                bool hasAI = m_AIManager && m_AIManager->HasActiveProvider();
                
                if (!hasAI) {
                    ImGui::BeginDisabled();
                }
                
                if (ImGui::MenuItem("Suggest New Feature")) {
                    m_ShowAI = true;
                    if (m_AIWindow) {
                        m_AIWindow->AddMessage(ChatMessage::USER, "I'm working on a game engine and need suggestions for new features. What are some innovative features I could add to enhance the development experience?");
                    }
                }
                if (ImGui::MenuItem("Code Review Help")) {
                    m_ShowAI = true;
                    if (m_AIWindow) {
                        m_AIWindow->AddMessage(ChatMessage::USER, "I need help reviewing my game engine code. What are the best practices for C++ game engine architecture?");
                    }
                }
                if (ImGui::MenuItem("Game Logic Advice")) {
                    m_ShowAI = true;
                    if (m_AIWindow) {
                        m_AIWindow->AddMessage(ChatMessage::USER, "I need advice on implementing efficient game logic systems. What patterns should I consider for my ECS-based game engine?");
                    }
                }
                
                if (!hasAI) {
                    ImGui::EndDisabled();
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Configure API key to enable");
                }
                
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void UIManager::DrawAPIKeyDialog() {
    if (m_ShowAPIKeyDialog) {
        ImGui::OpenPopup("Configure AI API");
    }
    
    // Always draw the popup modal
    if (ImGui::BeginPopupModal("Configure AI API", &m_ShowAPIKeyDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Configure AI Provider Settings");
        ImGui::Separator();
        
        // Provider selection
        const char* providers[] = {"Google Gemini"};
        ImGui::Combo("Provider", &m_SelectedProvider, providers, IM_ARRAYSIZE(providers));
        
        // API Key input
        ImGui::Text("API Key:");
        ImGui::SetNextItemWidth(400);
        
        bool apiKeyChanged = ImGui::InputText("##APIKey", m_APIKeyBuffer, sizeof(m_APIKeyBuffer));
        
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Enter your Gemini API key here");
        }
        
        if (ImGui::IsItemActive()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "(ACTIVE)");
        }
        
        // Paste and Clear buttons (keeping these since they work well)
        ImGui::SameLine();
        if (ImGui::Button("Paste##API")) {
            const char* clipText = ImGui::GetClipboardText();
            if (clipText) {
                strncpy_s(m_APIKeyBuffer, sizeof(m_APIKeyBuffer), clipText, _TRUNCATE);
                apiKeyChanged = true;
                m_ConsoleMessages.push_back("? Pasted API key from clipboard");
            } else {
                m_ConsoleMessages.push_back("? Nothing in clipboard to paste");
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Clear##API")) {
            memset(m_APIKeyBuffer, 0, sizeof(m_APIKeyBuffer));
            apiKeyChanged = true;
            m_ConsoleMessages.push_back("API key field cleared");
        }
        
        ImGui::Separator();
        
        // Test connection button
        if (ImGui::Button("Test Connection")) {
            std::string provider = providers[m_SelectedProvider];
            std::string apiKey = m_APIKeyBuffer;
            
            if (!apiKey.empty()) {
                m_ConsoleMessages.push_back("?? Testing connection to " + provider + "...");
                m_ConsoleMessages.push_back("?? Using minimal request to test connection");
                
                // Temporarily initialize AI for testing
                AIManager testManager;
                if (testManager.InitializeProvider(provider, apiKey, "")) {
                    AIResponse testResponse = testManager.TestConnection();
                    if (testResponse.success) {
                        m_ConsoleMessages.push_back("? Connection test successful!");
                        m_ConsoleMessages.push_back("Response: " + testResponse.content);
                    } else {
                        m_ConsoleMessages.push_back("? Connection test failed:");
                        // Split multi-line error messages
                        std::istringstream iss(testResponse.errorMessage);
                        std::string line;
                        while (std::getline(iss, line)) {
                            if (!line.empty()) {
                                m_ConsoleMessages.push_back(line);
                            }
                        }
                    }
                } else {
                    m_ConsoleMessages.push_back("? Failed to initialize provider for testing");
                }
            } else {
                m_ConsoleMessages.push_back("?? Please enter an API key to test");
            }
        }
        
        // Add diagnostic info button for Gemini
        ImGui::SameLine();
        if (ImGui::Button("Diagnostic Info")) {
            m_ConsoleMessages.push_back("?? GOOGLE GEMINI SETUP GUIDE:");
            m_ConsoleMessages.push_back("");
            m_ConsoleMessages.push_back("1. GET YOUR API KEY:");
            m_ConsoleMessages.push_back("   • Go to https://aistudio.google.com/app/apikey");
            m_ConsoleMessages.push_back("   • Sign in with your Google account");
            m_ConsoleMessages.push_back("   • Click 'Create API Key'");
            m_ConsoleMessages.push_back("   • Copy the generated key");
            m_ConsoleMessages.push_back("");
            m_ConsoleMessages.push_back("2. ENABLE GEMINI API:");
            m_ConsoleMessages.push_back("   • API is free with rate limits");
            m_ConsoleMessages.push_back("   • No billing setup required for basic usage");
            m_ConsoleMessages.push_back("   • Higher limits available with paid plans");
            m_ConsoleMessages.push_back("");
            m_ConsoleMessages.push_back("3. UPDATED MODELS (2024):");
            m_ConsoleMessages.push_back("   • gemini-1.5-flash: Fast & efficient (default)");
            m_ConsoleMessages.push_back("   • gemini-1.5-pro: Most capable model");
            m_ConsoleMessages.push_back("   • gemini-1.0-pro: Stable baseline");
            m_ConsoleMessages.push_back("   • Note: Old 'gemini-pro' is deprecated");
            m_ConsoleMessages.push_back("");
            m_ConsoleMessages.push_back("4. RATE LIMITS:");
            m_ConsoleMessages.push_back("   • Free tier: 15 requests/minute");
            m_ConsoleMessages.push_back("   • No daily token limits on free tier");
            m_ConsoleMessages.push_back("   • Much more generous than OpenAI free tier");
        }
        
        ImGui::SameLine();
        
        // Buttons
        bool saveClicked = ImGui::Button("Save & Connect");
        ImGui::SameLine();
        bool cancelClicked = ImGui::Button("Cancel");
        
        if (saveClicked) {
            std::string provider = providers[m_SelectedProvider];
            std::string apiKey = m_APIKeyBuffer;
            std::string endpoint = m_EndpointBuffer;
            
            if (!apiKey.empty()) {
                InitializeAI(apiKey, provider, endpoint);
                m_ShowAPIKeyDialog = false;
                
                // Clear buffers for security
                memset(m_APIKeyBuffer, 0, sizeof(m_APIKeyBuffer));
                memset(m_EndpointBuffer, 0, sizeof(m_EndpointBuffer));
                
                m_ConsoleMessages.push_back("API key configured for: " + provider);
            } else {
                m_ConsoleMessages.push_back("Please enter an API key");
            }
        }
        
        if (cancelClicked) {
            m_ShowAPIKeyDialog = false;
            // Clear buffers for security
            memset(m_APIKeyBuffer, 0, sizeof(m_APIKeyBuffer));
            memset(m_EndpointBuffer, 0, sizeof(m_EndpointBuffer));
        }
        
        ImGui::Spacing();
        ImGui::TextWrapped("Note: Your API key will be saved locally in ai_config.json. Keep this file secure and do not commit it to version control.");
        
        // Keyboard shortcuts help
        ImGui::Spacing();
        if (ImGui::CollapsingHeader("Keyboard Shortcuts")) {
            ImGui::BulletText("Ctrl+V: Paste");
            ImGui::BulletText("Ctrl+C: Copy");
            ImGui::BulletText("Ctrl+A: Select All");
            ImGui::BulletText("Ctrl+Z: Undo");
            ImGui::BulletText("Tab: Move between fields");
        }
        
        // Help text
        ImGui::Spacing();
        if (ImGui::CollapsingHeader("Help: Getting a Gemini API Key")) {
            ImGui::TextWrapped("Getting Started with Google Gemini:");
            ImGui::BulletText("Go to https://aistudio.google.com/app/apikey");
            ImGui::BulletText("Sign in with your Google account");
            ImGui::BulletText("Click 'Create API Key' button");
            ImGui::BulletText("Copy the generated API key");
            ImGui::BulletText("Paste it in the field above");
            
            ImGui::Spacing();
            ImGui::TextWrapped("Advantages of Gemini:");
            ImGui::BulletText("Free tier with generous limits");
            ImGui::BulletText("No billing setup required initially");
            ImGui::BulletText("15 requests/minute on free tier");
            ImGui::BulletText("High-quality responses comparable to GPT-4");
            ImGui::BulletText("Supports both text and vision models");
        }
        
        ImGui::EndPopup();
    }
}

void UIManager::ShowAPIKeyDialog() { 
    m_ShowAPIKeyDialog = true; 
    // Pre-fill with existing key if available (masked)
    if (AIConfig::Instance().HasAPIKey("Gemini")) {
        // Don't show the actual key for security, just indicate it exists
        std::string maskedKey = "***CONFIGURED***";
        strncpy_s(m_APIKeyBuffer, sizeof(m_APIKeyBuffer), maskedKey.c_str(), _TRUNCATE);
    }
}

void UIManager::DrawHierarchy() {
    ImGui::Begin("Hierarchy", &m_ShowHierarchy);
    
    if (ImGui::Button("Create Entity")) {
        CreateEntity("New Entity");
    }
    
    ImGui::Separator();
    
    // Update entity list (in a real engine this would be more efficient)
    if (m_Coordinator) {
        m_EntityList.clear();
        // Note: In a real ECS system, you'd have a way to iterate through all entities
        // For now, we'll just display the entities we're tracking
        for (int i = 0; i < m_EntityCounter; ++i) {
            Entity entity = i;
            std::string name = "Entity " + std::to_string(entity);
            m_EntityList.push_back(std::make_pair(entity, name));
        }
    }
    
    // Display entity list
    for (size_t i = 0; i < m_EntityList.size(); ++i) {
        Entity entity = m_EntityList[i].first;
        const std::string& name = m_EntityList[i].second;
        
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (m_HasSelectedEntity && m_SelectedEntity == entity) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        
        ImGui::TreeNodeEx(name.c_str(), flags, "%s", name.c_str());
        
        if (ImGui::IsItemClicked()) {
            SelectEntity(entity);
        }
        
        // Context menu for entity
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete Entity")) {
                DeleteEntity(entity);
            }
            ImGui::EndPopup();
        }
    }
    
    ImGui::End();
}

void UIManager::DrawInspector() {
    ImGui::Begin("Inspector", &m_ShowInspector);
    
    if (m_HasSelectedEntity && m_Coordinator) {
        ImGui::Text("Entity: %d", m_SelectedEntity);
        ImGui::Separator();
        
        // Transform Component
        try {
            auto& transform = m_Coordinator->GetComponent<TransformComponent>(m_SelectedEntity);
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                DrawTransformComponent(transform);
            }
        } catch (...) {
            // Component doesn't exist
        }
        
        // Render Component
        try {
            auto& render = m_Coordinator->GetComponent<RenderComponent>(m_SelectedEntity);
            if (ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_DefaultOpen)) {
                DrawRenderComponent(render);
            }
        } catch (...) {
            // Component doesn't exist
        }
        
        // Physics Component
        try {
            auto& physics = m_Coordinator->GetComponent<PhysicsComponent>(m_SelectedEntity);
            if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
                DrawPhysicsComponent(physics);
            }
        } catch (...) {
            // Component doesn't exist
        }
        
        ImGui::Separator();
        
        // Add Component button
        if (ImGui::Button("Add Component")) {
            ImGui::OpenPopup("AddComponentPopup");
        }
        
        if (ImGui::BeginPopup("AddComponentPopup")) {
            if (ImGui::MenuItem("Transform")) {
                try {
                    m_Coordinator->GetComponent<TransformComponent>(m_SelectedEntity);
                } catch (...) {
                    TransformComponent transform;
                    m_Coordinator->AddComponent(m_SelectedEntity, transform);
                }
            }
            if (ImGui::MenuItem("Render")) {
                try {
                    m_Coordinator->GetComponent<RenderComponent>(m_SelectedEntity);
                } catch (...) {
                    RenderComponent render;
                    render.renderable = nullptr;
                    render.visible = true;
                    m_Coordinator->AddComponent(m_SelectedEntity, render);
                }
            }
            if (ImGui::MenuItem("Physics")) {
                try {
                    m_Coordinator->GetComponent<PhysicsComponent>(m_SelectedEntity);
                } catch (...) {
                    PhysicsComponent physics;
                    physics.rigidBody = nullptr;
                    physics.syncTransform = true;
                    m_Coordinator->AddComponent(m_SelectedEntity, physics);
                }
            }
            ImGui::EndPopup();
        }
    } else {
        ImGui::Text("No entity selected");
    }
    
    ImGui::End();
}

void UIManager::DrawSceneView() {
    ImGui::Begin("Scene View", &m_ShowSceneView);
    
    ImGui::Text("Scene view will be implemented here");
    ImGui::Text("This would show the 3D scene viewport");
    
    ImGui::End();
}

void UIManager::DrawAssetBrowser() {
    ImGui::Begin("Asset Browser", &m_ShowAssetBrowser);
    
    ImGui::Text("Asset browser will be implemented here");
    ImGui::Text("This would show textures, models, etc.");
    
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
    if (m_Coordinator) {
        Entity entity = m_Coordinator->CreateEntity();
        m_EntityCounter = std::max(m_EntityCounter, (int)entity + 1);
        
        // Add default transform component
        TransformComponent transform;
        m_Coordinator->AddComponent(entity, transform);
        
        m_ConsoleMessages.push_back("Created entity: " + name);
        
        // Select the new entity
        SelectEntity(entity);
    }
}

void UIManager::DeleteEntity(Entity entity) {
    if (m_Coordinator) {
        m_Coordinator->DestroyEntity(entity);
        
        if (m_HasSelectedEntity && m_SelectedEntity == entity) {
            m_HasSelectedEntity = false;
            m_SelectedEntity = 0;
        }
        
        m_ConsoleMessages.push_back("Deleted entity: " + std::to_string(entity));
    }
}

void UIManager::SelectEntity(Entity entity) {
    m_SelectedEntity = entity;
    m_HasSelectedEntity = true;
}

void UIManager::CreateCube() {
    if (m_Coordinator && m_PhysicsSystem) {
        Entity entity = m_Coordinator->CreateEntity();
        m_EntityCounter = std::max(m_EntityCounter, (int)entity + 1);
        
        // Transform
        TransformComponent transform;
        transform.position = glm::vec3(0.0f, 1.0f, 0.0f);
        transform.scale = glm::vec3(1.0f);
        m_Coordinator->AddComponent(entity, transform);
        
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
        
        // Physics
        btRigidBody* body = m_PhysicsSystem->CreateCube(transform.position, 1.0f);
        PhysicsComponent physics;
        physics.rigidBody = body;
        physics.syncTransform = true;
        m_Coordinator->AddComponent(entity, physics);
        
        m_ConsoleMessages.push_back("Created cube entity");
        SelectEntity(entity);
    }
}

void UIManager::CreateSphere() {
    CreateEntity("Sphere");
    m_ConsoleMessages.push_back("Created sphere entity (mesh generation not implemented)");
}

void UIManager::CreatePlane() {
    if (m_Coordinator && m_PhysicsSystem) {
        Entity entity = m_Coordinator->CreateEntity();
        m_EntityCounter = std::max(m_EntityCounter, (int)entity + 1);
        
        // Transform
        TransformComponent transform;
        transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
        transform.scale = glm::vec3(10.0f, 1.0f, 10.0f);
        m_Coordinator->AddComponent(entity, transform);
        
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
        
        // Physics
        btRigidBody* body = m_PhysicsSystem->CreateGroundPlane(transform.position);
        PhysicsComponent physics;
        physics.rigidBody = body;
        physics.syncTransform = true;
        m_Coordinator->AddComponent(entity, physics);
        
        m_ConsoleMessages.push_back("Created plane entity");
        SelectEntity(entity);
    }
}

void UIManager::DrawTransformComponent(TransformComponent& transform) {
    DrawVec3Control("Position", transform.position);
    DrawVec3Control("Rotation", transform.rotation);
    DrawVec3Control("Scale", transform.scale, 1.0f);
}

void UIManager::DrawRenderComponent(RenderComponent& render) {
    ImGui::Checkbox("Visible", &render.visible);
    
    if (render.renderable) {
        ImGui::Text("Renderable: Valid");
    } else {
        ImGui::Text("Renderable: None");
    }
}

void UIManager::DrawPhysicsComponent(PhysicsComponent& physics) {
    ImGui::Checkbox("Sync Transform", &physics.syncTransform);
    
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