# ExpoCraft native runtime module

This module adds ExpoCraft-specific native C++ runtime functionality to ExpoCore.

## ExpoVoxelWorld

`ExpoVoxelWorld` stores a three-dimensional grid of 16-bit block IDs. Block ID
0 is reserved for air/empty space.

## JSON content definitions

ExpoCraft content metadata is stored as JSON under the repository's `data/`
directory. The initial data files are:

- `blocks.json`: stable namespaced block IDs, numeric voxel IDs, texture paths,
  physical properties, and drops.
- `biomes.json`: climate values, terrain block references, terrain parameters,
  features, and spawn groups.
- `entities.json`: entity scenes, stats, behavior configuration, and drops.
- `items.json`: item categories, icons, stack limits, block placement, tools,
  and food properties.
- `recipes.json`: shaped and shapeless crafting recipe data.

Definitions use namespaced string IDs such as `expo:stone`. Block definitions
also have a unique `numeric_id` from 1 through 65535; 0 remains reserved for air.
References between definitions are strings, so content can be extended without
renumbering every item or entity.

### Loading definitions from GDScript

```gdscript
var content := ExpoContentRegistry.new()

content.load_json("blocks", "res://data/blocks.json")
content.load_json("biomes", "res://data/biomes.json")
content.load_json("entities", "res://data/entities.json")
content.load_json("items", "res://data/items.json")
content.load_json("recipes", "res://data/recipes.json")

var stone: Dictionary = content.get_definition("blocks", "expo:stone")
var stone_numeric_id: int = content.get_block_numeric_id("expo:stone")
print(stone.get("name"), " -> ", stone_numeric_id)

for biome_id in content.get_ids("biomes"):
    print(biome_id)
```

The registry validates JSON structure, required string IDs, duplicate IDs,
and unique block numeric IDs. A category is replaced only after its entire file
passes validation. It exposes parsed dictionaries to GDScript; it does not yet
instantiate scenes, render blocks, execute entity AI, generate biome
transitions, or run crafting recipes. Those systems consume the metadata in
later runtime work.

## Seeded terrain generation

```gdscript
var world := ExpoVoxelWorld.new()
world.set_size(Vector3i(64, 64, 64))
world.generate_terrain(123456, 24, 12, 1, 2, 3)
```

The current terrain generator uses deterministic noise and the supplied numeric
IDs. Biome selection and per-biome terrain settings are not yet connected to
the generator.
