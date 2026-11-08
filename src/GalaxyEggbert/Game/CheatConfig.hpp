#pragma once

#ifndef GALAXY_EGGBERT_ENABLE_CHEATS
#define GALAXY_EGGBERT_ENABLE_CHEATS 1
#endif

namespace GalaxyEggbert::Game
{
    inline constexpr bool kCheatsEnabled = GALAXY_EGGBERT_ENABLE_CHEATS != 0;
}
