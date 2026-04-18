#include "Editor/EditorPlugin.h"

#include <utility>

namespace Mist::Editor {

// --- EditorContext ---

void EditorContext::RegisterDock(std::string_view title, DockDrawFn draw) {
    m_Docks.push_back({std::string(title), std::move(draw)});
}

void EditorContext::RegisterMenuItem(std::string_view path, MenuClickFn onClick) {
    m_Menus.push_back({std::string(path), std::move(onClick)});
}

// --- EditorPluginRegistry ---

EditorPluginRegistry& EditorPluginRegistry::Instance() {
    static EditorPluginRegistry inst;
    return inst;
}

void EditorPluginRegistry::Register(std::shared_ptr<IEditorPlugin> plugin) {
    if (!plugin) return;
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Entries.push_back({std::move(plugin), EditorContext{}, false});
}

void EditorPluginRegistry::EnableAll() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (auto& e : m_Entries) {
        if (!e.enabled) {
            e.plugin->OnEnable(e.ctx);
            e.enabled = true;
        }
    }
}

void EditorPluginRegistry::DisableAll() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (auto& e : m_Entries) {
        if (e.enabled) {
            e.plugin->OnDisable(e.ctx);
            e.enabled = false;
        }
    }
}

std::vector<EditorContext::DockRegistration> EditorPluginRegistry::SnapshotDocks() const {
    std::vector<EditorContext::DockRegistration> out;
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (const auto& e : m_Entries) {
        if (!e.enabled) continue;
        for (const auto& d : e.ctx.Docks()) out.push_back(d);
    }
    return out;
}

std::vector<EditorContext::MenuRegistration> EditorPluginRegistry::SnapshotMenus() const {
    std::vector<EditorContext::MenuRegistration> out;
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (const auto& e : m_Entries) {
        if (!e.enabled) continue;
        for (const auto& m : e.ctx.MenuItems()) out.push_back(m);
    }
    return out;
}

std::size_t EditorPluginRegistry::PluginCount() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Entries.size();
}

} // namespace Mist::Editor
