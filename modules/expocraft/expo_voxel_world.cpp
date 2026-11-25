/**************************************************************************/
/*  expo_voxel_world.cpp                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "expo_voxel_world.h"

#include <cstdint>

#include "core/object/class_db.h"

bool ExpoVoxelWorld::_is_position_valid(const Vector3i &p_position) const {
	return p_position.x >= 0 && p_position.y >= 0 && p_position.z >= 0 &&
			p_position.x < dimensions.x && p_position.y < dimensions.y && p_position.z < dimensions.z;
}

int ExpoVoxelWorld::_get_index(const Vector3i &p_position) const {
	const int64_t index = (static_cast<int64_t>(p_position.z) * dimensions.y + p_position.y) * dimensions.x + p_position.x;
	return static_cast<int>(index);
}

bool ExpoVoxelWorld::set_size(const Vector3i &p_dimensions) {
	ERR_FAIL_COND_V(p_dimensions.x < 0 || p_dimensions.y < 0 || p_dimensions.z < 0, false);

	const int64_t block_count = static_cast<int64_t>(p_dimensions.x) * p_dimensions.y * p_dimensions.z;
	ERR_FAIL_COND_V(block_count > INT32_MAX, false);

	dimensions = p_dimensions;
	blocks.resize(static_cast<int>(block_count));
	blocks.fill(0);
	return true;
}

Vector3i ExpoVoxelWorld::get_size() const {
	return dimensions;
}

bool ExpoVoxelWorld::set_block(const Vector3i &p_position, int p_block_id) {
	ERR_FAIL_COND_V(!_is_position_valid(p_position), false);
	ERR_FAIL_COND_V(p_block_id < 0 || p_block_id > 65535, false);

	blocks.write[_get_index(p_position)] = static_cast<uint16_t>(p_block_id);
	return true;
}

int ExpoVoxelWorld::get_block(const Vector3i &p_position) const {
	ERR_FAIL_COND_V(!_is_position_valid(p_position), 0);
	return blocks[_get_index(p_position)];
}

int ExpoVoxelWorld::get_block_count() const {
	return blocks.size();
}

void ExpoVoxelWorld::clear() {
	blocks.clear();
	dimensions = Vector3i();
}

void ExpoVoxelWorld::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_size", "dimensions"), &ExpoVoxelWorld::set_size);
	ClassDB::bind_method(D_METHOD("get_size"), &ExpoVoxelWorld::get_size);
	ClassDB::bind_method(D_METHOD("set_block", "position", "block_id"), &ExpoVoxelWorld::set_block);
	ClassDB::bind_method(D_METHOD("get_block", "position"), &ExpoVoxelWorld::get_block);
	ClassDB::bind_method(D_METHOD("get_block_count"), &ExpoVoxelWorld::get_block_count);
	ClassDB::bind_method(D_METHOD("clear"), &ExpoVoxelWorld::clear);
}
