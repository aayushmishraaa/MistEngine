#pragma once
#ifndef MIST_EVENT_H
#define MIST_EVENT_H

#include <functional>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <any>

class EventBus {
public:
    template<typename T>
    using Handler = std::function<void(const T&)>;

    template<typename T>
    void Subscribe(Handler<T> handler) {
        auto key = std::type_index(typeid(T));
        m_Handlers[key].push_back([handler](const std::any& event) {
            handler(std::any_cast<const T&>(event));
        });
    }

    template<typename T>
    void Publish(const T& event) {
        auto key = std::type_index(typeid(T));
        auto it = m_Handlers.find(key);
        if (it != m_Handlers.end()) {
            for (auto& handler : it->second) {
                handler(event);
            }
        }
    }

    void Clear() { m_Handlers.clear(); }

private:
    std::unordered_map<std::type_index, std::vector<std::function<void(const std::any&)>>> m_Handlers;
};

// Common events
struct EntityDamagedEvent {
    uint32_t entity;
    float damage;
    uint32_t source;
};

struct EntityDestroyedEvent {
    uint32_t entity;
};

#endif
