# ExpoCraft native runtime module

This module adds the first ExpoCraft-specific native C++ runtime class to ExpoCore.

## ExpoVoxelWorld

`ExpoVoxelWorld` is a `RefCounted` object that stores a three-dimensional grid of
16-bit block IDs. It is exposed to Godot's ClassDB, so GDScript and other engine
systems can create and use it.

Example:

```gdscript
var world := ExpoVoxelWorld.new()
world.set_size(Vector3i(16, 64, 16))
world.set_block(Vector3i(2, 10, 3), 1)

var block_id: int = world.get_block(Vector3i(2, 10, 3))
print(block_id)
```

Coordinates outside the configured dimensions are rejected. Block ID 0 represents
an empty block by convention; IDs 1 through 65535 are available to the game.

This is the storage foundation only. Chunk meshing, rendering, world generation,
saving, collision, and player movement are separate systems to be added later.
