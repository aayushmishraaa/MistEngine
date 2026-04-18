#include "ECS/Systems/ScriptSystem.h"

#include "Core/Logger.h"
#include "ECS/Components/ScriptComponent.h"
#include "ECS/Systems/HierarchySystem.h"

#if MIST_ENABLE_SCRIPTING
#include "Script/LuaScriptLanguage.h"
#endif

namespace {
// Set the thread-local current entity, run `fn`, clear it. Guards against
// exceptions thrown from Lua (sol2 can throw on internal errors) so the
// tls never leaks outside a single callback.
template <typename Fn>
void WithCurrentEntity(Entity e, Fn&& fn) {
#if MIST_ENABLE_SCRIPTING
    Mist::Script::LuaScriptLanguage::SetCurrentEntity(e);
    try {
        fn();
    } catch (...) {
        Mist::Script::LuaScriptLanguage::SetCurrentEntity(static_cast<Entity>(-1));
        throw;
    }
    Mist::Script::LuaScriptLanguage::SetCurrentEntity(static_cast<Entity>(-1));
#else
    (void)e;
    fn();
#endif
}
} // namespace

void ScriptSystem::WireReadyCallback(Coordinator& coord) {
    if (m_ReadyConnection != 0) return;
    m_ReadyConnection = HierarchySystem::OnReady().Connect([this, &coord](Entity e) {
        if (!coord.HasComponent<ScriptComponent>(e)) return;
        auto& sc = coord.GetComponent<ScriptComponent>(e);
        if (sc.readyFired || !sc.instance) return;
        WithCurrentEntity(e, [&] { sc.instance->CallVoid("_ready"); });
        sc.readyFired = true;
    });
}

void ScriptSystem::UnwireReadyCallback() {
    if (m_ReadyConnection == 0) return;
    HierarchySystem::OnReady().Disconnect(m_ReadyConnection);
    m_ReadyConnection = 0;
}

void ScriptSystem::Update(Coordinator& coord, float dt) {
#if MIST_ENABLE_SCRIPTING
    Mist::Script::LuaScriptLanguage::SetLastDeltaTime(dt);
#else
    (void)dt;
#endif
    for (auto e : m_Entities) {
        if (!coord.HasComponent<ScriptComponent>(e)) continue;
        auto& sc = coord.GetComponent<ScriptComponent>(e);
        if (!sc.instance) continue;
        WithCurrentEntity(e, [&] { sc.instance->CallVoid("_process"); });
    }
}
