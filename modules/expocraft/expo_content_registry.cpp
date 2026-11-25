/**************************************************************************/
/*  expo_content_registry.cpp                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#include "expo_content_registry.h"

#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/object/class_db.h"

bool ExpoContentRegistry::_is_supported_category(const StringName &p_category) const {
	return p_category == StringName("blocks") ||
			p_category == StringName("biomes") ||
			p_category == StringName("entities") ||
			p_category == StringName("items") ||
			p_category == StringName("recipes");
}

bool ExpoContentRegistry::load_json(const StringName &p_category, const String &p_path) {
	ERR_FAIL_COND_V(!_is_supported_category(p_category), false);

	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V(file.is_null(), false);

	Ref<JSON> parser;
	parser.instantiate();
	const Error parse_error = parser->parse(file->get_as_text());
	ERR_FAIL_COND_V_MSG(parse_error != OK, false, vformat("ExpoCraft: invalid JSON in '%s' at line %d: %s", p_path, parser->get_error_line(), parser->get_error_message()));

	const Variant parsed = parser->get_data();
	ERR_FAIL_COND_V_MSG(parsed.get_type() != Variant::ARRAY, false, "ExpoCraft content JSON root must be an array.");

	const Array entries = parsed;
	Dictionary loaded_definitions;
	Dictionary used_numeric_ids;

	for (int i = 0; i < entries.size(); i++) {
		const Variant entry = entries[i];
		ERR_FAIL_COND_V_MSG(entry.get_type() != Variant::DICTIONARY, false, vformat("ExpoCraft: entry %d in '%s' must be an object.", i, p_path));

		const Dictionary definition = entry;
		const Variant id_value = definition.get("id", Variant());
		ERR_FAIL_COND_V_MSG(id_value.get_type() != Variant::STRING, false, vformat("ExpoCraft: entry %d in '%s' needs a string id.", i, p_path));

		const StringName id = String(id_value);
		ERR_FAIL_COND_V_MSG(id == StringName(), false, vformat("ExpoCraft: entry %d in '%s' has an empty id.", i, p_path));
		ERR_FAIL_COND_V_MSG(loaded_definitions.has(id), false, vformat("ExpoCraft: duplicate id '%s' in '%s'.", String(id), p_path));

		if (p_category == StringName("blocks")) {
			const Variant numeric_id_value = definition.get("numeric_id", Variant());
			ERR_FAIL_COND_V_MSG(numeric_id_value.get_type() != Variant::INT, false, vformat("ExpoCraft: block '%s' needs an integer numeric_id.", String(id)));
			const int numeric_id = numeric_id_value;
			ERR_FAIL_COND_V_MSG(numeric_id < 1 || numeric_id > 65535, false, vformat("ExpoCraft: block '%s' numeric_id must be between 1 and 65535 (0 is air).", String(id)));
			ERR_FAIL_COND_V_MSG(used_numeric_ids.has(numeric_id), false, vformat("ExpoCraft: duplicate block numeric_id %d.", numeric_id));
			used_numeric_ids[numeric_id] = true;
		}

		loaded_definitions[id] = definition;
	}

	// Replace a category only after the entire file has passed validation.
	categories[p_category] = loaded_definitions;
	return true;
}

Dictionary ExpoContentRegistry::get_definition(const StringName &p_category, const StringName &p_id) const {
	const Dictionary category_definitions = categories.get(p_category, Dictionary());
	return category_definitions.get(p_id, Dictionary());
}

PackedStringArray ExpoContentRegistry::get_ids(const StringName &p_category) const {
	PackedStringArray ids;
	const Dictionary category_definitions = categories.get(p_category, Dictionary());
	const Array keys = category_definitions.keys();
	for (int i = 0; i < keys.size(); i++) {
		ids.push_back(String(keys[i]));
	}
	ids.sort();
	return ids;
}

int ExpoContentRegistry::get_block_numeric_id(const StringName &p_id) const {
	const Dictionary definition = get_definition(StringName("blocks"), p_id);
	ERR_FAIL_COND_V(definition.is_empty(), 0);
	return int(definition.get("numeric_id", 0));
}

bool ExpoContentRegistry::has_definition(const StringName &p_category, const StringName &p_id) const {
	const Dictionary category_definitions = categories.get(p_category, Dictionary());
	return category_definitions.has(p_id);
}

void ExpoContentRegistry::clear() {
	categories.clear();
}

void ExpoContentRegistry::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load_json", "category", "path"), &ExpoContentRegistry::load_json);
	ClassDB::bind_method(D_METHOD("get_definition", "category", "id"), &ExpoContentRegistry::get_definition);
	ClassDB::bind_method(D_METHOD("get_ids", "category"), &ExpoContentRegistry::get_ids);
	ClassDB::bind_method(D_METHOD("get_block_numeric_id", "id"), &ExpoContentRegistry::get_block_numeric_id);
	ClassDB::bind_method(D_METHOD("has_definition", "category", "id"), &ExpoContentRegistry::has_definition);
	ClassDB::bind_method(D_METHOD("clear"), &ExpoContentRegistry::clear);
}
