#include "GalaxyEggbert/MoveObjectRecord.hpp"

#include <algorithm>
#include <filesystem>
#include <system_error>

#include <gtest/gtest.h>

namespace GalaxyEggbert {

TEST(MoveObjectRecordTests, PlaceAndCollectRoundTripsAllFields) {
    Worlds::World world;

    MoveObjectRecord egg;
    egg.type = GalaxyEggbert::Def::ObjectType::ObjectType6;
    egg.posStartX = 12.0f;
    egg.posStartY = 3.0f;
    egg.posStartZ = 45.0f;
    egg.posEndX = 12.0f;
    egg.posEndY = 3.0f;
    egg.posEndZ = 45.0f;
    egg.speed = 1.5f;
    PlaceMoveObject(world, egg);

    MoveObjectRecord lift;
    lift.type = GalaxyEggbert::Def::ObjectType::ObjectType1;
    lift.visualIcon = 143;
    lift.posStartX = 60.5f;
    lift.posStartY = 2.25f;
    lift.posStartZ = 10.75f;
    lift.posEndX = 60.5f;
    lift.posEndY = 8.0f;
    lift.posEndZ = 10.75f;
    lift.speed = 2.0f;
    lift.stepAdvanceTicks = 25.0f;
    lift.stepRecedeTicks = 30.0f;
    lift.timeStopStartTicks = 15.0f;
    lift.timeStopEndTicks = 20.0f;
    PlaceMoveObject(world, lift);

    const auto collected = CollectMoveObjects(world);
    ASSERT_EQ(collected.size(), 2u);

    const bool hasEgg = std::any_of(collected.begin(), collected.end(), [](const MoveObjectRecord& r) {
        return r.type == GalaxyEggbert::Def::ObjectType::ObjectType6 && r.posStartX == 12.0f && r.posStartY == 3.0f
            && r.posStartZ == 45.0f && r.posEndX == 12.0f && r.posEndY == 3.0f && r.posEndZ == 45.0f
            && r.speed == 1.5f;
    });
    const bool hasLift = std::any_of(collected.begin(), collected.end(), [](const MoveObjectRecord& r) {
        return r.type == GalaxyEggbert::Def::ObjectType::ObjectType1 && r.posStartX == 60.5f && r.posStartY == 2.25f
            && r.posStartZ == 10.75f && r.posEndX == 60.5f && r.posEndY == 8.0f && r.posEndZ == 10.75f
            && r.speed == 2.0f && r.stepAdvanceTicks == 25.0f && r.stepRecedeTicks == 30.0f
            && r.timeStopStartTicks == 15.0f && r.timeStopEndTicks == 20.0f
            && r.visualIcon == 143;
    });
    EXPECT_TRUE(hasEgg);
    EXPECT_TRUE(hasLift);
}

TEST(MoveObjectRecordTests, RoundTripsThroughSaveAndLoadFile) {
    const auto filePath = std::filesystem::temp_directory_path()
        / "galaxy_eggbert_move_object_record_test.vwr";

    {
        Worlds::World world;
        MoveObjectRecord record;
        record.type = GalaxyEggbert::Def::ObjectType::ObjectType12;
        record.visualIcon = 79;
        record.posStartX = 33.0f;
        record.posStartY = 0.0f;
        record.posStartZ = 33.0f;
        record.posEndX = 33.0f;
        record.posEndY = 0.0f;
        record.posEndZ = 33.0f;
        record.speed = 1.5f;
        PlaceMoveObject(world, record);
        world.saveToFile(filePath);
    }

    const Worlds::World loaded = Worlds::World::loadFromFile(filePath);
    const auto collected = CollectMoveObjects(loaded);
    ASSERT_EQ(collected.size(), 1u);
    EXPECT_EQ(collected[0].type, GalaxyEggbert::Def::ObjectType::ObjectType12);
    EXPECT_EQ(collected[0].visualIcon, 79);
    EXPECT_EQ(collected[0].posStartX, 33.0f);
    EXPECT_EQ(collected[0].posStartZ, 33.0f);
    EXPECT_EQ(collected[0].speed, 1.5f);

    std::error_code removeError;
    std::filesystem::remove(filePath, removeError);
}

TEST(MoveObjectRecordTests, ReadsLegacyPayloadWithoutVisualOverride) {
    Worlds::World world;
    std::vector<std::uint8_t> legacyPayload(45, 0);
    legacyPayload[0] = static_cast<std::uint8_t>(GalaxyEggbert::Def::ObjectType::ObjectType6);
    world.setBlockExtraMetadata(4, 5, 6, kMoveObjectMetadataType, legacyPayload);

    const auto collected = CollectMoveObjects(world);
    ASSERT_EQ(collected.size(), 1u);
    EXPECT_EQ(collected[0].type, GalaxyEggbert::Def::ObjectType::ObjectType6);
    EXPECT_EQ(collected[0].visualIcon, 0);
}

TEST(MoveObjectRecordTests, CollectReturnsEmptyForWorldWithNoMoveObjects) {
    const Worlds::World world;
    EXPECT_TRUE(CollectMoveObjects(world).empty());
}

TEST(MoveObjectRecordTests, RemoveMoveObjectRemovesTheAnchoredRecord) {
    Worlds::World world;
    MoveObjectRecord record;
    record.type = GalaxyEggbert::Def::ObjectType::ObjectType44;
    record.posStartX = 12.0f;
    record.posStartY = 3.0f;
    record.posStartZ = 20.0f;
    record.posEndX = 12.0f;
    record.posEndY = 3.0f;
    record.posEndZ = 20.0f;
    PlaceMoveObject(world, record);
    ASSERT_EQ(CollectMoveObjects(world).size(), 1u);

    const bool removed = RemoveMoveObject(world, 12, 3, 20);
    EXPECT_TRUE(removed);
    EXPECT_TRUE(CollectMoveObjects(world).empty());
}

TEST(MoveObjectRecordTests, RemoveMoveObjectReturnsFalseWhenNothingIsAnchoredThere) {
    Worlds::World world;
    EXPECT_FALSE(RemoveMoveObject(world, 5, 5, 5));
}

}
