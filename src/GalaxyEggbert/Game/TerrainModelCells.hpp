#pragma once

#include "BlockModels.hpp"
#include "TerrainOcclusion.hpp"
#include "WorldRuntime.hpp"
#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <algorithm>
#include <cmath>
#include <optional>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

namespace GalaxyEggbert::Game
{
struct TerrainModelCell
{
    float x, y, z; // cell centre in render space
    std::uint16_t base;
    std::uint8_t sameSides = 0; // 1 +X, 2 -X, 4 +Z, 8 -Z
    std::uint8_t openSide = 2;  // 0 +X, 1 -X, 2 +Z, 3 -Z
};

inline bool HasStillModel(const std::vector<std::uint16_t> &icons, std::uint16_t icon)
{
    return std::find(icons.begin(), icons.end(), icon) != icons.end();
}

// The one per-cell rule, used by both initial terrain construction and incremental updates.
inline std::optional<TerrainModelCell> ComputeTerrainModelCell(const Worlds::World &world, int x, int y, int z,
                                                               const std::vector<std::uint16_t> &modelDrawnIcons,
                                                               bool drawSecret)
{
    const int blocksPerAxis = world.blocksPerAxis();
    if (x < 0 || y < 0 || z < 0 || x >= blocksPerAxis || y >= blocksPerAxis || z >= blocksPerAxis)
        return std::nullopt;
    const auto block = world.getBlock(x, y, z);
    if (block.isAir() || (!drawSecret && block.type() == BlockTypes::FlightBarrier) ||
        !HasStillModel(modelDrawnIcons, block.type()))
        return std::nullopt;
    const float worldX = static_cast<float>(x - WorldRuntime::kWorldCenterX);
    const float worldY = static_cast<float>(y);
    const float worldZ = static_cast<float>(z - WorldRuntime::kWorldCenterZ);
    const auto same = [&](int dx, int dz) {
        const int nx = x + dx, nz = z + dz;
        return nx >= 0 && nz >= 0 && nx < blocksPerAxis && nz < blocksPerAxis &&
               ModelRowFamily(world
                                  .getBlock(static_cast<std::uint16_t>(nx), static_cast<std::uint16_t>(y),
                                            static_cast<std::uint16_t>(nz))
                                  .type()) == ModelRowFamily(block.type());
    };
    const auto sides = static_cast<std::uint8_t>((same(1, 0) ? 1 : 0) | (same(-1, 0) ? 2 : 0) | (same(0, 1) ? 4 : 0) |
                                                 (same(0, -1) ? 8 : 0));
    // A bar shows its ceiling mounts only under a tile they hold to (ShownBarIcon),
    // when that icon's model is loaded too.
    std::uint16_t shown = block.type();
    if (IsBarIcon(shown))
    {
        const std::uint16_t above =
            y + 1 < blocksPerAxis ? world
                                        .getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y + 1),
                                                  static_cast<std::uint16_t>(z))
                                        .type()
                                  : std::uint16_t{0};
        const std::uint16_t bar = ShownBarIcon(above);
        if (std::find(modelDrawnIcons.begin(), modelDrawnIcons.end(), bar) != modelDrawnIcons.end())
        {
            shown = bar;
        }
    }
    // The side facing the most room to stand in (BlockModels.hpp
    // FrontModelOpenSide). A hub door's bar counts as open: the hub lifts it as
    // the missions are won, and the sign in its niche faces the doorway.
    const auto open = [&](int cx, int cy, int cz) {
        if (cx < 0 || cy < 0 || cz < 0 || cx >= blocksPerAxis || cy >= blocksPerAxis || cz >= blocksPerAxis)
        {
            return false;
        }
        const auto type = world
                              .getBlock(static_cast<std::uint16_t>(cx), static_cast<std::uint16_t>(cy),
                                        static_cast<std::uint16_t>(cz))
                              .type();
        return type == 0 || type == GalaxyEggbert::BlockTypes::HubDoor ||
               GalaxyEggbert::GetBlockDefinition(type).collisionMask == 0;
    };
    const std::uint8_t openSide = FrontModelOpenSide(x, y, z, sides, open);
    return TerrainModelCell{worldX, worldY, worldZ, shown, sides, openSide};
}

// A switch turning a saw on or off (378 <-> 379, WorldRuntime::TryActivateSwitch). Both are animated
// cells (TerrainRenderer's anim blocks): never occluders, so a swap changes only the anim-block base
// (OR-0074's patch, extended to the saws 2026-11-07: a toggle rebuilt the whole terrain).
inline bool IsSawIcon(Worlds::Block block)
{
    return !block.isAir() && (block.type() == BlockTypes::Saw || block.type() == BlockTypes::SawStopped);
}

inline bool IsSawSwap(const Worlds::World::BlockChange &c)
{
    return IsSawIcon(Worlds::Block(c.oldRaw)) && IsSawIcon(Worlds::Block(c.newRaw));
}

