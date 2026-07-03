#include "Game/GEBlupiController.hpp"

#include <GalaxyEggbert/Worlds/World.hpp>

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
    blupi.Step(world, 0.0f, 0.0f, false, dt);
    check(blupi.IsOnGround(), "spawns grounded on the ground floor");

    // 2. Walk west (-X) toward and up the 10-step staircase (world x from
    // -21 down to -30, world z band [-5,4] -- spawn z=0 is inside it).
    // Expect Y to climb via step-up traversal.
    for (int i = 0; i < 400; ++i) // ~6.7 simulated seconds
    {
        blupi.Step(world, -1.0f, 0.0f, false, dt);
    }
    std::cout << "After walking west: x=" << blupi.GetX() << " y=" << blupi.GetY()
              << " z=" << blupi.GetZ() << " onGround=" << blupi.IsOnGround() << std::endl;
    check(blupi.GetY() >= 8.0f, "climbed the staircase via step-up traversal (Y >= 8)");
    check(blupi.IsOnGround(), "stands on top of the staircase/platform, not falling through it");

    // 3. Keep walking west into the room's far wall (world x=-45) -- should
    // block, not clip through.
    for (int i = 0; i < 300; ++i)
    {
        blupi.Step(world, -1.0f, 0.0f, false, dt);
    }
    std::cout << "After walking into wall: x=" << blupi.GetX() << std::endl;
    check(blupi.GetX() > -45.0f, "wall blocks horizontal movement (did not clip through x=-45 wall)");

    // 4. Drop from height over an empty column (world x=40 -- outside any
    // placed structure) and confirm gravity + landing works.
    GEBlupiController faller;
    faller.SetPosition(40.0f, 20.0f, 40.0f);
    for (int i = 0; i < 200 && !faller.IsOnGround(); ++i)
    {
        faller.Step(world, 0.0f, 0.0f, false, dt);
    }
    std::cout << "Faller landed at y=" << faller.GetY() << std::endl;
    check(faller.IsOnGround(), "falls under gravity and lands");
    check(faller.GetY() < 5.0f, "lands far below the y=20 drop height (real gravity, not a snap)");

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
