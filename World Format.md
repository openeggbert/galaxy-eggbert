# World Format

Galaxy Eggbert uses a custom binary palette-compressed voxel format to save voxel worlds.

The format is designed for small and medium-sized handcrafted 3D worlds, especially for Blupi-style 3D levels, editors, and gameplay maps.

## Overview

Default world configuration:

- default world size: `100 × 100 × 100` blocks,
- default chunk size: `10 × 10 × 10` blocks,
- default chunk count: `10 × 10 × 10 = 1000`,
- block value: `uint16_t`,
- block encoding: 12-bit type id + 4-bit metadata,
- chunk storage: local palette + adaptive bit-packing,
- world file magic: `VWR1`,
- chunk payload magic: `VCH1`,
- empty chunks are skipped in the world file (a chunk counts as empty only when it is **both**
  all-air **and** has no sparse extra metadata — see "Empty Chunks" below).

The default `100³` world is only the first practical configuration. The format should not be treated as permanently limited to this size. Future versions may support larger or non-cubic worlds by storing `chunksX`, `chunksY`, and `chunksZ` separately.

## How to Use

```cpp
#include "GalaxyEggbert/Worlds/World.hpp"

using namespace GalaxyEggbert::Worlds;

int main() {
    World world;

    const Block grass = Block::make(1, 0);
    const Block dirt = Block::make(2, 0);

    world.setBlock(10, 5, 10, grass);
    world.setBlock(10, 4, 10, dirt);

    world.saveToFile("level.vwr");

    World loaded = World::loadFromFile("level.vwr");
    return loaded.getBlock(10, 5, 10) == grass ? 0 : 1;
}
````

## Format Goals

The format is optimized for:

* compact world files,
* fast loading,
* simple editing,
* fast mesh rebuilding,
* local chunk updates,
* handcrafted 3D platformer worlds,
* future compression extensions.

The first implementation intentionally keeps the core simple:

```text
Block -> Chunk palette -> bit-packed palette indices -> World file
```
RLE, LZ4/Zstd, palette deduplication, sparse storage, and octree storage are optional future extensions.

## Default World Layout

The default world has:

```text
world size:        100 × 100 × 100 blocks
chunk size:         10 ×  10 ×  10 blocks
chunks per axis:    10 ×  10 ×  10
total chunks:       1000
```

One chunk contains:

```text
10 × 10 × 10 = 1000 blocks
```

A `25 × 25 × 25` chunk size is also possible, but it would produce:

```text
4 × 4 × 4 = 64 chunks
25 × 25 × 25 = 15625 blocks per chunk
```

For the first implementation, `10³` chunks are preferred because they are easier to edit, rebuild, debug, and save incrementally.

## Block Encoding

Each block is stored as a 16-bit value:

```text
bits 15..4  = block type id, 0..4095
bits  3..0  = metadata, 0..15
```

Encoding convention:

```text
raw = (type << 4) | metadata
```

Type `0` with metadata `0` means `Air`.

Example:

```cpp
Block air   = Block::make(0, 0);
Block grass = Block::make(1, 0);
Block dirt  = Block::make(2, 0);
```

Metadata can be used for:

* block rotation,
* block variant,
* active/inactive state,
* small local block state,
* gameplay-specific flags.

The 12-bit type id allows up to `4096` block types, which is enough for this project. The 4-bit metadata field allows `16` local states per block type.

## Runtime Chunk Data Model

The runtime `Chunk` object contains:

```cpp
std::vector<Block> palette;
std::vector<uint64_t> packedIndices;
std::vector<ChunkBlockMetadataRecord> extraMetadata;
uint8_t bitsPerBlock;
bool dirty;
```

`dirty` is not a persistent part of the file format. It is used only by the runtime layer to decide whether the chunk mesh or saved chunk payload needs to be rebuilt.

## Palette + Bit-Packing

Each block in a chunk is stored as an index into the local chunk palette.

Instead of storing this for every block:

```text
uint16_t block
```

the chunk stores:

```text
palette index -> palette entry -> block value
```

The number of bits per block is calculated from the palette size:

| Palette entries | Bits per block |
| --------------: | -------------: |
|            1..2 |              1 |
|            3..4 |              2 |
|            5..8 |              3 |
|           9..16 |              4 |
|          17..32 |              5 |
|          33..64 |              6 |
|         65..128 |              7 |
|        129..256 |              8 |

Example for a `10³` chunk with 4 block types:

```text
1000 blocks × 2 bits = 2000 bits = 250 bytes
palette 4 × 2 bytes = 8 bytes
```

Approximate chunk content size without headers:

```text
250 + 8 = 258 bytes
```

This is why palette-compressed chunks are very efficient for handcrafted voxel worlds.

## Empty Chunks

Empty chunks are skipped in the world file.

An empty chunk has:

```text
flags  = EMPTY
offset = 0
size   = 0
```

No palette and no packed block data are stored for empty chunks.

This is important because many 3D worlds contain large empty areas.

**"Empty" means both conditions hold, not just an all-air block palette:** a chunk with real
blocks is obviously not empty, but a chunk that is *entirely air* still counts as non-empty if it
carries any sparse extra metadata (see "Extra Metadata Section" below) — for example a `MoveObject`
anchored on an otherwise-bare chunk. Found and fixed as a real bug (2026-07-09): the runtime
`Chunk::isEmpty()` originally checked only the block palette, so `World::saveToFile()` silently
dropped a chunk's metadata whenever that chunk had no real blocks. `Chunk::isEmpty()` now also
requires `extraMetadata.empty()`.

## Chunk Payload `VCH1`

The chunk payload is used inside the world file, but it can also be serialized separately.

All numeric values are little-endian.

### Chunk Header

Current v2 layout:

| Offset | Size | Field              | Meaning                              |
| -----: | ---: | ------------------ | ------------------------------------ |
|      0 |    4 | magic              | ASCII `VCH1`                         |
|      4 |    1 | version            | `2`                                  |
|      5 |    1 | chunkSize          | `10`                                 |
|      6 |    1 | bitsPerBlock       | 1..8                                 |
|      7 |    1 | flags              | bit flags (see below)                |
|      8 |    2 | paletteCount       | 1..256                               |
|     10 |    2 | reserved           | must be `0`                          |
|     12 |    4 | blockCount         | `1000`                               |
|     16 |    4 | dataSizeBytes      | size of packed `uint64_t` stream     |
|     20 |    4 | extraMetaSizeBytes | size of optional sparse metadata     |

Header size in v2: 24 bytes.

Legacy v1 chunks use the previous 20-byte header without `extraMetaSizeBytes` and with `version = 1`.

Chunk flags:

* `0x00`: no flags
* `0x01`: has extra sparse metadata section
* other bits are reserved for future use

### Chunk Body

Immediately after the header:

```text
palette[paletteCount] as uint16_t raw block values
packedIndices[dataSizeBytes / 8] as uint64_t little-endian words
optional extraMetadata[extraMetaSizeBytes]
```

If no sparse metadata are present:

```text
flags bit 0 is not set
extraMetaSizeBytes = 0
```

### Extra Metadata Section `BMD1`

The optional sparse metadata section starts with:

| Size | Field       | Meaning                    |
| ---: | ----------- | -------------------------- |
|    4 | magic       | ASCII `BMD1`               |
|    2 | version     | `1`                        |
|    2 | reserved    | must be `0`                |
|    4 | recordCount | number of metadata records |

Then follow `recordCount` metadata records:

| Size | Field           | Meaning                  |
| ---: | --------------- | ------------------------ |
|    4 | localBlockIndex | local linear block index |
|    2 | metadataType    | metadata kind id         |
|    2 | payloadSize     | payload byte size        |
|    N | payload         | type-specific payload    |

Sparse metadata are attached to concrete local block positions, not only to block type ids.

Local block index convention:

```cpp
localBlockIndex = localX + localY * chunkSize + localZ * chunkSize * chunkSize;
```

The first palette index is stored in the least significant bits of the first `uint64_t` word.

### Real usage example: embedding MoveObjects (2026-07-09)

`GalaxyEggbert::MoveObjectRecord` (`include/GalaxyEggbert/MoveObjectRecord.hpp`, engine-agnostic)
is the first real consumer of sparse extra metadata: it encodes a moving/interactive object
(pickup, enemy, platform lift, crate — anything Galaxy Eggbert's `ObjectType` enum names) as a
47-byte payload (`[objectType: 1 byte][posStartX,Y,Z: 3× float32][posEndX,Y,Z: 3× float32]
[speed: float32][four patrol-timing float32 fields][visualIcon: uint16]`) under a single
reserved `metadataType = 1`, anchored at
`floor(posStartX/Y/Z)` — the anchor block only buckets the record for storage; the payload itself
carries the exact float position, so nothing is lost to the block grid's integer resolution.
The decoder also accepts the previous 45-byte form without `visualIcon`, defaulting it to zero;
zero means that rendering continues to derive the icon from object type and animation phase.
`World::setBlockExtraMetadata(x, y, z, metadataType, payload)` and
`World::collectExtraMetadata(metadataType)` are thin `World`-level wrappers (added the same day)
around `Chunk::setExtraMetadata`/`extraMetadata()` so callers don't have to compute chunk/local
indices themselves. See `PlaceMoveObject()`/`CollectMoveObjects()` in `MoveObjectRecord.cpp` for
the full encode/decode, and `WorldRuntime::LoadFromVwrFile()` (CNA) for how a loaded world turns
these back into renderable objects.

`GalaxyEggbert::BigDecorRecord` is the second world-level consumer. It uses
`metadataType = 3` (type 2 is already reserved for plate rotation), stores a
two-byte non-zero object-m.png icon, and derives its exact X/Y/Z anchor from
the surrounding sparse metadata record. It represents Eggbert 2's
non-colliding BigDecor billboard layer rather than a solid voxel.

### Hint zones (metadata type 4, 2026-10-25)

`GalaxyEggbert::HintZoneRecord` (`include/GalaxyEggbert/HintZoneRecord.hpp`) places one of a
mission's training hints in a 3D world. Mobile Eggbert ties each hint of its tables
(`Tables::table_training1-4`, ported in `TrainingHints.cpp`) to a column/row range of its 2D map,
which means nothing in Galaxy's new 3D layouts; the world now says where each hint applies,
while the hint's text and gate (treasure count, vehicle, dynamite) stay the original's.

| Size | Field | Meaning |
| ---: | --- | --- |
| 1 | recordIndex | index into the mission's hint table |
| 1 | sizeX | box size in cells, at least 1 |
| 1 | sizeY | |
| 1 | sizeZ | |

The record is anchored at the box's minimum corner; the box spans that cell plus `size - 1`
on each axis, inside the world. A world with any hint zone uses them (scanned in the table's
record order, first open gate wins, as the original scans its rects); a world without keeps the
2D rects. Compatibility: additive, no version change — readers that predate type 4 keep the record
in the chunk and ignore it, and every existing world (none has type 4) reads as before.
`tools/worldkit.py` writes zones (`World.hint_zone`), `tools/vwr_inspect.py` lists them, and
`VerifyShippedWorlds` checks they survive load → save → load.

### Turns (metadata types 2 and 5, 2026-11-04)

Type 2 (`kPlateRotationMetadataType`, `src/GalaxyEggbert/Game/PlateRotationMetadata.hpp`) is a
cell's tile turn, one byte: bit0 swaps a plate's, a fan's or a wind tile's axis (X and Z,
M3D-0005). The stored byte is0-3, with its odd bit read for that axis. Worlds saved before
carry0 or1 and read as before. Front-facing models use type5, so a sign's older plate bit
does not rotate its 3D model too.

Type 5 (`kObjectTurnMetadataType`) is the quarter turns, one byte 0-3, of the object (a still
MoveObject) or BigDecor anchored in that cell: drawn turned are the still objects, the parked
vehicles and the BigDecor models; a moving object keeps the facing of its travel. A type of its
own, so turning a treasure that lies in a wind cell does not turn the wind. Both are set by the
editor's O key. Compatibility: additive, as type 4.

The editor also stores front-facing terrain models and single-face cubes (e.g. a house
window) in type5, separately from the plate/fan/wind axis bit in type2.

### Legacy switch row (metadata type 6, 2026-11-08)

Type6 is a one-byte payload `{1}` used only when upgrading an old implicit saw-floor file.
The switch stays in its original cell, while its saw row moves up one cell. Its existing
X±20/Z link therefore uses Y+1. New switches use their own Y and have no such record.
Editor copy/paste and its Undo/Redo carry this record with the switch.

## World File `VWR1`

The world file contains:

```text
WORLD HEADER
CHUNK TABLE
CHUNK DATA...
```

## World Header

**Breaking change (2026-07-09):** header bumped v1 -> v2 to add a world-level
`skyRegion` field (background/sky selector, see "World-level sky region"
below) plus 4 world-level metadata fields. Offset 24 now stores the mission
number; offsets 28-36 store the optional spawn cell. This is a deliberate,
non-backward-compatible break, not a silent reinterpretation of the old
reserved bytes: `VoxelConfig::FormatVersion` is now `2`, and
`World::loadFromFile` rejects any file whose header `version` byte isn't
exactly `2` with `std::runtime_error` — **old v1 `.vwr` files no longer
load and must be regenerated** (e.g. via `tools/GenerateSampleWorld3D.cpp`).
This mirrors `Chunk`'s own independently-versioned payload format bump
(v1 -> v2, "Chunk Payload" above) in spirit, but is a hard cutover rather
than dual v1/v2 support, since `.vwr` world files (unlike chunk payloads
nested inside them) are meant to be regenerated from source data, not
archived indefinitely.

Current v2 layout:

| Offset | Size | Field            | Meaning                                                  |
| -----: | ---: | ---------------- | --------------------------------------------------------- |
|      0 |    4 | magic            | ASCII `VWR1`                                               |
|      4 |    1 | version          | `2`                                                         |
|      5 |    1 | chunkSize        | `10`                                                        |
|      6 |    1 | chunksPerAxis    | usually `10`                                                |
|      7 |    1 | flags            | bit0: independent saw cells; other bits reserved             |
|      8 |    4 | chunkCount       | `chunksPerAxis³`                                            |
|     12 |    4 | chunkTableOffset | usually `40` (the v2 header size)                           |
|     16 |    4 | chunkDataOffset  | start of the chunk data section                             |
|     20 |    4 | skyRegion        | low byte: sky/background region id 0-31 (see below); second byte: music track 0-10 (2026-11-02) |
|     24 |    4 | missionNumber    | world mission id; `0` means no mission                      |
|     28 |    4 | spawnXPlusOne    | raw-grid spawn X + 1; all three spawn fields `0` means unset |
|     32 |    4 | spawnYPlusOne    | raw-grid spawn Y + 1                                         |
|     36 |    4 | spawnZPlusOne    | raw-grid spawn Z + 1                                         |

Since 2026-11-08 new worlds set flag bit0 (`1`): a saw occupies its own cell and generates
no terrain. Older v2 files have flags0. The low-level reader preserves those bytes/data;
WorldRuntime upgrades them in memory before editing/play: an unoccupied cell above an old
saw receives the blade, and the formerly invented floor becomes an explicit block. Plate
orientation and switch-row links are preserved. Occupied cells above are never overwritten;
those blades stay independent in their existing cells. Saving the upgraded world sets bit0,
so conversion does not repeat. Loading alone never rewrites a player file. Historical world
builders opt into `legacy_saw_floors=True`; new worldkit worlds default to independent cells.

Header size: 40 bytes (up from 32 in v1).

Legacy v1 layout, for historical reference only (no longer readable):

| Offset | Size | Field            | Meaning                         |
| -----: | ---: | ---------------- | ------------------------------- |
|      0 |    4 | magic            | ASCII `VWR1`                    |
|      4 |    1 | version          | `1`                             |
|      5 |    1 | chunkSize        | `10`                            |
|      6 |    1 | chunksPerAxis    | usually `10`                    |
|      7 |    1 | flags            | `0` in v1                       |
|      8 |    4 | chunkCount       | `chunksPerAxis³`                |
|     12 |    4 | chunkTableOffset | usually `32`                    |
|     16 |    4 | chunkDataOffset  | start of the chunk data section |
|     20 |    4 | reserved         | must be `0`                     |
|     24 |    4 | reserved         | must be `0`                     |
|     28 |    4 | reserved         | must be `0`                     |

### World-level sky region

`skyRegion` (`World::skyRegion()`/`setSkyRegion()`) is a direct pass-through
of mobile-eggbert's level-header `region=` field (0-31,
`mobile-eggbert-reference/05-backgrounds.md`) — it selects
`Content/backgrounds/decorNNN.png` as this world's background image
(`GalaxyEggbertGame`, NEXT.md §3, 2026-07-09). Defaults to `0` for worlds
that never call `setSkyRegion` (including every world loaded from a v1 file,
though those no longer load at all post-break). Only 28 of the 32 possible
ids (0-31) have a real background file — a hand-authored world referencing
one of the 4 missing ids, or any value a real level never uses, degrades
gracefully to a flat fallback clear color at render time rather than
failing to load.

### World background music

The second byte of the `skyRegion` word (bits 8-15) is the world's background music
(`World::musicTrack()`/`setMusicTrack()`, 2026-11-02, the owner's OR-0014): `0` none, `1`-`10`
the original Speedy Eggbert tracks `SOUND/MUSIC000-009.BLP` (MIDI), rendered to
`Content/music/music000-009.ogg` by `tools/music/render_music.py`. Worlds written before carry
`0` there, so they read as "no music"; the upper two bytes stay `0`. Chosen per world in the 3D
editor.

### Optional Blupi spawn point

`World::setSpawnPoint(x,y,z)` stores the exact raw-grid cell selected by the
3D editor's Level start tool. Coordinates are encoded plus one so the
all-zero header written by older v2 builds remains unambiguously “unset”;
callers then retain their historical default spawn. A present spawn must
have all three fields non-zero after encoding and remain inside the world,
otherwise loading rejects the malformed file. Runtime presentation shifts
X/Z by the normal world-centre offset and leaves Y unchanged.

### Independent coloured player starts (2026-11-08)

The editor has four independent starts: yellow/orange/blue/green (ranks 0/1/2/3).
The owner explicitly chose independent starts in custom worlds; unlike the original custom-world
editor's rank-0 aliasing, placing orange never moves yellow. Normal single-player gameplay
continues to use only the existing yellow header spawn and its historical unset fallback.

Yellow keeps the three existing header fields. Orange/blue/green use sparse `metadataType = 7`,
anchored at the exact raw-grid start cell. The payload is one non-zero byte: bit1 = orange,
bit2 = blue, bit3 = green; other bits must be zero. Several colours can share one cell. Each
colour may appear at most once in the world. `PlayerStart.hpp` collects, finds, moves and clears
these records while preserving other colours at the same cell. Moving a colour removes only
its old bit; removing the last bit removes the metadata record. The existing generic metadata
serialization preserves these records without changing header size or version. Old files have
no extra coloured starts. They are editor data, never MoveObjects or hazards. Clipboard stamps
continue to omit player starts of every colour, as they already omitted the header spawn.

## Recommended Future World Header

**Note (2026-07-09):** this section is still a hypothetical, unbuilt future
version aimed at non-cubic worlds specifically. The actual first breaking
header change that shipped took a smaller, unrelated route (see "World
Header" above) -- reusing/expanding the v1 header's existing reserved
fields for `skyRegion` and future world-level metadata, not the
`chunksX`/`chunksY`/`chunksZ` restructuring described below. Both are valid
independent future directions; they aren't in conflict, just don't confuse
this section's "v2" with the real, already-shipped header v2 above.

For bigger or non-cubic worlds, a future version should store chunk counts per axis:

```cpp
struct WorldHeader {
    char magic[4];          // VWR1
    uint8_t version;        // 2
    uint8_t chunkSize;      // 10
    uint16_t flags;

