#pragma once

#include <cmath>
#include <numbers>

namespace GalaxyEggbert::Game
{
struct ObjectiveHint
{
    float distance = 0;
    float dx = 0, dy = 0, dz = 0; // target minus player, world axes
};

struct CompassArrows
{
    char turn = '|';
    char height = '-';
};

// Blupi faces (sin(yaw), -cos(yaw)); increasing yaw turns him to his right.
// A small dead zone shows stable alignment bars instead of flickering arrows.
[[nodiscard]] inline CompassArrows ObjectiveCompassArrows(const ObjectiveHint& target, float yaw) noexcept
{
    CompassArrows arrows;
    if (target.dy > .1f) arrows.height = '^';
    else if (target.dy < -.1f) arrows.height = 'v';
    if (std::hypot(target.dx, target.dz) > .1f && std::isfinite(yaw))
    {
        constexpr float pi = std::numbers::pi_v<float>;
        const float turn = std::remainder(std::atan2(target.dx, -target.dz) - yaw, 2*pi);
        if (std::fabs(turn) > pi/60) // three degrees
            arrows.turn = std::fabs(std::fabs(turn)-pi) < .00001f || turn > 0 ? '>' : '<';
    }
    return arrows;
}
} // namespace GalaxyEggbert::Game
