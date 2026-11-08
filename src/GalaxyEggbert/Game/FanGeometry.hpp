#pragma once

#include <GalaxyEggbert/BlockTypes.hpp>
#include <algorithm>
#include <cmath>

namespace GalaxyEggbert::Game
{
struct FanLocalPoint
{
    float x, y, z;
};
// Inverse of the fan model's roll and optional quarter turn around Y. Coordinates here
// are render coordinates relative to the cell centre (controller feet are 0.5 higher).
[[nodiscard]] inline FanLocalPoint FanLocal(std::uint16_t base, bool turned, float x, float y, float z)
{
    if (turned)
    {
        const float oldX = x;
        x = z;
        z = -oldX;
    }
    if (base == BlockTypes::FanLeft)
        return {y, -x, z};
    if (base == BlockTypes::FanRight)
        return {-y, x, z};
    if (base == BlockTypes::FanDown)
        return {-x, -y, z};
    return {x, y, z};
}

// Bounds recovered from avatars3d/objects/fan_ventillo-h, from Decor/ventillo-h.rd4:
// base (bone 1), motor (2), shaft (3), hub (4); the four blades (5-8) are separate.
[[nodiscard]] inline bool FanBodySolid(std::uint16_t base, bool turned, float x, float y, float z)
{
    const auto p = FanLocal(base, turned, x, y, z);
    if (p.y >= -.515f && p.y <= -.237f && std::fabs(p.x) <= .515f && std::fabs(p.z) <= .54f)
        return true;
    const float r2 = (p.x + .011f) * (p.x + .011f) + (p.z - .011f) * (p.z - .011f);
    return (p.y >= -.261f && p.y <= -.17f && r2 <= .23f * .23f) ||
           (p.y >= -.17f && p.y <= .287f && r2 <= .092f * .092f) ||
           (p.y >= .104f && p.y <= .47f && r2 <= .183f * .183f);
}

[[nodiscard]] inline bool FanRotorTouchesBody(std::uint16_t base, bool turned, float x, float feetY, float z,
                                              float halfWidth, float height)
{
    const auto p = FanLocal(base, turned, x, feetY + height * .5f, z);
    const bool side = base == BlockTypes::FanLeft || base == BlockTypes::FanRight;
    const float ex = side ? height * .5f : halfWidth, ey = side ? halfWidth : height * .5f, ez = halfWidth;
    // Swept blade annulus: y .225..401, radial .19..50, from the four moving blade bones.
    // Neither the motor nor the safe top of the hub is part of this volume.
    if (p.y + ey < .225f || p.y - ey > .401f)
        return false;
    const float ax = std::fabs(p.x + .011f), az = std::fabs(p.z - .011f);
    const float nearX = std::max(0.f, ax - ex), nearZ = std::max(0.f, az - ez);
    const float farX = ax + ex, farZ = az + ez;
    return nearX * nearX + nearZ * nearZ < .50f * .50f - 1e-5f && farX * farX + farZ * farZ >= .19f * .19f;
}
} // namespace GalaxyEggbert::Game
