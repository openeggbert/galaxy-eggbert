#pragma once

#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>
#include <CnaEngine/Graphics/MeshBuilder.hpp>

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace GalaxyEggbert::Game
{
    // View-only immersion at a point in grid coordinates (cell centres are integers). The
    // camera, rather than Blupi's swimming state, decides whether the view is underwater.
    // Fade across the last 0.35 block below the surface instead of flashing at the waterline.
    [[nodiscard]] inline float WaterViewImmersion(const Worlds::World& world, float x, float y, float z)
    {
        const int size = static_cast<int>(world.blocksPerAxis());
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) ||
            x < -0.5f || y < -0.5f || z < -0.5f ||
            x >= size - 0.5f || y >= size - 0.5f || z >= size - 0.5f) return 0.0f;
        const int gx = static_cast<int>(std::floor(x + 0.5f));
        const int gy = static_cast<int>(std::floor(y + 0.5f));
        const int gz = static_cast<int>(std::floor(z + 0.5f));
        if (!GetBlockDefinition(world.getBlock(gx, gy, gz).type()).water) return 0.0f;
        if (gy + 1 < size && GetBlockDefinition(world.getBlock(gx, gy + 1, gz).type()).water) return 1.0f;
        return std::clamp((float(gy) + 0.5f - y) / 0.35f, 0.0f, 1.0f);
    }

    // Adjacent water cells form one volume. Drawing their internal faces stacked the original
    // translucent tile many times in front of a swimmer, obscuring Blupi, fish and the route.
    // Keep the animated exterior, including the surface seen from above, without internal walls.
    [[nodiscard]] inline std::uint8_t WaterSurfaceFaces(const Worlds::World& world, int x, int y, int z)
    {
        using CnaEngine::Graphics::BoxFace;
        struct Side { BoxFace face; int dx, dy, dz; };
        static constexpr std::array<Side, 6> sides{{
            {BoxFace::PositiveZ, 0, 0, 1}, {BoxFace::NegativeZ, 0, 0, -1},
            {BoxFace::PositiveX, 1, 0, 0}, {BoxFace::NegativeX, -1, 0, 0},
            {BoxFace::PositiveY, 0, 1, 0}, {BoxFace::NegativeY, 0, -1, 0}}};
        const int size = static_cast<int>(world.blocksPerAxis());
        std::uint8_t faces = 0;
        for (const auto& side : sides)
        {
            const int nx = x + side.dx, ny = y + side.dy, nz = z + side.dz;
            const bool water = nx >= 0 && ny >= 0 && nz >= 0 && nx < size && ny < size && nz < size &&
                               GetBlockDefinition(world.getBlock(nx, ny, nz).type()).water;
            if (!water) faces |= std::uint8_t(1u << static_cast<unsigned>(side.face));
        }
        return faces;
    }
}
