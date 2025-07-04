
#include <iostream>

#include "Renderer.h"
#include "Scene.h"
#include "PhysicsSystem.h"
#include "ECSManager.h"
#include "Components.h"

#include <glm/gtc/type_ptr.hpp> // For glm::make_mat4

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
      planeVAO(0), planeVBO(0), cubeVAO(0), cubeVBO(0), cubeEBO(0) // Still needed for cleanup, but setup is removed
{
    g_renderer = this; // Set the global pointer
}

Renderer::~Renderer() {
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

    // Removed: Setup basic shapes (plane and cube) - VAOs, VBOs, EBOs


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

    // Render entities with PositionComponent and RenderComponent to depth map
    auto renderableEntities = scene.ecsManager.getEntitiesWith<PositionComponent, RenderComponent>();
    for (Entity entity : renderableEntities) {
        PositionComponent* posComp = scene.ecsManager.getComponent<PositionComponent>(entity);
        RenderComponent* renderComp = scene.ecsManager.getComponent<RenderComponent>(entity);

        if (posComp && renderComp && renderComp->mesh) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, posComp->position);
            model *= glm::mat4_cast(posComp->rotation);
            model = glm::scale(model, posComp->scale);

            depthShader.setMat4("model", model);
            renderComp->mesh->Draw(depthShader);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Reset viewport
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw glowing orbs (assuming Orbs are entities with a specific OrbComponent and RenderComponent)
    // We might need a specific OrbComponent or tag for this later. For now, we can iterate through all renderable entities
    // and check if they are the orb entity, or add a specific OrbComponent and filter by it.
    // A better ECS approach would be a dedicated OrbGlowSystem that renders entities with OrbComponent and RenderComponent.
    // For this step, let's focus on rendering the standard renderable entities.

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();

    // Render scene with shadows (entities with PositionComponent and RenderComponent)
    objectShader.use();
    objectShader.setMat4("projection", projection);
    objectShader.setMat4("view", view);
    objectShader.setVec3("lightDir", lightDir);
    objectShader.setVec3("lightColor", lightColor);
    objectShader.setVec3("viewPos", camera.Position);
    objectShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    objectShader.setInt("shadowMap", 0);

    for (Entity entity : renderableEntities) {
        PositionComponent* posComp = scene.ecsManager.getComponent<PositionComponent>(entity);
        RenderComponent* renderComp = scene.ecsManager.getComponent<RenderComponent>(entity);

        if (posComp && renderComp && renderComp->mesh) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, posComp->position);
            model *= glm::mat4_cast(posComp->rotation);
            model = glm::scale(model, posComp->scale);

            objectShader.setMat4("model", model);
            renderComp->mesh->Draw(objectShader);
        }
    }


    // Render glowing orbs (assuming they have a RenderComponent and possibly a tag or specific component)
    // A more ECS-aligned way would be a dedicated rendering system for orbs.
    // For now, we can iterate through entities that might be orbs and draw them with the glow shader.
    // This part needs to be adapted based on how you identify Orb entities in the ECS.
    // For demonstration, let's assume Orb entities have a specific tag or component we can check.
    // Since we don't have a specific OrbComponent yet, I will skip this part for now.
    // When you add an OrbComponent, we can update this to get entities with OrbComponent and RenderComponent
    // and draw them using the glowShader.


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

void Renderer::ProcessInputWithPhysics(GLFWwindow* window, PhysicsSystem& physicsSystem, Scene& scene) {
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

    // Physics controls (applying force to the cube entity)
    // We need to find the cube entity based on its components.
    // A better way would be to store the cube entity ID when it's created in MistEngine.cpp
    // and pass it here, or use a tag component to identify it.
    // For now, let's iterate through entities with PhysicsComponent and PositionComponent
    // and assume the first one we find is the cube (this is not robust).

    // In a proper ECS, you would likely have a dedicated InputSystem that interacts with other systems.
    // For now, we will find the cube entity here and apply force using the PhysicsSystem.

    Entity cubeEntity = 0; // Initialize with an invalid entity ID
    auto physicsEntities = scene.ecsManager.getEntitiesWith<PhysicsComponent, PositionComponent>();
    for (Entity entity : physicsEntities) {
        // We need a way to identify the cube entity uniquely.
        // For now, let's assume the first entity with both components is the cube. THIS IS NOT ROBUST.
        cubeEntity = entity;
        break; // Found a potential cube entity, break for now
    }


    if (cubeEntity != 0) { // Check if a potential cube entity was found
         PhysicsComponent* physicsComp = scene.ecsManager.getComponent<PhysicsComponent>(cubeEntity);

        if (physicsComp && physicsComp->rigidBody) {
            float force = 100.0f;

            if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
                physicsSystem.ApplyForce(physicsComp->rigidBody, glm::vec3(0.0f, 0.0f, -force));
            if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
                physicsSystem.ApplyForce(physicsComp->rigidBody, glm::vec3(0.0f, 0.0f, force));
            if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
                physicsSystem.ApplyForce(physicsComp->rigidBody, glm::vec3(-force, 0.0f, 0.0f));
            if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
                physicsSystem.ApplyForce(physicsComp->rigidBody, glm::vec3(force, 0.0f, 0.0f));
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
                 physicsSystem.ApplyForce(physicsComp->rigidBody, glm::vec3(0.0f, force * 2.0f, 0.0f));
        }
    }

    // The physicsRenderables parameter is no longer needed with ECS
    // std::vector<PhysicsRenderable>& physicsRenderables
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
