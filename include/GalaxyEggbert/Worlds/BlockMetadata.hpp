#pragma once

#include "GalaxyEggbert/Worlds/VoxelConfig.hpp"

#include <cstdint>
#include <stdexcept>

/**
 * @file
 * @brief Definition of the compact 4-bit voxel block metadata helper.
 */

namespace GalaxyEggbert::Worlds {

/**
 * @brief Helper class for working with the 4-bit metadata part of a voxel block.
 *
 * Metadata is stored in the low 4 bits of a byte:
 *
 * - bit 0: custom meaning
 * - bit 1: custom meaning
 * - bit 2: custom meaning
 * - bit 3: custom meaning
 *
 * The upper 4 bits are always ignored when returning the packed value.
 *
 * This class does not define the semantic meaning of the bits. The game may use
 * them for rotation, variant, active/inactive state, or other small block-local state.
 */
class BlockMetadata final {
public:
    /**
     * @brief Bit mask selecting the valid 4 metadata bits.
     */
    static constexpr std::uint8_t MetadataMask = 0x0F;

    /**
     * @brief Bit mask for metadata bit 0.
     */
    static constexpr std::uint8_t Bit0Mask = 0x01;

    /**
     * @brief Bit mask for metadata bit 1.
     */
    static constexpr std::uint8_t Bit1Mask = 0x02;

    /**
     * @brief Bit mask for metadata bit 2.
     */
    static constexpr std::uint8_t Bit2Mask = 0x04;

    /**
     * @brief Bit mask for metadata bit 3.
     */
    static constexpr std::uint8_t Bit3Mask = 0x08;

    /**
     * @brief Creates empty metadata with all bits cleared.
     */
    constexpr BlockMetadata() noexcept = default;

    /**
     * @brief Creates metadata from a raw 4-bit value.
     *
     * @param value Metadata value in range <tt>[0, VoxelConfig::MaxBlockMetadata]</tt>.
     * @throws std::out_of_range If @p value does not fit into 4 bits.
     */
    explicit BlockMetadata(std::uint8_t value)
        : value_(value) {
        if (value > VoxelConfig::MaxBlockMetadata) {
            throw std::out_of_range("Block metadata must fit into 4 bits");
        }
    }

    /**
     * @brief Creates metadata from a raw value by keeping only the low 4 bits.
     *
     * @param value Input value.
     * @return Metadata containing only <tt>value & 0x0F</tt>.
     */
    [[nodiscard]] static constexpr BlockMetadata fromMasked(std::uint8_t value) noexcept {
        return BlockMetadata(value & MetadataMask, MaskedTag{});
    }

    /**
     * @brief Returns the packed 4-bit metadata value as uint8_t.
     *
     * The returned value is always in range <tt>0..15</tt>.
     */
    [[nodiscard]] constexpr std::uint8_t rawValue() const noexcept {
        return static_cast<std::uint8_t>(value_ & MetadataMask);
    }

    /**
     * @brief Returns the packed 4-bit metadata value as uint8_t.
     *
     * Alias for rawValue().
     */
    [[nodiscard]] constexpr std::uint8_t toUInt8() const noexcept {
        return rawValue();
    }

    /**
     * @brief Returns true if bit 0 is set.
     */
    [[nodiscard]] constexpr bool bit0() const noexcept {
        return (value_ & Bit0Mask) != 0;
    }

    /**
     * @brief Returns true if bit 1 is set.
     */
    [[nodiscard]] constexpr bool bit1() const noexcept {
        return (value_ & Bit1Mask) != 0;
    }

    /**
     * @brief Returns true if bit 2 is set.
     */
    [[nodiscard]] constexpr bool bit2() const noexcept {
        return (value_ & Bit2Mask) != 0;
    }

    /**
     * @brief Returns true if bit 3 is set.
     */
    [[nodiscard]] constexpr bool bit3() const noexcept {
        return (value_ & Bit3Mask) != 0;
    }

    /**
     * @brief Sets or clears bit 0.
     */
    constexpr void setBit0(bool enabled) noexcept {
        setMasked(Bit0Mask, enabled);
    }

    /**
     * @brief Sets or clears bit 1.
     */
    constexpr void setBit1(bool enabled) noexcept {
        setMasked(Bit1Mask, enabled);
    }

    /**
     * @brief Sets or clears bit 2.
     */
    constexpr void setBit2(bool enabled) noexcept {
        setMasked(Bit2Mask, enabled);
    }

    /**
     * @brief Sets or clears bit 3.
     */
    constexpr void setBit3(bool enabled) noexcept {
        setMasked(Bit3Mask, enabled);
    }

    /**
     * @brief Returns true if the selected bit is set.
     *
     * @param index Bit index in range 0..3.
     * @throws std::out_of_range If @p index is outside 0..3.
     */
    [[nodiscard]] bool bit(std::uint8_t index) const {
        if (index > 3) {
            throw std::out_of_range("Block metadata bit index must be in range 0..3");
        }

        return (value_ & static_cast<std::uint8_t>(1u << index)) != 0;
    }

    /**
     * @brief Sets or clears the selected bit.
     *
     * @param index Bit index in range 0..3.
     * @param enabled Whether the bit should be set.
     * @throws std::out_of_range If @p index is outside 0..3.
     */
    void setBit(std::uint8_t index, bool enabled) {
        if (index > 3) {
            throw std::out_of_range("Block metadata bit index must be in range 0..3");
        }

        setMasked(static_cast<std::uint8_t>(1u << index), enabled);
    }

    /**
     * @brief Returns new metadata with bit 0 changed.
     */
    [[nodiscard]] constexpr BlockMetadata withBit0(bool enabled) const noexcept {
        BlockMetadata copy = *this;
        copy.setBit0(enabled);
        return copy;
    }

    /**
     * @brief Returns new metadata with bit 1 changed.
     */
    [[nodiscard]] constexpr BlockMetadata withBit1(bool enabled) const noexcept {
        BlockMetadata copy = *this;
        copy.setBit1(enabled);
        return copy;
    }

    /**
     * @brief Returns new metadata with bit 2 changed.
     */
    [[nodiscard]] constexpr BlockMetadata withBit2(bool enabled) const noexcept {
        BlockMetadata copy = *this;
        copy.setBit2(enabled);
        return copy;
    }

    /**
     * @brief Returns new metadata with bit 3 changed.
     */
    [[nodiscard]] constexpr BlockMetadata withBit3(bool enabled) const noexcept {
        BlockMetadata copy = *this;
        copy.setBit3(enabled);
        return copy;
    }

    /**
     * @brief Compares two metadata values.
     */
    friend constexpr bool operator==(BlockMetadata lhs, BlockMetadata rhs) noexcept {
        return lhs.rawValue() == rhs.rawValue();
    }

    /**
     * @brief Compares two metadata values.
     */
    friend constexpr bool operator!=(BlockMetadata lhs, BlockMetadata rhs) noexcept {
        return !(lhs == rhs);
    }

private:
    struct MaskedTag {};

    constexpr BlockMetadata(std::uint8_t value, MaskedTag) noexcept
        : value_(static_cast<std::uint8_t>(value & MetadataMask)) {}

    constexpr void setMasked(std::uint8_t mask, bool enabled) noexcept {
        if (enabled) {
            value_ = static_cast<std::uint8_t>(value_ | mask);
        } else {
            value_ = static_cast<std::uint8_t>(value_ & ~mask);
        }

        value_ = static_cast<std::uint8_t>(value_ & MetadataMask);
    }

    std::uint8_t value_ = 0;
};

} // namespace GalaxyEggbert::Worlds