// The anim blocks (any struct with x, y, z in render space and base) the batch's saw swaps change, in
// the journal's order, as (index, new base). Nothing is changed; false when a swapped saw has no block.
template <typename AnimBlocks>
bool FindSawAnimSwaps(const Worlds::World::BlockChanges &changes, const AnimBlocks &blocks,
                      std::vector<std::pair<std::size_t, std::uint16_t>> &swaps)
{
    swaps.clear();
    for (const auto &c : changes.cells)
    {
        if (!IsSawSwap(c))
            continue;
        const float x = static_cast<float>(c.x - WorldRuntime::kWorldCenterX);
        const float y = static_cast<float>(c.y);
        const float z = static_cast<float>(c.z - WorldRuntime::kWorldCenterZ);
        const auto found = std::find_if(blocks.begin(), blocks.end(),
                                        [&](const auto &b) { return b.x == x && b.y == y && b.z == z; });
        if (found == blocks.end())
            return false;
        swaps.emplace_back(static_cast<std::size_t>(found - blocks.begin()),
                           GetBlockDefinition(Worlds::Block(c.newRaw).type()).animation.baseIcon);
    }
    return true;
}

// Only model/air transitions which leave all baked terrain data unchanged are eligible.
// Non-occlusion preserves cube/grass faces and directional room runs. No relief at either
// endpoint preserves isReliefCell, and model-drawn cells have reliefSolid=false, just like air.
template <typename HasRelief>
bool CanPatchTerrainModels(const Worlds::World &world, const Worlds::World::BlockChanges &changes,
                           const std::vector<std::uint16_t> &icons, const std::vector<TileCoverage> &coverage,
                           HasRelief &&hasRelief)
{
    if (!changes.known)
        return false;
    const auto safe = [&](Worlds::Block block) {
        if (block.isAir())
            return true;
        const auto &d = GetBlockDefinition(block.type());
        return HasStillModel(icons, block.type()) && block.type() != BlockTypes::FlightBarrier &&
               !IsTerrainOccluder(block, coverage) && !d.water && !d.animation.IsAnimated() && !d.grassTop &&
               !hasRelief(block.type());
    };
    const int axis = world.blocksPerAxis();
    for (const auto &c : changes.cells)
    {
        const Worlds::Block old(c.oldRaw), next(c.newRaw);
        if (IsSawSwap(c))
        {
            if (hasRelief(old.type()) || hasRelief(next.type()))
                return false;
            continue; // only its anim block's base changes (FindSawAnimSwaps)
        }
        if (!safe(old) || !safe(next))
            return false;
        // Water has no baked neighbour data. Lava records whether its bottom touches ANY non-air.
        if (old.isAir() != next.isAir() && c.y + 1 < axis)
        {
            const auto above = world.getBlock(c.x, c.y + 1, c.z);
            if (!above.isAir() && GetBlockDefinition(above.type()).animation.baseIcon == BlockTypes::Lava)
                return false;
        }

    }
    return true;
}

// FrontModelOpenSide reads a cross of length 8, at the model's height and four cells down.
// Row sides read immediate horizontal neighbours; a bar also reads the cell directly above.
inline bool TerrainModelDependsOn(int x, int y, int z, const Worlds::World::BlockChange &c)
{
    const int dx = std::abs(x - c.x), dz = std::abs(z - c.z);
    if (dx == 0 && dz == 0 && (y == c.y || y + 1 == c.y))
        return true;
    return y >= c.y && y <= c.y + kFrontModelFloorDepth &&
           ((dx == 0 && dz <= kFrontModelOpenRun) || (dz == 0 && dx <= kFrontModelOpenRun));
}

template <typename HasRelief>
bool PatchTerrainModelCells(const Worlds::World &world, const Worlds::World::BlockChanges &changes,
                            const std::vector<std::uint16_t> &icons, bool drawSecret,
                            const std::vector<TileCoverage> &coverage, HasRelief &&hasRelief,
                            std::vector<TerrainModelCell> &cells)
{
    // Validate the WHOLE batch before mutating the list.
    if (!CanPatchTerrainModels(world, changes, icons, coverage, hasRelief))
        return false;
    std::set<std::tuple<int, int, int>> affected; // y,z,x: initial constructor order
    for (const auto &c : changes.cells)
        affected.emplace(c.y, c.z, c.x);
    cells.erase(std::remove_if(cells.begin(), cells.end(),
                               [&](const TerrainModelCell &cell) {
                                   const int x = static_cast<int>(cell.x) + WorldRuntime::kWorldCenterX;
                                   const int y = static_cast<int>(cell.y);
                                   const int z = static_cast<int>(cell.z) + WorldRuntime::kWorldCenterZ;
                                   for (const auto &c : changes.cells)
                                       if (TerrainModelDependsOn(x, y, z, c))
                                       {
                                           affected.emplace(y, z, x);
                                           return true;
                                       }
                                   return false;
                               }),
                cells.end());
    const auto less = [](const TerrainModelCell &a, const TerrainModelCell &b) {
        return std::tie(a.y, a.z, a.x) < std::tie(b.y, b.z, b.x);
    };
    for (const auto &[y, z, x] : affected)
        if (auto cell = ComputeTerrainModelCell(world, x, y, z, icons, drawSecret))
            cells.insert(std::lower_bound(cells.begin(), cells.end(), *cell, less), *cell);
    return true;
}
} // namespace GalaxyEggbert::Game
