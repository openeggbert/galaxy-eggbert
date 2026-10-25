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
