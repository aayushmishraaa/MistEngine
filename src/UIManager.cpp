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
#include "Model.h"  // Add Model include for CreateModelEntity
#include "AI/AIManager.h"
#include "AI/AIWindow.h"
#include "AI/AIConfig.h"
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
    , m_SelectedEntity(0)
    , m_HasSelectedEntity(false)
    , m_Coordinator(nullptr)
    , m_Scene(nullptr)
    , m_PhysicsSystem(nullptr)
    , m_SelectedProvider(0)
    , m_EntityCounter(0)
    , m_MultiSelectMode(false)
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
    
    // Toolbar
    if (ImGui::Button("Create Entity")) {
        CreateEntity("New Entity");
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        m_ConsoleMessages.push_back("Hierarchy refreshed");
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Delete Selected")) {
        DeleteSelectedEntities();
    }
    
    ImGui::SameLine();
    ImGui::Checkbox("Multi-Select", &m_MultiSelectMode);
    
    ImGui::Separator();
    
    // Search/Filter
    static char searchBuffer[256] = "";
    ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer));
    std::string searchTerm = std::string(searchBuffer);
    std::transform(searchTerm.begin(), searchTerm.end(), searchTerm.begin(), ::tolower);
    
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
            std::string type = "";
            glm::vec3 position(0.0f);
            bool isVisible = true;
            bool hasPhysics = false;
            
            // Try to create more descriptive names based on components
            try {
                auto& transform = m_Coordinator->GetComponent<TransformComponent>(entity);
                position = transform.position;
                
                try {
                    auto& render = m_Coordinator->GetComponent<RenderComponent>(entity);
                    isVisible = render.visible;
                    if (render.renderable) {
                        // Try to determine object type
                        type = " [Renderable]";
                    }
                } catch (...) {}
                
                try {
                    auto& physics = m_Coordinator->GetComponent<PhysicsComponent>(entity);
                    hasPhysics = true;
                    if (physics.rigidBody) {
                        if (physics.rigidBody->getMass() == 0.0f) {
                            type = " [Static]";
                        } else {
                            type = " [Dynamic]";
                        }
                    }
                } catch (...) {}
                
            } catch (...) {
                // Use default name
            }
            
            // Apply search filter
            std::string fullName = name + type;
            std::string lowerName = fullName;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            
            if (searchTerm.empty() || lowerName.find(searchTerm) != std::string::npos) {
                m_EntityList.push_back(std::make_pair(entity, name + type));
            }
        }
    }
    
    // Display entity list with enhanced information
    for (size_t i = 0; i < m_EntityList.size(); ++i) {
        Entity entity = m_EntityList[i].first;
        const std::string& name = m_EntityList[i].second;
        
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (m_HasSelectedEntity && m_SelectedEntity == entity) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        
        // Check if entity is in multi-selection
        bool isMultiSelected = std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity) != m_SelectedEntities.end();
        if (isMultiSelected) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        
        // Color code entities based on state
        bool hasPhysics = false;
        bool isVisible = true;
        glm::vec3 position(0.0f);
        
        try {
            auto& physics = m_Coordinator->GetComponent<PhysicsComponent>(entity);
            hasPhysics = true;
        } catch (...) {}
        
        try {
            auto& render = m_Coordinator->GetComponent<RenderComponent>(entity);
            isVisible = render.visible;
        } catch (...) {}
        
        try {
            auto& transform = m_Coordinator->GetComponent<TransformComponent>(entity);
            position = transform.position;
        } catch (...) {}
        
        // Apply color based on entity state
        if (!isVisible) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // Grayed out for hidden
        } else if (hasPhysics) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f)); // Green for physics objects
        }
        
        ImGui::TreeNodeEx(name.c_str(), flags, "%s", name.c_str());
        
        if (!isVisible || hasPhysics) {
            ImGui::PopStyleColor();
        }
        
        // Show position as tooltip
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Position: (%.1f, %.1f, %.1f)\nClick to select\nRight-click for options", 
                             position.x, position.y, position.z);
        }
        
        if (ImGui::IsItemClicked()) {
            if (m_MultiSelectMode) {
                // Multi-selection mode
                ImGuiIO& io = ImGui::GetIO();
                if (io.KeyCtrl) {
                    // Toggle selection
                    auto it = std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity);
                    if (it != m_SelectedEntities.end()) {
                        m_SelectedEntities.erase(it);
                        if (m_SelectedEntities.empty()) {
                            m_HasSelectedEntity = false;
                            m_SelectedEntity = 0;
                        }
                    } else {
                        m_SelectedEntities.push_back(entity);
                        SelectEntity(entity);  // Also set as primary selection
                    }
                } else {
                    // Replace selection
                    m_SelectedEntities.clear();
                    m_SelectedEntities.push_back(entity);
                    SelectEntity(entity);
                }
            } else {
                // Single selection mode
                m_SelectedEntities.clear();
                SelectEntity(entity);
            }
        }
        
        // Enhanced context menu for entity
        if (ImGui::BeginPopupContextItem()) {
            ImGui::Text("Entity %d Options", entity);
            ImGui::Separator();
            
            if (ImGui::MenuItem("Focus on Entity")) {
                SelectEntity(entity);
                // Could add camera focusing here
                m_ConsoleMessages.push_back("Focused on entity " + std::to_string(entity));
            }
            
            if (ImGui::MenuItem("Duplicate Entity")) {
                DuplicateEntity(entity);
            }
            
            if (ImGui::MenuItem("Reset Position")) {
                ResetEntityPosition(entity);
            }
            
            ImGui::Separator();
            
            // Component-specific actions
            try {
                auto& render = m_Coordinator->GetComponent<RenderComponent>(entity);
                if (render.visible) {
                    if (ImGui::MenuItem("Hide")) {
                        render.visible = false;
                        m_ConsoleMessages.push_back("Hidden entity " + std::to_string(entity));
                    }
                } else {
                    if (ImGui::MenuItem("Show")) {
                        render.visible = true;
                        m_ConsoleMessages.push_back("Shown entity " + std::to_string(entity));
                    }
                }
            } catch (...) {}
            
            try {
                auto& physics = m_Coordinator->GetComponent<PhysicsComponent>(entity);
                if (physics.rigidBody && m_PhysicsSystem) {
                    if (ImGui::MenuItem("Apply Force Up")) {
                        m_PhysicsSystem->ApplyForce(physics.rigidBody, glm::vec3(0.0f, 500.0f, 0.0f));
                        m_ConsoleMessages.push_back("Applied upward force to entity " + std::to_string(entity));
                    }
                }
            } catch (...) {}
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Delete Entity")) {
                DeleteEntity(entity);
            }
            
            ImGui::EndPopup();
        }
    }
    
    if (m_EntityList.empty()) {
        if (searchTerm.empty()) {
            ImGui::Text("No entities in scene");
            ImGui::Text("Use GameObject menu to create objects");
        } else {
            ImGui::Text("No entities match search: %s", searchTerm.c_str());
        }
    }
    
    ImGui::End();
}

