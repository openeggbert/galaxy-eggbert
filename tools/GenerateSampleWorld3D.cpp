#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/MoveObjectRecord.hpp>
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

    // Real background image demo (NEXT.md §3, 2026-07-09) -- region 3 is a
    // real mobile-eggbert region (Content/backgrounds/decor003.png), not
    // the region=0 default, so this world visibly exercises the new
    // skyRegion header field/GalaxyEggbertCnaGame background-loading path
    // instead of silently matching the fallback.
    world.setSkyRegion(3);

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

    // Icons 15/16/17/18 demo (2026-07-09, §8 task 1 complete -- last 6
    // DirectionalCube icons backfilled): axis + open/color side pair,
    // defaulted per the user's explicit go-ahead (crops didn't give a
    // confident read even after direct review). Same off-path row (y=3,
    // z=42) as the other DirectionalCube demos above.
    fill(92, 92, 3, 3, 42, 42, static_cast<std::uint16_t>(15));
    fill(94, 94, 3, 3, 42, 42, static_cast<std::uint16_t>(16));
    fill(96, 96, 3, 3, 42, 42, static_cast<std::uint16_t>(17));
    fill(98, 98, 3, 3, 42, 42, static_cast<std::uint16_t>(18));

    // Icons 108/109 demo (2026-07-09, §8 task 1 complete): own texture + 2
    // sides, icon 107's texture on 1 side, 1 side open, grass top overlay
    // (same as icon 107) -- placed on the ground floor next to the icon-107
    // patch so the real grass-top surface is naturally visible from a
    // normal standing view, same reasoning as icon 107's own placement.
    fill(38, 38, 0, 0, 35, 35, static_cast<std::uint16_t>(108));
    fill(39, 39, 0, 0, 35, 35, static_cast<std::uint16_t>(109));

    // MoveObject demo (2026-07-09): the first 2 objects embedded directly in
    // the 3D .vwr format itself, via Worlds::World's block-extra-metadata
    // mechanism (GalaxyEggbert::MoveObjectRecord/PlaceMoveObject) rather than
    // a mobile-eggbert .txt file -- proves GEWorldRuntime::LoadFromVwrFile()
    // now populates GetMobileObjects() too, not just GetWorld(). One static
    // pickup (egg, no path) and one moving object (platform lift, real
    // posStart != posEnd) to exercise both cases. Same off-path row (y=3,
    // z=42) as the render-mode demo blocks above.
    {
        // Positions are raw grid coordinates (same space as fill()'s x/y/z
        // above), NOT the CNA-side "-kWorldCenterX/Z" render/camera space --
        // see MoveObjectRecord.hpp's doc comment. Grid x=84/86 sits right
        // next to the icon-368 demo block (grid x=82) on the same z=42 row.
        MoveObjectRecord egg;
        egg.type = ObjectType::ObjectType6; // extra-life egg
        egg.posStartX = 84.0f; egg.posStartY = 3.0f; egg.posStartZ = 42.0f;
        egg.posEndX = 84.0f; egg.posEndY = 3.0f; egg.posEndZ = 42.0f;
        PlaceMoveObject(world, egg);

        MoveObjectRecord lift;
        lift.type = ObjectType::ObjectType1; // standard platform lift
        lift.posStartX = 86.0f; lift.posStartY = 3.0f; lift.posStartZ = 42.0f;
        lift.posEndX = 86.0f; lift.posEndY = 6.0f; lift.posEndZ = 42.0f;
        PlaceMoveObject(world, lift);
    }

    // Full ObjectType catalog demo (2026-07-09, user request: "all confirmed
    // MoveObject types"): one static placement each for every remaining
    // named/confirmed-real-behavior ObjectType in include/GalaxyEggbert/def/
    // ObjectType.hpp (67 of the ~69 total named entries -- ObjectType1/6
    // already placed above). Excludes ObjectType0 (null/inactive slot) and
    // the "Unidentified/reserved" block (kept contiguous for level-file
    // round-trips only, no confirmed real behavior -- would just be a guess
    // to place them). A fresh, dedicated grid (z=75-96, well past every
    // other demo/terrain feature and Blupi's tested spawn/staircase/wall-
    // collision path) laid out 9 columns x 8 rows, 3 grid units apart, so
    // each renders without overlapping its neighbors in a survey screenshot.
    // Every entry uses the same simplification as the egg above (posEnd ==
    // posStart, no path) -- rendering doesn't consume posEnd yet regardless
    // (see GalaxyEggbertCnaGame.cpp's billboard/UniformCube draw code), and
    // most of these types have no confirmed real path shape to encode
    // faithfully anyway. Types 12/47/48 (crate, platform-lift variants)
    // render as UniformCube like the type-1 lift above (GEObjectIcons::
    // IsUniformCubeObject); everything else renders as a billboard.
    {
        struct CatalogEntry { ObjectType type; const char* name; };
        static const CatalogEntry kCatalog[] = {
            {ObjectType::ObjectType47, "platform lift, rightward carry bonus"},
            {ObjectType::ObjectType48, "platform lift, leftward carry bonus"},
            {ObjectType::ObjectType2,  "standard patrol enemy"},
            {ObjectType::ObjectType3,  "patrol enemy variant"},
            {ObjectType::ObjectType96, "follow enemy variant 1"},
            {ObjectType::ObjectType97, "follow enemy variant 2"},
            {ObjectType::ObjectType4,  "bulldozer"},
            {ObjectType::ObjectType5,  "treasure"},
            {ObjectType::ObjectType7,  "level-exit goal marker"},
            {ObjectType::ObjectType21, "secret-level exit marker"},
            {ObjectType::ObjectType39, "sparkle trail"},
            {ObjectType::ObjectType49, "key 1"},
            {ObjectType::ObjectType50, "key 2"},
            {ObjectType::ObjectType51, "key 3"},
            {ObjectType::ObjectType13, "helicopter"},
            {ObjectType::ObjectType19, "jeep"},
            {ObjectType::ObjectType24, "skateboard"},
            {ObjectType::ObjectType25, "shield"},
            {ObjectType::ObjectType26, "suction-cup power-up"},
            {ObjectType::ObjectType28, "tank"},
            {ObjectType::ObjectType29, "bullet ammo pack"},
            {ObjectType::ObjectType30, "drink power-up"},
            {ObjectType::ObjectType31, "charge/cloud power-up"},
            {ObjectType::ObjectType40, "mirror/invert power-up"},
            {ObjectType::ObjectType46, "balloon"},
            {ObjectType::ObjectType55, "dynamite stick"},
            {ObjectType::ObjectType8,  "primary explosion"},
            {ObjectType::ObjectType9,  "secondary small explosion"},
            {ObjectType::ObjectType10, "tertiary explosion"},
            {ObjectType::ObjectType11, "fan-hit shockwave"},
            {ObjectType::ObjectType12, "crate"},
            {ObjectType::ObjectType36, "pollution/cloud puff"},
            {ObjectType::ObjectType37, "clear/dissipate effect"},
            {ObjectType::ObjectType38, "electric arc"},
            {ObjectType::ObjectType41, "invert-start particle burst"},
            {ObjectType::ObjectType42, "invert-stop particle burst"},
            {ObjectType::ObjectType53, "tentacle hazard"},
            {ObjectType::ObjectType90, "electric spark"},
            {ObjectType::ObjectType91, "small flash"},
            {ObjectType::ObjectType92, "long energy arc"},
            {ObjectType::ObjectType93, "tiny flash"},
            {ObjectType::ObjectType98, "water splash variant 1"},
            {ObjectType::ObjectType99, "water splash variant 2"},
            {ObjectType::ObjectType100,"water splash variant 3"},
            {ObjectType::ObjectType14, "water plouf splash"},
            {ObjectType::ObjectType15, "water bubble rising"},
            {ObjectType::ObjectType34, "goo/glue particle"},
            {ObjectType::ObjectType35, "small plouf splash"},
            {ObjectType::ObjectType23, "fired projectile"},
            {ObjectType::ObjectType16, "spider/arthropod"},
            {ObjectType::ObjectType17, "fish"},
            {ObjectType::ObjectType18, "patrol variant"},
            {ObjectType::ObjectType20, "bird"},
            {ObjectType::ObjectType32, "blupih"},
            {ObjectType::ObjectType33, "blupit"},
            {ObjectType::ObjectType44, "wasp/bee"},
            {ObjectType::ObjectType54, "large creature"},
            {ObjectType::ObjectType22, "door opening animation"},
            {ObjectType::ObjectType27, "magic track sparkle"},
            {ObjectType::ObjectType52, "bridge construction"},
            {ObjectType::ObjectType56, "dynamite fuse"},
            {ObjectType::ObjectType57, "shield trail sparkle"},
            {ObjectType::ObjectType58, "shield disappear effect"},
            {ObjectType::ObjectType200,"Blupi skin: default"},
            {ObjectType::ObjectType201,"Blupi skin: variant 1"},
            {ObjectType::ObjectType202,"Blupi skin: variant 2"},
            {ObjectType::ObjectType203,"Blupi skin: variant 3"},
        };

        constexpr int kCatalogCols = 9;
        constexpr int kCatalogBaseX = 10;
        constexpr int kCatalogBaseZ = 75;
        constexpr int kCatalogSpacing = 3;
        constexpr float kCatalogY = 1.0f;

        int index = 0;
        for (const CatalogEntry& entry : kCatalog)
        {
            const int col = index % kCatalogCols;
            const int row = index / kCatalogCols;
            const float x = static_cast<float>(kCatalogBaseX + col * kCatalogSpacing);
            const float z = static_cast<float>(kCatalogBaseZ + row * kCatalogSpacing);

            MoveObjectRecord record;
            record.type = entry.type;
            record.posStartX = x; record.posStartY = kCatalogY; record.posStartZ = z;
            record.posEndX = x; record.posEndY = kCatalogY; record.posEndZ = z;
            PlaceMoveObject(world, record);
            ++index;
        }

        std::cout << "GenerateSampleWorld3D: placed " << index
                  << " catalog MoveObject(s) (grid x=" << kCatalogBaseX << ".., z=" << kCatalogBaseZ
                  << "..)." << std::endl;
    }

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
              << " non-air blocks, Y range [" << minY << ", " << maxY << "], skyRegion="
              << reloaded.skyRegion()
              << " (round-trip verified via World::loadFromFile)." << std::endl;

    return 0;
}
