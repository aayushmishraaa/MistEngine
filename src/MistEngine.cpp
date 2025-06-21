#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "Texture.h"
#include "Shader.h"
#include "Camera.h"

#include "Scene.h"
#include "GameObject.h"
#include "PlaneGameObject.h"
#include "BackpackGameObject.h"
#include "RigidBody.h"
#include "Collision.h"
// Settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void RenderScene(Shader& shader);

// Physics objects
PhysicsSystem physicsSystem;

// Scene instance
Scene scene;

// Function prototypes
void processInput(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);


int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "MistEngine", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    // Set context and callbacks
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);

    // Load shaders
    Shader sceneShader("shaders/vertex.glsl", "shaders/fragment.glsl");
    Shader depthShader("shaders/depth_vertex.glsl", "shaders/depth_fragment.glsl");
    Shader glowShader("shaders/glow_vertex.glsl", "shaders/glow_fragment.glsl");
    // Orb glowingOrb(glm::vec3(1.5f, 1.0f, 0.0f), 0.3f, glm::vec3(2.0f, 1.6f, 0.4f)); // Assuming Orb is not needed now
    // --- Scene Setup ---

    // Create the plane GameObject
    // You'll still need to generate the plane's VAO/VBO somewhere.
    // For now, let's assume you generate the planeVAO before this.
    unsigned int planeVAO = 0; // Replace with your actual plane VAO generation
    // Plane vertices
    float planeVertices[] = {
        // Positions         // Normals         // Texture Coords
         5.0f, -0.5f,  5.0f, 0.0f, 1.0f, 0.0f, 2.0f, 0.0f,
        -5.0f, -0.5f,  5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        -5.0f, -0.5f, -5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 2.0f,

         5.0f, -0.5f,  5.0f, 0.0f, 1.0f, 0.0f, 2.0f, 0.0f,
        -5.0f, -0.5f, -5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 2.0f,
         5.0f, -0.5f, -5.0f, 0.0f, 1.0f, 0.0f, 2.0f, 2.0f
    };
    unsigned int planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    auto plane = std::make_unique<PlaneGameObject>(planeVAO);

    // Assign collision shape and add to physics world
    // Plane vertices
        // Positions         // Normals         // Texture Coords
         5.0f, -0.5f,  5.0f, 0.0f, 1.0f, 0.0f, 2.0f, 0.0f,
        -5.0f, -0.5f,  5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        -5.0f, -0.5f, -5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 2.0f,

         5.0f, -0.5f,  5.0f, 0.0f, 1.0f, 0.0f, 2.0f, 0.0f,
        -5.0f, -0.5f, -5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 2.0f,
         5.0f, -0.5f, -5.0f, 0.0f, 1.0f, 0.0f, 2.0f, 2.0f
    };

    // Create RigidBody for the plane
    RigidBody* planeRigidBody = new RigidBody();
    // Set initial position to match where vertices are defined
    // The GameObject's initial position should likely be (0,0,0) and the model matrix handles placement
    // For now, we set the rigid body position to match the visual representation
    planeRigidBody->properties.position = glm::vec3(0.0f, -0.5f, 0.0f);
    planeRigidBody->properties.mass = 0.0f; // Static

    // Create BoxCollisionShape for the plane
    BoxCollisionShape* planeCollisionShape = new BoxCollisionShape(glm::vec3(5.0f, 0.0f, 5.0f)); // Half-extents
    plane->physicsObject = planeRigidBody;
    planeRigidBody->collisionShape = planeCollisionShape;
    scene.getPhysicsWorld()->addObject(planeRigidBody);

    // Add plane to the scene
    scene.addGameObject(std::move(plane));


    // Create the backpack GameObject
    auto backpack = std::make_unique<BackpackGameObject>("models/backpack/backpack.obj");

    // Create RigidBody for the backpack model
    RigidBody* backpackRigidBody = new RigidBody();
    backpackRigidBody->properties.position = glm::vec3(0.0f, 5.0f, 0.0f); // Match BackpackGameObject's initial position
    // Set a small mass to make it dynamic and affected by gravity
    backpackRigidBody->properties.mass = 1.0f;
    backpackRigidBody->properties.useGravity = true; // Ensure gravity is enabled

    // Create SphereCollisionShape for the backpack model (adjust radius as needed)
    SphereCollisionShape* backpackCollisionShape = new SphereCollisionShape(1.0f); // Approximate radius

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    // Link physics object and add to physics world
    backpack->physicsObject = backpackRigidBody;
    backpackRigidBody->collisionShape = backpackCollisionShape;
    scene.getPhysicsWorld()->addObject(backpackRigidBody);

    // Add backpack to the scene
    scene.addGameObject(std::move(backpack));


    // --- End Scene Setup ---

    // You might want to set up your shadow mapping FBO here if you still need it

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        // This deltaTime should be used for physics and anything frame-rate dependent
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        processInput(window); // Handle camera and basic input

        // Update the scene (including physics)
        scene.update(deltaTime);

        // --- Rendering ---

        // 1. Render to depth map
        glViewport(0, 0, 1024, 1024); // Assuming SHADOW_WIDTH and SHADOW_HEIGHT are 1024
        glBindFramebuffer(GL_FRAMEBUFFER, 1); // Assuming depthMapFBO is 1
        glClear(GL_DEPTH_BUFFER_BIT);

        depthShader.use(); // Assuming depthShader is loaded and accessible
        // TODO: Set light space matrix uniform in depthShader here (before rendering objects)

        for (const auto& obj : scene.getGameObjects()) {
            obj->render(depthShader); // Render using the depth shader
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Bind default framebuffer


        // 2. Render the scene as normal
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT); // Assuming SCR_WIDTH and SCR_HEIGHT are defined
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sceneShader.use(); // Assuming sceneShader is loaded and accessible
        // TODO: Set view and projection matrix uniforms in sceneShader here (before rendering objects)
        // TODO: Set light properties and bind depth map texture in sceneShader here

        for (const auto& obj : scene.getGameObjects()) {
            obj->render(sceneShader); // Render using the main scene shader
        }

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    // The Scene destructor will handle deleting GameObjects, which in turn manage VAOs/VBOs (ideally)
    // PhysicsObjects are owned by the PhysicsWorld which is owned by the Scene
    
    glfwTerminate();
    return 0;
}

// This RenderScene is likely for depth map rendering only now
void RenderScene(Shader& shader) {
    // Render plane
    glm::mat4 model = glm::mat4(1.0f);
    shader.setMat4("model", model);
    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Render cube
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f));
    model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.5f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}