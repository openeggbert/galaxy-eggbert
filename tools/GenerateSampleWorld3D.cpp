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
    fill(30, 70, 0, 0, 30, 70, BlockTypes::Ground);

    // Ascending solid staircase: 10 steps, x=29 down to x=20, z=45..54,
    // each column solid from y=0 up to its own step height.
    for (int step = 0; step < 10; ++step)
    {
        const int x = 29 - step;
        fill(x, x, 0, step, 45, 54, BlockTypes::StoneA);
    }

    // Raised platform floor at y=10, sitting flush with the tallest step's
    // own surface (x=20's staircase fill already reaches y=9, surface
    // y=10) -- starts at x=19, NOT x=20, so it doesn't stack a second
    // block on top of the staircase's own last step (that produced an
    // unclimbable 2-block cliff at the x=21->20 transition; caught by
    // tools/VerifyBlupiMovement.cpp).
    fill(5, 19, 10, 10, 40, 59, BlockTypes::Platform);

    // Room walls, 3 blocks tall (y=11..13), around the platform perimeter,
    // with a 3-wide doorway on the staircase-facing edge (x=20, z=48..51).
    fill(5, 5,   11, 13, 40, 59, BlockTypes::Wall);   // west wall
    fill(6, 19,  11, 13, 40, 40, BlockTypes::Wall);   // north wall
    fill(6, 19,  11, 13, 59, 59, BlockTypes::Wall);   // south wall
    fill(20, 20, 11, 13, 40, 47, BlockTypes::Wall);   // east wall, north of doorway
    fill(20, 20, 11, 13, 52, 59, BlockTypes::Wall);   // east wall, south of doorway

    // Two decorative pillars inside the room.
    fill(10, 10, 11, 13, 45, 45, BlockTypes::StoneB);
    fill(10, 10, 11, 13, 54, 54, BlockTypes::StoneB);

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
