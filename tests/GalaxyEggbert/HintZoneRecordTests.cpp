#include "GalaxyEggbert/HintZoneRecord.hpp"

#include <filesystem>
#include <stdexcept>
#include <system_error>

#include <gtest/gtest.h>

namespace GalaxyEggbert {

TEST(HintZoneRecordTests, PlaceCollectContainsAndOrder) {
    Worlds::World world;
    PlaceHintZone(world, {5, 40, 10, 20, 3, 2, 4});
    PlaceHintZone(world, {1, 10, 1, 10, 2, 2, 2});

    const auto zones = CollectHintZones(world);
    ASSERT_EQ(zones.size(), 2u);
    EXPECT_EQ(zones[0].recordIndex, 1);  // sorted by record index, the tables' scan order
    EXPECT_EQ(zones[1].recordIndex, 5);
    EXPECT_TRUE(zones[1].Contains(42, 11, 23));
    EXPECT_FALSE(zones[1].Contains(43, 10, 20));
    EXPECT_FALSE(zones[1].Contains(40, 12, 20));
}

TEST(HintZoneRecordTests, RejectsEmptyOrOutsideZones) {
    Worlds::World world;
    EXPECT_THROW(PlaceHintZone(world, {0, 1, 1, 1, 0, 1, 1}), std::invalid_argument);
    EXPECT_THROW(PlaceHintZone(world, {0, 98, 1, 1, 3, 1, 1}), std::invalid_argument);
}

TEST(HintZoneRecordTests, RoundTripsThroughWorldFile) {
    const auto path = std::filesystem::temp_directory_path() / "galaxy_eggbert_hint_zone_record_test.vwr";
    Worlds::World world;
    PlaceHintZone(world, {21, 3, 4, 5, 6, 7, 8});
    world.saveToFile(path);

    const auto zones = CollectHintZones(Worlds::World::loadFromFile(path));
    ASSERT_EQ(zones.size(), 1u);
    EXPECT_EQ(zones[0].recordIndex, 21);
    EXPECT_EQ(zones[0].x, 3);
    EXPECT_EQ(zones[0].sizeZ, 8);
    std::error_code error;
    std::filesystem::remove(path, error);
}

}
