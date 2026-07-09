#include "GalaxyEggbert/Worlds/Chunk.hpp"

#include <array>
#include <cstdint>
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

std::uint32_t readU32LE(const std::string& bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset]))
        | (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 1])) << 8)
        | (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 2])) << 16)
        | (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 3])) << 24);
}

void writeU32LE(std::string& bytes, std::size_t offset, std::uint32_t value) {
    bytes[offset] = static_cast<char>(value & 0xFFu);
    bytes[offset + 1] = static_cast<char>((value >> 8) & 0xFFu);
    bytes[offset + 2] = static_cast<char>((value >> 16) & 0xFFu);
    bytes[offset + 3] = static_cast<char>((value >> 24) & 0xFFu);
}

std::size_t extraMetadataSectionOffset(const Chunk& chunk) {
    return 24u
        + chunk.palette().size() * sizeof(std::uint16_t)
        + chunk.packedIndices().size() * sizeof(std::uint64_t);
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

TEST(ChunkTests, AllAirChunkWithExtraMetadataIsNotEmpty) {
    Chunk chunk;
    ASSERT_TRUE(chunk.isEmpty());

    chunk.setExtraMetadata(0, 1, {0xAB});
    EXPECT_FALSE(chunk.isEmpty());
    EXPECT_TRUE(chunk.isUniform());

    chunk.removeExtraMetadata(0, 1);
    EXPECT_TRUE(chunk.isEmpty());
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

TEST(ChunkSerializationTests, WriteUsesVersion2HeaderWithoutExtraMetadataByDefault) {
    const Chunk chunk;
    const std::string bytes = serializeChunk(chunk);
    ASSERT_GE(bytes.size(), 24u);
    EXPECT_EQ(static_cast<unsigned char>(bytes[4]), 2u);
    EXPECT_EQ(static_cast<unsigned char>(bytes[7]), 0u);
    EXPECT_EQ(readU32LE(bytes, 20), 0u);
}

TEST(ChunkSerializationTests, RoundTripPreservesSparseExtraMetadataRecords) {
    Chunk original;
    original.setExtraMetadata(0, 10, {0x01, 0x02, 0x03});
    original.setExtraMetadata(999, 11, {});

    const Chunk loaded = deserializeChunk(serializeChunk(original));
    EXPECT_EQ(loaded.extraMetadata(), original.extraMetadata());
    EXPECT_FALSE(loaded.isDirty());
}

TEST(ChunkTests, ExtraMetadataSetReplaceAndRemoveWorksByLocalBlockIndexAndType) {
    Chunk chunk;
    chunk.clearDirty();

    chunk.setExtraMetadata(5, 42, {0xAA});
    EXPECT_TRUE(chunk.isDirty());
    ASSERT_EQ(chunk.extraMetadata().size(), 1u);

    chunk.clearDirty();
    chunk.setExtraMetadata(5, 42, {0xAA});
    EXPECT_FALSE(chunk.isDirty());
    ASSERT_EQ(chunk.extraMetadata().size(), 1u);

    chunk.setExtraMetadata(5, 42, {0xBB, 0xCC});
    EXPECT_TRUE(chunk.isDirty());
    ASSERT_EQ(chunk.extraMetadata().size(), 1u);
    EXPECT_EQ(chunk.extraMetadata()[0].payload, (std::vector<std::uint8_t>{0xBB, 0xCC}));

    chunk.clearDirty();
    EXPECT_FALSE(chunk.removeExtraMetadata(5, 41));
    EXPECT_FALSE(chunk.isDirty());

    EXPECT_TRUE(chunk.removeExtraMetadata(5, 42));
    EXPECT_TRUE(chunk.isDirty());
    EXPECT_TRUE(chunk.extraMetadata().empty());
}

TEST(ChunkSerializationTests, ReadSupportsLegacyVersion1PayloadWithoutExtraMetadata) {
    Chunk chunk;
    chunk.setBlock(1, 2, 3, Block::make(8, 1));

    std::string bytes = serializeChunk(chunk);
    ASSERT_GE(bytes.size(), 24u);
    bytes[4] = static_cast<char>(1);
    bytes.erase(20, 4);

    const Chunk loaded = deserializeChunk(bytes);
    EXPECT_EQ(loaded.getBlock(1, 2, 3), Block::make(8, 1));
    EXPECT_TRUE(loaded.extraMetadata().empty());
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

TEST(ChunkSerializationTests, ReadRejectsMismatchedExtraMetadataFlagAndSize) {
    Chunk chunk;
    std::string bytes = serializeChunk(chunk);
    ASSERT_GE(bytes.size(), 24u);
    bytes[7] = static_cast<char>(0x01);
    EXPECT_THROW(static_cast<void>(deserializeChunk(bytes)), std::runtime_error);
}

TEST(ChunkSerializationTests, ReadRejectsInvalidExtraMetadataMagic) {
    Chunk chunk;
    chunk.setExtraMetadata(7, 3, {0x99});
    std::string bytes = serializeChunk(chunk);

    const std::size_t sectionOffset = extraMetadataSectionOffset(chunk);
    ASSERT_GT(readU32LE(bytes, 20), 0u);
    ASSERT_GE(bytes.size(), sectionOffset + 4u);
    bytes[sectionOffset] = 'X';
    EXPECT_THROW(static_cast<void>(deserializeChunk(bytes)), std::runtime_error);
}

TEST(ChunkSerializationTests, ReadRejectsOutOfRangeExtraMetadataLocalBlockIndex) {
    Chunk chunk;
    chunk.setExtraMetadata(4, 2, {0x11, 0x22});
    std::string bytes = serializeChunk(chunk);

    const std::size_t sectionOffset = extraMetadataSectionOffset(chunk);
    ASSERT_GE(bytes.size(), sectionOffset + 16u);
    writeU32LE(bytes, sectionOffset + 12u, static_cast<std::uint32_t>(Chunk::Volume));
    EXPECT_THROW(static_cast<void>(deserializeChunk(bytes)), std::runtime_error);
}

} // namespace GalaxyEggbert::Worlds