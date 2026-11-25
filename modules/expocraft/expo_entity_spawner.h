/**************************************************************************/
/*  expo_entity_spawner.h                                                 */
/**************************************************************************/

#pragma once

#include "scene/3d/node_3d.h"

class ExpoContentRegistry;

class ExpoEntitySpawner : public Node3D {
	GDCLASS(ExpoEntitySpawner, Node3D);

protected:
	static void _bind_methods();

public:
	Node *spawn_entity(const StringName &p_entity_id, const Vector3 &p_position, const Ref<ExpoContentRegistry> &p_content);
	Array spawn_biome_group(const Dictionary &p_biome_definition, const StringName &p_group, const Ref<ExpoContentRegistry> &p_content, const Vector3 &p_origin, int p_count, float p_radius, int64_t p_seed);
};
