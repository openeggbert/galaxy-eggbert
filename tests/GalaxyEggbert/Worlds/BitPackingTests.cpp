#include "GalaxyEggbert/Worlds/BitPacking.hpp"

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace GalaxyEggbert::Worlds {

TEST(BitPackingTests, BitsNeededForPaletteBoundaries) {
    EXPECT_EQ(bitsNeededForPalette(1), 1);
    EXPECT_EQ(bitsNeededForPalette(2), 1);
    EXPECT_EQ(bitsNeededForPalette(3), 2);
    EXPECT_EQ(bitsNeededForPalette(4), 2);
    EXPECT_EQ(bitsNeededForPalette(5), 3);
    EXPECT_EQ(bitsNeededForPalette(8), 3);
    EXPECT_EQ(bitsNeededForPalette(9), 4);
    EXPECT_EQ(bitsNeededForPalette(128), 7);
    EXPECT_EQ(bitsNeededForPalette(129), 8);
    EXPECT_EQ(bitsNeededForPalette(256), 8);
}

TEST(BitPackingTests, BitsNeededForPaletteRejectsOutOfRange) {
    EXPECT_THROW(static_cast<void>(bitsNeededForPalette(0)), std::out_of_range);
    EXPECT_THROW(static_cast<void>(bitsNeededForPalette(257)), std::out_of_range);
}

TEST(BitPackingTests, PackAndUnpackRoundTripAcrossWordBoundaries) {
    std::vector<std::uint16_t> indices;
    indices.reserve(40);
    for (std::uint16_t i = 0; i < 40; ++i) {
        indices.push_back(static_cast<std::uint16_t>((i * 3) % 31));
    }

    const std::uint8_t bitsPerValue = 5;
    const auto packed = packPaletteIndices(indices, bitsPerValue);
    const auto unpacked = unpackPaletteIndices(packed, indices.size(), bitsPerValue);
    EXPECT_EQ(unpacked, indices);
}

TEST(BitPackingTests, SetAndGetPackedIndexSupportCrossWordValues) {
    std::vector<std::uint64_t> words(packedWordCount(7, 10), 0);
    setPackedIndex(words, 6, 10, 939);
    EXPECT_EQ(getPackedIndex(words, 6, 10), 939);
}

TEST(BitPackingTests, PackedHelpersValidateArguments) {
    std::vector<std::uint64_t> words(1, 0);
    EXPECT_THROW(static_cast<void>(packedWordCount(1, 0)), std::out_of_range);
    EXPECT_THROW(static_cast<void>(packedWordCount(1, 17)), std::out_of_range);
    EXPECT_THROW(static_cast<void>(setPackedIndex(words, 0, 4, 16)), std::out_of_range);
    EXPECT_THROW(static_cast<void>(setPackedIndex(words, 64, 1, 1)), std::out_of_range);
    EXPECT_THROW(static_cast<void>(getPackedIndex(words, 64, 1)), std::out_of_range);
}

} // namespace GalaxyEggbert::Worlds