#include "Input/InputMapSerializer.h"

#include "Core/Logger.h"
#include "Core/PathGuard.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

using json = nlohmann::json;

namespace Mist::Input {

namespace {

constexpr const char* kVersion = "1.0";

// Same project-root boundary every other asset path goes through.
std::filesystem::path ResolveMapPath(const std::string& path) {
    return Mist::PathGuard::resolve_under(Mist::PathGuard::project_root(), path);
}

} // namespace

bool SaveInputMap(const InputContextMap& ctx, const std::string& path) {
    const auto resolved = ResolveMapPath(path);
    if (resolved.empty()) {
        LOG_ERROR("InputMap: refusing to write outside the project root: ", path);
        return false;
    }

    json doc = {
        {"type",    "MistInputMap"},
        {"version", kVersion},
        {"context", ctx.name},
        {"actions", json::object()},
    };

    for (const auto& [name, action] : ctx.actions) {
        json bindings = json::array();
        for (const auto& b : action.bindings) {
            bindings.push_back(json{
                {"device", static_cast<int>(b.device)},
                {"code",   b.code},
                {"scale",  b.scale},
            });
        }
        doc["actions"][name] = json{
            {"type",     static_cast<int>(action.type)},
            {"bindings", std::move(bindings)},
        };
    }

    std::error_code ec;
    std::filesystem::create_directories(resolved.parent_path(), ec);

    std::ofstream out(resolved, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        LOG_ERROR("InputMap: failed to open for writing: ", resolved.string());
        return false;
    }
    out << doc.dump(2);
    LOG_INFO("Input map saved: ", resolved.string());
    return true;
}

bool LoadInputMap(InputContextMap& ctx, const std::string& path) {
    const auto resolved = ResolveMapPath(path);
    if (resolved.empty()) {
        LOG_ERROR("InputMap: refusing to read outside the project root: ", path);
        return false;
    }
    std::error_code ec;
    if (!std::filesystem::exists(resolved, ec)) return false;  // no saved map yet

    std::ifstream in(resolved, std::ios::binary);
    if (!in.is_open()) {
        LOG_ERROR("InputMap: failed to open for reading: ", resolved.string());
        return false;
    }

    json doc;
    try { in >> doc; }
    catch (const std::exception& e) {
        LOG_ERROR("InputMap: parse failed (", e.what(), "); keeping built-in bindings");
        return false;
    }
    if (!doc.is_object() || !doc.contains("actions") || !doc["actions"].is_object()) {
        LOG_ERROR("InputMap: invalid document; keeping built-in bindings");
        return false;
    }

    int merged = 0;
    for (auto it = doc["actions"].begin(); it != doc["actions"].end(); ++it) {
        const auto& je = it.value();
        if (!je.is_object() || !je.contains("bindings") || !je["bindings"].is_array()) continue;

        InputAction action;
        action.name = it.key();
        action.type = static_cast<InputActionType>(je.value("type", 0));

        for (const auto& jb : je["bindings"]) {
            if (!jb.is_object()) continue;
            InputBinding b;
            b.device = static_cast<InputDevice>(jb.value("device", 0));
            b.code   = jb.value("code", 0);
            b.scale  = jb.value("scale", 1.0f);
            action.bindings.push_back(b);
        }

        // An action with no bindings left is a deliberate unbind, so it is
        // stored as-is rather than skipped — otherwise a user could never
        // remove a default binding.
        ctx.AddAction(action);
        ++merged;
    }

    LOG_INFO("Input map loaded: ", resolved.string(), " (", merged, " actions merged)");
    return true;
}

} // namespace Mist::Input
