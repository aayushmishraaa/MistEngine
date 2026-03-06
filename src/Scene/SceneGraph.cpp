#include "Scene/SceneGraph.h"
#include "Core/Logger.h"
#include <glm/gtc/matrix_transform.hpp>

// --- SceneNode ---

glm::mat4 SceneNode::GetLocalTransform() const {
    glm::mat4 m(1.0f);
    m = glm::translate(m, localPosition);
    m *= glm::mat4_cast(localRotation);
    m = glm::scale(m, localScale);
    return m;
}

void SceneNode::AddChild(std::shared_ptr<SceneNode> child) {
    child->parent = this;
    children.push_back(child);
    child->MarkDirty();
}

void SceneNode::RemoveChild(SceneNode* child) {
    for (auto it = children.begin(); it != children.end(); ++it) {
        if (it->get() == child) {
            child->parent = nullptr;
            children.erase(it);
            return;
        }
    }
}

void SceneNode::MarkDirty() {
    dirty = true;
    for (auto& child : children) {
        child->MarkDirty();
    }
}

void SceneNode::UpdateWorldTransform() {
    if (!dirty) return;

    glm::mat4 local = GetLocalTransform();
    if (parent) {
        m_CachedWorldTransform = parent->GetWorldTransform() * local;
    } else {
        m_CachedWorldTransform = local;
    }

    worldAABB = localAABB.Transform(m_CachedWorldTransform);
    dirty = false;
}

// --- SceneGraph ---

SceneGraph::SceneGraph() {
    m_Root = std::make_shared<SceneNode>("Root");
}

std::shared_ptr<SceneNode> SceneGraph::CreateNode(const std::string& name) {
    auto node = std::make_shared<SceneNode>(name);
    m_NodeMap[name] = node;
    return node;
}

std::shared_ptr<SceneNode> SceneGraph::FindNode(const std::string& name) {
    auto it = m_NodeMap.find(name);
    return (it != m_NodeMap.end()) ? it->second : nullptr;
}

void SceneGraph::AddToRoot(std::shared_ptr<SceneNode> node) {
    m_Root->AddChild(node);
}

void SceneGraph::RemoveNode(const std::string& name) {
    auto it = m_NodeMap.find(name);
    if (it != m_NodeMap.end()) {
        auto node = it->second;
        if (node->parent) {
            node->parent->RemoveChild(node.get());
        }
        m_NodeMap.erase(it);
    }
}

void SceneGraph::UpdateTransforms() {
    updateTransformRecursive(m_Root.get());
}

void SceneGraph::updateTransformRecursive(SceneNode* node) {
    node->UpdateWorldTransform();
    for (auto& child : node->children) {
        updateTransformRecursive(child.get());
    }
}

void SceneGraph::PerformFrustumCulling(const Frustum& frustum, std::vector<SceneNode*>& visibleNodes) {
    visibleNodes.clear();
    frustumCullRecursive(m_Root.get(), frustum, visibleNodes);
}

void SceneGraph::frustumCullRecursive(SceneNode* node, const Frustum& frustum, std::vector<SceneNode*>& visible) {
    if (!node->visible) return;

    if (node->worldAABB.IsValid() && !frustum.Intersects(node->worldAABB)) {
        return; // Culled
    }

    if (node->hasEntity) {
        visible.push_back(node);
    }

    for (auto& child : node->children) {
        frustumCullRecursive(child.get(), frustum, visible);
    }
}
