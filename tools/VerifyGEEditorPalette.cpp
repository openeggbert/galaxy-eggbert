#include "Editor/GEEditorPalette.hpp"
#include "Editor/GEEditorPaletteLayout.hpp"
#include "Editor/GEPaletteCategories.hpp"
#include "Editor/GEWorldEditor.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/BigDecorRecord.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <cstdio>
#include <iostream>
#include <set>

int main()
{
    using namespace GalaxyEggbert::CNA;
    using GalaxyEggbert::Worlds::World;
    using Microsoft::Xna::Framework::Input::ButtonState;
    using Microsoft::Xna::Framework::Input::KeyboardState;
    using Microsoft::Xna::Framework::Input::Keys;
    using Microsoft::Xna::Framework::Input::MouseState;

    bool allOk = true;
    const auto check = [&allOk](bool condition, const char* message)
    {
        std::cout << (condition ? "PASS" : "FAIL") << ": " << message << std::endl;
        if (!condition) allOk = false;
    };

    const auto allBlockIds = AllBlockIconIdsInOrder();
    check(allBlockIds.size() == 440,
          "AllBlockIconIdsInOrder() covers all 440 valid non-Air block types");
    check(allBlockIds.front() == 1 && allBlockIds.back() == 440,
          "AllBlockIconIdsInOrder() is in order from 1 through 440");

    const auto sourceCategories = ConfirmedBlockCategories();
    check(sourceCategories.size() ==
              static_cast<std::size_t>(GEEditorPaletteLayout::GalaxyBackgroundCategoryIndex),
          "the background group follows all ten Eggbert 2 source groups");
    bool blocksInRange = true;
    int curatedBlockCount = 0;
    for (const auto& category : sourceCategories)
    {
        check(!category.name.empty(), "every curated category has a non-empty name");
        curatedBlockCount += static_cast<int>(category.buttonIconIds.size());
        for (const int blockId : category.iconIds)
        {
            if (blockId != 0 && (blockId < 1 || blockId > 440))
            {
                blocksInRange = false;
            }
        }
    }
    check(blocksInRange, "every curated category's block ids are valid");
    check(curatedBlockCount > 0, "at least one curated block entry exists");
    const auto backgroundCategory = GalaxyBackgroundCategory();
    check(backgroundCategory.skyRegionIds.size() == 32 &&
              backgroundCategory.skyRegionIds.front() == 0 &&
              backgroundCategory.skyRegionIds.back() == 31,
          "the Galaxy-only background group exposes all 32 sky regions in order");
    check(sourceCategories[4].objectTypeIds[9] == 40,
          "the Eggbert Inverter entry maps to Galaxy's invert pickup");
    check(sourceCategories[7].objectTypeIds[8] == 12,
          "the Eggbert Wooden case entry maps to Galaxy's pushable crate");
    check(sourceCategories[8].objectTypeIds[0] == 46,
          "the Eggbert Hovercraft entry maps to Galaxy's Overcraft");
    check(sourceCategories[2].iconIds[3] == 284,
          "the Eggbert Cave entry maps to the named Cave_1 atlas tile");
    const std::vector<int> expectedBuildingBlocks = {
        386, 398, 186, 193, 261, 139, 41, 215, 223, 214,
    };
    check(sourceCategories[3].iconIds == expectedBuildingBlocks,
          "all ten Buildings entries map to their named first atlas tiles");
    check(std::vector<int>(
              sourceCategories[4].objectTypeIds.begin(),
              sourceCategories[4].objectTypeIds.begin() + 3) ==
              std::vector<int>{2, 3, 96},
          "Bomb, Hanging bomb, and Homing bomb use their exact Eggbert 2 object types");
    check(sourceCategories[5].objectTypeIds[10] == 16,
          "Moving bomb uses Eggbert 2's TYPE_BOMBEMOVE object type");
    check(sourceCategories[6].objectTypeIds[7] == 200,
          "Personal bomb uses Eggbert 2's first personal-bomb variant");
    check(sourceCategories[7].objectTypeIds[9] == 12 &&
              sourceCategories[7].objectVisualIconIds[9] == -1,
          "Secret wooden case retains the crate behavior and current-terrain camouflage profile");
    check(sourceCategories[9].spawnPointIds[6] == 1,
          "Level start is represented by the dedicated world spawn tool");
    const std::vector<int> expectedBigDecor = {
        20, 16, 23, 0, 26, 28, 45, 66, 87, 0, 0,
    };
    check(sourceCategories[0].bigDecorIconIds == expectedBigDecor,
          "all eight Scenery billboards use exact Eggbert 2 BigDecor table representatives");
    int sourceEntryCount = 0;
    bool everySourceEntryHasExactlyOneImplementation = true;
    for (const auto& category : sourceCategories)
    {
        const std::size_t count = category.buttonIconIds.size();
        sourceEntryCount += static_cast<int>(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            const auto valueAt = [index](const std::vector<int>& values)
            {
                return index < values.size() ? values[index] : 0;
            };
            const int implementationCount =
                (valueAt(category.iconIds) > 0 ? 1 : 0) +
                (valueAt(category.objectTypeIds) > 0 ? 1 : 0) +
                (valueAt(category.spawnPointIds) > 0 ? 1 : 0) +
                (valueAt(category.bigDecorIconIds) > 0 ? 1 : 0);
            everySourceEntryHasExactlyOneImplementation &=
                implementationCount == 1;
        }
    }
    check(sourceEntryCount == 96,
          "the exhaustive source-menu inventory contains all 96 Eggbert 2 entries");
    check(everySourceEntryHasExactlyOneImplementation,
          "every source-menu entry has exactly one block, object, spawn, or BigDecor implementation");

    constexpr int kViewportWidth = 800;
    constexpr int kViewportHeight = 480;
    const MouseState restMouse(
        0, 0, 0, ButtonState::Released, ButtonState::Released,
        ButtonState::Released, ButtonState::Released, ButtonState::Released);

    {
        const GEEditorPaletteLayout layout;
        check(layout.CategoryButtonRect(0).x0 == 10.0f,
              "the source category rail starts at the expected left edge");
        const auto backgroundButton = layout.CategoryButtonRect(
            GEEditorPaletteLayout::GalaxyBackgroundCategoryIndex);
        check(backgroundButton.x0 == layout.DeleteToolRect().x1 + 2.0f &&
                  backgroundButton.y0 == layout.DeleteToolRect().y0,
              "the Galaxy background group leaves all ten source-group positions unchanged");
        check(layout.PlayTestRect(kViewportWidth, kViewportHeight).x1 <=
                  layout.StopRect(kViewportWidth, kViewportHeight).x0,
              "play-test and stop occupy separate visible bottom-right buttons");

        check(layout.PlacementButtonRect(6, 360, 480).y0 >
                  layout.PlacementButtonRect(0, 360, 480).y0,
              "narrow screens wrap the placement action onto another row");
        check(layout.PaletteCellRect(7, 8, 7, 360, 480).y1 <= 480.0f,
              "the complete eight-entry source group remains visible on a narrow screen");
        const auto lastNarrowBackground = layout.PaletteCellRect(
            31, 32, GEEditorPaletteLayout::GalaxyBackgroundCategoryIndex, 360, 480);
        check(lastNarrowBackground.x0 >= backgroundButton.x1 &&
                  lastNarrowBackground.x1 <= 360.0f &&
                  lastNarrowBackground.y1 <= 480.0f,
              "all 32 background choices remain visible on a narrow screen");

        const auto wideCoordinates =
            layout.PlacementCoordinatesRect(120.0f, 16.0f, 800, 480);
        check(wideCoordinates.x0 >=
                  layout.PlacementButtonRect(6, 800, 480).x1 &&
                  wideCoordinates.x1 <= layout.PlayTestRect(800, 480).x0,
              "placement coordinates sit beside the XYZ and PLACE controls");
        const auto narrowCoordinates =
            layout.PlacementCoordinatesRect(120.0f, 16.0f, 360, 480);
        check(narrowCoordinates.x0 >= 0.0f &&
                  narrowCoordinates.x1 <= 360.0f &&
                  narrowCoordinates.y1 <=
                      layout.PlacementButtonRect(0, 360, 480).y0,
              "placement coordinates remain visible above wrapped controls");
        const auto coordinateBackground =
            layout.PlacementCoordinatesBackgroundRect(narrowCoordinates, 360, 480);
        check(coordinateBackground.x0 < narrowCoordinates.x0 &&
                  coordinateBackground.y0 < narrowCoordinates.y0 &&
                  coordinateBackground.x1 > narrowCoordinates.x1 &&
                  coordinateBackground.y1 > narrowCoordinates.y1,
              "placement coordinates have a padded background");
        const GEQuadBatch::Rect edgeCoordinates = {1.0f, 1.0f, 359.0f, 479.0f};
        const auto edgeBackground =
            layout.PlacementCoordinatesBackgroundRect(edgeCoordinates, 360, 480);
        check(edgeBackground.x0 == 0.0f && edgeBackground.y0 == 0.0f &&
                  edgeBackground.x1 == 360.0f && edgeBackground.y1 == 480.0f,
              "the coordinate background remains clipped to the viewport");
    }

    const auto click = [](GEEditorPalette& palette, float x, float y)
    {
        const MouseState down(
            static_cast<int>(x), static_cast<int>(y), 0,
            ButtonState::Pressed, ButtonState::Released, ButtonState::Released,
            ButtonState::Released, ButtonState::Released);
        const MouseState up(
            static_cast<int>(x), static_cast<int>(y), 0,
            ButtonState::Released, ButtonState::Released, ButtonState::Released,
            ButtonState::Released, ButtonState::Released);
        (void)palette.Update(down, kViewportWidth, kViewportHeight);
        return palette.Update(up, kViewportWidth, kViewportHeight);
    };

    {
        const GEEditorPaletteLayout layout;
        bool everyPointerSelectionMatches = true;
        bool anySourceNotice = false;
        for (std::size_t categoryIndex = 0;
             categoryIndex < sourceCategories.size(); ++categoryIndex)
        {
            const auto& category = sourceCategories[categoryIndex];
            for (std::size_t itemIndex = 0;
                 itemIndex < category.buttonIconIds.size(); ++itemIndex)
            {
                GEEditorPalette palette;
                const auto categoryRect =
                    layout.CategoryButtonRect(static_cast<int>(categoryIndex));
                (void)click(
                    palette, (categoryRect.x0 + categoryRect.x1) * 0.5f,
                    (categoryRect.y0 + categoryRect.y1) * 0.5f);
                const auto itemRect = layout.PaletteCellRect(
                    static_cast<int>(itemIndex),
                    static_cast<int>(category.buttonIconIds.size()),
                    static_cast<int>(categoryIndex),
                    kViewportWidth, kViewportHeight);
                const auto selected = click(
                    palette, (itemRect.x0 + itemRect.x1) * 0.5f,
                    (itemRect.y0 + itemRect.y1) * 0.5f);

                const auto valueAt = [itemIndex](const std::vector<int>& values)
                {
                    return itemIndex < values.size() ? values[itemIndex] : 0;
                };
                const int block = valueAt(category.iconIds);
                const int object = valueAt(category.objectTypeIds);
                const int spawn = valueAt(category.spawnPointIds);
                const int bigDecor = valueAt(category.bigDecorIconIds);
                bool matches = selected.clickConsumed;
                if (block > 0)
                {
                    matches &= palette.SelectedPlacementKind() ==
                            GEEditorPalette::PlacementKind::Block &&
                        palette.SelectedBlockType() ==
                            static_cast<std::uint16_t>(block);
                }
                else if (object > 0)
                {
                    matches &= palette.SelectedPlacementKind() ==
                            GEEditorPalette::PlacementKind::Object &&
                        palette.SelectedObjectType() ==
                            GalaxyEggbert::ToObjectType(object);
                }
                else if (spawn > 0)
                {
                    matches &= palette.SelectedPlacementKind() ==
                        GEEditorPalette::PlacementKind::SpawnPoint;
                }
                else if (bigDecor > 0)
                {
                    matches &= palette.SelectedPlacementKind() ==
                            GEEditorPalette::PlacementKind::BigDecor &&
                        palette.SelectedBigDecorIcon() ==
                            static_cast<std::uint16_t>(bigDecor);
                }
                everyPointerSelectionMatches &= matches;
                anySourceNotice |= palette.IsNotYetImplementedNoticeVisible();
            }
        }
        check(everyPointerSelectionMatches,
              "real pointer clicks select the exact implementation for all 96 source-menu cells");
        check(!anySourceNotice,
              "Not yet implemented is unreachable from every Eggbert 2 source-menu cell");
    }

    {
        GEEditorPalette palette;
        const auto openBackground = click(palette, 72.0f, 30.0f);
        check(openBackground.clickConsumed,
              "the Galaxy background representative consumes its click");
        const auto region7 = click(palette, 408.0f, 30.0f);
        check(region7.action == GEEditorPalette::Action::SelectSkyRegion &&
                  region7.skyRegion == 7,
              "a background thumbnail reports its exact sky-region id");
        const auto region31 = click(palette, 744.0f, 72.0f);
        check(region31.action == GEEditorPalette::Action::SelectSkyRegion &&
                  region31.skyRegion == 31,
              "the background popup stays open for immediate visual comparison");
    }

    {
        GEEditorPalette palette;
        check(palette.SelectedBlockType() == GalaxyEggbert::BlockTypes::RockPile,
              "the palette starts with RockPile selected");
        const auto erase = click(palette, 30.0f, 30.0f);
        check(erase.action == GEEditorPalette::Action::DeleteAtTarget &&
                  erase.clickConsumed,
              "the visible delete glyph reports a consumed removal action");
        check(!palette.IsNotYetImplementedNoticeVisible(),
              "the functional delete glyph no longer raises the not-implemented notice");
        const auto hiddenArea = click(palette, 466.0f, 26.0f);
        check(hiddenArea.action == GEEditorPalette::Action::None && !hiddenArea.clickConsumed,
              "the former invisible toolbar area no longer consumes clicks");
        const auto emptyArea = click(palette, 400.0f, 200.0f);
        check(emptyArea.action == GEEditorPalette::Action::None && !emptyArea.clickConsumed,
              "empty world-view space is not consumed");
    }

    {
        GEEditorPalette palette;
        constexpr float y = 451.0f;
        check(click(palette, 235.0f, y).action == GEEditorPalette::Action::PlacementXMinus,
              "X- reports PlacementXMinus");
        check(click(palette, 277.0f, y).action == GEEditorPalette::Action::PlacementXPlus,
              "X+ reports PlacementXPlus");
        check(click(palette, 319.0f, y).action == GEEditorPalette::Action::PlacementYMinus,
              "Y- reports PlacementYMinus");
        check(click(palette, 361.0f, y).action == GEEditorPalette::Action::PlacementYPlus,
              "Y+ reports PlacementYPlus");
        check(click(palette, 403.0f, y).action == GEEditorPalette::Action::PlacementZMinus,
              "Z- reports PlacementZMinus");
        check(click(palette, 445.0f, y).action == GEEditorPalette::Action::PlacementZPlus,
              "Z+ reports PlacementZPlus");
        const auto place = click(palette, 499.0f, y);
        check(place.action == GEEditorPalette::Action::PlaceSelection,
              "PLACE reports PlaceSelection");
        check(place.clickConsumed, "PLACE consumes its own pointer click");
    }

    {
        GEEditorPalette palette;
        const auto open = click(palette, 30.0f, 72.0f);
        check(open.clickConsumed, "a category representative consumes its click");
        check(palette.SelectedBlockType() == GalaxyEggbert::BlockTypes::RockPile,
              "opening a category does not alter selection");
        const auto first = click(palette, 72.0f, 72.0f);
        check(first.clickConsumed, "an expanded category entry consumes its click");
        check(palette.SelectedBlockType() == GalaxyEggbert::BlockTypes::RockPile,
              "selecting BigDecor does not alias it to a voxel block");
        check(palette.IsBigDecorMode(),
              "the first Scenery entry selects the Tree BigDecor tool");
        check(!palette.IsNotYetImplementedNoticeVisible(),
              "the implemented Tree entry never starts the temporary notice");
    }

    const auto allObjectTypes = AllObjectTypeIdsInOrder();
    check(allObjectTypes.size() == 203,
          "AllObjectTypeIdsInOrder() covers all 203 placeable object types");
    check(allObjectTypes.front() == 1 && allObjectTypes.back() == 203,
          "AllObjectTypeIdsInOrder() is ordered from 1 through 203");
    bool objectsInRange = true;
    bool noDuplicates = true;
    int curatedObjectCount = 0;
    std::set<int> seenObjectTypes;
    for (const auto& category : ConfirmedObjectCategories())
    {
        check(!category.name.empty(), "every curated object category has a name");
        curatedObjectCount += static_cast<int>(category.buttonIconIds.size());
        for (const int typeId : category.iconIds)
        {
            if (typeId == 0) continue;
            objectsInRange = objectsInRange && typeId >= 1 && typeId <= 203;
            noDuplicates = noDuplicates && seenObjectTypes.insert(typeId).second;
        }
    }
    check(objectsInRange, "every curated object type is valid");
    check(curatedObjectCount > 0, "at least one curated object entry exists");
    check(noDuplicates, "no object type is duplicated across curated categories");
    const auto objectCategories = ConfirmedObjectCategories();
    check(objectCategories[1].name == "Technical blocks" &&
              std::vector<int>(
                  objectCategories[1].buttonIconIds.begin(),
                  objectCategories[1].buttonIconIds.begin() + 3) == std::vector<int>{0, 1, 2},
          "the Technical blocks glyphs preserve Eggbert 2 order");
    check(seenObjectTypes.count(6) == 1, "the extra-life egg remains placeable");
    check(seenObjectTypes.count(39) == 0, "the sparkle effect is not placeable");
    check(seenObjectTypes.count(8) == 0, "transient explosions are not placeable");

    {
        GEEditorPalette palette;
        check(!palette.IsObjectMode(), "the palette starts in Blocks mode");
        check(palette.SelectedObjectType() == GalaxyEggbert::ObjectType::ObjectType6,
              "the default object selection remains the extra-life egg");
        check(click(palette, 30.0f, 324.0f).clickConsumed,
              "the Treasures category consumes its click");
        check(click(palette, 72.0f, 324.0f).clickConsumed,
              "the chest entry consumes its click");
        check(palette.SelectedObjectType() == GalaxyEggbert::ObjectType::ObjectType5,
              "the chest glyph selects the treasure object");
        check(palette.IsObjectMode(), "selecting an object enters Objects mode");
        check(palette.SelectedBlockType() == GalaxyEggbert::BlockTypes::RockPile,
              "object selection preserves the block selection");

        (void)click(palette, 30.0f, 114.0f);
        check(click(palette, 72.0f, 114.0f).clickConsumed,
              "a Technical block entry consumes its click");
        check(!palette.IsObjectMode(), "selecting a block returns to Blocks mode");
        check(palette.SelectedBlockType() == 2,
              "the first Technical block selects verified block type 2");
        check(palette.SelectedObjectType() == GalaxyEggbert::ObjectType::ObjectType5,
              "block selection preserves the object selection");
    }

    {
        GEEditorPalette palette;
        (void)click(palette, 30.0f, 240.0f);
        const auto inverter = click(palette, 450.0f, 240.0f);
        check(inverter.clickConsumed &&
                  palette.SelectedObjectType() == GalaxyEggbert::ObjectType::ObjectType40 &&
                  palette.IsObjectMode(),
              "the Inverter glyph selects the functional invert object");
        check(!palette.IsNotYetImplementedNoticeVisible(),
              "the mapped Inverter glyph does not raise the temporary notice");
    }

    {
        GEEditorPalette palette;
        (void)click(palette, 30.0f, 366.0f);
        const auto woodenCase = click(palette, 408.0f, 366.0f);
        check(woodenCase.clickConsumed &&
                  palette.SelectedObjectType() == GalaxyEggbert::ObjectType::ObjectType12 &&
                  palette.IsObjectMode(),
              "the Wooden case glyph selects the functional pushable crate");
        check(!palette.IsNotYetImplementedNoticeVisible(),
              "the mapped Wooden case glyph does not raise the temporary notice");
    }

    {
        GEEditorPalette palette;
        (void)click(palette, 30.0f, 408.0f);
        const auto hovercraft = click(palette, 72.0f, 408.0f);
        check(hovercraft.clickConsumed &&
                  palette.SelectedObjectType() == GalaxyEggbert::ObjectType::ObjectType46 &&
                  palette.IsObjectMode(),
              "the Hovercraft glyph selects Galaxy's functional Overcraft");
        check(!palette.IsNotYetImplementedNoticeVisible(),
              "the mapped Hovercraft glyph does not raise the temporary notice");
    }

    {
        GEEditorPalette palette;
        (void)click(palette, 30.0f, 156.0f);
        const auto cave = click(palette, 198.0f, 156.0f);
        check(cave.clickConsumed && palette.SelectedBlockType() == 284 &&
                  !palette.IsObjectMode(),
              "the Cave glyph selects the named Cave_1 block");
        check(!palette.IsNotYetImplementedNoticeVisible(),
              "the mapped Cave glyph does not raise the temporary notice");
    }

    for (std::size_t index = 0; index < expectedBuildingBlocks.size(); ++index)
    {
        GEEditorPalette palette;
        (void)click(palette, 30.0f, 198.0f);
        const auto building = click(
            palette, 72.0f + static_cast<float>(index) * 42.0f, 198.0f);
        check(building.clickConsumed &&
                  palette.SelectedBlockType() ==
                      static_cast<std::uint16_t>(expectedBuildingBlocks[index]) &&
                  !palette.IsObjectMode(),
              "a Buildings glyph selects its exact named atlas block");
        check(!palette.IsNotYetImplementedNoticeVisible(),
              "a mapped Buildings glyph does not raise the temporary notice");
    }

    {
        constexpr struct
        {
            float categoryY;
            int itemIndex;
            GalaxyEggbert::ObjectType expectedType;
            const char* message;
        } bombEntries[] = {
            {240.0f, 0, GalaxyEggbert::ObjectType::ObjectType2,
             "Bomb selects TYPE_BOMBEDOWN"},
            {240.0f, 1, GalaxyEggbert::ObjectType::ObjectType3,
             "Hanging bomb selects TYPE_BOMBEUP"},
            {240.0f, 2, GalaxyEggbert::ObjectType::ObjectType96,
             "Homing bomb selects TYPE_BOMBEFOLLOW1"},
            {282.0f, 10, GalaxyEggbert::ObjectType::ObjectType16,
             "Moving bomb selects TYPE_BOMBEMOVE"},
            {324.0f, 7, GalaxyEggbert::ObjectType::ObjectType200,
             "Personal bomb selects TYPE_BOMBEPERSO1"},
        };
        for (const auto& entry : bombEntries)
        {
            GEEditorPalette palette;
            (void)click(palette, 30.0f, entry.categoryY);
            const auto selected = click(
                palette, 72.0f + static_cast<float>(entry.itemIndex) * 42.0f,
                entry.categoryY);
            check(selected.clickConsumed &&
                      palette.SelectedObjectType() == entry.expectedType &&
                      palette.SelectedObjectVisualIcon() == 0 &&
                      palette.IsObjectMode(),
                  entry.message);
            check(!palette.IsNotYetImplementedNoticeVisible(),
                  "an implemented bomb entry does not raise the temporary notice");
        }
    }

    {
        GEEditorPalette palette;
        (void)click(palette, 30.0f, 366.0f);
        const auto secretCase = click(palette, 450.0f, 366.0f);
        check(secretCase.clickConsumed &&
                  palette.SelectedObjectType() == GalaxyEggbert::ObjectType::ObjectType12 &&
                  palette.SelectedObjectVisualIcon() ==
                      GalaxyEggbert::BlockTypes::RockPile &&
                  palette.IsObjectMode(),
              "Secret wooden case selects a crate camouflaged as the current terrain icon");
        check(!palette.IsNotYetImplementedNoticeVisible(),
              "the implemented Secret wooden case does not raise the temporary notice");
    }

    {
        GEEditorPalette palette;
        (void)click(palette, 30.0f, 450.0f);
        const auto levelStart = click(palette, 324.0f, 450.0f);
        check(levelStart.clickConsumed && palette.IsSpawnPointMode() &&
                  !palette.IsObjectMode(),
              "Level start selects the dedicated spawn-point placement mode");
        check(!palette.IsNotYetImplementedNoticeVisible(),
              "the implemented Level start does not raise the temporary notice");
    }

    for (const int index : {0, 1, 2, 4, 5, 6, 7, 8})
    {
        GEEditorPalette palette;
        (void)click(palette, 30.0f, 72.0f);
        const auto scenery = click(
            palette, 72.0f + static_cast<float>(index) * 42.0f, 72.0f);
        check(scenery.clickConsumed && palette.IsBigDecorMode() &&
                  palette.SelectedBigDecorIcon() ==
                      static_cast<std::uint16_t>(expectedBigDecor[index]),
              "a BigDecor Scenery glyph selects its exact non-colliding billboard");
        check(!palette.IsNotYetImplementedNoticeVisible(),
              "an implemented BigDecor entry does not raise the temporary notice");
    }

    {
        constexpr const char* kBackgroundSavePath =
            "verify_ge_editor_palette_background.vwr";
        World world;
        world.setSkyRegion(3);
        GEWorldEditor editor;
        editor.SetWorldPath(kBackgroundSavePath);
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        Easy3D::Camera3D camera;
        const auto tap = [&](int x, int y)
        {
            const MouseState down(
                x, y, 0, ButtonState::Pressed, ButtonState::Released,
                ButtonState::Released, ButtonState::Released, ButtonState::Released);
            const MouseState up(
                x, y, 0, ButtonState::Released, ButtonState::Released,
                ButtonState::Released, ButtonState::Released, ButtonState::Released);
            editor.Update(KeyboardState{}, down, 0.0f, 800, 480, camera, world);
            editor.Update(KeyboardState{}, up, 0.0f, 800, 480, camera, world);
        };
        const auto pressKey = [&](Keys key)
        {
            editor.Update(KeyboardState{key}, restMouse, 0.0f, 800, 480, camera, world);
            editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, world);
        };

        tap(72, 30);
        tap(366, 30);
        check(world.skyRegion() == 6,
              "the seventh thumbnail applies sky region 6 to the live world");
        check(editor.ConsumeNeedsPresentationRebuild(),
              "a thumbnail selection immediately rebuilds the background presentation");
        pressKey(Keys::U);
        check(world.skyRegion() == 3,
              "undo restores the background from before the thumbnail selection");
        pressKey(Keys::R);
        check(world.skyRegion() == 6,
              "redo restores the background thumbnail selection");
        pressKey(Keys::Enter);
        check(World::loadFromFile(kBackgroundSavePath).skyRegion() == 6,
              "saving persists the background chosen from the menu");
        std::remove(kBackgroundSavePath);
    }

    {
        World world;
        GEWorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        Easy3D::Camera3D camera;
        const auto tap = [&](int x, int y)
        {
            const MouseState down(
                x, y, 0, ButtonState::Pressed, ButtonState::Released,
                ButtonState::Released, ButtonState::Released, ButtonState::Released);
            const MouseState up(
                x, y, 0, ButtonState::Released, ButtonState::Released,
                ButtonState::Released, ButtonState::Released, ButtonState::Released);
            editor.Update(KeyboardState{}, down, 0.0f, 800, 480, camera, world);
            editor.Update(KeyboardState{}, up, 0.0f, 800, 480, camera, world);
        };
        tap(277, 451);
        check(world.getBlock(50, 0, 23).isAir() && world.getBlock(51, 0, 23).isAir(),
              "an axis tap only moves the preview");
        tap(499, 451);
        check(world.getBlock(50, 0, 23).isAir(),
              "PLACE does not use the unshifted target");
        check(world.getBlock(51, 0, 23).type() == GalaxyEggbert::BlockTypes::RockPile,
              "X+ followed by PLACE uses the moved preview cell");
        check(editor.ConsumeNeedsPresentationRebuild(),
              "PLACE requests a presentation rebuild");
    }

    {
        World world;
        GEWorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        Easy3D::Camera3D camera;
        const auto tap = [&](int x, int y)
        {
            const MouseState down(
                x, y, 0, ButtonState::Pressed, ButtonState::Released,
                ButtonState::Released, ButtonState::Released, ButtonState::Released);
            const MouseState up(
                x, y, 0, ButtonState::Released, ButtonState::Released,
                ButtonState::Released, ButtonState::Released, ButtonState::Released);
            editor.Update(KeyboardState{}, down, 0.0f, 800, 480, camera, world);
            editor.Update(KeyboardState{}, up, 0.0f, 800, 480, camera, world);
        };
        tap(30, 366);
        tap(450, 366);
        tap(499, 451);
        const auto objects = GalaxyEggbert::CollectMoveObjects(world);
        check(objects.size() == 1 &&
                  objects[0].type == GalaxyEggbert::ObjectType::ObjectType12 &&
                  objects[0].visualIcon == GalaxyEggbert::BlockTypes::RockPile,
              "placing Secret wooden case persists its exact camouflage variant");
    }

    {
        constexpr const char* kSpawnSavePath =
            "verify_ge_editor_palette_spawn.vwr";
        World world;
        GEWorldEditor editor;
        editor.SetWorldPath(kSpawnSavePath);
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        Easy3D::Camera3D camera;
        const auto tap = [&](int x, int y)
        {
            const MouseState down(
                x, y, 0, ButtonState::Pressed, ButtonState::Released,
                ButtonState::Released, ButtonState::Released, ButtonState::Released);
            const MouseState up(
                x, y, 0, ButtonState::Released, ButtonState::Released,
                ButtonState::Released, ButtonState::Released, ButtonState::Released);
            editor.Update(KeyboardState{}, down, 0.0f, 800, 480, camera, world);
            editor.Update(KeyboardState{}, up, 0.0f, 800, 480, camera, world);
        };
        const auto pressKey = [&](Keys key)
        {
            editor.Update(KeyboardState{key}, restMouse, 0.0f, 800, 480, camera, world);
            editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, world);
        };

        tap(30, 450);
        tap(324, 450);
        tap(499, 451);
        check(world.hasSpawnPoint() && world.spawnX() == 50 &&
                  world.spawnY() == 0 && world.spawnZ() == 23,
              "PLACE stores Level start at the exact red-preview cell");
        pressKey(Keys::U);
        check(!world.hasSpawnPoint(),
              "undo restores the legacy unset spawn state");
        pressKey(Keys::R);
        check(world.hasSpawnPoint() && world.spawnX() == 50 &&
                  world.spawnY() == 0 && world.spawnZ() == 23,
              "redo restores the placed Level start");
        pressKey(Keys::Enter);
        const World loaded = World::loadFromFile(kSpawnSavePath);
        check(loaded.hasSpawnPoint() && loaded.spawnX() == 50 &&
                  loaded.spawnY() == 0 && loaded.spawnZ() == 23,
              "saving and reloading retains the placed Level start");
        std::remove(kSpawnSavePath);
    }

    {
        constexpr const char* kBigDecorSavePath =
            "verify_ge_editor_palette_big_decor.vwr";
        World world;
        GEWorldEditor editor;
        editor.SetWorldPath(kBigDecorSavePath);
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        Easy3D::Camera3D camera;
        const auto tap = [&](int x, int y)
        {
            const MouseState down(
                x, y, 0, ButtonState::Pressed, ButtonState::Released,
                ButtonState::Released, ButtonState::Released, ButtonState::Released);
            const MouseState up(
                x, y, 0, ButtonState::Released, ButtonState::Released,
                ButtonState::Released, ButtonState::Released, ButtonState::Released);
            editor.Update(KeyboardState{}, down, 0.0f, 800, 480, camera, world);
            editor.Update(KeyboardState{}, up, 0.0f, 800, 480, camera, world);
        };
        const auto pressKey = [&](Keys key)
        {
            editor.Update(KeyboardState{key}, restMouse, 0.0f, 800, 480, camera, world);
            editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, world);
        };

        tap(30, 72);
        tap(72, 72);
        tap(499, 451);
        auto bigDecor = GalaxyEggbert::CollectBigDecor(world);
        check(bigDecor.size() == 1 && bigDecor[0].icon == 20 &&
                  bigDecor[0].x == 50 && bigDecor[0].y == 0 &&
                  bigDecor[0].z == 23 &&
                  world.getBlock(50, 0, 23).isAir(),
              "PLACE stores Tree in the separate non-colliding BigDecor layer");
        check(editor.ConsumeNeedsPresentationRebuild(),
              "BigDecor placement immediately requests a billboard rebuild");
        pressKey(Keys::U);
        check(GalaxyEggbert::CollectBigDecor(world).empty(),
              "undo removes a placed BigDecor billboard");
        pressKey(Keys::R);
        check(GalaxyEggbert::CollectBigDecor(world).size() == 1,
              "redo restores a placed BigDecor billboard");
        tap(30, 30);
        check(GalaxyEggbert::CollectBigDecor(world).empty(),
              "the delete tool removes BigDecor at the red-preview cell");
        pressKey(Keys::U);
        check(GalaxyEggbert::CollectBigDecor(world).size() == 1,
              "undo restores BigDecor removed through the delete tool");
        pressKey(Keys::Enter);
        const auto loaded =
            GalaxyEggbert::CollectBigDecor(World::loadFromFile(kBigDecorSavePath));
        check(loaded.size() == 1 && loaded[0].icon == 20,
              "saving and reloading retains the BigDecor billboard");
        std::remove(kBigDecorSavePath);
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
