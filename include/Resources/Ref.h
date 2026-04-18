#pragma once
#ifndef MIST_RESOURCE_REF_H
#define MIST_RESOURCE_REF_H

#include "Resources/ResourceHandle.h"
#include "Resources/ResourceManager.h"

#include <memory>
#include <string>
#include <utility>

namespace Mist::Assets {

// Ref<T> bundles the handle-side bookkeeping with a std::shared_ptr for
// non-owning access. The ResourceManager keeps its own strong reference
// so the asset stays alive regardless of how many Refs exist; Ref mostly
// exists as a call-site convenience so users don't have to remember the
// manager/handle dance:
//
//     auto mesh = AssetRegistry::Instance().meshes().LoadRef("builtin://cube");
//     mesh->Draw(shader);
//
// Non-owning because eviction semantics come later — for now two holders
// of the same path share one cached object and nothing frees it until
// ResourceManager::Release is called manually.
template <typename T>
class Ref {
public:
    Ref() = default;
    Ref(ResourceHandle<T> handle, std::shared_ptr<T> resource, std::string path)
        : m_Handle(handle), m_Resource(std::move(resource)), m_Path(std::move(path)) {}

    bool   IsValid() const { return m_Handle.IsValid() && m_Resource != nullptr; }
    explicit operator bool() const { return IsValid(); }

    T* operator->() const { return m_Resource.get(); }
    T& operator*()  const { return *m_Resource; }
    T* get()        const { return m_Resource.get(); }

    const std::string&     path()   const { return m_Path; }
    ResourceHandle<T>      handle() const { return m_Handle; }
    std::shared_ptr<T>     shared() const { return m_Resource; }

private:
    ResourceHandle<T>   m_Handle{};
    std::shared_ptr<T>  m_Resource;
    std::string         m_Path;
};

} // namespace Mist::Assets

// Extension: a LoadRef helper on ResourceManager<T> that returns a Ref<T>
// in one shot (cache lookup + Get in a single call).
template <typename T>
Mist::Assets::Ref<T> LoadRef(ResourceManager<T>& manager, const std::string& path) {
    auto handle = manager.Load(path);
    if (!handle.IsValid()) {
        return {};
    }
    auto resource = manager.Get(handle);
    return Mist::Assets::Ref<T>{handle, resource, path};
}

#endif // MIST_RESOURCE_REF_H
