#include "Editor/GEEditorPalette.hpp"
#include "Editor/GEEditorPaletteLayout.hpp"
#include "Editor/GEPaletteCategories.hpp"
#include "Editor/GEWorldEditor.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>
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
              "an unmapped entry never aliases to another block");
        (void)click(palette, 198.0f, 72.0f);
        check(palette.IsNotYetImplementedNoticeVisible(),
              "an unmapped source entry starts the two-second notice");
        const MouseState idle(
            400, 200, 0, ButtonState::Released, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        (void)palette.Update(idle, kViewportWidth, kViewportHeight, 2.01f);
        check(!palette.IsNotYetImplementedNoticeVisible(),
              "the not-implemented notice expires after two seconds");
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

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
