// godot_lua_loader.h - Project-layer Lua module resolver for the BlueprintRuntime
// GDExtension.
//
// Implements the "where to find a Lua module" policy in the HOST (this project),
// not in the engine. The engine only knows "ask the host for module source".
// This resolver maps a Lua module name like "ui.FGUIMan" to a Godot resource
// path "<root>/ui/FGUIMan.lua" and reads it through Godot's FileAccess, so
// `require` works on desktop, WebGL and packed/mini-game builds with no preload.

#ifndef GODOT_LUA_LOADER_H
#define GODOT_LUA_LOADER_H

#include <godot_cpp/variant/packed_string_array.hpp>

// Register the Godot-backed Lua module resolver. PROCESS-LEVEL: call once at
// startup (e.g. from GameLauncher). `runner` may be nullptr to target the
// process-default shared Lua engine — recommended. `roots` is an ordered list
// of Godot path prefixes to try, e.g. ["res://lua", "res://blueprints"];
// the first existing file wins.
void install_godot_lua_loader(void *runner, const godot::PackedStringArray &roots);

#endif // GODOT_LUA_LOADER_H
