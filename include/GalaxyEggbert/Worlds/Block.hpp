#pragma once

#include "GalaxyEggbert/Worlds/VoxelConfig.hpp"

#include <cstdint>
#include <stdexcept>

namespace GalaxyEggbert::Worlds {

/**
 * @brief One logical voxel block encoded into 16 bits.
 *
 * Layout:
 * - high 12 bits: block type id, range 0..4095
 * - low  4 bits: block metadata, range 0..15
 *
 * Metadata may represent rotation, variant, state, or a small per-block value.
 * Type id 0 with metadata 0 is reserved as Air by convention.
 */
class Block final {
public:
    static constexpr std::uint16_t MetadataMask = 0x000F;
    static constexpr std::uint16_t TypeMask = 0x0FFF;

    constexpr Block() noexcept = default;

    explicit constexpr Block(std::uint16_t rawValue) noexcept
        : rawValue_(rawValue) {}

    static constexpr Block air() noexcept {
        return Block(0);
    }

    static Block make(std::uint16_t type, std::uint8_t metadata = 0) {
        if (type > VoxelConfig::MaxBlockType) {
            throw std::out_of_range("Block type must fit into 12 bits");
        }
        if (metadata > VoxelConfig::MaxBlockMetadata) {
            throw std::out_of_range("Block metadata must fit into 4 bits");
        }
        return Block(static_cast<std::uint16_t>((type << 4) | (metadata & MetadataMask)));
    }

    [[nodiscard]] constexpr std::uint16_t rawValue() const noexcept {
        return rawValue_;
    }

    [[nodiscard]] constexpr std::uint16_t type() const noexcept {
        return static_cast<std::uint16_t>((rawValue_ >> 4) & TypeMask);
    }

    [[nodiscard]] constexpr std::uint8_t metadata() const noexcept {
        return static_cast<std::uint8_t>(rawValue_ & MetadataMask);
    }

    [[nodiscard]] constexpr bool isAir() const noexcept {
        return rawValue_ == 0;
    }

    friend constexpr bool operator==(Block lhs, Block rhs) noexcept {
        return lhs.rawValue_ == rhs.rawValue_;
    }

    friend constexpr bool operator!=(Block lhs, Block rhs) noexcept {
        return !(lhs == rhs);
    }

private:
    std::uint16_t rawValue_ = 0;
};

} // namespace GalaxyEggbert::Worlds
