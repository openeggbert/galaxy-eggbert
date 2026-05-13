#include "GalaxyEggbert/Worlds/Block.hpp"

#include <cstdint>

#include <gtest/gtest.h>

namespace GalaxyEggbert::Worlds {

TEST(BlockTests, AirDefaultsToZeroAndIsAir) {
    const Block block;
    EXPECT_EQ(block.rawValue(), 0);
    EXPECT_TRUE(block.isAir());
    EXPECT_EQ(block, Block::air());
}

TEST(BlockTests, MakePacksAndUnpacksTypeAndMetadata) {
    const Block block = Block::make(1234, 9);
    EXPECT_EQ(block.type(), 1234);
    EXPECT_EQ(block.metadata(), 9);
    EXPECT_EQ(block.rawValue(), static_cast<std::uint16_t>((1234 << 4) | 9));
}

TEST(BlockTests, MakeSupportsMaximumValues) {
    const Block block = Block::make(VoxelConfig::MaxBlockType, VoxelConfig::MaxBlockMetadata);
    EXPECT_EQ(block.type(), VoxelConfig::MaxBlockType);
    EXPECT_EQ(block.metadata(), VoxelConfig::MaxBlockMetadata);
}

TEST(BlockTests, MakeThrowsForOutOfRangeValues) {
    EXPECT_THROW(
        static_cast<void>(Block::make(VoxelConfig::MaxBlockType + 1, 0)),
        std::out_of_range);
    EXPECT_THROW(
        static_cast<void>(Block::make(1, static_cast<std::uint8_t>(VoxelConfig::MaxBlockMetadata + 1))),
        std::out_of_range);
}

TEST(BlockTests, EqualityComparesPackedValue) {
    const Block lhs = Block::make(7, 3);
    const Block rhs = Block::make(7, 3);
    const Block other = Block::make(7, 4);
    EXPECT_EQ(lhs, rhs);
    EXPECT_NE(lhs, other);
}

} // namespace GalaxyEggbert::Worlds