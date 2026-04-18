#pragma once
#ifndef MIST_RESOURCE_MANAGER_H
#define MIST_RESOURCE_MANAGER_H

#include "ResourceHandle.h"
#include "Core/Logger.h"
#include <atomic>
#include <future>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

template<typename T>
class ResourceManager {
public:
    using LoadFn = std::function<std::shared_ptr<T>(const std::string&)>;

    ResourceManager() : m_NextId(1), m_CurrentGeneration(1) {}

    void SetLoader(LoadFn loader) { m_Loader = std::move(loader); }

    ResourceHandle<T> Load(const std::string& path) {
        // Fast path: cache hit. Lock briefly, check, return.
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            auto it = m_PathToHandle.find(path);
            if (it != m_PathToHandle.end()) {
                return it->second;
            }
            if (!m_Loader) {
                LOG_ERROR("ResourceManager: No loader set for type");
                return {};
            }
        }

        // Slow path: run the loader *without* holding the mutex. Otherwise
        // N concurrent LoadAsync calls would serialise on this mutex and
        // the "async" would be async-in-name-only. Two threads racing the
        // same path will both build a shared_ptr; the second to insert
        // simply returns the first's cached handle.
        auto resource = m_Loader(path);
        if (!resource) {
            LOG_ERROR("ResourceManager: Failed to load: ", path);
            return {};
        }

        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_PathToHandle.find(path);
        if (it != m_PathToHandle.end()) {
            // Another thread beat us to insertion; drop ours.
            return it->second;
        }

        ResourceHandle<T> handle;
        handle.id         = m_NextId++;
        handle.generation = m_CurrentGeneration;

        m_Resources[handle.id]    = resource;
        m_HandleToPath[handle.id] = path;
        m_PathToHandle[path]      = handle;

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

    // ------------------------------------------------------------------
    // G14 — async loading.
    //
    // LoadAsync returns a future that resolves to the same handle Load
    // would return. Under the hood we use std::async with deferred status
    // tracked via a shared_ptr<atomic<LoadStatus>> keyed by path so
    // multiple callers against the same in-flight load see consistent
    // status. A proper thread pool lands with G6/G9.
    // ------------------------------------------------------------------
    enum class LoadStatus {
        NotLoaded,
        InProgress,
        Ready,
        Failed,
    };

    std::future<ResourceHandle<T>> LoadAsync(const std::string& path) {
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            auto cached = m_PathToHandle.find(path);
            if (cached != m_PathToHandle.end()) {
                // Already loaded synchronously; return a ready future.
                std::promise<ResourceHandle<T>> p;
                p.set_value(cached->second);
                m_Status[path] = LoadStatus::Ready;
                return p.get_future();
            }
            m_Status[path] = LoadStatus::InProgress;
        }

        return std::async(std::launch::async, [this, path]() {
            auto handle = Load(path); // Load is already thread-safe.
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_Status[path] = handle.IsValid() ? LoadStatus::Ready : LoadStatus::Failed;
            return handle;
        });
    }

    LoadStatus GetStatus(const std::string& path) const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Status.find(path);
        if (it == m_Status.end()) return LoadStatus::NotLoaded;
        return it->second;
    }

private:
    LoadFn             m_Loader;
    uint32_t           m_NextId;
    uint32_t           m_CurrentGeneration;
    mutable std::mutex m_Mutex;

    std::unordered_map<uint32_t,   std::shared_ptr<T>> m_Resources;
    std::unordered_map<uint32_t,   std::string>        m_HandleToPath;
    std::unordered_map<std::string, ResourceHandle<T>> m_PathToHandle;
    std::unordered_map<std::string, LoadStatus>        m_Status;
};

#endif // MIST_RESOURCE_MANAGER_H
