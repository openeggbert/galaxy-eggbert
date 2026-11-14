#include <GalaxyEggbert/Editor/CustomWorldStorage.hpp>
#include <GalaxyEggbert/Editor/EditorBrowserScreen.hpp>
#include <GalaxyEggbert/Editor/GalaxyBrowserScreen.hpp>
#include <GalaxyEggbert/Editor/WorldEditor.hpp>
#include <GalaxyEggbert/Game/DemoReplay.hpp>

#include <GalaxyEggbert/BigDecorRecord.hpp>
#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/MoveObjectRecord.hpp>
#include <GalaxyEggbert/PlayerStart.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

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
        // The gallery (OR-0044): the first free slot, then kept current while unedited.
        constexpr int kGallerySlot = 81;
        std::filesystem::remove_all(CustomWorldsDir(kGallerySlot), error);
        std::filesystem::create_directories(CustomWorldsDir(kGallerySlot), error);
        World own;
        own.setBlock(9, 9, 9, Block::make(5));
        own.saveToFile(SlotWorldPath(kGallerySlot, 1));
        const std::filesystem::path gallery = "verify_ge_editor_storage_gallery.vwr";
        World first;
        first.setBlock(1, 1, 1, Block::make(35));
        first.saveToFile(gallery);
        check(SeedGallerySlot(kGallerySlot, gallery) &&
                  World::loadFromFile(SlotWorldPath(kGallerySlot, 2)).getBlock(1, 1, 1).type() == 35 &&
                  World::loadFromFile(SlotWorldPath(kGallerySlot, 1)).getBlock(9, 9, 9).type() == 5,
              "the gallery goes into the first free slot (2), the gamer's slot 1 kept");
        check(!SeedGallerySlot(kGallerySlot, gallery), "an unchanged gallery is not copied again");
        World second;
        second.setBlock(2, 2, 2, Block::make(261));
        second.saveToFile(gallery);
        check(SeedGallerySlot(kGallerySlot, gallery) &&
                  World::loadFromFile(SlotWorldPath(kGallerySlot, 2)).getBlock(2, 2, 2).type() == 261,
              "a newer gallery replaces the unedited copy");
        World edited;
        edited.setBlock(3, 3, 3, Block::make(10));
        edited.saveToFile(SlotWorldPath(kGallerySlot, 2));
        first.saveToFile(gallery);
        check(!SeedGallerySlot(kGallerySlot, gallery) &&
                  World::loadFromFile(SlotWorldPath(kGallerySlot, 2)).getBlock(3, 3, 3).type() == 10,
              "an edited gallery copy is never overwritten");
        std::filesystem::remove(gallery, error);
        std::filesystem::remove_all(CustomWorldsDir(kGallerySlot), error);
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
        // The yellow Blupi (2026-11-11, as Speedy Blupi's new world): at the other edge, facing -X.
        const auto yellow = GalaxyEggbert::FindPlayerStart(starter, GalaxyEggbert::PlayerStartColor::Yellow);
        check(yellow && yellow->x == 51 && yellow->y == 1 && yellow->z == 50 && yellow->quarterTurns == 1 &&
                  starter.hasSpawnPoint(),
              "the starter board has the yellow Blupi at its edge, facing the chest and the exit");
        check(!GalaxyEggbert::FindPlayerStart(starter, GalaxyEggbert::PlayerStartColor::Orange),
              "the starter board has no other colour's start");
    }

    constexpr int kBrowserSlot = 78;
    std::filesystem::remove_all(CustomWorldsDir(kBrowserSlot), error);
    World().saveToFile(NextNewWorldPath(kBrowserSlot));
    {
        const auto centre = [](const auto& r) { return std::pair<int, int>((r.x0 + r.x1) / 2, (r.y0 + r.y1) / 2); };
        EditorBrowserScreen browser;
        browser.Refresh(kBrowserSlot);
        const auto [s1x, s1y] = centre(EditorBrowserScreen::SlotRect(0));
        // The cell's centre: its trash button is on the number button's corner.
        const auto open = clickBrowser(browser, s1x, s1y);
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
            400, 240, 0, ButtonState::Released, ButtonState::Released,
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
            400, 240, 0, ButtonState::Released, ButtonState::Released,
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
        check(!editor.ConsumePlayTestRequested(), "the play-test waits for the save's thumbnail (ED-0013)");
        check(editor.ConsumeThumbnailRequest() ==
                  std::filesystem::path("verify_ge_editor_storage_playtest_scratch.png") &&
                  editor.ConsumeThumbnailRequest().empty(),
              "a save asks once for its thumbnail beside the world");
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
        clickEditor(466, 300);
        check(solidCount() == before &&
                  GalaxyEggbert::CollectBigDecor(world).size() ==
                      bigDecorBefore + 1,
              "the former invisible toolbar area reaches the 3D world and places the selected non-colliding scenery");
    }

    // ED-0011: backups, three generations; restoring steps back round to the start; a deleted slot
    // is its newest backup; the editor backs a world up at its first save after opening it.
    {
        using Microsoft::Xna::Framework::Input::Keys;
        std::filesystem::remove_all(CustomWorldsDir(kStorageSlot), error);
        const auto path = SlotWorldPath(kStorageSlot, 4);
        std::filesystem::create_directories(path.parent_path(), error);
        const auto saveWith = [&](std::uint16_t icon)
        {
            World w;
            w.setBlock(1, 1, 1, Block::make(icon));
            w.saveToFile(path);
        };
        const auto iconIn = [](const std::filesystem::path& file)
        {
            return World::loadFromFile(file).getBlock(1, 1, 1).type();
        };
        saveWith(11);
        check(!BackUpWorld(SlotWorldPath(kStorageSlot, 5)) && BackupCount(path) == 0,
              "a slot without a world has nothing to back up");
        for (const std::uint16_t icon : {12, 13, 14, 15})
        {
            (void)BackUpWorld(path);
            saveWith(icon);
        }
        check(BackupCount(path) == 3 && iconIn(BackupPath(path, 1)) == 14 && iconIn(BackupPath(path, 3)) == 12 &&
                  BackupPath(path, 1).parent_path().filename() == "backups",
              "three generations in backups/, the newest first, the oldest dropped");
        bool cycle = RestoreBackup(path) && iconIn(path) == 14;
        cycle = cycle && RestoreBackup(path) && iconIn(path) == 13;
        cycle = cycle && RestoreBackup(path) && iconIn(path) == 12;
        cycle = cycle && RestoreBackup(path) && iconIn(path) == 15 && BackupCount(path) == 3;
        check(cycle, "restoring steps back through the backups and comes round to the world it began with");
        if (std::FILE* file = std::fopen(ThumbnailPath(path).string().c_str(), "w"))
        {
            std::fclose(file);
        }
        check(DeleteSlotWorld(path) && !std::filesystem::exists(path) &&
                  !std::filesystem::exists(ThumbnailPath(path)) && BackupCount(path) == 3 &&
                  iconIn(BackupPath(path, 1)) == 15,
              "deleting a slot keeps its world as the newest backup; its thumbnail goes");
        check(RestoreBackup(path) && iconIn(path) == 15 && BackupCount(path) == 2,
              "a deleted world comes back from its backups");

        EditorBrowserScreen browser;
        browser.Refresh(kStorageSlot);
        const auto mode = EditorBrowserScreen::RestoreModeRect();
        const auto slot = EditorBrowserScreen::SlotRect(3);
        (void)clickBrowser(browser, static_cast<int>((mode.x0 + mode.x1) / 2), static_cast<int>((mode.y0 + mode.y1) / 2));
        check(browser.CurrentMode() == EditorBrowserScreen::Mode::Restore, "the Backups button turns restoring on");
        const auto restored =
            clickBrowser(browser, static_cast<int>((slot.x0 + slot.x1) / 2), static_cast<int>(slot.y0 + 20.0f));
        check(restored.action == EditorBrowserScreen::Action::None && iconIn(path) == 14 &&
                  browser.Message().find("restored") != std::string::npos,
              "in Backups a click on a slot restores its backup and opens nothing");
        (void)clickBrowser(browser, static_cast<int>((mode.x0 + mode.x1) / 2), static_cast<int>((mode.y0 + mode.y1) / 2));
        check(browser.CurrentMode() == EditorBrowserScreen::Mode::Open, "the Backups button again: clicks open worlds");

        // The editor: the first save after the world was opened from the browser backs it up.
        const auto openAndSave = [&](std::uint16_t icon, int saves)
        {
            WorldEditor editor;
            editor.EnterBrowser(kStorageSlot);
            editor.ExitBrowser();
            editor.EnterEditing(0.0f, 10.0f, 0.0f);
            editor.SetWorldPath(path);
            World w;
            w.setBlock(1, 1, 1, Block::make(icon));
            CnaEngine::Graphics::Camera camera;
            const MouseState idle(400, 240, 0, ButtonState::Released, ButtonState::Released, ButtonState::Released,
                                  ButtonState::Released, ButtonState::Released);
            for (int i = 0; i < saves; ++i)
            {
                editor.Update(KeyboardState{Keys::Enter}, idle, 0.0f, 800, 480, camera, w);
                editor.Update(KeyboardState{}, idle, 0.0f, 800, 480, camera, w);
            }
        };
        openAndSave(20, 2);
        check(iconIn(path) == 20 && iconIn(BackupPath(path, 1)) == 14 && iconIn(BackupPath(path, 2)) == 13,
              "the first save after opening keeps the world as it was, a second save of that visit adds none");
        openAndSave(21, 1);
        check(iconIn(path) == 21 && iconIn(BackupPath(path, 1)) == 20 && BackupCount(path) == 3,
              "the next visit's first save keeps the last one");
    }

    // ED-0012: a slot exported into the shared folder, listed first beside the older worlds, copied
    // into a free slot by a click; Import (not the web) says where to put files.
    {
        std::filesystem::remove_all(CustomWorldsDir(kStorageSlot), error);
        const auto path = SlotWorldPath(kStorageSlot, 2);
        std::filesystem::create_directories(path.parent_path(), error);
        World w;
        w.setBlock(2, 2, 2, Block::make(33));
        w.saveToFile(path);
        check(ExportName(0, 3) == "A-world-3.vwr" && ExportName(kStorageSlot, 2) == "player77-world-2.vwr",
              "an export is named by the player and the slot");
        const auto shared = SharedWorldsDir() / ExportName(kStorageSlot, 2);
        std::filesystem::remove(shared, error);
        check(ExportWorld(kStorageSlot, 5).empty(), "an empty slot exports nothing");

        EditorBrowserScreen browser;
        browser.Refresh(kStorageSlot);
        const auto centre = [](const GalaxyEggbert::Game::QuadBatch::Rect& r, float dy = -1.0f)
        {
            return std::pair<int, int>{static_cast<int>((r.x0 + r.x1) / 2),
                                       static_cast<int>(dy < 0.0f ? (r.y0 + r.y1) / 2 : r.y0 + dy)};
        };
        auto [ex, ey] = centre(EditorBrowserScreen::ExportModeRect());
        (void)clickBrowser(browser, ex, ey);
        check(browser.CurrentMode() == EditorBrowserScreen::Mode::Export, "the Export button turns exporting on");
        auto [sx, sy] = centre(EditorBrowserScreen::SlotRect(1), 20.0f);
        const auto exported = clickBrowser(browser, sx, sy);
        check(exported.action == EditorBrowserScreen::Action::None && std::filesystem::exists(shared) &&
                  World::loadFromFile(shared).getBlock(2, 2, 2).type() == 33 &&
                  browser.Message().find("exported") != std::string::npos,
              "in Export a click on a slot copies its world into the shared folder and opens nothing");
        const auto& others = browser.OtherWorlds();
        check(!others.empty() && others.front() == shared, "the export is listed first among the other worlds");
        (void)clickBrowser(browser, ex, ey);
        check(browser.CurrentMode() == EditorBrowserScreen::Mode::Open, "Export again: clicks open worlds");
        auto [ox, oy] = centre(EditorBrowserScreen::OlderRect(0));
        const auto copied = clickBrowser(browser, ox, oy);
        check(copied.action == EditorBrowserScreen::Action::Open && copied.path == SlotWorldPath(kStorageSlot, 1) &&
                  World::loadFromFile(copied.path).getBlock(2, 2, 2).type() == 33 && std::filesystem::exists(shared),
              "a click on a shared world copies it into the first free slot, the shared file staying");
        auto [ix, iy] = centre(EditorBrowserScreen::ImportRect());
        (void)clickBrowser(browser, ix, iy);
        check(browser.Message().find("shared") != std::string::npos, "Import (not the web) says where to put files");
        std::filesystem::remove(shared, error);
        std::filesystem::remove(SharedWorldsDir(), error); // only when nothing else is in it
    }

    // BD-0119: the player's own galaxy -- the campaign's numbering in the gamer's galaxy folder.
    {
        check(GalaxyWorldPath(kStorageSlot, 23) == CustomWorldsDir(kStorageSlot) / "galaxy" / "world023.vwr" &&
                  GalaxyWorldPath(kStorageSlot, 1).filename() == "world001.vwr",
              "galaxy: world NNN of a gamer's galaxy lives in its galaxy folder");
        bool places = IsGalaxyMission(1) && IsGalaxyMission(10) && IsGalaxyMission(18) && IsGalaxyMission(140) &&
                      IsGalaxyMission(148) && IsGalaxyMission(199);
        for (int not_ : {0, 2, 9, 19, 149, 150, 198, 200, 999}) places = places && !IsGalaxyMission(not_);
        check(places, "galaxy: the main hub, world hubs 10-140, their missions 1-8 and 199 only");
        const auto parsed = ParseGalaxyWorldPath(GalaxyWorldPath(1, 23).generic_string());
        check(parsed && parsed->first == 1 && parsed->second == 23 &&
                  !ParseGalaxyWorldPath(GalaxyWorldPath(1, 19).generic_string()) &&
                  !ParseGalaxyWorldPath("worlds3d/world023.vwr") &&
                  !ParseGalaxyWorldPath(GalaxyWorldPath(1, 23).generic_string() + "x") &&
                  !ParseGalaxyWorldPath(GalaxyWorldPath(kStorageSlot, 23).generic_string()),
              "galaxy: a galaxy world's path names its player (A-C) and mission, nothing else does");
        check(GalaxyMissionName(1) == "Main hub" && GalaxyMissionName(30) == "World 3" &&
                  GalaxyMissionName(34) == "World 3, mission 4" && GalaxyMissionName(199) == "Bonus world",
              "galaxy: names of its places");
        const World hub = CreateGalaxyStarterWorld(1);
        const World worldHub = CreateGalaxyStarterWorld(70);
        const World mission = CreateGalaxyStarterWorld(72);
        check(hub.getBlock(50, 1, 49).type() == GalaxyEggbert::BlockTypes::WorldSelect1 && hub.hasSpawnPoint() &&
                  hub.spawnX() == 50 && hub.spawnZ() == 51 && hub.missionNumber() == 1 &&
                  worldHub.getBlock(50, 1, 49).type() == GalaxyEggbert::BlockTypes::MissionSign1 &&
                  worldHub.getBlock(49, 1, 51).type() == GalaxyEggbert::BlockTypes::ReturnSign &&
                  worldHub.missionNumber() == 70 && mission.missionNumber() == 72 && mission.hasSpawnPoint() &&
                  mission.getBlock(50, 0, 50).type() == GalaxyEggbert::BlockTypes::RockPile,
              "galaxy: new places start with their signs (main hub: world 1; world hub: mission 1 and the way "
              "back), their mission number and Blupi's start");

        // Backups elsewhere (xnjttjpo's shipped worlds, BD-0118).
        const auto dir = CustomWorldsDir(kStorageSlot) / "elsewhere";
        std::filesystem::create_directories(dir, error);
        const auto shippedWorld = dir / "world050.vwr";
        hub.saveToFile(shippedWorld);
        const bool first = BackUpWorldTo(shippedWorld, dir / "kept");
        mission.saveToFile(shippedWorld);
        const bool second = BackUpWorldTo(shippedWorld, dir / "kept");
        check(first && second && World::loadFromFile(dir / "kept" / "world050.2.vwr").missionNumber() == 1 &&
                  World::loadFromFile(dir / "kept" / "world050.1.vwr").missionNumber() == 72 &&
                  !std::filesystem::exists(dir / "backups") &&
                  CampaignBackupsDir() == CustomWorldsRoot() / "campaign-backups",
              "BackUpWorldTo keeps the generations in the folder given, the newest as .1");

        // The editor's galaxy screen.
        std::filesystem::remove_all(CustomWorldsDir(kBrowserSlot), error);
        WorldEditor editor;
        editor.EnterBrowser(kBrowserSlot);
        const auto press = [&](float x, float y, bool back = false) {
            const MouseState down(int(x), int(y), 0, ButtonState::Pressed, ButtonState::Released, ButtonState::Released,
                                  ButtonState::Released, ButtonState::Released);
            const MouseState up(int(x), int(y), 0, ButtonState::Released, ButtonState::Released,
                                ButtonState::Released, ButtonState::Released, ButtonState::Released);
            (void)editor.UpdateBrowsing(down, 800, 480, back);
            return editor.UpdateBrowsing(up, 800, 480, back);
        };
        const auto mid = [](const GalaxyEggbert::Game::QuadBatch::Rect& r) {
            return std::make_pair((r.x0 + r.x1) * 0.5f, (r.y0 + r.y1) * 0.5f);
        };
        auto [gx, gy] = mid(EditorBrowserScreen::GalaxyRect());
        const auto toGalaxy = press(gx, gy);
        check(editor.ShowsGalaxy() && !toGalaxy.shouldOpen && !toGalaxy.shouldCreateNew && !toGalaxy.shouldReturnToMenu,
              "galaxy: the world screen's Galaxy button shows the galaxy");
        auto [hx, hy] = mid(GalaxyBrowserScreen::MainHubRect());
        const auto newHub = press(hx, hy);
        check(newHub.shouldCreateNew && newHub.galaxyMission == 1 && newHub.createPath == GalaxyWorldPath(kBrowserSlot, 1),
              "galaxy: the main hub not made yet is created, from its starter");
        std::filesystem::create_directories(GalaxyDir(kBrowserSlot), error);
        CreateGalaxyStarterWorld(1).saveToFile(GalaxyWorldPath(kBrowserSlot, 1));
        editor.ShowGalaxy(); // rescans, as coming back from editing does
        const auto openHub = press(hx, hy);
        auto [w4x, w4y] = mid(GalaxyBrowserScreen::WorldRect(4));
        const auto select = press(w4x, w4y);
        auto [p3x, p3y] = mid(GalaxyBrowserScreen::PlaceRect(3));
        const auto place = press(p3x, p3y);
        auto [p0x, p0y] = mid(GalaxyBrowserScreen::PlaceRect(0));
        const auto worldHubRequest = press(p0x, p0y);
        check(openHub.shouldOpen && openHub.galaxyMission == 1 && !select.shouldOpen && !select.shouldCreateNew &&
                  place.shouldCreateNew && place.galaxyMission == 43 &&
                  place.createPath == GalaxyWorldPath(kBrowserSlot, 43) && worldHubRequest.galaxyMission == 40,
              "galaxy: an existing place opens; a world's number selects it; its hub and missions are its own");
        auto [bx, by] = mid(GalaxyBrowserScreen::BonusRect());
        check(press(bx, by).galaxyMission == 199, "galaxy: the bonus world is world 199");
        const MouseState idle(400, 240, 0, ButtonState::Released, ButtonState::Released, ButtonState::Released,
                              ButtonState::Released, ButtonState::Released);
        const auto escape = editor.UpdateBrowsing(idle, 800, 480, true);
        const auto held = editor.UpdateBrowsing(idle, 800, 480, true);
        const auto again = editor.UpdateBrowsing(idle, 800, 480, false);
        const auto menu = editor.UpdateBrowsing(idle, 800, 480, true);
        check(!escape.shouldReturnToMenu && !editor.ShowsGalaxy() && !held.shouldReturnToMenu &&
                  !again.shouldReturnToMenu && menu.shouldReturnToMenu,
              "galaxy: Escape goes back to the world screen, and only a second Escape to the menu");
        bool apart = true;
        std::vector<GalaxyEggbert::Game::QuadBatch::Rect> rects{GalaxyBrowserScreen::MainHubRect(),
                                                                 GalaxyBrowserScreen::BonusRect(),
                                                                 GalaxyBrowserScreen::BackupsRect(),
                                                                 GalaxyBrowserScreen::BackButtonRect()};
        for (int world = 1; world <= kGalaxyWorldCount; ++world) rects.push_back(GalaxyBrowserScreen::WorldRect(world));
        for (int p = 0; p <= kGalaxyMissionsPerWorld; ++p) rects.push_back(GalaxyBrowserScreen::PlaceRect(p));
        for (std::size_t i = 0; i < rects.size(); ++i)
            for (std::size_t j = i + 1; j < rects.size(); ++j)
                apart = apart && (rects[i].x1 <= rects[j].x0 || rects[j].x1 <= rects[i].x0 ||
                                  rects[i].y1 <= rects[j].y0 || rects[j].y1 <= rects[i].y0);
        for (const auto& r : rects) apart = apart && r.x0 >= 0.0f && r.x1 <= 800.0f && r.y0 >= 0.0f && r.y1 <= 480.0f;
        check(apart, "galaxy: its screen's buttons do not overlap and fit 800x480");
        bool binsApart = true;
        for (int p = 0; p < kGalaxyMissionsPerWorld; ++p)
            binsApart = binsApart && GalaxyBrowserScreen::DeleteRect(GalaxyBrowserScreen::PlaceRect(p)).x1 <=
                                         GalaxyBrowserScreen::PlaceRect(p + 1).x0;
        binsApart = binsApart && GalaxyBrowserScreen::DeleteRect(GalaxyBrowserScreen::MainHubRect()).x1 <=
                                     GalaxyBrowserScreen::BonusRect().x0;
        check(binsApart, "galaxy: a place's bin stays clear of the next place");

        // Deleting and bringing back (the main hub made above exists).
        GalaxyBrowserScreen screen;
        screen.Refresh(kBrowserSlot);
        const auto click = [&screen](const GalaxyEggbert::Game::QuadBatch::Rect& r) {
            const int x = int((r.x0 + r.x1) * 0.5f), y = int((r.y0 + r.y1) * 0.5f);
            (void)screen.Update(MouseState(x, y, 0, ButtonState::Pressed, ButtonState::Released, ButtonState::Released,
                                           ButtonState::Released, ButtonState::Released));
            return screen.Update(MouseState(x, y, 0, ButtonState::Released, ButtonState::Released,
                                            ButtonState::Released, ButtonState::Released, ButtonState::Released));
        };
        using GAction = GalaxyBrowserScreen::Action;
        const auto hubPath = GalaxyWorldPath(kBrowserSlot, 1);
        const auto hubBin = GalaxyBrowserScreen::DeleteRect(GalaxyBrowserScreen::MainHubRect());
        const auto arm = click(hubBin);
        const bool keptWhenArmed = std::filesystem::exists(hubPath) && arm.action == GAction::None;
        (void)click(GalaxyBrowserScreen::WorldRect(2)); // any other click disarms
        const auto rearm = click(hubBin);
        const bool keptAfterDisarm = std::filesystem::exists(hubPath) && rearm.action == GAction::None;
        const auto confirm = click(hubBin);
        check(keptWhenArmed && keptAfterDisarm && confirm.action == GAction::None && !std::filesystem::exists(hubPath) &&
                  BackupCount(hubPath) >= 1 && screen.Message() == "Main hub deleted (Backups brings it back).",
              "galaxy: a place's bin deletes on the second click in a row, into the galaxy's backups");
        const auto gone = click(GalaxyBrowserScreen::MainHubRect());
        const auto binOfNothing = click(hubBin); // no bin on a place not made: the click is the place's
        check(gone.action == GAction::New && binOfNothing.action == GAction::New && !std::filesystem::exists(hubPath),
              "galaxy: a deleted place is made anew on a click, and has no bin");
        (void)click(GalaxyBrowserScreen::BackupsRect());
        const bool restoring = screen.RestoreMode();
        const auto restore = click(GalaxyBrowserScreen::MainHubRect());
        const std::string restoreMessage = screen.Message();
        const bool back = std::filesystem::exists(hubPath) && World::loadFromFile(hubPath).missionNumber() == 1;
        const auto inRestore = click(hubBin); // the bins are off while restoring: the corner restores
        (void)click(GalaxyBrowserScreen::BackupsRect());
        const auto open = click(GalaxyBrowserScreen::MainHubRect());
        check(restoring && restore.action == GAction::None && back && inRestore.action == GAction::None &&
                  restoreMessage == "Main hub is back." &&
                  !screen.RestoreMode() && open.action == GAction::Open && open.mission == 1,
              "galaxy: Backups brings the deleted place back instead of opening it; off again, it opens");
        const auto noBackup = click(GalaxyBrowserScreen::BonusRect()); // opens/creates normally
        (void)click(GalaxyBrowserScreen::BackupsRect());
        (void)click(GalaxyBrowserScreen::BonusRect());
        const bool told = screen.Message() == "Bonus world has no backup.";
        (void)click(GalaxyBrowserScreen::BackupsRect());
        check(noBackup.action == GAction::New && told && !std::filesystem::exists(GalaxyWorldPath(kBrowserSlot, 199)),
              "galaxy: restoring a place without a backup only says so");
    }

    // The web's xnjttjpo edits: copies in CampaignEditsDir() laid over the shipped worlds at start.
    {
        const std::filesystem::path worlds = "verify_campaign_edits_worlds";
        std::filesystem::remove_all(worlds, error);
        std::filesystem::remove_all(CampaignEditsDir(), error);
        std::filesystem::create_directories(worlds, error);
        std::filesystem::create_directories(CampaignEditsDir(), error);
        CreateGalaxyStarterWorld(1).saveToFile(worlds / "world011.vwr");
        CreateGalaxyStarterWorld(10).saveToFile(worlds / "world012.vwr");
        CreateGalaxyStarterWorld(72).saveToFile(CampaignEditsDir() / "world011.vwr");
        CreateGalaxyStarterWorld(72).saveToFile(CampaignEditsDir() / "world11.vwr");
        CreateGalaxyStarterWorld(72).saveToFile(CampaignEditsDir() / "title.vwr");
        const bool intoMissing = ApplyCampaignEdits("verify_no_such_folder") == 0; // nowhere to copy to
        const int applied = ApplyCampaignEdits(worlds);
        check(CampaignEditsDir() == CustomWorldsRoot() / "campaign-edits" && applied == 1 &&
                  World::loadFromFile(worlds / "world011.vwr").missionNumber() == 72 &&
                  World::loadFromFile(worlds / "world012.vwr").missionNumber() == 10 &&
                  !std::filesystem::exists(worlds / "world11.vwr") && !std::filesystem::exists(worlds / "title.vwr") && intoMissing,
              "xnjttjpo on the web: the kept edits (worldNNN.vwr only) are laid over the shipped worlds");
        std::filesystem::remove_all(worlds, error);
        std::filesystem::remove_all(CampaignEditsDir(), error);
    }

    // ... and undone (Pause's "Original"): the shipped world as kept before an edit, the edit dropped.
    {
        const std::filesystem::path worlds = "verify_campaign_revert_worlds";
        const std::filesystem::path originals = "verify_campaign_revert_originals";
        for (const auto& dir : {worlds, originals, CampaignEditsDir()}) std::filesystem::remove_all(dir, error);
        std::filesystem::create_directories(worlds, error);
        std::filesystem::create_directories(CampaignEditsDir(), error);
        CreateGalaxyStarterWorld(1).saveToFile(worlds / "world011.vwr");   // as shipped: mission 1
        CreateGalaxyStarterWorld(72).saveToFile(CampaignEditsDir() / "world011.vwr"); // the edit kept
        const bool notYet = !HasCampaignEdit(worlds / "world011.vwr"); // the file is not the edit yet
        (void)ApplyCampaignEdits(worlds, originals); // a start: the original kept first
        const bool edited = notYet && HasCampaignEdit(worlds / "world011.vwr") && !HasCampaignEdit(worlds / "world012.vwr");
        const bool laid = World::loadFromFile(worlds / "world011.vwr").missionNumber() == 72 &&
                          World::loadFromFile(originals / "world011.vwr").missionNumber() == 1;
        (void)ApplyCampaignEdits(worlds, originals); // a second application keeps the first original
        KeepCampaignOriginal(worlds / "world011.vwr", originals);
        const bool stillShipped = World::loadFromFile(originals / "world011.vwr").missionNumber() == 1;
        const bool reverted = RevertCampaignEdit(worlds / "world011.vwr", originals);
        check(edited && laid && stillShipped && reverted &&
                  World::loadFromFile(worlds / "world011.vwr").missionNumber() == 1 &&
                  !HasCampaignEdit(worlds / "world011.vwr") && !std::filesystem::exists(CampaignEditsDir() / "world011.vwr") &&
                  !std::filesystem::exists(originals / "world011.vwr"),
              "xnjttjpo on the web undone: the world kept as shipped comes back, the edit and the kept original dropped");
        // The desktop: the shipped file edited in place, its original kept at the first visit.
        CreateGalaxyStarterWorld(1).saveToFile(worlds / "world013.vwr");
        KeepCampaignOriginal(worlds / "world013.vwr", originals);
        CreateGalaxyStarterWorld(72).saveToFile(worlds / "world013.vwr"); // the save, and its kept copy
        std::filesystem::copy_file(worlds / "world013.vwr", CampaignEditsDir() / "world013.vwr", error);
        KeepCampaignOriginal(worlds / "world013.vwr", originals); // the next visit keeps the first original
        const bool desktopEdited = HasCampaignEdit(worlds / "world013.vwr") &&
                                   World::loadFromFile(originals / "world013.vwr").missionNumber() == 1;
        CreateGalaxyStarterWorld(30).saveToFile(worlds / "world013.vwr"); // regenerated by tools/worlds
        const bool regenerated = !HasCampaignEdit(worlds / "world013.vwr");
        KeepCampaignOriginal(worlds / "world013.vwr", originals);
        check(desktopEdited && regenerated && World::loadFromFile(originals / "world013.vwr").missionNumber() == 30,
              "the desktop: an edit in place keeps its first original; a world regenerated since is no edit, its original renewed");
        CreateGalaxyStarterWorld(72).saveToFile(CampaignEditsDir() / "world012.vwr");
        check(!RevertCampaignEdit(worlds / "world012.vwr", originals) && !HasCampaignEdit(worlds / "world012.vwr"),
              "without a kept original the edit is still dropped (the next start loads the shipped world)");
        for (const auto& dir : {worlds, originals, CampaignEditsDir()}) std::filesystem::remove_all(dir, error);
        check(CampaignOriginalsDir() == CustomWorldsRoot() / "campaign-originals",
              "the desktop keeps the originals in customworlds/campaign-originals (the web in memory)");
    }

    // A whole galaxy in one file: packed, damaged, exported, imported over another gamer's galaxy.
    {
        std::filesystem::remove_all(CustomWorldsDir(kStorageSlot), error);
        std::filesystem::remove_all(CustomWorldsDir(kBrowserSlot), error);
        check(PackGalaxy(kStorageSlot).empty() && ExportGalaxy(kStorageSlot).empty(),
              "galaxy pack: nothing to pack or export while the galaxy has no place");
        std::filesystem::create_directories(GalaxyDir(kStorageSlot), error);
        for (const int mission : {1, 10, 11, 199})
            CreateGalaxyStarterWorld(mission).saveToFile(GalaxyWorldPath(kStorageSlot, mission));
        const auto readAll = [](const std::filesystem::path& path) {
            std::ifstream in(path, std::ios::binary);
            return std::string((std::istreambuf_iterator<char>(in)), {});
        };
        const std::string pack = PackGalaxy(kStorageSlot);
        const auto unpacked = UnpackGalaxy(pack);
        bool same = unpacked && unpacked->worlds.size() == 4 && unpacked->demos.empty();
        for (const int mission : {1, 10, 11, 199})
            same = same && unpacked->worlds.count(mission) == 1 &&
                   unpacked->worlds.at(mission) == readAll(GalaxyWorldPath(kStorageSlot, mission));
        check(same && pack.rfind("GALAXY_PACK 1\n", 0) == 0,
              "galaxy pack: its four places, byte for byte, behind the header");
        std::string flipped = pack;
        flipped[pack.size() / 2] = static_cast<char>(flipped[pack.size() / 2] ^ 0x20);
        const std::string otherVersion = "GALAXY_PACK 2\n" + pack.substr(std::string("GALAXY_PACK 1\n").size());
        // A place a galaxy does not have, with a correct footer: refused all the same.
        std::string foreign = "GALAXY_PACK 1\n";
        const std::string world = unpacked ? unpacked->worlds.at(11) : std::string();
        foreign += "world 19 " + std::to_string(world.size()) + "\n" + world;
        std::uint64_t hash = 1469598103934665603ULL;
        for (const unsigned char c : foreign) hash = (hash ^ c) * 1099511628211ULL;
        char footer[32];
        std::snprintf(footer, sizeof(footer), "END %016llx\n", static_cast<unsigned long long>(hash));
        const std::string foreignPack = foreign + footer;
        std::string notAWorld = "GALAXY_PACK 1\nworld 11 4\nVWR1";
        hash = 1469598103934665603ULL;
        for (const unsigned char c : notAWorld) hash = (hash ^ c) * 1099511628211ULL;
        std::snprintf(footer, sizeof(footer), "END %016llx\n", static_cast<unsigned long long>(hash));
        notAWorld += footer;
        check(!UnpackGalaxy(flipped) && !UnpackGalaxy(pack.substr(0, pack.size() - 3)) && !UnpackGalaxy(otherVersion) &&
                  !UnpackGalaxy(foreignPack) && !UnpackGalaxy(notAWorld) && !UnpackGalaxy(""),
              "galaxy pack: a changed byte, a cut, another version, a foreign place or a broken world is refused");

        const auto exported = ExportGalaxy(kStorageSlot);
        const auto listed = ListSharedGalaxies();
        check(exported == SharedWorldsDir() / "player77-galaxy.galaxy" && GalaxyExportName(0) == "A-galaxy.galaxy" &&
                  readAll(exported) == pack && std::find(listed.begin(), listed.end(), exported) != listed.end() &&
                  ListSharedWorlds().end() == std::find(ListSharedWorlds().begin(), ListSharedWorlds().end(), exported),
              "galaxy export: the pack in the shared folder, listed as a galaxy and not as a world");
        const auto again = ExportGalaxy(kStorageSlot);
        check(again == SharedWorldsDir() / "player77-galaxy-2.galaxy" && readAll(exported) == pack && readAll(again) == pack,
              "galaxy export: a second export is numbered, the first stays");
        std::filesystem::remove(again, error);

        // The other gamer has a galaxy of their own: hub 20 and mission 21.
        std::filesystem::create_directories(GalaxyDir(kBrowserSlot), error);
        CreateGalaxyStarterWorld(20).saveToFile(GalaxyWorldPath(kBrowserSlot, 20));
        CreateGalaxyStarterWorld(21).saveToFile(GalaxyWorldPath(kBrowserSlot, 21));
        const auto broken = SharedWorldsDir() / "broken.galaxy";
        { std::ofstream out(broken, std::ios::binary); out << flipped; }
        const int refused = ImportGalaxy(kBrowserSlot, broken);
        const bool untouched = std::filesystem::exists(GalaxyWorldPath(kBrowserSlot, 20)) &&
                               !std::filesystem::exists(GalaxyWorldPath(kBrowserSlot, 1));
        const int imported = ImportGalaxy(kBrowserSlot, exported);
        bool equal = imported == 4;
        for (const int mission : {1, 10, 11, 199})
            equal = equal && readAll(GalaxyWorldPath(kBrowserSlot, mission)) == readAll(GalaxyWorldPath(kStorageSlot, mission));
        check(refused == -1 && untouched && equal && !std::filesystem::exists(GalaxyWorldPath(kBrowserSlot, 20)) &&
                  BackupCount(GalaxyWorldPath(kBrowserSlot, 20)) == 1 && BackupCount(GalaxyWorldPath(kBrowserSlot, 21)) == 1,
              "galaxy import: a damaged pack touches nothing; a good one replaces the galaxy, the old places in its backups");

        // The galaxy screen: Export, Import's list, a row clicked twice.
        GalaxyBrowserScreen screen;
        screen.Refresh(kBrowserSlot);
        const auto click = [&screen](const GalaxyEggbert::Game::QuadBatch::Rect& r) {
            const int x = int((r.x0 + r.x1) * 0.5f), y = int((r.y0 + r.y1) * 0.5f);
            (void)screen.Update(MouseState(x, y, 0, ButtonState::Pressed, ButtonState::Released, ButtonState::Released,
                                           ButtonState::Released, ButtonState::Released));
            return screen.Update(MouseState(x, y, 0, ButtonState::Released, ButtonState::Released,
                                            ButtonState::Released, ButtonState::Released, ButtonState::Released));
        };
        (void)click(GalaxyBrowserScreen::ExportRect());
        const bool exportedHere = screen.Message() == "Galaxy exported to " +
                                                          (SharedWorldsDir() / "player78-galaxy.galaxy").generic_string() + "." &&
                                  std::filesystem::exists(SharedWorldsDir() / "player78-galaxy.galaxy");
        std::filesystem::remove(broken, error);
        std::filesystem::remove(SharedWorldsDir() / "player78-galaxy.galaxy", error);
        std::filesystem::remove_all(CustomWorldsDir(kStorageSlot), error);
        // The pack exported from slot 77 is the one listed; its galaxy is gone, slot 78 differs now.
        std::filesystem::remove(GalaxyWorldPath(kBrowserSlot, 199), error);
        screen.Refresh(kBrowserSlot);
        (void)click(GalaxyBrowserScreen::ImportRect());
        const bool listing = screen.ImportMode();
        const auto row = GalaxyBrowserScreen::ImportRowRect(0);
        const auto once = click(row);
        const bool notYet = !once.imported && !std::filesystem::exists(GalaxyWorldPath(kBrowserSlot, 199));
        const auto twice = click(row);
        check(exportedHere && listing && notYet && twice.imported && !screen.ImportMode() &&
                  std::filesystem::exists(GalaxyWorldPath(kBrowserSlot, 199)) &&
                  screen.Message() == "player77-galaxy.galaxy: 4 places imported (Backups brings back the old ones).",
              "galaxy screen: Export writes the pack; Import lists it, a row's second click makes it this galaxy");
        std::filesystem::remove(exported, error);

        // The list pages by eight; a row's bin deletes its file on the second click.
        for (int i = 0; i < 10; ++i)
        {
            std::ofstream out(SharedWorldsDir() / ("page-" + std::to_string(i) + ".galaxy"), std::ios::binary);
            out << "x";
        }
        GalaxyBrowserScreen pages;
        pages.Refresh(kBrowserSlot);
        const auto tap = [&pages](const GalaxyEggbert::Game::QuadBatch::Rect& r) {
            const int x = int((r.x0 + r.x1) * 0.5f), y = int((r.y0 + r.y1) * 0.5f);
            (void)pages.Update(MouseState(x, y, 0, ButtonState::Pressed, ButtonState::Released, ButtonState::Released,
                                          ButtonState::Released, ButtonState::Released));
            return pages.Update(MouseState(x, y, 0, ButtonState::Released, ButtonState::Released,
                                           ButtonState::Released, ButtonState::Released, ButtonState::Released));
        };
        (void)tap(GalaxyBrowserScreen::ImportRect());
        const bool ten = pages.ImportCount() == 10 && pages.ImportPage() == 0;
        (void)tap(GalaxyBrowserScreen::ImportNextRect());
        const bool second = pages.ImportPage() == 1;
        (void)tap(GalaxyBrowserScreen::ImportNextRect());
        const bool last = pages.ImportPage() == 1;
        (void)tap(GalaxyBrowserScreen::ImportPrevRect());
        check(ten && second && last && pages.ImportPage() == 0,
              "galaxy import list: ten files on two pages, the arrows step between them and stop at the ends");
        const auto armBin = tap(GalaxyBrowserScreen::ImportDeleteRect(0));
        const bool armedKept = pages.ImportCount() == 10 && !armBin.imported;
        (void)tap(GalaxyBrowserScreen::WorldRect(3)); // any other click disarms
        (void)tap(GalaxyBrowserScreen::ImportDeleteRect(0));
        const bool rearmedKept = pages.ImportCount() == 10;
        (void)tap(GalaxyBrowserScreen::ImportDeleteRect(0));
        check(armedKept && rearmedKept && pages.ImportCount() == 9 && ListSharedGalaxies().size() == 9 &&
                  pages.Message().find(" deleted.") != std::string::npos,
              "galaxy import list: a row's bin deletes the file on the second click in a row, nothing on the first");
        bool layout = true;
        for (int i = 0; i < GalaxyBrowserScreen::kImportRowsShown; ++i)
        {
            const auto row = GalaxyBrowserScreen::ImportRowRect(i), bin = GalaxyBrowserScreen::ImportDeleteRect(i);
            layout = layout && bin.x0 > row.x0 && bin.x1 <= row.x1 && bin.y0 >= row.y0 && bin.y1 <= row.y1;
        }
        const auto lastRow = GalaxyBrowserScreen::ImportRowRect(GalaxyBrowserScreen::kImportRowsShown - 1);
        layout = layout && GalaxyBrowserScreen::ImportPrevRect().y0 >= lastRow.y1 &&
                 GalaxyBrowserScreen::ImportNextRect().y1 <= 350.0f &&
                 GalaxyBrowserScreen::ImportPrevRect().x1 < GalaxyBrowserScreen::ImportNextRect().x0;
        check(layout, "galaxy import list: the bins in their rows, the arrows under the last row in the panel");
        for (const auto& file : ListSharedGalaxies()) std::filesystem::remove(file, error);
        std::filesystem::remove(SharedWorldsDir(), error); // only when nothing else is in it
    }

    // A galaxy's name: kept beside it, in its pack, its export's file name, typed on its screen.
    {
        using Microsoft::Xna::Framework::Input::Keys;
        std::filesystem::remove_all(CustomWorldsDir(kStorageSlot), error);
        std::filesystem::remove_all(CustomWorldsDir(kBrowserSlot), error);
        std::filesystem::create_directories(GalaxyDir(kStorageSlot), error);
        CreateGalaxyStarterWorld(1).saveToFile(GalaxyWorldPath(kStorageSlot, 1));
        check(SanitizeGalaxyName("  Star   Fields!? ") == "Star Fields" &&
                  SanitizeGalaxyName("Blupi's land-2") == "Blupi's land-2" &&
                  SanitizeGalaxyName(std::string(40, 'x')).size() == kGalaxyNameMax && SanitizeGalaxyName("!!!").empty(),
              "galaxy name: letters, digits, single spaces, '-' and '\'', at most 24");
        const bool none = GalaxyName(kStorageSlot).empty() && GalaxyExportName(kStorageSlot) == "player77-galaxy.galaxy";
        (void)SetGalaxyName(kStorageSlot, "Star Fields");
        check(none && GalaxyName(kStorageSlot) == "Star Fields" && GalaxyExportName(kStorageSlot) == "Star-Fields.galaxy",
              "galaxy name: kept beside the galaxy, its export named after it");
        const auto exported = ExportGalaxy(kStorageSlot);
        const auto unpacked = UnpackGalaxy([&] {
            std::ifstream in(exported, std::ios::binary);
            return std::string((std::istreambuf_iterator<char>(in)), {});
        }());
        (void)SetGalaxyName(kBrowserSlot, "Old name");
        const int places = ImportGalaxy(kBrowserSlot, exported);
        check(exported.filename() == "Star-Fields.galaxy" && unpacked && unpacked->name == "Star Fields" &&
                  PeekGalaxyPackName(exported) == "Star Fields" && places == 1 && GalaxyName(kBrowserSlot) == "Star Fields",
              "galaxy name: in the pack, read from it without unpacking, the receiver's galaxy takes it");
        (void)SetGalaxyName(kStorageSlot, "");
        const auto plain = ExportGalaxy(kStorageSlot);
        (void)ImportGalaxy(kBrowserSlot, plain);
        check(GalaxyName(kStorageSlot).empty() && PeekGalaxyPackName(plain).empty() && GalaxyName(kBrowserSlot).empty(),
              "galaxy name: forgotten when emptied; a pack without one leaves the receiver's galaxy unnamed");
        std::filesystem::remove(exported, error);
        std::filesystem::remove(plain, error);

        GalaxyBrowserScreen named;
        named.Refresh(kStorageSlot);
        const auto tapName = [&named](const GalaxyEggbert::Game::QuadBatch::Rect& r) {
            const int x = int((r.x0 + r.x1) * 0.5f), y = int((r.y0 + r.y1) * 0.5f);
            (void)named.Update(MouseState(x, y, 0, ButtonState::Pressed, ButtonState::Released, ButtonState::Released,
                                          ButtonState::Released, ButtonState::Released));
            return named.Update(MouseState(x, y, 0, ButtonState::Released, ButtonState::Released,
                                           ButtonState::Released, ButtonState::Released, ButtonState::Released));
        };
        (void)tapName(GalaxyBrowserScreen::NameRect());
        const bool typing = named.Naming();
        for (const auto& [key, shift] : std::vector<std::pair<Keys, bool>>{
                 {Keys::S, true}, {Keys::K, false}, {Keys::Y, false}, {Keys::Space, false}, {Keys::D2, false},
                 {Keys::X, false}, {Keys::Back, false}})
            named.TypeKey(key, shift);
        named.TypeKey(Keys::Enter, false);
        check(typing && !named.Naming() && named.Name() == "Sky 2" && GalaxyName(kStorageSlot) == "Sky 2",
              "galaxy screen: a click on the title, typed keys (Shift, digits, Backspace), Enter keeps the name");
        (void)tapName(GalaxyBrowserScreen::NameRect());
        named.TypeKey(Keys::Z, false);
        const auto idle = MouseState(400, 240, 0, ButtonState::Released, ButtonState::Released, ButtonState::Released,
                                     ButtonState::Released, ButtonState::Released);
        (void)named.Update(idle, true); // Escape
        const bool cancelled = !named.Naming() && named.Name() == "Sky 2" && GalaxyName(kStorageSlot) == "Sky 2";
        (void)named.Update(idle, false);
        (void)tapName(GalaxyBrowserScreen::NameRect());
        named.TypeKey(Keys::Back, false);
        (void)tapName({650.0f, 200.0f, 660.0f, 210.0f}); // a click off the keyboard keeps it
        check(cancelled && !named.Naming() && GalaxyName(kStorageSlot) == "Sky", // trimmed
              "galaxy screen: Escape leaves the name as it was; a click elsewhere keeps the one typed");
        // The keyboard on the screen (2026-11-10): its keys type, OK keeps the name.
        (void)tapName(GalaxyBrowserScreen::NameRect());
        const auto& keys = named.Keyboard().Keys();
        const auto keyRect = [&](GalaxyEggbert::Game::ScreenKeyboard::Kind kind, char c) {
            for (const auto& key : keys)
                if (key.kind == kind && key.c == c) return key.rect;
            return keys.front().rect;
        };
        using Kind = GalaxyEggbert::Game::ScreenKeyboard::Kind;
        (void)tapName(keyRect(Kind::Space, ' '));
        (void)tapName(keyRect(Kind::Char, '3'));
        (void)tapName(keyRect(Kind::Done, 0));
        check(!named.Naming() && GalaxyName(kStorageSlot) == "Sky 3",
              "galaxy screen: the keyboard on the screen types (Space, 3) and OK keeps the name");
        std::filesystem::remove_all(CustomWorldsDir(kStorageSlot), error);
        std::filesystem::remove_all(CustomWorldsDir(kBrowserSlot), error);
        std::filesystem::remove(SharedWorldsDir(), error);
    }

    // The player's recordings made in a galaxy travel in its pack, to the receiver's shared demos.
    {
        using namespace GalaxyEggbert::Game;
        constexpr int kGamer = 2; // a real gamer (galaxy paths name players A-C); verify-run is scratch
        const auto readAll = [](const std::filesystem::path& path) {
            std::ifstream in(path, std::ios::binary);
            return std::string((std::istreambuf_iterator<char>(in)), {});
        };
        std::filesystem::remove_all(CustomWorldsDir(kGamer), error);
        std::filesystem::remove_all(UserDemoDir(), error);
        std::filesystem::create_directories(GalaxyDir(kGamer), error);
        CreateGalaxyStarterWorld(1).saveToFile(GalaxyWorldPath(kGamer, 1));
        CreateGalaxyStarterWorld(10).saveToFile(GalaxyWorldPath(kGamer, 10));
        const auto demoIn = [](const std::string& worldPath, int mission) {
            DemoRecording demo;
            demo.title = "a recording";
            demo.log.start.build = "build";
            demo.log.start.worldPath = worldPath;
            demo.log.start.worldHash = ResumeLog::HashBytes("world");
            demo.log.start.mission = mission;
            ResumeInput input;
            input.viewportWidth = 800;
            input.viewportHeight = 480;
            demo.log.Append(input);
            demo.endSignature = HashDemoSignature({{"Blupi", "1 2 3"}});
            return demo.Serialize();
        };
        std::filesystem::create_directories(UserDemoDir(), error);
        const std::string ours = demoIn(GalaxyWorldPath(kGamer, 10).generic_string(), 10);
        { std::ofstream out(UserDemoPath(4), std::ios::binary); out << ours; }
        { std::ofstream out(UserDemoPath(5), std::ios::binary); out << demoIn("worlds3d/world011.vwr", 11); }
        { std::ofstream out(UserDemoPath(6), std::ios::binary); out << demoIn(GalaxyWorldPath(1, 10).generic_string(), 10); }
        const auto recordings = GalaxyRecordings(kGamer);
        const std::string pack = PackGalaxy(kGamer);
        const auto unpacked = UnpackGalaxy(pack);
        check(recordings == std::vector<int>{4} && unpacked && unpacked->worlds.size() == 2 &&
                  unpacked->demos.size() == 1 && unpacked->demos.count(4) == 1 && unpacked->demos.at(4) == ours,
              "galaxy pack: the player's recordings made in this galaxy go with it, not the campaign's or another galaxy's");
        // A demo of another simulation revision is left out; the galaxy is kept.
        std::string body = pack.substr(0, pack.size() - 21);
        DemoRecording staleDemo;
        check(ParseDemo(ours, kDemoSimulationRevision, staleDemo) == DemoLoadStatus::Ok,
              "the galaxy recording parses before changing its simulation revision");
        staleDemo.simulationRevision = kDemoSimulationRevision + 1;
        const std::string stale = staleDemo.Serialize();
        body += "demo 7 " + std::to_string(stale.size()) + "\n" + stale;
        std::uint64_t hash = 1469598103934665603ULL;
        for (const unsigned char c : body) hash = (hash ^ c) * 1099511628211ULL;
        char footer[32];
        std::snprintf(footer, sizeof(footer), "END %016llx\n", static_cast<unsigned long long>(hash));
        const auto withStale = UnpackGalaxy(body + footer);
        check(withStale && withStale->worlds.size() == 2 && withStale->demos.size() == 1 && withStale->demos.count(7) == 0,
              "galaxy pack: a demo this game cannot play is left out, the galaxy kept");
        const auto file = SharedWorldsDir() / "friend.galaxy";
        std::filesystem::create_directories(SharedWorldsDir(), error);
        { std::ofstream out(file, std::ios::binary); out << pack; }
        int demos = -1;
        const int places = ImportGalaxy(kBrowserSlot, file, &demos);
        const auto shared = std::filesystem::path(SharedDemoDir()) / "friend-demo-4.gdemo";
        check(places == 2 && demos == 1 && readAll(shared) == ours && readAll(UserDemoPath(4)) == ours &&
                  std::filesystem::exists(UserDemoPath(5)),
              "galaxy import: its demos land among the shared demos (friend-demo-4.gdemo), no position overwritten");
        std::filesystem::remove(file, error);
        std::filesystem::remove(SharedWorldsDir(), error);
        std::filesystem::remove_all(UserDemoDir(), error);
        std::filesystem::remove_all(CustomWorldsDir(kGamer), error);
    }

    std::filesystem::remove_all(CustomWorldsDir(kStorageSlot), error);
    std::filesystem::remove_all(CustomWorldsDir(kBrowserSlot), error);
    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
