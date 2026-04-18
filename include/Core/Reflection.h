#pragma once
#ifndef MIST_REFLECTION_H
#define MIST_REFLECTION_H

// Lightweight reflection for MistEngine components + assets.
//
// Problem this solves: before G1, every new ECS component required a
// hand-coded ImGui panel inside UIManager::DrawInspector. Roughly 50 LOC of
// boilerplate per field, and the inspector couldn't surface anything the
// engine didn't know to show. After G1:
//
//     struct HealthComponent { float health = 100.0f; float maxHealth = 100.0f; };
//
//     MIST_REFLECT(HealthComponent)
//         MIST_FIELD(HealthComponent, health,    Mist::PropertyHint::Range, "0,100")
//         MIST_FIELD(HealthComponent, maxHealth, Mist::PropertyHint::Range, "0,1000")
//     MIST_REFLECT_END(HealthComponent)
//
// …and the inspector auto-generates the right widgets at runtime with no
// further UIManager edits.
//
// Shape borrowed from Godot's ClassDB + PropertyInfo: each property carries
// a type tag, an editor hint, and a byte offset from the struct base.
// UIManager walks the PropertyList for a given registered type and dispatches
// to one ImGui widget per (PropertyType, PropertyHint) pair.

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Mist {

// Concrete C++ types we know how to inspect. Extend as new components need
// new field types; every addition must also gain a draw case in UIManager.
enum class PropertyType : std::uint8_t {
    Unknown = 0,
    Bool,
    Int,
    Float,
    Vec2,
    Vec3,
    Vec4,
    String,
};

// Editor hints — guide the inspector widget choice and any constraints.
// Modeled on Godot's PropertyHint enum but trimmed to what MistEngine can
// actually render today.
enum class PropertyHint : std::uint8_t {
    None = 0,
    Range,       // hint_string = "min,max" or "min,max,step"
    Enum,        // hint_string = "A,B,C"
    File,        // hint_string = filter (unused for now)
    Color,       // vec3/vec4 → color picker
    Multiline,   // string → multi-line text box
    ResourceRef, // future: ResourceHandle picker
};

struct PropertyInfo {
    const char*   name;          // pointer into the registering TU's string table
    PropertyType  type;
    PropertyHint  hint;
    const char*   hintString;    // interpretation depends on hint
    std::size_t   offset;        // offsetof inside the registered struct
    std::size_t   size;          // sizeof(field) — used for sanity checks
};

// All properties registered for a single type.
using PropertyList = std::vector<PropertyInfo>;

// Global type→properties registry. Not thread-safe at registration time;
// registration is expected to happen during static-init + early main() only.
class TypeRegistry {
public:
    static TypeRegistry& Instance();

    // Register a reflected type. Returns true on first registration,
    // false if already registered (overwrites properties in either case).
    bool Register(std::string_view name, PropertyList props);

    const PropertyList* Get(std::string_view name) const;

    // Iterate over every registered type. Caller receives (name, props).
    template <typename Fn>
    void ForEach(Fn&& fn) const {
        for (const auto& [name, props] : m_Types) {
            fn(name, props);
        }
    }

private:
    TypeRegistry() = default;
    std::unordered_map<std::string, PropertyList> m_Types;
};

// Helper for parsing PropertyHint::Range hint strings of the form
// "min,max" or "min,max,step". Returns false on malformed input.
bool parse_range_hint(std::string_view hint, float& min, float& max, float& step);

// Compile-time C++ type → PropertyType mapping, used by MIST_FIELD. Kept in
// this header so macro expansion sees it without a second include.
template <typename T>
constexpr PropertyType mist_type_tag() {
    if constexpr (std::is_same_v<T, bool>)               return PropertyType::Bool;
    else if constexpr (std::is_integral_v<T>)            return PropertyType::Int;
    else if constexpr (std::is_same_v<T, float>)         return PropertyType::Float;
    else if constexpr (std::is_same_v<T, double>)        return PropertyType::Float;
    else if constexpr (std::is_same_v<T, glm::vec2>)     return PropertyType::Vec2;
    else if constexpr (std::is_same_v<T, glm::vec3>)     return PropertyType::Vec3;
    else if constexpr (std::is_same_v<T, glm::vec4>)     return PropertyType::Vec4;
    else if constexpr (std::is_same_v<T, std::string>)   return PropertyType::String;
    else                                                 return PropertyType::Unknown;
}

} // namespace Mist

// ---------------------------------------------------------------------------
// User-facing macros
// ---------------------------------------------------------------------------
//
// Usage:
//
//     MIST_REFLECT(MyComponent)
//         MIST_FIELD(MyComponent, someFloat, Mist::PropertyHint::Range, "0,100")
//         MIST_FIELD(MyComponent, someVec3,  Mist::PropertyHint::None,  "")
//     MIST_REFLECT_END(MyComponent)
//
// The pair of macros wrap a file-scope static initializer that appends to
// the global TypeRegistry during program start-up. PropertyType deduction
// relies on the field's C++ type via mist_type_tag<T>().

#define MIST_REFLECT(TypeName)                                               \
    namespace {                                                              \
    const bool mist_reflect_##TypeName##_registered = []() {                 \
        using _MistReflectType = TypeName;                                   \
        ::Mist::PropertyList _mist_props;                                    \
        /* fields appended below */

#define MIST_FIELD(TypeName, FieldName, Hint, HintStr)                       \
        _mist_props.push_back(::Mist::PropertyInfo{                          \
            #FieldName,                                                      \
            ::Mist::mist_type_tag<decltype(_MistReflectType::FieldName)>(),  \
            (Hint),                                                          \
            (HintStr),                                                       \
            offsetof(_MistReflectType, FieldName),                           \
            sizeof(decltype(_MistReflectType::FieldName))                    \
        });

#define MIST_REFLECT_END(TypeName)                                           \
        return ::Mist::TypeRegistry::Instance().Register(                    \
            #TypeName, std::move(_mist_props));                              \
    }();                                                                     \
    } /* anonymous namespace */

#endif // MIST_REFLECTION_H
