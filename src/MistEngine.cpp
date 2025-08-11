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
#include "GameManager.h"     // FPS Game Manager
#include <glm/gtc/type_ptr.hpp>

// ECS includes
#include "ECS/Coordinator.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/ECSPhysicsSystem.h"  // ECS physics system

// FPS Game ECS components
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/WeaponComponent.h"
#include "ECS/Components/BotComponent.h" 
#include "ECS/Components/HealthComponent.h"
#include "ECS/Components/ProjectileComponent.h"

// FPS Game ECS systems
#include "ECS/Systems/PlayerSystem.h"
#include "ECS/Systems/WeaponSystem.h"
#include "ECS/Systems/BotSystem.h"

// Global ECS coordinator
Coordinator gCoordinator;

// Global managers
UIManager* g_uiManager = nullptr;
InputManager* g_inputManager = nullptr;
ModuleManager* g_moduleManager = nullptr;
GameManager* g_gameManager = nullptr;
PhysicsSystem* g_physicsSystem = nullptr;

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
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !g_gameManager->IsGameMode())
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

    std::cout << "=== MistEngine v0.3.0 - FPS Game Edition ===" << std::endl;
    std::cout << "Scene Editor Mode: Enabled (F3 to toggle Game Mode)" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  EDITOR MODE:" << std::endl;
    std::cout << "  - WASD/QE: Camera movement" << std::endl;
    std::cout << "  - RIGHT-CLICK + HOLD: Enable mouse look" << std::endl;
    std::cout << "  - Mouse Scroll: Zoom in/out" << std::endl;
    std::cout << "  GAME MODE:" << std::endl;
    std::cout << "  - WASD: Player movement" << std::endl;
    std::cout << "  - Mouse: Look around (locked cursor)" << std::endl;
    std::cout << "  - LEFT CLICK: Shoot" << std::endl;
    std::cout << "  - R: Reload" << std::endl;
    std::cout << "  - SPACE: Jump" << std::endl;
    std::cout << "  - ESC: Pause/Resume" << std::endl;
    std::cout << "  GENERAL:" << std::endl;
    std::cout << "  - F3: Toggle Scene Editor / Game Mode" << std::endl;
    std::cout << "  - F1: Toggle ImGui Demo" << std::endl;
    std::cout << "  - F2: Toggle AI Assistant" << std::endl;

    // Initialize ECS
    gCoordinator.Init();

    // Register components
    gCoordinator.RegisterComponent<TransformComponent>();
    gCoordinator.RegisterComponent<RenderComponent>();
    gCoordinator.RegisterComponent<PhysicsComponent>();
    
    // Register FPS game components
    gCoordinator.RegisterComponent<PlayerComponent>();
    gCoordinator.RegisterComponent<WeaponComponent>();
    gCoordinator.RegisterComponent<BotComponent>();
    gCoordinator.RegisterComponent<HealthComponent>();
    gCoordinator.RegisterComponent<ProjectileComponent>();

    // Register systems
    auto renderSystem = gCoordinator.RegisterSystem<RenderSystem>();
    auto ecsPhysicsSystem = gCoordinator.RegisterSystem<ECSPhysicsSystem>();
    
    // Register FPS game systems
    auto playerSystem = gCoordinator.RegisterSystem<PlayerSystem>();
    auto weaponSystem = gCoordinator.RegisterSystem<WeaponSystem>();
    auto botSystem = gCoordinator.RegisterSystem<BotSystem>();

    // Set system signatures
    Signature renderSignature;
    renderSignature.set(gCoordinator.GetComponentType<TransformComponent>());
    renderSignature.set(gCoordinator.GetComponentType<RenderComponent>());
    gCoordinator.SetSystemSignature<RenderSystem>(renderSignature);

    Signature physicsSignature;
    physicsSignature.set(gCoordinator.GetComponentType<TransformComponent>());
    physicsSignature.set(gCoordinator.GetComponentType<PhysicsComponent>());
    gCoordinator.SetSystemSignature<ECSPhysicsSystem>(physicsSignature);
    
    // Set FPS game system signatures
    Signature playerSignature;
    playerSignature.set(gCoordinator.GetComponentType<PlayerComponent>());
    playerSignature.set(gCoordinator.GetComponentType<TransformComponent>());
    gCoordinator.SetSystemSignature<PlayerSystem>(playerSignature);
    
    Signature weaponSignature;
    weaponSignature.set(gCoordinator.GetComponentType<WeaponComponent>());
    weaponSignature.set(gCoordinator.GetComponentType<PlayerComponent>());
    gCoordinator.SetSystemSignature<WeaponSystem>(weaponSignature);
    
    Signature botSignature;
    botSignature.set(gCoordinator.GetComponentType<BotComponent>());
    botSignature.set(gCoordinator.GetComponentType<TransformComponent>());
    gCoordinator.SetSystemSignature<BotSystem>(botSignature);

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

    // Initialize Input Manager AFTER UI Manager (NEW!)
    // This ensures ImGui doesn't override our callbacks
    InputManager inputManager;
    g_inputManager = &inputManager;
    inputManager.Initialize(renderer.GetWindow());
    inputManager.SetCamera(&renderer.GetCamera());
    inputManager.EnableSceneEditorMode(true); // Start in scene editor mode
    std::cout << "Input Manager initialized AFTER UI Manager - callbacks should work now" << std::endl;

    // Initialize Module Manager (NEW!)
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

    // Initialize Physics (original system)
    PhysicsSystem physicsSystem;
    g_physicsSystem = &physicsSystem;
    uiManager.SetPhysicsSystem(&physicsSystem);
    
    // Initialize FPS Game Manager (NEW!)
    GameManager gameManager;
    g_gameManager = &gameManager;
    gameManager.Initialize(renderer.GetWindow(), &renderer, &uiManager, &physicsSystem);
    
    // Initialize FPS game systems
    playerSystem->Init(renderer.GetWindow(), &renderer.GetCamera());
    weaponSystem->Init(renderer.GetWindow(), &renderer.GetCamera());
    
    // Pass system pointers to GameManager
    gameManager.SetSystems(playerSystem, weaponSystem, botSystem);

    // Set up UI references
    uiManager.SetCoordinator(&gCoordinator);
    moduleManager.SetScene(nullptr); // Will be set after scene creation

    // Create Scene (keeping for backward compatibility)
    Scene scene;
    uiManager.SetScene(&scene);
    moduleManager.SetScene(&scene);

    // Create glowing orb (existing system)
    Orb* glowingOrb = new Orb(glm::vec3(1.5f, 1.0f, 0.0f), 0.3f, glm::vec3(2.0f, 1.6f, 0.4f));
    scene.AddOrb(glowingOrb);

    // Load 3D model (existing system)
    Model* ourModel = new Model("models/backpack/backpack.obj");
    scene.AddRenderable(ourModel);

    // Create physics ground plane (ECS version)
    Entity groundEntity = gCoordinator.CreateEntity();

    btRigidBody* groundBody = physicsSystem.CreateGroundPlane(glm::vec3(0.0f, -0.5f, 0.0f));

    std::vector<Vertex> planeVertices;
    std::vector<unsigned int> planeIndices;
    generatePlaneMesh(planeVertices, planeIndices);
    std::vector<Texture> groundTextures;
    Mesh* groundMesh = new Mesh(planeVertices, planeIndices, groundTextures);

    gCoordinator.AddComponent(groundEntity, TransformComponent{
        glm::vec3(0.0f, -0.5f, 0.0f),
        glm::vec3(0.0f),
        glm::vec3(10.0f, 1.0f, 10.0f) // Larger ground plane
        });
    gCoordinator.AddComponent(groundEntity, RenderComponent{ groundMesh, true });
    gCoordinator.AddComponent(groundEntity, PhysicsComponent{ groundBody, true });

    // Create physics cube (ECS version)
    Entity cubeEntity = gCoordinator.CreateEntity();

    btRigidBody* cubeBody = physicsSystem.CreateCube(glm::vec3(0.0f, 0.5f, 0.0f), 1.0f);

    std::vector<Vertex> cubeVertices;
    std::vector<unsigned int> cubeIndices;
    generateCubeMesh(cubeVertices, cubeIndices);
    Texture cubeTexture;
    if (!cubeTexture.LoadFromFile("textures/container.jpg")) {
        std::cout << "Warning: Failed to load cube texture, continuing without texture" << std::endl;
    }
    std::vector<Texture> cubeTextures;
    if (cubeTexture.GetID() != 0) {
        cubeTextures.push_back(cubeTexture);
    }
    Mesh* cubeMesh = new Mesh(cubeVertices, cubeIndices, cubeTextures);

    gCoordinator.AddComponent(cubeEntity, TransformComponent{
        glm::vec3(0.0f, 0.5f, 0.0f),
        glm::vec3(0.0f),
        glm::vec3(1.0f)
        });
    gCoordinator.AddComponent(cubeEntity, RenderComponent{ cubeMesh, true });
    gCoordinator.AddComponent(cubeEntity, PhysicsComponent{ cubeBody, true });

    std::cout << "=== Initialization Complete ===" << std::endl;
    std::cout << "Engine ready!" << std::endl;
    std::cout << "Press F3 to enter FPS Game Mode!" << std::endl;

    // Main loop
    while (!glfwWindowShouldClose(renderer.GetWindow())) {
        float deltaTime = renderer.GetDeltaTime();

        // Handle UI toggle inputs (F1, F2) - only in editor mode
        if (!gameManager.IsGameMode()) {
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
        }

        // Update Game Manager (handles F3 toggle and game logic)
        gameManager.Update(deltaTime);

        // Update input system (only in editor mode, player system handles game mode input)
        if (!gameManager.IsGameMode()) {
            inputManager.Update(deltaTime);
        }

        // Legacy physics input (only in editor mode and for backwards compatibility)
        if (!gameManager.IsGameMode()) {
            ProcessLegacyPhysicsInput(renderer.GetWindow(), physicsSystem, scene.getPhysicsRenderables(), deltaTime);
        }

        // Update modules
        moduleManager.UpdateModules(deltaTime);

        // Physics Update (both systems)
        physicsSystem.Update(deltaTime);
        ecsPhysicsSystem->Update(deltaTime);
        
        // Update FPS game systems
        playerSystem->Update(deltaTime);
        weaponSystem->Update(deltaTime);
        botSystem->Update(deltaTime);

        // Render with UI
        renderer.RenderWithECSAndUI(scene, renderSystem, &uiManager);
    }

    std::cout << "=== MistEngine Shutting Down ===" << std::endl;
    
    // Cleanup
    uiManager.Shutdown();
    moduleManager.UnloadAllModules();
    
    delete groundMesh;
    delete cubeMesh;
    delete glowingOrb;
    delete ourModel;

    std::cout << "=== Shutdown Complete ===" << std::endl;
    return 0;
}

