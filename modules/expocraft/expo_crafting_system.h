/**************************************************************************/
/*  expo_crafting_system.h                                                */
/**************************************************************************/

#pragma once

#include "core/object/ref_counted.h"
#include "core/variant/variant.h"

class ExpoContentRegistry;

class ExpoCraftingSystem : public RefCounted {
	GDCLASS(ExpoCraftingSystem, RefCounted);

protected:
	static void _bind_methods();

public:
	Dictionary craft(const Array &p_grid, const Ref<ExpoContentRegistry> &p_content) const;
};
