#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

#include "Shader.h"
#include "Camera.h"
#include "Renderable.h"
#include "PhysicsSystem.h"
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/ECSPhysicsSystem.h"

// New subsystems
#include "PostProcessStack.h"
#include "ShadowSystem.h"
#include "LightManager.h"
#include "IBL.h"
#include "SkyboxRenderer.h"
#include "ParticleSystem.h"
#include "UniformBufferObjects.h"
#include "Debug/Profiler.h"
#include "Renderer/Viewport.h"
#include "Renderer/GLRenderingDevice.h"

class Scene;
struct PhysicsRenderable;
class UIManager;

class Renderer {
public:
    Renderer(unsigned int width, unsigned int height);
    ~Renderer();

    bool Init();
    void Render(Scene& scene);
    void ProcessInput(GLFWwindow* window);
    void ProcessInputWithPhysics(GLFWwindow* window, PhysicsSystem& physicsSystem, std::vector<PhysicsRenderable>& physicsRenderables);
    void RenderWithECS(Scene& scene, std::shared_ptr<RenderSystem> renderSystem);
    void RenderWithECSAndUI(Scene& scene, std::shared_ptr<RenderSystem> renderSystem, UIManager* uiManager);

    Camera& GetCamera() { return camera; }
    GLFWwindow* GetWindow() const { return window; }
    float GetDeltaTime() const;

    // New subsystem accessors
    PostProcessStack& GetPostProcess() { return m_PostProcess; }
    ShadowSystem& GetShadowSystem() { return m_ShadowSystem; }
    LightManager& GetLightManager() { return m_LightManager; }
    IBL& GetIBL() { return m_IBL; }
    SkyboxRenderer& GetSkybox() { return m_Skybox; }
    GPUParticleSystem& GetParticles() { return m_Particles; }
    Profiler& GetProfiler() { return m_Profiler; }
    UBOManager& GetUBOManager() { return m_UBOManager; }

    float GetExposure() const { return m_Exposure; }
    void SetExposure(float e) { m_Exposure = e; }

    // Previous frame data for TAA motion vectors
    glm::mat4 GetPrevViewProjection() const { return m_PrevViewProjection; }

    // Viewport-as-first-class-concept (G2). The editor uses a single
    // primary viewport; this accessor exposes it so downstream code (UI,
    // plugins) can read dimensions and output texture without reaching into
    // Renderer internals.
    const Viewport& GetPrimaryViewport() const { return m_PrimaryViewport; }

    // When true, Renderer bypasses the ImGui-panel handoff and blits the
    // viewport's output texture directly to the default framebuffer. UI code
    // sets this via ToggleFullscreenPresent() when the Scene View panel is
    // hidden — without it, closing the panel shows only ImGui's background
    // (the "blue screen" bug).
    void SetFullscreenPresent(bool on) { m_PrimaryViewport.presentFullscreen = on; }
    bool GetFullscreenPresent() const  { return m_PrimaryViewport.presentFullscreen; }

private:
    unsigned int screenWidth;
    unsigned int screenHeight;
    GLFWwindow* window;

    // Shaders
    Shader objectShader;    // Legacy Phong (kept as fallback)
    Shader depthShader;     // Legacy depth (kept as fallback)
    Shader glowShader;
    Shader skyboxShader;    // Legacy skybox (kept as fallback)
    Shader pbrShader;       // PBR Cook-Torrance
    Shader skinnedPBRShader; // Skinned PBR for animated models

    // Camera
    Camera camera;
    float lastX;
    float lastY;
    bool firstMouse;

    // Timing
    float deltaTime;
    float lastFrame;

    // Lighting (legacy - kept for backward compat, migrating to LightManager)
    glm::vec3 lightDir;
    glm::vec3 lightColor;

    // Shadow mapping (legacy - kept as fallback)
    unsigned int shadowWidth;
    unsigned int shadowHeight;
    unsigned int depthMapFBO;
    unsigned int depthMap;

    // Skybox (legacy)
    unsigned int skyboxVAO;
    unsigned int skyboxVBO;

    unsigned int planeVAO, planeVBO;
    unsigned int cubeVAO, cubeVBO, cubeEBO;

    // Backend-agnostic GPU device. Held here so every migrated subsystem
    // (Framebuffer, Shader, Mesh, ShadowSystem) can route creation and
    // destruction through `Mist::GPU::Device()` without taking a pointer.
    Mist::GPU::GLRenderingDevice m_GpuDevice;

    // New subsystems
    PostProcessStack m_PostProcess;
    ShadowSystem m_ShadowSystem;
    LightManager m_LightManager;
    IBL m_IBL;
    SkyboxRenderer m_Skybox;
    GPUParticleSystem m_Particles;
    UBOManager m_UBOManager;
    Profiler m_Profiler;
    float m_Exposure = 1.0f;
    bool m_UsePBR = true;
    bool m_ShowEditorGrid = true;
    GLuint m_DummyTex2D = 0;
    GLuint m_DummyTexCube = 0;
    void CreateDummyTextures();

    // TAA previous frame state
    glm::mat4 m_PrevViewProjection = glm::mat4(1.0f);

    // G2 viewport descriptor. Kept in sync with screenWidth/screenHeight and
    // the post-process output texture inside RenderWithECSAndUI. A future
    // cycle will migrate per-viewport state (PostProcessStack, ShadowSystem,
    // camera) fully into this member.
    Viewport m_PrimaryViewport{};

    void setupShadowMap();
    void setupSkybox();
    void renderSkybox();

    // Callback functions
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
};

void updateModelMatrixFromPhysics(btRigidBody* body, glm::mat4& modelMatrix);

#endif // RENDERER_H
