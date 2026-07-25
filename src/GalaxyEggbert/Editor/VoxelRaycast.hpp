#pragma once

#include <GalaxyEggbert/Worlds/World.hpp>

#include <cstdint>

namespace GalaxyEggbert::Editor
{
    // Result of a Raycast() call (see below).
    struct RaycastHit
    {
        bool hit = false;
        std::uint16_t x = 0;
        std::uint16_t y = 0;
        std::uint16_t z = 0;

        // Outward face normal of the hit cell, i.e. the side the ray
        // arrived from -- (hitCell + normal) is the adjacent, currently-air
        // cell a "place block" tool should write to; the hit cell itself
        // is what a "remove block" tool should clear. All-zero if the ray's
        // OWN ORIGIN started inside a solid block (no face was crossed).
        std::int8_t normalX = 0;
        std::int8_t normalY = 0;
        std::int8_t normalZ = 0;

        float distance = 0.0f;
    };

    // Amanatides-Woo voxel DDA raycast against @p world's dense block grid,
    // stopping at the first non-air cell within @p maxDistance.
    //
    // @p originX/Y/Z and @p dirX/Y/Z are in @p world's own RAW GRID space
    // (same convention as World::getBlock's x/y/z arguments, [0,
    // blocksPerAxis())) -- NOT the "-kWorldCenterX/Z"-shifted render space
    // TerrainRenderer/WorldRuntime use for drawing (that shift is a
    // presentation-layer concern the caller must apply/reverse itself, same
    // convention as GalaxyEggbert::MoveObjectRecord). @p dirX/Y/Z need not
    // be pre-normalized -- this function normalizes defensively.
    //
    // Internally accounts for this engine's own block-centering convention
    // (confirmed via TerrainRenderer.cpp/WorldRuntime.cpp: a block with
    // integer grid index N is rendered/collided as spanning [N-0.5, N+0.5),
    // i.e. continuous-to-index conversion elsewhere in this engine always
    // rounds via std::lround, never floors) -- callers do not need to
    // apply any offset themselves.
    [[nodiscard]] RaycastHit Raycast(const Worlds::World& world,
                                      float originX, float originY, float originZ,
                                      float dirX, float dirY, float dirZ,
                                      float maxDistance);
}
