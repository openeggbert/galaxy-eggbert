#include "GalaxyEggbert/Worlds/BlockMetadata.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>

namespace GalaxyEggbert::Worlds {

TEST(BlockMetadataTest, DefaultConstructorCreatesZeroMetadata) {
    const BlockMetadata metadata;

    EXPECT_EQ(metadata.rawValue(), 0);
    EXPECT_EQ(metadata.toUInt8(), 0);

    EXPECT_FALSE(metadata.bit0());
    EXPECT_FALSE(metadata.bit1());
    EXPECT_FALSE(metadata.bit2());
    EXPECT_FALSE(metadata.bit3());
}

TEST(BlockMetadataTest, ConstructorAcceptsValidFourBitValues) {
    for (std::uint8_t value = 0; value <= 15; ++value) {
        const BlockMetadata metadata(value);

        EXPECT_EQ(metadata.rawValue(), value);
        EXPECT_EQ(metadata.toUInt8(), value);
    }
}

TEST(BlockMetadataTest, ConstructorRejectsValuesLargerThanFourBits) {
    EXPECT_THROW(BlockMetadata(16), std::out_of_range);
    EXPECT_THROW(BlockMetadata(31), std::out_of_range);
    EXPECT_THROW(BlockMetadata(255), std::out_of_range);
}

TEST(BlockMetadataTest, FromMaskedKeepsOnlyLowFourBits) {
    EXPECT_EQ(BlockMetadata::fromMasked(0x00).rawValue(), 0x00);
    EXPECT_EQ(BlockMetadata::fromMasked(0x0F).rawValue(), 0x0F);
    EXPECT_EQ(BlockMetadata::fromMasked(0x10).rawValue(), 0x00);
    EXPECT_EQ(BlockMetadata::fromMasked(0x1F).rawValue(), 0x0F);
    EXPECT_EQ(BlockMetadata::fromMasked(0xA5).rawValue(), 0x05);
    EXPECT_EQ(BlockMetadata::fromMasked(0xFF).rawValue(), 0x0F);
}

TEST(BlockMetadataTest, IndividualBitGettersReturnExpectedValues) {
    EXPECT_FALSE(BlockMetadata(0b0000).bit0());
    EXPECT_FALSE(BlockMetadata(0b0000).bit1());
    EXPECT_FALSE(BlockMetadata(0b0000).bit2());
    EXPECT_FALSE(BlockMetadata(0b0000).bit3());

    EXPECT_TRUE(BlockMetadata(0b0001).bit0());
    EXPECT_TRUE(BlockMetadata(0b0010).bit1());
    EXPECT_TRUE(BlockMetadata(0b0100).bit2());
    EXPECT_TRUE(BlockMetadata(0b1000).bit3());

    const BlockMetadata metadata(0b1011);

    EXPECT_TRUE(metadata.bit0());
    EXPECT_TRUE(metadata.bit1());
    EXPECT_FALSE(metadata.bit2());
    EXPECT_TRUE(metadata.bit3());
}

TEST(BlockMetadataTest, IndividualBitSettersSetBits) {
    BlockMetadata metadata;

    metadata.setBit0(true);
    EXPECT_EQ(metadata.rawValue(), 0b0001);

    metadata.setBit1(true);
    EXPECT_EQ(metadata.rawValue(), 0b0011);

    metadata.setBit2(true);
    EXPECT_EQ(metadata.rawValue(), 0b0111);

    metadata.setBit3(true);
    EXPECT_EQ(metadata.rawValue(), 0b1111);
}

TEST(BlockMetadataTest, IndividualBitSettersClearBits) {
    BlockMetadata metadata(0b1111);

    metadata.setBit0(false);
    EXPECT_EQ(metadata.rawValue(), 0b1110);

    metadata.setBit1(false);
    EXPECT_EQ(metadata.rawValue(), 0b1100);

    metadata.setBit2(false);
    EXPECT_EQ(metadata.rawValue(), 0b1000);

    metadata.setBit3(false);
    EXPECT_EQ(metadata.rawValue(), 0b0000);
}

TEST(BlockMetadataTest, SettersDoNotAffectOtherBits) {
    BlockMetadata metadata(0b1010);

    metadata.setBit0(true);
    EXPECT_EQ(metadata.rawValue(), 0b1011);

    metadata.setBit2(true);
    EXPECT_EQ(metadata.rawValue(), 0b1111);

    metadata.setBit1(false);
    EXPECT_EQ(metadata.rawValue(), 0b1101);

    metadata.setBit3(false);
    EXPECT_EQ(metadata.rawValue(), 0b0101);
}

TEST(BlockMetadataTest, GenericBitGetterReturnsExpectedValues) {
    const BlockMetadata metadata(0b1010);

    EXPECT_FALSE(metadata.bit(0));
    EXPECT_TRUE(metadata.bit(1));
    EXPECT_FALSE(metadata.bit(2));
    EXPECT_TRUE(metadata.bit(3));
}

TEST(BlockMetadataTest, GenericBitGetterRejectsInvalidIndex) {
    const BlockMetadata metadata;

    EXPECT_THROW(metadata.bit(4), std::out_of_range);
    EXPECT_THROW(metadata.bit(255), std::out_of_range);
}

TEST(BlockMetadataTest, GenericSetBitSetsAndClearsSelectedBits) {
    BlockMetadata metadata;

    metadata.setBit(0, true);
    EXPECT_EQ(metadata.rawValue(), 0b0001);

    metadata.setBit(1, true);
    EXPECT_EQ(metadata.rawValue(), 0b0011);

    metadata.setBit(2, true);
    EXPECT_EQ(metadata.rawValue(), 0b0111);

    metadata.setBit(3, true);
    EXPECT_EQ(metadata.rawValue(), 0b1111);

    metadata.setBit(1, false);
    EXPECT_EQ(metadata.rawValue(), 0b1101);

    metadata.setBit(3, false);
    EXPECT_EQ(metadata.rawValue(), 0b0101);
}

TEST(BlockMetadataTest, GenericSetBitRejectsInvalidIndex) {
    BlockMetadata metadata;

    EXPECT_THROW(metadata.setBit(4, true), std::out_of_range);
    EXPECT_THROW(metadata.setBit(255, true), std::out_of_range);
}

TEST(BlockMetadataTest, WithBitMethodsReturnModifiedCopy) {
    const BlockMetadata original(0b0000);

    const BlockMetadata withBit0 = original.withBit0(true);
    const BlockMetadata withBit1 = original.withBit1(true);
    const BlockMetadata withBit2 = original.withBit2(true);
    const BlockMetadata withBit3 = original.withBit3(true);

    EXPECT_EQ(original.rawValue(), 0b0000);

    EXPECT_EQ(withBit0.rawValue(), 0b0001);
    EXPECT_EQ(withBit1.rawValue(), 0b0010);
    EXPECT_EQ(withBit2.rawValue(), 0b0100);
    EXPECT_EQ(withBit3.rawValue(), 0b1000);
}

TEST(BlockMetadataTest, WithBitMethodsCanClearBits) {
    const BlockMetadata original(0b1111);

    const BlockMetadata withoutBit0 = original.withBit0(false);
    const BlockMetadata withoutBit1 = original.withBit1(false);
    const BlockMetadata withoutBit2 = original.withBit2(false);
    const BlockMetadata withoutBit3 = original.withBit3(false);

    EXPECT_EQ(original.rawValue(), 0b1111);

    EXPECT_EQ(withoutBit0.rawValue(), 0b1110);
    EXPECT_EQ(withoutBit1.rawValue(), 0b1101);
    EXPECT_EQ(withoutBit2.rawValue(), 0b1011);
    EXPECT_EQ(withoutBit3.rawValue(), 0b0111);
}

TEST(BlockMetadataTest, EqualityComparesRawMetadataValue) {
    EXPECT_EQ(BlockMetadata(0), BlockMetadata(0));
    EXPECT_EQ(BlockMetadata(15), BlockMetadata(15));

    EXPECT_NE(BlockMetadata(0), BlockMetadata(1));
    EXPECT_NE(BlockMetadata(7), BlockMetadata(8));
}

TEST(BlockMetadataTest, ConstantsHaveExpectedValues) {
    EXPECT_EQ(BlockMetadata::MetadataMask, 0x0F);
    EXPECT_EQ(BlockMetadata::Bit0Mask, 0x01);
    EXPECT_EQ(BlockMetadata::Bit1Mask, 0x02);
    EXPECT_EQ(BlockMetadata::Bit2Mask, 0x04);
    EXPECT_EQ(BlockMetadata::Bit3Mask, 0x08);
}

} // namespace GalaxyEggbert::Worlds