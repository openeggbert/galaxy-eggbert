#include "GalaxyEggbert/Worlds/World.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

namespace GalaxyEggbert::Worlds {

namespace {

std::filesystem::path makeTempPath(std::string_view extension) {
    static std::atomic<std::uint64_t> sequence{0};
    const auto uniqueSuffix = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count())
        + "_" + std::to_string(sequence.fetch_add(1, std::memory_order_relaxed));
    return std::filesystem::temp_directory_path()
        / ("galaxy_eggbert_world_test_" + uniqueSuffix + std::string(extension));
}

void removeFileNoThrow(const std::filesystem::path& path) {
    std::error_code removeError;
    std::filesystem::remove(path, removeError);
}

std::string readAllBytes(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Cannot open file for reading in test helper");
    }

    std::ostringstream stream;
    stream << in.rdbuf();
    return stream.str();
}

void writeAllBytes(const std::filesystem::path& path, const std::string& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Cannot open file for writing in test helper");
    }
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (!out) {
        throw std::runtime_error("Cannot write file in test helper");
    }
}

} // namespace

TEST(WorldTests, ConstructorAndWorldDimensionsAreConsistent) {
    const World defaultWorld;
    EXPECT_EQ(defaultWorld.chunksPerAxis(), VoxelConfig::WorldChunksPerAxis);
    EXPECT_EQ(defaultWorld.blocksPerAxis(), VoxelConfig::WorldBlocksPerAxis);
    EXPECT_EQ(
        defaultWorld.chunkCount(),
        static_cast<std::size_t>(VoxelConfig::WorldChunksPerAxis)
            * VoxelConfig::WorldChunksPerAxis
            * VoxelConfig::WorldChunksPerAxis);
    EXPECT_THROW(static_cast<void>(World(0)), std::out_of_range);
}

TEST(WorldTests, SetAndGetBlockAcrossChunkBoundaries) {
    World world;
    const Block edgeA = Block::make(10, 0);
    const Block edgeB = Block::make(11, 1);

    world.setBlock(9, 9, 9, edgeA);
    world.setBlock(10, 10, 10, edgeB);

    EXPECT_EQ(world.getBlock(9, 9, 9), edgeA);
    EXPECT_EQ(world.getBlock(10, 10, 10), edgeB);
}

TEST(WorldTests, BlockCoordinateValidationRejectsOutOfRange) {
    World world;
    const auto limit = world.blocksPerAxis();

    EXPECT_THROW(static_cast<void>(world.getBlock(limit, 0, 0)), std::out_of_range);
    EXPECT_THROW(static_cast<void>(world.getBlock(0, limit, 0)), std::out_of_range);
    EXPECT_THROW(static_cast<void>(world.getBlock(0, 0, limit)), std::out_of_range);
    EXPECT_THROW(static_cast<void>(world.setBlock(limit, 0, 0, Block::make(1, 0))), std::out_of_range);
    EXPECT_THROW(world.setSpawnPoint(limit, 0, 0), std::out_of_range);
}

TEST(WorldTests, ChunkCoordinateValidationRejectsOutOfRange) {
    World world;
    const auto limit = world.chunksPerAxis();

    EXPECT_THROW(static_cast<void>(world.chunk(limit, 0, 0)), std::out_of_range);
    EXPECT_THROW(static_cast<void>(world.chunk(0, limit, 0)), std::out_of_range);
    EXPECT_THROW(static_cast<void>(world.chunk(0, 0, limit)), std::out_of_range);
}

