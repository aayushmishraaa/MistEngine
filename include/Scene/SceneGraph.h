#pragma once
#ifndef MIST_SCENE_GRAPH_H
#define MIST_SCENE_GRAPH_H

#include "SceneNode.h"
#include "Frustum.h"
#include <unordered_map>
#include <string>

class SceneGraph {
public:
    SceneGraph();

    std::shared_ptr<SceneNode> CreateNode(const std::string& name = "Node");
    std::shared_ptr<SceneNode> FindNode(const std::string& name);
    void AddToRoot(std::shared_ptr<SceneNode> node);
    void RemoveNode(const std::string& name);

    void UpdateTransforms();
    void PerformFrustumCulling(const Frustum& frustum, std::vector<SceneNode*>& visibleNodes);

    SceneNode* GetRoot() { return m_Root.get(); }

private:
    std::shared_ptr<SceneNode> m_Root;
    std::unordered_map<std::string, std::shared_ptr<SceneNode>> m_NodeMap;

    void updateTransformRecursive(SceneNode* node);
    void frustumCullRecursive(SceneNode* node, const Frustum& frustum, std::vector<SceneNode*>& visible);
};

#endif
