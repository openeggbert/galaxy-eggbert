#include "GalaxyEggbert/HintZoneRecord.hpp"

#include <algorithm>
#include <stdexcept>

namespace GalaxyEggbert {

void PlaceHintZone(Worlds::World& world, const HintZoneRecord& record) {
    if (record.sizeX == 0 || record.sizeY == 0 || record.sizeZ == 0) {
        throw std::invalid_argument("Hint zone sizes must be non-zero");
    }
    const int limit = world.blocksPerAxis();
    if (record.x + record.sizeX > limit || record.y + record.sizeY > limit || record.z + record.sizeZ > limit) {
        throw std::invalid_argument("Hint zone leaves the world");
    }
    const std::vector<std::uint8_t> payload = {record.recordIndex, record.sizeX, record.sizeY, record.sizeZ};
    world.setBlockExtraMetadata(record.x, record.y, record.z, kHintZoneMetadataType, payload);
}

std::vector<HintZoneRecord> CollectHintZones(const Worlds::World& world) {
    std::vector<HintZoneRecord> zones;
    for (const auto& resolved : world.collectExtraMetadata(kHintZoneMetadataType)) {
        if (resolved.payload.size() != 4) {
            throw std::runtime_error("Hint zone payload has an unexpected size");
        }
        HintZoneRecord zone;
        zone.recordIndex = resolved.payload[0];
        zone.x = resolved.x;
        zone.y = resolved.y;
        zone.z = resolved.z;
        zone.sizeX = resolved.payload[1];
        zone.sizeY = resolved.payload[2];
        zone.sizeZ = resolved.payload[3];
        if (zone.sizeX == 0 || zone.sizeY == 0 || zone.sizeZ == 0) {
            throw std::runtime_error("Hint zone payload has a zero size");
        }
        zones.push_back(zone);
    }
    std::stable_sort(zones.begin(), zones.end(),
                     [](const HintZoneRecord& a, const HintZoneRecord& b) { return a.recordIndex < b.recordIndex; });
    return zones;
}

}
