#include "GalaxyEggbert/Worlds/test_worlds.hpp"

#include "GalaxyEggbert/Worlds/World.hpp"
#include <iostream>
using namespace GalaxyEggbert::Worlds;
namespace GalaxyEggbert::Worlds
{
    int test_worlds() {
        World world;

        const Block grass = Block::make(1, 0);
        const Block dirt = Block::make(2, 0);
        const Block stone = Block::make(3, 0);

        // Create a tiny terrain patch.
        for (std::uint16_t x = 0; x < 20; ++x) {
            for (std::uint16_t z = 0; z < 20; ++z) {
                world.setBlock(x, 0, z, stone);
                world.setBlock(x, 1, z, dirt);
                world.setBlock(x, 2, z, grass);
            }
        }

        world.saveToFile("example_world.vwr");
        World loaded = World::loadFromFile("example_world.vwr");

        if (loaded.getBlock(10, 2, 10) != grass) {
            std::cerr << "Loaded world verification failed\n";
            return 1;
        }

        std::cout << "example_world.vwr written successfully\n";
        return 0;
    }
}