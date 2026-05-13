#include "GalaxyEggbert/Worlds/BitPacking.hpp"

#include "GalaxyEggbert/Worlds/VoxelConfig.hpp"

#include <limits>
#include <stdexcept>

namespace GalaxyEggbert::Worlds {

std::uint8_t bitsNeededForPalette(std::size_t paletteCount) {
    if (paletteCount == 0 || paletteCount > VoxelConfig::MaxPaletteEntries) {
        throw std::out_of_range("Palette count must be in range 1..256");
    }

    std::uint8_t bits = 1;
    std::size_t capacity = 2;
    while (capacity < paletteCount) {
        ++bits;
        capacity <<= 1;
    }
    return bits;
}

    std::vector<std::uint64_t> packPaletteIndices(
    const std::vector<std::uint16_t>& indices,
    std::uint8_t bitsPerValue) {

    std::vector<std::uint64_t> words(packedWordCount(indices.size(), bitsPerValue), 0);
    for (std::size_t i = 0; i < indices.size(); ++i) {
        setPackedIndex(words, i, bitsPerValue, indices[i]);
    }
    return words;
}

    
std::size_t packedWordCount(std::size_t valueCount, std::uint8_t bitsPerValue) {
    if (bitsPerValue == 0 || bitsPerValue > 16) {
        throw std::out_of_range("bitsPerValue must be in range 1..16");
    }
    const std::size_t totalBits = valueCount * static_cast<std::size_t>(bitsPerValue);
    return (totalBits + 63) / 64;
}

std::uint16_t getPackedIndex(
    const std::vector<std::uint64_t>& words,
    std::size_t valueIndex,
    std::uint8_t bitsPerValue) {

    if (bitsPerValue == 0 || bitsPerValue > 16) {
        throw std::out_of_range("bitsPerValue must be in range 1..16");
    }

    const std::size_t bitOffset = valueIndex * static_cast<std::size_t>(bitsPerValue);
    const std::size_t wordIndex = bitOffset / 64;
    const std::uint8_t bitInWord = static_cast<std::uint8_t>(bitOffset % 64);

    if (wordIndex >= words.size()) {
        throw std::out_of_range("Packed value index is outside of storage");
    }

    const std::uint64_t mask = (std::uint64_t{1} << bitsPerValue) - 1;
    std::uint64_t value = words[wordIndex] >> bitInWord;

    const std::uint8_t remainingInWord = static_cast<std::uint8_t>(64 - bitInWord);
    if (remainingInWord < bitsPerValue) {
        if (wordIndex + 1 >= words.size()) {
            throw std::out_of_range("Packed value crosses outside of storage");
        }
        value |= words[wordIndex + 1] << remainingInWord;
    }

    return static_cast<std::uint16_t>(value & mask);
}

void setPackedIndex(
    std::vector<std::uint64_t>& words,
    std::size_t valueIndex,
    std::uint8_t bitsPerValue,
    std::uint16_t value) {

    if (bitsPerValue == 0 || bitsPerValue > 16) {
        throw std::out_of_range("bitsPerValue must be in range 1..16");
    }

    const std::uint64_t mask = (std::uint64_t{1} << bitsPerValue) - 1;
    if (value > mask) {
        throw std::out_of_range("Packed value does not fit into bitsPerValue");
    }

    const std::size_t bitOffset = valueIndex * static_cast<std::size_t>(bitsPerValue);
    const std::size_t wordIndex = bitOffset / 64;
    const std::uint8_t bitInWord = static_cast<std::uint8_t>(bitOffset % 64);

    if (wordIndex >= words.size()) {
        throw std::out_of_range("Packed value index is outside of storage");
    }

    words[wordIndex] &= ~(mask << bitInWord);
    words[wordIndex] |= (static_cast<std::uint64_t>(value) & mask) << bitInWord;

    const std::uint8_t remainingInWord = static_cast<std::uint8_t>(64 - bitInWord);
    if (remainingInWord < bitsPerValue) {
        if (wordIndex + 1 >= words.size()) {
            throw std::out_of_range("Packed value crosses outside of storage");
        }
        const std::uint8_t overflowBits = static_cast<std::uint8_t>(bitsPerValue - remainingInWord);
        const std::uint64_t overflowMask = (std::uint64_t{1} << overflowBits) - 1;
        words[wordIndex + 1] &= ~overflowMask;
        words[wordIndex + 1] |= (static_cast<std::uint64_t>(value) >> remainingInWord) & overflowMask;
    }
}

std::vector<std::uint16_t> unpackPaletteIndices(
    const std::vector<std::uint64_t>& words,
    std::size_t valueCount,
    std::uint8_t bitsPerValue) {

    std::vector<std::uint16_t> indices;
    indices.reserve(valueCount);
    for (std::size_t i = 0; i < valueCount; ++i) {
        indices.push_back(getPackedIndex(words, i, bitsPerValue));
    }
    return indices;
}

} // namespace GalaxyEggbert::Worlds
