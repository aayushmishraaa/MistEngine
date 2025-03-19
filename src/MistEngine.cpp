#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include "Shader.h"
#include "Camera.h"
#include "Enemy.h"
#include "Room.h"

// Global variables
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f)); // Camera initialized at position (0, 0, 3)
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f;

// Game objects
std::vector<Enemy> enemies;
std::vector<Room> rooms;

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void createEnemies();
void createRooms();
void renderEnemies(Shader& shader);
void renderRooms(Shader& shader);
bool checkPlayerCollision(const glm::vec3& newPosition);
void updatePlayerPosition(glm::vec3& newPosition);
bool checkRaycastHit(const glm::vec3& rayDirection);
void shoot();
void renderCrosshair(Shader& shader);

float vertices[] = {
    // Positions          // Colors
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f, // Red
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f, // Green
     0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f, // Blue
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f, // Yellow
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f, // Magenta
     0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f, // Cyan
     0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.5f, // Gray
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 1.0f  // White
};

unsigned int indices[] = {
    0, 1, 2, 2, 3, 0, // Front face
    4, 5, 6, 6, 7, 4, // Back face
    0, 4, 7, 7, 3, 0, // Left face
    1, 5, 6, 6, 2, 1, // Right face
    0, 1, 5, 5, 4, 0, // Bottom face
    3, 2, 6, 6, 7, 3  // Top face
};

unsigned int VBO, VAO, EBO;
unsigned int crosshairVAO, crosshairVBO;

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Set OpenGL version to 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Simple FPS", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Set callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Capture the mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Build and compile shaders
    Shader shader("shaders/vertex.glsl", "shaders/fragment.glsl");
    Shader crosshairShader("shaders/crosshair_vertex.glsl", "shaders/crosshair_fragment.glsl");

    // Create enemies and rooms
    createEnemies();
    createRooms();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Crosshair setup
    float crosshairVertices[] = {
        // Positions
        -0.01f,  0.0f,
         0.01f,  0.0f,
         0.0f,  -0.01f,
         0.0f,   0.01f
    };

    glGenVertexArrays(1, &crosshairVAO);
    glGenBuffers(1, &crosshairVBO);

    glBindVertexArray(crosshairVAO);

    glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        processInput(window);

        // Render
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activate shader
        shader.use();

        // Pass matrices to shader
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        // Render rooms
        renderRooms(shader);

        // Render enemies
        renderEnemies(shader);

        // Render crosshair
        renderCrosshair(crosshairShader);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    glfwTerminate();
    return 0;
}

// Input handling
void processInput(GLFWwindow* window) {
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

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        shoot();
    }
}

// Create enemies
void createEnemies() {
    enemies.push_back(Enemy(glm::vec3(5.0f, 0.0f, 5.0f), 100.0f)); // Enemy at (5, 0, 5)
    enemies.push_back(Enemy(glm::vec3(-5.0f, 0.0f, 5.0f), 100.0f)); // Enemy at (-5, 0, 5)
}

// Create rooms
void createRooms() {
    rooms.push_back(Room(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(10.0f, 5.0f, 10.0f))); // Room 1
    rooms.push_back(Room(glm::vec3(15.0f, 0.0f, 0.0f), glm::vec3(10.0f, 5.0f, 10.0f))); // Room 2
}

// Render enemies
void renderEnemies(Shader& shader) {
    for (auto& enemy : enemies) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, enemy.position); // Position the enemy
        shader.setMat4("model", model);                // Pass the model matrix to the shader

        // Bind the cube's VAO and draw it
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }
}

// Render rooms
void renderRooms(Shader& shader) {
    for (auto& room : rooms) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, room.position); // Position the room
        model = glm::scale(model, room.size);         // Scale the room to its size
        shader.setMat4("model", model);               // Pass the model matrix to the shader

        // Bind the cube's VAO and draw it
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }
}

// Render crosshair
void renderCrosshair(Shader& shader) {
    shader.use();
    glBindVertexArray(crosshairVAO);
    glDrawArrays(GL_LINES, 0, 4);
    glBindVertexArray(0);
}

// Check player collision
bool checkPlayerCollision(const glm::vec3& newPosition) {
    for (auto& room : rooms) {
        if (newPosition.x < room.position.x + room.size.x / 2 &&
            newPosition.x > room.position.x - room.size.x / 2 &&
            newPosition.z < room.position.z + room.size.z / 2 &&
            newPosition.z > room.position.z - room.size.z / 2) {
            return true; // Collision detected
        }
    }
    return false; // No collision
}

// Update player position
void updatePlayerPosition(glm::vec3& newPosition) {
    if (!checkPlayerCollision(newPosition)) {
        camera.Position = newPosition;
    }
}

// Check raycast hit
bool checkRaycastHit(const glm::vec3& rayDirection) {
    for (auto& enemy : enemies) {
        if (glm::distance(camera.Position, enemy.position) < 5.0f) { // Simple distance check
            enemy.takeDamage(50.0f); // Deal 50 damage
            return true;
        }
    }
    return false;
}

// Shoot
void shoot() {
    glm::vec3 rayDirection = camera.Front; // Use the camera's forward direction
    if (checkRaycastHit(rayDirection)) {
        std::cout << "Enemy hit!\n";
    }
}

// Window resize callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Mouse movement callback
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// Mouse scroll callback
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}
