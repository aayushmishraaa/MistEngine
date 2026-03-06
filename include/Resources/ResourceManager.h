#pragma once
#ifndef MIST_RESOURCE_MANAGER_H
#define MIST_RESOURCE_MANAGER_H

#include "ResourceHandle.h"
#include "Core/Logger.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>

template<typename T>
class ResourceManager {
public:
    using LoadFn = std::function<std::shared_ptr<T>(const std::string&)>;

    ResourceManager() : m_NextId(1), m_CurrentGeneration(1) {}

    void SetLoader(LoadFn loader) { m_Loader = std::move(loader); }

    ResourceHandle<T> Load(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_Mutex);

        // Check cache
        auto it = m_PathToHandle.find(path);
        if (it != m_PathToHandle.end()) {
            return it->second;
        }

        if (!m_Loader) {
            LOG_ERROR("ResourceManager: No loader set for type");
            return {};
        }

        auto resource = m_Loader(path);
        if (!resource) {
            LOG_ERROR("ResourceManager: Failed to load: ", path);
            return {};
        }

        ResourceHandle<T> handle;
        handle.id = m_NextId++;
        handle.generation = m_CurrentGeneration;

        m_Resources[handle.id] = resource;
        m_HandleToPath[handle.id] = path;
        m_PathToHandle[path] = handle;

        LOG_DEBUG("ResourceManager: Loaded '", path, "' as handle ", handle.id);
        return handle;
    }

    std::shared_ptr<T> Get(ResourceHandle<T> handle) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Resources.find(handle.id);
        if (it != m_Resources.end()) return it->second;
        return nullptr;
    }

    void Release(ResourceHandle<T> handle) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto pathIt = m_HandleToPath.find(handle.id);
        if (pathIt != m_HandleToPath.end()) {
            m_PathToHandle.erase(pathIt->second);
            m_HandleToPath.erase(pathIt);
        }
        m_Resources.erase(handle.id);
    }

    void ReloadAll() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_CurrentGeneration++;
        for (auto& [id, path] : m_HandleToPath) {
            if (m_Loader) {
                auto resource = m_Loader(path);
                if (resource) {
                    m_Resources[id] = resource;
                    LOG_INFO("ResourceManager: Reloaded '", path, "'");
                }
            }
        }
    }

    size_t Count() const { return m_Resources.size(); }

private:
    LoadFn m_Loader;
    uint32_t m_NextId;
    uint32_t m_CurrentGeneration;
    std::mutex m_Mutex;

    std::unordered_map<uint32_t, std::shared_ptr<T>> m_Resources;
    std::unordered_map<uint32_t, std::string> m_HandleToPath;
    std::unordered_map<std::string, ResourceHandle<T>> m_PathToHandle;
};

#endif // MIST_RESOURCE_MANAGER_H
