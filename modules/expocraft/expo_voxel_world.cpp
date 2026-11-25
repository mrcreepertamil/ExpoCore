/**************************************************************************/
/*  expo_voxel_world.cpp                                                  */
/**************************************************************************/

#include "expo_voxel_world.h"

#include <cmath>
#include <cstdint>

#include "core/io/resource_loader.h"
#include "core/object/class_db.h"
#include "core/variant/variant.h"
#include "expo_content_registry.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"
#include "scene/resources/texture.h"

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

struct ExpoMeshSurface {
	String texture_path;
	PackedVector3Array vertices;
	PackedVector3Array normals;
	PackedVector2Array uvs;
};

static void _append_face(ExpoMeshSurface &r_surface, const Vector3 &p_origin, int p_face) {
	Vector3 points[6];
	Vector3 normal;
	switch (p_face) {
		case 0: // +X
			normal = Vector3(1, 0, 0);
			points[0] = Vector3(1, 0, 0); points[1] = Vector3(1, 1, 0); points[2] = Vector3(1, 1, 1);
			points[3] = Vector3(1, 0, 0); points[4] = Vector3(1, 1, 1); points[5] = Vector3(1, 0, 1);
			break;
		case 1: // -X
			normal = Vector3(-1, 0, 0);
			points[0] = Vector3(0, 0, 1); points[1] = Vector3(0, 1, 1); points[2] = Vector3(0, 1, 0);
			points[3] = Vector3(0, 0, 1); points[4] = Vector3(0, 1, 0); points[5] = Vector3(0, 0, 0);
			break;
		case 2: // +Y
			normal = Vector3(0, 1, 0);
			points[0] = Vector3(0, 1, 0); points[1] = Vector3(0, 1, 1); points[2] = Vector3(1, 1, 1);
			points[3] = Vector3(0, 1, 0); points[4] = Vector3(1, 1, 1); points[5] = Vector3(1, 1, 0);
			break;
		case 3: // -Y
			normal = Vector3(0, -1, 0);
			points[0] = Vector3(0, 0, 1); points[1] = Vector3(0, 0, 0); points[2] = Vector3(1, 0, 0);
			points[3] = Vector3(0, 0, 1); points[4] = Vector3(1, 0, 0); points[5] = Vector3(1, 0, 1);
			break;
		case 4: // +Z
			normal = Vector3(0, 0, 1);
			points[0] = Vector3(1, 0, 1); points[1] = Vector3(1, 1, 1); points[2] = Vector3(0, 1, 1);
			points[3] = Vector3(1, 0, 1); points[4] = Vector3(0, 1, 1); points[5] = Vector3(0, 0, 1);
			break;
		default: // -Z
			normal = Vector3(0, 0, -1);
			points[0] = Vector3(0, 0, 0); points[1] = Vector3(0, 1, 0); points[2] = Vector3(1, 1, 0);
			points[3] = Vector3(0, 0, 0); points[4] = Vector3(1, 1, 0); points[5] = Vector3(1, 0, 0);
			break;
	}

	const Vector2 face_uvs[6] = {
		Vector2(0, 1), Vector2(0, 0), Vector2(1, 0),
		Vector2(0, 1), Vector2(1, 0), Vector2(1, 1)
	};
	for (int i = 0; i < 6; i++) {
		r_surface.vertices.push_back(p_origin + points[i]);
		r_surface.normals.push_back(normal);
		r_surface.uvs.push_back(face_uvs[i]);
	}
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

bool ExpoVoxelWorld::generate_biome_terrain(int64_t p_seed, const Dictionary &p_biome_definition, const Ref<ExpoContentRegistry> &p_content) {
	ERR_FAIL_COND_V(p_content.is_null(), false);
	ERR_FAIL_COND_V(p_biome_definition.is_empty(), false);
	const Dictionary terrain = p_biome_definition.get("terrain", Dictionary());
	const StringName surface_id = StringName(String(terrain.get("surface_block", "")));
	const StringName subsurface_id = StringName(String(terrain.get("subsurface_block", "")));
	const StringName stone_id = StringName(String(terrain.get("stone_block", "")));
	const int surface_numeric_id = p_content->get_block_numeric_id(surface_id);
	const int subsurface_numeric_id = p_content->get_block_numeric_id(subsurface_id);
	const int stone_numeric_id = p_content->get_block_numeric_id(stone_id);
	ERR_FAIL_COND_V(surface_numeric_id == 0 || subsurface_numeric_id == 0 || stone_numeric_id == 0, false);
	const int base_height = int(terrain.get("base_height", 24));
	const int height_variation = int(terrain.get("height_variation", 4));
	return generate_terrain(p_seed, base_height, height_variation, surface_numeric_id, subsurface_numeric_id, stone_numeric_id);
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

Ref<ArrayMesh> ExpoVoxelWorld::build_mesh(const Ref<ExpoContentRegistry> &p_content) const {
	ERR_FAIL_COND_V(p_content.is_null(), Ref<ArrayMesh>());
	Ref<ArrayMesh> mesh;
	mesh.instantiate();
	if (blocks.is_empty()) {
		return mesh;
	}

	Vector<ExpoMeshSurface> surfaces;
	const Vector3i neighbor_offsets[6] = {
		Vector3i(1, 0, 0), Vector3i(-1, 0, 0), Vector3i(0, 1, 0),
		Vector3i(0, -1, 0), Vector3i(0, 0, 1), Vector3i(0, 0, -1)
	};
	const char *face_names[6] = { "side", "side", "top", "bottom", "side", "side" };

	for (int z = 0; z < dimensions.z; z++) {
		for (int y = 0; y < dimensions.y; y++) {
			for (int x = 0; x < dimensions.x; x++) {
				const Vector3i position(x, y, z);
				const int numeric_id = get_block(position);
				if (numeric_id == 0) {
					continue;
				}

				const StringName block_id = p_content->get_block_identifier(numeric_id);
				if (block_id == StringName()) {
					continue;
				}
				const Dictionary block = p_content->get_definition(StringName("blocks"), block_id);
				const Dictionary textures = block.get("textures", Dictionary());

				for (int face = 0; face < 6; face++) {
					if (get_block(position + neighbor_offsets[face]) != 0) {
						continue;
					}
					const String texture_path = String(textures.get(face_names[face], textures.get("all", textures.get("side", ""))));
					int surface_index = -1;
					for (int s = 0; s < surfaces.size(); s++) {
						if (surfaces[s].texture_path == texture_path) {
							surface_index = s;
							break;
						}
					}
					if (surface_index == -1) {
						ExpoMeshSurface new_surface;
						new_surface.texture_path = texture_path;
						surfaces.push_back(new_surface);
						surface_index = surfaces.size() - 1;
					}
					_append_face(surfaces.write[surface_index], Vector3(x, y, z), face);
				}
			}
		}
	}

	for (int s = 0; s < surfaces.size(); s++) {
		Array arrays;
		arrays.resize(Mesh::ARRAY_MAX);
		arrays[Mesh::ARRAY_VERTEX] = surfaces[s].vertices;
		arrays[Mesh::ARRAY_NORMAL] = surfaces[s].normals;
		arrays[Mesh::ARRAY_TEX_UV] = surfaces[s].uvs;
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

		Ref<StandardMaterial3D> material;
		material.instantiate();
		material->set_texture_filter(BaseMaterial3D::TEXTURE_FILTER_NEAREST);
		if (!surfaces[s].texture_path.is_empty()) {
			Ref<Resource> loaded = ResourceLoader::load(surfaces[s].texture_path);
			Ref<Texture2D> texture = loaded;
			if (texture.is_valid()) {
				material->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, texture);
			}
		}
		mesh->surface_set_material(s, material);
	}
	return mesh;
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
	ClassDB::bind_method(D_METHOD("generate_biome_terrain", "seed", "biome_definition", "content"), &ExpoVoxelWorld::generate_biome_terrain);
	ClassDB::bind_method(D_METHOD("set_block", "position", "block_id"), &ExpoVoxelWorld::set_block);
	ClassDB::bind_method(D_METHOD("get_block", "position"), &ExpoVoxelWorld::get_block);
	ClassDB::bind_method(D_METHOD("build_mesh", "content"), &ExpoVoxelWorld::build_mesh);
	ClassDB::bind_method(D_METHOD("get_block_count"), &ExpoVoxelWorld::get_block_count);
	ClassDB::bind_method(D_METHOD("clear"), &ExpoVoxelWorld::clear);
}
