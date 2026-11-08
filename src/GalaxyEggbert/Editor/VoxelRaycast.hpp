#pragma once

#include <GalaxyEggbert/Worlds/World.hpp>
#include <GalaxyEggbert/Worlds/GridRaycast.hpp>

#include <cstdint>

namespace GalaxyEggbert::Editor
{
    // Result of a Raycast() call (see below).
    using RaycastHit = Worlds::GridRaycastHit;

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
