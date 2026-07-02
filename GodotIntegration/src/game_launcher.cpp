// game_launcher.cpp - process-level bootstrap implementation.

#include "game_launcher.h"
#include "blueprint_node.h"
#include "godot_file_bridge.h"
#include "godot_lua_loader.h"
#ifdef BLUEPRINT_HAS_LUA
#include "godot_lua_bindings.h"
#endif

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include "BlueprintCAPI.h"
}

using namespace godot;

GameLauncher::GameLauncher() {
#ifdef BLUEPRINT_HAS_LUA
    // Sensible default require root.
    _lua_roots.append("res://lua");
#endif
}

void GameLauncher::_ready() {
    if (Engine::get_singleton()->is_editor_hint()) return;
    // setup() is ALWAYS needed (both the update phase and the game phase rely on
    // the environment it installs), so run it as early as possible. auto_setup
    // only controls whether we do it here in _ready or let Boot.gd do it first;
    // either way it runs before anything loads a blueprint.
    if (_auto_setup) setup();
}
#ifdef BLUEPRINT_HAS_LUA
void GameLauncher::install_resolver() {
    // (Re)bind the host Lua require resolver to the CURRENT default shared VM.
    // Called by setup() and again after BP_ResetSharedLuaVM() so the freshly
    // created game VM gets the resolver. install_godot_lua_loader(nullptr, ...)
    // targets the process-default VM (created on demand).
    install_godot_lua_loader(nullptr, _lua_roots);
}
#endif

bool GameLauncher::setup() {
    if (_did_setup) return true;
    // Process-level environment — needed by EVERY phase (update + game):
    BP_InitDefaultHttpClient();        // LLM/http nodes
    install_godot_file_reader("");     // all file reads -> Godot FileAccess
#ifdef BLUEPRINT_HAS_LUA
    install_resolver();                // require -> res://lua/...
#endif
    _did_setup = true;
    UtilityFunctions::print("[GameLauncher] setup complete (file reader + lua resolver)");
    return true;
}

BlueprintNode *GameLauncher::begin_update_phase() {
    setup();                           // guarantee env is up (idempotent)
    if (_update_node) return _update_node; // already running
    _update_node = memnew(BlueprintNode);
    _update_node->set_autorun(false);
    add_child(_update_node);
    UtilityFunctions::print("[GameLauncher] update phase begun (isolated VM)");
    return _update_node;
}

void GameLauncher::end_update_phase() {
    if (_update_node) {
        // Destroy the update runner FIRST so its shared_ptr no longer keeps the
        // old VM alive, then close the old lua_State.
        _update_node->queue_free();
        _update_node = nullptr;
    }
    BP_ResetSharedLuaVM();   // discard the update VM
#ifdef BLUEPRINT_HAS_LUA
    install_resolver();      // re-bind resolver to the fresh game VM (created on demand)
#endif
    UtilityFunctions::print("[GameLauncher] update phase ended; fresh game VM ready");
}

void GameLauncher::bind_lua_api(BlueprintNode *node) {
    if (node == nullptr || node->runner_ptr() == nullptr) {
        UtilityFunctions::printerr("[GameLauncher] bind_lua_api: node has no runner (load a blueprint first)");
        return;
    }
#ifdef BLUEPRINT_HAS_LUA
    // Use the node itself as tree context: it can reach /root/ autoloads
    // (SceneService/UiService) and stays valid in the current scene, whereas
    // this GameLauncher may live in a Boot scene already replaced by change_scene.
    register_godot_lua_bindings(node->runner_ptr(), node);
#else
    UtilityFunctions::printerr("[GameLauncher] bind_lua_api: built without Lua (GDEXT_WITH_LUA=OFF)");
#endif
}
#ifdef BLUEPRINT_HAS_LUA
void GameLauncher::set_lua_roots(const PackedStringArray &roots) { _lua_roots = roots; }
PackedStringArray GameLauncher::get_lua_roots() const { return _lua_roots; }
#endif
void GameLauncher::set_auto_setup(bool v) { _auto_setup = v; }
bool GameLauncher::get_auto_setup() const { return _auto_setup; }

void GameLauncher::_bind_methods() {
    ClassDB::bind_method(D_METHOD("setup"), &GameLauncher::setup);
    ClassDB::bind_method(D_METHOD("is_ready"), &GameLauncher::is_ready);
    ClassDB::bind_method(D_METHOD("begin_update_phase"), &GameLauncher::begin_update_phase);
    ClassDB::bind_method(D_METHOD("end_update_phase"), &GameLauncher::end_update_phase);
    ClassDB::bind_method(D_METHOD("in_update_phase"), &GameLauncher::in_update_phase);
#ifdef BLUEPRINT_HAS_LUA
    ClassDB::bind_method(D_METHOD("bind_lua_api", "node"), &GameLauncher::bind_lua_api);
    ClassDB::bind_method(D_METHOD("set_lua_roots", "roots"), &GameLauncher::set_lua_roots);
    ClassDB::bind_method(D_METHOD("get_lua_roots"), &GameLauncher::get_lua_roots);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "lua_roots"),
                 "set_lua_roots", "get_lua_roots");
#endif

    ClassDB::bind_method(D_METHOD("set_auto_setup", "v"), &GameLauncher::set_auto_setup);
    ClassDB::bind_method(D_METHOD("get_auto_setup"), &GameLauncher::get_auto_setup);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_setup"), "set_auto_setup", "get_auto_setup");
}
