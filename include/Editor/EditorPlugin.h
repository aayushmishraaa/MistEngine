#pragma once
#ifndef MIST_EDITOR_PLUGIN_H
#define MIST_EDITOR_PLUGIN_H

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace Mist::Editor {

// EditorContext — the handle a plugin uses to add extensions. Passed
// through on OnEnable + retained internally by the registry so the
// plugin's registrations persist until OnDisable.
class EditorContext {
public:
    using DockDrawFn   = std::function<void()>;
    using MenuClickFn  = std::function<void()>;

    // Register a dockable panel. The registry wires this into UIManager's
    // frame loop; the plugin doesn't have to know about ImGui layout.
    void RegisterDock(std::string_view title, DockDrawFn draw);

    // Register a menu item reachable via a slash-separated path, e.g.
    // "Tools/My Feature". Registration location (top-level vs nested) is
    // derived from the path; unrecognised prefixes fall back to "Tools".
    void RegisterMenuItem(std::string_view path, MenuClickFn onClick);

    struct DockRegistration { std::string title; DockDrawFn draw; };
    struct MenuRegistration { std::string path;  MenuClickFn onClick; };

    const std::vector<DockRegistration>& Docks()     const { return m_Docks; }
    const std::vector<MenuRegistration>& MenuItems() const { return m_Menus; }

private:
    std::vector<DockRegistration> m_Docks;
    std::vector<MenuRegistration> m_Menus;
};

// Plugins inherit this and implement OnEnable/OnDisable. GetName() is
// surfaced in the editor's Plugin panel (future G1-backed inspector) to
// let users toggle individual plugins without a rebuild.
class IEditorPlugin {
public:
    virtual ~IEditorPlugin() = default;

    virtual std::string GetName() const  = 0;
    virtual void OnEnable(EditorContext&) = 0;
    virtual void OnDisable(EditorContext&) {}
};

// Process-wide plugin registry. UIManager queries this each frame to
// iterate docks; menu items land in DrawMainMenuBar. Plugins call
// Register + Enable during startup (or from a modular hot-load path).
class EditorPluginRegistry {
public:
    static EditorPluginRegistry& Instance();

    void Register(std::shared_ptr<IEditorPlugin> plugin);
    void EnableAll();
    void DisableAll();

    // Snapshot for UIManager to iterate. Returns a copy to sidestep
    // mutex lifetime concerns on the caller side — the snapshot is
    // cheap (function pointers + small strings).
    std::vector<EditorContext::DockRegistration> SnapshotDocks() const;
    std::vector<EditorContext::MenuRegistration> SnapshotMenus() const;

    std::size_t PluginCount() const;

private:
    EditorPluginRegistry() = default;

    struct Entry {
        std::shared_ptr<IEditorPlugin> plugin;
        EditorContext                  ctx;
        bool                           enabled = false;
    };
    mutable std::mutex  m_Mutex;
    std::vector<Entry>  m_Entries;
};

} // namespace Mist::Editor

#endif // MIST_EDITOR_PLUGIN_H
