#pragma once

#include <cstdint>

namespace Mist::ecs {

// Assigns a stable, process-unique integer ID to each distinct T the first
// time it's encountered. Keys were previously `typeid(T).name()` strings,
// which forced a string hash + compare on every AddComponent / GetComponent
// call — measurably hot on profiles of systems with many component accesses.
//
// Caveat: the exact numeric value depends on template-instantiation order and
// so differs between translation units if called before any TU has "seen"
// the type. Always go through RegisterComponent / RegisterSystem first so all
// TUs agree on the mapping. The same constraint existed under the string
// scheme (the same mangled name appears everywhere), so the behavioural
// contract is unchanged.
namespace detail {
inline std::uint32_t& type_counter() {
    static std::uint32_t n = 0;
    return n;
}
} // namespace detail

template <typename T> inline std::uint32_t type_id() {
    static const std::uint32_t id = detail::type_counter()++;
    return id;
}

} // namespace Mist::ecs
