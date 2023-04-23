#ifndef REGISTER_TYPES_H
#define REGISTER_TYPES_H

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void register_gdsam(ModuleInitializationLevel p_level);
void unregister_gdsam(ModuleInitializationLevel p_level);

#endif