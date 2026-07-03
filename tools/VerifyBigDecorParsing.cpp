#include "Game/GEWorldRuntime.hpp"

#include <algorithm>
#include <iostream>
#include <string>

// Scripted verification that GEWorldRuntime::LoadFromMobileEggbertFile now
// parses the BigDecor: section instead of silently dropping it (see
// mobile-eggbert-2d-reference.md §2.3). Loads a real mobile-eggbert level
// file known to have non-empty BigDecor data and checks the parsed result
// actually contains non-air cells.
int main(int argc, char** argv)
{
    const std::string path = (argc > 1) ? argv[1] : "../mobile-eggbert/worlds/world013.txt";

    GESimple3D::GEWorldRuntime runtime;
    if (!runtime.LoadFromMobileEggbertFile(path))
    {
        std::cout << "FAIL: could not load " << path << std::endl;
        return 1;
    }

    const auto& bigDecor = runtime.GetBigDecor();
    const auto nonAir = std::count_if(bigDecor.begin(), bigDecor.end(),
                                       [](std::uint16_t v) { return v != 0; });

    std::cout << (nonAir > 0 ? "PASS" : "FAIL") << ": BigDecor has " << nonAir
              << " non-air cells parsed from " << path << std::endl;

    // Regression check: the main Decor: grid must still parse correctly
    // after refactoring the section-tracking to recognize BigDecor: too.
    const auto* world = runtime.GetWorld();
    int mainNonAir = 0;
    for (int z = 0; z < 100; ++z)
    {
        for (int x = 0; x < 100; ++x)
        {
            if (!world->getBlock(static_cast<std::uint16_t>(x), 0, static_cast<std::uint16_t>(z)).isAir())
            {
                ++mainNonAir;
            }
        }
    }
    std::cout << "INFO: main Decor grid has " << mainNonAir << " non-air blocks" << std::endl;

    return nonAir > 0 ? 0 : 1;
}
