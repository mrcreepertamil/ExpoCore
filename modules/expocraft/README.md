# ExpoCraft native runtime module

This module adds ExpoCraft-specific native C++ runtime functionality to ExpoCore.

## ExpoVoxelWorld

`ExpoVoxelWorld` stores a three-dimensional grid of 16-bit block IDs. Block ID
0 is reserved for air/empty space.

### Manual block editing

```gdscript
var world := ExpoVoxelWorld.new()
world.set_size(Vector3i(16, 64, 16))
world.set_block(Vector3i(2, 10, 3), 1)

var block_id: int = world.get_block(Vector3i(2, 10, 3))
print(block_id)
```

### Seeded terrain generation

```gdscript
var world := ExpoVoxelWorld.new()
world.set_size(Vector3i(64, 64, 64))
world.generate_terrain(123456, 24, 12, 1, 2, 3)
print("World seed: ", world.get_seed())
```

The current generator uses deterministic noise and the supplied numeric block IDs.
Biome selection and per-biome terrain settings are not yet connected.

## JSON content definitions

Content metadata is stored as JSON under the repository's `data/` directory:

- `blocks.json`: namespaced IDs, numeric voxel IDs, textures, physical properties, and drops.
- `biomes.json`: climate, terrain settings, features, and spawn groups.
- `entities.json`: entity scenes, stats, behaviour settings, and drops.
- `items.json`: item categories, icons, stack limits, tools, and food properties.
- `recipes.json`: shaped and shapeless crafting recipe data.

Definitions use namespaced IDs such as `expo:stone`. Blocks also have unique
numeric IDs from 1 to 65535; 0 is reserved for air.

### Load JSON from GDScript

```gdscript
var content := ExpoContentRegistry.new()

content.load_json("blocks", "res://data/blocks.json")
content.load_json("biomes", "res://data/biomes.json")
content.load_json("entities", "res://data/entities.json")
content.load_json("items", "res://data/items.json")
content.load_json("recipes", "res://data/recipes.json")

var stone: Dictionary = content.get_definition("blocks", "expo:stone")
print(stone.get("name"))
print(content.get_block_numeric_id("expo:stone"))
```

The registry validates JSON arrays, string IDs, duplicate IDs, and unique block
numeric IDs. It only replaces a category after its full file passes validation.
It exposes metadata dictionaries to GDScript; it does not yet instantiate scenes,
execute entity AI, render blocks, select biomes during world generation, or run
crafting recipes. Those gameplay systems must consume the loaded definitions in
subsequent implementation work.
