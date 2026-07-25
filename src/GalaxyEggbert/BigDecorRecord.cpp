#include "GalaxyEggbert/BigDecorRecord.hpp"

#include <stdexcept>

namespace GalaxyEggbert {

void PlaceBigDecor(Worlds::World& world, const BigDecorRecord& record) {
    if (record.icon == 0) {
        throw std::invalid_argument("BigDecor icon must be non-zero");
    }
    const std::vector<std::uint8_t> payload = {
        static_cast<std::uint8_t>(record.icon & 0xFF),
        static_cast<std::uint8_t>((record.icon >> 8) & 0xFF),
    };
    world.setBlockExtraMetadata(
        record.x, record.y, record.z, kBigDecorMetadataType, payload);
}

std::vector<BigDecorRecord> CollectBigDecor(const Worlds::World& world) {
    std::vector<BigDecorRecord> records;
    for (const auto& resolved : world.collectExtraMetadata(kBigDecorMetadataType)) {
        if (resolved.payload.size() != 2) {
            throw std::runtime_error("BigDecor payload has an unexpected size");
        }
        const std::uint16_t icon =
            static_cast<std::uint16_t>(resolved.payload[0]) |
            (static_cast<std::uint16_t>(resolved.payload[1]) << 8);
        if (icon == 0) {
            throw std::runtime_error("BigDecor payload contains the Air icon");
        }
        records.push_back({icon, resolved.x, resolved.y, resolved.z});
    }
    return records;
}

bool RemoveBigDecor(
    Worlds::World& world, std::uint16_t x, std::uint16_t y, std::uint16_t z) {
    return world.removeBlockExtraMetadata(x, y, z, kBigDecorMetadataType);
}

}
