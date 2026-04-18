#pragma once
#ifndef MIST_RID_H
#define MIST_RID_H

#include <cstdint>
#include <functional>

// Resource ID — opaque handle to a GPU-side object. Patterned after
// Godot's RID: the render backend (RenderingDevice) owns the actual GPU
// resource; main-thread code only holds the integer ID and never
// dereferences a pointer to something the render thread may have moved.
//
// Landing this type in isolation gives new GPU code a contract to build
// against. The next cycle migrates existing Renderer/LightManager/
// PostProcess sites from raw GLuints to RIDs, which is what will
// eventually unlock multi-threaded rendering and backend swaps
// (Vulkan/D3D12). Until then, RIDs coexist with raw handles and the
// RenderingDevice interface (see RenderingDevice.h) translates between
// the two.
struct RID {
    std::uint64_t id = 0;

    constexpr bool IsValid() const noexcept { return id != 0; }

    constexpr bool operator==(const RID& other) const noexcept { return id == other.id; }
    constexpr bool operator!=(const RID& other) const noexcept { return id != other.id; }
};

namespace std {
template <>
struct hash<RID> {
    std::size_t operator()(const RID& r) const noexcept {
        return std::hash<std::uint64_t>{}(r.id);
    }
};
} // namespace std

#endif // MIST_RID_H
