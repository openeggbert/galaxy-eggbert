#include "GalaxyEggbert/Worlds/Chunk.hpp"

#include <array>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

namespace GalaxyEggbert::Worlds {

namespace {

std::string serializeChunk(const Chunk& chunk) {
    std::ostringstream stream(std::ios::binary);
    chunk.write(stream);
    return stream.str();
}

Chunk deserializeChunk(const std::string& bytes) {
    std::istringstream stream(bytes, std::ios::binary);
    return Chunk::read(stream);
}

} // namespace

TEST(ChunkTests, DefaultChunkIsUniformEmptyAndClean) {
    const Chunk chunk;
    EXPECT_TRUE(chunk.isEmpty());
    EXPECT_TRUE(chunk.isUniform());
    EXPECT_FALSE(chunk.isDirty());
    EXPECT_EQ(chunk.palette().size(), 1);
    EXPECT_EQ(chunk.palette().front(), Block::air());
    EXPECT_EQ(chunk.bitsPerBlock(), 1);
}

TEST(ChunkTests, CreateUniformCreatesExpectedBlockContent) {
    const Block stone = Block::make(3, 2);
    const Chunk chunk = Chunk::createUniform(stone);
    EXPECT_TRUE(chunk.isUniform());
    EXPECT_FALSE(chunk.isEmpty());
    EXPECT_FALSE(chunk.isDirty());
    EXPECT_EQ(chunk.getBlock(0, 0, 0), stone);
    EXPECT_EQ(chunk.getBlock(Chunk::Size - 1, Chunk::Size - 1, Chunk::Size - 1), stone);
}

TEST(ChunkTests, SetAndGetBlockAtChunkBoundaries) {
    Chunk chunk;
    const Block grass = Block::make(1, 0);
    const Block dirt = Block::make(2, 1);

    chunk.setBlock(0, 0, 0, grass);
    chunk.setBlock(Chunk::Size - 1, Chunk::Size - 1, Chunk::Size - 1, dirt);

    EXPECT_EQ(chunk.getBlock(0, 0, 0), grass);
    EXPECT_EQ(chunk.getBlock(Chunk::Size - 1, Chunk::Size - 1, Chunk::Size - 1), dirt);
}

TEST(ChunkTests, SetBlockMarksDirtyAndClearDirtyResetsFlag) {
    Chunk chunk;
    EXPECT_FALSE(chunk.isDirty());
    chunk.setBlock(1, 2, 3, Block::make(4, 0));
    EXPECT_TRUE(chunk.isDirty());
    chunk.clearDirty();
    EXPECT_FALSE(chunk.isDirty());
}

TEST(ChunkTests, LocalCoordinateValidationRejectsOutOfRange) {
    Chunk chunk;
    EXPECT_THROW(static_cast<void>(chunk.getBlock(Chunk::Size, 0, 0)), std::out_of_range);
    EXPECT_THROW(static_cast<void>(chunk.getBlock(0, Chunk::Size, 0)), std::out_of_range);
    EXPECT_THROW(static_cast<void>(chunk.getBlock(0, 0, Chunk::Size)), std::out_of_range);
    EXPECT_THROW(static_cast<void>(chunk.setBlock(Chunk::Size, 0, 0, Block::make(1, 0))), std::out_of_range);
}

TEST(ChunkTests, PaletteGrowthUpdatesBitWidth) {
    Chunk chunk;
    chunk.setBlock(0, 0, 0, Block::make(1, 0));
    EXPECT_EQ(chunk.palette().size(), 2);
    EXPECT_EQ(chunk.bitsPerBlock(), 1);

    chunk.setBlock(1, 0, 0, Block::make(2, 0));
    EXPECT_EQ(chunk.palette().size(), 3);
    EXPECT_EQ(chunk.bitsPerBlock(), 2);

    chunk.setBlock(2, 0, 0, Block::make(3, 0));
    chunk.setBlock(3, 0, 0, Block::make(4, 0));
    EXPECT_EQ(chunk.palette().size(), 5);
    EXPECT_EQ(chunk.bitsPerBlock(), 3);
}

TEST(ChunkTests, FromBlocksRoundTripPreservesDenseData) {
    std::array<Block, Chunk::Volume> blocks{};
    blocks.fill(Block::air());
    blocks[0] = Block::make(1, 1);
    blocks[7] = Block::make(2, 2);
    blocks[33] = Block::make(3, 3);
    blocks[999] = Block::make(4, 4);

    const Chunk chunk = Chunk::fromBlocks(blocks);
    EXPECT_FALSE(chunk.isDirty());
    EXPECT_EQ(chunk.unpackBlocks(), blocks);
}

TEST(ChunkSerializationTests, RoundTripPreservesPaletteAndValues) {
    Chunk original;
    original.setBlock(0, 0, 0, Block::make(1, 0));
    original.setBlock(1, 2, 3, Block::make(2, 1));
    original.setBlock(9, 9, 9, Block::make(3, 2));

    const Chunk loaded = deserializeChunk(serializeChunk(original));
    EXPECT_EQ(loaded.getBlock(0, 0, 0), Block::make(1, 0));
    EXPECT_EQ(loaded.getBlock(1, 2, 3), Block::make(2, 1));
    EXPECT_EQ(loaded.getBlock(9, 9, 9), Block::make(3, 2));
    EXPECT_FALSE(loaded.isDirty());
}

TEST(ChunkSerializationTests, ReadRejectsInvalidMagic) {
    Chunk chunk;
    std::string bytes = serializeChunk(chunk);
    ASSERT_GE(bytes.size(), 4u);
    bytes[0] = 'X';
    EXPECT_THROW(static_cast<void>(deserializeChunk(bytes)), std::runtime_error);
}

TEST(ChunkSerializationTests, ReadRejectsInvalidPaletteCount) {
    Chunk chunk;
    std::string bytes = serializeChunk(chunk);
    ASSERT_GE(bytes.size(), 10u);
    bytes[8] = 0;
    bytes[9] = 0;
    EXPECT_THROW(static_cast<void>(deserializeChunk(bytes)), std::runtime_error);
}

TEST(ChunkSerializationTests, ReadRejectsMismatchedBitsPerBlock) {
    Chunk chunk;
    std::string bytes = serializeChunk(chunk);
    ASSERT_GE(bytes.size(), 7u);
    bytes[6] = static_cast<char>(8);
    EXPECT_THROW(static_cast<void>(deserializeChunk(bytes)), std::runtime_error);
}

} // namespace GalaxyEggbert::Worlds