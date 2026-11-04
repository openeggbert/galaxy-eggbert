#pragma once

#include <GalaxyEggbert/Worlds/World.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace GalaxyEggbert::Game
{
    // Per-PLACEMENT 90-degree rotation for InnerFlatPlate icons whose axis
    // needs to vary by placement (currently only Saw/SawStopped, see
    // InnerFlatPlateTiles.hpp's GetInnerFlatPlateAxis()) rather than a
    // single fixed per-icon default -- a Saw mounted in a Z-running
    // corridor needs the opposite axis from one in an X-running corridor,
    // and real mobile-eggbert's 2D sprite has no axis concept at all to
    // derive a "correct" per-icon default from (2026-07-11, user feedback).
    // Deliberately engine-agnostic (only depends on GalaxyEggbert::Worlds,
    // no cna-engine) so world-authoring tools (e.g. tools/GenerateSampleWorld3D.cpp)
    // can set this metadata without linking cna-engine/CNA, matching
    // BlupiController/WorldRuntime's own established precedent for
    // this tree. Stored as sparse block extra-metadata
    // (Worlds::World::setBlockExtraMetadata), 1-byte payload: 0 (or
    // absent) = the icon's own default axis, 1 = rotated 90 degrees
    // (X<->Z swap; PlateAxis::Y is left unchanged, since no
    // rotation-eligible horizontal-plate icon exists yet).
    constexpr std::uint16_t kPlateRotationMetadataType = 2; // kMoveObjectMetadataType (MoveObjectRecord.hpp) == 1

    // Sets whether the block at (x, y, z) renders rotated 90 degrees, IF it
    // later turns out to be an InnerFlatPlate icon (a no-op flag otherwise,
    // just stored data) -- safe to call before or after the block itself is
    // placed. World-space (raw grid) coordinates, same convention as
    // Worlds::World::setBlock().
    void SetPlateRotated(Worlds::World& world, std::uint16_t x, std::uint16_t y, std::uint16_t z, bool rotated);

    // Every block position across the whole world with rotation metadata
    // set to true -- callers (TerrainRenderer) collect this ONCE and
    // build their own fast lookup, rather than querying per-block on every
    // render rebuild (metadata is sparse: real worlds have at most a
    // handful of rotated plates, not one per block).
    [[nodiscard]] std::vector<std::array<std::uint16_t, 3>> CollectRotatedPlatePositions(const Worlds::World& world);

    // Whether the block at (x, y, z) is turned 90 degrees: a plate's axis, and (M3D-0005, owner
    // 2026-11-02) a fan's or wind tile's horizontal way, -X/+X becoming -Z/+Z.
    [[nodiscard]] bool IsPlateRotated(const Worlds::World& world, std::uint16_t x, std::uint16_t y, std::uint16_t z);
    // The same byte read as quarter turns, 0-3 (OR-0071, owner 2026-11-04: the editor turns a
    // model with a front -- teleporter, sign, switch, balustrade, trestle, emitter -- a quarter at a
    // time from the side it faces by itself). A plate, fan or wind tile reads only its odd bit (90
    // or 270 degrees: X and Z swapped), so the worlds saved with 0/1 read as before.
    [[nodiscard]] int QuarterTurns(const Worlds::World& world, std::uint16_t x, std::uint16_t y, std::uint16_t z);
    void SetQuarterTurns(Worlds::World& world, std::uint16_t x, std::uint16_t y, std::uint16_t z, int turns);

    // An object's or a BigDecor's quarter turns, 0-3, at the cell it stands in (OR-0071): a type of
    // its own, so that turning a treasure in a wind cell does not turn the wind.
    constexpr std::uint16_t kObjectTurnMetadataType = 5; // 1 objects, 2 plate turns, 3 BigDecor, 4 hints
    [[nodiscard]] int ObjectQuarterTurns(const Worlds::World& world, std::uint16_t x, std::uint16_t y, std::uint16_t z);
    void SetObjectQuarterTurns(Worlds::World& world, std::uint16_t x, std::uint16_t y, std::uint16_t z, int turns);
}
