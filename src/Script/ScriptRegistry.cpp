#include "Script/ScriptRegistry.h"

#include <cctype>

namespace Mist::Script {

namespace {
std::string toLower(std::string_view s) {
    std::string out(s);
    for (auto& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}
} // namespace

ScriptRegistry& ScriptRegistry::Instance() {
    static ScriptRegistry inst;
    return inst;
}

void ScriptRegistry::Register(std::shared_ptr<IScriptLanguage> language) {
    if (!language) return;
    language->Init();

    std::string key = toLower(language->GetExtension());
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Languages[std::move(key)] = std::move(language);
}

std::shared_ptr<IScriptLanguage> ScriptRegistry::Get(std::string_view extension) const {
    std::string key = toLower(extension);
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Languages.find(key);
    if (it == m_Languages.end()) return nullptr;
    return it->second;
}

void ScriptRegistry::ShutdownAll() {
    decltype(m_Languages) drained;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        drained.swap(m_Languages);
    }
    for (auto& [ext, lang] : drained) {
        lang->Shutdown();
    }
}

} // namespace Mist::Script
