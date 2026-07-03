// register_types.cpp - GDExtension entry point.
// Registers BlueprintNode with Godot's ClassDB.

#include "register_types.h"
#include "blueprint_node.h"
#include "game_launcher.h"
#ifdef BLUEPRINT_HAS_LUA
#include "godot_lua_bindings.h"   // GdUiClickRelay
#endif

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_blueprint_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    GDREGISTER_CLASS(GameLauncher);
    GDREGISTER_CLASS(BlueprintNode);
#ifdef BLUEPRINT_HAS_LUA
    // Internal relays used by the project-layer Lua UI bridge. Must be
    // registered here at init time — registering a GDExtension class at runtime
    // crashes Godot.
    GDREGISTER_INTERNAL_CLASS(GdUiClickRelay);      // UI.on_click
    GDREGISTER_INTERNAL_CLASS(GdUiPanelReadyRelay); // UI.open_panel_async
    GDREGISTER_INTERNAL_CLASS(GdInputActionRelay);  // Input.on_action
    GDREGISTER_INTERNAL_CLASS(GdUiValueRelay);      // UI.on_text_changed / on_value_changed
    GDREGISTER_INTERNAL_CLASS(GdTimerRelay);        // Timer.after / every
#endif
}

void uninitialize_blueprint_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {
// Initialization function referenced by blueprint.gdextension (entry_symbol).
GDExtensionBool GDE_EXPORT blueprint_library_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address,
        GDExtensionClassLibraryPtr p_library,
        GDExtensionInitialization *r_initialization) {

    GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_blueprint_module);
    init_obj.register_terminator(uninitialize_blueprint_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}
