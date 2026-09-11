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
namespace detail {

// weakly_canonical() that works for paths which do not exist yet.
//
// libstdc++ sets the error_code for a path whose final component is missing,
// where libc++ happily returns the lexically-resolved result. is_under() took
// that as a rejection, so on Linux EVERY write of a not-yet-existing file was
// refused as "outside the project root" — scenes, materials and input maps
// alike — while the same code passed on macOS. Reads were unaffected, because
// the file was already there, which is why this hid for so long.
//
// The security property is preserved: symlinks are still resolved on the part
// of the path that exists, which is the only part that can be a symlink. The
// remainder is appended lexically, so "../.." in a missing tail is normalised
// away by lexically_normal() before the prefix comparison.
inline std::filesystem::path resolve_possibly_missing(const std::filesystem::path& p,
                                                      std::error_code& ec) {
    ec.clear();
    auto canon = std::filesystem::weakly_canonical(p, ec);
    if (!ec) return canon;

    // Walk up to the nearest ancestor that exists, canonicalise it, then
    // re-append what we walked past.
    ec.clear();
    std::filesystem::path absolute = p.is_absolute()
                                   ? p
                                   : (std::filesystem::current_path(ec) / p);
    if (ec) return {};

    std::filesystem::path existing = absolute;
    std::filesystem::path tail;
    while (!existing.empty()) {
        std::error_code exists_ec;
        if (std::filesystem::exists(existing, exists_ec) && !exists_ec) break;
        if (!existing.has_parent_path() || existing.parent_path() == existing) {
            existing.clear();
            break;
        }
        tail = existing.filename() / tail;
        existing = existing.parent_path();
    }

    if (existing.empty()) {
        ec.clear();
        return absolute.lexically_normal();
    }

    std::error_code canon_ec;
    auto canon_existing = std::filesystem::canonical(existing, canon_ec);
    if (canon_ec) {
        ec.clear();
        return absolute.lexically_normal();
    }

    ec.clear();
    return (canon_existing / tail).lexically_normal();
}

} // namespace detail

inline bool is_under(const std::filesystem::path& base, const std::filesystem::path& candidate,
                     std::filesystem::path* canonical_out = nullptr) {
    std::error_code ec;
    auto canon_base = detail::resolve_possibly_missing(base, ec);
    if (ec || canon_base.empty()) {
        return false;
    }
    auto canon_cand = detail::resolve_possibly_missing(candidate, ec);
    if (ec || canon_cand.empty()) {
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
