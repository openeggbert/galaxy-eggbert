#pragma once

#include <GalaxyEggbert/Worlds/World.hpp>
#include <array>
#include <optional>
#include <stdexcept>
#include <vector>

namespace GalaxyEggbert
{
    // Eggbert 2 PlaceDecor10 / decblock BUILD: four separately coloured player starts.
    // Galaxy's owner requests independent editor starts; single-player still uses yellow.
    enum class PlayerStartColor : std::uint8_t { Yellow, Orange, Blue, Green };
    inline constexpr std::array<const char*,4> PlayerStartNames{"Yellow", "Orange", "Blue", "Green"};

    struct PlayerStart
    {
        PlayerStartColor color = PlayerStartColor::Yellow;
        std::uint16_t x=0, y=0, z=0;
        std::uint8_t quarterTurns=0;
        bool operator==(const PlayerStart&) const = default;
    };

    // Yellow retains the existing header fields. Other colours share sparse metadata7:
    // one byte, bits1/2/3 = orange/blue/green. Several starts may occupy the same cell.
    inline constexpr std::uint16_t kPlayerStartMetadataType = 7;

    // Each colour has independent two-bit heading data at its start cell (0 = default -Z).
    inline constexpr std::uint16_t kPlayerStartHeadingMetadataType = 8;

    [[nodiscard]] inline std::uint8_t PlayerStartHeading(const Worlds::World& world, PlayerStartColor color,
                                                       std::uint16_t x, std::uint16_t y, std::uint16_t z)
    {
        const auto* bytes=world.findBlockExtraMetadata(x,y,z,kPlayerStartHeadingMetadataType);
        if (!bytes) return 0;
        if (bytes->size()!=1) throw std::runtime_error("Invalid player start heading metadata");
        return ((*bytes)[0]>>(2*static_cast<int>(color)))&3;
    }

    inline void SetPlayerStartHeading(Worlds::World& world, PlayerStartColor color,
                                     std::uint16_t x, std::uint16_t y, std::uint16_t z, std::uint8_t turns)
    {
        const auto* bytes=world.findBlockExtraMetadata(x,y,z,kPlayerStartHeadingMetadataType);
        if (bytes && bytes->size()!=1) throw std::runtime_error("Invalid player start heading metadata");
        const int shift=2*static_cast<int>(color);
        const auto packed=static_cast<std::uint8_t>(((bytes ? (*bytes)[0] : 0)&~(3<<shift))|(turns<<shift));
        if (packed) world.setBlockExtraMetadata(x,y,z,kPlayerStartHeadingMetadataType,{packed});
        else world.removeBlockExtraMetadata(x,y,z,kPlayerStartHeadingMetadataType);
    }

    // Match the editor's positive model quarter turns to the controller's yaw convention.
    [[nodiscard]] inline float PlayerStartYaw(const PlayerStart& start) noexcept
    {
        return -float(start.quarterTurns)*1.57079632679f;
    }

    [[nodiscard]] inline std::vector<PlayerStart> CollectPlayerStarts(const Worlds::World& world)
    {
        std::vector<PlayerStart> starts;
        if (world.hasSpawnPoint())
            starts.push_back({PlayerStartColor::Yellow,world.spawnX(),world.spawnY(),world.spawnZ(),
                PlayerStartHeading(world,PlayerStartColor::Yellow,world.spawnX(),world.spawnY(),world.spawnZ())});
        std::array<bool,4> seen{};
        for (const auto& r : world.collectExtraMetadata(kPlayerStartMetadataType))
        {
            if (r.payload.size()!=1 || r.payload[0]==0 || (r.payload[0]&~0x0e)!=0)
                throw std::runtime_error("Invalid coloured player start metadata");
            for (int c=1;c<4;++c)
                if (r.payload[0]&(1<<c))
                {
                    if (seen[c]) throw std::runtime_error("Duplicate coloured player start");
                    seen[c]=true;
                    starts.push_back({static_cast<PlayerStartColor>(c),r.x,r.y,r.z,
                        PlayerStartHeading(world,static_cast<PlayerStartColor>(c),r.x,r.y,r.z)});
                }
        }
        return starts;
    }

    [[nodiscard]] inline std::optional<PlayerStart> FindPlayerStart(const Worlds::World& world, PlayerStartColor color)
    {
        for (const auto& start : CollectPlayerStarts(world))
            if (start.color==color) return start;
        return std::nullopt;
    }

    inline void ClearPlayerStart(Worlds::World& world, PlayerStartColor color)
    {
        const int c=static_cast<int>(color);
        if (c<0 || c>3) throw std::invalid_argument("Invalid player start colour");
        const auto old=FindPlayerStart(world,color);
        if (!old) return;
        SetPlayerStartHeading(world,color,old->x,old->y,old->z,0);
        if (c==0) { world.clearSpawnPoint(); return; }
        const auto* bytes=world.findBlockExtraMetadata(old->x,old->y,old->z,kPlayerStartMetadataType);
        const auto mask=static_cast<std::uint8_t>((*bytes)[0]&~(1<<c));
        if (mask) world.setBlockExtraMetadata(old->x,old->y,old->z,kPlayerStartMetadataType,{mask});
        else world.removeBlockExtraMetadata(old->x,old->y,old->z,kPlayerStartMetadataType);
    }

    inline void SetPlayerStart(Worlds::World& world, const PlayerStart& start)
    {
        // Validate before removing the previous start, keeping failed edits non-destructive.
        (void)world.getBlock(start.x,start.y,start.z);
        const int c=static_cast<int>(start.color);
        if (c<0 || c>3) throw std::invalid_argument("Invalid player start colour");
        if (start.quarterTurns>3) throw std::invalid_argument("Invalid player start heading");
        (void)PlayerStartHeading(world,start.color,start.x,start.y,start.z); // validate before mutation
        ClearPlayerStart(world,start.color);
        if (c==0) world.setSpawnPoint(start.x,start.y,start.z);
        else
        {
            const auto* bytes=world.findBlockExtraMetadata(start.x,start.y,start.z,kPlayerStartMetadataType);
            const auto mask=static_cast<std::uint8_t>((bytes ? (*bytes)[0] : 0)|(1<<c));
            world.setBlockExtraMetadata(start.x,start.y,start.z,kPlayerStartMetadataType,{mask});
        }
        SetPlayerStartHeading(world,start.color,start.x,start.y,start.z,start.quarterTurns);
    }
}
