#include <iostream>
#include "Renderer.h"
#include "Scene.h"
#include "PhysicsSystem.h"
#include <glm/gtc/type_ptr.hpp> // For glm::make_mat4 (used in updateModelMatrixFromPhysics)

#include "Orb.h" // Add this include to ensure the Orb class is defined
#include "ECS/Coordinator.h"

extern Coordinator gCoordinator;

// Global pointer to the renderer instance for callbacks
Renderer* g_renderer = nullptr;

Renderer::Renderer(unsigned int width, unsigned int height)
    : screenWidth(width), screenHeight(height),
      camera(glm::vec3(0.0f, 0.0f, 3.0f)),
      lastX(width / 2.0f), lastY(height / 2.0f), firstMouse(true),
      deltaTime(0.0f), lastFrame(0.0f),
      lightDir(-0.2f, -1.0f, -0.3f), lightColor(1.0f, 1.0f, 1.0f),
      shadowWidth(1024), shadowHeight(1024),
      planeVAO(0), planeVBO(0), cubeVAO(0), cubeVBO(0), cubeEBO(0),
      m_editorUI(std::make_unique<EditorUI>())
{
    g_renderer = this; // Set the global pointer
}

Renderer::~Renderer() {
    // Cleanup (VAOs, VBOs, EBOs for basic shapes are now managed by Mesh objects)
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
    window = glfwCreateWindow(screenWidth, screenHeight, "MistEngine", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    // Set context and callbacks
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Changed from DISABLED for UI

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }

    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);

    // Load shaders
    objectShader = Shader("shaders/vertex.glsl", "shaders/fragment.glsl");
    depthShader = Shader("shaders/depth_vertex.glsl", "shaders/depth_fragment.glsl");
    glowShader = Shader("shaders/glow_vertex.glsl", "shaders/glow_fragment.glsl");

    // Setup shadow mapping
    setupShadowMap();

    // Initialize Editor UI
    if (!m_editorUI->Init(window)) {
        std::cerr << "Failed to initialize Editor UI\n";
        return false;
    }

    // Set up UI references
    m_editorUI->SetCoordinator(&gCoordinator);
    m_editorUI->SetCamera(&camera);

    return true;
}

void Renderer::Render(Scene& scene) {
    // Calculate delta time
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Update UI
    m_editorUI->Update(deltaTime);

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

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Reset viewport
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
         object->Draw(objectShader);
    }

    // Render UI
    m_editorUI->Render();

    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void Renderer::ProcessInput(GLFWwindow* window) {
    // Only process camera input if viewport is focused
    if (!m_editorUI->IsViewportFocused()) {
        return;
    }

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
    // Only process camera input if viewport is focused
    if (!m_editorUI->IsViewportFocused()) {
        return;
    }

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

// --- Callback implementations ---
void Renderer::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    if (g_renderer) {
        glViewport(0, 0, width, height);
        g_renderer->screenWidth = width;
        g_renderer->screenHeight = height;
    }
}

void Renderer::mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (g_renderer) {
        // Only process mouse input if viewport is focused and hovered
        if (!g_renderer->m_editorUI->IsViewportFocused() || !g_renderer->m_editorUI->IsViewportHovered()) {
            g_renderer->firstMouse = true;
            return;
        }

        if (g_renderer->firstMouse) {
            g_renderer->lastX = xpos;
            g_renderer->lastY = ypos;
            g_renderer->firstMouse = false;
        }

        float xoffset = xpos - g_renderer->lastX;
        float yoffset = g_renderer->lastY - ypos; // reversed since y-coordinates go from bottom to top
        g_renderer->lastX = xpos;
        g_renderer->lastY = ypos;

        g_renderer->camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

void Renderer::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (g_renderer) {
        // Only process scroll input if viewport is hovered
        if (!g_renderer->m_editorUI->IsViewportHovered()) {
            return;
        }
        g_renderer->camera.ProcessMouseScroll(yoffset);
    }
}

void Renderer::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        // Toggle cursor mode with Tab key
        if (key == GLFW_KEY_TAB) {
            static bool cursorEnabled = true;
            cursorEnabled = !cursorEnabled;
            glfwSetInputMode(window, GLFW_CURSOR, cursorEnabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        }
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

    // Update UI
    m_editorUI->Update(deltaTime);
    m_editorUI->SetScene(&scene);

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

    // Render UI
    m_editorUI->Render();

    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
}
