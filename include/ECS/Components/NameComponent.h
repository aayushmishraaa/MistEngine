#ifndef NAMECOMPONENT_H
#define NAMECOMPONENT_H

#include "Core/Reflection.h"

#include <string>

// Human-readable entity name — Godot's Node::name, moved out of the editor
// and into the engine.
//
// Before this existed, names lived in `UIManager::m_EntityNames`, a private
// std::unordered_map<Entity,std::string> owned by the *editor*. Three things
// followed from that, all of which read as bugs:
//
//   1. Names were never serialized. SceneSerializer.h's own format example
//      documented a "name" field the Save path never wrote, so renaming an
//      entity and saving silently discarded the rename — on reload every
//      entity was "Entity 7" again.
//   2. Nothing outside the editor could address an entity. A Lua script had
//      `entity_id()` for *self* and no way to reach anything else.
//   3. A prefab override has to name the child it overrides, so the whole
//      prefab system was blocked on a std::string living in the wrong class.
//
// Names are NOT required to be unique here. Godot enforces sibling-uniqueness
// at the scene-tree level; MistEngine enforces it only where it matters —
// inside a prefab, where a duplicate name would make an override ambiguous.
struct NameComponent {
    std::string name;
};

MIST_REFLECT(NameComponent)
    MIST_FIELD(NameComponent, name, ::Mist::PropertyHint::None, "")
MIST_REFLECT_END(NameComponent)

#endif // NAMECOMPONENT_H
