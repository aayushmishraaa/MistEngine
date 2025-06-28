c++
#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

#include "Shader.h"
#include "Camera.h"
#include "Renderable.h"
#include "PhysicsSystem.h" // Include PhysicsSystem

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


    Camera& GetCamera() { return camera; }
    GLFWwindow* GetWindow() const { return window; }
    float GetDeltaTime() const;


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


    void setupShadowMap();
    // Removed: void renderBasicShapes(Shader& shader);


    // Callback functions (will be set as static and call member functions)
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
};

#endif // RENDERER_H
