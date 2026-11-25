/**************************************************************************/
/*  expo_content_registry.h                                               */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#pragma once

#include "core/object/ref_counted.h"
#include "core/variant/variant.h"

class ExpoContentRegistry : public RefCounted {
	GDCLASS(ExpoContentRegistry, RefCounted);

	Dictionary categories;

	bool _is_supported_category(const StringName &p_category) const;

protected:
	static void _bind_methods();

public:
	bool load_json(const StringName &p_category, const String &p_path);
	Dictionary get_definition(const StringName &p_category, const StringName &p_id) const;
	PackedStringArray get_ids(const StringName &p_category) const;
	int get_block_numeric_id(const StringName &p_id) const;
	bool has_definition(const StringName &p_category, const StringName &p_id) const;
	void clear();
};
