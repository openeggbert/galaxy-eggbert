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

    // 4. Drop from height over an empty column (world x=40 -- outside any
    // placed structure) and confirm gravity + landing works.
    GEBlupiController faller;
    faller.SetPosition(40.0f, 20.0f, 40.0f);
    for (int i = 0; i < 200 && !faller.IsOnGround(); ++i)
    {
        faller.Step(world, 0.0f, 0.0f, false, false, false, dt);
    }
    std::cout << "Faller landed at y=" << faller.GetY() << std::endl;
    check(faller.IsOnGround(), "falls under gravity and lands");
    check(faller.GetY() < 5.0f, "lands far below the y=20 drop height (real gravity, not a snap)");

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

        // Reduced move speed while squashed: same moveInput/dt, less
        // distance covered than an un-squashed Blupi over one Step(). At
        // yaw=0 (the default), forward movement changes Z, not X.
        const float zBeforeCrushedMove = crushed.GetZ();
        crushed.Step(synthetic, 0.0f, 1.0f, false, false, false, dt);
        const float crushedDelta = std::fabs(crushed.GetZ() - zBeforeCrushedMove);

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
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
