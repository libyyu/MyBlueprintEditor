// game_launcher.h - Process-level bootstrap for the BlueprintRuntime host.
//
// GameLauncher is meant to be an autoload (singleton) in the Godot project. It
// owns everything that must be set up ONCE per process, independent of how many
// blueprints or BlueprintNodes exist:
//
//   - the Godot FileAccess file reader  (BP_SetFileReader)
//   - the Godot Lua module resolver / require searcher  (BP_SetLuaModuleResolver)
//
// This is the Godot equivalent of Unity's GameLauncher four-phase startup, but
// far simpler: Godot's resource system replaces YooAsset init, and the Lua
// searcher replaces the "preload all Lua" phase (modules load on demand).
//
// BlueprintNode no longer touches process-level setup; it just loads & runs a
// single blueprint, relying on GameLauncher having configured the environment.

#ifndef GAME_LAUNCHER_H
#define GAME_LAUNCHER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot {

class BlueprintNode; // fwd

class GameLauncher : public Node {
    GDCLASS(GameLauncher, Node)

private:
    bool _auto_setup = true;          // run setup() automatically in _ready
    bool _did_setup = false;          // process-level guard
    BlueprintNode *_update_node = nullptr; // dedicated update-phase runner
#ifdef BLUEPRINT_HAS_LUA
    PackedStringArray _lua_roots;     // require search roots, e.g. ["res://lua"]
    void install_resolver();          // (re)bind Lua require resolver to current VM
#endif
protected:
    static void _bind_methods();

public:
    GameLauncher();

    void _ready() override;
    void _process(double delta) override;

    // Idempotent process-level initialization (file reader + Lua resolver).
    // Safe to call multiple times; only the first call has effect.
    bool setup();

    // --- Two-phase startup (update VM -> game VM) ---
    // Use when the UPDATE phase logic/UI is also written in Lua and must run on
    // an isolated VM that is discarded before the game VM loads updated code.
    //
    // Phase A: ensure setup() has run (env up), then create a dedicated update
    //   runner whose Lua VM is used only for the update phase.
    //   Returns the update BlueprintNode; load your update blueprint on it.
    BlueprintNode *begin_update_phase();

    // Phase B: tear down the update VM. Destroys the update runner and closes
    //   its lua_State, so the next runner builds a fresh VM. Call after the
    //   update blueprint signals completion.
    void end_update_phase();

    // Whether an update phase is currently active.
    bool in_update_phase() const { return _update_node != nullptr; }

    // Whether process-level setup has completed.
    bool is_ready() const { return _did_setup; }

#ifdef BLUEPRINT_HAS_LUA
    // Register the project-layer Scene.*/UI.* Lua tables onto the given node's
    // Lua VM (via BP_GetLuaState). Call after the node has loaded a blueprint.
    // Lets Lua scripts drive Godot SceneService/UiService. Engine untouched.
    void bind_lua_api(BlueprintNode *node);

    // Configure Lua require roots BEFORE setup() (or call setup() again is a no-op).
    void set_lua_roots(const PackedStringArray &roots);
    PackedStringArray get_lua_roots() const;

#endif

    void set_auto_setup(bool v);
    bool get_auto_setup() const;
};

} // namespace godot

#endif // GAME_LAUNCHER_H
