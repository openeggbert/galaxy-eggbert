#pragma once

#include "GalaxyEggbert/Worlds/World.hpp"
#include "GalaxyEggbert/Def/ObjectType.hpp"

#include <cstdint>
#include <vector>

namespace GalaxyEggbert {

// One MoveObject placement (pickup, enemy, platform lift, crate, ...)
// embedded directly in the 3D `.vwr` world format, via Worlds::World's
// block-extra-metadata mechanism (Worlds::Chunk::ChunkBlockMetadataRecord) --
// not a separate top-level file section. Mirrors GalaxyEggbertCNA's
// MobileObjSpec shape for mobile-eggbert .txt-loaded MoveObject: lines
// (type, posStart,
// posEnd, speed, plus the real patrol-timing fields below, 2026-07-11 --
// posEnd == posStart means the object doesn't move, matching the real
// guard). Kept engine-agnostic (plain floats, no CNA/XNA vector types) so
// world serialization and the active renderer remain cleanly separated.
//
// IMPORTANT: positions here are in Worlds::World's own RAW GRID space
// (range [0, world.blocksPerAxis()), same as World::setBlock's x/y/z) --
// NOT the "-kWorldCenterX/Z"-shifted render/camera space WorldRuntime's
// own MobileObjSpec/BigDecor conversions apply. That shift is a
// presentation-layer concern (WorldRuntime::kWorldCenterX/Z); this struct
// and PlaceMoveObject/CollectMoveObjects stay agnostic to it, matching
// every other Worlds::World coordinate in this codebase. Y identifies the
// center of the occupied voxel cell: an object standing on a solid block at
// Y=0 therefore has Y=1. Renderers must not add another vertical block.
struct MoveObjectRecord final {
    GalaxyEggbert::Def::ObjectType type = GalaxyEggbert::Def::ObjectType::ObjectType0;
    // Optional object-m.png tile override for placed variants whose visual
    // identity cannot be derived from GalaxyEggbert::Def::ObjectType alone. Zero uses the
    // normal GetObjIcon(type, phase) mapping. Eggbert 2's secret wooden
    // case uses this to retain its terrain camouflage across save/load.
    std::uint16_t visualIcon = 0;
    float posStartX = 0.0f, posStartY = 0.0f, posStartZ = 0.0f;
    float posEndX = 0.0f, posEndY = 0.0f, posEndZ = 0.0f;
    float speed = 1.5f;

    // Real shared patrol-turn timing fields (plan.md E3D-MIG-131,
    // `Decor::MoveObjectStepLine` per mobile-eggbert-reference/
    // 04-enemy-behavior.md/01-world-file-format.md §4, verified directly
    // against Decor.cpp:8005-8141): a 4-phase cycle -- dwell at posStart
    // for timeStopStartTicks, advance to posEnd over stepAdvanceTicks,
    // dwell at posEnd for timeStopEndTicks, recede back over
    // stepRecedeTicks, then loop. Ticks are at the real 20Hz reference
    // rate (same convention as MobileObjSpec::phase). Real values are
    // level-authored per placed instance, not a per-type constant
    // (01-world-file-format.md's own field table) -- these defaults (2s
    // dwell, 3s traversal) are a reasonable placeholder for hand-authored
    // .vwr worlds that don't set them explicitly, not a transcribed real
    // constant.
    float stepAdvanceTicks = 60.0f;
    float stepRecedeTicks = 60.0f;
    float timeStopStartTicks = 40.0f;
    float timeStopEndTicks = 40.0f;
};

// metadataType discriminator reserved for MoveObjectRecord payloads in
// Worlds::World's block-extra-metadata records. Every MoveObjectRecord in a
// world uses this same value; the payload itself (not the metadataType)
// distinguishes which GalaxyEggbert::Def::ObjectType it is.
constexpr std::uint16_t kMoveObjectMetadataType = 1;

// Places @p record in @p world, anchored at floor(posStartX/Y/Z) -- the
// anchor block only buckets the record for storage; the exact position stays
// float-precise in the encoded payload. At most one MoveObjectRecord may be
// anchored to the exact same block (Worlds::Chunk::setExtraMetadata's
// (localBlockIndex, metadataType) replace semantics -- placing a second
// record on the same block silently replaces the first).
//
// @throws std::out_of_range If the anchor block is outside @p world's bounds.
void PlaceMoveObject(Worlds::World& world, const MoveObjectRecord& record);

// Collects every MoveObjectRecord stored in @p world (from any chunk).
[[nodiscard]] std::vector<MoveObjectRecord> CollectMoveObjects(const Worlds::World& world);

// Removes whichever MoveObjectRecord is anchored at raw-grid-space cell
// (x,y,z), if any (plan.md EDITOR-109/110 -- the in-game world editor's
// object-removal tool). Returns true when a record was actually removed.
bool RemoveMoveObject(Worlds::World& world, std::uint16_t x, std::uint16_t y, std::uint16_t z);

}
