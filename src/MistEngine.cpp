
#include <iostream>
#include <vector>
#include <cstddef> // For offsetof

#include "Renderer.h"
#include "Scene.h"
#include "Model.h"
#include "Orb.h"
#include "PhysicsSystem.h"
#include "Mesh.h" // Include Mesh class
#include "Texture.h" // Include Texture class
#include "ShapeGenerator.h" // Include ShapeGenerator
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

    // Create Scene
    Scene scene;

    // Create glowing orb
    Orb* glowingOrb = new Orb(glm::vec3(1.5f, 1.0f, 0.0f), 0.3f, glm::vec3(2.0f, 1.6f, 0.4f));
    scene.AddOrb(glowingOrb);

    // Load 3D model (non-physics for now)
    Model* ourModel = new Model("models/backpack/backpack.obj");
    scene.AddRenderable(ourModel);


    // Initialize Physics
    PhysicsSystem physicsSystem;

    // Create physics ground plane
    btRigidBody* groundBody = physicsSystem.CreateGroundPlane(glm::vec3(0.0f, -0.5f, 0.0f));

    // Create Mesh for the ground plane using ShapeGenerator
    std::vector<Vertex> planeVertices;
    std::vector<unsigned int> planeIndices;
    generatePlaneMesh(planeVertices, planeIndices);
    // Load ground texture (assuming you have one)
    Texture groundTexture;
    // if (!groundTexture.LoadFromFile("textures/ground.jpg")) {
    //     std::cerr << "Failed to load ground texture" << std::endl;
    //     return -1;
    // }
    std::vector<Texture> groundTextures;
    // groundTextures.push_back(groundTexture); // Add if you have a ground texture

    Mesh* groundMesh = new Mesh(planeVertices, planeIndices, groundTextures);
    scene.AddPhysicsRenderable(groundBody, groundMesh);


    // Create physics cube
    btRigidBody* cubeBody = physicsSystem.CreateCube(glm::vec3(0.0f, 0.5f, 0.0f), 1.0f);

    // Create Mesh for the cube using ShapeGenerator
    std::vector<Vertex> cubeVertices;
    std::vector<unsigned int> cubeIndices;
    generateCubeMesh(cubeVertices, cubeIndices);
    // Load cube texture
    Texture cubeTexture;
    if (!cubeTexture.LoadFromFile("textures/container.jpg")) {
        std::cerr << "Failed to load cube texture" << std::endl;
        return -1;
    }
    std::vector<Texture> cubeTextures;
    cubeTextures.push_back(cubeTexture); // Assuming your Mesh class handles textures this way

    Mesh* cubeMesh = new Mesh(cubeVertices, cubeIndices, cubeTextures);
    scene.AddPhysicsRenderable(cubeBody, cubeMesh);


    // Main loop
    while (!glfwWindowShouldClose(renderer.GetWindow())) {
        // Calculate delta time is now in Renderer::Render()

        // Input
        renderer.ProcessInputWithPhysics(renderer.GetWindow(), physicsSystem, scene.getPhysicsRenderables());

        // Physics Update
        physicsSystem.Update(renderer.GetDeltaTime());

        // Model matrices are updated in Renderer::Render now

        // Render
        renderer.Render(scene);
    }

    // Cleanup is handled by Renderer and Scene destructors

    return 0;
}


