#include "GalaxyEggbert/HubDoors.hpp"

#include "GalaxyEggbert/BlockTypes.hpp"
#include "GalaxyEggbert/Worlds/Block.hpp"

#include <cmath>

#include <gtest/gtest.h>

namespace GalaxyEggbert {

namespace {

void Put(Worlds::World& world, int x, int y, int z, std::uint16_t icon) {
    world.setBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y), static_cast<std::uint16_t>(z),
                   Worlds::Block::make(icon));
}

void Floor(Worlds::World& world, int x0, int z0, int x1, int z1) {
    for (int x = x0; x <= x1; ++x)
        for (int z = z0; z <= z1; ++z)
            Put(world, x, 0, z, BlockTypes::Ground);
}

} // namespace

TEST(HubDoorsTests, MissionDoorsAreOneOrTwoCellsBesideTheirSign) {
    Worlds::World world;
    Floor(world, 30, 30, 70, 70);
    Put(world, 40, 1, 40, BlockTypes::MissionSign1 + 1); // sign 2
    Put(world, 39, 1, 40, BlockTypes::HubDoor);          // one cell to -X
    Put(world, 40, 1, 42, BlockTypes::HubDoor);          // two cells to +Z
    Put(world, 43, 1, 40, BlockTypes::HubDoor);          // three cells away: not its door
    Put(world, 39, 2, 40, BlockTypes::HubDoor);          // other height: not its door

    const auto doors = FindMissionDoors(world, 2);
    ASSERT_EQ(doors.size(), 2u);
    EXPECT_EQ(doors[0].door, (HubCell{39, 1, 40}));
    EXPECT_EQ(doors[1].door, (HubCell{40, 1, 42}));
    EXPECT_EQ(doors[1].sign, (HubCell{40, 1, 40}));
    EXPECT_TRUE(FindMissionDoors(world, 3).empty());
}

TEST(HubDoorsTests, GoldDoorsOpenLowestFirstThenFromPlusX) {
    Worlds::World world;
    Put(world, 40, 2, 40, BlockTypes::GoldDoor);
    Put(world, 41, 1, 40, BlockTypes::GoldDoor);
    Put(world, 45, 1, 40, BlockTypes::GoldDoor);
    const auto doors = FindGoldDoors(world);
    ASSERT_EQ(doors.size(), 3u);
    EXPECT_EQ(doors[0], (HubCell{45, 1, 40}));
    EXPECT_EQ(doors[1], (HubCell{41, 1, 40}));
    EXPECT_EQ(doors[2], (HubCell{40, 2, 40}));
}

// Decor::CheatAction(OpenDoors) -> AdaptDoors(): the cheat opens every hub door, the main hub's
// gold bars and signs as the world hubs' mission doors (owner 2026-11-02: it did nothing).
TEST(HubDoorsTests, AdaptHubDoorsOpensWhatIsUnlockedAndEverythingWithTheCheat) {
    Worlds::World hub;
    Put(hub, 40, 1, 40, BlockTypes::GoldDoor);
    Put(hub, 44, 1, 40, BlockTypes::GoldDoor);
    Put(hub, 48, 1, 40, BlockTypes::GoldDoor);
    Put(hub, 50, 1, 50, BlockTypes::WorldSelect1);     // world 1
    Put(hub, 52, 1, 50, BlockTypes::WorldSelect1 + 1); // world 2
    const auto worldOneDone = [](int index) { return index == 181; };
    auto adapted = AdaptHubDoors(hub, 1, worldOneDone, false);
    ASSERT_EQ(adapted.goldDoors.size(), 1u);
    EXPECT_EQ(adapted.goldDoors[0], (HubCell{48, 1, 40}));
    ASSERT_EQ(adapted.completedSigns.size(), 1u);
    EXPECT_EQ(adapted.completedSigns[0], (HubCell{50, 1, 50}));
    adapted = AdaptHubDoors(hub, 1, worldOneDone, true);
    EXPECT_EQ(adapted.goldDoors.size(), 3u);
    EXPECT_EQ(adapted.completedSigns.size(), 2u);

    Worlds::World worldHub;
    Put(worldHub, 40, 1, 40, BlockTypes::MissionSign1);     // sign 1
    Put(worldHub, 41, 1, 40, BlockTypes::HubDoor);
    Put(worldHub, 50, 1, 40, BlockTypes::MissionSign1 + 2); // sign 3
    Put(worldHub, 50, 1, 42, BlockTypes::HubDoor);
    const auto none = [](int) { return false; };
    EXPECT_TRUE(AdaptHubDoors(worldHub, 20, none, false).missionDoors.empty());
    const auto cheated = AdaptHubDoors(worldHub, 20, none, true);
    ASSERT_EQ(cheated.missionDoors.size(), 2u);
    EXPECT_EQ(cheated.missionDoors[0].door, (HubCell{41, 1, 40}));
    EXPECT_EQ(cheated.missionDoors[1].door, (HubCell{50, 1, 42}));
    const auto missionOne = [](int index) { return index == 21; };
    ASSERT_EQ(AdaptHubDoors(worldHub, 20, missionOne, false).missionDoors.size(), 1u);
    // In a mission AdaptDoors() returns at once: the cheat gives the keys only.
    EXPECT_TRUE(AdaptHubDoors(worldHub, 23, none, true).missionDoors.empty());
}

