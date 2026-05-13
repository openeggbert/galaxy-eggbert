#pragma once

#include "GalaxyEggbert/Worlds/VoxelConfig.hpp"
#include "GalaxyEggbert/Worlds/BlockMetadata.hpp"

#include <cstdint>
#include <stdexcept>

/**
 * @file
 * @brief Definition of the compact voxel block value type.
 */

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
    /**
     * @brief Bit mask selecting the low 4 metadata bits.
     */
    static constexpr std::uint16_t MetadataMask = 0x000F;

    /**
     * @brief Bit mask selecting the 12-bit block type field.
     */
    static constexpr std::uint16_t TypeMask = 0x0FFF;

    /**
     * @brief Creates an air block (<tt>rawValue() == 0</tt>).
     */
    constexpr Block() noexcept = default;

    /**
     * @brief Creates a block directly from its packed 16-bit representation.
     *
     * No range validation is performed because the value is already packed.
     *
     * @param rawValue Packed block value.
     */
    explicit constexpr Block(std::uint16_t rawValue) noexcept
        : rawValue_(rawValue) {}

    /**
     * @brief Returns the canonical air block.
     *
     * @return Block with type and metadata both set to zero.
     */
    static constexpr Block air() noexcept {
        return Block(0);
    }

    /**
     * @brief Builds a validated block from type and metadata components.
     *
     * @param type Block type id in range <tt>[0, VoxelConfig::MaxBlockType]</tt>.
     * @param metadata Metadata nibble in range
     *        <tt>[0, VoxelConfig::MaxBlockMetadata]</tt>.
     * @return New packed block value.
     * @throws std::out_of_range If @p type or @p metadata is outside the supported range.
     */
    static Block make(std::uint16_t type, std::uint8_t metadata = 0) {
        if (type > VoxelConfig::MaxBlockType) {
            throw std::out_of_range("Block type must fit into 12 bits");
        }
        if (metadata > VoxelConfig::MaxBlockMetadata) {
            throw std::out_of_range("Block metadata must fit into 4 bits");
        }
        return Block(static_cast<std::uint16_t>((type << 4) | (metadata & MetadataMask)));
    }

    /**
 * @brief Builds a validated block from type and metadata components.
 *
 * @param type Block type id.
 * @param metadata Block metadata helper.
 * @return New packed block value.
 */
    static Block make(std::uint16_t type, BlockMetadata metadata) {
        return make(type, metadata.rawValue());
    }

    /**
     * @brief Returns the packed 16-bit block representation.
     */
    [[nodiscard]] constexpr std::uint16_t rawValue() const noexcept {
        return rawValue_;
    }

    /**
     * @brief Returns the 12-bit block type id.
     */
    [[nodiscard]] constexpr std::uint16_t type() const noexcept {
        return static_cast<std::uint16_t>((rawValue_ >> 4) & TypeMask);
    }

    /**
     * @brief Returns the 4-bit metadata value.
     */
    [[nodiscard]] constexpr std::uint8_t metadata() const noexcept {
        return static_cast<std::uint8_t>(rawValue_ & MetadataMask);
    }

    /**
 * @brief Returns the 4-bit metadata value as a helper object.
 */
    [[nodiscard]] BlockMetadata blockMetadata() const {
        return BlockMetadata(metadata());
    }

    /**
     * @brief Checks whether this block is air.
     *
     * @return <tt>true</tt> when <tt>rawValue() == 0</tt>, otherwise <tt>false</tt>.
     */
    [[nodiscard]] constexpr bool isAir() const noexcept {
        return rawValue_ == 0;
    }

    /**
     * @brief Compares two blocks by packed value.
     */
    friend constexpr bool operator==(Block lhs, Block rhs) noexcept {
        return lhs.rawValue_ == rhs.rawValue_;
    }

    /**
     * @brief Compares two blocks by packed value.
     */
    friend constexpr bool operator!=(Block lhs, Block rhs) noexcept {
        return !(lhs == rhs);
    }

private:
    std::uint16_t rawValue_ = 0;
};

} // namespace GalaxyEggbert::Worlds
