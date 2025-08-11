#ifndef WEAPONSYSTEM_H
#define WEAPONSYSTEM_H

#include "../System.h"
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class Camera;

class WeaponSystem : public System {
public:
    void Init(GLFWwindow* window, Camera* camera);
    void Update(float deltaTime);
    void SetGameMode(bool isGameMode) { m_IsGameMode = isGameMode; }

private:
    GLFWwindow* m_Window = nullptr;
    Camera* m_Camera = nullptr;
    bool m_IsGameMode = false;
    
    void ProcessShooting(float deltaTime);
    void ProcessReloading(float deltaTime);
    void FireProjectile(const glm::vec3& origin, const glm::vec3& direction, float damage, Entity owner);
    bool PerformRaycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, Entity& hitEntity, glm::vec3& hitPoint);
};

#endif // WEAPONSYSTEM_H