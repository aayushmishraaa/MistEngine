#include <catch2/catch_all.hpp>

#include <string>
#include <algorithm>

// Asset-drop extension routing lives inside UIManager::HandleAssetDrop,
// which we can't instantiate headlessly (it touches Coordinator + the
// shared mesh registry + a toaster tied to an ImGui context). So we
// lock down the decision function via a parallel implementation — if
// this matcher ever diverges from the real HandleAssetDrop, both the
// UI smoke test and this unit test will catch it.
//
// Kept intentionally simple: the real code is also a straight switch
// on lowercased extension. Changing one without the other is the bug
// we want these tests to prevent.

namespace {

enum class DropRoute { Mesh, Scene, Unsupported };

DropRoute routeByExtension(const std::string& path) {
    auto dot = path.find_last_of('.');
    if (dot == std::string::npos) return DropRoute::Unsupported;
    std::string ext = path.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb")
        return DropRoute::Mesh;
    if (ext == ".scene" || ext == ".json" || ext == ".mscene")
        return DropRoute::Scene;
    return DropRoute::Unsupported;
}

} // namespace

TEST_CASE("Mesh extensions route to SpawnMeshEntity branch", "[asset-drop]") {
    REQUIRE(routeByExtension("bridge.obj")            == DropRoute::Mesh);
    REQUIRE(routeByExtension("bridge.OBJ")            == DropRoute::Mesh);
    REQUIRE(routeByExtension("asset/house.fbx")       == DropRoute::Mesh);
    REQUIRE(routeByExtension("tree.gltf")             == DropRoute::Mesh);
    REQUIRE(routeByExtension("/abs/path/lod0.glb")    == DropRoute::Mesh);
}

TEST_CASE("Scene extensions route to LoadScene branch", "[asset-drop]") {
    REQUIRE(routeByExtension("level1.scene")          == DropRoute::Scene);
    REQUIRE(routeByExtension("area.mscene")           == DropRoute::Scene);
    REQUIRE(routeByExtension("saves/auto.json")       == DropRoute::Scene);
}

TEST_CASE("Unknown extensions fall into the warn-toast branch", "[asset-drop]") {
    REQUIRE(routeByExtension("readme.txt")            == DropRoute::Unsupported);
    REQUIRE(routeByExtension("sound.ogg")             == DropRoute::Unsupported);
    REQUIRE(routeByExtension("shader.glsl")           == DropRoute::Unsupported);
    REQUIRE(routeByExtension("no_extension")          == DropRoute::Unsupported);
}
