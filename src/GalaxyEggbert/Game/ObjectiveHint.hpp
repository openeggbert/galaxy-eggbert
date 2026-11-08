#pragma once

namespace GalaxyEggbert::Game
{
struct ObjectiveHint
{
    float distance = 0;
    float dx = 0, dy = 0, dz = 0; // target minus player, world axes
};
} // namespace GalaxyEggbert::Game
