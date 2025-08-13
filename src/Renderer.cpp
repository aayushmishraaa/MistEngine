#include <iostream>
#include "Renderer.h"
#include "Scene.h"
#include "PhysicsSystem.h"
#include "UIManager.h"
#include "Version.h"  // Add Version.h include
#include <glm/gtc/type_ptr.hpp> // For glm::make_mat4 (used in updateModelMatrixFromPhysics)

#include "Orb.h" // Add this include to ensure the Orb class is defined



// Global pointer to the renderer instance for callbacks
Renderer* g_renderer = nullptr;

Renderer::Renderer(unsigned int width, unsigned int height)
    : screenWidth(width), screenHeight(height),
      camera(glm::vec3(0.0f, 0.0f, 3.0f)),
      lastX(width / 2.0f), lastY(height / 2.0f), firstMouse(true),
      deltaTime(0.0f), lastFrame(0.0f),
      lightDir(-0.2f, -1.0f, -0.3f), lightColor(1.0f, 1.0f, 1.0f),
      shadowWidth(1024), shadowHeight(1024),
      planeVAO(0), planeVBO(0), cubeVAO(0), cubeVBO(0), cubeEBO(0), // Still needed for cleanup, but setup is removed
      skyboxVAO(0), skyboxVBO(0)
{
    g_renderer = this; // Set the global pointer
}

Renderer::~Renderer() {
    // Cleanup skybox
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);

    // Cleanup (VAOs, VBOs, EBOs for basic shapes are now managed by Mesh objects)
    // glDeleteVertexArrays(1, &cubeVAO); // Removed
    // glDeleteVertexArrays(1, &planeVAO); // Removed
    // glDeleteBuffers(1, &cubeVBO); // Removed
    // glDeleteBuffers(1, &cubeEBO); // Removed
    // glDeleteBuffers(1, &planeVBO); // Removed

    glDeleteFramebuffers(1, &depthMapFBO);
    glDeleteTextures(1, &depthMap);

    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Renderer::Init() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    std::string windowTitle = std::string(MIST_ENGINE_NAME) + " " + MIST_ENGINE_VERSION_STRING;
    window = glfwCreateWindow(screenWidth, screenHeight, windowTitle.c_str(), NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    // Set context and callbacks
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    // REMOVED: Legacy mouse callback that was causing unwanted camera movement
    // glfwSetCursorPosCallback(window, mouse_callback);
    // InputManager now handles all mouse input via polling
    
    glfwSetScrollCallback(window, scroll_callback);
    
    // REMOVED: Don't set cursor to disabled by default
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // InputManager now controls cursor mode

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }

    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);

    // Set a nice sky-like clear color instead of black
    glClearColor(0.5f, 0.7f, 1.0f, 1.0f);

    // Load shaders
    objectShader = Shader("shaders/vertex.glsl", "shaders/fragment.glsl");
    depthShader = Shader("shaders/depth_vertex.glsl", "shaders/depth_fragment.glsl");
    glowShader = Shader("shaders/glow_vertex.glsl", "shaders/glow_fragment.glsl");
    skyboxShader = Shader("shaders/skybox_vertex.glsl", "shaders/skybox_fragment.glsl");

    // Setup shadow mapping and skybox
    setupShadowMap();
    setupSkybox();

    std::cout << "Renderer: Legacy mouse callback DISABLED - InputManager has full control" << std::endl;

    return true;
}

