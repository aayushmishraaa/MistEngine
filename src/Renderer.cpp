#include "/github/workspace/include/Renderer.h"
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "/github/workspace/include/Shader.h"
#include "/github/workspace/include/Texture.h"

// Shadow mapping settings (should ideally be in a config or renderer settings)
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

Renderer::Renderer(GLFWwindow* window) : m_window(window) {
    std::cout << "Renderer constructor called." << std::endl;
    // Initialize GLAD here since we have the window and context
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
    }
}

Renderer::~Renderer() {
    if (m_cubeVAO) glDeleteVertexArrays(1, &m_cubeVAO);
    if (m_planeVAO) glDeleteVertexArrays(1, &m_planeVAO);
    if (m_cubeVBO) glDeleteBuffers(1, &m_cubeVBO);
    if (m_cubeEBO) glDeleteBuffers(1, &m_cubeEBO);
    if (m_planeVBO) glDeleteBuffers(1, &m_planeVBO);
    if (m_depthMapFBO) glDeleteFramebuffers(1, &m_depthMapFBO);
    if (m_depthMap) glDeleteTextures(1, &m_depthMap);
    std::cout << "Renderer destructor called." << std::endl;
}

void Renderer::initialize() {
    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);

    // Load shaders
    m_shader = std::make_unique<Shader>("/github/workspace/shaders/vertex.glsl", "/github/workspace/shaders/fragment.glsl");
    m_depthShader = std::make_unique<Shader>("/github/workspace/shaders/depth_vertex.glsl", "/github/workspace/shaders/depth_fragment.glsl");
    m_glowShader = std::make_unique<Shader>("/github/workspace/shaders/glow_vertex.glsl", "/github/workspace/shaders/glow_fragment.glsl");

    // Load texture
    m_cubeTexture = std::make_unique<Texture>();
    if (!m_cubeTexture->LoadFromFile("/github/workspace/textures/container.jpg")) {
        std::cerr << "Failed to load texture" << std::endl;
        // Handle error appropriately, perhaps return false from initialize
    }

    // Setup scene geometry and shadow map
    setupGeometry();
}

void Renderer::render() {
    // Render to depth map
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    float near_plane = 1.0f, far_plane = 7.5f;
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
    // lightDir needs to be accessible, perhaps passed in or as a member
    glm::vec3 lightDir(-0.2f, -1.0f, -0.3f); 
    glm::mat4 lightView = glm::lookAt(-lightDir * 5.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    m_depthShader->use();
    m_depthShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    RenderScene(*m_depthShader); // Use the private RenderScene method

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Reset viewport (need screen dimensions, perhaps passed in or as members)
    int display_w, display_h;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw glowing orb first (for proper blending if enabled)
    // Camera and Orb objects need to be accessible, perhaps passed in or as members
    // For now, using placeholders and assuming their presence
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)display_w / (float)display_h, 0.1f, 100.0f); // Use a default FOV
    glm::mat4 view = glm::mat4(1.0f); // Placeholder view matrix
    glm::vec3 cameraPosition(0.0f, 0.0f, 3.0f); // Placeholder camera position
    glm::vec3 orbPosition(1.5f, 1.0f, 0.0f); // Placeholder orb position
    glm::vec3 orbColor(2.0f, 1.6f, 0.4f); // Placeholder orb color


    m_glowShader->use();
    m_glowShader->setMat4("projection", projection);
    m_glowShader->setMat4("view", view);
    // Need a way to draw the orb - this might involve passing it in or having a list of renderables
    // For now, a placeholder:
    // glowingOrb.Draw(*m_glowShader);

    // Draw the loaded 3D model
    m_shader->use();
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
    m_shader->setMat4("model", model);
    // Need a way to draw the model - this might involve passing it in or having a list of renderables
    // For now, a placeholder:
    // ourModel.Draw(*m_shader);

    // Render scene with shadows
    m_shader->use();
    m_shader->setMat4("projection", projection);
    m_shader->setMat4("view", view);
    m_shader->setVec3("lightDir", lightDir);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f); // Placeholder light color
    m_shader->setVec3("lightColor", lightColor);
    m_shader->setVec3("viewPos", cameraPosition);
    m_shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

    // Pass orb properties to shader
    m_shader->setVec3("orbPosition", orbPosition);
    m_shader->setVec3("orbColor", orbColor);

    // Bind depth map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_depthMap);
    m_shader->setInt("shadowMap", 0);

    // Bind cube texture
    m_cubeTexture->Bind(1);
    m_shader->setInt("diffuseTexture", 1);

    RenderScene(*m_shader); // Use the private RenderScene method
}

void Renderer::setupGeometry() {
    // Cube vertices with texture coordinates and normals
    float vertices[] = {
        // Positions          // Normals           // Texture Coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f
    };

    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };

    // Cube VAO, VBO, EBO
    glGenVertexArrays(1, &m_cubeVAO);
    glGenBuffers(1, &m_cubeVBO);
    glGenBuffers(1, &m_cubeEBO);

    glBindVertexArray(m_cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0); // Unbind VAO

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

    // Plane VAO and VBO
    glGenVertexArrays(1, &m_planeVAO);
    glGenBuffers(1, &m_planeVBO);

    glBindVertexArray(m_planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0); // Unbind VAO

    // Shadow mapping setup
    glGenFramebuffers(1, &m_depthMapFBO);
    glGenTextures(1, &m_depthMap);
    glBindTexture(GL_TEXTURE_2D, m_depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Moved and adapted from MistEngine.cpp
void Renderer::RenderScene(Shader& shader) {
    // Render plane
    glm::mat4 model = glm::mat4(1.0f);
    shader.setMat4("model", model);
    glBindVertexArray(m_planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

Renderer::~Renderer() {
    std::cout << "Renderer destructor called." << std::endl;
}

void Renderer::initialize() {
}

void Renderer::render() {
}

void Renderer::shutdown() {
    std::cout << "Renderer shutdown called." << std::endl;
    // TODO: Add OpenGL cleanup code here (delete VAOs, VBOs, framebuffers, textures, etc.)
}