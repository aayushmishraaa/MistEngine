#pragma once
#ifndef MIST_RESOURCE_HANDLE_H
#define MIST_RESOURCE_HANDLE_H

#include <cstdint>

template<typename T>
struct ResourceHandle {
    uint32_t id = 0;
    uint32_t generation = 0;

    bool IsValid() const { return id != 0; }
    bool operator==(const ResourceHandle& other) const {
        return id == other.id && generation == other.generation;
    }
    bool operator!=(const ResourceHandle& other) const { return !(*this == other); }

    struct Hash {
        size_t operator()(const ResourceHandle& h) const {
            return std::hash<uint64_t>()(((uint64_t)h.id << 32) | h.generation);
        }
    };
};

#endif // MIST_RESOURCE_HANDLE_H
