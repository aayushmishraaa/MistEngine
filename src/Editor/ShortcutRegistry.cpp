#include "Editor/ShortcutRegistry.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace Mist::Editor {

namespace {

// GLFW key → display string. Covers the keys we actually bind today;
// unmapped keys fall through to "Key<n>" so adding a new binding never
// crashes a menu render.
const char* keyName(int key) {
    switch (key) {
        case GLFW_KEY_A: return "A";
        case GLFW_KEY_C: return "C";
        case GLFW_KEY_D: return "D";
        case GLFW_KEY_E: return "E";
        case GLFW_KEY_N: return "N";
        case GLFW_KEY_O: return "O";
        case GLFW_KEY_Q: return "Q";
        case GLFW_KEY_R: return "R";
        case GLFW_KEY_S: return "S";
        case GLFW_KEY_T: return "T";
        case GLFW_KEY_V: return "V";
        case GLFW_KEY_W: return "W";
        case GLFW_KEY_X: return "X";
        case GLFW_KEY_Y: return "Y";
        case GLFW_KEY_Z: return "Z";
        case GLFW_KEY_F1: return "F1";
        case GLFW_KEY_F2: return "F2";
        case GLFW_KEY_F3: return "F3";
        case GLFW_KEY_F5: return "F5";
        case GLFW_KEY_F8: return "F8";
        case GLFW_KEY_DELETE: return "Del";
        case GLFW_KEY_ENTER:  return "Enter";
        case GLFW_KEY_ESCAPE: return "Esc";
        default: return "?";
    }
}

} // namespace

std::string Shortcut::DisplayText() const {
    std::string out;
    if (mods & GLFW_MOD_CONTROL) out += "Ctrl+";
    if (mods & GLFW_MOD_SHIFT)   out += "Shift+";
    if (mods & GLFW_MOD_ALT)     out += "Alt+";
    if (mods & GLFW_MOD_SUPER)   out += "Cmd+";
    out += keyName(key);
    return out;
}

ShortcutRegistry& ShortcutRegistry::Instance() {
    static ShortcutRegistry s;
    return s;
}

void ShortcutRegistry::Register(const Shortcut& s) {
    auto it = m_IdToIndex.find(s.id);
    if (it != m_IdToIndex.end()) {
        m_Ordered[it->second] = s;   // last writer wins — supports rebinding
        return;
    }
    m_IdToIndex[s.id] = m_Ordered.size();
    m_Ordered.push_back(s);
}

const Shortcut* ShortcutRegistry::Find(const std::string& id) const {
    auto it = m_IdToIndex.find(id);
    if (it == m_IdToIndex.end()) return nullptr;
    return &m_Ordered[it->second];
}

bool ShortcutRegistry::Matches(const std::string& id, int key, int mods) const {
    const Shortcut* s = Find(id);
    if (!s) return false;
    // Strict mod equality: pressing Ctrl+Shift+S must not fire Ctrl+S.
    return s->key == key && s->mods == mods;
}

const Shortcut* ShortcutRegistry::FindByChord(int key, int mods) const {
    for (const auto& s : m_Ordered) {
        if (s.key == key && s.mods == mods) return &s;
    }
    return nullptr;
}

} // namespace Mist::Editor
