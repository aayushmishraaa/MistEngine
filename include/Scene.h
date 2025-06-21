
#ifndef MISTENGINE_SCENE_H
#define MISTENGINE_SCENE_H

#include <vector>
#include <memory> // For std::unique_ptr

// Forward declarations
class GameObject; // We'll create this class soon
class PhysicsWorld;

class Scene {
public:
    // Constructor and Destructor
    Scene();
    ~Scene();

    // Add a GameObject to the scene
    void addGameObject(std::unique_ptr<GameObject> gameObject);

    // Remove a GameObject from the scene
    void removeGameObject(GameObject* gameObject);

    // Update the scene (including physics)
    void update(float deltaTime); 

    // Render the scene with specified shaders

    // Get access to the physics world
    PhysicsWorld* getPhysicsWorld() const { return m_physicsWorld.get(); }

    // Get the list of game objects (for iteration, e.g., in rendering or editor)
    const std::vector<std::unique_ptr<GameObject>>& getGameObjects() const { return m_gameObjects; }

private:
    std::vector<std::unique_ptr<GameObject>> m_gameObjects;
    std::unique_ptr<PhysicsWorld> m_physicsWorld;
};

#endif // MISTENGINE_SCENE_H