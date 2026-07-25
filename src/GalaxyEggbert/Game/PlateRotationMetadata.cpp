#include "PlateRotationMetadata.hpp"

namespace GalaxyEggbert::Game
{
    void SetPlateRotated(Worlds::World& world, std::uint16_t x, std::uint16_t y, std::uint16_t z, bool rotated)
    {
        world.setBlockExtraMetadata(x, y, z, kPlateRotationMetadataType,
                                     {static_cast<std::uint8_t>(rotated ? 1 : 0)});
    }

    std::vector<std::array<std::uint16_t, 3>> CollectRotatedPlatePositions(const Worlds::World& world)
    {
        std::vector<std::array<std::uint16_t, 3>> result;
        for (const auto& record : world.collectExtraMetadata(kPlateRotationMetadataType))
        {
            if (!record.payload.empty() && record.payload[0] != 0)
            {
                result.push_back({record.x, record.y, record.z});
            }
        }
        return result;
    }
}
