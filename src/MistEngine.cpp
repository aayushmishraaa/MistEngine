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

int main() {
    // Settings
    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;

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

    // Create Scene (keeping for backward compatibility)
    Scene scene;

    // Create glowing orb (existing system)
    Orb* glowingOrb = new Orb(glm::vec3(1.5f, 1.0f, 0.0f), 0.3f, glm::vec3(2.0f, 1.6f, 0.4f));
    scene.AddOrb(glowingOrb);

    // Load 3D model (existing system)
    Model* ourModel = new Model("models/backpack/backpack.obj");
    scene.AddRenderable(ourModel);

    // Initialize Physics (original system)
    PhysicsSystem physicsSystem;

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

        // Input (keeping existing input system)
        renderer.ProcessInputWithPhysics(renderer.GetWindow(), physicsSystem, scene.getPhysicsRenderables());

        // Physics Update (both systems)
        physicsSystem.Update(deltaTime);
        ecsPhysicsSystem->Update(deltaTime);

        // Render (hybrid approach - both old and new systems)
        renderer.RenderWithECS(scene, renderSystem);
    }

    // Cleanup
    delete groundMesh;
    delete cubeMesh;
    delete glowingOrb;
    delete ourModel;

    return 0;
}

