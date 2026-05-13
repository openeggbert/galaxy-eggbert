#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace GalaxyEggbert::Worlds {

/**
 * @brief Returns the minimum number of bits needed for palette indices.
 *
 * The result is never smaller than 1. For example:
 * - 1..2 entries   -> 1 bit
 * - 3..4 entries   -> 2 bits
 * - 5..8 entries   -> 3 bits
 * - 129..256 entries -> 8 bits
 */
std::uint8_t bitsNeededForPalette(std::size_t paletteCount);

std::vector<std::uint64_t> packPaletteIndices(
    const std::vector<std::uint16_t>& indices,
    std::uint8_t bitsPerValue);

std::vector<std::uint16_t> unpackPaletteIndices(
    const std::vector<std::uint64_t>& words,
    std::size_t valueCount,
    std::uint8_t bitsPerValue);

std::uint16_t getPackedIndex(
    const std::vector<std::uint64_t>& words,
    std::size_t valueIndex,
    std::uint8_t bitsPerValue);

void setPackedIndex(
    std::vector<std::uint64_t>& words,
    std::size_t valueIndex,
    std::uint8_t bitsPerValue,
    std::uint16_t value);

std::size_t packedWordCount(std::size_t valueCount, std::uint8_t bitsPerValue);

} // namespace GalaxyEggbert::Worlds
