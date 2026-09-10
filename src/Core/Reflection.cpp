#include "Core/Reflection.h"

#include <charconv>
#include <string>

namespace Mist {

TypeRegistry& TypeRegistry::Instance() {
    static TypeRegistry inst;
    return inst;
}

bool TypeRegistry::Register(std::string_view name, PropertyList props) {
    // string_view isn't directly usable as an unordered_map key with a
    // std::string comparator, so we materialise once up-front.
    std::string key(name);
    const bool firstTime = (m_Types.find(key) == m_Types.end());
    m_Types[std::move(key)] = std::move(props);
    return firstTime;
}

const PropertyList* TypeRegistry::Get(std::string_view name) const {
    auto it = m_Types.find(std::string(name));
    if (it == m_Types.end()) return nullptr;
    return &it->second;
}

namespace {
// Parse a single float out of s; returns true on full consumption.
bool parse_float(std::string_view s, float& out) {
    // std::from_chars doesn't accept float on all libstdc++ versions we ship
    // against, so fall back to std::stof with explicit bounds. The input is
    // user-provided but fixed-format ("min,max[,step]") — worst case a bad
    // string throws and we return false.
    try {
        size_t pos = 0;
        out = std::stof(std::string(s), &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}
} // namespace

std::vector<std::string> parse_enum_hint(std::string_view hint) {
    std::vector<std::string> out;
    std::size_t start = 0;
    while (start <= hint.size()) {
        auto comma = hint.find(',', start);
        auto end   = (comma == std::string_view::npos) ? hint.size() : comma;
        auto tok   = hint.substr(start, end - start);
        // Trim surrounding spaces so "Box, Sphere" works as well as "Box,Sphere".
        while (!tok.empty() && tok.front() == ' ') tok.remove_prefix(1);
        while (!tok.empty() && tok.back() == ' ')  tok.remove_suffix(1);
        if (!tok.empty()) out.emplace_back(tok);
        if (comma == std::string_view::npos) break;
        start = comma + 1;
    }
    return out;
}

long long enum_value(const void* field, std::size_t size) {
    // Scoped enums are read through their actual width. Treating a 1-byte
    // enum as an int would read three bytes of neighbouring members.
    switch (size) {
        case 1: return static_cast<long long>(*static_cast<const std::uint8_t*>(field));
        case 2: return static_cast<long long>(*static_cast<const std::uint16_t*>(field));
        case 4: return static_cast<long long>(*static_cast<const std::uint32_t*>(field));
        case 8: return static_cast<long long>(*static_cast<const std::uint64_t*>(field));
        default: return 0;
    }
}

void set_enum_value(void* field, std::size_t size, long long value) {
    switch (size) {
        case 1: *static_cast<std::uint8_t*>(field)  = static_cast<std::uint8_t>(value);  break;
        case 2: *static_cast<std::uint16_t*>(field) = static_cast<std::uint16_t>(value); break;
        case 4: *static_cast<std::uint32_t*>(field) = static_cast<std::uint32_t>(value); break;
        case 8: *static_cast<std::uint64_t*>(field) = static_cast<std::uint64_t>(value); break;
        default: break;
    }
}

bool parse_range_hint(std::string_view hint, float& min, float& max, float& step) {
    auto comma1 = hint.find(',');
    if (comma1 == std::string_view::npos) return false;

    if (!parse_float(hint.substr(0, comma1), min)) return false;

    auto rest = hint.substr(comma1 + 1);
    auto comma2 = rest.find(',');
    if (comma2 == std::string_view::npos) {
        // "min,max" — step defaults to 0.01, matching Godot's convention.
        step = 0.01f;
        return parse_float(rest, max);
    }

    if (!parse_float(rest.substr(0, comma2), max)) return false;
    return parse_float(rest.substr(comma2 + 1), step);
}

} // namespace Mist
