#pragma once
#ifndef MIST_SYSTEM_SCHEDULER_H
#define MIST_SYSTEM_SCHEDULER_H

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include "System.h"

class SystemScheduler {
public:
    struct SystemEntry {
        std::string name;
        std::shared_ptr<System> system;
        std::vector<std::string> dependencies;
    };

    void AddSystem(const std::string& name, std::shared_ptr<System> system);
    void AddDependency(const std::string& system, const std::string& dependsOn);
    void BuildExecutionOrder();
    void Execute(float dt);

private:
    std::vector<SystemEntry> m_Systems;
    std::vector<int> m_ExecutionOrder;
    std::unordered_map<std::string, int> m_NameToIndex;
    bool m_OrderBuilt = false;

    void topologicalSort();
};

#endif
