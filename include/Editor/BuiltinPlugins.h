#pragma once
#ifndef MIST_BUILTIN_PLUGINS_H
#define MIST_BUILTIN_PLUGINS_H

// Registration entry point for plugins that ship with the engine.
//
// Called once from main(), before EditorPluginRegistry::EnableAll(). Kept as a
// single function so adding a built-in plugin touches one file rather than
// growing main().
namespace Mist::Editor::Plugins {
void RegisterBuiltinPlugins();
}

#endif // MIST_BUILTIN_PLUGINS_H
