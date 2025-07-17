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

// Global UI manager
UIManager* g_uiManager = nullptr;

// Input handling for UI
void ProcessInputWithUI(GLFWwindow* window, PhysicsSystem& physicsSystem, std::vector<PhysicsRenderable>& physicsRenderables, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Toggle UI demo window with F1
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
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

    // Check if ImGui wants to capture mouse/keyboard
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
        return; // Don't process game input when UI is active
    }

    // Camera controls
    Camera& camera = *reinterpret_cast<Camera*>(glfwGetWindowUserPointer(window));
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    // Physics controls
    if (!physicsRenderables.empty()) {
        // Assuming the cube is the second physics object added
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

int main() {
    // Settings
    const unsigned int SCR_WIDTH = 1200;
    const unsigned int SCR_HEIGHT = 800;

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

    // Set up UI references
    uiManager.SetCoordinator(&gCoordinator);
    uiManager.SetPhysicsSystem(nullptr); // Will be set after physics system is created

    // Set up mouse input for UI
    glfwSetInputMode(renderer.GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetWindowUserPointer(renderer.GetWindow(), &renderer.GetCamera());

    // Create Scene (keeping for backward compatibility)
    Scene scene;
    uiManager.SetScene(&scene);

    // Create glowing orb (existing system)
    Orb* glowingOrb = new Orb(glm::vec3(1.5f, 1.0f, 0.0f), 0.3f, glm::vec3(2.0f, 1.6f, 0.4f));
    scene.AddOrb(glowingOrb);

    // Load 3D model (existing system)
    Model* ourModel = new Model("models/backpack/backpack.obj");
    scene.AddRenderable(ourModel);

    // Initialize Physics (original system)
    PhysicsSystem physicsSystem;
    uiManager.SetPhysicsSystem(&physicsSystem);

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
        glm::vec3(1.0f)
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
        std::cerr << "Failed to load cube texture" << std::endl;
        return -1;
    }
    std::vector<Texture> cubeTextures;
    cubeTextures.push_back(cubeTexture);
    Mesh* cubeMesh = new Mesh(cubeVertices, cubeIndices, cubeTextures);

    gCoordinator.AddComponent(cubeEntity, TransformComponent{
        glm::vec3(0.0f, 0.5f, 0.0f),
        glm::vec3(0.0f),
        glm::vec3(1.0f)
        });
    gCoordinator.AddComponent(cubeEntity, RenderComponent{ cubeMesh, true });
    gCoordinator.AddComponent(cubeEntity, PhysicsComponent{ cubeBody, true });

    // Main loop
    while (!glfwWindowShouldClose(renderer.GetWindow())) {
        float deltaTime = renderer.GetDeltaTime();

        // Input handling with UI support
        ProcessInputWithUI(renderer.GetWindow(), physicsSystem, scene.getPhysicsRenderables(), deltaTime);

        // Physics Update (both systems)
        physicsSystem.Update(deltaTime);
        ecsPhysicsSystem->Update(deltaTime);

        // Render with UI
        renderer.RenderWithECSAndUI(scene, renderSystem, &uiManager);
    }

    // Cleanup
    uiManager.Shutdown();
    delete groundMesh;
    delete cubeMesh;
    delete glowingOrb;
    delete ourModel;

    return 0;
}

