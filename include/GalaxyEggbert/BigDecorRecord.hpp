#pragma once

#include "GalaxyEggbert/Worlds/World.hpp"

#include <cstdint>
#include <vector>

namespace GalaxyEggbert {

// One non-colliding explo.png billboard anchored to an exact 3D world cell.
// Eggbert 2 draws BigDecor through CHEXPLO (despite that channel's name);
// the numeric icon vocabulary is therefore not object-m.png's. This is the
// `.vwr` counterpart of Eggbert 2's separate BigDecor: layer; it is
// deliberately not a voxel block and never affects collision.
struct BigDecorRecord final {
    std::uint16_t icon = 0;
    std::uint16_t x = 0;
    std::uint16_t y = 0;
    std::uint16_t z = 0;
};

// Metadata type 2 is already used by CNA plate rotation. Type 3 is reserved
// globally for BigDecor records.
constexpr std::uint16_t kBigDecorMetadataType = 3;

// Places or replaces the BigDecor billboard anchored at record.x/y/z.
// @throws std::out_of_range when the anchor is outside the world.
// @throws std::invalid_argument when icon is zero.
void PlaceBigDecor(Worlds::World& world, const BigDecorRecord& record);

// Collects every BigDecor record, resolving its anchor from chunk metadata.
[[nodiscard]] std::vector<BigDecorRecord> CollectBigDecor(const Worlds::World& world);

// Removes the BigDecor billboard anchored at x/y/z, if present.
bool RemoveBigDecor(
    Worlds::World& world, std::uint16_t x, std::uint16_t y, std::uint16_t z);

}
