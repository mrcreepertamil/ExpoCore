/**************************************************************************/
/*  expo_entity_spawner.cpp                                               */
/**************************************************************************/

#include "expo_entity_spawner.h"

#include "core/io/resource_loader.h"
#include "core/object/class_db.h"
#include "expo_content_registry.h"
#include "scene/resources/packed_scene.h"

namespace {
static uint64_t _next_spawn_random(uint64_t &r_state) {
	r_state += 0x9e3779b97f4a7c15ULL;
	uint64_t value = r_state;
	value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
	value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
	return value ^ (value >> 31);
}
}

Node *ExpoEntitySpawner::spawn_entity(const StringName &p_entity_id, const Vector3 &p_position, const Ref<ExpoContentRegistry> &p_content) {
	ERR_FAIL_COND_V(p_content.is_null(), nullptr);
	const Dictionary definition = p_content->get_definition(StringName("entities"), p_entity_id);
	ERR_FAIL_COND_V_MSG(definition.is_empty(), nullptr, vformat("ExpoCraft: unknown entity '%s'.", String(p_entity_id)));

	const String scene_path = String(definition.get("scene", ""));
	ERR_FAIL_COND_V_MSG(scene_path.is_empty(), nullptr, vformat("ExpoCraft: entity '%s' has no scene path.", String(p_entity_id)));

	Ref<PackedScene> scene = ResourceLoader::load(scene_path);
	ERR_FAIL_COND_V_MSG(scene.is_null(), nullptr, vformat("ExpoCraft: could not load scene '%s' for entity '%s'.", scene_path, String(p_entity_id)));
	ERR_FAIL_COND_V_MSG(!scene->can_instantiate(), nullptr, vformat("ExpoCraft: entity scene '%s' cannot be instantiated.", scene_path));

	Node *instance = scene->instantiate();
	ERR_FAIL_COND_V(instance == nullptr, nullptr);
	if (Node3D *instance_3d = Object::cast_to<Node3D>(instance)) {
		instance_3d->set_position(p_position);
	}
	add_child(instance);
	return instance;
}

Array ExpoEntitySpawner::spawn_biome_group(const Dictionary &p_biome_definition, const StringName &p_group, const Ref<ExpoContentRegistry> &p_content, const Vector3 &p_origin, int p_count, float p_radius, int64_t p_seed) {
	Array spawned;
	ERR_FAIL_COND_V(p_content.is_null(), spawned);
	ERR_FAIL_COND_V(p_count < 0 || p_radius < 0.0f, spawned);

	const Dictionary spawns = p_biome_definition.get("spawns", Dictionary());
	const Array entity_ids = spawns.get(String(p_group), Array());
	if (entity_ids.is_empty()) {
		return spawned;
	}

	uint64_t random_state = static_cast<uint64_t>(p_seed);
	for (int i = 0; i < p_count; i++) {
		const uint64_t pick = _next_spawn_random(random_state);
		const StringName entity_id = StringName(String(entity_ids[static_cast<int>(pick % entity_ids.size())]));
		const float x_unit = static_cast<float>(_next_spawn_random(random_state) & 0xFFFFFF) / 16777215.0f;
		const float z_unit = static_cast<float>(_next_spawn_random(random_state) & 0xFFFFFF) / 16777215.0f;
		const Vector3 position = p_origin + Vector3((x_unit * 2.0f - 1.0f) * p_radius, 0.0f, (z_unit * 2.0f - 1.0f) * p_radius);
		Node *instance = spawn_entity(entity_id, position, p_content);
		if (instance != nullptr) {
			spawned.push_back(instance);
		}
	}
	return spawned;
}

void ExpoEntitySpawner::_bind_methods() {
	ClassDB::bind_method(D_METHOD("spawn_entity", "entity_id", "position", "content"), &ExpoEntitySpawner::spawn_entity);
	ClassDB::bind_method(D_METHOD("spawn_biome_group", "biome_definition", "group", "content", "origin", "count", "radius", "seed"), &ExpoEntitySpawner::spawn_biome_group);
}
