/**************************************************************************/
/*  register_types.cpp                                                    */
/**************************************************************************/

#include "register_types.h"

#include "core/object/class_db.h"
#include "expo_content_registry.h"
#include "expo_crafting_system.h"
#include "expo_entity_spawner.h"
#include "expo_voxel_world.h"

void initialize_expocraft_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		GDREGISTER_CLASS(ExpoVoxelWorld);
		GDREGISTER_CLASS(ExpoContentRegistry);
		GDREGISTER_CLASS(ExpoEntitySpawner);
		GDREGISTER_CLASS(ExpoCraftingSystem);
	}
}

void uninitialize_expocraft_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
