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
#include "PhysicsSystem.h" // Original physics system
#include "ECS/Systems/RenderSystem.h"
#include "ECS/Systems/ECSPhysicsSystem.h" // ECS physics system
#include "UI/EditorUI.h" // ImGui editor UI

class Scene; // Forward declaration
struct PhysicsRenderable; // Forward declaration for the struct

class Renderer {
public:
    Renderer(unsigned int width, unsigned int height);
    ~Renderer();

    bool Init();
    void Render(Scene& scene);
    void ProcessInput(GLFWwindow* window);
    void ProcessInputWithPhysics(GLFWwindow* window, PhysicsSystem& physicsSystem, std::vector<PhysicsRenderable>& physicsRenderables);
    void RenderWithECS(Scene& scene, std::shared_ptr<RenderSystem> renderSystem);

    Camera& GetCamera() { return camera; }
    GLFWwindow* GetWindow() const { return window; }
    float GetDeltaTime() const;
    EditorUI& GetEditorUI() { return *m_editorUI; }

private:
    unsigned int screenWidth;
    unsigned int screenHeight;
    GLFWwindow* window;

    // Shaders
    Shader objectShader;
    Shader depthShader;
    Shader glowShader;

    // Camera
    Camera camera;
    float lastX;
    float lastY;
    bool firstMouse;

    // Timing
    float deltaTime;
    float lastFrame;

    // Lighting
    glm::vec3 lightDir;
    glm::vec3 lightColor;

    // Shadow mapping
    unsigned int shadowWidth;
    unsigned int shadowHeight;
    unsigned int depthMapFBO;
    unsigned int depthMap;

    // Basic shapes (VAOs, VBOs, EBOs are now managed by Mesh objects)
    unsigned int planeVAO, planeVBO; // Still needed for cleanup in destructor
    unsigned int cubeVAO, cubeVBO, cubeEBO; // Still needed for cleanup in destructor

    // Editor UI
    std::unique_ptr<EditorUI> m_editorUI;

    void setupShadowMap();

    // Callback functions (will be set as static and call member functions)
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

void updateModelMatrixFromPhysics(btRigidBody* body, glm::mat4& modelMatrix);

#endif // RENDERER_H

