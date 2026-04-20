#pragma once
#ifndef MIST_MATERIAL_SERIALIZER_H
#define MIST_MATERIAL_SERIALIZER_H

#include <string>

struct PBRMaterial;

namespace Mist::Assets {

// .mistmat JSON round-trip for PBRMaterial. Reflection-driven: walks
// TypeRegistry::Get("PBRMaterial") so field additions to Material.h
// pick up automatically here. Path fields round-trip as strings;
// runtime shared_ptr handles are rebuilt via PBRMaterial::Refresh
// after parse.
struct MaterialSerializer {
    // Serialize to JSON string. Stable key order = field registration
    // order from MIST_REFLECT (useful for diffable .mistmat files).
    static std::string ToJsonString(const PBRMaterial& mat);

    // Write to path. Returns false on I/O failure.
    static bool Save(const PBRMaterial& mat, const std::string& path);

    // Read JSON from path into a fresh PBRMaterial. Missing fields are
    // left at defaults. Returns true on success; sets out to defaults
    // on failure.
    static bool Load(const std::string& path, PBRMaterial& out);
};

} // namespace Mist::Assets

#endif // MIST_MATERIAL_SERIALIZER_H
