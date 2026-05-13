#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

/**
 * @file
 * @brief Bit-level packing helpers used by palette-compressed voxel chunks.
 */

namespace GalaxyEggbert::Worlds {

/**
 * @brief Returns the minimum number of bits needed for palette indices.
 *
 * The result is never smaller than 1. For example:
 * - 1..2 entries   -> 1 bit
 * - 3..4 entries   -> 2 bits
 * - 5..8 entries   -> 3 bits
 * - 129..256 entries -> 8 bits
 *
 * @param paletteCount Number of palette entries.
 * @return Minimum bit width that can represent all indices in range
 *         <tt>[0, paletteCount - 1]</tt>.
 * @throws std::out_of_range If @p paletteCount is outside the supported range
 *         <tt>[1, VoxelConfig::MaxPaletteEntries]</tt>.
 */
std::uint8_t bitsNeededForPalette(std::size_t paletteCount);

/**
 * @brief Packs palette indices into a dense little-endian bit stream of 64-bit words.
 *
 * Index <tt>i</tt> occupies the bit range
 * <tt>[i * bitsPerValue, (i + 1) * bitsPerValue)</tt>.
 *
 * @param indices Source palette indices.
 * @param bitsPerValue Number of bits used for one index.
 * @return Packed words containing all values from @p indices.
 * @throws std::out_of_range If @p bitsPerValue is outside <tt>[1, 16]</tt>,
 *         if an index cannot be represented in @p bitsPerValue bits,
 *         or if addressing would exceed provided storage.
 */
std::vector<std::uint64_t> packPaletteIndices(
    const std::vector<std::uint16_t>& indices,
    std::uint8_t bitsPerValue);

/**
 * @brief Unpacks a sequence of packed palette indices.
 *
 * @param words Packed storage created by @ref packPaletteIndices.
 * @param valueCount Number of values to decode.
 * @param bitsPerValue Number of bits used for one index.
 * @return Vector with exactly @p valueCount decoded indices.
 * @throws std::out_of_range If @p bitsPerValue is outside <tt>[1, 16]</tt>
 *         or packed storage does not contain all requested values.
 */
std::vector<std::uint16_t> unpackPaletteIndices(
    const std::vector<std::uint64_t>& words,
    std::size_t valueCount,
    std::uint8_t bitsPerValue);

/**
 * @brief Reads one index from packed storage.
 *
 * @param words Packed storage words.
 * @param valueIndex Zero-based index of the logical value to read.
 * @param bitsPerValue Number of bits used for one index.
 * @return Decoded index value.
 * @throws std::out_of_range If @p bitsPerValue is outside <tt>[1, 16]</tt>
 *         or @p valueIndex addresses bits beyond @p words.
 */
std::uint16_t getPackedIndex(
    const std::vector<std::uint64_t>& words,
    std::size_t valueIndex,
    std::uint8_t bitsPerValue);

/**
 * @brief Writes one index into packed storage.
 *
 * @param words Packed storage words.
 * @param valueIndex Zero-based index of the logical value to write.
 * @param bitsPerValue Number of bits used for one index.
 * @param value Value to encode at @p valueIndex.
 * @throws std::out_of_range If @p bitsPerValue is outside <tt>[1, 16]</tt>,
 *         if @p value does not fit in @p bitsPerValue bits,
 *         or @p valueIndex addresses bits beyond @p words.
 */
void setPackedIndex(
    std::vector<std::uint64_t>& words,
    std::size_t valueIndex,
    std::uint8_t bitsPerValue,
    std::uint16_t value);

/**
 * @brief Returns the number of 64-bit words needed for packed storage.
 *
 * @param valueCount Number of logical values that will be stored.
 * @param bitsPerValue Number of bits used for one value.
 * @return Number of 64-bit words required to store all values.
 * @throws std::out_of_range If @p bitsPerValue is outside <tt>[1, 16]</tt>.
 */
std::size_t packedWordCount(std::size_t valueCount, std::uint8_t bitsPerValue);

} // namespace GalaxyEggbert::Worlds
