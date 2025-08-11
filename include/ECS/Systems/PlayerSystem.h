#ifndef PLAYERSYSTEM_H
#define PLAYERSYSTEM_H

#include "../System.h"

struct GLFWwindow;
class Camera;

class PlayerSystem : public System {
public:
    void Init(GLFWwindow* window, Camera* camera);
    void Update(float deltaTime);
    void HandleInput(float deltaTime);
    void UpdateCamera();
    
    void SetGameMode(bool isGameMode) { m_IsGameMode = isGameMode; }
    bool IsGameMode() const { return m_IsGameMode; }

private:
    GLFWwindow* m_Window = nullptr;
    Camera* m_Camera = nullptr;
    bool m_IsGameMode = false;
    
    // Input state
    bool m_FirstMouse = true;
    double m_LastX = 0.0;
    double m_LastY = 0.0;
    
    void ProcessMovement(float deltaTime);
    void ProcessMouseLook();
    void ProcessJumping(float deltaTime);
};

#endif // PLAYERSYSTEM_H