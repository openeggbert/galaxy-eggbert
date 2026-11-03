#include <GalaxyEggbert/Editor/AutoTile.hpp>
#include <GalaxyEggbert/Editor/BoxRegion.hpp>
#include <GalaxyEggbert/Editor/PaletteCategories.hpp>
#include <GalaxyEggbert/Game/PlateRotationMetadata.hpp>
#include <GalaxyEggbert/BigDecorRecord.hpp>
#include <GalaxyEggbert/MoveObjectRecord.hpp>
#include <GalaxyEggbert/Editor/EditCommandStack.hpp>
#include <GalaxyEggbert/Editor/VoxelRaycast.hpp>
#include <GalaxyEggbert/Editor/WorldEditor.hpp>

#include <GalaxyEggbert/BigDecorRecord.hpp>
#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <cmath>
#include <cstdio>
#include <iostream>

int main()
{
    using namespace GalaxyEggbert::Game;
    using namespace GalaxyEggbert::Editor;
    using GalaxyEggbert::Worlds::Block;
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
    // A rock placed is re-tiled to its variant for what borders it (ED-0007): any icon of the rock
    // rows of the original's table_adapt_decor.
    const auto isRockVariant = [](std::uint16_t icon)
    {
        for (std::size_t i = 0; i < 32; ++i)
        {
            if (AutoTile::kAdaptDecor[i] == static_cast<std::int16_t>(icon))
            {
                return true;
            }
        }
        return false;
    };

    const MouseState restMouse(
        0, 0, 0, ButtonState::Released, ButtonState::Released,
        ButtonState::Released, ButtonState::Released, ButtonState::Released);

    {
        World world;
        WorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        CnaEngine::Graphics::Camera camera;
        editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, world);
        check(camera.Position().X == 50.0f &&
                  camera.Position().Y == 15.0f &&
                  camera.Position().Z == 50.0f,
              "EnterEditing sets the requested camera position");
        check(editor.CameraTarget().Z < camera.Position().Z &&
                  editor.CameraTarget().Y < camera.Position().Y,
              "the default camera looks forward and slightly downward");

        editor.Update(KeyboardState{Keys::W}, restMouse, 1.0f, 800, 480, camera, world);
        check(std::fabs(camera.Position().X - 50.0f) < 0.001f &&
                  camera.Position().Z < 40.0f,
              "W flies forward without sideways drift");
    }

    {
        World world;
        WorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        CnaEngine::Graphics::Camera camera;
        editor.Update(KeyboardState{Keys::D}, restMouse, 1.0f, 800, 480, camera, world);
        check(camera.Position().X > 60.0f &&
                  std::fabs(camera.Position().Z - 50.0f) < 0.001f,
              "D strafes right without forward drift");

        editor.Update(KeyboardState{Keys::Space}, restMouse, 1.0f, 800, 480, camera, world);
        check(camera.Position().Y > 25.0f,
              "Space moves the editor camera upward");
    }

    {
        World world;
        WorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        CnaEngine::Graphics::Camera camera;
        editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, world);
        const float defaultFov = camera.FieldOfView();
        const MouseState wheelUp(
            0, 0, 120, ButtonState::Released, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, wheelUp, 0.0f, 800, 480, camera, world);
        check(camera.FieldOfView() < defaultFov,
              "the mouse wheel zooms in by narrowing the field of view");
    }

    {
        World world;
        world.setBlock(50, 4, 50, Block::make(1));
        const RaycastHit down =
            Raycast(world, 50.0f, 10.0f, 50.0f, 0.0f, -1.0f, 0.0f, 200.0f);
        check(down.hit && down.x == 50 && down.y == 4 && down.z == 50,
              "a downward voxel ray reports the expected block");
        check(down.normalX == 0 && down.normalY == 1 && down.normalZ == 0,
              "the downward hit reports the upward face normal");
        check(std::fabs(down.distance - 5.5f) < 0.001f,
              "ray distance respects the engine's centred-block convention");

        check(!Raycast(
                   world, 50.0f, 10.0f, 50.0f,
                   0.0f, 1.0f, 0.0f, 200.0f).hit,
              "a ray aimed away from all blocks misses");
    }

    {
        World world;
        world.setBlock(60, 5, 50, Block::make(1));
        const RaycastHit horizontal =
            Raycast(world, 50.0f, 5.0f, 50.0f, 1.0f, 0.0f, 0.0f, 200.0f);
        check(horizontal.hit && horizontal.x == 60 && horizontal.normalX == -1,
              "a horizontal ray reports the hit cell and incoming face");
        check(!Raycast(
                   world, 50.0f, 5.0f, 50.0f,
                   1.0f, 0.0f, 0.0f, 2.0f).hit,
              "maxDistance prevents a farther ray hit");
    }

    World slab;
    for (std::uint16_t z = 0; z < 100; ++z)
    {
        for (std::uint16_t y = 0; y < 4; ++y)
        {
            slab.setBlock(50, y, z, Block::make(1));
        }
    }
    constexpr float kDefaultPitch = -0.35f;
    const float cosPitch = std::cos(kDefaultPitch);
    const RaycastHit slabHit =
        Raycast(slab, 50.0f, 10.0f, 50.0f,
                0.0f, std::sin(kDefaultPitch), -cosPitch, 200.0f);
    const auto placeX =
        static_cast<std::uint16_t>(static_cast<int>(slabHit.x) + slabHit.normalX);
    const auto placeY =
        static_cast<std::uint16_t>(static_cast<int>(slabHit.y) + slabHit.normalY);
    const auto placeZ =
        static_cast<std::uint16_t>(static_cast<int>(slabHit.z) + slabHit.normalZ);
    check(slabHit.hit && slab.getBlock(placeX, placeY, placeZ).isAir(),
          "the reference editor ray has an empty adjacent placement cell");

    {
        World world = slab;
        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        CnaEngine::Graphics::Camera camera;

        const MouseState left(
            400, 200, 0, ButtonState::Pressed, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, left, 0.0f, 800, 480, camera, world);
        check(isRockVariant(world.getBlock(placeX, placeY, placeZ).type()),
              "left click places the selected block in the preview cell (its rock variant, ED-0007)");
        check(editor.ConsumeNeedsPresentationRebuild(),
              "block placement requests a presentation rebuild");
        editor.Update(KeyboardState{}, left, 0.0f, 800, 480, camera, world);
        check(!editor.ConsumeNeedsPresentationRebuild(),
              "holding the placement button remains edge-triggered");

        const MouseState middle(
            400, 200, 0, ButtonState::Released, ButtonState::Pressed,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, middle, 0.0f, 800, 480, camera, world);
        check(world.getBlock(placeX, placeY, placeZ).isAir(),
              "middle click removes the aimed-at block");
        check(!world.getBlock(slabHit.x, slabHit.y, slabHit.z).isAir(),
              "removing the nearest block preserves the slab below it");
    }

    // ED-0001 (the original's variant cycling, MR-0003): a click on the block the previous click
    // placed cycles it through its family's variants; another click places the current variant;
    // Ctrl places without cycling; each step undoes on its own.
    {
        using GalaxyEggbert::Editor::FindVariantFamily;
        using GalaxyEggbert::Editor::VariantCycler;
        check(FindVariantFamily(126) >= 0 && FindVariantFamily(126) == FindVariantFamily(135) &&
                  FindVariantFamily(35) < 0,
              "the fans 126-135 are one family; the rock pile has none");
        VariantCycler cycler;
        check(cycler.IconToPlace(126, false) == 126, "a family places the palette entry's icon until cycled");
        check(!cycler.Cycle(5, 5, 5, 126, 126), "no cycle before anything was placed there");
        cycler.NotePlaced(5, 5, 5);
        check(!cycler.Cycle(6, 5, 5, 126, 126), "no cycle on another cell");
        check(cycler.Cycle(5, 5, 5, 126, 126).value_or(0) == 129, "a click on the placed cell cycles it");
        check(cycler.Cycle(5, 5, 5, 129, 126).value_or(0) == 132 && cycler.Cycle(5, 5, 5, 132, 126).value_or(0) == 135 &&
                  cycler.Cycle(5, 5, 5, 135, 126).value_or(0) == 126,
              "the variants go round and wrap to the first");
        check(!cycler.Cycle(5, 5, 5, 129, 126), "a cell no longer holding the current variant is not cycled");
        cycler.Cycle(5, 5, 5, 126, 126);
        check(cycler.IconToPlace(126, false) == 129, "the next new cell gets the variant last cycled to");

        World world = slab;
        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        CnaEngine::Graphics::Camera camera;
        editor.Palette().SelectBlock(126);
        const MouseState press(
            400, 200, 0, ButtonState::Pressed, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        const auto click = [&](KeyboardState keys)
        {
            editor.Update(keys, press, 0.0f, 800, 480, camera, world);
            editor.Update(keys, restMouse, 0.0f, 800, 480, camera, world);
        };
        click(KeyboardState{});
        check(world.getBlock(placeX, placeY, placeZ).type() == 126, "the first click places the fan");
        click(KeyboardState{});
        check(world.getBlock(placeX, placeY, placeZ).type() == 129,
              "a second click without moving (on the placed fan) cycles it: 126 -> 129");
        check(world.getBlock(placeX, placeY, placeZ + 1).isAir() && world.getBlock(placeX, placeY + 1, placeZ).isAir(),
              "cycling places nothing new beside it");
        click(KeyboardState{Keys::RightControl});
        bool stacked = false;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dz = -1; dz <= 1; ++dz)
                if ((dy != 0 || dz != 0) &&
                    world.getBlock(placeX, static_cast<std::uint16_t>(placeY + dy), static_cast<std::uint16_t>(placeZ + dz))
                            .type() == 129)
                    stacked = true;
        check(world.getBlock(placeX, placeY, placeZ).type() == 129 && stacked,
              "Ctrl+click places another (the current variant) instead of cycling");
        editor.Update(KeyboardState{Keys::U}, restMouse, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{Keys::U}, restMouse, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, world);
        check(world.getBlock(placeX, placeY, placeZ).type() == 126, "undo takes a cycle step back on its own");
    }

    // ED-0006: the BigDecor families cycle the same way: a click again where the previous click put
    // a tree (the same cell: BigDecor are no voxels) makes it the next tree.
    {
        World world = slab;
        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        CnaEngine::Graphics::Camera camera;
        editor.Palette().SelectBigDecor(20);
        const MouseState press(
            400, 200, 0, ButtonState::Pressed, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        const auto treeAt = [&]() -> int
        {
            for (const auto& record : GalaxyEggbert::CollectBigDecor(world))
                if (record.x == placeX && record.y == placeY && record.z == placeZ)
                    return record.icon;
            return -1;
        };
        editor.Update(KeyboardState{}, press, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, world);
        const int first = treeAt();
        editor.Update(KeyboardState{}, press, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, world);
        check(first == 20 && treeAt() == 21, "a second click on the tree just placed makes it the next tree (20 -> 21)");
    }

    {
        World world = slab;
        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        CnaEngine::Graphics::Camera camera;
        const auto tapDelete = [&]()
        {
            const MouseState down(
                30, 30, 0, ButtonState::Pressed, ButtonState::Released,
                ButtonState::Released, ButtonState::Released, ButtonState::Released);
            const MouseState up(
                30, 30, 0, ButtonState::Released, ButtonState::Released,
                ButtonState::Released, ButtonState::Released, ButtonState::Released);
            editor.Update(KeyboardState{}, down, 0.0f, 800, 480, camera, world);
            editor.Update(KeyboardState{}, up, 0.0f, 800, 480, camera, world);
        };
        const auto pressKey = [&](Keys key)
        {
            editor.Update(KeyboardState{key}, restMouse, 0.0f, 800, 480, camera, world);
            editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, world);
        };

        tapDelete();
        check(world.getBlock(slabHit.x, slabHit.y, slabHit.z).isAir(),
              "the visible delete glyph removes the aimed-at block");
        check(editor.ConsumeNeedsPresentationRebuild(),
              "toolbar block removal requests a presentation rebuild");
        pressKey(Keys::U);
        check(world.getBlock(slabHit.x, slabHit.y, slabHit.z).type() == 1,
              "undo restores a block removed by the visible delete glyph");
        pressKey(Keys::R);
        check(world.getBlock(slabHit.x, slabHit.y, slabHit.z).isAir(),
              "redo removes the toolbar target again");
    }

    {
        World world;
        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        CnaEngine::Graphics::Camera camera;
        const MouseState left(
            400, 200, 0, ButtonState::Pressed, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, left, 0.0f, 800, 480, camera, world);

        int solidCount = 0;
        for (std::uint16_t x = 0; x < world.blocksPerAxis(); ++x)
        {
            for (std::uint16_t y = 0; y < world.blocksPerAxis(); ++y)
            {
                for (std::uint16_t z = 0; z < world.blocksPerAxis(); ++z)
                {
                    if (!world.getBlock(x, y, z).isAir()) ++solidCount;
                }
            }
        }
        check(solidCount == 1,
              "an all-air world accepts its first block on the y=0 build plane");
    }

    {
        constexpr const char* kSavePath = "verify_ge_world_editor_core.vwr";
        World world;
        world.setBlock(50, 4, 50, Block::make(42));
        WorldEditor editor;
        editor.SetWorldPath(kSavePath);
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        CnaEngine::Graphics::Camera camera;
        editor.Update(
            KeyboardState{Keys::Enter}, restMouse, 0.0f, 800, 480,
            camera, world);
        check(World::loadFromFile(kSavePath).getBlock(50, 4, 50).type() == 42,
              "Enter saves the current world and it round-trips");
        std::remove(kSavePath);
    }

    {
        World world;
        world.setBlock(10, 5, 10, Block::make(1));
        EditCommandStack stack;
        EditCommand edit;
        edit.blockChanges.push_back(
            {10, 5, 10, Block::make(1), Block::make(2)});
        world.setBlock(10, 5, 10, Block::make(2));
        stack.Push(edit);

        check(stack.Undo(world) && world.getBlock(10, 5, 10).type() == 1,
              "command-stack undo restores a block's previous value");
        check(stack.Redo(world) && world.getBlock(10, 5, 10).type() == 2,
              "command-stack redo restores a block's new value");

        for (int index = 0; index < 250; ++index)
        {
            EditCommand command;
            command.blockChanges.push_back(
                {0, 0, 0, Block::air(), Block::make(1)});
            stack.Push(command);
        }
        check(stack.UndoCount() == 200,
              "the undo history remains capped at 200 commands");
    }

    {
        const BoxRegion normalized =
            NormalizeAndClamp(5, 20, 8, 2, 10, 30, 100);
        check(normalized.minX == 2 && normalized.maxX == 5 &&
                  normalized.minY == 10 && normalized.maxY == 20 &&
                  normalized.minZ == 8 && normalized.maxZ == 30,
              "box corners normalize independently on all three axes");
        const BoxRegion clamped =
            NormalizeAndClamp(-5, 50, 105, 3, 200, 99, 100);
        check(clamped.minX == 0 && clamped.maxY == 99 &&
                  clamped.minZ == 99 && clamped.maxZ == 99,
              "box corners clamp to valid world boundaries");
    }

    {
        World world = slab;
        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        CnaEngine::Graphics::Camera camera;

        editor.Update(
            KeyboardState{Keys::F}, restMouse, 0.0f, 800, 480,
            camera, world);
        editor.Update(
            KeyboardState{Keys::Space}, restMouse, 0.3f, 800, 480,
            camera, world);
        const RaycastHit second =
            Raycast(world, 50.0f, 14.5f, 50.0f,
                    0.0f, std::sin(kDefaultPitch), -cosPitch, 200.0f);
        editor.Update(
            KeyboardState{Keys::F}, restMouse, 0.0f, 800, 480,
            camera, world);

        const BoxRegion region =
            NormalizeAndClamp(
                slabHit.x, slabHit.y, slabHit.z,
                second.x, second.y, second.z,
                static_cast<int>(world.blocksPerAxis()));
        bool allFilled = true;
        int volume = 0;
        for (std::uint16_t x = region.minX; x <= region.maxX; ++x)
        {
            for (std::uint16_t y = region.minY; y <= region.maxY; ++y)
            {
                for (std::uint16_t z = region.minZ; z <= region.maxZ; ++z)
                {
                    ++volume;
                    allFilled &= isRockVariant(world.getBlock(x, y, z).type());
                }
            }
        }
        check(volume > 1 && allFilled,
              "the second F press fills the complete 3D box as one action");

        editor.Update(
            KeyboardState{Keys::U}, restMouse, 0.0f, 800, 480,
            camera, world);
        check(world.getBlock(slabHit.x, slabHit.y, slabHit.z).type() == 1 &&
                  world.getBlock(second.x, second.y, second.z).type() == 1,
              "one undo restores both ends of a box fill");
    }

    {
        World world;
        world.setSkyRegion(5);
        EditCommandStack stack;
        EditCommand command;
        command.kind = EditCommand::Kind::SkyRegionEdit;
        command.skyRegionBefore = 5;
        command.skyRegionAfter = 6;
        world.setSkyRegion(6);
        stack.Push(command);
        check(stack.Undo(world) && world.skyRegion() == 5,
              "sky-region command undo restores the previous background");
        check(stack.Redo(world) && world.skyRegion() == 6,
              "sky-region command redo restores the next background");
    }

    {
        World world;
        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        CnaEngine::Graphics::Camera camera;
        const auto pressKey = [&](Keys key)
        {
            editor.Update(
                KeyboardState{key}, restMouse, 0.0f, 800, 480,
                camera, world);
            editor.Update(
                KeyboardState{}, restMouse, 0.0f, 800, 480,
                camera, world);
        };

        // OR-0014: M steps the world's music (none, 1-10, none), undoable.
        pressKey(Keys::M);
        pressKey(Keys::M);
        check(world.musicTrack() == 2, "M steps the world's background music (none -> 1 -> 2)");
        pressKey(Keys::U);
        check(world.musicTrack() == 1, "undo restores the previous music");
        for (int i = 0; i < 10; ++i)
            pressKey(Keys::M);
        check(world.musicTrack() == 0, "after the tenth track M comes back to no music");
        pressKey(Keys::Left);
        check(world.skyRegion() == 31,
              "Left wraps the background selection from 0 to 31");
        pressKey(Keys::Right);
        check(world.skyRegion() == 0,
              "Right wraps the background selection from 31 to 0");
        pressKey(Keys::U);
        check(world.skyRegion() == 31,
              "undo restores the previous background selection");
        pressKey(Keys::R);
        check(world.skyRegion() == 0,
              "redo restores the wrapped background selection");
    }

    const auto clickStop = [&](WorldEditor& editor, World& world,
                               CnaEngine::Graphics::Camera& camera)
    {
        const MouseState down(
            766, 451, 0, ButtonState::Pressed, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        const MouseState up(
            766, 451, 0, ButtonState::Released, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, down, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{}, up, 0.0f, 800, 480, camera, world);
    };

    {
        World world;
        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        CnaEngine::Graphics::Camera camera;
        clickStop(editor, world, camera);
        check(editor.IsBrowsing(),
              "Stop leaves immediately when the world is unchanged");
    }

    {
        World world;
        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        CnaEngine::Graphics::Camera camera;
        editor.Update(
            KeyboardState{Keys::Right}, restMouse, 0.0f, 800, 480,
            camera, world);
        clickStop(editor, world, camera);
        check(!editor.IsBrowsing(),
              "the first Stop press protects an unsaved world");
        clickStop(editor, world, camera);
        check(editor.IsBrowsing(),
              "a second Stop press confirms discarding unsaved changes");
    }

    // ED-0007: the original editor's auto-tiling (AutoTile.hpp, Decor::AdaptBorder). Dice that
    // never swap a variant for its twin (Next() odd).
    {
        const auto odd = []() { return 1; };
        World tiles;
        for (std::uint16_t x = 48; x <= 52; ++x)
        {
            for (std::uint16_t z = 48; z <= 52; ++z)
            {
                tiles.setBlock(x, 0, z, Block::make(35));
            }
        }
        std::vector<BlockChange> changes;
        tiles.setBlock(50, 1, 50, Block::make(35));
        changes.push_back({50, 1, 50, Block::air(), Block::make(35)});
        AutoTile::AdaptBorder(tiles, 50, 1, 50, odd, changes);
        check(tiles.getBlock(50, 1, 50).type() == 147,
              "a rock placed alone on rock takes the variant bordered below only (table_adapt_decor 147)");
        check(tiles.getBlock(50, 0, 50).type() == 35,
              "the rock under it, bordered all round, stays the plain rock");
        check(tiles.getBlock(51, 0, 50).type() == 35 && tiles.getBlock(48, 0, 48).type() != 0,
              "its floor's other cells keep a rock variant");
        int placedEntries = 0;
        for (const auto& change : changes)
        {
            placedEntries += (change.x == 50 && change.y == 1 && change.z == 50) ? 1 : 0;
        }
        check(placedEntries == 1 && changes.front().before.isAir() && changes.front().after.type() == 147,
              "one change per cell (undo restores the first before): the placed cell's entry ends as 147");

        // A side bordered on X or on Z (the owner's MR-0005): a rock beside it on Z only.
        tiles.setBlock(50, 1, 51, Block::make(35));
        AutoTile::AdaptBorder(tiles, 50, 1, 51, odd, changes);
        check(tiles.getBlock(50, 1, 50).type() == 151,
              "with a rock beside it on +Z the cell counts as bordered on its right (mask 5: 151)");

        // The bars: a mount under a block (138), the plain pipe in the open (202).
        tiles.setBlock(60, 5, 60, Block::make(35));
        tiles.setBlock(60, 4, 60, Block::make(202));
        AutoTile::AdaptBorder(tiles, 60, 4, 60, odd, changes);
        tiles.setBlock(62, 4, 60, Block::make(138));
        AutoTile::AdaptBorder(tiles, 62, 4, 60, odd, changes);
        check(tiles.getBlock(60, 4, 60).type() == 138 && tiles.getBlock(62, 4, 60).type() == 202,
              "a bar under a block takes its mounts (138), one in the open the plain pipe (202)");

        // The lightning column: an emitter grows its bolt into four empty cells, emptied it goes.
        tiles.setBlock(70, 9, 70, Block::make(304));
        AutoTile::AdaptBorder(tiles, 70, 9, 70, odd, changes);
        check(tiles.getBlock(70, 8, 70).type() == 305 && tiles.getBlock(70, 5, 70).type() == 305 &&
                  tiles.getBlock(70, 4, 70).isAir(),
              "an emitter (304) placed grows its bolt (305) four cells down");
        tiles.setBlock(70, 9, 70, Block::air());
        AutoTile::AdaptBorder(tiles, 70, 9, 70, odd, changes);
        check(tiles.getBlock(70, 8, 70).isAir() && tiles.getBlock(70, 5, 70).isAir(),
              "the emitter removed takes its bolt away");
        check(AutoTile::IsRightBorder(AutoTile::kEmpty, 35, 0, -1) == false &&
                  AutoTile::IsRightBorder(68, 35, 0, -1) == false && AutoTile::IsRightBorder(68, 35, 0, 1) == true,
              "IsRightBorder: air is no border; lava is none under a cell, one over it");
    }

    // ED-0005: one history mixing every kind of edit -- music, spawn point, BigDecor, a turned tile,
    // an object's end and timing -- undoes back to the start and redoes to the end, step by step.
    {
        using GalaxyEggbert::BigDecorRecord;
        using GalaxyEggbert::CollectBigDecor;
        using GalaxyEggbert::CollectMoveObjects;
        using GalaxyEggbert::MoveObjectRecord;
        using GalaxyEggbert::PlaceBigDecor;
        using GalaxyEggbert::PlaceMoveObject;
        World world;
        world.setMusicTrack(1);
        world.setSpawnPoint(10, 1, 10);
        MoveObjectRecord lift;
        lift.type = GalaxyEggbert::Def::ObjectType::ObjectType1;
        lift.posStartX = lift.posEndX = 20.0f;
        lift.posStartY = lift.posEndY = 1.0f;
        lift.posStartZ = lift.posEndZ = 20.0f;
        lift.stepAdvanceTicks = lift.stepRecedeTicks = 10.0f;
        PlaceMoveObject(world, lift);
        EditCommandStack stack;

        EditCommand music;
        music.kind = EditCommand::Kind::MusicEdit;
        music.musicBefore = 1;
        music.musicAfter = 4;
        world.setMusicTrack(4);
        stack.Push(music);

        EditCommand spawn;
        spawn.kind = EditCommand::Kind::SpawnPointEdit;
        spawn.spawnBefore = {true, 10, 1, 10};
        spawn.spawnAfter = {true, 30, 2, 30};
        world.setSpawnPoint(30, 2, 30);
        stack.Push(spawn);

        EditCommand decor;
        decor.kind = EditCommand::Kind::BigDecorEdit;
        decor.bigDecorAnchorX = 40;
        decor.bigDecorAnchorY = 1;
        decor.bigDecorAnchorZ = 40;
        decor.bigDecorAfter = BigDecorRecord{20, 40, 1, 40};
        PlaceBigDecor(world, *decor.bigDecorAfter);
        stack.Push(decor);

        EditCommand turn;
        turn.kind = EditCommand::Kind::TurnEdit;
        world.setBlock(50, 1, 50, Block::make(126));
        turn.turnX = 50;
        turn.turnY = 1;
        turn.turnZ = 50;
        turn.turnBefore = false;
        SetPlateRotated(world, 50, 1, 50, true);
        stack.Push(turn);

        EditCommand target;
        target.kind = EditCommand::Kind::MoveObjectEdit;
        target.objectAnchorX = 20;
        target.objectAnchorY = 1;
        target.objectAnchorZ = 20;
        target.objectBefore = lift;
        MoveObjectRecord moved = lift;
        moved.posEndX = 23.0f;
        moved.stepAdvanceTicks = moved.stepRecedeTicks = 30.0f;
        target.objectAfter = moved;
        PlaceMoveObject(world, moved);
        stack.Push(target);

        const auto liftEndX = [&]() {
            const auto all = CollectMoveObjects(world);
            return all.empty() ? -1.0f : all.front().posEndX;
        };
        bool undone = stack.Undo(world) && liftEndX() == 20.0f && CollectMoveObjects(world).front().stepAdvanceTicks == 10.0f;
        undone = undone && stack.Undo(world) && !IsPlateRotated(world, 50, 1, 50);
        undone = undone && stack.Undo(world) && CollectBigDecor(world).empty();
        undone = undone && stack.Undo(world) && world.spawnX() == 10 && world.spawnY() == 1;
        undone = undone && stack.Undo(world) && world.musicTrack() == 1;
        check(undone, "undo walks a mixed history back: an object's end and timing, a turn, a BigDecor, the spawn, the music");
        bool redone = stack.Redo(world) && world.musicTrack() == 4;
        redone = redone && stack.Redo(world) && world.spawnX() == 30;
        redone = redone && stack.Redo(world) && CollectBigDecor(world).size() == 1;
        redone = redone && stack.Redo(world) && IsPlateRotated(world, 50, 1, 50);
        redone = redone && stack.Redo(world) && liftEndX() == 23.0f;
        check(redone, "redo walks it forward again to the last edit");
    }

    // ED-0003: the moving mechanisms' lifts are one type at the PC editor's four speeds (PlaceDecor4),
    // the fifth entry the conveyor lift, the slime creature the big creature 54.
    {
        using GalaxyEggbert::Def::ObjectType;
        bool found = false;
        for (const auto& category : ConfirmedBlockCategories())
        {
            if (category.name != "Moving mechanisms")
            {
                continue;
            }
            found = true;
            const auto& types = category.objectTypeIds;
            check(types.size() >= 10 && types[0] == 1 && types[1] == 1 && types[2] == 1 && types[3] == 1 &&
                      types[4] == 47 && types[9] == 54,
                  "the four lifts are type 1, the conveyor lift 47, the slime creature 54");
        }
        check(found, "the palette has its moving mechanisms");
        const auto slow = LiftPreset(ObjectType::ObjectType1, 21);
        const auto fast = LiftPreset(ObjectType::ObjectType1, 19);
        const auto mixed = LiftPreset(ObjectType::ObjectType1, 28);
        check(slow && slow->advance == 20.0f && slow->rest == 20.0f && fast && fast->advance == 6.0f &&
                  fast->rest == 3.0f && mixed && mixed->advance == 5.0f && mixed->recede == 22.0f &&
                  !LiftPreset(ObjectType::ObjectType4, 21),
              "the lifts' speeds per cell and rests are the PC editor's (slow 20, fast 6/3, fast-slow 5/22/2)");
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
