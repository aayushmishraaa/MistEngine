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

// Create a procedural "backpack" model using cubes to verify rendering
void CreateProceduralBackpackMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    vertices.clear();
    indices.clear();
    
    // Create a simple backpack-like shape using multiple cube parts
    // Main body (larger cube)
    float bodySize = 0.8f;
    std::vector<glm::vec3> bodyPositions = {
        // Front face
        glm::vec3(-bodySize, -bodySize, bodySize),
        glm::vec3(bodySize, -bodySize, bodySize),
        glm::vec3(bodySize, bodySize, bodySize),
        glm::vec3(-bodySize, bodySize, bodySize),
        
        // Back face
        glm::vec3(-bodySize, -bodySize, -bodySize),
        glm::vec3(bodySize, -bodySize, -bodySize),
        glm::vec3(bodySize, bodySize, -bodySize),
        glm::vec3(-bodySize, bodySize, -bodySize)
    };
    
    // Add vertices for main body
    for (const auto& pos : bodyPositions) {
        Vertex vertex;
        vertex.Position = pos;
        vertex.Normal = glm::normalize(pos); // Simple normal approximation
        vertex.TexCoords = glm::vec2(0.5f, 0.5f); // Center UV
        vertices.push_back(vertex);
    }
    
    // Add indices for cube faces (12 triangles, 36 indices)
    std::vector<unsigned int> cubeIndices = {
        // Front face
        0, 1, 2, 2, 3, 0,
        // Back face  
        4, 6, 5, 6, 4, 7,
        // Left face
        4, 0, 3, 3, 7, 4,
        // Right face
        1, 5, 6, 6, 2, 1,
        // Top face
        3, 2, 6, 6, 7, 3,
        // Bottom face
        4, 1, 0, 1, 4, 5
    };
    
    indices.insert(indices.end(), cubeIndices.begin(), cubeIndices.end());
    
    // Add straps (smaller cubes)
    unsigned int baseIndex = vertices.size();
    float strapSize = 0.1f;
    
    // Left strap
    std::vector<glm::vec3> leftStrapPos = {
        glm::vec3(-bodySize - 0.2f, bodySize - 0.1f, 0.0f),
        glm::vec3(-bodySize - 0.1f, bodySize - 0.1f, 0.0f),
        glm::vec3(-bodySize - 0.1f, bodySize, 0.0f),
        glm::vec3(-bodySize - 0.2f, bodySize, 0.0f)
    };
    
    // Add vertices for left strap (simplified as quad)
    for (const auto& pos : leftStrapPos) {
        Vertex vertex;
        vertex.Position = pos;
        vertex.Normal = glm::vec3(0.0f, 0.0f, 1.0f);
        vertex.TexCoords = glm::vec2(0.8f, 0.8f); // Different UV for straps
        vertices.push_back(vertex);
    }
    
    // Add indices for left strap (2 triangles)
    std::vector<unsigned int> strapIndices = {
        baseIndex + 0, baseIndex + 1, baseIndex + 2,
        baseIndex + 2, baseIndex + 3, baseIndex + 0
    };
    indices.insert(indices.end(), strapIndices.begin(), strapIndices.end());
    
    std::cout << "Created procedural backpack with " << vertices.size() << " vertices and " << indices.size() << " indices" << std::endl;
}

