#include "ECS/Signals.h"

namespace Mist::Events {

// Function-local statics: initialised on first use, and no ordering dependency
// between translation units at static-init time.

Signal<Entity>& OnEntityCreated() {
    static Signal<Entity> sig;
    return sig;
}

Signal<Entity>& OnEntityDestroyed() {
    static Signal<Entity> sig;
    return sig;
}

Signal<Entity>& OnSelectionChanged() {
    static Signal<Entity> sig;
    return sig;
}

Signal<>& OnSceneLoaded() {
    static Signal<> sig;
    return sig;
}

} // namespace Mist::Events
