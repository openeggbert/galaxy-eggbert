#include "GalaxyEggbert/MoveObjectRecord.hpp"

#include <cmath>
#include <cstring>
#include <stdexcept>

namespace GalaxyEggbert {

namespace {

void AppendU8(std::vector<std::uint8_t>& out, std::uint8_t value) {
    out.push_back(value);
}

void AppendU16LE(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xFF));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
}

void AppendFloatLE(std::vector<std::uint8_t>& out, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    out.push_back(static_cast<std::uint8_t>(bits & 0xFF));
    out.push_back(static_cast<std::uint8_t>((bits >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((bits >> 16) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((bits >> 24) & 0xFF));
}

float ReadFloatLE(const std::vector<std::uint8_t>& payload, std::size_t offset) {
    std::uint32_t bits = static_cast<std::uint32_t>(payload[offset])
        | (static_cast<std::uint32_t>(payload[offset + 1]) << 8)
        | (static_cast<std::uint32_t>(payload[offset + 2]) << 16)
        | (static_cast<std::uint32_t>(payload[offset + 3]) << 24);
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

// Legacy payload:
// [objectType: 1 byte][posStartX,Y,Z: 3x float32][posEndX,Y,Z: 3x float32]
// [speed: float32][stepAdvanceTicks,stepRecedeTicks,timeStopStartTicks,
// timeStopEndTicks: 4x float32] = 45 bytes total, fixed size (no
// variable-length fields).
//
// Current payload appends [visualIcon: uint16] for exact per-instance
// object-m.png variants (EDITOR-124). Decode deliberately accepts the
// legacy 45-byte payload so every existing custom world remains valid.
constexpr std::size_t kLegacyPayloadSize = 1 + 3 * 4 + 3 * 4 + 4 + 4 * 4;
constexpr std::size_t kPayloadSize = kLegacyPayloadSize + 2;

std::vector<std::uint8_t> EncodeMoveObjectRecord(const MoveObjectRecord& record) {
    std::vector<std::uint8_t> payload;
    payload.reserve(kPayloadSize);
    AppendU8(payload, static_cast<std::uint8_t>(record.type));
    AppendFloatLE(payload, record.posStartX);
    AppendFloatLE(payload, record.posStartY);
    AppendFloatLE(payload, record.posStartZ);
    AppendFloatLE(payload, record.posEndX);
    AppendFloatLE(payload, record.posEndY);
    AppendFloatLE(payload, record.posEndZ);
    AppendFloatLE(payload, record.speed);
    AppendFloatLE(payload, record.stepAdvanceTicks);
    AppendFloatLE(payload, record.stepRecedeTicks);
    AppendFloatLE(payload, record.timeStopStartTicks);
    AppendFloatLE(payload, record.timeStopEndTicks);
    AppendU16LE(payload, record.visualIcon);
    return payload;
}

MoveObjectRecord DecodeMoveObjectRecord(const std::vector<std::uint8_t>& payload) {
    if (payload.size() != kLegacyPayloadSize && payload.size() != kPayloadSize) {
        throw std::runtime_error("MoveObjectRecord payload has an unexpected size");
    }

    MoveObjectRecord record;
    record.type = static_cast<ObjectType>(payload[0]);
    record.posStartX = ReadFloatLE(payload, 1);
    record.posStartY = ReadFloatLE(payload, 5);
    record.posStartZ = ReadFloatLE(payload, 9);
    record.posEndX = ReadFloatLE(payload, 13);
    record.posEndY = ReadFloatLE(payload, 17);
    record.posEndZ = ReadFloatLE(payload, 21);
    record.speed = ReadFloatLE(payload, 25);
    record.stepAdvanceTicks = ReadFloatLE(payload, 29);
    record.stepRecedeTicks = ReadFloatLE(payload, 33);
    record.timeStopStartTicks = ReadFloatLE(payload, 37);
    record.timeStopEndTicks = ReadFloatLE(payload, 41);
    if (payload.size() == kPayloadSize) {
        record.visualIcon = static_cast<std::uint16_t>(payload[45]) |
            (static_cast<std::uint16_t>(payload[46]) << 8);
    }
    return record;
}

}

void PlaceMoveObject(Worlds::World& world, const MoveObjectRecord& record) {
    const auto anchorX = static_cast<std::uint16_t>(std::floor(record.posStartX));
    const auto anchorY = static_cast<std::uint16_t>(std::floor(record.posStartY));
    const auto anchorZ = static_cast<std::uint16_t>(std::floor(record.posStartZ));
    world.setBlockExtraMetadata(anchorX, anchorY, anchorZ, kMoveObjectMetadataType, EncodeMoveObjectRecord(record));
}

std::vector<MoveObjectRecord> CollectMoveObjects(const Worlds::World& world) {
    std::vector<MoveObjectRecord> records;
    for (const auto& resolved : world.collectExtraMetadata(kMoveObjectMetadataType)) {
        records.push_back(DecodeMoveObjectRecord(resolved.payload));
    }
    return records;
}

bool RemoveMoveObject(Worlds::World& world, std::uint16_t x, std::uint16_t y, std::uint16_t z) {
    return world.removeBlockExtraMetadata(x, y, z, kMoveObjectMetadataType);
}

}
