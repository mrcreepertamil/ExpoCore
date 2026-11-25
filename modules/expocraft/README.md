# ExpoCraft native runtime module

This module contains native C++ runtime systems used by ExpoCraft. The public
classes are registered with Godot and can be called from GDScript.

## 1. Voxel storage, terrain, and rendering

`ExpoVoxelWorld` stores a three-dimensional grid of 16-bit block IDs. Block ID
0 is reserved for air/empty space.

```gdscript
var content := ExpoContentRegistry.new()
content.load_json("blocks", "res://data/blocks.json")
content.load_json("biomes", "res://data/biomes.json")

var world := ExpoVoxelWorld.new()
world.set_size(Vector3i(64, 64, 64))

var biome_id: StringName = content.get_best_biome(0.8, 0.4)
var biome: Dictionary = content.get_definition("biomes", biome_id)
world.generate_biome_terrain(123456, biome, content)

var mesh_instance := MeshInstance3D.new()
mesh_instance.mesh = world.build_mesh(content)
add_child(mesh_instance)
```

`build_mesh()` emits only faces exposed to air, groups surfaces by the block
texture path, and uses nearest texture filtering. It reads the `top`,
`side`, `bottom`, or `all` texture entries from `blocks.json`. Missing
texture resources leave the corresponding surface with the default material.
This is a first renderer foundation: chunk meshing, greedy meshing, collision
generation, texture atlases, transparent/liquid sorting, and block lighting are
not included yet.

`get_best_biome(temperature, humidity)` selects the loaded biome with the
nearest climate values. `generate_biome_terrain()` uses that biome's terrain
height and surface/subsurface/stone block references. Biome features such as
trees and plants are metadata only at this stage.

## 2. JSON content definitions

Content metadata is stored as JSON under the repository's `data/` directory:

- `blocks.json`: namespaced IDs, numeric voxel IDs, textures, physical properties, and drops.
- `biomes.json`: climate, terrain settings, features, and spawn groups.
- `entities.json`: entity scenes, stats, behaviour settings, and drops.
- `items.json`: item categories, icons, stack limits, tools, and food properties.
- `recipes.json`: shaped and shapeless crafting recipe data.

Definitions use namespaced IDs such as `expo:stone`. Blocks have unique numeric
IDs from 1 to 65535; 0 is reserved for air.

### Load JSON

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
numeric IDs. A category is replaced only after its complete file passes
validation.

## 3. Entity spawning

`ExpoEntitySpawner` loads the scene path from an entity definition, instantiates
it, places its root Node3D at the requested local position, and adds it as a
child of the spawner.

```gdscript
var spawner := ExpoEntitySpawner.new()
add_child(spawner)

var pig := spawner.spawn_entity("expo:pig", Vector3(4, 25, 4), content)
var biome_spawns := spawner.spawn_biome_group(
    biome,
    "passive",
    content,
    Vector3(8, 25, 8),
    4,       # entity count
    10.0,    # horizontal spawn radius
    123456   # deterministic selection seed
)
```

Entity scene files must exist at the paths declared in `entities.json`. The
spawner handles scene instantiation and placement; entity AI, navigation,
animation, combat, and drops remain responsibilities of the entity scenes and
their gameplay scripts.

## 4. Crafting

`ExpoCraftingSystem` checks a 3x3 crafting grid against loaded shaped and
shapeless recipes. Each grid slot is an item ID string or an empty string. One
item is consumed from each matching occupied slot.

```gdscript
var crafting := ExpoCraftingSystem.new()
var grid := [
    "expo:oak_planks", "expo:oak_planks", "expo:oak_planks",
    "",                "expo:stick",      "",
    "",                "expo:stick",      ""
]

var craft_result: Dictionary = crafting.craft(grid, content)
if craft_result.success:
    print(craft_result.item, " x", craft_result.count)
    grid = craft_result.remaining_grid
```

The returned dictionary contains `success`, `recipe_id`, `item`, `count`,
and `remaining_grid`. This API works with item IDs; it does not yet provide an
inventory, stack merging, crafting UI, furnace recipes, or persistent player
inventory.

## Build and packaging notes

The module is included by the ExpoCore module build system. The JSON files and
the referenced entity scenes/textures must also be included in the exported
ExpoCraft project at the same `res://` paths. This repository update has not
been compiled or run in an ExpoCore build yet, so platform and runtime
compatibility still need to be verified.
