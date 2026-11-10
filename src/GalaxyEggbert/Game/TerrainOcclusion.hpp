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

// Whether @p block hides a neighbour's face lying against its own face @p face (a
// CnaEngine::Graphics::BoxFace): an occluder hides all of them; a DirectionalCube with a face open
// (the grass 107's top, under its grass plate) the ones against its closed faces. Without it the
// faces between a grass cube and its neighbours were drawn on the cells' boundary, under the grass
// plates' edges, and showed through as thin lines with the 3D effects (the owner, 2026-11-10).
inline bool IsTerrainOccluderFace(Worlds::Block block, const std::vector<TileCoverage> &coverage, int face)
{
    if (IsTerrainOccluder(block, coverage))
        return true;
    if (block.isAir() || block.type() == BlockTypes::FlightBarrier || face < 0 || face >= 6)
        return false;
    const auto &d = GetBlockDefinition(block.type());
    const auto &anim = GetBlockDefinition(d.animation.baseIcon);
    if (anim.water || anim.animation.IsAnimated() || d.alphaBlend || d.renderMode != BlockRenderMode::DirectionalCube ||
        CoverageOf(coverage, block.type()) != TileCoverage::Opaque)
        return false;
    static std::array<std::array<std::int8_t, 6>, BlockDefinitionRegistry::DefinitionCount> closed{};
    auto &known = closed[d.type][static_cast<std::size_t>(face)];
    if (known == 0)
        known = DirectionalCubeFaceClosed(d.type, face) ? 1 : -1;
    return known > 0;
}
} // namespace GalaxyEggbert::Game