    uint16_t chunksX;
    uint16_t chunksY;
    uint16_t chunksZ;
    uint16_t reserved0;

    uint32_t chunkCount;
    uint64_t chunkTableOffset;
    uint64_t chunkDataOffset;
};
```

This allows worlds such as:

```text
100 × 100 × 100 blocks     = 10 × 10 × 10 chunks
200 × 100 × 200 blocks     = 20 × 10 × 20 chunks
500 × 100 × 500 blocks     = 50 × 10 × 50 chunks
1000 × 100 × 1000 blocks   = 100 × 10 × 100 chunks
```

## Chunk Table Entry

Each chunk table entry has 20 bytes:

| Offset | Size | Field  | Meaning                                   |
| -----: | ---: | ------ | ----------------------------------------- |
|      0 |    8 | offset | absolute file offset of the chunk payload |
|      8 |    4 | size   | chunk payload size in bytes               |
|     12 |    2 | x      | chunk X                                   |
|     14 |    2 | y      | chunk Y                                   |
|     16 |    2 | z      | chunk Z                                   |
|     18 |    2 | flags  | `0x0001 = EMPTY`                          |

If a chunk is empty, `flags` contains `EMPTY`, `offset = 0`, and `size = 0`.

## Runtime Architecture

Recommended layers:

### 1. Voxel Data Layer

Responsible for persistent voxel data:

* `Block`,
* `Chunk`,
* `World`,
* palette handling,
* bit-packing,
* binary read/write.

### 2. Mesh Layer

Responsible for rendering data generated from chunks:

* one mesh per visible chunk,
* rebuild only dirty chunks,
* generate only exposed faces,
* later replace simple face generation with greedy meshing.

### 3. Editor Layer

Responsible for world editing tools:

* brush,
* erase,
* fill,
* box fill,
* layer view,
* chunk debug view.

### 4. Game Layer

Responsible for gameplay logic:

* collision,
* gameplay objects,
* trigger blocks,
* player view,
* level logic.

## Editor Notes

The editor should not edit packed bits directly.

Correct flow:

```text
SetBlock(worldX, worldY, worldZ, block):
  chunkX = worldX / 10
  chunkY = worldY / 10
  chunkZ = worldZ / 10

  localX = worldX % 10
  localY = worldY % 10
  localZ = worldZ % 10

  ensure block is in chunk palette
  if palette grew across power-of-two boundary:
    repack chunk indices with larger bitsPerBlock

  write palette index into packed data
  mark chunk dirty
