#include <GalaxyEggbert/Editor/CustomWorldStorage.hpp>
#include <GalaxyEggbert/Editor/EditorBrowserScreen.hpp>
#include <GalaxyEggbert/Editor/WorldEditor.hpp>

#include <GalaxyEggbert/BigDecorRecord.hpp>
#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/MoveObjectRecord.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <iostream>

int main()
{
    using namespace GalaxyEggbert::Game;
    using namespace GalaxyEggbert::Editor;
    using GalaxyEggbert::Worlds::Block;
    using GalaxyEggbert::Worlds::World;
    using Microsoft::Xna::Framework::Input::ButtonState;
    using Microsoft::Xna::Framework::Input::KeyboardState;
    using Microsoft::Xna::Framework::Input::MouseState;

    bool allOk = true;
    const auto check = [&allOk](bool condition, const char* message)
    {
        std::cout << (condition ? "PASS" : "FAIL") << ": " << message << std::endl;
        if (!condition) allOk = false;
    };

    const auto clickBrowser = [](EditorBrowserScreen& browser, int x, int y)
    {
        const MouseState down(
            x, y, 0, ButtonState::Pressed, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        const MouseState up(
            x, y, 0, ButtonState::Released, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        (void)browser.Update(down, 800, 480);
        return browser.Update(up, 800, 480);
    };

    constexpr int kStorageSlot = 77;
    std::error_code error;
    std::filesystem::remove_all(CustomWorldsDir(kStorageSlot), error);

#if defined(__EMSCRIPTEN__)
    const std::filesystem::path expectedRoot = "/save/customworlds";
#else
    const std::filesystem::path expectedRoot = "customworlds";
#endif
    check(CustomWorldsDir(kStorageSlot) == expectedRoot / "gamer77",
          "custom worlds use the expected per-gamer directory");
    check(ListCustomWorlds(kStorageSlot).empty(),
          "an unused gamer slot initially has no custom worlds");

    const auto firstPath = NextNewWorldPath(kStorageSlot);
    check(firstPath.filename() == "custom_001.vwr",
          "the first generated world name is custom_001.vwr");
    check(std::filesystem::exists(CustomWorldsDir(kStorageSlot)),
          "requesting a new world path creates its directory");
    World().saveToFile(firstPath);

    const auto secondPath = NextNewWorldPath(kStorageSlot);
    check(secondPath.filename() == "custom_002.vwr",
          "world numbering advances to the first unused number");
    World().saveToFile(secondPath);
    check(ListCustomWorlds(kStorageSlot).size() == 2,
          "the storage scan returns both saved worlds");

    // Ten slots per gamer (owner, 2026-10-25): slot n is custom_00n.vwr.
    check(SlotWorldPath(kStorageSlot, 1) == firstPath &&
              SlotWorldPath(kStorageSlot, 10).filename() == "custom_010.vwr",
          "slot n is custom_00n.vwr, so the first ten numbered worlds keep their slots");
    for (int slot = 3; slot <= kCustomWorldSlotCount; ++slot)
    {
        World().saveToFile(NextNewWorldPath(kStorageSlot));
    }
    check(NextNewWorldPath(kStorageSlot).empty(),
          "with all ten slots used there is no new world path (nothing is overwritten)");
    {
        // Older worlds: custom_011.vwr from the 999-world numbering and any
        // other name stay listed, outside the slots.
        World().saveToFile(CustomWorldsDir(kStorageSlot) / "custom_011.vwr");
        World legacy;
        legacy.setBlock(3, 4, 5, Block::make(42));
        legacy.saveToFile(CustomWorldsDir(kStorageSlot) / "slot01.vwr");
        const auto older = ListOlderCustomWorlds(kStorageSlot);
        check(older.size() == 2 && older[0].filename() == "custom_011.vwr" && older[1].filename() == "slot01.vwr",
              "files outside the ten slots are listed as older worlds");
        check(CopyIntoFreeSlot(kStorageSlot, older[1]).empty(),
              "an older world cannot be copied while every slot is taken");
        std::filesystem::remove(SlotWorldPath(kStorageSlot, 4), error);
        const auto copy = CopyIntoFreeSlot(kStorageSlot, older[1]);
        check(copy == SlotWorldPath(kStorageSlot, 4) && World::loadFromFile(copy).getBlock(3, 4, 5).type() == 42 &&
                  std::filesystem::exists(older[1]),
              "an older world copies into the free slot and the original stays");
    }

    {
        // Slot 1 is seeded from the showcase once, only when empty.
        constexpr int kSeedSlot = 79;
        std::filesystem::remove_all(CustomWorldsDir(kSeedSlot), error);
        const std::filesystem::path showcase = "verify_ge_editor_storage_showcase.vwr";
        World show;
        show.setBlock(1, 2, 3, Block::make(77));
        show.saveToFile(showcase);
        check(SeedShowcaseSlot(kSeedSlot, showcase) &&
                  World::loadFromFile(SlotWorldPath(kSeedSlot, 1)).getBlock(1, 2, 3).type() == 77,
              "an empty slot 1 is seeded with the showcase world");
        World edited;
        edited.setBlock(9, 9, 9, Block::make(5));
        edited.saveToFile(SlotWorldPath(kSeedSlot, 1));
        check(!SeedShowcaseSlot(kSeedSlot, showcase) &&
                  World::loadFromFile(SlotWorldPath(kSeedSlot, 1)).getBlock(9, 9, 9).type() == 5,
              "seeding happens once: an edited slot 1 is never overwritten");
        std::filesystem::remove(SlotWorldPath(kSeedSlot, 1), error);
        check(!SeedShowcaseSlot(kSeedSlot, showcase) && !std::filesystem::exists(SlotWorldPath(kSeedSlot, 1)),
              "a deleted slot 1 is not seeded again");

        constexpr int kTakenSlot = 80;
        std::filesystem::remove_all(CustomWorldsDir(kTakenSlot), error);
        std::filesystem::create_directories(CustomWorldsDir(kTakenSlot), error);
        edited.saveToFile(SlotWorldPath(kTakenSlot, 1));
        check(!SeedShowcaseSlot(kTakenSlot, showcase) &&
                  World::loadFromFile(SlotWorldPath(kTakenSlot, 1)).getBlock(9, 9, 9).type() == 5,
              "a gamer's own world in slot 1 is kept; the showcase is not seeded over it");
        std::filesystem::remove(showcase, error);
        std::filesystem::remove_all(CustomWorldsDir(kSeedSlot), error);
        std::filesystem::remove_all(CustomWorldsDir(kTakenSlot), error);
    }

    {
        const World starter = CreateEditorStarterWorld();
        bool rockBoardIsComplete = true;
        for (std::uint16_t x = 49; x <= 51; ++x)
        {
            for (std::uint16_t z = 49; z <= 51; ++z)
            {
                rockBoardIsComplete &=
                    starter.getBlock(x, 0, z).type() == GalaxyEggbert::BlockTypes::RockPile;
            }
        }
        check(rockBoardIsComplete,
              "a new editor world contains the complete 3x3 RockPile board");

        const auto objects = GalaxyEggbert::CollectMoveObjects(starter);
        const auto contains = [&objects](GalaxyEggbert::Def::ObjectType type, float x, float z)
        {
            return std::any_of(objects.begin(), objects.end(), [=](const auto& object)
            {
                return object.type == type &&
                       object.posStartX == x &&
                       object.posStartY == 1.0f &&
                       object.posStartZ == z;
            });
        };
        check(contains(GalaxyEggbert::Def::ObjectType::ObjectType5, 50.0f, 50.0f),
              "the starter board has a treasure chest in its centre");
        check(contains(GalaxyEggbert::Def::ObjectType::ObjectType7, 49.0f, 50.0f),
              "the starter board has an exit arrow opposite Blupi");
    }

    constexpr int kBrowserSlot = 78;
    std::filesystem::remove_all(CustomWorldsDir(kBrowserSlot), error);
    World().saveToFile(NextNewWorldPath(kBrowserSlot));
    {
        const auto centre = [](const auto& r) { return std::pair<int, int>((r.x0 + r.x1) / 2, (r.y0 + r.y1) / 2); };
        EditorBrowserScreen browser;
        browser.Refresh(kBrowserSlot);
        const auto [s1x, s1y] = centre(EditorBrowserScreen::SlotRect(0));
        const auto open = clickBrowser(browser, s1x - 60, s1y);
        check(open.action == EditorBrowserScreen::Action::Open && open.path == SlotWorldPath(kBrowserSlot, 1),
              "an occupied slot row opens its world");
        const auto [s2x, s2y] = centre(EditorBrowserScreen::SlotRect(1));
        const auto create = clickBrowser(browser, s2x, s2y);
        check(create.action == EditorBrowserScreen::Action::New && create.path == SlotWorldPath(kBrowserSlot, 2),
              "an empty slot row asks for a new world in that slot");

        const auto [bx, by] = centre(EditorBrowserScreen::BackButtonRect());
        const auto back = clickBrowser(browser, bx, by);
        check(back.action == EditorBrowserScreen::Action::Back,
              "the browser's visible Main Menu row reports a back action");

        const MouseState idle(
            0, 0, 0, ButtonState::Released, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        check(browser.Update(idle, 800, 480, true).action ==
                  EditorBrowserScreen::Action::Back,
              "Escape reports a browser back action");
        check(browser.Update(idle, 800, 480, true).action ==
                  EditorBrowserScreen::Action::None,
              "holding Escape does not repeat the browser back action");
        (void)browser.Update(idle, 800, 480, false);

        const auto [dx, dy] = centre(EditorBrowserScreen::DeleteButtonRect(0));
        const auto armDelete = clickBrowser(browser, dx, dy);
        check(armDelete.action == EditorBrowserScreen::Action::None &&
                  std::filesystem::exists(SlotWorldPath(kBrowserSlot, 1)),
              "the first delete click only arms confirmation");
        (void)clickBrowser(browser, dx, dy);
        check(!std::filesystem::exists(SlotWorldPath(kBrowserSlot, 1)),
              "the second delete click removes the slot's world");

        World legacy;
        legacy.setBlock(7, 7, 7, Block::make(9));
        legacy.saveToFile(CustomWorldsDir(kBrowserSlot) / "custom_042.vwr");
        browser.Refresh(kBrowserSlot);
        const auto [ox, oy] = centre(EditorBrowserScreen::OlderRect(0));
        const auto copied = clickBrowser(browser, ox, oy);
        check(copied.action == EditorBrowserScreen::Action::Open && copied.path == SlotWorldPath(kBrowserSlot, 1) &&
                  World::loadFromFile(copied.path).getBlock(7, 7, 7).type() == 9 &&
                  std::filesystem::exists(CustomWorldsDir(kBrowserSlot) / "custom_042.vwr"),
              "clicking an older world copies it into the first free slot and opens the copy");
    }

    {
        WorldEditor editor;
        check(editor.IsBrowsing(), "the editor starts in browser mode");
        std::filesystem::remove_all(CustomWorldsDir(kBrowserSlot), error);
        editor.EnterBrowser(kBrowserSlot);
        const auto r = EditorBrowserScreen::SlotRect(2);
        const int cx = static_cast<int>((r.x0 + r.x1) / 2), cy = static_cast<int>((r.y0 + r.y1) / 2);

        const MouseState down(
            cx, cy, 0, ButtonState::Pressed, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        const MouseState up(
            cx, cy, 0, ButtonState::Released, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        (void)editor.UpdateBrowsing(down, 800, 480);
        const auto request = editor.UpdateBrowsing(up, 800, 480);
        check(request.shouldCreateNew && request.createPath == SlotWorldPath(kBrowserSlot, 3),
              "the editor forwards the browser's create request with the slot's path");

        const MouseState idle(
            0, 0, 0, ButtonState::Released, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        check(editor.UpdateBrowsing(idle, 800, 480, true).shouldReturnToMenu,
              "the editor forwards Escape from the browser to the main menu");

        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        check(!editor.IsBrowsing(), "EnterEditing leaves browser mode");
        editor.EnterBrowser(kBrowserSlot);
        editor.ExitBrowser();
        check(!editor.IsBrowsing(), "ExitBrowser leaves browser mode");
        std::filesystem::remove_all(CustomWorldsDir(kBrowserSlot), error);
        std::filesystem::remove_all(CustomWorldsDir(kStorageSlot), error);
    }

    {
        constexpr const char* kPlayTestPath =
            "verify_ge_editor_storage_playtest_scratch.vwr";
        World world;
        world.setBlock(10, 5, 10, Block::make(42));
        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        editor.SetWorldPath(kPlayTestPath);
        CnaEngine::Graphics::Camera camera;

        const MouseState down(
            724, 451, 0, ButtonState::Pressed, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        const MouseState up(
            724, 451, 0, ButtonState::Released, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, down, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{}, up, 0.0f, 800, 480, camera, world);

        check(editor.GetWorldPath() == std::filesystem::path(kPlayTestPath),
              "the editor retains the configured world path");
        check(editor.ConsumePlayTestRequested(),
              "the dice button requests play-testing");
        check(!editor.ConsumePlayTestRequested(),
              "the play-test request is consumed exactly once");
        check(World::loadFromFile(kPlayTestPath).getBlock(10, 5, 10).type() == 42,
              "play-testing saves the current world before switching mode");
        std::remove(kPlayTestPath);
    }

    {
        World world;
        for (std::uint16_t z = 0; z < 100; ++z)
        {
            for (std::uint16_t y = 0; y < 4; ++y)
            {
                world.setBlock(50, y, z, Block::make(1));
            }
        }
        const auto solidCount = [&world]()
        {
            int count = 0;
            for (std::uint16_t y = 0; y < 100; ++y)
            {
                for (std::uint16_t z = 0; z < 100; ++z)
                {
                    if (!world.getBlock(50, y, z).isAir()) ++count;
                }
            }
            return count;
        };

        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        CnaEngine::Graphics::Camera camera;
        const int before = solidCount();
        const std::size_t bigDecorBefore =
            GalaxyEggbert::CollectBigDecor(world).size();
        const auto clickEditor = [&](int x, int y)
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

        clickEditor(30, 72);
        clickEditor(72, 72);
        check(solidCount() == before,
              "visible palette clicks never edit the 3D world behind them");
        clickEditor(466, 26);
        check(solidCount() == before &&
                  GalaxyEggbert::CollectBigDecor(world).size() ==
                      bigDecorBefore + 1,
              "the former invisible toolbar area reaches the 3D world and places the selected non-colliding scenery");
    }

    std::filesystem::remove_all(CustomWorldsDir(kStorageSlot), error);
    std::filesystem::remove_all(CustomWorldsDir(kBrowserSlot), error);
    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
