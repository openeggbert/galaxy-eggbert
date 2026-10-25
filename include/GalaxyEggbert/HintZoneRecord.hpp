#pragma once

#include "GalaxyEggbert/Worlds/World.hpp"

#include <cstdint>
#include <vector>

namespace GalaxyEggbert {

// Where one of a mission's training hints shows in a 3D world (2026-10-25,
// World Format.md "Hint zones"). Mobile Eggbert ties each hint of its tables
// (Tables::table_training1-4, TrainingHints.cpp) to a column/row range of its
// 2D map; Galaxy's worlds are new 3D designs, so the world itself says where
// each hint applies: the box from (x, y, z) to (x + sizeX - 1, ...), inclusive,
// in raw grid cells, for hint `recordIndex` of the mission's table. The hint's
// text and gate (treasure count, vehicle, dynamite) stay the original's.
struct HintZoneRecord final {
    std::uint8_t recordIndex = 0;
    std::uint16_t x = 0;
    std::uint16_t y = 0;
    std::uint16_t z = 0;
    std::uint8_t sizeX = 1;
    std::uint8_t sizeY = 1;
    std::uint8_t sizeZ = 1;

    [[nodiscard]] bool Contains(int cellX, int cellY, int cellZ) const noexcept {
        return cellX >= x && cellX < x + sizeX && cellY >= y && cellY < y + sizeY &&
               cellZ >= z && cellZ < z + sizeZ;
    }
};

// Types 1-3 are MoveObject, plate rotation and BigDecor. A 4-byte payload
// [recordIndex, sizeX, sizeY, sizeZ] anchored at the zone's minimum corner.
// Readers that predate it keep the record and ignore it.
constexpr std::uint16_t kHintZoneMetadataType = 4;

// Places or replaces the zone anchored at record.x/y/z.
// @throws std::out_of_range when the anchor is outside the world.
// @throws std::invalid_argument when a size is zero or the box leaves the world.
void PlaceHintZone(Worlds::World& world, const HintZoneRecord& record);

// Every hint zone of the world, sorted by recordIndex (the original tables'
// scan order, see FindTrainingHintInZones()).
[[nodiscard]] std::vector<HintZoneRecord> CollectHintZones(const Worlds::World& world);

}
