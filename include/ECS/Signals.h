#pragma once
#ifndef MIST_ECS_SIGNALS_H
#define MIST_ECS_SIGNALS_H

// Engine-wide lifecycle signals.
//
// Mist::Signal<T> is solid — deferred connect/disconnect while emitting, and
// dispatch outside the lock — and exactly one instance existed engine-wide
// (HierarchySystem::OnReady). Everything else that wanted to know when
// something happened polled for it instead, which is why the editor rebuilds
// its entity list every frame.
//
// These are function-local statics rather than members so any translation unit
// can connect without a pointer to whoever emits. Same shape OnReady already
// uses.

#include "Core/Signal.h"
#include "ECS/Entity.h"

namespace Mist::Events {

// Emitted after the entity exists and its initial components are attached, so
// a handler can inspect it.
Signal<Entity>& OnEntityCreated();

// Emitted BEFORE the entity is destroyed, while its components are still
// readable — a handler that needs to know what it was has no other chance.
Signal<Entity>& OnEntityDestroyed();

// Editor selection. Emits kInvalidEntity-equivalent (Entity(-1)) on deselect.
Signal<Entity>& OnSelectionChanged();

// Emitted after a scene finishes loading and the world is fully rebuilt.
Signal<>& OnSceneLoaded();

} // namespace Mist::Events

#endif // MIST_ECS_SIGNALS_H
