#include <iostream>
#include <vector>
#include <cstddef> 

#include "Renderer.h"
#include "Scene.h"
#include "Model.h"
#include "Orb.h"
#include "PhysicsSystem.h"  // Original physics system
#include "Mesh.h" 
#include "Texture.h" 
#include "ShapeGenerator.h" 
#include "UIManager.h"
#include "InputManager.h"    // New input system
#include "ModuleManager.h"   // New module system
#include "FPSGameManager.h"  // FPS Game Manager
#include "Version.h"         // Version information
#include <glm/gtc/type_ptr.hpp>

// ECS includes
#include "ECS/Coordinator.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/ECSPhysicsSystem.h"  // ECS physics system

// Global ECS coordinator
Coordinator gCoordinator;

// Global managers
UIManager* g_uiManager = nullptr;
InputManager* g_inputManager = nullptr;
ModuleManager* g_moduleManager = nullptr;
FPSGameManager* g_fpsGameManager = nullptr;  // NEW: FPS Game Manager

// Global physics system reference for FPS enemies
PhysicsSystem* g_physicsSystem = nullptr;

// Global enemy system reference for projectile damage
EnemyAISystem* g_enemySystem = nullptr;

// Legacy input handling for backward compatibility with physics controls
void ProcessLegacyPhysicsInput(GLFWwindow* window, PhysicsSystem& physicsSystem, std::vector<PhysicsRenderable>& physicsRenderables, float deltaTime) {
    // Check if ImGui wants to capture input
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
        return;
    }

    // Physics controls (legacy system - keep for now)
    if (!physicsRenderables.empty() && physicsRenderables.size() > 1) {
        auto cubeBody = physicsRenderables[1].body;
        float force = 100.0f;

        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
            physicsSystem.ApplyForce(cubeBody, glm::vec3(0.0f, 0.0f, -force));
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
            physicsSystem.ApplyForce(cubeBody, glm::vec3(0.0f, 0.0f, force));
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
            physicsSystem.ApplyForce(cubeBody, glm::vec3(-force, 0.0f, 0.0f));
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
            physicsSystem.ApplyForce(cubeBody, glm::vec3(force, 0.0f, 0.0f));
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
             physicsSystem.ApplyForce(cubeBody, glm::vec3(0.0f, force * 2.0f, 0.0f));
    }
}

