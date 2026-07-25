#pragma once

#include <cstdint>

namespace GalaxyEggbert::Editor
{
    // Inclusive axis-aligned block-index range, raw grid space (see
    // VoxelRaycast.hpp) -- both min* and max* are valid, in-bounds cells
    // to fill/scan, not a half-open [min,max) range.
    struct BoxRegion
    {
        std::uint16_t minX = 0;
        std::uint16_t minY = 0;
        std::uint16_t minZ = 0;
        std::uint16_t maxX = 0;
        std::uint16_t maxY = 0;
        std::uint16_t maxZ = 0;
    };

    // Normalizes two arbitrary (possibly unordered, possibly
    // out-of-world-bounds) corner cells into an inclusive BoxRegion clamped
    // to [0, blocksPerAxis) on every axis. Two independent raycasts
    // naturally produce corners from different camera angles/distances --
    // this makes a true 3D cuboid (arbitrary Y-span too) out of whichever
    // order/values they came in.
    [[nodiscard]] BoxRegion NormalizeAndClamp(int x0, int y0, int z0,
                                               int x1, int y1, int z1,
                                               int blocksPerAxis);
}
