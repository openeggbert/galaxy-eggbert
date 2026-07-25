#include "GEPaletteCategories.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        PaletteCategory FreeMenuCategory(const char* name, std::vector<int> selectableIds,
                                         std::vector<int> freeButtonIds)
        {
            selectableIds.resize(freeButtonIds.size(), 0);
            return {name, freeButtonIds.front(), std::move(selectableIds),
                    std::vector<int>(freeButtonIds.size(), 0), std::move(freeButtonIds)};
        }

        PaletteCategory FreeMixedMenuCategory(const char* name, std::vector<int> blockIds,
                                              std::vector<int> objectTypeIds,
                                              std::vector<int> freeButtonIds,
                                              std::vector<int> objectVisualIconIds = {})
        {
            blockIds.resize(freeButtonIds.size(), 0);
            objectTypeIds.resize(freeButtonIds.size(), 0);
            objectVisualIconIds.resize(freeButtonIds.size(), 0);
            return {name, freeButtonIds.front(), std::move(blockIds), std::move(objectTypeIds),
                    std::move(freeButtonIds), {}, std::move(objectVisualIconIds)};
        }
    }

    std::vector<PaletteCategory> ConfirmedBlockCategories()
    {
        using namespace GalaxyEggbert::BlockTypes;
        // Exact Eggbert 2 order. Values come from decdesign.cpp's ten
        // PlaceItemFromMenu* switch statements and dectables.cpp, not from
        // visual guesses. Zero means that original item itself is TODO or
        // has no verified Galaxy representation yet.
        return {
            // Menu 1: most entries are BigDecor backgrounds; only the
            // normal-cell marine plant, support and web map to voxel ids.
            FreeMixedMenuCategory("Scenery",
                                  {0, 0, 0, Marine, 0, 0, 0, 0, 0, 76, 403},
                                  {}, {31, 29, 32, 69, 33, 37, 82, 130, 139, 30, 142}),
            // Menu 2: exact representative tile from each source table.
            FreeMixedMenuCategory("Technical blocks",
                                  {2, 20, 15, 22, 79, 88, 86, 250},
                                  {}, {0, 1, 2, 27, 34, 35, 67, 106}),
            FreeMixedMenuCategory("Rock and terrain",
                                  {153, 154, 185, 284, 337, 247, 339, 341, 157, 91},
                                  {}, {22, 59, 68, 118, 127, 100, 128, 129, 39, 38}),
            FreeMixedMenuCategory("Buildings",
                                  {386, 398, 186, 193, 261, 139, 41, 215, 223, 214}, {},
                                  {137, 138, 65, 66, 112, 58, 23, 80, 81, 79}),
            FreeMixedMenuCategory("Hazards",
                                  {0, 0, 0, Lava, FanLeft, 110, Spike, Drip, Saw, 0,
                                   BlitzEmitter, Crusher},
                                  {2, 3, 96, 0, 0, 0, 0, 0, 0, 40},
                                  {8, 9, 107, 26, 42, 41, 131, 143, 132, 101, 120, 122}),
            FreeMixedMenuCategory("Moving mechanisms", {},
                                  {1, 47, 48, 1, 1, 4, 17, 20, 44, 18, 16, 32, 33},
                                  {21, 20, 19, 28, 121, 16, 55, 60, 113, 140, 54, 95, 99}),
            FreeMixedMenuCategory("Treasures", {},
                                  {5, 6, 26, 25, 30, 29, 31, 200, 55},
                                  {7, 10, 75, 74, 89, 88, 93, 92, 87}),
            FreeMixedMenuCategory("Keys and progression",
                                  {0, Door1, ProgressDoor2, Teleport1, 202, Spring, Temp, Bridge, 0, 0},
                                  {49, 0, 0, 0, 0, 0, 0, 0, 12, 12},
                                  {125, 126, 144, 124, 56, 70, 123, 141, 17, 76},
                                  {0, 0, 0, 0, 0, 0, 0, 0, 0, -1}),
            FreeMixedMenuCategory("Vehicles", {},
                                  {46, 13, 24, 19, 28},
                                  {119, 24, 71, 57, 85}),
            FreeMixedMenuCategory("Characters and goals",
                                  {0, 0, 0, 0, 0, Door1, 0, 0},
                                  {7, 200, 201, 202, 203, 0, 0, 49},
                                  {11, 5, 96, 97, 98, 63, 62, 64}),
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

    PaletteCategory GalaxyBackgroundCategory()
    {
        constexpr int kBackgroundButtonIcon = 77;
        PaletteCategory category;
        category.name = "Background";
        category.buttonIconId = kBackgroundButtonIcon;
        category.buttonIconIds.assign(32, kBackgroundButtonIcon);
        category.iconIds.assign(32, 0);
        category.objectTypeIds.assign(32, 0);
        category.skyRegionIds.reserve(32);
        for (int region = 0; region < 32; ++region)
        {
            category.skyRegionIds.push_back(region);
        }
        return category;
    }

    std::vector<PaletteCategory> ConfirmedObjectCategories()
    {
        return {
            FreeMenuCategory("Scenery", {16, 17, 18, 20, 32, 33, 44, 54, 56, 57, 58},
                             {31, 29, 32, 69, 33, 37, 82, 130, 139, 30, 142}),
            FreeMenuCategory("Technical blocks", {1, 47, 48, 59, 60, 61, 62, 63},
                             {0, 1, 2, 27, 34, 35, 67, 106}),
            FreeMenuCategory("Rock and terrain", {64, 65, 66, 67, 68, 69, 70, 71, 72, 73},
                             {22, 59, 68, 118, 127, 100, 128, 129, 39, 38}),
            FreeMenuCategory("Buildings", {74, 75, 76, 77, 78, 79, 80, 81, 82, 83},
                             {137, 138, 65, 66, 112, 58, 23, 80, 81, 79}),
            FreeMenuCategory("Hazards", {2, 3, 96, 97, 4, 84, 85, 86, 87, 88, 89, 90},
                             {8, 9, 107, 26, 42, 41, 131, 143, 132, 101, 120, 122}),
            FreeMenuCategory("Moving mechanisms", {91, 92, 93, 94, 95, 98, 99, 100, 101, 102, 103, 104, 105},
                             {21, 20, 19, 28, 121, 16, 55, 60, 113, 140, 54, 95, 99}),
            FreeMenuCategory("Treasures", {5, 6, 7, 21, 49, 50, 51, 106, 107},
                             {7, 10, 75, 74, 89, 88, 93, 92, 87}),
            FreeMenuCategory("Keys and progression", {13, 19, 24, 25, 26, 28, 29, 30, 31, 40},
                             {125, 126, 144, 124, 56, 70, 123, 141, 17, 76}),
            FreeMenuCategory("Special world", {46, 55, 108, 109, 110},
                             {119, 24, 71, 57, 85}),
            FreeMenuCategory("Characters", {200, 201, 202, 203, 111, 112, 113, 114},
                             {11, 5, 96, 97, 98, 63, 62, 64}),
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
