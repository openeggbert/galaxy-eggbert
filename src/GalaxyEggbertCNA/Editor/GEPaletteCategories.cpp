#include "GEPaletteCategories.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>

namespace GalaxyEggbert::CNA
{
    std::vector<PaletteCategory> ConfirmedBlockCategories()
    {
        using namespace GalaxyEggbert::BlockTypes;
        return {
            {"Terrain", {RockPile, BrickWall, GoldPillar, Platform, Ground, StoneA, StoneB}},
            {"Hazards", {Lava, Spike, Crusher, Saw, Drip, Blitz, BlitzEmitter,
                         FanLeft, FanRight, FanUp, FanDown}},
            {"Water/Animated", {Water1, Water2, Marine, Temp}},
            {"Mechanisms", {Switch, SwitchOff, SawStopped, Teleport1, Teleport2, Teleport3, Teleport4,
                            Bridge, Spring, Door1, Door2, Door3}},
            {"Progression (hub only)",
             {WorldSelect1, WorldSelect2, WorldSelect3, WorldSelect4, WorldSelect5, WorldSelect6,
              WorldSelect7, WorldSelect8, WorldSelect9, WorldSelect10, WorldSelect11, WorldSelect12,
              ProgressDoor2, ProgressDoor3, ProgressDoor4, ProgressDoor5, ProgressDoor6, ProgressDoor7,
              ProgressDoor8, DemoPortal}},
        };
    }

    std::vector<int> AllBlockIconIdsInOrder()
    {
        std::vector<int> ids;
        ids.reserve(440);
        for (int icon = 1; icon <= 440; ++icon)
        {
            ids.push_back(icon);
        }
        return ids;
    }

    std::vector<PaletteCategory> ConfirmedObjectCategories()
    {
        return {
            {"Platform Lifts", {1, 47, 48}},
            {"Patrol Enemies", {2, 3, 96, 97, 4}},
            {"Patrol Walkers", {16, 17, 18, 20, 32, 33, 44, 54}},
            {"Collectibles", {5, 6, 7, 21, 49, 50, 51}},
            {"Pickups", {13, 19, 24, 25, 26, 28, 29, 30, 31, 40, 46, 55}},
            {"Blupi Skins", {200, 201, 202, 203}},
        };
    }

    std::vector<int> AllObjectTypeIdsInOrder()
    {
        std::vector<int> ids;
        ids.reserve(203);
        for (int type = 1; type <= 203; ++type)
        {
            ids.push_back(type);
        }
        return ids;
    }
}
