#pragma once

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Game/PlateRotationMetadata.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <algorithm>
#include <array>
#include <vector>
#include <cstdlib>

namespace GalaxyEggbert::Game
{
    // Only old files need this compatibility mapping. New saw cells never create terrain.
    // A migrated switch retains its original row link after its saws move one cell up.
    inline constexpr std::uint16_t kLegacySwitchSawRowMetadataType = 6;

    inline void UpgradeLegacySawFloors(Worlds::World& world)
    {
        if (!world.hasLegacySawFloors()) return;
        struct Saw { int x,y,z; std::uint16_t icon,floor; bool rotated; };
        std::vector<Saw> saws;
        std::vector<std::array<int,3>> switches;
        const int n=world.blocksPerAxis();
        for (int y=0;y<n;++y) for (int z=0;z<n;++z) for (int x=0;x<n;++x)
        {
            const auto icon=world.getBlock(x,y,z).type();
            if (icon==BlockTypes::Switch || icon==BlockTypes::SwitchOff) switches.push_back({x,y,z});
            if (icon<BlockTypes::Saw || icon>383 || y+1>=n || !world.getBlock(x,y+1,z).isAir()) continue;
            // Materialize the floor that the old renderer used to invent. Evaluate every
            // choice before changing anything, so adjacent saws keep the same material.
            std::uint16_t floor=BlockTypes::RockPile;
            for (int dy : {0,-1})
            {
                std::array<std::pair<std::uint16_t,int>,8> seen{};
                for (int dz=-3;dz<=3;++dz) for (int dx=-3;dx<=3;++dx)
                {
                    const int nx=x+dx,ny=y+dy,nz=z+dz;
                    if ((dx==0 && dy==0 && dz==0) || nx<0 || ny<0 || nz<0 || nx>=n || nz>=n) continue;
                    const auto other=world.getBlock(nx,ny,nz).type();
                    const auto& d=GetBlockDefinition(other);
                    if (!other || d.collisionMask!=0xFFFF || d.animation.IsAnimated() || d.hazard!=BlockHazardKind::None) continue;
                    for (auto& entry : seen)
                        if (!entry.second || entry.first==other) { entry.first=other; ++entry.second; break; }
                }
                int best=0;
                for (const auto& entry : seen) if (entry.second>best) { best=entry.second; floor=entry.first; }
                if (best) break;
            }
            saws.push_back({x,y,z,icon,floor,IsPlateRotated(world,x,y,z)});
        }
        for (const auto& s : saws)
        {
            world.setBlock(s.x,s.y,s.z,Worlds::Block::make(s.floor));
            world.setBlock(s.x,s.y+1,s.z,Worlds::Block::make(s.icon));
            world.removeBlockExtraMetadata(s.x,s.y,s.z,kPlateRotationMetadataType);
            SetPlateRotated(world,s.x,s.y+1,s.z,s.rotated);
        }
        for (const auto& [x,y,z] : switches)
        {
            const bool linked=std::any_of(saws.begin(),saws.end(),[&](const Saw& s) {
                return s.y==y && s.z==z && std::abs(s.x-x)<=20;
            });
            if (linked) world.setBlockExtraMetadata(x,y,z,kLegacySwitchSawRowMetadataType,{1});
        }
        // Occupied cells above a legacy saw are never overwritten; that saw simply becomes
        // independent in its existing cell. No data or models belonging to another cell are lost.
        world.setLegacySawFloors(false);
    }
}