TEST(HubDoorsTests, WorldSignsAndTheirCompletedIcons) {
    Worlds::World world;
    Put(world, 50, 1, 50, BlockTypes::WorldSelect10 + 2);  // world 12
    Put(world, 52, 1, 50, BlockTypes::WorldDone1 + 2);     // world 3, completed
    EXPECT_EQ(FindWorldSigns(world, 12), (std::vector<HubCell>{{50, 1, 50}}));
    EXPECT_EQ(FindWorldSigns(world, 3), (std::vector<HubCell>{{52, 1, 50}}));
    EXPECT_TRUE(FindWorldSigns(world, 4).empty());

    EXPECT_EQ(CompletedWorldSignIcon(BlockTypes::WorldSelect1), BlockTypes::WorldDone1);
    EXPECT_EQ(CompletedWorldSignIcon(BlockTypes::WorldSelect8), BlockTypes::WorldDone1 + 7);
    EXPECT_EQ(CompletedWorldSignIcon(BlockTypes::WorldSelect9), BlockTypes::WorldDone9);
    EXPECT_EQ(CompletedWorldSignIcon(BlockTypes::WorldSelect10 + 2), BlockTypes::WorldDone10 + 2);
    EXPECT_EQ(CompletedWorldSignIcon(BlockTypes::WorldDone1), BlockTypes::WorldDone1);
    EXPECT_EQ(CompletedWorldSignIcon(BlockTypes::MissionSign1), BlockTypes::MissionSign1);
}

TEST(HubDoorsTests, ArrivalsFaceTheSignOrTheDoor) {
    Worlds::World world;
    Floor(world, 30, 30, 70, 70);
    Put(world, 50, 1, 50, BlockTypes::WorldSelect1);
    Put(world, 49, 1, 50, BlockTypes::Ground); // -X blocked: next try is +X
    const auto besideSign = ArrivalBesideSign(world, {50, 1, 50});
    ASSERT_TRUE(besideSign.has_value());
    EXPECT_EQ(besideSign->cell, (HubCell{51, 1, 50}));
    EXPECT_NEAR(besideSign->yaw, -1.5707963f, 1e-5f); // facing -X, toward the sign

    // Door two cells to -Z of its sign: Blupi starts one further out, facing +Z.
    const auto pastDoor = ArrivalPastDoor(world, {60, 1, 48}, {60, 1, 50});
    ASSERT_TRUE(pastDoor.has_value());
    EXPECT_EQ(pastDoor->cell, (HubCell{60, 1, 47}));
    EXPECT_NEAR(std::fabs(pastDoor->yaw), 3.1415927f, 1e-5f);
    EXPECT_FALSE(ArrivalPastDoor(world, {60, 1, 48}, {61, 1, 50}).has_value()); // not on one axis
}

} // namespace GalaxyEggbert
