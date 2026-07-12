#include "Game/GEBlupiController.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <cmath>
#include <filesystem>
#include <iostream>

// Scripted, non-interactive verification of GEBlupiController's grid
// collision (plan.md E3D-MIG-060): loads a world and drives Step() with
// scripted input instead of live keyboard input, so this proves step-up
// traversal, wall blocking, and gravity/landing actually work against the
// real worlds3d/world001.vwr geometry — not just "compiles and doesn't
// crash".
int main(int argc, char** argv)
{
    using namespace GalaxyEggbert;
    using namespace GalaxyEggbert::CNA;

    const std::filesystem::path worldPath = (argc > 1) ? argv[1] : "worlds3d/world001.vwr";
    const Worlds::World world = Worlds::World::loadFromFile(worldPath);

    bool allOk = true;
    const auto check = [&allOk](bool cond, const char* what)
    {
        std::cout << (cond ? "PASS" : "FAIL") << ": " << what << std::endl;
        if (!cond) allOk = false;
    };

    constexpr float dt = 1.0f / 60.0f;

    // 1. Spawn on the ground floor (world (0,1,0) == grid (50,*,50)); the
    // controller should recognize it is already standing on solid ground.
    GEBlupiController blupi;
    blupi.SetPosition(0.0f, 1.0f, 0.0f);
    blupi.Step(world, 0.0f, 0.0f, false, false, false, dt);
    check(blupi.IsOnGround(), "spawns grounded on the ground floor");

    // 2. Walk west (-X) toward and up the 10-step staircase (world x from
    // -21 down to -30, world z band [-5,4] -- spawn z=0 is inside it).
    // Expect Y to climb via step-up traversal. Tank controls move along the
    // current facing direction, so face west (-X) first: yaw=0 faces -Z,
    // so -90 deg (-pi/2 rad) faces -X.
    constexpr float kFaceWest = -1.57079633f;
    blupi.SetYaw(kFaceWest);
    for (int i = 0; i < 400; ++i) // ~6.7 simulated seconds
    {
        blupi.Step(world, 0.0f, 1.0f, false, false, false, dt);
    }
    std::cout << "After walking west: x=" << blupi.GetX() << " y=" << blupi.GetY()
              << " z=" << blupi.GetZ() << " onGround=" << blupi.IsOnGround() << std::endl;
    check(blupi.GetY() >= 8.0f, "climbed the staircase via step-up traversal (Y >= 8)");
    check(blupi.IsOnGround(), "stands on top of the staircase/platform, not falling through it");

    // 3. Keep walking west into the room's far wall (world x=-45) -- should
    // block, not clip through.
    for (int i = 0; i < 300; ++i)
    {
        blupi.Step(world, 0.0f, 1.0f, false, false, false, dt);
    }
    std::cout << "After walking into wall: x=" << blupi.GetX() << std::endl;
    check(blupi.GetX() > -45.0f, "wall blocks horizontal movement (did not clip through x=-45 wall)");

    // 4. Drop from height over the tested corridor's real floor (world
    // (10,*,0), raw grid (60,*,50) -- inside fill(20,75,0,0,48,52,RockPile))
    // and confirm gravity + landing works. NOT world (40,*,40) (outside any
    // placed structure) as this test originally used -- plan.md
    // E3D-MIG-067's fall-off-world fix (2026-07-11) means a genuinely
    // floorless column now correctly free-falls forever rather than
    // "landing" at a fake Y=0 floor, so this test needs a location with a
    // REAL floor to verify actual gravity/landing physics; the empty-
    // column case is covered separately below (GroundHeightAt() returning
    // kNoGround).
    GEBlupiController faller;
    faller.SetPosition(10.0f, 20.0f, 0.0f);
    for (int i = 0; i < 200 && !faller.IsOnGround(); ++i)
    {
        faller.Step(world, 0.0f, 0.0f, false, false, false, dt);
    }
    std::cout << "Faller landed at y=" << faller.GetY() << std::endl;
    check(faller.IsOnGround(), "falls under gravity and lands");
    check(faller.GetY() < 5.0f, "lands far below the y=20 drop height (real gravity, not a snap)");
    check(faller.GetY() >= 0.0f, "lands on the real corridor floor, not the removed fake Y=0 fallback");

    // 4b. Fall-off-world (plan.md E3D-MIG-067, fixed 2026-07-11): drop over
    // a genuinely floorless column (world (40,*,40), raw grid (90,*,90) --
    // outside any placed structure in this world) and confirm Blupi falls
    // straight through, well past the old buggy Y=0 "floor", rather than
    // stopping there. This is a direct regression test for a real bug
    // found via live playtest: GroundHeightAt()'s old `return 0` fallback
    // for "no solid block in this column" was silently treated as solid
    // ground, making Blupi stop dead at Y=0.000 in open space with
    // nothing beneath him -- confirmed live before this fix (onGround
    // became true at exactly Y=0), and unreachable via any existing test
    // before this one, since every other drop test in this file lands on
    // a real floor.
    GEBlupiController fallsForever;
    fallsForever.SetPosition(40.0f, 20.0f, 40.0f);
    for (int i = 0; i < 180; ++i) // 3s -- comfortably past the y=20->y<0 fall time seen live (~2.25s)
    {
        fallsForever.Step(world, 0.0f, 0.0f, false, false, false, dt);
    }
    std::cout << "Fell-forever Y after 3s over an empty column: " << fallsForever.GetY() << std::endl;
    check(!fallsForever.IsOnGround(), "Blupi never lands over a genuinely floorless column (no fake Y=0 floor)");
    check(fallsForever.GetY() < 0.0f,
          "Blupi's Y drops below 0 over a floorless column, proving he's NOT clamped to a fake floor there");

    // 4c. Animation state: Jump (ascending) vs Air (falling) split
    // (2026-07-11, plan.md E3D-MIG-064 -- expanding the bottom-right
    // animation indicator beyond its original Stop/March/Jump/Down/Up
    // debug-stopgap set). Real BlupiAction IDs 4 (Jump) and 5 (Air); Air's
    // frame data is ported from GalaxyEggbertSimple3D::GEBlupiController's
    // own already-approved kAirFrames, not a fresh mobile-eggbert
    // transcription. Verifies the velocity-sign-based split this class
    // uses (ascending = Jump, falling/apex = Air) in place of Simple3D's
    // own frame-counted trigger window.
    {
        GEBlupiController anim;
        anim.SetPosition(0.0f, 1.0f, 0.0f);
        anim.Step(world, 0.0f, 0.0f, false, false, false, dt);
        check(anim.GetAnimState() == GEBlupiController::AnimState::Stop,
              "grounded and idle starts in the Stop anim state");
        check(anim.GetAnimIcon() == 0, "Stop anim icon is the real icon 0");

        anim.Step(world, 0.0f, 0.0f, true, false, false, dt); // jumpPressed
        check(!anim.IsOnGround(), "jump launches Blupi airborne");
        check(anim.GetAnimState() == GEBlupiController::AnimState::Jump,
              "freshly-launched jump (ascending, velocityY > 0) is the Jump anim state");
        check(anim.GetAnimIcon() == 17, "Jump anim icon starts at the real first jump frame (icon 17)");

        int stepsToApex = 0;
        while (anim.GetAnimState() == GEBlupiController::AnimState::Jump && stepsToApex < 200)
        {
            anim.Step(world, 0.0f, 0.0f, false, false, false, dt);
            ++stepsToApex;
        }
        check(stepsToApex < 200, "reaches the jump apex (velocityY turns non-positive) within a bounded time");
        check(anim.GetAnimState() == GEBlupiController::AnimState::Air,
              "past the apex, falling (velocityY <= 0) switches to the Air anim state, not still Jump");
        check(anim.GetAnimIcon() == 169, "Air anim icon starts at the real first air frame (icon 169)");

        int stepsToLand = 0;
        while (!anim.IsOnGround() && stepsToLand < 200)
        {
            anim.Step(world, 0.0f, 0.0f, false, false, false, dt);
            ++stepsToLand;
        }
        check(anim.IsOnGround(), "lands again after the jump arc completes");
        check(anim.GetAnimState() == GEBlupiController::AnimState::Stop,
              "back on the ground and idle returns to the Stop anim state, not stuck in Air");
    }

    // 5. GetGroundBlockType() (plan.md E3D-MIG-140, lava-hazard detection) --
    // a small synthetic world (not worlds3d/world001.vwr, which has no lava
    // placed yet) with one lava block and one ordinary ground block,
    // isolated from anything else this tool tests against.
    {
        Worlds::World synthetic;
        constexpr std::uint16_t kGroundX = 10, kGroundZ = 10;
        constexpr std::uint16_t kLavaX = 20, kLavaZ = 20;
        synthetic.setBlock(kGroundX, 0, kGroundZ, Worlds::Block::make(BlockTypes::Ground));
        synthetic.setBlock(kLavaX, 0, kLavaZ, Worlds::Block::make(BlockTypes::Lava));

        GEBlupiController onGround;
        onGround.SetPosition(static_cast<float>(kGroundX) - 50.0f /* kWorldCenterX */,
                              1.0f, static_cast<float>(kGroundZ) - 50.0f /* kWorldCenterZ */);
        onGround.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        check(onGround.GetGroundBlockType(synthetic) == BlockTypes::Ground,
              "GetGroundBlockType() identifies ordinary ground correctly");

        GEBlupiController onLava;
        onLava.SetPosition(static_cast<float>(kLavaX) - 50.0f, 1.0f, static_cast<float>(kLavaZ) - 50.0f);
        onLava.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        check(onLava.IsOnGround(), "Blupi stands on a lava block rather than falling through it");
        check(onLava.GetGroundBlockType(synthetic) == BlockTypes::Lava,
              "GetGroundBlockType() identifies lava correctly (E3D-MIG-140 hazard detection)");

        GEBlupiController airborne;
        airborne.SetPosition(static_cast<float>(kLavaX) - 50.0f, 20.0f, static_cast<float>(kLavaZ) - 50.0f);
        check(airborne.GetGroundBlockType(synthetic) == BlockTypes::Air,
              "GetGroundBlockType() returns Air while airborne, even directly above lava");

        // Spikes (plan.md E3D-MIG-141) -- same synthetic world, one more block.
        constexpr std::uint16_t kSpikeX = 30, kSpikeZ = 30;
        synthetic.setBlock(kSpikeX, 0, kSpikeZ, Worlds::Block::make(BlockTypes::Spike));
        GEBlupiController onSpike;
        onSpike.SetPosition(static_cast<float>(kSpikeX) - 50.0f, 1.0f, static_cast<float>(kSpikeZ) - 50.0f);
        onSpike.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        check(onSpike.IsOnGround(), "Blupi stands on a spike block rather than falling through it");
        check(onSpike.GetGroundBlockType(synthetic) == BlockTypes::Spike,
              "GetGroundBlockType() identifies spikes correctly (E3D-MIG-141 hazard detection)");

        // Crusher squash state (plan.md E3D-MIG-143) -- TriggerCrush()/
        // IsEcrased()/recovery, standing on the same ordinary ground block
        // used above (the trigger *condition* -- Crusher block + active
        // cycle -- is GalaxyEggbertCnaGame's job, tested separately in
        // GEWorldRuntime::IsCrusherActiveAtPhase(); this only tests
        // GEBlupiController's own state machine once triggered).
        GEBlupiController crushed;
        crushed.SetPosition(static_cast<float>(kGroundX) - 50.0f, 1.0f, static_cast<float>(kGroundZ) - 50.0f);
        crushed.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        check(crushed.TriggerCrush(), "TriggerCrush() returns true on a genuinely new trigger");
        check(crushed.IsEcrased(), "IsEcrased() is true immediately after TriggerCrush()");
        check(!crushed.TriggerCrush(), "TriggerCrush() is a no-op (returns false) while already squashed");

        // Animation indicator (plan.md E3D-MIG-064, 2026-07-11, real
        // BlupiAction IDs 72/73, `table_blupi` icon data transcribed with
        // explicit user approval): squashed+idle is StopEcrase, real icon
        // 320.
        crushed.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        check(crushed.GetAnimState() == GEBlupiController::AnimState::StopEcrase,
              "squashed and idle is the StopEcrase anim state");
        check(crushed.GetAnimIcon() == 320, "StopEcrase anim icon is the real icon 320");

        // Reduced move speed while squashed: same moveInput/dt, less
        // distance covered than an un-squashed Blupi over one Step(). At
        // yaw=0 (the default), forward movement changes Z, not X.
        const float zBeforeCrushedMove = crushed.GetZ();
        crushed.Step(synthetic, 0.0f, 1.0f, false, false, false, dt);
        const float crushedDelta = std::fabs(crushed.GetZ() - zBeforeCrushedMove);
        check(crushed.GetAnimState() == GEBlupiController::AnimState::MarchEcrase,
              "squashed and moving switches to the MarchEcrase anim state");
        check(crushed.GetAnimIcon() == 319, "MarchEcrase anim icon starts at the real first frame (icon 319)");

        GEBlupiController normal;
        normal.SetPosition(static_cast<float>(kGroundX) - 50.0f, 1.0f, static_cast<float>(kGroundZ) - 50.0f);
        normal.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        const float zBeforeNormalMove = normal.GetZ();
        normal.Step(synthetic, 0.0f, 1.0f, false, false, false, dt);
        const float normalDelta = std::fabs(normal.GetZ() - zBeforeNormalMove);
        check(crushedDelta > 0.0f && crushedDelta < normalDelta,
              "squashed Blupi moves slower than normal, but still moves (not fully immobilized)");

        // Jump blocked while squashed.
        crushed.Step(synthetic, 0.0f, 0.0f, true, false, false, dt);
        check(crushed.IsOnGround(), "jump input is ignored while squashed (still on ground, not launched)");

        // Auto-recovery after kEcraseDuration seconds.
        const int stepsToRecover = static_cast<int>(GEBlupiController::kEcraseDuration / dt) + 5;
        for (int i = 0; i < stepsToRecover && crushed.IsEcrased(); ++i)
        {
            crushed.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        }
        check(!crushed.IsEcrased(), "squash state auto-recovers after kEcraseDuration seconds");

        // Wasp "balloon" status (plan.md E3D-MIG-135) -- TriggerBalloon()/
        // IsBallooned()/PopBalloon(), same synthetic world (falling
        // behavior only, doesn't need any particular ground block).
        GEBlupiController ballooned;
        ballooned.SetPosition(0.0f, 20.0f, 0.0f);
        check(ballooned.TriggerBalloon(), "TriggerBalloon() returns true on a genuinely new trigger");
        check(ballooned.IsBallooned(), "IsBallooned() is true immediately after TriggerBalloon()");
        check(!ballooned.TriggerBalloon(), "TriggerBalloon() is a no-op (returns false) while already ballooned");

        // Animation indicator (plan.md E3D-MIG-064, 2026-07-11, real
        // BlupiAction ID 66, `table_blupi` icon data transcribed with
        // explicit user approval): ballooned is always the Balloon anim
        // state, regardless of grounded/airborne (the real data has no
        // separate air variant).
        ballooned.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        check(ballooned.GetAnimState() == GEBlupiController::AnimState::Balloon,
              "ballooned is the Balloon anim state");
        check(ballooned.GetAnimIcon() == 291, "Balloon anim icon starts at the real first frame (icon 291)");

        // Reduced gravity while ballooned: falls less over the same number
        // of steps than a normal Blupi dropped from the same height.
        GEBlupiController falling;
        falling.SetPosition(0.0f, 20.0f, 0.0f);
        constexpr int kFallSteps = 30; // ~0.5s, short enough neither instance reaches the ground
        for (int i = 0; i < kFallSteps; ++i)
        {
            ballooned.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            falling.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        }
        check(ballooned.GetY() > falling.GetY(),
              "ballooned Blupi falls slower than normal (reduced gravity, approximates 'floats')");

        // PopBalloon() clears the status early and forces Blupi briefly
        // airborne, matching the real m_blupiAir=true on a hazard pop.
        ballooned.PopBalloon();
        check(!ballooned.IsBallooned(), "PopBalloon() clears the balloon status immediately");
        check(!ballooned.IsOnGround(), "PopBalloon() forces Blupi airborne (real m_blupiAir=true)");

        // PopBalloon() while NOT ballooned is a documented no-op.
        GEBlupiController notBallooned;
        notBallooned.SetPosition(static_cast<float>(kGroundX) - 50.0f, 1.0f, static_cast<float>(kGroundZ) - 50.0f);
        notBallooned.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        notBallooned.PopBalloon();
        check(notBallooned.IsOnGround(), "PopBalloon() is a no-op while not ballooned (still grounded)");

        // Auto-recovery after kBalloonDuration seconds.
        GEBlupiController recovering;
        recovering.SetPosition(0.0f, 20.0f, 0.0f);
        recovering.TriggerBalloon();
        const int stepsToRecoverBalloon = static_cast<int>(GEBlupiController::kBalloonDuration / dt) + 5;
        for (int i = 0; i < stepsToRecoverBalloon && recovering.IsBallooned(); ++i)
        {
            recovering.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        }
        check(!recovering.IsBallooned(), "balloon status auto-recovers after kBalloonDuration seconds");

        // Spring bounce (plan.md E3D-MIG-145) -- same synthetic world, one
        // more block (icon 211 = BlockTypes::Spring). GetGroundBlockType()
        // detection first (the trigger *condition* -- GalaxyEggbertCnaGame's
        // job), then TriggerSpringBounce()'s own state machine in
        // isolation, same split as the Crusher tests above.
        constexpr std::uint16_t kSpringX = 40, kSpringZ = 40;
        synthetic.setBlock(kSpringX, 0, kSpringZ, Worlds::Block::make(BlockTypes::Spring));
        GEBlupiController onSpring;
        onSpring.SetPosition(static_cast<float>(kSpringX) - 50.0f, 1.0f, static_cast<float>(kSpringZ) - 50.0f);
        onSpring.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        check(onSpring.IsOnGround(), "Blupi stands on a spring block rather than falling through it");
        check(onSpring.GetGroundBlockType(synthetic) == BlockTypes::Spring,
              "GetGroundBlockType() identifies a spring correctly (E3D-MIG-145 hazard detection)");

        check(onSpring.TriggerSpringBounce(/*jumpHeld=*/false),
              "TriggerSpringBounce() returns true while grounded");
        check(!onSpring.IsOnGround(), "the bounce launches Blupi airborne immediately");
        check(!onSpring.TriggerSpringBounce(/*jumpHeld=*/false),
              "TriggerSpringBounce() is a no-op (returns false) while already airborne");

        // Held-jump bounce launches noticeably higher than a not-held
        // bounce, matching the real source's two distinct magnitudes
        // (-19 held vs -10 not-held, both noPower).
        GEBlupiController bounceHeld;
        bounceHeld.SetPosition(static_cast<float>(kSpringX) - 50.0f, 1.0f, static_cast<float>(kSpringZ) - 50.0f);
        bounceHeld.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        bounceHeld.TriggerSpringBounce(/*jumpHeld=*/true);

        GEBlupiController bounceNotHeld;
        bounceNotHeld.SetPosition(static_cast<float>(kSpringX) - 50.0f, 1.0f, static_cast<float>(kSpringZ) - 50.0f);
        bounceNotHeld.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        bounceNotHeld.TriggerSpringBounce(/*jumpHeld=*/false);

        constexpr int kBounceSampleSteps = 10;
        for (int i = 0; i < kBounceSampleSteps; ++i)
        {
            bounceHeld.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            bounceNotHeld.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        }
        std::cout << "Spring bounce Y after " << kBounceSampleSteps << " steps: held=" << bounceHeld.GetY()
                  << " notHeld=" << bounceNotHeld.GetY() << std::endl;
        check(bounceHeld.GetY() > bounceNotHeld.GetY(),
              "holding Jump on contact launches Blupi noticeably higher than not holding it");

        // Vanishing/Temp tile (plan.md E3D-MIG-146) -- a real Ground block
        // (Y=0) with a Temp tile directly above it (Y=1), so
        // tempPassable=true has a genuine lower floor to fall through TO,
        // avoiding any ambiguity with GroundHeightAt's own "nothing solid
        // anywhere in this column" fallback.
        constexpr std::uint16_t kTempX = 45, kTempZ = 45;
        synthetic.setBlock(kTempX, 0, kTempZ, Worlds::Block::make(BlockTypes::Ground));
        synthetic.setBlock(kTempX, 1, kTempZ, Worlds::Block::make(BlockTypes::Temp));

        GEBlupiController onTemp;
        onTemp.SetPosition(static_cast<float>(kTempX) - 50.0f, 2.0f, static_cast<float>(kTempZ) - 50.0f);
        onTemp.Step(synthetic, 0.0f, 0.0f, false, false, false, dt, /*tempPassable=*/false);
        check(onTemp.IsOnGround(), "Blupi stands on a Temp tile during its solid (non-passable) window");
        check(onTemp.GetGroundBlockType(synthetic) == BlockTypes::Temp,
              "GetGroundBlockType() identifies a Temp tile correctly (E3D-MIG-146 detection)");

        // During the real passable window (2 of 20 phase buckets), the SAME
        // Temp tile stops being solid ground -- Blupi should fall through
        // to the real Ground block one cell below instead of staying put.
        for (int i = 0; i < 60 && !(onTemp.IsOnGround() && onTemp.GetY() < 1.9f); ++i)
        {
            onTemp.Step(synthetic, 0.0f, 0.0f, false, false, false, dt, /*tempPassable=*/true);
        }
        std::cout << "Blupi Y after the Temp tile turns passable: " << onTemp.GetY() << std::endl;
        check(std::fabs(onTemp.GetY() - 1.0f) < 0.1f,
              "Blupi falls through the Temp tile onto the real ground one cell below once it's passable");
        check(onTemp.GetGroundBlockType(synthetic) == BlockTypes::Ground,
              "Blupi now stands on the real Ground block beneath, not the Temp tile");

        // Teleporter (plan.md E3D-MIG-147) -- a real Ground floor (Y=0)
        // with a Teleport1 pillar FLOATING one cell above Blupi's standing
        // height (Y=2), matching the real "one tile above his feet"
        // detection exactly. Teleporter icons are always non-solid for
        // collision (GroundHeightAt's own IsTeleporterIcon() skip), so
        // Blupi must land on the REAL floor beneath it (Y=1), not be
        // blocked by or land on top of the floating pillar -- this is
        // also a direct regression test for the bug this redesign fixed
        // (an earlier attempt made teleporter pillars solid, which made
        // Blupi land ON the pillar instead of the floor beneath it).
        constexpr std::uint16_t kTeleX = 42, kTeleZ = 42;
        synthetic.setBlock(kTeleX, 0, kTeleZ, Worlds::Block::make(BlockTypes::Ground));
        synthetic.setBlock(kTeleX, 2, kTeleZ, Worlds::Block::make(BlockTypes::Teleport1));

        GEBlupiController onTeleporter;
        onTeleporter.SetPosition(static_cast<float>(kTeleX) - 50.0f, 1.0f, static_cast<float>(kTeleZ) - 50.0f);
        onTeleporter.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        std::cout << "Blupi Y beneath the floating teleporter pillar: " << onTeleporter.GetY() << std::endl;
        check(onTeleporter.IsOnGround() && std::fabs(onTeleporter.GetY() - 1.0f) < 0.01f,
              "Blupi stands on the real floor beneath the floating teleporter pillar, not on/blocked by it");
        check(onTeleporter.GetBlockTypeAbove(synthetic) == BlockTypes::Teleport1,
              "GetBlockTypeAbove() identifies the teleporter pillar one cell above Blupi (E3D-MIG-147 detection)");

        check(onTeleporter.TriggerTeleport(BlockTypes::Teleport1), "TriggerTeleport() returns true while grounded");
        check(onTeleporter.IsTeleporting(), "IsTeleporting() is true immediately after TriggerTeleport()");
        check(onTeleporter.GetTeleportIcon() == BlockTypes::Teleport1, "GetTeleportIcon() remembers which icon triggered it");
        check(!onTeleporter.TriggerTeleport(BlockTypes::Teleport1),
              "TriggerTeleport() is a no-op (returns false) while already teleporting");

        // Animation indicator (plan.md E3D-MIG-064, 2026-07-11, real
        // BlupiAction ID 74, `table_blupi` icon data transcribed with
        // explicit user approval): teleporting is always the Teleporting
        // anim state. Real frame 0 is icon 1 (not -1), so no invisible-
        // frame substitution is exercised at this specific phase -- see
        // GetAnimIcon()'s own comment for that behavior.
        onTeleporter.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        check(onTeleporter.GetAnimState() == GEBlupiController::AnimState::Teleporting,
              "teleporting is the Teleporting anim state");
        check(onTeleporter.GetAnimIcon() == 1, "Teleporting anim icon starts at the real first frame (icon 1)");

        // Fully frozen during transit: turning/moving input has no effect
        // at all (real m_blupiFocus=false blocks essentially every other
        // per-frame input block).
        const float xBeforeFrozenStep = onTeleporter.GetX();
        const float yawBeforeFrozenStep = onTeleporter.GetYaw();
        onTeleporter.Step(synthetic, 1.0f, 1.0f, true, false, false, dt);
        check(onTeleporter.GetX() == xBeforeFrozenStep && onTeleporter.GetYaw() == yawBeforeFrozenStep,
              "Blupi is fully frozen (no movement or turning) while teleporting");

        // Auto-completion after kTeleportDuration seconds.
        const int stepsToTeleport = static_cast<int>(GEBlupiController::kTeleportDuration / dt) + 5;
        for (int i = 0; i < stepsToTeleport && onTeleporter.IsTeleporting(); ++i)
        {
            onTeleporter.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        }
        check(!onTeleporter.IsTeleporting(), "teleport transit auto-completes after kTeleportDuration seconds");

        // Real gate: grounded, not ballooned, not squashed.
        GEBlupiController airborneTeleport;
        airborneTeleport.SetPosition(0.0f, 20.0f, 0.0f);
        check(!airborneTeleport.TriggerTeleport(BlockTypes::Teleport1),
              "TriggerTeleport() is a no-op while airborne (real !m_blupiAir gate)");

        GEBlupiController ballooned2;
        ballooned2.SetPosition(static_cast<float>(kTeleX) - 50.0f, 1.0f, static_cast<float>(kTeleZ) - 50.0f);
        ballooned2.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        ballooned2.TriggerBalloon();
        check(!ballooned2.TriggerTeleport(BlockTypes::Teleport1),
              "TriggerTeleport() is a no-op while ballooned (real !m_blupiBalloon gate)");

        GEBlupiController crushed2;
        crushed2.SetPosition(static_cast<float>(kTeleX) - 50.0f, 1.0f, static_cast<float>(kTeleZ) - 50.0f);
        crushed2.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        crushed2.TriggerCrush();
        check(!crushed2.TriggerTeleport(BlockTypes::Teleport1),
              "TriggerTeleport() is a no-op while squashed (real !m_blupiEcrase gate)");

        // Fan hazard collision (plan.md E3D-MIG-149) -- same non-solid
        // architecture as the teleporter above: a real Ground floor (Y=0)
        // with a FanLeft head FLOATING one cell above Blupi's standing
        // height (Y=2), matching the real placement convention already
        // established for the teleporter (GEWorldRuntime::TryConsumeFan()'s
        // own comment). Fan head icons are always non-solid for collision
        // (GroundHeightAt's own BlockTypes::isFan() skip), so Blupi must
        // land on the REAL floor beneath it (Y=1), not be blocked by or
        // land on top of the fan.
        constexpr std::uint16_t kFanX = 44, kFanZ = 44;
        synthetic.setBlock(kFanX, 0, kFanZ, Worlds::Block::make(BlockTypes::Ground));
        synthetic.setBlock(kFanX, 2, kFanZ, Worlds::Block::make(BlockTypes::FanLeft));

        GEBlupiController underFan;
        underFan.SetPosition(static_cast<float>(kFanX) - 50.0f, 1.0f, static_cast<float>(kFanZ) - 50.0f);
        underFan.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        std::cout << "Blupi Y beneath the floating fan: " << underFan.GetY() << std::endl;
        check(underFan.IsOnGround() && std::fabs(underFan.GetY() - 1.0f) < 0.01f,
              "Blupi stands on the real floor beneath the floating fan, not on/blocked by it");
        check(underFan.GetBlockTypeAbove(synthetic) == BlockTypes::FanLeft,
              "GetBlockTypeAbove() identifies the fan head one cell above Blupi (E3D-MIG-149 detection)");

        // Real 10-slot safe-position FIFO respawn (plan.md E3D-MIG-067) --
        // a 13-wide flat strip so Blupi can occupy 13 distinct safe grid
        // positions in a row (more than the 10-slot capacity), to prove
        // both that the FIFO tracks a lagged "safe" position and that it
        // shifts out its oldest entry once full.
        constexpr std::uint16_t kSafeStripZ = 60;
        for (std::uint16_t x = 60; x <= 72; ++x)
        {
            synthetic.setBlock(x, 0, kSafeStripZ, Worlds::Block::make(BlockTypes::Ground));
        }

        GEBlupiController defaultValid;
        check(defaultValid.GetValidX() == 0.0f && defaultValid.GetValidY() == 1.0f && defaultValid.GetValidZ() == 0.0f,
              "GetValidX/Y/Z() default to the spawn point before any safe frame is ever recorded");

        GEBlupiController airborneSafe;
        airborneSafe.SetPosition(0.0f, 20.0f, 0.0f);
        airborneSafe.UpdateSafePosition(/*externallySafe=*/true);
        check(airborneSafe.GetValidX() == 0.0f && airborneSafe.GetValidZ() == 0.0f,
              "UpdateSafePosition() is a no-op while airborne (still the spawn-point default)");

        GEBlupiController unsafeCaller;
        unsafeCaller.SetPosition(60.0f - 50.0f, 1.0f, static_cast<float>(kSafeStripZ) - 50.0f);
        unsafeCaller.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        unsafeCaller.UpdateSafePosition(/*externallySafe=*/false);
        check(unsafeCaller.GetValidX() == 0.0f && unsafeCaller.GetValidZ() == 0.0f,
              "UpdateSafePosition() is a no-op when the caller reports externallySafe=false");

        // Walk across 13 distinct grid positions (one per frame), each a
        // real safe frame -- the FIFO holds only 10, so the tracked valid
        // position should lag behind Blupi's current position by roughly
        // that buffer, never equal to (or ahead of) wherever he currently
        // is.
        GEBlupiController safeWalker;
        for (int i = 0; i < 13; ++i)
        {
            safeWalker.SetPosition(static_cast<float>(60 + i) - 50.0f, 1.0f,
                                    static_cast<float>(kSafeStripZ) - 50.0f);
            safeWalker.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            safeWalker.UpdateSafePosition(/*externallySafe=*/true);
        }
        std::cout << "Safe walker at X=" << safeWalker.GetX() << ", valid respawn X=" << safeWalker.GetValidX()
                  << std::endl;
        check(safeWalker.GetValidX() < safeWalker.GetX(),
              "the tracked valid respawn position lags behind Blupi's current position (the real 'buffer'), "
              "not equal to or ahead of it");
        check(safeWalker.GetX() - safeWalker.GetValidX() >= 9.0f,
              "the lag is roughly the FIFO's 10-slot capacity, proving the oldest entry is what's used, "
              "not just the immediately-prior one");

        // Water Surf/Nage/drowning (plan.md E3D-MIG-148) -- a shallow
        // 1-layer pool (Ground at y=0, Water1 at y=1) and a deep 2-layer
        // pool (Ground at y=0, Water1 at y=1 AND y=2) elsewhere in the same
        // synthetic world. Water is now non-solid for collision (see
        // GroundHeightAt's own comment), so Blupi sinks through any depth
        // of water to the real floor beneath it -- the tile(s) at/above his
        // resting position then determine Surf vs Nage, matching real
        // mobile-eggbert's per-tile-independent collision well enough for
        // this purpose, the same technique already used for the teleporter
        // pillar and fan head.
        constexpr std::uint16_t kShallowX = 80, kShallowZ = 80;
        synthetic.setBlock(kShallowX, 0, kShallowZ, Worlds::Block::make(BlockTypes::Ground));
        synthetic.setBlock(kShallowX, 1, kShallowZ, Worlds::Block::make(BlockTypes::Water1));

        constexpr std::uint16_t kDeepX = 85, kDeepZ = 80;
        synthetic.setBlock(kDeepX, 0, kDeepZ, Worlds::Block::make(BlockTypes::Ground));
        synthetic.setBlock(kDeepX, 1, kDeepZ, Worlds::Block::make(BlockTypes::Water1));
        synthetic.setBlock(kDeepX, 2, kDeepZ, Worlds::Block::make(BlockTypes::Water1));

        const auto stepWithWaterDetection = [](GEBlupiController& c, const Worlds::World& w, float stepDt)
        {
            const auto blockAt = c.GetBlockTypeAt(w);
            const auto blockAbove = c.GetBlockTypeAbove(w);
            const bool surf = BlockTypes::isWater(blockAt) && !BlockTypes::isWater(blockAbove);
            const bool nage = BlockTypes::isWater(blockAt) && BlockTypes::isWater(blockAbove);
            c.Step(w, 0.0f, 0.0f, false, false, false, stepDt, false, surf, nage);
        };

        GEBlupiController inShallow;
        inShallow.SetPosition(static_cast<float>(kShallowX) - 50.0f, 20.0f, static_cast<float>(kShallowZ) - 50.0f);
        for (int i = 0; i < 200; ++i) stepWithWaterDetection(inShallow, synthetic, dt);
        check(inShallow.IsOnGround() && std::fabs(inShallow.GetY() - 1.0f) < 0.01f,
              "Blupi sinks through a shallow 1-layer water pool and rests on the real floor beneath it");
        check(inShallow.IsSurf() && !inShallow.IsNage(),
              "standing in a shallow 1-layer pool is Surf (dry above), not Nage");
        check(inShallow.GetWaterGaugeLevel() == GEBlupiController::kWaterGaugeMax,
              "the breath gauge stays full while merely Surf, not Nage");

        GEBlupiController inDeep;
        inDeep.SetPosition(static_cast<float>(kDeepX) - 50.0f, 20.0f, static_cast<float>(kDeepZ) - 50.0f);
        for (int i = 0; i < 200; ++i) stepWithWaterDetection(inDeep, synthetic, dt);
        check(inDeep.IsOnGround() && std::fabs(inDeep.GetY() - 1.0f) < 0.01f,
              "Blupi sinks through a deep 2-layer water pool and rests on the real floor beneath it");
        check(inDeep.IsNage() && !inDeep.IsSurf(),
              "standing beneath 2 layers of water is Nage (water above too), not merely Surf");

        // Gauge countdown + drowning -- run enough simulated time to
        // exhaust the full 100-level gauge (the real ~25s), confirming
        // JustDrowned() fires exactly at the moment it reaches 0.
        GEBlupiController drowning;
        drowning.SetPosition(static_cast<float>(kDeepX) - 50.0f, 20.0f, static_cast<float>(kDeepZ) - 50.0f);
        constexpr float kDrownDt = 0.05f;
        bool drownedOnce = false;
        int drownedFrame = -1;
        for (int i = 0; i < 700 && !drownedOnce; ++i) // 700 * 0.05s = 35s, comfortably past ~25s + fall time
        {
            stepWithWaterDetection(drowning, synthetic, kDrownDt);
            if (drowning.JustDrowned())
            {
                drownedOnce = true;
                drownedFrame = i;
            }
        }
        std::cout << "Drowned at simulated t=" << (drownedFrame * kDrownDt) << "s" << std::endl;
        check(drownedOnce, "JustDrowned() fires after prolonged Nage submersion (the real ~25s breath gauge)");
        check(drowning.GetWaterGaugeLevel() == 0, "the gauge is exactly 0 at the moment JustDrowned() fires");

        // Resurfacing resets the gauge -- a few seconds of genuine Nage
        // (not to exhaustion), then Surf, confirms the gauge snaps back to
        // full instead of resuming from where it left off (matches the
        // real "gauge hidden on resurfacing" behavior, not a persisted
        // shared resource across dives).
        GEBlupiController resurfacer;
        resurfacer.SetPosition(static_cast<float>(kDeepX) - 50.0f, 1.0f, static_cast<float>(kDeepZ) - 50.0f);
        for (int i = 0; i < 60; ++i)
        {
            resurfacer.Step(synthetic, 0.0f, 0.0f, false, false, false, dt, false, false, true);
        }
        check(resurfacer.GetWaterGaugeLevel() < GEBlupiController::kWaterGaugeMax,
              "the gauge has ticked down after a few seconds of genuine Nage");
        resurfacer.Step(synthetic, 0.0f, 0.0f, false, false, false, dt, false, true, false);
        check(resurfacer.GetWaterGaugeLevel() == GEBlupiController::kWaterGaugeMax,
              "the gauge resets to full the instant Nage ends (Surf), matching the real 'gauge hidden' behavior");
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
