#include "ECS/SystemScheduler.h"
#include "Core/Logger.h"
#include <queue>
#include <algorithm>

void SystemScheduler::AddSystem(const std::string& name, std::shared_ptr<System> system) {
    int idx = (int)m_Systems.size();
    m_NameToIndex[name] = idx;
    m_Systems.push_back({name, system, {}});
    m_OrderBuilt = false;
}

void SystemScheduler::AddDependency(const std::string& system, const std::string& dependsOn) {
    auto it = m_NameToIndex.find(system);
    if (it != m_NameToIndex.end()) {
        m_Systems[it->second].dependencies.push_back(dependsOn);
        m_OrderBuilt = false;
    }
}

void SystemScheduler::BuildExecutionOrder() {
    topologicalSort();
    m_OrderBuilt = true;
}

void SystemScheduler::topologicalSort() {
    int n = (int)m_Systems.size();
    std::vector<int> inDegree(n, 0);
    std::vector<std::vector<int>> adj(n);

    for (int i = 0; i < n; i++) {
        for (const auto& dep : m_Systems[i].dependencies) {
            auto it = m_NameToIndex.find(dep);
            if (it != m_NameToIndex.end()) {
                adj[it->second].push_back(i);
                inDegree[i]++;
            }
        }
    }

    std::queue<int> q;
    for (int i = 0; i < n; i++) {
        if (inDegree[i] == 0) q.push(i);
    }

    m_ExecutionOrder.clear();
    while (!q.empty()) {
        int u = q.front(); q.pop();
        m_ExecutionOrder.push_back(u);
        for (int v : adj[u]) {
            if (--inDegree[v] == 0) q.push(v);
        }
    }

    if ((int)m_ExecutionOrder.size() != n) {
        LOG_ERROR("SystemScheduler: Cycle detected in dependency graph!");
    }
}

void SystemScheduler::Execute(float dt) {
    if (!m_OrderBuilt) BuildExecutionOrder();
    for (int idx : m_ExecutionOrder) {
        m_Systems[idx].system->Update(dt);
    }
}
