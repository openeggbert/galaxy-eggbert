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
}
