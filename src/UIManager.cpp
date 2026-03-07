#include "UIManager.h"
#include "GameExporter.h"
#include "FPSGameManager.h"
#include "ECS/Systems/EnemyAISystem.h"
#include "ECS/Coordinator.h"
#include "Editor/GameHUD.h"
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
#include "Version.h"
#include "Renderer.h"
#include "Editor/UndoRedo.h"
#include "Editor/AssetBrowser.h"
#include "Editor/GizmoSystem.h"
#include "Editor/EditorState.h"
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
    , m_ShowAI(false)
    , m_ShowAPIKeyDialog(false)
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
    , m_FPSGameManager(nullptr)
    , m_SelectedProvider(0)
    , m_EntityCounter(0)
{
    // Initialize AI components
    m_AIManager = std::make_unique<AIManager>();
    m_AIWindow = std::make_unique<AIWindow>();
    m_AIWindow->SetAIManager(m_AIManager.get());

    // Initialize editor subsystems
    m_UndoRedo = std::make_unique<UndoRedoManager>();
    m_AssetBrowser = std::make_unique<AssetBrowser>();
    m_GizmoSystem = std::make_unique<GizmoSystem>();
    m_EditorState = std::make_unique<EditorState>();
    m_ConsoleSystem = std::make_unique<ConsoleSystem>();
    m_ConsoleSystem->RegisterBuiltins();

    m_GameExporter = std::make_unique<GameExporter>();

    // Initialize dialog buffers
    memset(m_APIKeyBuffer, 0, sizeof(m_APIKeyBuffer));
    memset(m_EndpointBuffer, 0, sizeof(m_EndpointBuffer));
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

void UIManager::SetFPSGameManager(FPSGameManager* fpsManager) {
    m_FPSGameManager = fpsManager;
    if (m_EditorState) {
        m_EditorState->SetSnapshotCallbacks(
            [this]() { if (m_FPSGameManager) m_FPSGameManager->StartNewGame(); },
            [this]() { if (m_FPSGameManager) m_FPSGameManager->StopGame(); }
        );
    }
}

bool UIManager::Initialize(GLFWwindow* window) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // Enable keyboard controls and clipboard
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

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

    // Setup Dear ImGui style — Godot-inspired dark theme
    ImGui::StyleColorsDark();
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.FrameRounding = 2.0f;
        style.ScrollbarRounding = 2.0f;
        style.TabRounding = 2.0f;
        style.WindowBorderSize = 1.0f;
        style.WindowPadding = ImVec2(8, 8);
        style.FramePadding = ImVec2(5, 3);
        style.ItemSpacing = ImVec2(6, 4);

        ImVec4* c = style.Colors;
        c[ImGuiCol_WindowBg]        = ImVec4(0.15f, 0.16f, 0.19f, 1.0f);
        c[ImGuiCol_TitleBg]         = ImVec4(0.12f, 0.13f, 0.15f, 1.0f);
        c[ImGuiCol_TitleBgActive]   = ImVec4(0.12f, 0.13f, 0.15f, 1.0f);
        c[ImGuiCol_MenuBarBg]       = ImVec4(0.14f, 0.15f, 0.17f, 1.0f);
        c[ImGuiCol_Tab]             = ImVec4(0.18f, 0.19f, 0.22f, 1.0f);
        c[ImGuiCol_TabSelected]     = ImVec4(0.24f, 0.26f, 0.30f, 1.0f);
        c[ImGuiCol_Header]          = ImVec4(0.22f, 0.35f, 0.55f, 0.5f);
        c[ImGuiCol_HeaderHovered]   = ImVec4(0.26f, 0.45f, 0.65f, 0.7f);
        c[ImGuiCol_Button]          = ImVec4(0.24f, 0.26f, 0.30f, 1.0f);
        c[ImGuiCol_ButtonHovered]   = ImVec4(0.30f, 0.33f, 0.38f, 1.0f);
        c[ImGuiCol_FrameBg]         = ImVec4(0.20f, 0.21f, 0.24f, 1.0f);
        c[ImGuiCol_Separator]       = ImVec4(0.22f, 0.23f, 0.26f, 1.0f);
        c[ImGuiCol_PopupBg]         = ImVec4(0.17f, 0.18f, 0.21f, 0.95f);
        c[ImGuiCol_Border]          = ImVec4(0.22f, 0.23f, 0.26f, 1.0f);
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

    // Load AI configuration
    AIConfig::Instance().LoadFromFile();
    
    // Try to auto-initialize AI if API key is available
    if (AIConfig::Instance().HasAPIKey("Gemini")) {
        std::string apiKey = AIConfig::Instance().GetAPIKey("Gemini");
        InitializeAI(apiKey, "Gemini", "");
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
    m_ConsoleMessages.push_back("Press F2 to open AI assistant");
    m_ConsoleMessages.push_back("Ctrl+Z/Y for Undo/Redo");
    m_ConsoleMessages.push_back("Layout persists across sessions (imgui_layout.ini)");

    return true;
}

void UIManager::Shutdown() {
    if (!ImGui::GetCurrentContext()) return; // Already shut down

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
        // Live HUD from FPSGameManager
        auto hud = m_FPSGameManager->GetHUDData();
        GameHUD::HUDState hudState;
        hudState.health = (int)hud.health; hudState.maxHealth = (int)hud.maxHealth;
        hudState.ammo = hud.ammo; hudState.maxAmmo = hud.maxAmmo; hudState.reserveAmmo = hud.reserveAmmo;
        hudState.score = hud.score; hudState.weaponName = hud.weaponName;
        hudState.wave = hud.currentRoom + 1; hudState.isReloading = hud.isReloading;
        hudState.reloadProgress = hud.reloadProgress;
        GameHUD::Render(hudState, ImGui::GetIO().DeltaTime);

        if (isPlayerDead) {
            DrawGameOverScreen();
        }
    } else {
        // Process Ctrl+Z / Ctrl+Y for undo/redo
        ImGuiIO& undoIO = ImGui::GetIO();
        if (undoIO.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) && m_UndoRedo->CanUndo()) {
            m_UndoRedo->Undo();
            m_ConsoleMessages.push_back("Undo: " + m_UndoRedo->GetRedoDescription());
        }
        if (undoIO.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y) && m_UndoRedo->CanRedo()) {
            m_UndoRedo->Redo();
            m_ConsoleMessages.push_back("Redo: " + m_UndoRedo->GetUndoDescription());
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
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                m_ConsoleMessages.push_back("New scene created");
                m_EntityList.clear();
                m_HasSelectedEntity = false;
            }
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                SaveScene(m_ScenePathBuffer);
            }
            if (ImGui::MenuItem("Save Scene As...")) {
                ImGui::OpenPopup("SaveSceneAs");
            }
            if (ImGui::MenuItem("Load Scene", "Ctrl+O")) {
                LoadScene(m_ScenePathBuffer);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Export FPS Game...", "Ctrl+E")) {
                m_ShowExportDialog = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                // Will be handled by GLFW window close
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            bool canUndo = m_UndoRedo && m_UndoRedo->CanUndo();
            bool canRedo = m_UndoRedo && m_UndoRedo->CanRedo();
            std::string undoLabel = canUndo ? ("Undo " + m_UndoRedo->GetUndoDescription()) : "Undo";
            std::string redoLabel = canRedo ? ("Redo " + m_UndoRedo->GetRedoDescription()) : "Redo";
            if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, canUndo)) {
                m_UndoRedo->Undo();
            }
            if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, canRedo)) {
                m_UndoRedo->Redo();
            }
            ImGui::Separator();
            ImGui::Text("History: %zu undo, %zu redo",
                m_UndoRedo->GetUndoCount(), m_UndoRedo->GetRedoCount());
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
            ImGui::MenuItem("Ask AI", "F2", &m_ShowAI);
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
        if (ImGui::BeginMenu("AI")) {
            if (ImGui::MenuItem("Open AI Assistant", "F2")) {
                m_ShowAI = true;
            }
            ImGui::Separator();

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
                strncpy(m_APIKeyBuffer, clipText, sizeof(m_APIKeyBuffer) - 1); m_APIKeyBuffer[sizeof(m_APIKeyBuffer) - 1] = '\0';
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
            m_ConsoleMessages.push_back("   � Go to https://aistudio.google.com/app/apikey");
            m_ConsoleMessages.push_back("   � Sign in with your Google account");
            m_ConsoleMessages.push_back("   � Click 'Create API Key'");
            m_ConsoleMessages.push_back("   � Copy the generated key");
            m_ConsoleMessages.push_back("");
            m_ConsoleMessages.push_back("2. ENABLE GEMINI API:");
            m_ConsoleMessages.push_back("   � API is free with rate limits");
            m_ConsoleMessages.push_back("   � No billing setup required for basic usage");
            m_ConsoleMessages.push_back("   � Higher limits available with paid plans");
            m_ConsoleMessages.push_back("");
            m_ConsoleMessages.push_back("3. UPDATED MODELS (2024):");
            m_ConsoleMessages.push_back("   � gemini-1.5-flash: Fast & efficient (default)");
            m_ConsoleMessages.push_back("   � gemini-1.5-pro: Most capable model");
            m_ConsoleMessages.push_back("   � gemini-1.0-pro: Stable baseline");
            m_ConsoleMessages.push_back("   � Note: Old 'gemini-pro' is deprecated");
            m_ConsoleMessages.push_back("");
            m_ConsoleMessages.push_back("4. RATE LIMITS:");
            m_ConsoleMessages.push_back("   � Free tier: 15 requests/minute");
            m_ConsoleMessages.push_back("   � No daily token limits on free tier");
            m_ConsoleMessages.push_back("   � Much more generous than OpenAI free tier");
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
        strncpy(m_APIKeyBuffer, maskedKey.c_str(), sizeof(m_APIKeyBuffer) - 1); m_APIKeyBuffer[sizeof(m_APIKeyBuffer) - 1] = '\0';
    }
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

    // Build entity list
    if (m_Coordinator) {
        m_EntityList.clear();
        std::set<Entity> validEntities;
        for (int i = 0; i <= m_EntityCounter && i < MAX_ENTITIES; ++i) {
            Entity entity = static_cast<Entity>(i);
            bool hasComponents = false;
            try { m_Coordinator->GetComponent<TransformComponent>(entity); hasComponents = true; } catch (...) {}
            if (!hasComponents) { try { m_Coordinator->GetComponent<RenderComponent>(entity); hasComponents = true; } catch (...) {} }
            if (!hasComponents) { try { m_Coordinator->GetComponent<PhysicsComponent>(entity); hasComponents = true; } catch (...) {} }
            if (hasComponents) validEntities.insert(entity);
        }

        std::string filterStr(m_HierarchyFilter);
        std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

        for (Entity entity : validEntities) {
            // Get name from map, or generate default
            std::string name;
            auto it = m_EntityNames.find(entity);
            if (it != m_EntityNames.end()) {
                name = it->second;
            } else {
                name = "Entity " + std::to_string(entity);
            }

            // Apply filter
            if (!filterStr.empty()) {
                std::string lowerName = name;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                if (lowerName.find(filterStr) == std::string::npos) continue;
            }

            m_EntityList.push_back(std::make_pair(entity, name));
        }
    }

    // Display entity tree
    for (size_t i = 0; i < m_EntityList.size(); ++i) {
        Entity entity = m_EntityList[i].first;
        const std::string& name = m_EntityList[i].second;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (m_HasSelectedEntity && m_SelectedEntity == entity) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        ImGui::PushID((int)entity);
        ImGui::TreeNodeEx("##node", flags, "%s", name.c_str());

        if (ImGui::IsItemClicked()) {
            SelectEntity(entity);
        }

        // Right-click context menu
        if (ImGui::BeginPopupContextItem("EntityContext")) {
            if (ImGui::MenuItem("Rename")) {
                // Start inline rename — set as selected entity name
                SelectEntity(entity);
            }
            if (ImGui::MenuItem("Duplicate")) {
                // Simple duplicate: create entity with same components
                if (m_Coordinator) {
                    Entity newEntity = m_Coordinator->CreateEntity();
                    m_EntityCounter = std::max(m_EntityCounter, (int)newEntity + 1);
                    try {
                        auto& srcT = m_Coordinator->GetComponent<TransformComponent>(entity);
                        TransformComponent t = srcT;
                        t.position.x += 1.0f; // offset
                        m_Coordinator->AddComponent(newEntity, t);
                    } catch (...) {}
                    try {
                        auto& srcR = m_Coordinator->GetComponent<RenderComponent>(entity);
                        RenderComponent r = srcR;
                        m_Coordinator->AddComponent(newEntity, r);
                    } catch (...) {}
                    std::string newName = name + " (copy)";
                    m_EntityNames[newEntity] = newName;
                    m_ConsoleMessages.push_back("Duplicated: " + newName);
                    SelectEntity(newEntity);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete")) {
                DeleteEntity(entity);
                m_EntityNames.erase(entity);
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }

    if (m_EntityList.empty()) {
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
        ImGui::TextDisabled("No entity selected");
        ImGui::TextDisabled("Click an entity in Hierarchy");
    }
}

void UIManager::DrawSceneView() {
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
    if (m_Coordinator) {
        Entity entity = m_Coordinator->CreateEntity();
        m_EntityCounter = std::max(m_EntityCounter, (int)entity + 1);

        // Add default transform component
        TransformComponent transform;
        m_Coordinator->AddComponent(entity, transform);

        m_EntityNames[entity] = name;
        m_ConsoleMessages.push_back("Created entity: " + name);

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

        if (transformChanged && m_UndoRedo) {
            // Capture for undo
            Entity entity = m_SelectedEntity;
            glm::vec3 newPos = transform.position;
            glm::vec3 newRot = transform.rotation;
            glm::vec3 newScale = transform.scale;
            Coordinator* coord = m_Coordinator;

            m_UndoRedo->ExecuteCommand(std::make_unique<LambdaCommand>(
                "Transform",
                [coord, entity, newPos, newRot, newScale]() {
                    try {
                        auto& t = coord->GetComponent<TransformComponent>(entity);
                        t.position = newPos;
                        t.rotation = newRot;
                        t.scale = newScale;
                    } catch (...) {}
                },
                [coord, entity, originalPos, originalRot, originalScale]() {
                    try {
                        auto& t = coord->GetComponent<TransformComponent>(entity);
                        t.position = originalPos;
                        t.rotation = originalRot;
                        t.scale = originalScale;
                    } catch (...) {}
                }
            ));
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
    ImGui::Checkbox("Visible", &render.visible);

    if (render.renderable) {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Renderable: Active");
        // Try to show mesh info if it's a Mesh*
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
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
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
        if (playState == EditorPlayState::Paused && m_FPSGameManager) m_FPSGameManager->ResumeGame();
        m_EditorState->Play();
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, playState == EditorPlayState::Paused
        ? ImVec4(0.6f, 0.6f, 0.2f, 1.0f) : ImVec4(0.24f, 0.26f, 0.30f, 1.0f));
    if (ImGui::Button("Pause", ImVec2(0, 24)) && m_EditorState) {
        m_EditorState->Pause();
        if (m_FPSGameManager) m_FPSGameManager->PauseGame();
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

// --- Godot-like Fixed Layout ---

void UIManager::DrawEditorLayout() {
    ImGuiIO& io = ImGui::GetIO();
    float displayW = io.DisplaySize.x;
    float displayH = io.DisplaySize.y;
    float topOffset = m_Layout.menuBarHeight + m_Layout.toolbarHeight;

    float leftW = m_Layout.leftPanelVisible ? m_Layout.leftPanelWidth : 0.0f;
    float rightW = m_Layout.rightPanelVisible ? m_Layout.rightPanelWidth : 0.0f;
    float bottomH = m_Layout.bottomPanelVisible ? m_Layout.bottomPanelHeight : 0.0f;

    float centerW = displayW - leftW - rightW;
    float centerH = displayH - topOffset - bottomH;

    ImGuiWindowFlags panelFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;

    // === LEFT PANEL (Hierarchy) ===
    if (m_Layout.leftPanelVisible) {
        ImGui::SetNextWindowPos(ImVec2(0, topOffset));
        ImGui::SetNextWindowSize(ImVec2(leftW, centerH));
        ImGui::Begin("Hierarchy", &m_Layout.leftPanelVisible, panelFlags);
        DrawHierarchy();
        ImGui::End();
    }

    // === CENTER PANEL (Viewport) ===
    {
        ImGui::SetNextWindowPos(ImVec2(leftW, topOffset));
        ImGui::SetNextWindowSize(ImVec2(centerW, centerH));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport", nullptr, panelFlags | ImGuiWindowFlags_NoTitleBar);

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
            ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + offsetX, ImGui::GetCursorPosY() + offsetY));
            ImGui::Image((ImTextureID)(intptr_t)m_ViewportTexture,
                ImVec2(displayW2, displayH2),
                ImVec2(0, 1), ImVec2(1, 0));
        } else {
            ImGui::Text("No viewport texture");
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    // === RIGHT PANEL (Inspector) ===
    if (m_Layout.rightPanelVisible) {
        ImGui::SetNextWindowPos(ImVec2(displayW - rightW, topOffset));
        ImGui::SetNextWindowSize(ImVec2(rightW, centerH));
        ImGui::Begin("Inspector", &m_Layout.rightPanelVisible, panelFlags);
        DrawInspector();
        ImGui::End();
    }

    // === BOTTOM PANEL (Console / Asset Browser / Output tabs) ===
    if (m_Layout.bottomPanelVisible) {
        ImGui::SetNextWindowPos(ImVec2(0, topOffset + centerH));
        ImGui::SetNextWindowSize(ImVec2(displayW, bottomH));
        ImGui::Begin("##BottomPanel", &m_Layout.bottomPanelVisible, panelFlags);

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
    } else {
        m_ConsoleMessages.push_back("Failed to load scene from: " + path);
    }
}

UndoRedoManager& UIManager::GetUndoRedo() {
    return *m_UndoRedo;
}