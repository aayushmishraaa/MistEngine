#include "UIManager.h"
#include "GameExporter.h"  // NEW: Add GameExporter include
#include "FPSGameManager.h"  // NEW: Add FPSGameManager include
#include "ECS/Systems/EnemyAISystem.h"  // NEW: Add EnemyAISystem include
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
#include "Version.h"  // Add Version.h include for version information
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
    , m_ShowSceneView(false)
    , m_ShowAssetBrowser(false)
    , m_ShowConsole(true)    // Open console by default to see debug messages
    , m_ShowAI(false)
    , m_ShowAPIKeyDialog(false)
    , m_ShowExportDialog(false)  // NEW: Initialize export dialog flag
    , m_SelectedEntity(0)
    , m_HasSelectedEntity(false)
    , m_Coordinator(nullptr)
    , m_Scene(nullptr)
    , m_PhysicsSystem(nullptr)
    , m_FPSGameManager(nullptr)  // NEW: Initialize FPS Game Manager reference
    , m_SelectedProvider(0)
    , m_EntityCounter(0)
{
    // Initialize AI components
    m_AIManager = std::make_unique<AIManager>();
    m_AIWindow = std::make_unique<AIWindow>();
    m_AIWindow->SetAIManager(m_AIManager.get());
    
    // NEW: Initialize GameExporter
    m_GameExporter = std::make_unique<GameExporter>();
    
    // Initialize dialog buffers
    memset(m_APIKeyBuffer, 0, sizeof(m_APIKeyBuffer));
    memset(m_EndpointBuffer, 0, sizeof(m_EndpointBuffer));
    memset(m_GameNameBuffer, 0, sizeof(m_GameNameBuffer));
    memset(m_OutputPathBuffer, 0, sizeof(m_OutputPathBuffer));
    
    // Initialize export settings with defaults
    strncpy_s(m_GameNameBuffer, sizeof(m_GameNameBuffer), "MistFPS", _TRUNCATE);
    strncpy_s(m_OutputPathBuffer, sizeof(m_OutputPathBuffer), "exports", _TRUNCATE);
    m_NumLevels = 5;
    m_EnemiesPerLevel = 10;
    m_IncludeAssets = true;
    m_CompressAssets = true;
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

    // Check if we're in FPS game mode
    bool isFPSGameActive = (m_FPSGameManager && m_FPSGameManager->IsGameActive());
    bool isPlayerDead = false; // TODO: Check if player is dead
    
    // DEBUG: Log the FPS game state transitions only
    static bool lastGameState = false;
    if (isFPSGameActive != lastGameState) {
        std::string stateText = isFPSGameActive ? "?? UI SWITCHED TO FPS MODE!" : "??? UI SWITCHED TO EDITOR MODE";
        m_ConsoleMessages.push_back(stateText);
        lastGameState = isFPSGameActive;
    }
    
    if (isFPSGameActive) {
        // In FPS game mode - show FPS HUD and possibly game over screen
        DrawFPSGameHUD();
        DrawCrosshair();
        
        if (isPlayerDead) {
            DrawGameOverScreen();
        }
    } else {
        // In editor mode - show all editor UI
        DrawMainMenuBar();
        DrawFPSGameLauncher();

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
            m_ShowAI = m_AIWindow->IsVisible();
        }
        
        // Draw API Key dialog
        if (m_ShowAPIKeyDialog) {
            DrawAPIKeyDialog();
        }
        
        // Draw Export Game dialog
        if (m_ShowExportDialog) {
            DrawExportDialog();
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
            if (ImGui::MenuItem("Export FPS Game...", "Ctrl+E")) {
                // TODO: Show export dialog
                m_ShowExportDialog = true;
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
    
    // Update entity list - show all created entities
    if (m_Coordinator) {
        m_EntityList.clear();
        
        // Create list of entities that actually exist and have components
        std::set<Entity> validEntities;
        
        // Check all entities up to our counter
        for (int i = 0; i <= m_EntityCounter && i < MAX_ENTITIES; ++i) {
            Entity entity = static_cast<Entity>(i);
            
            // Check if entity has at least one component (meaning it exists)
            bool hasComponents = false;
            try {
                m_Coordinator->GetComponent<TransformComponent>(entity);
                hasComponents = true;
            } catch (...) {
                // Try other components
                try {
                    m_Coordinator->GetComponent<RenderComponent>(entity);
                    hasComponents = true;
                } catch (...) {
                    try {
                        m_Coordinator->GetComponent<PhysicsComponent>(entity);
                        hasComponents = true;
                    } catch (...) {
                        // Entity doesn't exist or has no components
                    }
                }
            }
            
            if (hasComponents) {
                validEntities.insert(entity);
            }
        }
        
        // Add valid entities to display list
        for (Entity entity : validEntities) {
            std::string name = "Entity " + std::to_string(entity);
            
            // Try to create more descriptive names based on components
            try {
                auto& render = m_Coordinator->GetComponent<RenderComponent>(entity);
                auto& physics = m_Coordinator->GetComponent<PhysicsComponent>(entity);
                if (render.renderable && physics.rigidBody) {
                    if (physics.rigidBody->getMass() == 0.0f) {
                        name = "Ground " + std::to_string(entity);
                    } else {
                        name = "Cube " + std::to_string(entity);
                    }
                }
            } catch (...) {
                // Use default name
            }
            
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
    
    if (m_EntityList.empty()) {
        ImGui::Text("No entities in scene");
        ImGui::Text("Use GameObject menu to create objects");
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
    
    // If transform was modified, disable physics sync temporarily
    if (m_HasSelectedEntity && m_Coordinator) {
        bool transformChanged = (transform.position != originalPos || 
                                transform.rotation != originalRot || 
                                transform.scale != originalScale);
        
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
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f)); // Red
    ImGui::SetWindowFontScale(3.0f);
    ImGui::Text("GAME OVER");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    
    // Restart button
    ImGui::SetCursorPos(ImVec2(screenWidth * 0.5f - 75, screenHeight * 0.6f));
    if (ImGui::Button("RESTART", ImVec2(150, 50))) {
        m_FPSGameManager->RestartGame();
    }
    
    ImGui::End();
}