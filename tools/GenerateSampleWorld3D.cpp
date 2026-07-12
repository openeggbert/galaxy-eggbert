#include "Game/GEObjectIcons.hpp"
#include "Game/GEPlateRotationMetadata.hpp"

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
//
// Redesigned 2026-07-10 (user request): this used to be a flat 41x41
// showroom floor plus an isolated row of one-of-each render-mode specimen
// blocks and a mega-grid of all 67 confirmed ObjectTypes crammed together —
// useful for proving each render mode/object works, but nothing like a real
// Galaxy Eggbert level. Replaced with an actual small playable layout:
// a narrow walkable path (not a big flat plain), a real underground tunnel
// (enclosed by walls + a ceiling, not just a lower Y), a terraced hill with
// its own platform lift, and the walled room from before -- populated with
// real ObjectTypes at roughly the density/mix real mobile-eggbert levels
// use (checked directly against ../mobile-eggbert/worlds/world011/013/014/
// 021/022/023.txt's MoveObject: type= tallies: crates (12) are the single
// most common type, then standard patrol enemies (2), eggs (6), chests (5),
// platform lifts (1), then dynamite/skateboard/keys/exit in smaller
// numbers) -- not an invented distribution.
//
// The spawn point, staircase, wall-collision corridor, and walled room are
// UNCHANGED from before -- tools/VerifyBlupiMovement.cpp hardcodes exact
// grid coordinates against this exact path (spawn (0,1,0), walk west along
// grid z=50 to the staircase at grid x=29..20, continue into the west wall
// at grid x=5), so that one line/corridor stays geometrically identical.
// Everything off that single tested line was free to redesign.
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

    // ------------------------------------------------------------------
    // Tested corridor (UNCHANGED geometry -- see VerifyBlupiMovement.cpp):
    // a narrow walkable path along grid z=48..52 from spawn (grid x=50) to
    // the staircase (grid x=29..20), instead of the old 41x41 flat floor.
    // Uses RockPile (icon 35, confirmed genuine bulk stone-like material) --
    // was BlockTypes::Ground until 2026-07-08, when direct user
    // identification found Ground to actually be a machine-piece graphic
    // too (see BlockTypes.hpp's note on Ground/StoneA/StoneB and
    // mobile-eggbert-reference/questionnaire-all-remaining-tiles.md).
    // ------------------------------------------------------------------
    fill(20, 75, 0, 0, 48, 52, BlockTypes::RockPile);

    // Ascending solid staircase: 10 steps, x=29 down to x=20, z=45..54,
    // each column solid from y=0 up to its own step height. UNCHANGED --
    // see VerifyBlupiMovement.cpp's "climbed the staircase" check.
    // Uses RockPile (icon 35) -- was BlockTypes::StoneA until 2026-07-08,
    // same real-identification fix as above.
    for (int step = 0; step < 10; ++step)
    {
        const int x = 29 - step;
        fill(x, x, 0, step, 45, 54, BlockTypes::RockPile);
    }

    // Raised platform floor at y=10, sitting flush with the tallest step's
    // own surface. UNCHANGED. Uses RockPile (icon 35) -- was
    // BlockTypes::Platform (icon 200) until 2026-07-08, when round-1 Q&A
    // found icon 200 to actually be a passable grate/grid graphic, not a
    // real solid floor surface (see BlockTypes.hpp and 02-tiles.md's icon
    // 200 entry).
    fill(5, 19, 10, 10, 40, 59, BlockTypes::RockPile);

    // Room walls, 3 blocks tall (y=11..13), around the platform perimeter,
    // with a 3-wide doorway on the staircase-facing edge (x=20, z=48..51).
    // UNCHANGED. Uses BrickWall (icon 261, confirmed genuine "zeď z
    // cihel"/brick-wall material).
    fill(5, 5,   11, 13, 40, 59, BlockTypes::BrickWall); // west wall
    fill(6, 19,  11, 13, 40, 40, BlockTypes::BrickWall); // north wall
    fill(6, 19,  11, 13, 59, 59, BlockTypes::BrickWall); // south wall
    fill(20, 20, 11, 13, 40, 47, BlockTypes::BrickWall); // east wall, north of doorway
    fill(20, 20, 11, 13, 52, 59, BlockTypes::BrickWall); // east wall, south of doorway

    // Two decorative pillars inside the room. UNCHANGED.
    fill(10, 10, 11, 13, 45, 45, BlockTypes::BrickWall);
    fill(10, 10, 11, 13, 54, 54, BlockTypes::BrickWall);

    // ------------------------------------------------------------------
    // North hill: a short terraced ascent off the tested corridor (grid
    // x=45..55, climbing from z=47 up to z=33), leading to a plateau and a
    // second platform lift up to a small "treasure nest" -- real height
    // variation away from the tested path, on the opposite side from the
    // walled room/staircase so neither can overlap it.
    // ------------------------------------------------------------------
    fill(45, 55, 0, 0, 38, 47, BlockTypes::RockPile); // short flat connector off the corridor
    for (int step = 0; step < 5; ++step)
    {
        const int z = 37 - step;
        fill(45, 55, 0, step, z, z, BlockTypes::RockPile);
    }
    // Plateau at the top of the hill, y=4 -- small (11x8 = 88 cells), not a
    // big flat plain.
    fill(45, 55, 4, 4, 25, 32, BlockTypes::RockPile);
    // Real grass-top demo (icon 107, 2026-07-08 §8 task 3 -- top face
    // deliberately left open, covered by GETerrainRenderer's separate
    // grass_top.png overlay plate) relocated here from the old flat floor
    // patch -- grass makes far more sense on a hilltop than on a showroom
    // floor. Icons 108/109 (own texture + icon 107's texture + open side +
    // grass top, 2026-07-09 §8 task 1) placed right next to it.
    fill(47, 48, 4, 4, 26, 27, static_cast<std::uint16_t>(107));
    fill(49, 49, 4, 4, 26, 26, static_cast<std::uint16_t>(108));
    fill(50, 50, 4, 4, 26, 26, static_cast<std::uint16_t>(109));

    // Second platform lift, from the plateau (y=4) up to a small crow's-nest
    // (y=8) -- a real, gameplay-motivated use of ObjectType1, not a
    // stationary demo placement.
    //
    // Fixed 2026-07-12 (NEXT.md 8 task 3, user-reported): the lift's own
    // shaft column (x=50, z=28) sat directly under this floor's solid
    // footprint with no opening, so the lift could only ever rise to touch
    // the floor's own underside from below -- a fully solid "board" with no
    // passage, exactly the "presouva se skrze desku" (moves through the
    // board) complaint, and useless for ever actually reaching the top even
    // once platform-riding exists (plan.md E3D-MIG-152). Carved a 1-cell
    // shaft opening at the lift's own column so the platform has a real hole
    // to rise through and can park flush in it (see the matching posEndY
    // change below -- the cube's own top face is tuned to land exactly at
    // this floor's top face once the center cell is Air).
    fill(49, 51, 8, 8, 27, 29, BlockTypes::RockPile);
    world.setBlock(50, 8, 28, Block::make(BlockTypes::Air)); // lift shaft opening

    // ------------------------------------------------------------------
    // South tunnel: a real underground/enclosed corridor -- floor, side
    // walls, AND a ceiling (grid y=1..3), not just "lower Y", reached via a
    // short connector off the tested corridor. World Y can't go negative
    // (unsigned grid), so "underground" here means genuinely enclosed
    // (dark, walled, roofed), the same way a real cave reads as underground
    // regardless of its absolute elevation.
    // ------------------------------------------------------------------
    fill(55, 57, 0, 0, 53, 64, BlockTypes::RockPile); // connector off the corridor
    fill(20, 75, 0, 0, 65, 69, BlockTypes::RockPile);  // tunnel floor
    fill(20, 75, 1, 3, 65, 65, BlockTypes::BrickWall);  // tunnel south wall
    fill(20, 75, 1, 3, 69, 69, BlockTypes::BrickWall);  // tunnel north wall
    fill(20, 75, 3, 3, 66, 68, BlockTypes::BrickWall);  // tunnel ceiling (over the open z=66..68 interior)

    // Two fans built into the tunnel's south wall as real ventilation
    // (FanLeft/FanRight, icons 126/129 -- confirmed 4 side faces textured,
    // base flat-color, open face matching the real horizontal blow
    // direction, fixed 2026-07-09/10) -- purely a visual/render placement,
    // NOT the reachable hazard placement (see the two open fan alcoves
    // near the teleporter rooms below, plan.md E3D-MIG-149).
    world.setBlock(30, 2, 65, Block::make(BlockTypes::FanLeft));
    world.setBlock(60, 2, 65, Block::make(BlockTypes::FanRight));

    // Platform grate (icon 200, DirectionalCube -- 4 side faces textured,
    // top/bottom genuinely open, confirmed round-1 Q&A) as a real floor
    // grate over a shallow 1-deep pit near the tunnel's west end, instead of
    // floating in isolation on the old demo row.
    fill(24, 26, 0, 0, 66, 68, static_cast<std::uint16_t>(200));

    // A short, shallow water crossing on the tunnel floor (BlockTypes::Water1
    // -- semi-transparent alpha-blended cube, GETerrainRenderer's dedicated
    // water pass, 2026-07-08 design decision) partway along the tunnel.
    // Fixed 2026-07-12 (plan.md E3D-MIG-148): water is now non-solid for
    // collision (real swimming needs Blupi to sink into/through it, see
    // GEBlupiController::GroundHeightAt()'s own comment) -- placing the
    // water AT the same layer as the surrounding floor (y=0) would have
    // left NOTHING solid beneath it (world Y can't go negative), turning a
    // shallow wade into a bottomless-pit death trap. The real tunnel floor
    // (y=0) stays intact here; the water sits ONE layer above it (y=1),
    // matching a real shallow crossing -- Blupi sinks through it and rests
    // on the real floor at y=1 (Surf: water at his position, dry above).
    fill(45, 46, 1, 1, 66, 68, BlockTypes::Water1);

    // A switch/saw pair (plan.md E3D-MIG-142, 2026-07-11) -- real linking is
    // "same Y and Z, X within +-20" (Decor.cpp:7131-7148), both satisfied
    // here (5 cells apart, same floor row). Switch starts SwitchOff (closed/
    // safe) so the saw starts SawStopped (safe) to match -- pressing Action
    // on the switch is what makes the saw an active hazard, a real
    // interactive element instead of a passive specimen.
    world.setBlock(65, 0, 67, Block::make(BlockTypes::SwitchOff));
    world.setBlock(70, 0, 67, Block::make(BlockTypes::SawStopped));
    // Saw's default InnerFlatPlate axis is Z (same as every other confirmed
    // icon) -- this placement's own corridor runs along X, so it needs the
    // per-instance 90-degree rotation metadata (plan.md E3D-MIG-149,
    // 2026-07-11, GEInnerFlatPlateTiles.hpp's kPlateRotationMetadataType),
    // not a hardcoded per-icon override.
    GalaxyEggbert::CNA::SetPlateRotated(world, 70, 0, 67, true);

    // ------------------------------------------------------------------
    // Real MoveObject population -- ObjectTypes and rough density chosen to
    // match real mobile-eggbert levels, not invented: checked directly
    // against ../mobile-eggbert/worlds/world011/013/014/021/022/023.txt's
    // real MoveObject: type= counts across all 6 files combined (188
    // objects total) -- crates (type 12, 35 occurrences) are the single
    // most common type, then standard patrol enemies (type 2, 31), eggs
    // (type 6, 22), chests (type 5, 21), platform lifts (type 1, 20), then
    // smaller numbers of dynamite/skateboard/keys/exit-marker. Positions
    // are raw grid coordinates (same space as fill()'s x/y/z above), NOT
    // the CNA-side "-kWorldCenterX/Z" render/camera space -- see
    // MoveObjectRecord.hpp's doc comment.
    // ------------------------------------------------------------------
    const auto place = [&world](ObjectType type, float x, float y, float z)
    {
        MoveObjectRecord record;
        record.type = type;
        record.posStartX = x; record.posStartY = y; record.posStartZ = z;
        record.posEndX = x;   record.posEndY = y;   record.posEndZ = z;
        PlaceMoveObject(world, record);
    };

    // The one platform lift in this world (north hill's plateau -> crow's-
    // nest, see the fill() calls above) -- the actual MoveObject record
    // driving it. Uses PlaceMoveObject directly, NOT the place() helper
    // above, since a lift needs a real posStart != posEnd patrol path --
    // place() always sets them equal (fixed 2026-07-10: this lift was
    // originally placed via place(), giving it zero patrol range and
    // making it sit permanently stationary despite GEInteractionSystem's
    // real patrol movement, caught by tools/VerifyInteractionSystem.cpp).
    //
    // posEndY fixed 2026-07-12 (NEXT.md 8 task 3): rendered UniformCube
    // objects sit with their OWN top face at (currentY + 1.5) -- see
    // GalaxyEggbertCnaGame.cpp's kObjectCubeGroundOffset (+1.0) plus the
    // cube's own half-height (0.5). The old posEndY=8.0f (matching the
    // crow's-nest floor's own grid Y exactly) put the lift's top face a full
    // unit ABOVE the surrounding floor's own top face (9.5 vs 8.5) instead
    // of flush with it, and left the floor solid at that column (see the
    // shaft-opening comment above) -- confirmed live via a temporary debug
    // screenshot (camera override, reverted, not part of this diff) showing
    // the lift parked on top of a fully solid slab with no passage. Lowered
    // by 1 to 7.0f so the lift's top face (7.0+1.0+0.5=8.5) lands exactly
    // flush with the floor's own top face (8.5), plugging the new shaft
    // opening above instead of sitting proud on solid rock.
    {
        MoveObjectRecord lift;
        lift.type = ObjectType::ObjectType1;
        lift.posStartX = 50.0f; lift.posStartY = 4.0f; lift.posStartZ = 28.0f;
        lift.posEndX = 50.0f;   lift.posEndY = 7.0f;   lift.posEndZ = 28.0f;
        PlaceMoveObject(world, lift);
    }

    // North hill: 2 eggs + 1 chest along the ascent, 1 wasp patrolling the
    // plateau, 1 more chest + the level-exit goal in the crow's-nest.
    place(ObjectType::ObjectType6, 47.0f, 1.0f, 44.0f);  // egg, on the connector
    place(ObjectType::ObjectType6, 52.0f, 3.0f, 36.0f);  // egg, partway up the steps
    place(ObjectType::ObjectType5, 45.0f, 5.0f, 29.0f);  // chest, on the plateau
    place(ObjectType::ObjectType44, 50.0f, 5.0f, 30.0f); // wasp, patrolling the plateau
    place(ObjectType::ObjectType5, 51.0f, 9.0f, 28.0f);  // chest, in the crow's-nest
    place(ObjectType::ObjectType7, 49.0f, 9.0f, 28.0f);  // level-exit goal, in the crow's-nest

    // South tunnel: 4 crates blocking/lining the path, 1 patrol enemy, 1
    // spider (real shared kill-list contact-death, plan.md E3D-MIG-132
    // widened 2026-07-11), 1 chest tucked at the grate/pit end, 1 key past
    // the water hazard.
    place(ObjectType::ObjectType12, 35.0f, 1.0f, 67.0f);
    place(ObjectType::ObjectType12, 38.0f, 1.0f, 67.0f);
    place(ObjectType::ObjectType12, 52.0f, 1.0f, 67.0f);
    place(ObjectType::ObjectType12, 63.0f, 1.0f, 67.0f);
    place(ObjectType::ObjectType2, 41.0f, 1.0f, 67.0f);   // standard patrol enemy
    place(ObjectType::ObjectType16, 57.0f, 1.0f, 67.0f);  // spider
    place(ObjectType::ObjectType49, 44.0f, 1.0f, 67.0f);  // key 1, guarding the water crossing
    place(ObjectType::ObjectType5, 25.0f, 1.0f, 67.0f);   // chest, over the grate

    // Walled room on the hill: not just empty architecture -- 1 chest, 1
    // large creature guarding it, and 1 more standard enemy by the doorway.
    place(ObjectType::ObjectType5, 8.0f, 11.0f, 50.0f);
    place(ObjectType::ObjectType2, 18.0f, 11.0f, 49.0f);

    // Large creature (ObjectType54, plan.md E3D-MIG-136, 2026-07-11) --
    // needs a real posStart != posEnd patrol path, same reason the lift/
    // blupih/blupit needed PlaceMoveObject directly above (place() always
    // sets them equal, and the real turn-dwell lethality gate never
    // advances past patrolStep 1 at all when it can't patrol). Patrols
    // between the two decorative pillars (x=12..16, clear of both x=10
    // pillars and the x=20 doorway), directly between the chest (x=8) and
    // the doorway, so reaching the chest means passing it -- a genuine
    // guardian, not just a specimen. Real behavior: safe to touch while it
    // walks (patrolStep 2/4), lethal only during its turn-dwell
    // (patrolStep 1/3, see GEInteractionSystem.cpp).
    {
        MoveObjectRecord creature;
        creature.type = ObjectType::ObjectType54;
        creature.posStartX = 12.0f; creature.posStartY = 11.0f; creature.posStartZ = 50.0f;
        creature.posEndX = 16.0f;   creature.posEndY = 11.0f;   creature.posEndZ = 50.0f;
        PlaceMoveObject(world, creature);
    }

    // ------------------------------------------------------------------
    // Blupih/blupit stationary shooters (plan.md E3D-MIG-134, 2026-07-11) --
    // a real placement, not just the static (posStart==posEnd, never fires)
    // exhibition specimen below, so the actual projectile attack is
    // genuinely playable. Both need place()-shaped placements REJECTED --
    // firing is gated on the real turn-dwell cycle, which never advances at
    // all when posStart==posEnd (same real guard the platform lift needed
    // above), so both use PlaceMoveObject directly with a small but
    // non-zero patrol range, in a previously-empty area south of the tunnel.
    // ------------------------------------------------------------------
    // Blupih perch: a 3x3 ledge with a notch open straight down to a floor
    // 3 cells below, so its dwell-frame-21 downward shot (Decor.cpp:8878-
    // 8886) has real room to fall and land. Only the posStart-side dwell
    // sits over the notch (posEnd sits back on solid ledge, so that half of
    // the cycle drops nothing -- a real "no room" cancellation per
    // SearchAirDistance's own comment, not a bug), so watching one full
    // cycle shows both a live shot and a cancelled one.
    fill(84, 86, 3, 3, 79, 81, BlockTypes::RockPile);
    world.setBlock(85, 3, 80, Block::make(BlockTypes::Air));       // notch: clear shot straight down
    world.setBlock(85, 0, 80, Block::make(BlockTypes::RockPile));  // landing floor, 3 cells below
    {
        MoveObjectRecord blupih;
        blupih.type = ObjectType::ObjectType32;
        blupih.posStartX = 85.0f; blupih.posStartY = 4.0f; blupih.posStartZ = 80.0f; // over the notch
        blupih.posEndX = 86.0f;   blupih.posEndY = 4.0f;   blupih.posEndZ = 80.0f;   // over solid ledge
        PlaceMoveObject(world, blupih);
    }

    // Blupit sentry corridor: a narrow walled passage so its two real
    // horizontal shots (dwell-frame 3 away from the upcoming walk
    // direction, dwell-frame 21 toward it, Decor.cpp:8928-8969) travel
    // toward and land against real walls instead of an unbounded plain.
    fill(78, 92, 0, 0, 85, 85, BlockTypes::RockPile);
    world.setBlock(78, 1, 85, Block::make(BlockTypes::BrickWall)); // west wall
    world.setBlock(92, 1, 85, Block::make(BlockTypes::BrickWall)); // east wall
    {
        MoveObjectRecord blupit;
        blupit.type = ObjectType::ObjectType33;
        blupit.posStartX = 84.0f; blupit.posStartY = 1.0f; blupit.posStartZ = 85.0f;
        blupit.posEndX = 85.0f;   blupit.posEndY = 1.0f;   blupit.posEndZ = 85.0f;
        PlaceMoveObject(world, blupit);
    }

    // ------------------------------------------------------------------
    // Teleporter pair (plan.md E3D-MIG-147, 2026-07-11, redesigned
    // 2026-07-11 per live-playtest user feedback) -- two small open rooms,
    // each with exactly one Teleport1 (icon 330) pillar FLOATING one cell
    // above the walkable floor (teleporter icons are always non-solid for
    // collision, GEBlupiController::GroundHeightAt's own IsTeleporterIcon()
    // skip, so Blupi genuinely walks INTO the open space directly beneath
    // it -- matching the real "one tile above his feet" detection exactly,
    // GEBlupiController::GetBlockTypeAbove()). Not a wide multi-cell
    // teleporter structure -- GEWorldRuntime::FindTeleportDestination()
    // matches by exact block type, so more than one cell per room would
    // risk matching another cell of the SAME room instead of the other
    // one (see that method's own comment). South of the tunnel,
    // previously-empty space.
    // ------------------------------------------------------------------
    fill(10, 16, 0, 0, 71, 77, BlockTypes::RockPile); // room A floor
    world.setBlock(13, 2, 74, Block::make(BlockTypes::Teleport1)); // floating pillar, walk beneath it

    fill(30, 36, 0, 0, 71, 77, BlockTypes::RockPile); // room B floor
    world.setBlock(33, 2, 74, Block::make(BlockTypes::Teleport1)); // floating pillar, walk beneath it

    // ------------------------------------------------------------------
    // Fan hazard pair (plan.md E3D-MIG-149, 2026-07-11) -- two more small
    // OPEN rooms, same "no walls/ceiling at all" pattern as the teleporter
    // rooms directly above, each with a fan head FLOATING one cell above
    // the walkable floor. Deliberately NOT placed inside the south
    // tunnel's own enclosed/roofed interior (where the 2 purely-visual
    // fan placements above sit) -- a real, pre-existing
    // GEBlupiController::GroundHeightAt() limitation was found while
    // building this task: it always resolves a column's "floor" as the
    // SINGLE topmost solid block in that ENTIRE column (scanning from the
    // top of the world down), with no concept of "the nearest solid
    // surface AT OR BELOW my own current height" -- so a solid ceiling
    // anywhere above an open interior (like the tunnel's own y=3
    // BrickWall roof) makes that interior's REAL floor (y=1, resting on
    // y=0 RockPile) completely unreachable via normal walking: Blupi gets
    // resolved onto TOP of the ceiling (y=4) instead, the same failure
    // mode already documented for a floating solid pillar (the ORIGINAL
    // teleporter bug, see that placement's own comment above) but here
    // triggered by a ceiling rather than a pillar. Confirmed live via a
    // standalone scripted walk test (not just single-position Step()
    // calls, which is all every other hazard's own verification in this
    // session actually exercised, including the tunnel's own already-
    // shipped switch/saw pair -- none of them walk-tested entering this
    // specific enclosed interior either). This is a real, deeper
    // collision-architecture limitation affecting ANY roofed/enclosed
    // space, not something fixed here -- see NEXT.md §5 for the tracked
    // limitation. Sidestepped for THIS task the same way the teleporter's
    // own redesign sidestepped a different collision limitation: choosing
    // an open-sky placement instead of attempting a collision-system
    // rewrite.
    // ------------------------------------------------------------------
    fill(10, 16, 0, 0, 79, 85, BlockTypes::RockPile); // fan room A floor
    world.setBlock(13, 2, 82, Block::make(BlockTypes::FanLeft)); // floating fan, walk beneath it

    fill(30, 36, 0, 0, 79, 85, BlockTypes::RockPile); // fan room B floor
    world.setBlock(33, 2, 82, Block::make(BlockTypes::FanRight)); // floating fan, walk beneath it

    // ------------------------------------------------------------------
    // Two more platform lifts (NEXT.md 8 task 3, 2026-07-12, user request:
    // "vice presouvacich bloku" -- more moving/lift blocks), bringing the
    // total to 3. Same open-sky room pattern as the teleporter/fan rooms
    // directly above (a flat floor, no walls or ceiling at all) so neither
    // lift's own shaft risks the GroundHeightAt() roofed-interior limitation
    // (NEXT.md 5) the tunnel's fixtures already hit twice. Each destination
    // platform has its own carved shaft opening at the lift's exact column,
    // same fix as the north-hill lift above -- posEndY = platformGridY - 1
    // so the lift's own top face (posEndY + 1.5) lands flush with the
    // platform's top face (platformGridY + 0.5), plugging the hole instead
    // of sitting proud on solid rock.
    // ------------------------------------------------------------------
    fill(45, 51, 0, 0, 71, 77, BlockTypes::RockPile); // lift room A floor
    fill(48, 50, 5, 5, 73, 75, BlockTypes::RockPile); // destination platform, y=5
    world.setBlock(49, 5, 74, Block::make(BlockTypes::Air)); // lift shaft opening
    {
        MoveObjectRecord liftA;
        liftA.type = ObjectType::ObjectType1;
        liftA.posStartX = 49.0f; liftA.posStartY = 0.0f; liftA.posStartZ = 74.0f;
        liftA.posEndX = 49.0f;   liftA.posEndY = 4.0f;   liftA.posEndZ = 74.0f;
        PlaceMoveObject(world, liftA);
    }

    fill(53, 59, 0, 0, 71, 77, BlockTypes::RockPile); // lift room B floor
    fill(55, 57, 3, 3, 73, 75, BlockTypes::RockPile); // destination platform, y=3 (shorter rise)
    world.setBlock(56, 3, 74, Block::make(BlockTypes::Air)); // lift shaft opening
    {
        MoveObjectRecord liftB;
        liftB.type = ObjectType::ObjectType1;
        liftB.posStartX = 56.0f; liftB.posStartY = 0.0f; liftB.posStartZ = 74.0f;
        liftB.posEndX = 56.0f;   liftB.posEndY = 2.0f;   liftB.posEndZ = 74.0f;
        PlaceMoveObject(world, liftB);
    }

    // ------------------------------------------------------------------
    // Deep water pool (plan.md E3D-MIG-148, 2026-07-12) -- real Nage
    // (fully submerged swimming) / breath-gauge / drowning mechanic, same
    // open-sky room pattern as the teleporter/fan/lift rooms above. A 2-cell
    // -deep pool (Water1 at y=1 AND y=2, resting on a real solid floor at
    // y=0) so Blupi genuinely sinks in and finds water both at his own
    // position and directly above it (Nage), not just a 1-layer wade
    // (Surf) like the tunnel's own shallow crossing.
    // ------------------------------------------------------------------
    fill(61, 67, 0, 0, 71, 77, BlockTypes::RockPile); // pool room floor
    fill(63, 65, 1, 2, 73, 75, BlockTypes::Water1);   // 3x3 pool, 2 layers deep

    // Linked-crate demo (plan.md E3D-MIG-150, 2026-07-12) -- 2 crates
    // side-by-side (real SearchLinkCaisse horizontal linking) plus a third
    // stacked on top of one of them (real vertical linking), on the same
    // room's floor away from the pool, so pushing the pair genuinely moves
    // all 3 atomically -- not just unit-tested.
    place(ObjectType::ObjectType12, 61.0f, 1.0f, 73.0f);
    place(ObjectType::ObjectType12, 62.0f, 1.0f, 73.0f);
    place(ObjectType::ObjectType12, 61.0f, 2.0f, 73.0f);

    // Dynamite demo (plan.md E3D-MIG-155, 2026-07-12) -- one stick in lift
    // room B, a short walk from the linked-crate demo above, so picking it
    // up and placing it near the crates is a genuinely playable scenario,
    // not just a unit test.
    place(ObjectType::ObjectType55, 58.0f, 1.0f, 73.0f);

    // ------------------------------------------------------------------
    // Doors demo (plan.md E3D-MIG-160/161/162, 2026-07-12) -- a small room
    // south of the lift/pool rooms, two short corridors each blocked by a
    // real wall with exactly one door-tile gap: a key-gated Door1 (needs
    // Key1/ObjectType49, picked up just before it) and, further along, a
    // treasure-gated door (icon 422, needs 2 treasures -- deliberately more
    // than the single chest a normal playthrough would have collected by
    // here, so this one stays genuinely closed until a second treasure is
    // found elsewhere in the level, demonstrating the gate rather than
    // starting pre-opened).
    // ------------------------------------------------------------------
    fill(45, 51, 0, 0, 87, 95, BlockTypes::RockPile);           // doors room floor
    fill(45, 51, 1, 2, 90, 90, BlockTypes::BrickWall);          // wall 1 (key-gated door)
    world.setBlock(48, 1, 90, Block::make(BlockTypes::Door1));  // key-gated door gap
    fill(45, 51, 1, 2, 93, 93, BlockTypes::BrickWall);          // wall 2 (treasure-gated door)
    world.setBlock(48, 1, 93, Block::make(static_cast<std::uint16_t>(422))); // treasure-gated door gap (needs 2)
    place(ObjectType::ObjectType49, 46.0f, 1.0f, 88.0f);        // key 1, before wall 1

    // ------------------------------------------------------------------
    // Secret powers demo (plan.md E3D-MIG-170, 2026-07-12) -- one of each
    // real pickup (Shield/Sucette-Power/Drink-Hide/Charge-Cloud) in a small
    // open room, so all 4 real buffs (and the Shield/Hide hazard-immunity
    // they grant, verified this session against dozens of Decor.cpp call
    // sites) are genuinely playable, not just unit-tested.
    // ------------------------------------------------------------------
    fill(53, 59, 0, 0, 87, 93, BlockTypes::RockPile); // secret-powers room floor
    place(ObjectType::ObjectType25, 54.0f, 1.0f, 88.0f); // shield stick
    place(ObjectType::ObjectType26, 56.0f, 1.0f, 88.0f); // suction-cup (-> Power)
    place(ObjectType::ObjectType30, 54.0f, 1.0f, 92.0f); // drink (-> Hide)
    place(ObjectType::ObjectType31, 56.0f, 1.0f, 92.0f); // charge (-> Cloud)

    // ------------------------------------------------------------------
    // Vehicle demo (plan.md E3D-MIG-171, 2026-07-12) -- one Jeep pickup
    // (the simplest confirmed ground vehicle) in a small open room, so
    // mount/dismount and the real accel/decel speed ramp are genuinely
    // playable, not just unit-tested. Action button mounts/dismounts it
    // (real: requires the action button at contact, not an automatic
    // walk-over pickup).
    // ------------------------------------------------------------------
    fill(61, 67, 0, 0, 87, 93, BlockTypes::RockPile); // vehicle-demo room floor
    place(ObjectType::ObjectType19, 64.0f, 1.0f, 90.0f); // jeep

    // ------------------------------------------------------------------
    // Bullet pack demo (plan.md E3D-MIG-175, 2026-07-12) -- one ammo pack,
    // automatic pickup (no button, unlike the jeep next door), so the real
    // "caps at 10, no-op once full" gate is genuinely playable, not just
    // unit-tested. The actual firing mechanic (Helicopter/Tank vehicle fire
    // button) is a separate, not-yet-implemented follow-up -- this only
    // exercises the pickup/cap logic.
    // ------------------------------------------------------------------
    fill(69, 75, 0, 0, 87, 93, BlockTypes::RockPile); // bullet-pack-demo room floor
    place(ObjectType::ObjectType29, 72.0f, 1.0f, 90.0f); // bullet pack

    // ------------------------------------------------------------------
    // Exhibition area (2026-07-10, user request): a museum of everything
    // the renderer supports, for visual inspection in-game.
    //
    // Tile exhibition -- the ENTIRE icon range 1..440 on a flat slab in
    // the previously-empty north strip (z=1..24), one block per icon at a
    // 2-cell pitch so every block stands free (all 5 visible faces
    // exposed) and each renders via whatever mode the terrain renderer
    // assigns it (DirectionalCube / InnerPillarBox / InnerFlatPlate /
    // TripleCrossBillboard / water pass / plain UniformCube fallback).
    // Hazard tiles (lava/spikes/saw/Blitz) are genuinely lethal to step
    // ON here, same as anywhere else -- walk the aisles, don't climb the
    // exhibits. Reachable by jumping down from the north hill plateau's
    // north edge (a 4-block drop, well short of the fall-death limit).
    // ------------------------------------------------------------------
    fill(1, 98, 0, 0, 1, 24, BlockTypes::RockPile); // exhibition floor
    for (int icon = 1; icon <= 440; ++icon)
    {
        // Icon 330 (Teleport1) is deliberately skipped here -- it already
        // has a real, matched pair placed above (the teleporter rooms),
        // and a 3rd stray occurrence would risk breaking that pairing
        // (see FindTeleportDestination()'s own comment on why a wide/
        // ambiguous set of same-icon cells is a real risk). Icons 331-333
        // (Teleport2-4) are still exhibited normally as lone specimens,
        // faithfully demonstrating the real "no partner found -> regains
        // control in place" behavior.
        if (icon == BlockTypes::Teleport1)
        {
            continue;
        }
        const int idx = icon - 1;
        const int col = idx % 48;
        const int row = idx / 48;
        world.setBlock(static_cast<std::uint16_t>(3 + col * 2), 1,
                       static_cast<std::uint16_t>(3 + row * 2),
                       Block::make(static_cast<std::uint16_t>(icon)));
    }

    // Object exhibition -- every ObjectType the renderer has an icon for
    // (enumerated via GEObjectIcons::GetObjIcon, the renderer's own
    // source of truth, rather than a hand-duplicated list), on a second
    // slab east of the corridor's end (x=76..97 adjoins the corridor at
    // x=75, so it's a seamless walk east from the tested path). Static
    // exhibits: posEnd == posStart, so nothing patrols. Their real
    // behaviors stay live -- exhibition pickups are collectable (the
    // chest exhibit raises the level's treasure total), the shared-kill-
    // list hazards kill on touch, the wasp balloons -- which is itself
    // part of the exhibition.
    fill(76, 97, 0, 0, 25, 63, BlockTypes::RockPile); // object-exhibition floor
    {
        int slot = 0;
        for (int t = 1; t <= 203; ++t)
        {
            const auto type = static_cast<ObjectType>(t);
            if (GalaxyEggbert::CNA::GetObjIcon(type, 0) == 0)
            {
                continue; // no icon in source data (e.g. 0/18/22/58) -- nothing to exhibit
            }
            const int col = slot % 7;
            const int row = slot / 7;
            place(type, static_cast<float>(78 + col * 3), 1.0f, static_cast<float>(27 + row * 3));
            ++slot;
        }
        std::cout << "GenerateSampleWorld3D: exhibition placed -- 439 tile icons in the exhibition slab "
                     "(icon 330 excluded, see the teleporter rooms above), "
                  << slot << " object types." << std::endl;
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
