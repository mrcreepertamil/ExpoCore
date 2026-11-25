/**************************************************************************/
/*  expo_voxel_world.h                                                    */
/**************************************************************************/

#pragma once

#include <cstdint>

#include "core/object/ref_counted.h"
#include "core/templates/vector.h"
#include "core/math/vector3i.h"
#include "core/variant/variant.h"

class ArrayMesh;
class ExpoContentRegistry;

class ExpoVoxelWorld : public RefCounted {
	GDCLASS(ExpoVoxelWorld, RefCounted);

	Vector3i dimensions;
	Vector<uint16_t> blocks;
	int64_t world_seed = 0;

	bool _is_position_valid(const Vector3i &p_position) const;
	int _get_index(const Vector3i &p_position) const;

protected:
	static void _bind_methods();

public:
	bool set_size(const Vector3i &p_dimensions);
	Vector3i get_size() const;

	void set_seed(int64_t p_seed);
	int64_t get_seed() const;
	bool generate_terrain(int64_t p_seed, int p_base_height, int p_height_variation, int p_grass_block_id, int p_dirt_block_id, int p_stone_block_id);
	bool generate_biome_terrain(int64_t p_seed, const Dictionary &p_biome_definition, const Ref<ExpoContentRegistry> &p_content);

	bool set_block(const Vector3i &p_position, int p_block_id);
	int get_block(const Vector3i &p_position) const;

	Ref<ArrayMesh> build_mesh(const Ref<ExpoContentRegistry> &p_content) const;

	int get_block_count() const;
	void clear();
};
