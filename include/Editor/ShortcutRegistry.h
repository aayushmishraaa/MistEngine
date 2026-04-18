#pragma once
#ifndef MIST_SHORTCUT_REGISTRY_H
#define MIST_SHORTCUT_REGISTRY_H

#include <string>
#include <unordered_map>
#include <vector>

// Keyboard shortcut registry patterned after Godot's ED_SHORTCUT macro.
// Every bindable action has a string id (e.g. "editor/save_scene"),
// a human-readable label ("Save Scene"), and a chord (key + mods).
// Input code dispatches by matching the chord against the registry,
// menus render the chord label auto-populated from the same table.
//
// The registry is process-global (singleton) — every editor that
// exists is driven by one consistent shortcut map. Rebinding later
// flows through `Register` with the same id ("last writer wins"),
// which in turn updates every displayed menu label.
namespace Mist::Editor {

struct Shortcut {
    std::string id;     // "editor/save_scene"
    std::string label;  // "Save Scene"
    int         key;    // GLFW_KEY_*
    int         mods;   // bitmask of GLFW_MOD_*

    // Returns a human-readable chord string like "Ctrl+S" or
    // "Ctrl+Shift+Z" suitable for ImGui::MenuItem's shortcut column.
    std::string DisplayText() const;
};

class ShortcutRegistry {
public:
    static ShortcutRegistry& Instance();

    // Register (or re-register; last writer wins) a shortcut by id.
    // Re-registering with the same id updates the chord in place,
    // which is how runtime rebinding will work later.
    void Register(const Shortcut& s);

    // Lookup by id. Returns nullptr if unknown.
    const Shortcut* Find(const std::string& id) const;

    // Returns true if (key, mods) matches the chord registered for
    // `id`. Mod-match is strict: exact equality so Ctrl+S doesn't
    // fire on Ctrl+Shift+S.
    bool Matches(const std::string& id, int key, int mods) const;

    // Find the shortcut whose chord matches this event (or nullptr).
    // Used by the top-level key handler to dispatch the matching id.
    const Shortcut* FindByChord(int key, int mods) const;

    // All registered shortcuts in insertion order. Lets menus /
    // preferences panels enumerate the full table.
    const std::vector<Shortcut>& All() const { return m_Ordered; }

private:
    ShortcutRegistry() = default;
    std::unordered_map<std::string, std::size_t> m_IdToIndex;
    std::vector<Shortcut>                        m_Ordered;
};

} // namespace Mist::Editor

#endif // MIST_SHORTCUT_REGISTRY_H