```

Undo/redo should store logical block operations, not binary diffs.

Example:

```cpp
struct SetBlockOperation {
    uint16_t x;
    uint16_t y;
    uint16_t z;
    Block oldBlock;
    Block newBlock;
};
```

## Mesh Generation

The binary world format should not store render meshes in v1.

The mesh is a runtime cache generated from block data.

Basic algorithm:

```text
for each solid block:
  for each of 6 directions:
    if neighbor is air or outside world:
      emit one quad
```

**Implemented (2026-07-09)** for `GalaxyEggbertCNA`'s static (non-animated) terrain path —
`TerrainRenderer::IsOccluderBlock()` is a conservative version of the "neighbor is air or
outside world" check above (a face is also kept, not culled, if the neighbor uses a
holed/special-geometry render mode or needs alpha blending, since those aren't guaranteed to
fully cover the shared face). Cut the default sample world's terrain mesh from 67788 to 23012
vertices (66%). The animated/water paths do not use this yet — deliberately scoped out, since
those tend to be sparse decorative elements rather than the bulk fills this optimization targets
most.

Later optimizations:

* greedy meshing,
* frustum culling,
* one draw call per chunk,
* background rebuild for dirty chunks.

## Compression Roadmap

Compression is not required in v1.

The current pipeline is:

```text
Block -> Chunk palette -> bit-packed palette indices -> World file
```

Future pipeline may become:

```text
Block -> Chunk palette -> palette indices -> RLE -> bit-packing -> LZ4/Zstd -> World file
```

Recommended first compression extension:

```cpp
struct RleRun {
    uint16_t count;
    uint16_t paletteIndex;
};
```

RLE should be applied before bit-packing, over the logical sequence of palette indices.

## Future Extensions

The format reserves flags for future versions:

* RLE per chunk,
* LZ4/Zstd compression per chunk,
* global palette deduplication,
* procedural generation + delta storage,
* sparse sub-chunks,
* octree representation,
* region files for bigger worlds,
* non-cubic world dimensions,
* larger worlds with `chunksX`, `chunksY`, and `chunksZ`.

For the default `100³` world, v1 without RLE is already sufficient.
