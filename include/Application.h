
#ifndef APPLICATION_H
#define APPLICATION_H

#include <GLFW/glfw3.h>
#include <memory> // For std::unique_ptr
#include <iostream>

// Forward declarations for components we'll create later
class Renderer;
class Scene;
class InputManager;

class Application {
public:
    Application();
    ~Application();

    void run();

private:
    bool initialize();
    void shutdown();
    void processInput();
    void update(float deltaTime);
    void render();

    GLFWwindow* m_window;

    // Pointers to core components
    std::unique_ptr<Renderer> m_renderer;
    Scene* m_scene;
    InputManager* m_inputManager;

    // Timing
    float m_deltaTime;
    float m_lastFrame;

    // Window dimensions (can be made configurable later)
    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;
};

#endif // APPLICATION_H