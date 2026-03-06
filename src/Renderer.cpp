#include <iostream>
#include "Renderer.h"
#include "Scene.h"
#include "PhysicsSystem.h"
#include "UIManager.h"
#include "Version.h"
#include "Core/Logger.h"
#include "Debug/DebugDraw.h"
#include <glm/gtc/type_ptr.hpp>

#include "Orb.h"

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
      skyboxVAO(0), skyboxVBO(0)
{
    g_renderer = this;
}

Renderer::~Renderer() {
    m_Profiler.Shutdown();
    DebugDraw::Shutdown();

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteFramebuffers(1, &depthMapFBO);
    glDeleteTextures(1, &depthMap);

    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Renderer::Init() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    std::string windowTitle = std::string(MIST_ENGINE_NAME) + " " + MIST_ENGINE_VERSION_STRING;
    window = glfwCreateWindow(screenWidth, screenHeight, windowTitle.c_str(), NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }

    LOG_INFO("OpenGL Version: ", (const char*)glGetString(GL_VERSION));
    LOG_INFO("GLSL Version: ", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
    LOG_INFO("Renderer: ", (const char*)glGetString(GL_RENDERER));

    if (!GLAD_GL_VERSION_4_3) {
        LOG_ERROR("OpenGL 4.3 minimum required but not available. Aborting.");
        return false;
    }
    if (!GLAD_GL_VERSION_4_6) {
        LOG_WARN("OpenGL 4.6 not available, some features may be limited.");
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Load legacy shaders (kept as fallback)
    objectShader = Shader("shaders/vertex.glsl", "shaders/fragment.glsl");
    depthShader = Shader("shaders/depth_vertex.glsl", "shaders/depth_fragment.glsl");
    glowShader = Shader("shaders/glow_vertex.glsl", "shaders/glow_fragment.glsl");
    skyboxShader = Shader("shaders/skybox_vertex.glsl", "shaders/skybox_fragment.glsl");

    // Load PBR shaders
    pbrShader = Shader("shaders/pbr_vertex.glsl", "shaders/pbr_fragment.glsl");
    skinnedPBRShader = Shader("shaders/skinned_pbr.vert", "shaders/pbr_fragment.glsl");

    // Legacy shadow map setup (fallback)
    setupShadowMap();
    setupSkybox();

    // Initialize new subsystems
    m_UBOManager.Init();

    m_ShadowSystem.Init();
    LOG_INFO("Cascaded shadow maps initialized (4 cascades, 2048x2048)");

    m_LightManager.Init();
    // Add default directional light
    Light dirLight;
    dirLight.position = glm::vec4(lightDir, 0.0f); // w=0 = directional
    dirLight.color = glm::vec4(lightColor, 3.0f);
    dirLight.direction = glm::vec4(glm::normalize(lightDir), 0.0f);
    m_LightManager.AddLight(dirLight);

    m_PostProcess.Init(screenWidth, screenHeight);

    m_Skybox.Init();

    m_Particles.Init();

    DebugDraw::Init();
    m_Profiler.Init();

    LOG_INFO("Renderer initialized: all subsystems ready");
    LOG_INFO("  PBR: enabled, HDR pipeline: enabled, Post-processing: bloom/SSAO/FXAA");

    return true;
}

void Renderer::RenderWithECSAndUI(Scene& scene, std::shared_ptr<RenderSystem> renderSystem, UIManager* uiManager) {
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    m_Profiler.BeginFrame();

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
        (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 viewProjection = projection * view;

    // Update UBOs
    PerFrameUBO perFrame;
    perFrame.view = view;
    perFrame.projection = projection;
    perFrame.viewPos = glm::vec4(camera.Position, 1.0f);
    perFrame.lightDir = glm::vec4(lightDir, 0.0f);
    perFrame.lightColor = glm::vec4(lightColor, 1.0f);
    perFrame.time = currentFrame;
    perFrame.deltaTime = deltaTime;
    perFrame.nearPlane = 0.1f;
    perFrame.farPlane = 100.0f;
    m_UBOManager.UpdatePerFrame(perFrame);

    // Update light manager
    m_LightManager.UploadToGPU();
    m_LightManager.CullLights(view, projection);

    // === SHADOW PASS ===
    m_Profiler.BeginCPUSection("Shadows");
    m_Profiler.BeginGPUSection("Shadows");

    // Cascaded shadow maps
    m_ShadowSystem.CalculateCascades(camera, glm::normalize(lightDir), 0.1f, 100.0f);

    Shader& csmDepthShader = depthShader; // Reuse depth shader for CSM
    for (int cascade = 0; cascade < ShadowSystem::NUM_CASCADES; cascade++) {
        m_ShadowSystem.BeginShadowPass(cascade);
        csmDepthShader.use();
        csmDepthShader.setMat4("lightSpaceMatrix", m_ShadowSystem.GetLightSpaceMatrix(cascade));

        // Render ECS entities to shadow map
        renderSystem->Update(csmDepthShader);

        // Render legacy physics objects to shadow map
        for (auto& obj : scene.getPhysicsRenderables()) {
            updateModelMatrixFromPhysics(obj.body, obj.modelMatrix);
            csmDepthShader.setMat4("model", obj.modelMatrix);
            if (obj.renderable) obj.renderable->Draw(csmDepthShader);
        }

        m_ShadowSystem.EndShadowPass();
    }

    m_Profiler.EndGPUSection("Shadows");
    m_Profiler.EndCPUSection("Shadows");

    // === HDR SCENE PASS (render to HDR framebuffer) ===
    m_PostProcess.BeginSceneCapture();

    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Skybox
    m_Profiler.BeginGPUSection("Skybox");
    m_Skybox.Render(view, projection);
    m_Profiler.EndGPUSection("Skybox");

    // Draw glowing orbs (legacy)
    glowShader.use();
    glowShader.setMat4("projection", projection);
    glowShader.setMat4("view", view);
    for (Orb* orb : scene.getOrbs()) {
        orb->Draw(glowShader);
    }

    // === PBR MAIN PASS ===
    m_Profiler.BeginCPUSection("Scene");
    m_Profiler.BeginGPUSection("Scene");

    Shader& mainShader = m_UsePBR ? pbrShader : objectShader;
    mainShader.use();
    mainShader.setMat4("projection", projection);
    mainShader.setMat4("view", view);
    mainShader.setVec3("viewPos", camera.Position);

    if (m_UsePBR) {
        // PBR lighting setup
        mainShader.setVec3("lightDir", glm::normalize(lightDir));
        mainShader.setVec3("lightColor", lightColor);

        // CSM shadow maps
        m_ShadowSystem.BindCascadeShadowMaps(mainShader, 0);

        // IBL textures (units 10-12)
        if (m_IBL.IsLoaded()) {
            m_IBL.Bind(mainShader, 10, 11, 12);
            mainShader.setBool("useIBL", true);
        } else {
            mainShader.setBool("useIBL", false);
        }

        // SSAO texture (will be available after post-process, but PBR reads it)
        mainShader.setBool("useSSAO", m_PostProcess.enableSSAO);

        // Bind light SSBOs (bindings 2-5 set by LightManager)
        m_LightManager.BindForRendering();
    } else {
        // Legacy Phong setup
        mainShader.setVec3("lightDir", lightDir);
        mainShader.setVec3("lightColor", lightColor);
        mainShader.setMat4("lightSpaceMatrix",
            m_ShadowSystem.GetLightSpaceMatrix(0));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        mainShader.setInt("shadowMap", 0);
    }

    // Render ECS entities
    renderSystem->Update(mainShader);
    m_Profiler.IncrementDrawCalls(1); // Approximate

    // Render legacy scene objects
    for (auto& obj : scene.getPhysicsRenderables()) {
        mainShader.setMat4("model", obj.modelMatrix);
        if (obj.renderable) {
            obj.renderable->Draw(mainShader);
            m_Profiler.IncrementDrawCalls();
        }
    }
    for (Renderable* object : scene.getRenderables()) {
        object->Draw(mainShader);
        m_Profiler.IncrementDrawCalls();
    }

    m_Profiler.EndGPUSection("Scene");
    m_Profiler.EndCPUSection("Scene");

    // === GPU PARTICLES ===
    m_Profiler.BeginGPUSection("Particles");
    m_Particles.Update(deltaTime, camera.Position);
    // Particle rendering (additive blending)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    m_Particles.Render(view, projection, m_PostProcess.GetDepthTexture());
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    m_Profiler.EndGPUSection("Particles");

    // === DEBUG DRAW ===
    DebugDraw::Flush(viewProjection);

    // === END HDR CAPTURE ===
    m_PostProcess.EndSceneCapture();

    // === POST-PROCESSING (tone map + bloom + SSAO + FXAA → default framebuffer) ===
    m_Profiler.BeginGPUSection("PostProcess");
    m_PostProcess.Execute(m_Exposure, projection, view);
    m_Profiler.EndGPUSection("PostProcess");

    // === UI RENDERING (after tone mapping, directly to screen) ===
    if (uiManager) {
        uiManager->NewFrame();
        uiManager->Render();
    }

    m_Profiler.EndFrame();

    glfwSwapBuffers(window);
    glfwPollEvents();
}

// === Legacy render methods (kept for backward compatibility) ===

void Renderer::Render(Scene& scene) {
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Shadow pass
    glViewport(0, 0, shadowWidth, shadowHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    float near_plane = 1.0f, far_plane = 7.5f;
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
    glm::mat4 lightView = glm::lookAt(-lightDir * 5.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    depthShader.use();
    depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

    for (auto& obj : scene.getPhysicsRenderables()) {
        updateModelMatrixFromPhysics(obj.body, obj.modelMatrix);
        depthShader.setMat4("model", obj.modelMatrix);
        if (obj.renderable) obj.renderable->Draw(depthShader);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderSkybox();

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();

    glowShader.use();
    glowShader.setMat4("projection", projection);
    glowShader.setMat4("view", view);
    for (Orb* orb : scene.getOrbs()) {
        orb->Draw(glowShader);
    }

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

    for (auto& obj : scene.getPhysicsRenderables()) {
        objectShader.setMat4("model", obj.modelMatrix);
        if (obj.renderable) obj.renderable->Draw(objectShader);
    }

    for (Renderable* object : scene.getRenderables()) {
        object->Draw(objectShader);
    }

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

    if (!physicsRenderables.empty()) {
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

void Renderer::RenderWithECS(Scene& scene, std::shared_ptr<RenderSystem> renderSystem) {
    // Delegate to the full render path without UI
    RenderWithECSAndUI(scene, renderSystem, nullptr);
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
    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
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
    glDepthFunc(GL_LEQUAL);
    skyboxShader.use();
    glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
    skyboxShader.setMat4("view", view);
    skyboxShader.setMat4("projection", projection);
    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}

// --- Callback implementations ---
void Renderer::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    if (g_renderer) {
        glViewport(0, 0, width, height);
        g_renderer->screenWidth = width;
        g_renderer->screenHeight = height;
        if (width > 0 && height > 0) {
            g_renderer->m_PostProcess.Resize(width, height);
        }
    }
}

void Renderer::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (g_renderer) {
        g_renderer->camera.ProcessMouseScroll(yoffset);
    }
}

float Renderer::GetDeltaTime() const {
    return deltaTime;
}

void updateModelMatrixFromPhysics(btRigidBody* body, glm::mat4& modelMatrix) {
    if (body) {
        const btTransform& transform = body->getCenterOfMassTransform();
        float matrix[16];
        transform.getOpenGLMatrix(matrix);
        modelMatrix = glm::make_mat4(matrix);
    }
}
