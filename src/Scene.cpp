
#include "Scene.h"
#include "GameObject.h" // We'll create this soon
#include "PhysicsWorld.h"
#include <algorithm> // For std::remove_if

Scene::Scene()
    : m_physicsWorld(std::make_unique<PhysicsWorld>())
{
}

Scene::~Scene()
{
    // The unique_ptrs in m_gameObjects will automatically delete the GameObjects
}

void Scene::addGameObject(std::unique_ptr<GameObject> gameObject)
{
    if (gameObject) {
        // Add the game object to the list
        m_gameObjects.push_back(std::move(gameObject));

        // If the game object has a physics component, add it to the physics world
        // We'll need to check for a PhysicsComponent here later when we implement components.
        // For now, if the GameObject itself contains a PhysicsObject pointer, add it.
        // This part will need adjustment based on how we structure GameObject and components.
        // For the current simple RigidBody, we might do something like:
        // if (m_gameObjects.back()->getPhysicsObject()) {
        //     m_physicsWorld->addObject(m_gameObjects.back()->getPhysicsObject());
        // }
    }
}

void Scene::removeGameObject(GameObject* gameObject)
{
    // Remove the game object from the list
    m_gameObjects.erase(
        std::remove_if(m_gameObjects.begin(), m_gameObjects.end(),
                       [&](const std::unique_ptr<GameObject>& obj) {
                           return obj.get() == gameObject;
                       }),
        m_gameObjects.end());

    // If the removed game object had a physics component, remove it from the physics world
    // This will also need adjustment based on components.
    // if (gameObject->getPhysicsObject()) {
    //     m_physicsWorld->removeObject(gameObject->getPhysicsObject());
    // }
}

void Scene::update(float deltaTime)
{
    // Update all game objects
    for (const auto& obj : m_gameObjects) {
        obj->update(deltaTime);
    }

    // Update the physics world (collision detection and response will go here later)
    m_physicsWorld->update(deltaTime);
}

void Scene::render(Shader& depthShader, Shader& sceneShader)