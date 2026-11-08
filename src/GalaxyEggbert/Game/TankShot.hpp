#pragma once

#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/GridRaycast.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>
#include <algorithm>

namespace GalaxyEggbert::Game
{
struct TankShotAim
{
    float dx = 0, dz = -1, distance = 0;
};

// Raw grid origin, continuous tank yaw. Keep SearchAirDistance's blocker policy
// and half-cell stand-off, using the editor's shared DDA for arbitrary headings.
[[nodiscard]] inline TankShotAim AimTankShot(const Worlds::World& world, float x, float y, float z, float yaw)
{
    TankShotAim aim{std::sin(yaw), -std::cos(yaw), 0};
    float range=std::numeric_limits<float>::infinity();
    const float edge=float(world.blocksPerAxis())-.5f;
    const auto limit=[&](float origin,float direction) {
        if (direction>1e-6f) range=std::min(range,(edge-origin)/direction);
        else if (direction< -1e-6f) range=std::min(range,(-.5f-origin)/direction);
    };
    limit(x,aim.dx); limit(z,aim.dz);
    if (!(range>0) || !std::isfinite(range)) return aim;
    const auto hit=Worlds::RaycastGrid(world.blocksPerAxis(),x,y,z,aim.dx,0,aim.dz,range,
        [&world](int gx,int gy,int gz) {
            const auto type=world.getBlock(static_cast<std::uint16_t>(gx),static_cast<std::uint16_t>(gy),
                                           static_cast<std::uint16_t>(gz)).type();
            return GetBlockDefinition(type).collisionMask==0xffff || type==BlockTypes::Temp;
        });
    aim.distance=std::max(0.f,(hit.hit ? hit.distance : range)-.5f);
    return aim;
}
} // namespace GalaxyEggbert::Game
