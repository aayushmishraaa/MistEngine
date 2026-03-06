#pragma once
#ifndef MIST_SCENE_NODE_H
#define MIST_SCENE_NODE_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <memory>
#include "AABB.h"
#include "ECS/Entity.h"

class SceneNode {
public:
    SceneNode(const std::string& name = "Node") : m_Name(name) {}

    // Transform
    glm::vec3 localPosition = glm::vec3(0.0f);
    glm::quat localRotation = glm::quat(1, 0, 0, 0);
    glm::vec3 localScale = glm::vec3(1.0f);

    // Hierarchy
    SceneNode* parent = nullptr;
    std::vector<std::shared_ptr<SceneNode>> children;

    // Data
    Entity entity = 0;
    bool hasEntity = false;
    std::string m_Name;
    AABB localAABB;
    AABB worldAABB;
    bool dirty = true;
    bool visible = true;

    glm::mat4 GetWorldTransform() const { return m_CachedWorldTransform; }
    glm::mat4 GetLocalTransform() const;

    void AddChild(std::shared_ptr<SceneNode> child);
    void RemoveChild(SceneNode* child);
    void MarkDirty();
    void UpdateWorldTransform();

private:
    glm::mat4 m_CachedWorldTransform = glm::mat4(1.0f);
};

#endif
