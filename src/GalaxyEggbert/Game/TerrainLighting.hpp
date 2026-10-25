#pragma once

#include <algorithm>
#include <cmath>

namespace GalaxyEggbert::Game
{
    // The terrain's light (2026-10-25). Until then the terrain was drawn unlit, every face of a
    // block exactly as bright as every other, so in a world built mostly of one tile (the blue
    // machine panel) walls, steps and edges dissolved into one flat colour. One directional
    // light from above and over the left shoulder of a camera looking along -Z (the 2D tiles'
    // own top-left light) plus ambient: top faces keep the tile's own colours, the two faces
    // turned to the light are a little darker, the rest darker still. Used for the terrain, its
    // grass overlay and the cube objects standing in it (lifts, crates).
    struct TerrainLight
    {
        float ambient;
        float diffuse;
        float directionX, directionY, directionZ; // the way the light travels
    };

    inline constexpr TerrainLight kTerrainLight{0.63f, 0.42f, 0.3f, -1.0f, -0.45f};

    // The brightness BasicEffect gives a face with unit normal (nx, ny, nz) under kTerrainLight.
    [[nodiscard]] inline float TerrainFaceBrightness(float nx, float ny, float nz) noexcept
    {
        const auto& light = kTerrainLight;
        const float length = std::sqrt(light.directionX * light.directionX + light.directionY * light.directionY +
                                       light.directionZ * light.directionZ);
        const float facing = -(light.directionX * nx + light.directionY * ny + light.directionZ * nz) / length;
        return light.ambient + light.diffuse * std::max(0.0f, facing);
    }
}
