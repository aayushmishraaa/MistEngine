#pragma once

#include <filesystem>
#include <string>
#include <system_error>

namespace Mist::PathGuard {

// Resolve a candidate path and verify it stays under `base` after `..` and
// symlink resolution. Returns the canonical form on success.
//
// Use at every boundary where a path originates from a user, a config file,
// or a scene/module file — it prevents path-traversal (`../../etc/passwd`)
// and symlink-escape attacks against any directory we treat as a sandbox
// (scenes/, modules/, exports/, asset browser roots).
inline bool is_under(const std::filesystem::path& base, const std::filesystem::path& candidate,
                     std::filesystem::path* canonical_out = nullptr) {
    std::error_code ec;
    auto canon_base = std::filesystem::weakly_canonical(base, ec);
    if (ec) {
        return false;
    }
    auto canon_cand = std::filesystem::weakly_canonical(candidate, ec);
    if (ec) {
        return false;
    }

    const auto base_str = canon_base.lexically_normal().generic_string();
    const auto cand_str = canon_cand.lexically_normal().generic_string();

    if (cand_str.size() < base_str.size()) {
        return false;
    }
    if (cand_str.compare(0, base_str.size(), base_str) != 0) {
        return false;
    }
    // Ensure the next char is a separator or end-of-string so that
    // "/foo/bar" doesn't match "/foo/barbaz".
    if (cand_str.size() > base_str.size() && cand_str[base_str.size()] != '/') {
        return false;
    }

    if (canonical_out) {
        *canonical_out = canon_cand;
    }
    return true;
}

// Convenience wrapper returning the resolved path or an empty path on failure.
inline std::filesystem::path resolve_under(const std::filesystem::path& base,
                                           const std::filesystem::path& candidate) {
    std::filesystem::path resolved;
    if (!is_under(base, candidate, &resolved)) {
        return {};
    }
    return resolved;
}

// Project-root ("res://") support. Pattern borrowed from Godot's res:// virtual
// filesystem: any asset path that starts with "res://" resolves against a
// single, explicit project root rather than the process CWD. This lets
// scene files reference assets portably regardless of where the binary is
// launched from, and still runs everything through is_under() so a scene
// can't sneak "res://../../etc/passwd" past us.
//
// Root is set once at engine startup via set_project_root() and is
// typically the current working directory at launch.
namespace detail {
inline std::filesystem::path& project_root_storage() {
    static std::filesystem::path root = std::filesystem::current_path();
    return root;
}
} // namespace detail

inline void set_project_root(const std::filesystem::path& root) {
    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(root, ec);
    detail::project_root_storage() = ec ? root : canonical;
}

inline const std::filesystem::path& project_root() {
    return detail::project_root_storage();
}

// Resolve "res://subdir/file.ext" to an absolute filesystem path under the
// project root. Returns an empty path if the input doesn't start with the
// prefix or if the resolved path escapes the root.
inline std::filesystem::path resolve_res_path(std::string_view uri) {
    constexpr std::string_view kPrefix = "res://";
    if (uri.size() < kPrefix.size() || uri.substr(0, kPrefix.size()) != kPrefix) {
        return {};
    }
    std::filesystem::path relative(uri.substr(kPrefix.size()));
    std::filesystem::path candidate = project_root() / relative;
    return resolve_under(project_root(), candidate);
}

} // namespace Mist::PathGuard