void UIManager::DrawInspector() {
    ImGui::Begin("Inspector", &m_ShowInspector);
    
    if (m_HasSelectedEntity && m_Coordinator) {
        ImGui::Text("Entity: %d", m_SelectedEntity);
        ImGui::Separator();
        
        // Entity actions toolbar
        if (ImGui::Button("Duplicate")) {
            DuplicateEntity(m_SelectedEntity);
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Delete")) {
            DeleteEntity(m_SelectedEntity);
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Reset Position")) {
            ResetEntityPosition(m_SelectedEntity);
        }
        
        ImGui::Separator();
        
        // Transform Component
        try {
            auto& transform = m_Coordinator->GetComponent<TransformComponent>(m_SelectedEntity);
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                DrawTransformComponent(transform);
                
                // Quick presets
                if (ImGui::Button("Ground Level")) {
                    transform.position.y = 0.0f;
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset Scale")) {
                    transform.scale = glm::vec3(1.0f);
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset Rotation")) {
                    transform.rotation = glm::vec3(0.0f);
                }
            }
        } catch (...) {
            // Component doesn't exist
        }
        
        // Render Component
        try {
            auto& render = m_Coordinator->GetComponent<RenderComponent>(m_SelectedEntity);
            if (ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_DefaultOpen)) {
                DrawRenderComponent(render);
                
                // Render options
                if (ImGui::Button("Hide")) {
                    render.visible = false;
                }
                ImGui::SameLine();
                if (ImGui::Button("Show")) {
                    render.visible = true;
                }
            }
        } catch (...) {
            // Component doesn't exist
        }
        
        // Physics Component
        try {
            auto& physics = m_Coordinator->GetComponent<PhysicsComponent>(m_SelectedEntity);
            if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
                DrawPhysicsComponent(physics);
                
                // Physics actions
                if (physics.rigidBody) {
                    if (ImGui::Button("Apply Upward Force")) {
                        if (m_PhysicsSystem) {
                            m_PhysicsSystem->ApplyForce(physics.rigidBody, glm::vec3(0.0f, 500.0f, 0.0f));
                        }
                    }
                    
                    ImGui::SameLine();
                    if (ImGui::Button("Reset Velocity")) {
                        physics.rigidBody->setLinearVelocity(btVector3(0, 0, 0));
                        physics.rigidBody->setAngularVelocity(btVector3(0, 0, 0));
                    }
                    
                    // Mass control
                    float mass = physics.rigidBody->getMass();
                    if (ImGui::SliderFloat("Mass", &mass, 0.0f, 10.0f)) {
                        // Update mass (this requires recreating the rigid body in Bullet)
                        btVector3 inertia(0, 0, 0);
                        if (mass > 0.0f) {
                            physics.rigidBody->getCollisionShape()->calculateLocalInertia(mass, inertia);
                        }
                        physics.rigidBody->setMassProps(mass, inertia);
                    }
                }
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
                    m_ConsoleMessages.push_back("Added Transform component to entity " + std::to_string(m_SelectedEntity));
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
                    m_ConsoleMessages.push_back("Added Render component to entity " + std::to_string(m_SelectedEntity));
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
                    m_ConsoleMessages.push_back("Added Physics component to entity " + std::to_string(m_SelectedEntity));
                }
            }
            ImGui::EndPopup();
        }
    } else {
        ImGui::Text("No entity selected");
        ImGui::Text("Select an entity from the Hierarchy to inspect it.");
        
        // Quick actions when nothing is selected
        if (ImGui::Button("Create New Entity")) {
            CreateEntity("New Entity");
        }
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
    
    ImGui::Text("Asset Browser");
    ImGui::Separator();
    
    // Toolbar with buttons
    if (ImGui::Button("Refresh")) {
        m_ConsoleMessages.push_back("Asset browser refreshed");
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Import Model")) {
        ImGui::OpenPopup("Import Model Dialog");
    }
    
    // Import Model Dialog
    if (ImGui::BeginPopupModal("Import Model Dialog", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Import 3D Model");
        ImGui::Separator();
        
        static char modelPath[512] = "models/";
        ImGui::InputText("Model Path", modelPath, sizeof(modelPath));
        
        if (ImGui::Button("Browse")) {
            // TODO: Implement file dialog
            m_ConsoleMessages.push_back("File dialog not yet implemented");
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Import")) {
            std::string path = std::string(modelPath);
            if (!path.empty()) {
                CreateModelEntity(path);
                ImGui::CloseCurrentPopup();
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    ImGui::Separator();
    
    // Asset categories
    if (ImGui::CollapsingHeader("Models", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Common models with better error handling
        if (ImGui::Selectable("Backpack")) {
            // Use proper path format - already exists in models/backpack/
            CreateModelEntity("models\\backpack\\backpack.obj");
        }
        
        // Add tooltip with file info
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Click to create backpack model in scene\nPath: models\\backpack\\backpack.obj\nNote: Model files verified to exist");
        }
        
        // Test with different path formats
        if (ImGui::Selectable("Backpack (Forward Slash)")) {
            CreateModelEntity("models/backpack/backpack.obj");
        }
        
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Test with forward slash path format");
        }
        
        // Debug options
        if (ImGui::Button("Toggle Wireframe")) {
            static bool wireframe = false;
            wireframe = !wireframe;
            if (wireframe) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                m_ConsoleMessages.push_back("Wireframe mode enabled");
            } else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                m_ConsoleMessages.push_back("Wireframe mode disabled");
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Test Cube (ECS)")) {
            CreateCube();  // This should test if ECS rendering works
        }
        
        // Add simple triangle test
        ImGui::SameLine();
        if (ImGui::Button("Test Triangle")) {
            // Create a simple test triangle to verify rendering works
            Entity testEntity = m_Coordinator->CreateEntity();
            m_EntityCounter = std::max(m_EntityCounter, (int)testEntity + 1);
            
            // Create simple triangle geometry
            std::vector<Vertex> vertices = {
                {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // Bottom left
                {{ 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},  // Bottom right
                {{ 0.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}}   // Top
            };
            
            std::vector<unsigned int> indices = {0, 1, 2};  // Single triangle
            std::vector<Texture> textures;  // No textures
            
            // Transform
            TransformComponent transform;
            transform.position = glm::vec3(0.0f, 1.0f, 0.0f);
            transform.scale = glm::vec3(2.0f);  // Large triangle
            m_Coordinator->AddComponent(testEntity, transform);
            
            // Render component
            RenderComponent render;
            render.renderable = new Mesh(vertices, indices, textures);
            render.visible = true;
            m_Coordinator->AddComponent(testEntity, render);
            
            m_ConsoleMessages.push_back("Created test triangle entity " + std::to_string(testEntity));
            SelectEntity(testEntity);
        }
        
        // Additional model slots
        static char customModelPath[512] = "models\\";
        ImGui::InputText("Custom Model Path", customModelPath, sizeof(customModelPath));
        ImGui::SameLine();
        if (ImGui::Button("Load Custom")) {
            std::string path = std::string(customModelPath);
            if (!path.empty() && path != "models\\" && path != "models/") {
                CreateModelEntity(path);
            } else {
                m_ConsoleMessages.push_back("Please enter a valid model path");
            }
        }
        
        ImGui::Text("Supported formats: .obj, .fbx, .dae, .gltf, .ply");
        ImGui::Text("Available: backpack.obj (verified present)");
    }
    
    if (ImGui::CollapsingHeader("Textures")) {
        if (ImGui::Selectable("Container.jpg")) {
            m_ConsoleMessages.push_back("Selected texture: container.jpg");
        }
        
        ImGui::Text("  (Add more textures to textures/ folder)");
    }
    
    if (ImGui::CollapsingHeader("Primitives")) {
        if (ImGui::Button("Create Cube")) {
            CreateCube();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Create Sphere")) {
            CreateSphere();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Create Plane")) {
            CreatePlane();
        }
    }
    
    // Asset info panel
    ImGui::Separator();
    ImGui::Text("Asset Information");
    if (m_HasSelectedEntity) {
        ImGui::Text("Selected Entity: %d", m_SelectedEntity);
        
        // Show asset details for selected entity
        if (m_Coordinator) {
            try {
                auto& render = m_Coordinator->GetComponent<RenderComponent>(m_SelectedEntity);
                if (render.renderable) {
                    ImGui::Text("Has renderable component");
                } else {
                    ImGui::Text("No renderable component");
                }
            } catch (...) {
                ImGui::Text("No render component");
            }
        }
    } else {
        ImGui::Text("No asset selected");
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
    if (!m_Coordinator) return;
    
    try {
        m_ConsoleMessages.push_back("Starting deletion of entity " + std::to_string(entity));
        
        // Step 1: Remove from multi-selection if present
        auto it = std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity);
        if (it != m_SelectedEntities.end()) {
            m_SelectedEntities.erase(it);
        }
        
        // Step 2: Clean up physics resources FIRST (most likely to crash)
        try {
            auto& physics = m_Coordinator->GetComponent<PhysicsComponent>(entity);
            if (physics.rigidBody) {
                m_ConsoleMessages.push_back("Cleaning up physics body for entity " + std::to_string(entity));
                
                // Remove from physics world FIRST
                if (m_PhysicsSystem) {
                    btDiscreteDynamicsWorld* world = m_PhysicsSystem->GetDynamicsWorld();
                    if (world) {
                        world->removeRigidBody(physics.rigidBody);
                        m_ConsoleMessages.push_back("Removed rigid body from dynamics world");
                    }
                }
                
                // Clean up motion state
                if (physics.rigidBody->getMotionState()) {
                    delete physics.rigidBody->getMotionState();
                    m_ConsoleMessages.push_back("Deleted motion state");
                }
                
                // Clean up collision shape
                if (physics.rigidBody->getCollisionShape()) {
                    delete physics.rigidBody->getCollisionShape();
                    m_ConsoleMessages.push_back("Deleted collision shape");
                }
                
                // Finally delete the rigid body
                delete physics.rigidBody;
                physics.rigidBody = nullptr;
                m_ConsoleMessages.push_back("Deleted rigid body");
            }
        } catch (const std::exception& e) {
            m_ConsoleMessages.push_back("Exception during physics cleanup: " + std::string(e.what()));
        } catch (...) {
            m_ConsoleMessages.push_back("Unknown exception during physics cleanup");
        }
        
        // Step 3: Clean up render resources (less likely to crash)
        try {
            auto& render = m_Coordinator->GetComponent<RenderComponent>(entity);
            if (render.renderable) {
                // Don't delete shared renderables (like models/meshes that might be used elsewhere)
                // Just null the pointer
                render.renderable = nullptr;
                m_ConsoleMessages.push_back("Cleared renderable pointer");
            }
        } catch (const std::exception& e) {
            m_ConsoleMessages.push_back("Exception during render cleanup: " + std::string(e.what()));
        } catch (...) {
            m_ConsoleMessages.push_back("No render component found");
        }
        
        // Step 4: Update UI selection state BEFORE destroying entity
        if (m_HasSelectedEntity && m_SelectedEntity == entity) {
            m_HasSelectedEntity = false;
            m_SelectedEntity = 0;
            m_ConsoleMessages.push_back("Cleared entity selection");
        }
        
        // Step 5: Finally destroy the entity in ECS
        // This should be the last step as it invalidates the entity ID
        m_Coordinator->DestroyEntity(entity);
        m_ConsoleMessages.push_back("Entity " + std::to_string(entity) + " destroyed successfully");
        
    } catch (const std::exception& e) {
        m_ConsoleMessages.push_back("ERROR: Exception during entity deletion: " + std::string(e.what()));
        
        // Try to at least clear selection to prevent further issues
        if (m_HasSelectedEntity && m_SelectedEntity == entity) {
            m_HasSelectedEntity = false;
            m_SelectedEntity = 0;
        }
        
        // Remove from multi-selection as well
        auto it = std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity);
        if (it != m_SelectedEntities.end()) {
            m_SelectedEntities.erase(it);
        }
        
    } catch (...) {
        m_ConsoleMessages.push_back("ERROR: Unknown exception during entity deletion");
        
        // Emergency cleanup of selection state
        if (m_HasSelectedEntity && m_SelectedEntity == entity) {
            m_HasSelectedEntity = false;
            m_SelectedEntity = 0;
        }
        
        auto it = std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity);
        if (it != m_SelectedEntities.end()) {
            m_SelectedEntities.erase(it);
        }
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
        transform.position = glm::vec3(0.0f, 2.0f, 0.0f);  // Spawn higher up
        transform.scale = glm::vec3(1.0f);
        m_Coordinator->AddComponent(entity, transform);
        m_ConsoleMessages.push_back("Added transform component");
        
        // Create sphere mesh
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        generateSphereMesh(vertices, indices, 0.5f, 32, 16);  // radius, sectors, stacks
        
        // Load default texture for sphere
        std::vector<Texture> textures;
        Texture sphereTexture;
        if (sphereTexture.LoadFromFile("textures/container.jpg")) {
            textures.push_back(sphereTexture);
        }
        
        Mesh* mesh = new Mesh(vertices, indices, textures);
        
        // Render
        RenderComponent render;
        render.renderable = mesh;
        render.visible = true;
        m_Coordinator->AddComponent(entity, render);
        m_ConsoleMessages.push_back("Added render component");
        
        // Physics - create sphere physics body
        btCollisionShape* sphereShape = new btSphereShape(0.5f);
        btVector3 sphereInertia(0, 0, 0);
        float mass = 1.0f;
        sphereShape->calculateLocalInertia(mass, sphereInertia);
        
        btTransform sphereTransform;
        sphereTransform.setIdentity();
        sphereTransform.setOrigin(btVector3(transform.position.x, transform.position.y, transform.position.z));
        
        btDefaultMotionState* sphereMotionState = new btDefaultMotionState(sphereTransform);
        btRigidBody::btRigidBodyConstructionInfo sphereRbInfo(mass, sphereMotionState, sphereShape, sphereInertia);
        btRigidBody* sphereBody = new btRigidBody(sphereRbInfo);
        
        // Add to physics world
        if (m_PhysicsSystem) {
            m_PhysicsSystem->AddRigidBody(sphereBody);
        }
        
        PhysicsComponent physics;
        physics.rigidBody = sphereBody;
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
        
        // Transform - create plane at origin, not below
        TransformComponent transform;
        transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
        transform.scale = glm::vec3(5.0f, 1.0f, 5.0f);  // Scale the plane
        m_Coordinator->AddComponent(entity, transform);
        m_ConsoleMessages.push_back("Added transform component");
        
        // Create smaller plane mesh (2x2 units)
        std::vector<Vertex> vertices = {
            // Single quad for the plane (2x2 units, centered at origin)
            {{-1.0f, 0.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
            {{ 1.0f, 0.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{ 1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
            {{-1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}
        };
        
        std::vector<unsigned int> indices = {
            0, 1, 2,  // First triangle
            2, 3, 0   // Second triangle
        };
        
        // Load texture for plane
        std::vector<Texture> textures;
        Texture planeTexture;
        if (planeTexture.LoadFromFile("textures/container.jpg")) {
            textures.push_back(planeTexture);
        }
        
        Mesh* mesh = new Mesh(vertices, indices, textures);
        
        // Render
        RenderComponent render;
        render.renderable = mesh;
        render.visible = true;
        m_Coordinator->AddComponent(entity, render);
        m_ConsoleMessages.push_back("Added render component");
        
        // Physics - create a thin box instead of static plane for better interaction
        btCollisionShape* planeShape = new btBoxShape(btVector3(5.0f, 0.1f, 5.0f));
        btVector3 planeInertia(0, 0, 0);
        float mass = 0.0f;  // Static body
        
        btTransform planeTransform;
        planeTransform.setIdentity();
        planeTransform.setOrigin(btVector3(transform.position.x, transform.position.y, transform.position.z));
        
        btDefaultMotionState* planeMotionState = new btDefaultMotionState(planeTransform);
        btRigidBody::btRigidBodyConstructionInfo planeRbInfo(mass, planeMotionState, planeShape, planeInertia);
        btRigidBody* planeBody = new btRigidBody(planeRbInfo);
        
        // Add to physics world
        if (m_PhysicsSystem) {
            m_PhysicsSystem->AddRigidBody(planeBody);
        }
        
        PhysicsComponent physics;
        physics.rigidBody = planeBody;
        physics.syncTransform = true;
        m_Coordinator->AddComponent(entity, physics);
        m_ConsoleMessages.push_back("Added physics component");
        
        m_ConsoleMessages.push_back("Plane entity created successfully with " + std::to_string(vertices.size()) + " vertices");
        SelectEntity(entity);
    } else {
        m_ConsoleMessages.push_back("ERROR: Cannot create plane - missing coordinator or physics system");
    }
}

void UIManager::CreateModelEntity(const std::string& modelPath) {
    if (!m_Coordinator || !m_PhysicsSystem) {
        m_ConsoleMessages.push_back("ERROR: Cannot create model entity - missing coordinator or physics system");
        return;
    }
    
    // Check if the model file exists
    std::string fullPath = modelPath;
    
    // Convert path separators for Windows
    std::replace(fullPath.begin(), fullPath.end(), '/', '\\');
    
    m_ConsoleMessages.push_back("Attempting to create model entity from: " + fullPath);
    
    Entity entity = m_Coordinator->CreateEntity();
    m_EntityCounter = std::max(m_EntityCounter, (int)entity + 1);
    
    try {
        // Transform component
        TransformComponent transform;
        transform.position = glm::vec3(0.0f, 1.0f, 0.0f);  // Spawn above ground
        transform.scale = glm::vec3(1.0f);
        m_Coordinator->AddComponent(entity, transform);
        m_ConsoleMessages.push_back("Added transform component to entity " + std::to_string(entity) + " with scale 1.0 (increased from 0.1)");
        
        // Create model with comprehensive error handling
        Model* model = nullptr;
        try {
            model = new Model(fullPath);
            if (!model) {
                throw std::runtime_error("Model constructor returned null");
            }
            m_ConsoleMessages.push_back("Model loaded successfully from: " + fullPath);
        } catch (const std::runtime_error& e) {
            m_ConsoleMessages.push_back("ERROR: Runtime error loading model: " + std::string(e.what()));
            m_Coordinator->DestroyEntity(entity);
            return;
        } catch (const std::exception& e) {
            m_ConsoleMessages.push_back("ERROR: Exception loading model: " + std::string(e.what()));
            m_Coordinator->DestroyEntity(entity);
            return;
        } catch (...) {
            m_ConsoleMessages.push_back("ERROR: Unknown error loading model");
            m_Coordinator->DestroyEntity(entity);
            return;
        }
        
        // Render component - Model implements Renderable interface
        RenderComponent render;
        render.renderable = model;  // Model inherits from Renderable
        render.visible = true;
        m_Coordinator->AddComponent(entity, render);
        
        // Physics component - create a bounding box for the model
        try {
            btCollisionShape* modelShape = new btBoxShape(btVector3(1.0f, 1.0f, 1.0f));  // Reasonable bounding box
            btVector3 modelInertia(0, 0, 0);
            float mass = 0.0f;  // Static by default
            
            if (mass > 0.0f) {
                modelShape->calculateLocalInertia(mass, modelInertia);
            }
            
            btTransform modelTransform;
            modelTransform.setIdentity();
            modelTransform.setOrigin(btVector3(transform.position.x, transform.position.y, transform.position.z));
            
            btDefaultMotionState* modelMotionState = new btDefaultMotionState(modelTransform);
            if (!modelMotionState) {
                throw std::runtime_error("Failed to create motion state");
            }
            
            btRigidBody::btRigidBodyConstructionInfo modelRbInfo(mass, modelMotionState, modelShape, modelInertia);
            btRigidBody* modelBody = new btRigidBody(modelRbInfo);
            if (!modelBody) {
                delete modelMotionState;
                delete modelShape;
                throw std::runtime_error("Failed to create rigid body");
            }
            
            // Add to physics world
            m_PhysicsSystem->AddRigidBody(modelBody);
            
            PhysicsComponent physics;
            physics.rigidBody = modelBody;
            physics.syncTransform = false;  // Models usually don't need physics sync
            m_Coordinator->AddComponent(entity, physics);
            
        } catch (const std::exception& e) {
            m_ConsoleMessages.push_back("WARNING: Failed to create physics component: " + std::string(e.what()));
            // Continue without physics rather than failing completely
        }
        
        m_ConsoleMessages.push_back("Model entity " + std::to_string(entity) + " created successfully: " + fullPath);
        
        // Debug: Check if entity is in the render system
        try {
            auto& testTransform = m_Coordinator->GetComponent<TransformComponent>(entity);
            auto& testRender = m_Coordinator->GetComponent<RenderComponent>(entity);
            
            // Test if the Model class is properly inheriting from Renderable
            Model* modelPtr = dynamic_cast<Model*>(testRender.renderable);
            if (!modelPtr) {
                m_ConsoleMessages.push_back("WARNING: Model cast failed - check inheritance");
            }
            
        } catch (const std::exception& e) {
            m_ConsoleMessages.push_back("ERROR: Failed to verify entity components: " + std::string(e.what()));
        }
        
        SelectEntity(entity);
        
    } catch (const std::exception& e) {
        m_ConsoleMessages.push_back("ERROR: Failed to create model entity: " + std::string(e.what()));
        
        // Clean up the entity if something went wrong
        try {
            m_Coordinator->DestroyEntity(entity);
            m_ConsoleMessages.push_back("Cleaned up failed entity " + std::to_string(entity));
        } catch (...) {
            m_ConsoleMessages.push_back("WARNING: Failed to clean up entity after error");
        }
    } catch (...) {
        m_ConsoleMessages.push_back("ERROR: Unknown error while creating model entity");
        
        try {
            m_Coordinator->DestroyEntity(entity);
        } catch (...) {}
    }
}

void UIManager::DuplicateEntity(Entity originalEntity) {
    if (!m_Coordinator) return;
    
    try {
        Entity newEntity = m_Coordinator->CreateEntity();
        m_EntityCounter = std::max(m_EntityCounter, (int)newEntity + 1);
        
        m_ConsoleMessages.push_back("Duplicating entity " + std::to_string(originalEntity) + " as " + std::to_string(newEntity));
        
        // Copy Transform component if it exists
        try {
            auto& originalTransform = m_Coordinator->GetComponent<TransformComponent>(originalEntity);
            TransformComponent newTransform = originalTransform;
            newTransform.position.x += 2.0f;  // Offset so it doesn't overlap
            m_Coordinator->AddComponent(newEntity, newTransform);
            m_ConsoleMessages.push_back("Copied transform component");
        } catch (...) {}
        
        // Copy Render component if it exists - CREATE INDEPENDENT COPY
        try {
            auto& originalRender = m_Coordinator->GetComponent<RenderComponent>(originalEntity);
            RenderComponent newRender;
            newRender.visible = originalRender.visible;
            
            if (originalRender.renderable) {
                // Try to determine the type of renderable and create a copy
                
                // Check if it's a Model (try dynamic cast)
                Model* originalModel = dynamic_cast<Model*>(originalRender.renderable);
                if (originalModel) {
                    // For models, we need to create a new instance
                    // This is tricky because we don't know the original path
                    // For now, share the renderable but log the issue
                    newRender.renderable = originalRender.renderable;
                    m_ConsoleMessages.push_back("WARNING: Model renderable shared - true duplication not yet implemented for models");
                } else {
                    // Check if it's a Mesh (primitive objects)
                    Mesh* originalMesh = dynamic_cast<Mesh*>(originalRender.renderable);
                    if (originalMesh) {
                        // Create a new Mesh with the same data
                        // This requires access to the mesh's vertex data
                        // For now, share the mesh but create independent render component
                        newRender.renderable = originalRender.renderable;
                        m_ConsoleMessages.push_back("WARNING: Mesh renderable shared - geometric data is shared but transform is independent");
                    } else {
                        // Unknown renderable type, share it
                        newRender.renderable = originalRender.renderable;
                        m_ConsoleMessages.push_back("WARNING: Unknown renderable type shared");
                    }
                }
            }
            
            m_Coordinator->AddComponent(newEntity, newRender);
            m_ConsoleMessages.push_back("Copied render component");
        } catch (...) {}
        
        // Copy Physics component if it exists - CREATE NEW PHYSICS BODY
        try {
            auto& originalPhysics = m_Coordinator->GetComponent<PhysicsComponent>(originalEntity);
            if (originalPhysics.rigidBody && m_PhysicsSystem) {
                
                PhysicsComponent newPhysics;
                newPhysics.syncTransform = originalPhysics.syncTransform;
                
                // Get the original body's properties
                btRigidBody* originalBody = originalPhysics.rigidBody;
                btCollisionShape* originalShape = originalBody->getCollisionShape();
                float mass = originalBody->getMass();
                
                // Create new collision shape of the same type
                btCollisionShape* newShape = nullptr;
                
                // Check shape type and create appropriate copy
                if (originalShape->getShapeType() == BOX_SHAPE_PROXYTYPE) {
                    btBoxShape* boxShape = static_cast<btBoxShape*>(originalShape);
                    btVector3 halfExtents = boxShape->getHalfExtentsWithMargin();
                    newShape = new btBoxShape(halfExtents);
                } else if (originalShape->getShapeType() == SPHERE_SHAPE_PROXYTYPE) {
                    btSphereShape* sphereShape = static_cast<btSphereShape*>(originalShape);
                    float radius = sphereShape->getRadius();
                    newShape = new btSphereShape(radius);
                } else {
                    // For other shapes, create a generic box
                    newShape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
                    m_ConsoleMessages.push_back("WARNING: Unknown collision shape, using box");
                }
                
                if (newShape) {
                    // Calculate inertia
                    btVector3 localInertia(0, 0, 0);
                    if (mass != 0.0f) {
                        newShape->calculateLocalInertia(mass, localInertia);
                    }
                    
                    // Get transform for new body (offset position)
                    btTransform newTransform;
                    newTransform.setIdentity();
                    
                    try {
                        auto& transformComp = m_Coordinator->GetComponent<TransformComponent>(newEntity);
                        newTransform.setOrigin(btVector3(transformComp.position.x, transformComp.position.y, transformComp.position.z));
                    } catch (...) {
                        newTransform.setOrigin(btVector3(2.0f, 1.0f, 0.0f));  // Default offset position
                    }
                    
                    // Create motion state and rigid body
                    btDefaultMotionState* motionState = new btDefaultMotionState(newTransform);
                    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, newShape, localInertia);
                    btRigidBody* newBody = new btRigidBody(rbInfo);
                    
                    // Copy physics properties
                    newBody->setFriction(originalBody->getFriction());
                    newBody->setRestitution(originalBody->getRestitution());
                    
                    // Add to physics world
                    m_PhysicsSystem->AddRigidBody(newBody);
                    
                    newPhysics.rigidBody = newBody;
                    m_Coordinator->AddComponent(newEntity, newPhysics);
                    m_ConsoleMessages.push_back("Created independent physics body for duplicate");
                }
            }
        } catch (...) {}
        
        m_ConsoleMessages.push_back("Entity duplicated successfully with independent components");
        SelectEntity(newEntity);
        
    } catch (...) {
        m_ConsoleMessages.push_back("ERROR: Failed to duplicate entity");
    }
}

void UIManager::ResetEntityPosition(Entity entity) {
    if (!m_Coordinator) return;
    
    try {
        auto& transform = m_Coordinator->GetComponent<TransformComponent>(entity);
        transform.position = glm::vec3(0.0f, 1.0f, 0.0f);
        transform.rotation = glm::vec3(0.0f);
        
        // Also reset physics body position if it exists
        try {
            auto& physics = m_Coordinator->GetComponent<PhysicsComponent>(entity);
            if (physics.rigidBody) {
                btTransform resetTransform;
                resetTransform.setIdentity();
                resetTransform.setOrigin(btVector3(0.0f, 1.0f, 0.0f));
                physics.rigidBody->setWorldTransform(resetTransform);
                physics.rigidBody->getMotionState()->setWorldTransform(resetTransform);
                physics.rigidBody->setLinearVelocity(btVector3(0, 0, 0));
                physics.rigidBody->setAngularVelocity(btVector3(0, 0, 0));
                physics.rigidBody->activate(true);
            }
        } catch (...) {}
        
        m_ConsoleMessages.push_back("Reset position for entity " + std::to_string(entity));
        
    } catch (...) {
        m_ConsoleMessages.push_back("ERROR: Cannot reset position - entity has no transform component");
    }
}

void UIManager::DeleteSelectedEntities() {
    if (m_MultiSelectMode && !m_SelectedEntities.empty()) {
        m_ConsoleMessages.push_back("Deleting " + std::to_string(m_SelectedEntities.size()) + " selected entities");
        
        // Create a copy of the selected entities list to avoid iterator invalidation
        std::vector<Entity> entitiesToDelete = m_SelectedEntities;
        
        for (Entity entity : entitiesToDelete) {
            DeleteEntity(entity);
        }
        
        // Clear selections
        m_SelectedEntities.clear();
        m_HasSelectedEntity = false;
        m_SelectedEntity = 0;
        
        m_ConsoleMessages.push_back("Finished deleting selected entities");
    } else if (m_HasSelectedEntity) {
        // Delete single selected entity
        DeleteEntity(m_SelectedEntity);
    } else {
        m_ConsoleMessages.push_back("No entities selected for deletion");
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
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
    if (ImGui::Button("X", buttonSize))
        values.x = resetValue;
    ImGui::PopStyleColor(3);
    
    ImGui::SameLine();
    ImGui::SetNextItemWidth(widthEach);
    ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::SameLine();
    
    // Y Component
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    if (ImGui::Button("Y", buttonSize))
        values.y = resetValue;
    ImGui::PopStyleColor(3);
    
    ImGui::SameLine();
    ImGui::SetNextItemWidth(widthEach);
    ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::SameLine();
    
    // Z Component
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.35f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
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

void UIManager::DrawConsole() {
    ImGui::Begin("Console", &m_ShowConsole);
    
    // Console toolbar
    if (ImGui::Button("Clear")) {
        m_ConsoleMessages.clear();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Copy All")) {
        std::string allMessages;
        for (const auto& message : m_ConsoleMessages) {
            allMessages += message + "\n";
        }
        ImGui::SetClipboardText(allMessages.c_str());
    }
    
    ImGui::Separator();
    
    // Display console messages with auto-scroll
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
    
    for (size_t i = 0; i < m_ConsoleMessages.size(); ++i) {
        const std::string& message = m_ConsoleMessages[i];
        
        // Color coding messages based on content
        ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default white
        
        if (message.find("ERROR:") != std::string::npos) {
            color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red for errors
        } else if (message.find("WARNING:") != std::string::npos) {
            color = ImVec4(1.0f, 0.7f, 0.3f, 1.0f); // Orange for warnings
        } else if (message.find("SUCCESS") != std::string::npos || message.find("successfully") != std::string::npos) {
            color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f); // Green for success
        } else if (message.find("Created") != std::string::npos || message.find("Added") != std::string::npos) {
            color = ImVec4(0.7f, 0.7f, 1.0f, 1.0f); // Light blue for creation messages
        }
        
        ImGui::TextColored(color, "%s", message.c_str());
    }
    
    // Auto-scroll to bottom if we were at the bottom
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
    
    // Input field for commands (future feature)
    static char commandBuffer[512] = "";
    if (ImGui::InputText("Command", commandBuffer, sizeof(commandBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        std::string command = std::string(commandBuffer);
        if (!command.empty()) {
            m_ConsoleMessages.push_back("> " + command);
            // TODO: Process commands
            m_ConsoleMessages.push_back("Command processing not yet implemented");
            memset(commandBuffer, 0, sizeof(commandBuffer));
        }
    }
    
    ImGui::End();
}