#pragma once

#include "DirectionalCubeTiles.hpp"
#include "TileCoverage.hpp"
#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <array>

namespace GalaxyEggbert::Game
{
// Shared by face culling and the model-cell fast path. Pure geometry/coverage logic; no device.
inline bool IsTerrainOccluder(Worlds::Block block, const std::vector<TileCoverage> &coverage)
{
    if (block.isAir() || block.type() == BlockTypes::FlightBarrier)
        return false;
    const auto &d = GetBlockDefinition(block.type());
    const auto &anim = GetBlockDefinition(d.animation.baseIcon);
    if (anim.water || anim.animation.IsAnimated() || d.alphaBlend)
        return false;
    if (d.renderMode != BlockRenderMode::Air && d.renderMode != BlockRenderMode::UniformCube)
    {
        if (d.renderMode != BlockRenderMode::DirectionalCube)
            return false;
        static std::array<std::int8_t, BlockDefinitionRegistry::DefinitionCount> closed{};
        auto &known = closed[d.type];
        if (known == 0)
            known = IsClosedDirectionalCube(d.type) ? 1 : -1;
        if (known < 0)
            return false;
    }
    return CoverageOf(coverage, block.type()) == TileCoverage::Opaque;
}
} // namespace GalaxyEggbert::Game