void Renderer::Render(Scene& scene) {
    // Calculate delta time
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Render to depth map
    glViewport(0, 0, shadowWidth, shadowHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    float near_plane = 1.0f, far_plane = 7.5f;
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
    glm::mat4 lightView = glm::lookAt(-lightDir * 5.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    depthShader.use();
    depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

    // Render physics objects to depth map
    for (auto& obj : scene.getPhysicsRenderables()) {
         // Update model matrix from physics body
        updateModelMatrixFromPhysics(obj.body, obj.modelMatrix);
        depthShader.setMat4("model", obj.modelMatrix);
        if (obj.renderable) {
            obj.renderable->Draw(depthShader);
        }
    }

    // Render non-physics renderable objects to depth map (if they cast shadows)
     for (Renderable* object : scene.getRenderables()) {
         // For non-physics objects, you would set their model matrix here
         // based on their position/orientation in the scene (not from physics)
         // Assuming the Model class handles its own model matrix internally or it's static
         // If you want physics on the model, it should be in the physicsObjects loop
         // object->Draw(depthShader); // Uncomment if non-physics objects cast shadows
     }


    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Reset viewport
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render skybox first (before other objects)
    renderSkybox();

    // Draw glowing orbs first (for proper blending if enabled)
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();

    glowShader.use();
    glowShader.setMat4("projection", projection);
    glowShader.setMat4("view", view);
    for (Orb* orb : scene.getOrbs()) {
        orb->Draw(glowShader);
    }

    // Render scene with shadows
    objectShader.use();
    objectShader.setMat4("projection", projection);
    objectShader.setMat4("view", view);
    objectShader.setVec3("lightDir", lightDir);
    objectShader.setVec3("lightColor", lightColor);
    objectShader.setVec3("viewPos", camera.Position);
    objectShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

    // Bind depth map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    objectShader.setInt("shadowMap", 0);

    // Render physics objects in color pass
    for (auto& obj : scene.getPhysicsRenderables()) {
        // The model matrix is already updated from physics at the beginning of Render()
        objectShader.setMat4("model", obj.modelMatrix);
         if (obj.renderable) {
            obj.renderable->Draw(objectShader);
         }
    }


    // Render non-physics renderable objects (like the backpack model)
    for (Renderable* object : scene.getRenderables()) {
         // For non-physics objects, you would set their model matrix here
         // based on their position/orientation in the scene (not from physics)
         // Assuming the Model class handles its own model matrix internally or it's static
         // If you want physics on the model, it should be in the physicsObjects loop
         object->Draw(objectShader);
    }


    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void Renderer::ProcessInput(GLFWwindow* window) {
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

void Renderer::ProcessInputWithPhysics(GLFWwindow* window, PhysicsSystem& physicsSystem, std::vector<PhysicsRenderable>& physicsRenderables) {
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


void Renderer::setupShadowMap() {
    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::setupSkybox() {
    // Skybox vertices (cube centered at origin)
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void Renderer::renderSkybox() {
    // Change depth function to draw skybox behind everything else
    glDepthFunc(GL_LEQUAL);
    
    skyboxShader.use();
    glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // Remove translation
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
    
    skyboxShader.setMat4("view", view);
    skyboxShader.setMat4("projection", projection);
    
    // Render skybox cube
    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    
    // Reset depth function
    glDepthFunc(GL_LESS);
}



// --- Callback implementations ---
void Renderer::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    if (g_renderer) {
        glViewport(0, 0, width, height);
        g_renderer->screenWidth = width;
        g_renderer->screenHeight = height;
    }
}

void Renderer::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (g_renderer) {
        g_renderer->camera.ProcessMouseScroll(yoffset);
    }
}

// --- Getter implementations ---
float Renderer::GetDeltaTime() const {
    return deltaTime;
}
void updateModelMatrixFromPhysics(btRigidBody* body, glm::mat4& modelMatrix) {
    if (body) {
        // Get the world transform from the physics body
        const btTransform& transform = body->getCenterOfMassTransform();

        // Convert Bullet's btTransform to glm::mat4
        float matrix[16];
        transform.getOpenGLMatrix(matrix);
        modelMatrix = glm::make_mat4(matrix);
    }
}
void Renderer::RenderWithECS(Scene& scene, std::shared_ptr<RenderSystem> renderSystem) {
    // Calculate delta time
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // === SHADOW MAPPING PASS ===
    glViewport(0, 0, shadowWidth, shadowHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    float near_plane = 1.0f, far_plane = 7.5f;
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
    glm::mat4 lightView = glm::lookAt(-lightDir * 5.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    depthShader.use();
    depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

    // Render ECS entities to depth map
    renderSystem->Update(depthShader);

    // Render legacy physics objects to depth map
    for (auto& obj : scene.getPhysicsRenderables()) {
        updateModelMatrixFromPhysics(obj.body, obj.modelMatrix);
        depthShader.setMat4("model", obj.modelMatrix);
        if (obj.renderable) {
            obj.renderable->Draw(depthShader);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // === MAIN RENDERING PASS ===
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();

    // Render skybox first (before other objects)
    renderSkybox();

    // Draw glowing orbs first
    glowShader.use();
    glowShader.setMat4("projection", projection);
    glowShader.setMat4("view", view);
    for (Orb* orb : scene.getOrbs()) {
        orb->Draw(glowShader);
    }

    // Setup main shader for scene rendering
    objectShader.use();
    objectShader.setMat4("projection", projection);
    objectShader.setMat4("view", view);
    objectShader.setVec3("lightDir", lightDir);
    objectShader.setVec3("lightColor", lightColor);
    objectShader.setVec3("viewPos", camera.Position);
    objectShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

    // Bind shadow map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    objectShader.setInt("shadowMap", 0);

    // Render ECS entities
    renderSystem->Update(objectShader);

    // Render legacy scene objects for backward compatibility
    for (auto& obj : scene.getPhysicsRenderables()) {
        objectShader.setMat4("model", obj.modelMatrix);
        if (obj.renderable) {
            obj.renderable->Draw(objectShader);
        }
    }

    for (Renderable* object : scene.getRenderables()) {
        object->Draw(objectShader);
    }

    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
}
void Renderer::RenderWithECSAndUI(Scene& scene, std::shared_ptr<RenderSystem> renderSystem, UIManager* uiManager) {
    // Calculate delta time
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // === SHADOW MAPPING PASS ===
    glViewport(0, 0, shadowWidth, shadowHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    float near_plane = 1.0f, far_plane = 7.5f;
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
    glm::mat4 lightView = glm::lookAt(-lightDir * 5.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    depthShader.use();
    depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

    // Render ECS entities to depth map
    renderSystem->Update(depthShader);

    // Render legacy physics objects to depth map
    for (auto& obj : scene.getPhysicsRenderables()) {
        updateModelMatrixFromPhysics(obj.body, obj.modelMatrix);
        depthShader.setMat4("model", obj.modelMatrix);
        if (obj.renderable) {
            obj.renderable->Draw(depthShader);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // === MAIN RENDERING PASS ===
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();

    // Render skybox first (before other objects)
    renderSkybox();

    // Draw glowing orbs first
    glowShader.use();
    glowShader.setMat4("projection", projection);
    glowShader.setMat4("view", view);
    for (Orb* orb : scene.getOrbs()) {
        orb->Draw(glowShader);
    }

    // Setup main shader for scene rendering
    objectShader.use();
    objectShader.setMat4("projection", projection);
    objectShader.setMat4("view", view);
    objectShader.setVec3("lightDir", lightDir);
    objectShader.setVec3("lightColor", lightColor);
    objectShader.setVec3("viewPos", camera.Position);
    objectShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

    // Bind shadow map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    objectShader.setInt("shadowMap", 0);

    // Render ECS entities
    renderSystem->Update(objectShader);

    // Render legacy scene objects for backward compatibility
    for (auto& obj : scene.getPhysicsRenderables()) {
        objectShader.setMat4("model", obj.modelMatrix);
        if (obj.renderable) {
            obj.renderable->Draw(objectShader);
        }
    }

    for (Renderable* object : scene.getRenderables()) {
        object->Draw(objectShader);
    }

    // === UI RENDERING ===
    if (uiManager) {
        uiManager->NewFrame();
        uiManager->Render();
    }

    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
}

// These getters are no longer needed as basic shape VAOs/EBOs are managed by Mesh objects
/*
unsigned int Renderer::GetCubeVAO() const {
    return cubeVAO;
}

unsigned int Renderer::GetCubeEBO() const {
    return cubeEBO;
}

unsigned int Renderer::GetPlaneVAO() const {
    return planeVAO;
}
*/
