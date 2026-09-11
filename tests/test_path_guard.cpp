#include "Core/PathGuard.h"

#include <catch2/catch_all.hpp>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace {
fs::path make_temp_sandbox(const char* label) {
    fs::path root = fs::temp_directory_path() / (std::string("mist-test-") + label);
    fs::remove_all(root);
    fs::create_directories(root / "sub");
    std::ofstream(root / "sub" / "a.mist").put('x');
    return root;
}
} // namespace

TEST_CASE("PathGuard::is_under accepts paths inside the sandbox", "[path_guard]") {
    auto root = make_temp_sandbox("accept");
    auto inside = root / "sub" / "a.mist";

    fs::path resolved;
    REQUIRE(Mist::PathGuard::is_under(root, inside, &resolved));
    REQUIRE(resolved == fs::weakly_canonical(inside));
}

TEST_CASE("PathGuard::is_under rejects parent-traversal attempts", "[path_guard]") {
    auto root = make_temp_sandbox("traverse");
    // ../ back out of the sandbox. weakly_canonical should resolve this and
    // fail the prefix comparison.
    auto escape = root / "sub" / ".." / ".." / ".." / "etc" / "passwd";

    fs::path resolved;
    REQUIRE_FALSE(Mist::PathGuard::is_under(root, escape, &resolved));
}

TEST_CASE("PathGuard::is_under rejects siblings that share a prefix", "[path_guard]") {
    // "/foo/barbaz" shouldn't match a sandbox of "/foo/bar" just because the
    // string starts the same way. The trailing-separator check matters.
    fs::path root = fs::temp_directory_path() / "mist-prefix" / "bar";
    fs::path sibling = fs::temp_directory_path() / "mist-prefix" / "barbaz" / "x";
    fs::create_directories(sibling.parent_path());

    fs::path resolved;
    REQUIRE_FALSE(Mist::PathGuard::is_under(root, sibling, &resolved));
}

namespace {
// PathGuard::project_root() is a process-wide function-local static. These
// cases have to repoint it, and before this guard existed they never put it
// back — so every test that ran afterwards resolved project paths against a
// temp directory. That is what made tests/test_material_asset.cpp fail under
// Catch2's randomised order while passing in declaration order, and it later
// broke the prefab scene tests the same way.
struct ScopedProjectRoot {
    fs::path previous;
    explicit ScopedProjectRoot(const fs::path& next)
        : previous(Mist::PathGuard::project_root()) {
        Mist::PathGuard::set_project_root(next);
    }
    ~ScopedProjectRoot() { Mist::PathGuard::set_project_root(previous); }
};
} // namespace

TEST_CASE("PathGuard::resolve_res_path resolves under the project root", "[path_guard][res]") {
    fs::path root = fs::temp_directory_path() / "mist-res-root";
    fs::create_directories(root / "meshes");
    ScopedProjectRoot scoped(root);

    auto out = Mist::PathGuard::resolve_res_path("res://meshes/cube.mesh");
    REQUIRE(!out.empty());
    // Resolved path should share the canonicalised root as a prefix.
    REQUIRE(out.generic_string().find(fs::weakly_canonical(root).generic_string())
            != std::string::npos);
}

TEST_CASE("PathGuard::resolve_res_path rejects traversal", "[path_guard][res][security]") {
    fs::path root = fs::temp_directory_path() / "mist-res-escape";
    fs::create_directories(root);
    ScopedProjectRoot scoped(root);

    // Classic escape attempt — should not resolve.
    auto out = Mist::PathGuard::resolve_res_path("res://../../etc/passwd");
    REQUIRE(out.empty());
}

TEST_CASE("PathGuard::resolve_res_path returns empty for non-res scheme", "[path_guard][res]") {
    auto a = Mist::PathGuard::resolve_res_path("/abs/path");
    auto b = Mist::PathGuard::resolve_res_path("file://whatever");
    auto c = Mist::PathGuard::resolve_res_path("");
    REQUIRE(a.empty());
    REQUIRE(b.empty());
    REQUIRE(c.empty());
}

// --- Not-yet-existing paths -------------------------------------------------
//
// The case that made CI red on Linux while passing on macOS.
//
// is_under() used std::filesystem::weakly_canonical(p, ec) directly. libstdc++
// sets the error_code when the final component does not exist; libc++ returns
// the lexically-resolved path instead. is_under treated that as a rejection, so
// on Linux every write of a NEW file was refused as "outside the project root"
// — scene saves, material saves and input-map saves alike — while reads passed
// because the file already existed. Seven tests failed on Linux and none on
// macOS.

TEST_CASE("is_under accepts a file that does not exist yet", "[path_guard][security]") {
    fs::path root = fs::temp_directory_path() / "mist-missing-target";
    fs::create_directories(root);

    const fs::path fresh = root / "not_created_yet.json";
    std::error_code ec;
    fs::remove(fresh, ec);
    REQUIRE_FALSE(fs::exists(fresh));

    fs::path resolved;
    REQUIRE(Mist::PathGuard::is_under(root, fresh, &resolved));
    REQUIRE(resolved.filename() == "not_created_yet.json");
}

TEST_CASE("is_under accepts a nested path whose directories do not exist yet",
          "[path_guard][security]") {
    // Serializers call create_directories() *after* the guard, so the guard has
    // to accept a path several missing levels deep.
    fs::path root = fs::temp_directory_path() / "mist-missing-nested";
    fs::create_directories(root);

    fs::path resolved;
    REQUIRE(Mist::PathGuard::is_under(root, root / "a" / "b" / "c.mistprefab", &resolved));
}

TEST_CASE("A missing path still cannot escape the base", "[path_guard][security]") {
    // The fix must not have traded correctness for permissiveness: traversal in
    // a non-existent tail is normalised away before the prefix compare.
    fs::path root = fs::temp_directory_path() / "mist-missing-escape";
    fs::create_directories(root);

    REQUIRE_FALSE(Mist::PathGuard::is_under(root, root / ".." / ".." / "escaped.json"));
    REQUIRE_FALSE(Mist::PathGuard::is_under(root, fs::path("/etc/definitely_not_here.json")));
}

TEST_CASE("resolve_under returns a usable path for a new file", "[path_guard]") {
    // What every serializer's Save() actually depends on.
    fs::path root = fs::temp_directory_path() / "mist-resolve-new";
    fs::create_directories(root);

    const auto out = Mist::PathGuard::resolve_under(root, root / "brand_new.mistmat");
    REQUIRE_FALSE(out.empty());
    REQUIRE(out.filename() == "brand_new.mistmat");
}