TEST(WorldTests, SetAndCollectBlockExtraMetadataAcrossChunkBoundaries) {
    World world;
    world.setBlockExtraMetadata(9, 9, 9, 7, {0x01, 0x02});
    world.setBlockExtraMetadata(10, 10, 10, 7, {0x03});
    world.setBlockExtraMetadata(50, 20, 70, 8, {0xAA, 0xBB, 0xCC});

    const auto typeSeven = world.collectExtraMetadata(7);
    ASSERT_EQ(typeSeven.size(), 2u);
    const bool hasEdgeA = std::any_of(typeSeven.begin(), typeSeven.end(), [](const auto& r) {
        return r.x == 9 && r.y == 9 && r.z == 9 && r.payload == std::vector<std::uint8_t>{0x01, 0x02};
    });
    const bool hasEdgeB = std::any_of(typeSeven.begin(), typeSeven.end(), [](const auto& r) {
        return r.x == 10 && r.y == 10 && r.z == 10 && r.payload == std::vector<std::uint8_t>{0x03};
    });
    EXPECT_TRUE(hasEdgeA);
    EXPECT_TRUE(hasEdgeB);

    const auto typeEight = world.collectExtraMetadata(8);
    ASSERT_EQ(typeEight.size(), 1u);
    EXPECT_EQ(typeEight[0].x, 50);
    EXPECT_EQ(typeEight[0].y, 20);
    EXPECT_EQ(typeEight[0].z, 70);
    EXPECT_EQ(typeEight[0].payload, (std::vector<std::uint8_t>{0xAA, 0xBB, 0xCC}));

    EXPECT_TRUE(world.collectExtraMetadata(9).empty());
}

TEST(WorldTests, SetBlockExtraMetadataRejectsOutOfRangeCoordinates) {
    World world;
    const auto limit = world.blocksPerAxis();
    EXPECT_THROW(world.setBlockExtraMetadata(limit, 0, 0, 1, {}), std::out_of_range);
}

TEST(WorldSerializationTests, SaveAndLoadPreservesBlockExtraMetadata) {
    const auto filePath = makeTempPath(".vwr");

    World world;
    world.setBlockExtraMetadata(3, 4, 5, 42, {0x10, 0x20, 0x30});
    world.saveToFile(filePath);
    const World loaded = World::loadFromFile(filePath);

    const auto records = loaded.collectExtraMetadata(42);
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].x, 3);
    EXPECT_EQ(records[0].y, 4);
    EXPECT_EQ(records[0].z, 5);
    EXPECT_EQ(records[0].payload, (std::vector<std::uint8_t>{0x10, 0x20, 0x30}));

    removeFileNoThrow(filePath);
}

TEST(WorldSerializationTests, SaveAndLoadPreservesSparseAndBoundaryBlocks) {
    const auto filePath = makeTempPath(".vwr");

    World world;
    const Block grass = Block::make(1, 0);
    const Block dirt = Block::make(2, 0);
    const Block stone = Block::make(3, 0);
    const Block lava = Block::make(4, 7);

    world.setBlock(0, 0, 0, grass);
    world.setBlock(10, 1, 10, dirt);
    world.setBlock(99, 99, 99, stone);
    world.setBlock(50, 20, 70, lava);

    world.saveToFile(filePath);
    const World loaded = World::loadFromFile(filePath);

    EXPECT_EQ(loaded.getBlock(0, 0, 0), grass);
    EXPECT_EQ(loaded.getBlock(10, 1, 10), dirt);
    EXPECT_EQ(loaded.getBlock(99, 99, 99), stone);
    EXPECT_EQ(loaded.getBlock(50, 20, 70), lava);
    EXPECT_EQ(loaded.getBlock(5, 5, 5), Block::air());

    removeFileNoThrow(filePath);
}

TEST(WorldSerializationTests, SaveAndLoadPreservesSkyRegion) {
    const auto filePath = makeTempPath(".vwr");

    World world;
    EXPECT_EQ(world.skyRegion(), 0u);
    world.setSkyRegion(7);
    world.saveToFile(filePath);
    const World loaded = World::loadFromFile(filePath);

    EXPECT_EQ(loaded.skyRegion(), 7u);

    removeFileNoThrow(filePath);
}

TEST(WorldSerializationTests, SaveAndLoadPreservesMissionNumber) {
    const auto filePath = makeTempPath(".vwr");

    World world;
    EXPECT_EQ(world.missionNumber(), 0u);
    world.setMissionNumber(11);
    world.saveToFile(filePath);
    const World loaded = World::loadFromFile(filePath);

    EXPECT_EQ(loaded.missionNumber(), 11u);

    removeFileNoThrow(filePath);
}

