// godot_file_bridge.h - Routes BlueprintRuntime file reads through Godot's
// FileAccess, so blueprint dependency loading works identically on:
//   - desktop (res:// in editor / OS path in export)
//   - WebGL / HTML5 export (virtual FS, no real disk)
//   - packed .pck builds and mini-game style hosts
//
// Installed once at startup via install_godot_file_reader(). It calls the
// engine's C-ABI BP_SetFileReader (added to BlueprintCAPI), passing plain C
// function pointers — no C++ ABI crosses the DLL boundary.

#ifndef GODOT_FILE_BRIDGE_H
#define GODOT_FILE_BRIDGE_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

// Install the Godot FileAccess-backed reader as BlueprintRuntime's file system.
// `res_root` is a Godot path prefix (e.g. "res://") prepended to relative paths
// the engine asks for, so a blueprint dependency like "blueprint_1_1.bjson"
// resolves to "res://blueprints/blueprint_1_1.bjson". Pass an empty string to
// treat incoming paths as already fully-qualified Godot/OS paths.
void install_godot_file_reader(const char *res_root);

// Restore BlueprintRuntime's built-in file system (raw ifstream). Mostly for
// teardown / tests.
void uninstall_godot_file_reader();

#endif // GODOT_FILE_BRIDGE_H
