#pragma once

#include "GalaxyEggbert/Worlds/World.hpp"
#include "GalaxyEggbert/def/ObjectType.hpp"

#include <cstdint>
#include <vector>

namespace GalaxyEggbert {

// One MoveObject placement (pickup, enemy, platform lift, crate, ...)
// embedded directly in the 3D `.vwr` world format, via Worlds::World's
// block-extra-metadata mechanism (Worlds::Chunk::ChunkBlockMetadataRecord) --
// not a separate top-level file section. Mirrors the simplified shape both
// GalaxyEggbertSimple3D's and GalaxyEggbertCNA's own MobileObjSpec already
// use for mobile-eggbert .txt-loaded MoveObject: lines (type, posStart,
// posEnd, speed -- no separate step/timing fields; posEnd == posStart means
// the object doesn't move). Kept engine-agnostic (plain floats, no CNA/XNA
// or Simple3D/U3D vector types) so both targets can convert it into their
// own local MobileObjSpec-equivalent struct.
//
// IMPORTANT: positions here are in Worlds::World's own RAW GRID space
// (range [0, world.blocksPerAxis()), same as World::setBlock's x/y/z) --
// NOT the "-kWorldCenterX/Z"-shifted render/camera space GEWorldRuntime's
// own MobileObjSpec/BigDecor conversions apply. That shift is a
// presentation-layer concern (GEWorldRuntime::kWorldCenterX/Z); this struct
// and PlaceMoveObject/CollectMoveObjects stay agnostic to it, matching
// every other Worlds::World coordinate in this codebase.
struct MoveObjectRecord final {
    ObjectType type = ObjectType::ObjectType0;
    float posStartX = 0.0f, posStartY = 0.0f, posStartZ = 0.0f;
    float posEndX = 0.0f, posEndY = 0.0f, posEndZ = 0.0f;
    float speed = 1.5f;
};

// metadataType discriminator reserved for MoveObjectRecord payloads in
// Worlds::World's block-extra-metadata records. Every MoveObjectRecord in a
// world uses this same value; the payload itself (not the metadataType)
// distinguishes which ObjectType it is.
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

}
