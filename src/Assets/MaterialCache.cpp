#include "Assets/MaterialCache.h"

#include "Assets/MaterialSerializer.h"
#include "Material.h"

namespace Mist::Assets {

MaterialCache& MaterialCache::Instance() {
    static MaterialCache c;
    return c;
}

std::shared_ptr<PBRMaterial> MaterialCache::Get(const std::string& path) {
    {
        std::lock_guard<std::mutex> lk(m_Mu);
        auto it = m_ByPath.find(path);
        if (it != m_ByPath.end()) return it->second;
    }
    // Load outside the lock so concurrent misses don't serialise on
    // disk I/O. Losing the race and caching someone else's pointer is
    // fine — both callers get an equivalent material.
    auto mat = std::make_shared<PBRMaterial>();
    if (!MaterialSerializer::Load(path, *mat)) return nullptr;

    std::lock_guard<std::mutex> lk(m_Mu);
    auto it = m_ByPath.find(path);
    if (it != m_ByPath.end()) return it->second;
    m_ByPath[path] = mat;
    return mat;
}

std::shared_ptr<PBRMaterial> MaterialCache::Reload(const std::string& path) {
    auto mat = std::make_shared<PBRMaterial>();
    if (!MaterialSerializer::Load(path, *mat)) return nullptr;
    std::lock_guard<std::mutex> lk(m_Mu);
    m_ByPath[path] = mat;
    return mat;
}

void MaterialCache::Clear() {
    std::lock_guard<std::mutex> lk(m_Mu);
    m_ByPath.clear();
}

} // namespace Mist::Assets
