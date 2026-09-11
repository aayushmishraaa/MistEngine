#pragma once
#ifndef MIST_REFLECTION_JSON_H
#define MIST_REFLECTION_JSON_H

// Reflection <-> JSON codec, shared by every serializer in the engine.
//
// Problem this solves: the PropertyType switch below existed twice, both
// file-static and therefore uncallable across TUs — once in
// SceneSerializer.cpp (writeReflectedFields / readReflectedFields, per-list)
// and once in MaterialSerializer.cpp (writeField / readField, per-field).
// Two copies already drifted in shape; a third was about to be written for
// the Environment resource and a fourth for prefabs. Hoisted here instead.
//
// Adding a field to a MIST_REFLECT block now makes it round-trip through
// every serializer at once, which is the whole point of the reflection layer.
//
// Reads are individually try/caught: a single malformed field in a scene or
// material file degrades to "field keeps its default + a warning" rather than
// aborting the load of everything after it. That behaviour is relied upon by
// the version-skew path, where an old file legitimately lacks newer fields.

#include "Core/Logger.h"
#include "Core/Reflection.h"

#include <nlohmann/json.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <string>

namespace Mist::Reflect {

// Writes one reflected field of `obj` into `j` under its property name.
inline void WriteField(nlohmann::json& j, const void* obj, const PropertyInfo& p) {
    const void* field = reinterpret_cast<const char*>(obj) + p.offset;
    switch (p.type) {
        case PropertyType::Bool:
            j[p.name] = *reinterpret_cast<const bool*>(field); break;
        case PropertyType::Int:
            j[p.name] = *reinterpret_cast<const int*>(field); break;
        case PropertyType::Enum:
            j[p.name] = enum_value(field, p.size); break;
        case PropertyType::Float:
            j[p.name] = *reinterpret_cast<const float*>(field); break;
        case PropertyType::Vec2: {
            const auto* v = reinterpret_cast<const glm::vec2*>(field);
            j[p.name] = {v->x, v->y}; break;
        }
        case PropertyType::Vec3: {
            const auto* v = reinterpret_cast<const glm::vec3*>(field);
            j[p.name] = {v->x, v->y, v->z}; break;
        }
        case PropertyType::Vec4: {
            const auto* v = reinterpret_cast<const glm::vec4*>(field);
            j[p.name] = {v->x, v->y, v->z, v->w}; break;
        }
        case PropertyType::String:
            j[p.name] = *reinterpret_cast<const std::string*>(field); break;
        default: break;
    }
}

// Reads one reflected field of `obj` from `j`. A missing key leaves the
// field at whatever the caller initialised it to — that is how a file
// written by an older build loads without clobbering new defaults.
inline void ReadField(const nlohmann::json& j, void* obj, const PropertyInfo& p) {
    if (!j.is_object()) return;
    auto it = j.find(p.name);
    if (it == j.end()) return;
    void* field = reinterpret_cast<char*>(obj) + p.offset;
    try {
        switch (p.type) {
            case PropertyType::Bool:
                *reinterpret_cast<bool*>(field) = it->get<bool>(); break;
            case PropertyType::Int:
                *reinterpret_cast<int*>(field) = it->get<int>(); break;
            case PropertyType::Enum:
                set_enum_value(field, p.size, it->get<long long>()); break;
            case PropertyType::Float:
                *reinterpret_cast<float*>(field) = it->get<float>(); break;
            case PropertyType::Vec2:
                if (it->is_array() && it->size() >= 2) {
                    auto* v = reinterpret_cast<glm::vec2*>(field);
                    v->x = (*it)[0].get<float>();
                    v->y = (*it)[1].get<float>();
                }
                break;
            case PropertyType::Vec3:
                if (it->is_array() && it->size() >= 3) {
                    auto* v = reinterpret_cast<glm::vec3*>(field);
                    v->x = (*it)[0].get<float>();
                    v->y = (*it)[1].get<float>();
                    v->z = (*it)[2].get<float>();
                }
                break;
            case PropertyType::Vec4:
                if (it->is_array() && it->size() >= 4) {
                    auto* v = reinterpret_cast<glm::vec4*>(field);
                    v->x = (*it)[0].get<float>();
                    v->y = (*it)[1].get<float>();
                    v->z = (*it)[2].get<float>();
                    v->w = (*it)[3].get<float>();
                }
                break;
            case PropertyType::String:
                *reinterpret_cast<std::string*>(field) = it->get<std::string>(); break;
            default: break;
        }
    } catch (const std::exception& e) {
        LOG_WARN("ReflectionJson: field '", p.name, "' read failed: ", e.what());
    }
}

// Whole-list convenience wrappers. `props` is what
// TypeRegistry::Instance().Get("TypeName") returns.
inline void WriteFields(nlohmann::json& j, const void* obj, const PropertyList& props) {
    for (const auto& p : props) WriteField(j, obj, p);
}

inline void ReadFields(const nlohmann::json& j, void* obj, const PropertyList& props) {
    if (!j.is_object()) return;
    for (const auto& p : props) ReadField(j, obj, p);
}

} // namespace Mist::Reflect

#endif // MIST_REFLECTION_JSON_H
