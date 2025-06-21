#include "/workspace/include/Application.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "/workspace/include/Renderer.h" // Include Renderer header

// Settings (you might want to move these to a configuration struct or class later)
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Application::Application() : m_window(nullptr) {
    // Constructor
}

Application::~Application() {
    // Destructor
    shutdown(); // Ensure shutdown is called if the Application object is destroyed
}
 
void Application::run() {
    if (!initialize()) {
        std::cerr << "Failed to initialize application!" << std::endl;
        return;
    }

    // Main application loop
    while (!glfwWindowShouldClose(m_window)) {
        // Render the scene using the Renderer
        if (m_renderer) {
            m_renderer->render();
        }

        // Swap buffers and poll events
        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

    shutdown();
}

bool Application::initialize() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    m_window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "MistEngine", NULL, NULL);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    // Set context
    glfwMakeContextCurrent(m_window);

    // Initialize GLAD (Ensure glad.h is included and gladLoadGLLoader is called)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
         std::cerr << "Failed to initialize GLAD" << std::endl;
         return false;
     }
    
    // Create the Renderer instance
    m_renderer = std::make_unique<Renderer>(m_window);



    std::cout << "Application initialized successfully!" << std::endl;
    return true;
}

void Application::shutdown() {
    if (m_window) {
        // Clean up OpenGL resources if any are managed directly by Application
        // (Most resources will be managed by Renderer, Scene, etc.)
    }

    // Terminate GLFW
    glfwTerminate();
    std::cout << "Application shutting down." << std::endl;
}
#include "Application.h"
#include <iostream>
#include <GLFW/glfw3.h>

// Settings (you might want to move these to a configuration struct or class later)
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Application::Application() : m_window(nullptr) {
    // Constructor
}

Application::~Application() {
    // Destructor
    shutdown(); // Ensure shutdown is called if the Application object is destroyed
}
 
void Application::run() {
    if (!initialize()) {
        std::cerr << "Failed to initialize application!" << std::endl;
        return;
    }

    // Main application loop
    while (!glfwWindowShouldClose(m_window)) {
        // Process input (will be handled by InputManager later)
        // processInput(m_window);

        // Update (will be handled by PhysicsManager and Scene later)
        // physicsSystem.stepSimulation(deltaTime);
        // Update object transforms from physics

        // Render (will be handled by Renderer later)
        // glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Render your scene here

        // Swap buffers and poll events
    }

    shutdown();
}

bool Application::initialize() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    m_window = glfwCreateWindow(800, 600, "MistEngine", NULL, NULL);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    // Set context
    glfwMakeContextCurrent(m_window);

    // Initialize GLAD (Ensure glad.h is included and gladLoadGLLoader is called)
    //     std::cerr << "Failed to initialize GLAD" << std::endl;
    //     return false;
    // }


    std::cout << "Application initialized successfully!" << std::endl;
    return true;
}

void Application::shutdown() {
    if (m_window) {
        // Clean up OpenGL resources if any are managed directly by Application
        // (Most resources will be managed by Renderer, Scene, etc.)
    }

    // Terminate GLFW
    glfwTerminate();
    std::cout << "Application shutting down." << std::endl;
}