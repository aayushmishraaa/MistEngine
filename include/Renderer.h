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
#include "Environment.h"
#include "Renderer/CameraView.h"
#include "PostProcessStack.h"
#include "HiZPyramid.h"
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
    // Fallback depth range, used by the editor's free-fly camera and by any
    // scene with no active CameraComponent.
    //
    // These two numbers have to agree across the projection matrix, the PBR
    // shader's nearPlane/farPlane uniforms, the CSM cascade splits and the
    // cluster grid's log-z slicing. That agreement is now enforced by
    // CameraView rather than by everyone reading the same two constants —
    // which is what makes a per-camera range safe at all.
    static constexpr float kNearPlane = 0.1f;
    static constexpr float kFarPlane  = 100.0f;

    Renderer(unsigned int width, unsigned int height);
    ~Renderer();

    bool Init();
    void Render(Scene& scene);
    void ProcessInput(GLFWwindow* window);
    void ProcessInputWithPhysics(GLFWwindow* window, PhysicsSystem& physicsSystem, std::vector<PhysicsRenderable>& physicsRenderables);
    void RenderWithECS(Scene& scene, std::shared_ptr<RenderSystem> renderSystem);
    void RenderWithECSAndUI(Scene& scene, std::shared_ptr<RenderSystem> renderSystem, UIManager* uiManager);

    // The editor's free-fly viewport camera. Not a scene object — see
    // CameraComponent for the in-scene viewpoint.
    Camera& GetCamera() { return camera; }

    // The viewpoint the last frame actually rendered with: an active
    // CameraComponent when the scene has one, otherwise the editor camera.
    // ImGuizmo needs the same matrices and depth range the frame used, and
    // reading `camera` plus the constants is how those drift apart.
    const CameraView& GetActiveView() const { return m_ActiveView; }
    GLFWwindow* GetWindow() const { return window; }
    float GetDeltaTime() const;

    // The scene's Environment — the single owner of every render tunable.
    // The View menu, the Inspector panel and the scene serializer all go
    // through this one object; before it existed each renderer carried its own
    // public fields and none of them were saved.
    Environment&       GetEnvironment()       { return m_Environment; }
    const Environment& GetEnvironment() const { return m_Environment; }

    // New subsystem accessors
    PostProcessStack& GetPostProcess() { return m_PostProcess; }
    ShadowSystem& GetShadowSystem() { return m_ShadowSystem; }
    LightManager& GetLightManager() { return m_LightManager; }
    IBL& GetIBL() { return m_IBL; }
    SkyboxRenderer& GetSkybox() { return m_Skybox; }
    GPUParticleSystem& GetParticles() { return m_Particles; }
    Profiler& GetProfiler() { return m_Profiler; }
    UBOManager& GetUBOManager() { return m_UBOManager; }

    // Kept as a convenience proxy; exposure lives on the Environment.
    float GetExposure() const { return m_Environment.exposure; }
    void SetExposure(float e) { m_Environment.exposure = e; }

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
    Shader depthPrepassShader; // Writes screen-space depth + packed normal/roughness
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
    HiZPyramid       m_HiZ;
    ShadowSystem m_ShadowSystem;
    LightManager m_LightManager;
    IBL m_IBL;
    SkyboxRenderer m_Skybox;
    GPUParticleSystem m_Particles;
    UBOManager m_UBOManager;
    Profiler m_Profiler;
    Environment m_Environment;

    // Resolved once per frame at the top of RenderWithECSAndUI.
    CameraView m_ActiveView;

    // Picks the active CameraComponent, falling back to the editor camera.
    CameraView ResolveActiveView() const;
    GLuint m_DummyTex2D = 0;
    GLuint m_DummyTexCube = 0;
    void CreateDummyTextures();

    // TAA previous frame state
    glm::mat4 m_PrevViewProjection = glm::mat4(1.0f);

    // Cluster-grid cache keys. The grid is a pure function of projection +
    // screen size, so it only needs rebuilding when one of these changes.
    unsigned int m_ClusterGridWidth  = 0;
    unsigned int m_ClusterGridHeight = 0;
    float        m_ClusterGridZoom   = 0.0f;
    float        m_ClusterGridNear   = 0.0f;
    float        m_ClusterGridFar    = 0.0f;

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
