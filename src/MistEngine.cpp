c++
#include <iostream>
#include <vector>
#include <cstddef>
#include <memory> // For std::unique_ptr

#include "Renderer.h"
#include "Scene.h"
#include "Model.h"
#include "Orb.h"
#include "PhysicsSystem.h" // Keep this for creating rigid bodies initially
#include "Mesh.h"
#include "Texture.h"
#include "ShapeGenerator.h"
#include "ECSManager.h" // Include ECSManager
#include "Components.h" // Include Components
#include "Systems.h"    // Include Systems

#include <glm/gtc/type_ptr.hpp>



int main() {
    // Settings
    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;

    // Create Renderer
    Renderer renderer(SCR_WIDTH, SCR_HEIGHT);
    if (!renderer.Init()) {
        return -1;
    }

    // Create Scene (which now contains the ECSManager)
    Scene scene;

    // --- Create Entities using ECS ---

    // Create glowing orb entity
    Entity glowingOrbEntity = scene.ecsManager.createEntity();
    scene.ecsManager.addComponent<PositionComponent>(glowingOrbEntity, glm::vec3(1.5f, 1.0f, 0.0f));
    // Assuming Orb has a RenderComponent and potentially an OrbComponent for glow effects
    // You'll need to create a Mesh for the Orb and add a RenderComponent
    // For now, we'll skip adding a RenderComponent to the Orb entity until Orb rendering is integrated into ECS.
    // scene.ecsManager.addComponent<RenderComponent>(glowingOrbEntity, orbMesh); // Add orbMesh when created
    // scene.ecsManager.addComponent<OrbComponent>(glowingOrbEntity, glm::vec3(2.0f, 1.6f, 0.4f)); // Add OrbComponent when created


    // Load 3D model (backpack) and create entity
    Model* ourModel = new Model("models/backpack/backpack.obj"); // Model loading is still separate
    Entity backpackEntity = scene.ecsManager.createEntity();
    scene.ecsManager.addComponent<PositionComponent>(backpackEntity, glm::vec3(0.0f, 0.0f, 0.0f));
    scene.ecsManager.addComponent<RenderComponent>(backpackEntity, ourModel->meshes[0]); // Assuming Model loads meshes


    // Initialize Physics System (keep the instance for creating rigid bodies initially)
    // The actual physics simulation will be handled by the PhysicsSystem instance in the systems list.
    PhysicsSystem physicsSystemCreator; // Use a temporary instance for creating rigid bodies


    // Create physics ground plane entity
    Entity groundEntity = scene.ecsManager.createEntity();
    // Create Bullet Physics rigid body for the ground
    btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 0); // Normal and distance from origin
    btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0.0f, -0.5f, 0.0f)));
    btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, groundMotionState, groundShape, btVector3(0, 0, 0)); // Mass 0 for static object
    btRigidBody* groundBody = new btRigidBody(groundRigidBodyCI);
    // Add components to ground entity
    scene.ecsManager.addComponent<PositionComponent>(groundEntity, glm::vec3(0.0f, -0.5f, 0.0f));
    // Create Mesh for the ground plane using ShapeGenerator
    std::vector<Vertex> planeVertices;
    std::vector<unsigned int> planeIndices;
    generatePlaneMesh(planeVertices, planeIndices);
    Mesh* groundMesh = new Mesh(planeVertices, planeIndices, std::vector<Texture>()); // Add textures if needed
    scene.ecsManager.addComponent<RenderComponent>(groundEntity, groundMesh);
    scene.ecsManager.addComponent<PhysicsComponent>(groundEntity, groundBody);
    // Add shapes and motion states to the physicsSystemCreator's lists for cleanup later
    physicsSystemCreator.collisionShapes.push_back(groundShape);
    physicsSystemCreator.motionStates.push_back(groundMotionState);
    physicsSystemCreator.dynamicsWorld->addRigidBody(groundBody); // Add to the temporary world for creation


    // Create physics cube entity
    Entity cubeEntity = scene.ecsManager.createEntity();
    // Create Bullet Physics rigid body for the cube
    btCollisionShape* cubeShape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f)); // Half extents
    btDefaultMotionState* cubeMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), glm::vec3(0.0f, 0.5f, 0.0f)));
    btVector3 fallInertia(0, 0, 0);
    float cubeMass = 1.0f;
    cubeShape->calculateLocalInertia(cubeMass, fallInertia);
    btRigidBody::btRigidBodyConstructionInfo cubeRigidBodyCI(cubeMass, cubeMotionState, cubeShape, fallInertia);
    btRigidBody* cubeBody = new btRigidBody(cubeRigidBodyCI);
    // Add components to cube entity
    scene.ecsManager.addComponent<PositionComponent>(cubeEntity, glm::vec3(0.0f, 0.5f, 0.0f));
    // Create Mesh for the cube using ShapeGenerator
    std::vector<Vertex> cubeVertices;
    std::vector<unsigned int> cubeIndices;
    generateCubeMesh(cubeVertices, cubeIndices);
    Texture cubeTexture;
    if (!cubeTexture.LoadFromFile("textures/container.jpg")) {
        std::cerr << "Failed to load cube texture" << std::endl;
        // Handle error
    }
    std::vector<Texture> cubeTextures;
    cubeTextures.push_back(cubeTexture);
    Mesh* cubeMesh = new Mesh(cubeVertices, cubeIndices, cubeTextures);
    scene.ecsManager.addComponent<RenderComponent>(cubeEntity, cubeMesh);
    scene.ecsManager.addComponent<PhysicsComponent>(cubeEntity, cubeBody);
    // Add shapes and motion states to the physicsSystemCreator's lists for cleanup later
    physicsSystemCreator.collisionShapes.push_back(cubeShape);
    physicsSystemCreator.motionStates.push_back(cubeMotionState);
    physicsSystemCreator.dynamicsWorld->addRigidBody(cubeBody); // Add to the temporary world for creation


    // --- Setup Systems ---
    std::vector<std::unique_ptr<System>> systems;
    systems.push_back(std::make_unique<RenderSystem>(&renderer)); // RenderSystem needs the renderer
    systems.push_back(std::make_unique<PhysicsSystem>());       // PhysicsSystem initializes its own Bullet world


    // --- Main loop ---
    while (!glfwWindowShouldClose(renderer.GetWindow())) {
        // Calculate delta time is now in Renderer::Render() - but we need it for system updates

        // Input
        // Pass the cube entity ID to the input processing function
        renderer.ProcessInputWithPhysics(renderer.GetWindow(), dynamic_cast<PhysicsSystem&>(*systems[1]), scene);


        // Update Systems
        float deltaTime = renderer.GetDeltaTime(); // Get delta time from renderer
        for (const auto& system : systems) {
            system->update(deltaTime, scene);
        }

        // Render (Renderer still handles the main drawing calls)
        renderer.Render(scene);

    }

    // Cleanup: Systems' destructors will handle their cleanup.
    // Meshes and Models created with 'new' still need to be deleted.
    delete ourModel;
    delete groundMesh;
    delete cubeMesh;
    // The physicsSystemCreator will be destroyed when it goes out of scope,
    // cleaning up the rigid bodies, shapes, and motion states it created.

    return 0;
}
