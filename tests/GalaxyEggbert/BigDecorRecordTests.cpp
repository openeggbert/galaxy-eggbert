#include "GalaxyEggbert/BigDecorRecord.hpp"

#include <filesystem>
#include <system_error>

#include <gtest/gtest.h>

namespace GalaxyEggbert {

TEST(BigDecorRecordTests, PlaceCollectReplaceAndRemove) {
    Worlds::World world;
    PlaceBigDecor(world, {20, 12, 3, 45});

    auto records = CollectBigDecor(world);
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].icon, 20);
    EXPECT_EQ(records[0].x, 12);
    EXPECT_EQ(records[0].y, 3);
    EXPECT_EQ(records[0].z, 45);

    PlaceBigDecor(world, {87, 12, 3, 45});
    records = CollectBigDecor(world);
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].icon, 87);
    EXPECT_TRUE(RemoveBigDecor(world, 12, 3, 45));
    EXPECT_TRUE(CollectBigDecor(world).empty());
    EXPECT_FALSE(RemoveBigDecor(world, 12, 3, 45));
}

TEST(BigDecorRecordTests, RoundTripsThroughWorldFile) {
    const auto path = std::filesystem::temp_directory_path() /
        "galaxy_eggbert_big_decor_record_test.vwr";
    Worlds::World world;
    PlaceBigDecor(world, {66, 99, 8, 1});
    world.saveToFile(path);

    const Worlds::World loaded = Worlds::World::loadFromFile(path);
    const auto records = CollectBigDecor(loaded);
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].icon, 66);
    EXPECT_EQ(records[0].x, 99);
    EXPECT_EQ(records[0].y, 8);
    EXPECT_EQ(records[0].z, 1);

    std::error_code error;
    std::filesystem::remove(path, error);
}

TEST(BigDecorRecordTests, RejectsAirIcon) {
    Worlds::World world;
    EXPECT_THROW(PlaceBigDecor(world, {0, 1, 2, 3}), std::invalid_argument);
}

}