// Helper function to check if directory exists (C++14 compatible)
bool DirectoryExists(const std::string& path) {
#ifdef _WIN32
    DWORD attributes = GetFileAttributesA(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat statbuf;
    return (stat(path.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
#endif
}

int main() {
    // Settings
    const unsigned int SCR_WIDTH = 1200;
    const unsigned int SCR_HEIGHT = 800;

    std::cout << "=== " << MIST_ENGINE_NAME << " " << MIST_ENGINE_VERSION_STRING << " with FPS Game Starting Up ===" << std::endl;
    std::cout << "NEW: FPS Game Mode Available!" << std::endl;
    std::cout << "Built with " << MIST_ENGINE_COMPILER << " on " << MIST_ENGINE_PLATFORM << std::endl;
    std::cout << "Build Type: " << MIST_ENGINE_BUILD_TYPE << std::endl;
    std::cout << "Features: ";
    #if MIST_ENGINE_HAS_AI_INTEGRATION
    std::cout << "AI ";
    #endif
    #if MIST_ENGINE_HAS_PHYSICS
    std::cout << "Physics ";
    #endif
    #if MIST_ENGINE_HAS_OPENGL
    std::cout << "OpenGL ";
    #endif
    #if MIST_ENGINE_HAS_IMGUI
    std::cout << "ImGui ";
    #endif
    #if MIST_ENGINE_HAS_FPS_GAME
    std::cout << "FPS-Game ";
    #endif
    std::cout << std::endl;

    // Initialize ECS
    gCoordinator.Init();

    // Register core components
    gCoordinator.RegisterComponent<TransformComponent>();
    gCoordinator.RegisterComponent<RenderComponent>();
    gCoordinator.RegisterComponent<PhysicsComponent>();

    // Register core systems
    auto renderSystem = gCoordinator.RegisterSystem<RenderSystem>();
    auto ecsPhysicsSystem = gCoordinator.RegisterSystem<ECSPhysicsSystem>();

    // Set core system signatures
    Signature renderSignature;
    renderSignature.set(gCoordinator.GetComponentType<TransformComponent>());
    renderSignature.set(gCoordinator.GetComponentType<RenderComponent>());
    gCoordinator.SetSystemSignature<RenderSystem>(renderSignature);

    Signature physicsSignature;
    physicsSignature.set(gCoordinator.GetComponentType<TransformComponent>());
    physicsSignature.set(gCoordinator.GetComponentType<PhysicsComponent>());
    gCoordinator.SetSystemSignature<ECSPhysicsSystem>(physicsSignature);

    // Create Renderer
    Renderer renderer(SCR_WIDTH, SCR_HEIGHT);
    if (!renderer.Init()) {
        return -1;
    }

    // Initialize UI Manager
    UIManager uiManager;
    g_uiManager = &uiManager;
    if (!uiManager.Initialize(renderer.GetWindow())) {
        std::cerr << "Failed to initialize UI Manager" << std::endl;
        return -1;
    }

    // Initialize Input Manager AFTER UI Manager
    InputManager inputManager;
    g_inputManager = &inputManager;
    inputManager.Initialize(renderer.GetWindow());
    inputManager.SetCamera(&renderer.GetCamera());
    inputManager.EnableSceneEditorMode(true); // Start in scene editor mode
    std::cout << "Input Manager initialized successfully" << std::endl;

    // Initialize Physics System
    PhysicsSystem physicsSystem;
    g_physicsSystem = &physicsSystem;  // Set global reference for FPS enemies
    uiManager.SetPhysicsSystem(&physicsSystem);
    
    // NEW: Initialize FPS Game Manager
    FPSGameManager fpsGameManager;
    g_fpsGameManager = &fpsGameManager;
    if (!fpsGameManager.Initialize(&inputManager, &renderer.GetCamera(), &uiManager, &physicsSystem)) {
        std::cerr << "Failed to initialize FPS Game Manager" << std::endl;
        return -1;
    }
    
    // Set global enemy system reference after FPS manager is initialized
    if (fpsGameManager.m_enemySystem) {
        g_enemySystem = fpsGameManager.m_enemySystem.get();
    }
    
    std::cout << "FPS Game Manager initialized successfully" << std::endl;

    // Initialize Module Manager
    ModuleManager moduleManager;
    g_moduleManager = &moduleManager;
    moduleManager.SetCoordinator(&gCoordinator);
    moduleManager.SetRenderer(&renderer);
    
    // Try to load modules from modules directory
    if (DirectoryExists("modules")) {
        std::cout << "Loading modules from 'modules' directory..." << std::endl;
        moduleManager.LoadModulesFromDirectory("modules");
    } else {
        std::cout << "No 'modules' directory found - continuing without external modules" << std::endl;
    }

    // Create Clean Scene
    Scene scene;
    uiManager.SetScene(&scene);
    uiManager.SetCoordinator(&gCoordinator);
    uiManager.SetFPSGameManager(&fpsGameManager);  // NEW: Set FPS Game Manager reference
    moduleManager.SetScene(&scene);

    std::cout << "=== Engine Initialization Complete ===" << std::endl;
    std::cout << "Ready! Press SPACE to start FPS game, or use Scene Editor (F3)" << std::endl;
    std::cout << "FPS Game Features:" << std::endl;
    std::cout << "  - Room-based levels with enemy AI" << std::endl;
    std::cout << "  - Multiple weapon types" << std::endl;
    std::cout << "  - Projectile physics and hit detection" << std::endl;
    std::cout << "  - Score and health systems" << std::endl;

    // Main loop
    while (!glfwWindowShouldClose(renderer.GetWindow())) {
        float deltaTime = renderer.GetDeltaTime();

        // Handle ESC to close (only if not in FPS game mode)
        if (glfwGetKey(renderer.GetWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            if (!fpsGameManager.IsGameActive()) {
                glfwSetWindowShouldClose(renderer.GetWindow(), true);
            }
        }

        // Toggle UI demo window with F1
        if (glfwGetKey(renderer.GetWindow(), GLFW_KEY_F1) == GLFW_PRESS) {
            static bool f1Pressed = false;
            if (!f1Pressed) {
                if (g_uiManager) {
                    g_uiManager->SetShowDemo(!g_uiManager->IsShowingDemo());
                }
                f1Pressed = true;
            }
        } else {
            static bool f1Pressed = false;
            f1Pressed = false;
        }

        // Toggle AI window with F2
        if (glfwGetKey(renderer.GetWindow(), GLFW_KEY_F2) == GLFW_PRESS) {
            static bool f2Pressed = false;
            if (!f2Pressed) {
                if (g_uiManager) {
                    g_uiManager->SetShowAI(!g_uiManager->IsShowingAI());
                }
                f2Pressed = true;
            }
        } else {
            static bool f2Pressed = false;
            f2Pressed = false;
        }

        // NEW: Update FPS Game Manager FIRST (before legacy physics input)
        fpsGameManager.Update(deltaTime);

        // Update input system
        inputManager.Update(deltaTime);

        // Legacy physics input (for backwards compatibility) - only if FPS game is not active
        if (!scene.getPhysicsRenderables().empty() && !fpsGameManager.IsGameActive()) {
            ProcessLegacyPhysicsInput(renderer.GetWindow(), physicsSystem, scene.getPhysicsRenderables(), deltaTime);
        }

        // Update modules
        moduleManager.UpdateModules(deltaTime);

        // Physics Update (both systems)
        physicsSystem.Update(deltaTime);
        ecsPhysicsSystem->Update(deltaTime);

        // Render with UI
        renderer.RenderWithECSAndUI(scene, renderSystem, &uiManager);
    }

    std::cout << "=== MistEngine Shutting Down ===" << std::endl;
    
    // Cleanup
    fpsGameManager.Shutdown();  // NEW: Cleanup FPS Game Manager
    uiManager.Shutdown();
    moduleManager.UnloadAllModules();

    std::cout << "=== Shutdown Complete ===" << std::endl;
    return 0;
}