TEST(WorldSerializationTests, SaveAndLoadPreservesOptionalSpawnPoint) {
    const auto filePath = makeTempPath(".vwr");

    World world;
    EXPECT_FALSE(world.hasSpawnPoint());
    world.setSpawnPoint(12, 34, 56);
    world.saveToFile(filePath);
    const World loaded = World::loadFromFile(filePath);

    EXPECT_TRUE(loaded.hasSpawnPoint());
    EXPECT_EQ(loaded.spawnX(), 12);
    EXPECT_EQ(loaded.spawnY(), 34);
    EXPECT_EQ(loaded.spawnZ(), 56);

    removeFileNoThrow(filePath);
}

TEST(WorldSerializationTests, LegacyZeroSpawnSlotsRemainUnset) {
    const auto filePath = makeTempPath(".vwr");

    const World world;
    world.saveToFile(filePath);
    const World loaded = World::loadFromFile(filePath);

    EXPECT_FALSE(loaded.hasSpawnPoint());
    removeFileNoThrow(filePath);
}

TEST(WorldSerializationTests, LoadRejectsV1FormatVersion) {
    const auto filePath = makeTempPath(".vwr");
    {
        World world;
        world.saveToFile(filePath);
    }

    std::string bytes = readAllBytes(filePath);
    ASSERT_GE(bytes.size(), 5u);
    bytes[4] = static_cast<char>(1); // pretend this is a pre-2026-07-09 v1 file
    writeAllBytes(filePath, bytes);

    EXPECT_THROW(static_cast<void>(World::loadFromFile(filePath)), std::runtime_error);
    removeFileNoThrow(filePath);
}

TEST(WorldSerializationTests, SaveAndLoadEmptyWorldKeepsAirChunks) {
    const auto filePath = makeTempPath(".vwr");

    const World world(2);
    world.saveToFile(filePath);
    const World loaded = World::loadFromFile(filePath);

    EXPECT_EQ(loaded.chunksPerAxis(), 2);
    EXPECT_EQ(loaded.chunkCount(), 8u);
    EXPECT_EQ(loaded.getBlock(0, 0, 0), Block::air());
    EXPECT_EQ(loaded.getBlock(19, 19, 19), Block::air());

    removeFileNoThrow(filePath);
}

TEST(WorldSerializationTests, LoadFromMissingFileThrows) {
    const auto filePath = makeTempPath(".vwr");
    removeFileNoThrow(filePath);
    EXPECT_THROW(static_cast<void>(World::loadFromFile(filePath)), std::runtime_error);
}

TEST(WorldSerializationTests, LoadRejectsInvalidMagic) {
    const auto filePath = makeTempPath(".vwr");
    {
        World world;
        world.saveToFile(filePath);
    }

    std::string bytes = readAllBytes(filePath);
    ASSERT_GE(bytes.size(), 4u);
    bytes[0] = 'X';
    writeAllBytes(filePath, bytes);

    EXPECT_THROW(static_cast<void>(World::loadFromFile(filePath)), std::runtime_error);
    removeFileNoThrow(filePath);
}

TEST(WorldSerializationTests, LoadRejectsUnsupportedFlags) {
    const auto filePath = makeTempPath(".vwr");
    {
        World world;
        world.saveToFile(filePath);
    }

    std::string bytes = readAllBytes(filePath);
    ASSERT_GE(bytes.size(), 8u);
    bytes[7] = static_cast<char>(1);
    writeAllBytes(filePath, bytes);

    EXPECT_THROW(static_cast<void>(World::loadFromFile(filePath)), std::runtime_error);
    removeFileNoThrow(filePath);
}

TEST(WorldSerializationTests, LoadRejectsChunkCountMismatch) {
    const auto filePath = makeTempPath(".vwr");
    {
        World world;
        world.saveToFile(filePath);
    }

    std::string bytes = readAllBytes(filePath);
    ASSERT_GE(bytes.size(), 12u);
    bytes[8] = 0;
    bytes[9] = 0;
    bytes[10] = 0;
    bytes[11] = 0;
    writeAllBytes(filePath, bytes);

    EXPECT_THROW(static_cast<void>(World::loadFromFile(filePath)), std::runtime_error);
    removeFileNoThrow(filePath);
}

} // namespace GalaxyEggbert::Worlds