int main() {
    // Settings
    const unsigned int SCR_WIDTH = 1200;
    const unsigned int SCR_HEIGHT = 800;

    std::cout << "=== MistEngine v0.3.0 Starting Up ===" << std::endl;
    std::cout << "Scene Editor Mode: Clean Unity-like startup" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  - WASD/QE: Camera movement (always available)" << std::endl;
    std::cout << "  - RIGHT-CLICK + HOLD: Enable mouse look (locked by default)" << std::endl;
    std::cout << "  - Mouse Scroll: Zoom in/out" << std::endl;
    std::cout << "  - F3: Toggle Scene Editor / Gameplay mode" << std::endl;
    std::cout << "  - F1: Toggle ImGui Demo" << std::endl;
    std::cout << "  - F2: Toggle AI Assistant" << std::endl;
    std::cout << "  - F4: Create procedural backpack model (test rendering)" << std::endl;
    std::cout << "  - GameObject Menu: Create cubes, spheres, planes, models" << std::endl;

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
    
    // Create Renderer with Unity-like settings
    Renderer renderer(SCR_WIDTH, SCR_HEIGHT);
    if (!renderer.Init()) {
        return -1;
    }
    
    // Set Unity-like camera position and skybox-like background
    renderer.SetBackgroundColor(0.2f, 0.3f, 0.3f, 1.0f);  // Unity-like gray background
    renderer.SetCameraPosition(glm::vec3(0.0f, 1.0f, 5.0f));  // Better starting position

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
    inputManager.EnableSceneEditorMode(true);

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

    // Set up UI references
    uiManager.SetCoordinator(&gCoordinator);

    // Create Scene (clean - no default objects)
    Scene scene;
    uiManager.SetScene(&scene);
    moduleManager.SetScene(&scene);

    // Initialize Physics System
    PhysicsSystem physicsSystem;
    uiManager.SetPhysicsSystem(&physicsSystem);

    // Create Unity-like default directional light (as ECS entity)
    Entity lightEntity = gCoordinator.CreateEntity();
    TransformComponent lightTransform;
    lightTransform.position = glm::vec3(0.0f, 5.0f, 0.0f);
    lightTransform.rotation = glm::vec3(50.0f, 30.0f, 0.0f);  // Typical Unity directional light angle
    lightTransform.scale = glm::vec3(1.0f);
    gCoordinator.AddComponent(lightEntity, lightTransform);
    
    // Set up Unity-like lighting in renderer
    renderer.SetDirectionalLight(glm::vec3(-0.2f, -1.0f, -0.3f), glm::vec3(1.0f, 1.0f, 1.0f));

    // Instead of using legacy Scene system, let's test with ECS entities
    // Create a test cube ECS entity
    try {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures; // Empty textures for basic cube
        
        generateCubeMesh(vertices, indices);
        
        Mesh* testCube = new Mesh(vertices, indices, textures);
        
        // Create ECS entity for the test cube
        Entity testCubeEntity = gCoordinator.CreateEntity();
        
        TransformComponent transform;
        transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
        transform.rotation = glm::vec3(0.0f, 45.0f, 0.0f); // Rotate 45 degrees for visibility
        transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);
        gCoordinator.AddComponent(testCubeEntity, transform);
        
        RenderComponent render;
        render.renderable = testCube;
        render.visible = true;
        gCoordinator.AddComponent(testCubeEntity, render);
        
        std::cout << "Test cube ECS entity created successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Warning: Could not create test cube ECS entity: " << e.what() << std::endl;
    }

    // Variables for test model loading (now as ECS entity)
    Entity testModelEntity = 0;
    bool testModelLoadRequested = false;

    std::cout << "=== Clean Scene Initialization Complete ===" << std::endl;
    std::cout << "Engine ready! Use GameObject menu to create objects." << std::endl;
    std::cout << "Right-click and drag to look around in Scene Editor mode." << std::endl;
    std::cout << "Press F4 to create a procedural backpack model for testing." << std::endl;

    // Main loop
    while (!glfwWindowShouldClose(renderer.GetWindow())) {
        float deltaTime = renderer.GetDeltaTime();

        // Handle UI toggle inputs (F1, F2, F4)
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

        // Test model loading with F4 (now creates procedural backpack)
        if (glfwGetKey(renderer.GetWindow(), GLFW_KEY_F4) == GLFW_PRESS) {
            static bool f4Pressed = false;
            if (!f4Pressed && !testModelLoadRequested) {
                testModelLoadRequested = true;
                f4Pressed = true;
                
                std::cout << "Creating procedural backpack model..." << std::endl;
                
                try {
                    std::vector<Vertex> backpackVertices;
                    std::vector<unsigned int> backpackIndices;
                    std::vector<Texture> backpackTextures; // Empty textures
                    
                    CreateProceduralBackpackMesh(backpackVertices, backpackIndices);
                    
                    Mesh* proceduralBackpack = new Mesh(backpackVertices, backpackIndices, backpackTextures);
                    
                    // Create ECS entity for the procedural backpack
                    testModelEntity = gCoordinator.CreateEntity();
                    
                    TransformComponent backpackTransform;
                    backpackTransform.position = glm::vec3(3.0f, 0.0f, 0.0f);
                    backpackTransform.rotation = glm::vec3(0.0f, 45.0f, 0.0f); // Rotate for better view
                    backpackTransform.scale = glm::vec3(1.0f, 1.0f, 1.0f);
                    gCoordinator.AddComponent(testModelEntity, backpackTransform);
                    
                    RenderComponent backpackRender;
                    backpackRender.renderable = proceduralBackpack;
                    backpackRender.visible = true;
                    gCoordinator.AddComponent(testModelEntity, backpackRender);
                    
                    std::cout << "Successfully created procedural backpack as ECS entity!" << std::endl;
                    
                } catch (const std::exception& e) {
                    std::cout << "Failed to create procedural backpack: " << e.what() << std::endl;
                    
                    // Fallback to simple cube
                    try {
                        std::vector<Vertex> vertices2;
                        std::vector<unsigned int> indices2;
                        std::vector<Texture> textures2;
                        
                        generateCubeMesh(vertices2, indices2);
                        Mesh* testCube2 = new Mesh(vertices2, indices2, textures2);
                        
                        testModelEntity = gCoordinator.CreateEntity();
                        
                        TransformComponent fallbackTransform;
                        fallbackTransform.position = glm::vec3(3.0f, 0.0f, 0.0f);
                        fallbackTransform.rotation = glm::vec3(0.0f, 0.0f, 45.0f); // Different rotation
                        fallbackTransform.scale = glm::vec3(0.5f, 1.5f, 0.5f);     // Different scale
                        gCoordinator.AddComponent(testModelEntity, fallbackTransform);
                        
                        RenderComponent fallbackRender;
                        fallbackRender.renderable = testCube2;
                        fallbackRender.visible = true;
                        gCoordinator.AddComponent(testModelEntity, fallbackRender);
                        
                        std::cout << "Created fallback test cube as ECS entity" << std::endl;
                    } catch (const std::exception& e2) {
                        std::cout << "Failed to create fallback cube: " << e2.what() << std::endl;
                    }
                }
            }
        } else {
            static bool f4Pressed = false;
            f4Pressed = false;
        }

        // Update systems
        inputManager.Update(deltaTime);
        moduleManager.UpdateModules(deltaTime);
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

