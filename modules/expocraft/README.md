# ExpoCraft native runtime module

This module adds ExpoCraft-specific native C++ runtime functionality to ExpoCore.

## ExpoVoxelWorld

`ExpoVoxelWorld` is a `RefCounted` object that stores a three-dimensional grid of
16-bit block IDs. It is exposed to Godot's ClassDB, so GDScript and other engine
systems can create and use it.

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

# The same seed and settings produce the same terrain.
world.generate_terrain(123456, 24, 12, 1, 2, 3)
print("World seed: ", world.get_seed())
```

The generator creates deterministic, smoothly varying terrain from the supplied
64-bit seed. It fills each column from the bottom upward, using the supplied
grass, dirt, and stone block IDs. The height is controlled by the base height
and height variation. Block ID 0 is reserved for empty space.

This is an initial terrain generator, not a complete Minecraft-style world
generator. Biomes, caves, ores, trees, chunk streaming, meshing, rendering,
saving, collision, and player movement are future systems.
