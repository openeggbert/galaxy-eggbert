#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>

// Generates the first genuinely 3D, hand-authored .vwr world: a ground
// floor, an ascending solid staircase, and a walled room on a raised
// platform — real Y variation (Y 0-13), unlike the flat (Y=0 only)
// mobile-eggbert-derived worlds GalaxyEggbertCNA currently loads. Run once
// to (re)produce the output file; re-run any time to regenerate it.
int main(int argc, char** argv)
{
    using namespace GalaxyEggbert;
    using namespace GalaxyEggbert::Worlds;

    const std::filesystem::path outPath = (argc > 1) ? argv[1] : "worlds3d/world001.vwr";

    World world;

    const auto fill = [&world](int x0, int x1, int y0, int y1, int z0, int z1, std::uint16_t type)
    {
        for (int x = x0; x <= x1; ++x)
            for (int y = y0; y <= y1; ++y)
                for (int z = z0; z <= z1; ++z)
                    world.setBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                   static_cast<std::uint16_t>(z), Block::make(type));
    };

    // Ground floor, y=0.
    // Uses RockPile (icon 35, confirmed genuine bulk stone-like material) --
    // was BlockTypes::Ground until 2026-07-08, when direct user identification
    // found Ground to actually be a machine-piece graphic too, not ground/grass
    // texture (see BlockTypes.hpp's note on Ground/StoneA/StoneB and
    // mobile-eggbert-reference/questionnaire-all-remaining-tiles.md). This
    // usage was missed in the earlier StoneA/StoneB fix (59c2e61) even though
    // it's the same file and the same invariant violation.
    fill(30, 70, 0, 0, 30, 70, BlockTypes::RockPile);

    // Ascending solid staircase: 10 steps, x=29 down to x=20, z=45..54,
    // each column solid from y=0 up to its own step height.
    // Uses RockPile (icon 35, confirmed genuine bulk stone-like material) --
    // was BlockTypes::StoneA until 2026-07-08, when direct user identification
    // found StoneA to actually be a machine-piece graphic, not stone texture
    // (see BlockTypes.hpp's note on Ground/StoneA/StoneB and
    // mobile-eggbert-reference/questionnaire-all-remaining-tiles.md).
    for (int step = 0; step < 10; ++step)
    {
        const int x = 29 - step;
        fill(x, x, 0, step, 45, 54, BlockTypes::RockPile);
    }

    // Raised platform floor at y=10, sitting flush with the tallest step's
    // own surface (x=20's staircase fill already reaches y=9, surface
    // y=10) -- starts at x=19, NOT x=20, so it doesn't stack a second
    // block on top of the staircase's own last step (that produced an
    // unclimbable 2-block cliff at the x=21->20 transition; caught by
    // tools/VerifyBlupiMovement.cpp).
    // Uses RockPile (icon 35), same material as the staircase it sits flush
    // with -- was BlockTypes::Platform (icon 200) until 2026-07-08, when
    // round-1 Q&A found icon 200 to actually be a passable grate/grid graphic
    // (DirectionalCube: 4 side faces textured, top/bottom genuinely open, not
    // just a fallback color), not a real solid floor surface -- same class of
    // bug as Ground/StoneA/StoneB (see BlockTypes.hpp and 02-tiles.md's icon
    // 200 entry). BlockTypes::Platform is kept defined (not renamed/removed)
    // for the same reason as Ground/StoneA/StoneB -- do not use it for new
    // solid-floor fills; wait for the DirectionalCube render mode (§8 task 2)
    // to render it as the passable grate it actually is.
    fill(5, 19, 10, 10, 40, 59, BlockTypes::RockPile);

    // Room walls, 3 blocks tall (y=11..13), around the platform perimeter,
    // with a 3-wide doorway on the staircase-facing edge (x=20, z=48..51).
    // Uses BrickWall (icon 261, confirmed genuine "zeď z cihel"/brick-wall
    // material) -- was BlockTypes::StoneB until 2026-07-08, when direct user
    // identification found StoneB to actually be a machine-piece graphic too
    // (StoneB itself replaced BlockTypes::Wall/GoldPillar here on 2026-07-06
    // for the same reason -- see BlockTypes.hpp's note on Ground/StoneA/
    // StoneB and mobile-eggbert-reference/questionnaire-all-remaining-tiles.md).
    fill(5, 5,   11, 13, 40, 59, BlockTypes::BrickWall); // west wall
    fill(6, 19,  11, 13, 40, 40, BlockTypes::BrickWall); // north wall
    fill(6, 19,  11, 13, 59, 59, BlockTypes::BrickWall); // south wall
    fill(20, 20, 11, 13, 40, 47, BlockTypes::BrickWall); // east wall, north of doorway
    fill(20, 20, 11, 13, 52, 59, BlockTypes::BrickWall); // east wall, south of doorway

    // Two decorative pillars inside the room.
    fill(10, 10, 11, 13, 45, 45, BlockTypes::BrickWall);
    fill(10, 10, 11, 13, 54, 54, BlockTypes::BrickWall);

    // Floating demo grate, y=3, directly ahead of Blupi's spawn (grid
    // (50,*,50) == world origin, default yaw faces -Z -- see
    // GEWorldRuntime::kWorldCenterX/Z and GalaxyEggbertCnaGame::Update's
    // camera setup) -- first real placement of icon 200 (Platform) using the
    // new DirectionalCube render mode (NEXT.md §8 task 1, 2026-07-08):
    // textured on its 4 side faces, top/bottom genuinely open. Deliberately
    // floating well above Blupi's collision height (y=3, spawn/staircase-
    // walk tests only ever occupy y<=2) and off to the side of the westward
    // staircase-walk/wall-collision test path (which stays at z=50 the whole
    // time; this sits at z=40..44) so it cannot affect
    // tools/VerifyBlupiMovement.cpp's existing checks -- purely decorative,
    // proving the render mode renders correctly, not a floor Blupi walks on.
    fill(48, 52, 3, 3, 40, 44, BlockTypes::Platform);

    // Two more floating DirectionalCube demo blocks, right next to the icon
    // 200 grate, showing the two other confirmed face patterns from the
    // 2026-07-08 GEDirectionalCubeTiles.cpp backfill (NEXT.md §8 task 1):
    // icon 2 (top+bottom flat fallback color, no open faces) and icon 25
    // (top flat fallback color, bottom open) -- both "unnamed variant" icons
    // per 02-tiles.md, no BlockTypes constant needed for a demo placement.
    // Same off-path reasoning as the grate above (y=3, z=40..44).
    fill(54, 56, 3, 3, 40, 44, static_cast<std::uint16_t>(2));
    fill(58, 60, 3, 3, 40, 44, static_cast<std::uint16_t>(25));

    // Three more demo blocks showing the newer single-face/axis/fan
    // DirectionalCube patterns added 2026-07-08: icon 126 (FanLeft -- 4
    // sides tex, base flat-color at bottom, open top), icon 392 (single
    // face tex on -X, 5 other faces flat color), icon 49 (2 opposite sides
    // + top + bottom tex on the X axis, other 2 sides flat color). Same
    // off-path reasoning as the rest of this row.
    fill(62, 64, 3, 3, 40, 44, static_cast<std::uint16_t>(126));
    fill(66, 68, 3, 3, 40, 44, static_cast<std::uint16_t>(392));
    fill(70, 72, 3, 3, 40, 44, static_cast<std::uint16_t>(49));

    // One demo block each for the 3 new-geometry render modes added
    // 2026-07-08 (NEXT.md §8 task 2): icon 76 (InnerPillarBox, 4 sides of a
    // smaller inner box textured, top/bottom open), icon 384 (InnerPillarBox
    // variant, BlockTypes::Switch -- 1 side of the inner box textured, other
    // 5 flat color), icon 77 (InnerFlatPlate -- single double-sided plate,
    // outer cube fully transparent), icon 53 (TripleCrossBillboard -- 3
    // planes at 60° through the block's center). Single blocks, not walls,
    // since these are individual objects, not bulk terrain. Same off-path
    // reasoning as the rest of this row (y=3, away from Blupi's tested
    // spawn/staircase/wall-collision path).
    fill(74, 74, 3, 3, 42, 42, static_cast<std::uint16_t>(76));
    fill(76, 76, 3, 3, 42, 42, static_cast<std::uint16_t>(384));
    fill(78, 78, 3, 3, 42, 42, static_cast<std::uint16_t>(77));
    fill(80, 80, 3, 3, 42, 42, static_cast<std::uint16_t>(53));

    // Water render mode demo (2026-07-08 design decision, §8 task 2):
    // BlockTypes::Water1 is a semi-transparent alpha-blended cube now
    // (GETerrainRenderer's dedicated water pass), not a solid animated
    // UniformCube like every other terrain tile. Places a BrickWall block
    // 2 cells behind (further -Z from spawn) a Water1 block so both are
    // visible from spawn's default camera -- the wall should show through
    // the water block if the semi-transparent pass actually works.
    fill(85, 85, 3, 3, 40, 40, BlockTypes::BrickWall);
    fill(85, 85, 3, 3, 42, 42, BlockTypes::Water1);

    // Icon 30 demo (2026-07-08, §8 task 1): a DirectionalCube whose own side
    // texture has real per-pixel alpha, using the same static-but-
    // transparent render pass as water (see GETerrainRenderer.cpp's
    // NeedsAlphaBlend()). Same BrickWall-behind-it setup as the water demo,
    // to visually confirm this icon is genuinely alpha-blended too.
    fill(90, 90, 3, 3, 40, 40, BlockTypes::BrickWall);
    fill(90, 90, 3, 3, 42, 42, static_cast<std::uint16_t>(30));

    // Icon 107 demo (2026-07-08, §8 task 3): a DirectionalCube whose top
    // face is deliberately left open in GEDirectionalCubeTiles.cpp, covered
    // instead by GETerrainRenderer's separate grass_top.png overlay plate.
    // Placed as a small patch directly on the ground floor (y=0, not the
    // elevated y=3 demo row) so its real top surface is naturally visible
    // from a normal standing view, the way the ground floor itself already
    // is -- away from the spawn/staircase/wall-collision test coordinates
    // (grid x=35-37, z=35-37; ground floor spans x/z 30-70, tests use
    // z=50 and x<=29).
    fill(35, 37, 0, 0, 35, 37, static_cast<std::uint16_t>(107));

    // Icon 368 demo (2026-07-09, §8 task 4 -- InnerFlatPlate axis spot-check):
    // GEInnerFlatPlateTiles's confirmed InnerFlatPlate icons all defaulted to
    // a vertical (Z-axis) plate, but icons 368-372's crops show a clearly
    // horizontal, ground-lying shape unlike the other 58 icons' vertical
    // frame/bracket look -- GetInnerFlatPlateAxis() now returns PlateAxis::Y
    // for them. Same off-path reasoning as the rest of this row (y=3, away
    // from Blupi's tested spawn/staircase/wall-collision path).
    fill(82, 82, 3, 3, 42, 42, static_cast<std::uint16_t>(368));

    world.saveToFile(outPath);

    // Round-trip verification: reload and report real stats, proving this is
    // a genuinely 3D structure (not another flat Y=0 layout).
    const World reloaded = World::loadFromFile(outPath);
    int nonAir = 0;
    int minY = 999;
    int maxY = -1;
    const int axis = static_cast<int>(reloaded.blocksPerAxis());
    for (int x = 0; x < axis; ++x)
    {
        for (int y = 0; y < axis; ++y)
        {
            for (int z = 0; z < axis; ++z)
            {
                if (!reloaded.getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                       static_cast<std::uint16_t>(z)).isAir())
                {
                    ++nonAir;
                    minY = std::min(minY, y);
                    maxY = std::max(maxY, y);
                }
            }
        }
    }

    std::cout << "GenerateSampleWorld3D: wrote " << outPath << " -- " << nonAir
              << " non-air blocks, Y range [" << minY << ", " << maxY << "]"
              << " (round-trip verified via World::loadFromFile)." << std::endl;

    return 0;
}
