// register_types.cpp - GDExtension entry point.
// Registers BlueprintNode with Godot's ClassDB.

#include "register_types.h"
#include "blueprint_node.h"
#include "game_launcher.h"

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
