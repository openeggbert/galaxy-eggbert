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
fixed 29-byte payload (`[objectType: 1 byte][posStartX,Y,Z: 3× float32][posEndX,Y,Z: 3× float32]
[speed: float32]`) under a single reserved `metadataType = 1`, anchored at
`floor(posStartX/Y/Z)` — the anchor block only buckets the record for storage; the payload itself
carries the exact float position, so nothing is lost to the block grid's integer resolution.
`World::setBlockExtraMetadata(x, y, z, metadataType, payload)` and
`World::collectExtraMetadata(metadataType)` are thin `World`-level wrappers (added the same day)
around `Chunk::setExtraMetadata`/`extraMetadata()` so callers don't have to compute chunk/local
indices themselves. See `PlaceMoveObject()`/`CollectMoveObjects()` in `MoveObjectRecord.cpp` for
the full encode/decode, and `GEWorldRuntime::LoadFromVwrFile()` (CNA) for how a loaded world turns
these back into renderable objects.

## World File `VWR1`

The world file contains:

```text
WORLD HEADER
CHUNK TABLE
CHUNK DATA...
```

## World Header

Current v1 layout:

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

Header size: 32 bytes.

## Recommended Future World Header

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
`GETerrainRenderer::IsOccluderBlock()` is a conservative version of the "neighbor is air or
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

