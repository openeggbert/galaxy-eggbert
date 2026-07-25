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
    using namespace GalaxyEggbert::CNA;
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
        EditorBrowserScreen browser;
        browser.Refresh(kBrowserSlot);
        const auto create = clickBrowser(browser, 300, 38);
        check(create.action == EditorBrowserScreen::Action::New,
              "the browser's New row reports a create action");

        const auto back = clickBrowser(browser, 80, 38);
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

        const auto open = clickBrowser(browser, 300, 80);
        check(open.action == EditorBrowserScreen::Action::Open &&
                  open.path.extension() == ".vwr",
              "a browser world row reports its .vwr path");

        const auto armDelete = clickBrowser(browser, 500, 80);
        check(armDelete.action == EditorBrowserScreen::Action::None &&
                  !ListCustomWorlds(kBrowserSlot).empty(),
              "the first delete click only arms confirmation");
        (void)clickBrowser(browser, 500, 80);
        check(ListCustomWorlds(kBrowserSlot).empty(),
              "the second delete click removes the selected world");
    }

    {
        WorldEditor editor;
        check(editor.IsBrowsing(), "the editor starts in browser mode");
        editor.EnterBrowser(kBrowserSlot);

        const MouseState down(
            300, 38, 0, ButtonState::Pressed, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        const MouseState up(
            300, 38, 0, ButtonState::Released, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        (void)editor.UpdateBrowsing(down, 800, 480);
        check(editor.UpdateBrowsing(up, 800, 480).shouldCreateNew,
              "the editor forwards the browser's create request");

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
    }

    {
        constexpr const char* kPlayTestPath =
            "verify_ge_editor_storage_playtest_scratch.vwr";
        World world;
        world.setBlock(10, 5, 10, Block::make(42));
        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        editor.SetWorldPath(kPlayTestPath);
        Easy3D::Camera3D camera;

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
        Easy3D::Camera3D camera;
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
