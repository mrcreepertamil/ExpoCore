/**************************************************************************/
/*  expo_crafting_system.cpp                                              */
/**************************************************************************/

#include "expo_crafting_system.h"

#include "core/object/class_db.h"
#include "expo_content_registry.h"

namespace {
static String _slot_item(const Array &p_grid, int p_index) {
	if (p_index < 0 || p_index >= p_grid.size()) {
		return String();
	}
	return String(p_grid[p_index]);
}

static bool _matches_shaped_recipe(const Array &p_grid, const Array &p_pattern, const Dictionary &p_key, int &r_offset_x, int &r_offset_y) {
	if (p_grid.size() != 9 || p_pattern.is_empty() || p_pattern.size() > 3) {
		return false;
	}
	int pattern_width = 0;
	for (int y = 0; y < p_pattern.size(); y++) {
		pattern_width = MAX(pattern_width, String(p_pattern[y]).length());
	}
	if (pattern_width < 1 || pattern_width > 3) {
		return false;
	}

	for (int offset_y = 0; offset_y <= 3 - p_pattern.size(); offset_y++) {
		for (int offset_x = 0; offset_x <= 3 - pattern_width; offset_x++) {
			bool matches = true;
			for (int y = 0; y < 3 && matches; y++) {
				for (int x = 0; x < 3; x++) {
					const int local_x = x - offset_x;
					const int local_y = y - offset_y;
					String expected;
					if (local_x >= 0 && local_x < pattern_width && local_y >= 0 && local_y < p_pattern.size()) {
						const String row = String(p_pattern[local_y]);
						if (local_x < row.length()) {
							const String symbol = row.substr(local_x, 1);
							if (symbol != " ") {
								expected = String(p_key.get(symbol, ""));
								if (expected.is_empty()) {
									matches = false;
									break;
								}
							}
						}
					}
					if (_slot_item(p_grid, y * 3 + x) != expected) {
						matches = false;
						break;
					}
				}
			}
			if (matches) {
				r_offset_x = offset_x;
				r_offset_y = offset_y;
				return true;
			}
		}
	}
	return false;
}

static bool _matches_shapeless_recipe(const Array &p_grid, const Array &p_ingredients) {
	if (p_grid.size() != 9 || p_ingredients.is_empty()) {
		return false;
	}
	Dictionary available;
	Dictionary required;
	for (int i = 0; i < p_grid.size(); i++) {
		const String item = _slot_item(p_grid, i);
		if (!item.is_empty()) {
			available[item] = int(available.get(item, 0)) + 1;
		}
	}
	for (int i = 0; i < p_ingredients.size(); i++) {
		const String item = String(p_ingredients[i]);
		if (item.is_empty()) {
			return false;
		}
		required[item] = int(required.get(item, 0)) + 1;
	}
	if (available.size() != required.size()) {
		return false;
	}
	const Array required_keys = required.keys();
	for (int i = 0; i < required_keys.size(); i++) {
		const Variant key = required_keys[i];
		if (int(available.get(key, 0)) != int(required.get(key, 0))) {
			return false;
		}
	}
	return true;
}
}

Dictionary ExpoCraftingSystem::craft(const Array &p_grid, const Ref<ExpoContentRegistry> &p_content) const {
	Dictionary result;
	result["success"] = false;
	result["remaining_grid"] = p_grid;
	ERR_FAIL_COND_V(p_content.is_null(), result);
	ERR_FAIL_COND_V(p_grid.size() != 9, result);

	const PackedStringArray recipe_ids = p_content->get_ids(StringName("recipes"));
	for (int recipe_index = 0; recipe_index < recipe_ids.size(); recipe_index++) {
		const StringName recipe_id = recipe_ids[recipe_index];
		const Dictionary recipe = p_content->get_definition(StringName("recipes"), recipe_id);
		const String type = String(recipe.get("type", ""));
		bool matched = false;
		Array consumed_slots;

		if (type == "crafting_shaped") {
			const Array pattern = recipe.get("pattern", Array());
			const Dictionary key = recipe.get("key", Dictionary());
			int offset_x = 0;
			int offset_y = 0;
			matched = _matches_shaped_recipe(p_grid, pattern, key, offset_x, offset_y);
			if (matched) {
				for (int y = 0; y < pattern.size(); y++) {
					const String row = String(pattern[y]);
					for (int x = 0; x < row.length(); x++) {
						const String symbol = row.substr(x, 1);
						if (symbol != " ") {
							consumed_slots.push_back((offset_y + y) * 3 + offset_x + x);
						}
					}
				}
			}
		} else if (type == "crafting_shapeless") {
			const Array ingredients = recipe.get("ingredients", Array());
			matched = _matches_shapeless_recipe(p_grid, ingredients);
			if (matched) {
				Dictionary remaining_counts;
				for (int i = 0; i < ingredients.size(); i++) {
					const String item = String(ingredients[i]);
					remaining_counts[item] = int(remaining_counts.get(item, 0)) + 1;
				}
				for (int slot = 0; slot < 9; slot++) {
					const String item = _slot_item(p_grid, slot);
					const int needed = int(remaining_counts.get(item, 0));
					if (needed > 0) {
						consumed_slots.push_back(slot);
						remaining_counts[item] = needed - 1;
					}
				}
			}
		}

		if (!matched) {
			continue;
		}

		Array remaining_grid = p_grid;
		for (int i = 0; i < consumed_slots.size(); i++) {
			remaining_grid[int(consumed_slots[i])] = "";
		}
		const Dictionary output = recipe.get("result", Dictionary());
		result["success"] = true;
		result["recipe_id"] = recipe_id;
		result["item"] = String(output.get("item", ""));
		result["count"] = int(output.get("count", 1));
		result["remaining_grid"] = remaining_grid;
		return result;
	}
	return result;
}

void ExpoCraftingSystem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("craft", "grid", "content"), &ExpoCraftingSystem::craft);
}
