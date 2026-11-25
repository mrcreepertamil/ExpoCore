/**************************************************************************/
/*  expo_voxel_world.cpp                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "expo_voxel_world.h"

#include <cmath>
#include <cstdint>

#include "core/object/class_db.h"

namespace {
static uint64_t _mix_seed(uint64_t p_value) {
	p_value += 0x9e3779b97f4a7c15ULL;
	p_value = (p_value ^ (p_value >> 30)) * 0xbf58476d1ce4e5b9ULL;
	p_value = (p_value ^ (p_value >> 27)) * 0x94d049bb133111ebULL;
	return p_value ^ (p_value >> 31);
}

static float _lattice_noise(int p_x, int p_z, int64_t p_seed) {
	uint64_t value = static_cast<uint64_t>(p_seed);
	value ^= static_cast<uint64_t>(static_cast<int64_t>(p_x)) * 0x632be59bd9b4e019ULL;
	value ^= static_cast<uint64_t>(static_cast<int64_t>(p_z)) * 0x9e3779b185ebca87ULL;
	const uint32_t bits = static_cast<uint32_t>(_mix_seed(value) >> 40);
	return (static_cast<float>(bits) / 8388607.5f) - 1.0f;
}

static float _smooth_noise(float p_x, float p_z, int64_t p_seed, int p_scale) {
	const float grid_x = p_x / static_cast<float>(p_scale);
	const float grid_z = p_z / static_cast<float>(p_scale);
	const int x0 = static_cast<int>(std::floor(grid_x));
	const int z0 = static_cast<int>(std::floor(grid_z));
	const float tx = grid_x - static_cast<float>(x0);
	const float tz = grid_z - static_cast<float>(z0);
	const float sx = tx * tx * (3.0f - 2.0f * tx);
	const float sz = tz * tz * (3.0f - 2.0f * tz);

	const float a = _lattice_noise(x0, z0, p_seed);
	const float b = _lattice_noise(x0 + 1, z0, p_seed);
	const float c = _lattice_noise(x0, z0 + 1, p_seed);
	const float d = _lattice_noise(x0 + 1, z0 + 1, p_seed);
	const float top = a + (b - a) * sx;
	const float bottom = c + (d - c) * sx;
	return top + (bottom - top) * sz;
}

static float _terrain_noise(float p_x, float p_z, int64_t p_seed) {
	return _smooth_noise(p_x, p_z, p_seed, 32) * 0.57f +
			_smooth_noise(p_x, p_z, p_seed ^ 0x5deece66dLL, 16) * 0.29f +
			_smooth_noise(p_x, p_z, p_seed ^ 0x1347a5b9LL, 8) * 0.14f;
}
} // namespace

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

void ExpoVoxelWorld::set_seed(int64_t p_seed) {
	world_seed = p_seed;
}

int64_t ExpoVoxelWorld::get_seed() const {
	return world_seed;
}

bool ExpoVoxelWorld::generate_terrain(int64_t p_seed, int p_base_height, int p_height_variation, int p_grass_block_id, int p_dirt_block_id, int p_stone_block_id) {
	ERR_FAIL_COND_V(dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0, false);
	ERR_FAIL_COND_V(p_height_variation < 0, false);
	ERR_FAIL_COND_V(p_grass_block_id < 1 || p_grass_block_id > 65535, false);
	ERR_FAIL_COND_V(p_dirt_block_id < 1 || p_dirt_block_id > 65535, false);
	ERR_FAIL_COND_V(p_stone_block_id < 1 || p_stone_block_id > 65535, false);

	world_seed = p_seed;
	blocks.fill(0);

	for (int z = 0; z < dimensions.z; z++) {
		for (int x = 0; x < dimensions.x; x++) {
			const float noise = _terrain_noise(static_cast<float>(x), static_cast<float>(z), world_seed);
			const int surface_y = CLAMP(p_base_height + static_cast<int>(std::round(noise * p_height_variation)), 0, dimensions.y - 1);

			for (int y = 0; y <= surface_y; y++) {
				const int block_id = y == surface_y ? p_grass_block_id : (y >= surface_y - 3 ? p_dirt_block_id : p_stone_block_id);
				blocks.write[_get_index(Vector3i(x, y, z))] = static_cast<uint16_t>(block_id);
			}
		}
	}
	return true;
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
	world_seed = 0;
}

void ExpoVoxelWorld::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_size", "dimensions"), &ExpoVoxelWorld::set_size);
	ClassDB::bind_method(D_METHOD("get_size"), &ExpoVoxelWorld::get_size);
	ClassDB::bind_method(D_METHOD("set_seed", "seed"), &ExpoVoxelWorld::set_seed);
	ClassDB::bind_method(D_METHOD("get_seed"), &ExpoVoxelWorld::get_seed);
	ClassDB::bind_method(D_METHOD("generate_terrain", "seed", "base_height", "height_variation", "grass_block_id", "dirt_block_id", "stone_block_id"), &ExpoVoxelWorld::generate_terrain);
	ClassDB::bind_method(D_METHOD("set_block", "position", "block_id"), &ExpoVoxelWorld::set_block);
	ClassDB::bind_method(D_METHOD("get_block", "position"), &ExpoVoxelWorld::get_block);
	ClassDB::bind_method(D_METHOD("get_block_count"), &ExpoVoxelWorld::get_block_count);
	ClassDB::bind_method(D_METHOD("clear"), &ExpoVoxelWorld::clear);
}
