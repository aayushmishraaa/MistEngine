#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"
#include "Camera.h" // Assuming Camera class will be used by the Renderer
#include "Texture.h" // Include Texture header
#include "Model.h" // Include Model header
#include "Orb.h"   // Include Orb header

#include <vector> // Might need for storing objects to render

// Forward declarations to avoid circular dependencies if needed

class Renderer

{
public:
    Renderer();
    ~Renderer();

    void Initialize();
    void Render(const Camera& camera); // Pass camera for view/projection matrices
    void Cleanup();


private:
    // Shaders
    Shader m_shader;
    Shader m_depthShader;
    Shader m_glowShader;

    // Shadow mapping
    unsigned int m_depthMapFBO;
    unsigned int m_depthMap;
    const unsigned int SHADOW_WIDTH = 1024; // Consider making these configurable
    const unsigned int SHADOW_HEIGHT = 1024;

    // Geometry (VAOs, VBOs, EBOs) for static objects
    unsigned int m_planeVAO, m_planeVBO;
    unsigned int m_cubeVAO, m_cubeVBO, m_cubeEBO;

    // Texture
    Texture m_cubeTexture; // Use Texture class member

    // Light properties (might be better in a Light class or Scene)
    glm::vec3 m_lightDir;
    glm::vec3 m_lightColor;

    // Private helper methods for setup and rendering specific parts
    void setupOpenGLState();
    void loadShaders();
    void setupDepthMap();
    void setupGeometry();
    void loadTextures(); // Assuming you have a Texture loading utility

    // Rendering methods for different passes or object types
    void renderDepthMap(const Shader& depthShader, const glm::mat4& lightSpaceMatrix);
    void renderSceneObjects(const Shader& shader, const glm::mat4& view, const glm::mat4& projection, const glm::mat4& lightSpaceMatrix, const glm::vec3& viewPos, const glm::vec3& orbPosition, const glm::vec3& orbColor);
    void renderGlowObjects(const Shader& glowShader, const glm::mat4& view, const glm::mat4& projection);

    // Temporary members for loaded model and orb
    // These should ideally be managed by a Scene class
    Model m_backpackModel;
    Orb m_glowingOrb;


    // Helper render functions for specific geometry
    void renderCube();
    void renderPlane();
    // void renderModel(const Model& model, const Shader& shader);
    // void renderOrb(const Orb& orb, const Shader& shader);

};
#endif // RENDERER_H