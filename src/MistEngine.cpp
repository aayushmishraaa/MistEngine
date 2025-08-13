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

    std::cout << "=== MistEngine v0.2.0 Starting Up ===" << std::endl;
    std::cout << "Clean Scene Mode: Enabled" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  - WASD/QE: Camera movement (always available)" << std::endl;
    std::cout << "  - RIGHT-CLICK + HOLD: Enable mouse look (locked by default)" << std::endl;
    std::cout << "  - Mouse Scroll: Zoom in/out" << std::endl;
    std::cout << "  - F3: Toggle Scene Editor / Gameplay mode" << std::endl;
    std::cout << "  - F1: Toggle ImGui Demo" << std::endl;
    std::cout << "  - F2: Toggle AI Assistant" << std::endl;
    std::cout << "  - GameObject Menu: Create cubes, planes, spheres, etc." << std::endl;
    std::cout << "  - Unity-like Directional Light and Skyline Background" << std::endl;

    // Initialize ECS
    gCoordinator.Init();

    // Register components
    gCoordinator.RegisterComponent<TransformComponent>();
    gCoordinator.RegisterComponent<RenderComponent>();
    gCoordinator.RegisterComponent<PhysicsComponent>();

    // Register systems
    auto renderSystem = gCoordinator.RegisterSystem<RenderSystem>();
    auto ecsPhysicsSystem = gCoordinator.RegisterSystem<ECSPhysicsSystem>();

    // Set system signatures
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

    // Create Clean Scene - NO DEFAULT OBJECTS
    Scene scene;
    uiManager.SetScene(&scene);
    uiManager.SetCoordinator(&gCoordinator);  // Add this missing line!
    std::cout << "UIManager: Coordinator reference set successfully" << std::endl;
    moduleManager.SetScene(&scene);

    // Initialize Physics System (for when objects are created via UI)
    PhysicsSystem physicsSystem;
    uiManager.SetPhysicsSystem(&physicsSystem);
    std::cout << "UIManager: PhysicsSystem reference set successfully" << std::endl;

    std::cout << "=== Clean Scene Initialization Complete ===" << std::endl;
    std::cout << "Scene is empty! Use the GameObject menu to add cubes, planes, spheres, etc." << std::endl;
    std::cout << "Engine ready with Unity-like directional lighting and skyline background!" << std::endl;

    // Main loop
    while (!glfwWindowShouldClose(renderer.GetWindow())) {
        float deltaTime = renderer.GetDeltaTime();

        // Handle UI toggle inputs (F1, F2)
        if (glfwGetKey(renderer.GetWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(renderer.GetWindow(), true);

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

        // Update new input system
        inputManager.Update(deltaTime);

        // Legacy physics input (for backwards compatibility) - now only if objects exist
        if (!scene.getPhysicsRenderables().empty()) {
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
    uiManager.Shutdown();
    moduleManager.UnloadAllModules();

    std::cout << "=== Shutdown Complete ===" << std::endl;
    return 0;
}

