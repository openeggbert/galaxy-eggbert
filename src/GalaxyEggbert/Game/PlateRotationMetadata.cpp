#include "PlateRotationMetadata.hpp"

namespace GalaxyEggbert::Game
{
    void SetPlateRotated(Worlds::World& world, std::uint16_t x, std::uint16_t y, std::uint16_t z, bool rotated)
    {
        world.setBlockExtraMetadata(x, y, z, kPlateRotationMetadataType,
                                     {static_cast<std::uint8_t>(rotated ? 1 : 0)});
    }

    bool IsPlateRotated(const Worlds::World& world, std::uint16_t x, std::uint16_t y, std::uint16_t z)
    {
        const auto* payload = world.findBlockExtraMetadata(x, y, z, kPlateRotationMetadataType);
        return payload != nullptr && !payload->empty() && ((*payload)[0] & 1) != 0;
    }

    int QuarterTurns(const Worlds::World& world, std::uint16_t x, std::uint16_t y, std::uint16_t z)
    {
        const auto* payload = world.findBlockExtraMetadata(x, y, z, kPlateRotationMetadataType);
        return payload != nullptr && !payload->empty() ? ((*payload)[0] & 3) : 0;
    }

    void SetQuarterTurns(Worlds::World& world, std::uint16_t x, std::uint16_t y, std::uint16_t z, int turns)
    {
        world.setBlockExtraMetadata(x, y, z, kPlateRotationMetadataType,
                                     {static_cast<std::uint8_t>(((turns % 4) + 4) % 4)});
    }

    std::vector<std::array<std::uint16_t, 3>> CollectRotatedPlatePositions(const Worlds::World& world)
    {
        std::vector<std::array<std::uint16_t, 3>> result;
        for (const auto& record : world.collectExtraMetadata(kPlateRotationMetadataType))
        {
            if (!record.payload.empty() && (record.payload[0] & 1) != 0)
            {
                result.push_back({record.x, record.y, record.z});
            }
        }
        return result;
    }
}
