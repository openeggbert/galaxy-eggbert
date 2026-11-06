#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace GalaxyEggbert::Game
{
    // An axis-aligned box solid for Blupi, raw grid coordinates (X and Z cell centres on integers,
    // collision Y: a cell gy owns [gy, gy + 1)). The large BigDecor stand in his way as boxes
    // fitted to their models (Q-014, owner 2026-11-05: the pink house was walked through).
    struct SolidBox
    {
        float minX = 0.0f, maxX = 0.0f;
        float minY = 0.0f, maxY = 0.0f;
        float minZ = 0.0f, maxZ = 0.0f;

        [[nodiscard]] bool Contains(float x, float y, float z) const noexcept
        {
            return x > minX && x < maxX && y >= minY && y < maxY && z > minZ && z < maxZ;
        }
    };

    // A BigDecor model's shape as drawn, in boxes relative to its anchor cell: X and Z from the
    // cell's centre, Y from its floor, in blocks, before the cell's own quarter turns. The game
    // measures them from the models (one box per half-block slice of height: a palm's trunk is
    // thin, its crown wide) and hands them to WorldRuntime::SetBigDecorShapes.
    using BigDecorShapes = std::unordered_map<std::uint16_t, std::vector<SolidBox>>;
}
