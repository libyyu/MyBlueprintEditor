#ifndef BLUEPRINT_REGISTER_TYPES_H
#define BLUEPRINT_REGISTER_TYPES_H

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void initialize_blueprint_module(ModuleInitializationLevel p_level);
void uninitialize_blueprint_module(ModuleInitializationLevel p_level);

#endif // BLUEPRINT_REGISTER_TYPES_H
