/**************************************************************************/
/*  expo_voxel_world.h                                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#pragma once

#include "core/object/ref_counted.h"
#include "core/templates/vector.h"
#include "core/math/vector3i.h"

class ExpoVoxelWorld : public RefCounted {
	GDCLASS(ExpoVoxelWorld, RefCounted);

	Vector3i dimensions;
	Vector<uint16_t> blocks;

	bool _is_position_valid(const Vector3i &p_position) const;
	int _get_index(const Vector3i &p_position) const;

protected:
	static void _bind_methods();

public:
	bool set_size(const Vector3i &p_dimensions);
	Vector3i get_size() const;

	bool set_block(const Vector3i &p_position, int p_block_id);
	int get_block(const Vector3i &p_position) const;

	int get_block_count() const;
	void clear();
};
