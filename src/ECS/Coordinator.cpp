#include "ECS/Coordinator.h"

// Single definition of the global coordinator. Previously this lived in
// MistEngine.cpp (the executable entry point), which meant the static library
// had unresolved references to gCoordinator and any non-main consumer (tests,
// hosts, modules) had to supply its own definition.
Coordinator gCoordinator;
