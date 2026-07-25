#include <GalaxyEggbert/Editor/BoxRegion.hpp>
#include <GalaxyEggbert/Editor/EditCommandStack.hpp>
#include <GalaxyEggbert/Editor/VoxelRaycast.hpp>
#include <GalaxyEggbert/Editor/WorldEditor.hpp>

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

    const MouseState restMouse(
        0, 0, 0, ButtonState::Released, ButtonState::Released,
        ButtonState::Released, ButtonState::Released, ButtonState::Released);

    {
        World world;
        WorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        Easy3D::Camera3D camera;
        editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, world);
        check(camera.GetPosition().X == 50.0f &&
                  camera.GetPosition().Y == 15.0f &&
                  camera.GetPosition().Z == 50.0f,
              "EnterEditing sets the requested camera position");
        check(camera.GetTarget().Z < camera.GetPosition().Z &&
                  camera.GetTarget().Y < camera.GetPosition().Y,
              "the default camera looks forward and slightly downward");

        editor.Update(KeyboardState{Keys::W}, restMouse, 1.0f, 800, 480, camera, world);
        check(std::fabs(camera.GetPosition().X - 50.0f) < 0.001f &&
                  camera.GetPosition().Z < 40.0f,
              "W flies forward without sideways drift");
    }

    {
        World world;
        WorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        Easy3D::Camera3D camera;
        editor.Update(KeyboardState{Keys::D}, restMouse, 1.0f, 800, 480, camera, world);
        check(camera.GetPosition().X > 60.0f &&
                  std::fabs(camera.GetPosition().Z - 50.0f) < 0.001f,
              "D strafes right without forward drift");

        editor.Update(KeyboardState{Keys::Space}, restMouse, 1.0f, 800, 480, camera, world);
        check(camera.GetPosition().Y > 25.0f,
              "Space moves the editor camera upward");
    }

    {
        World world;
        WorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        Easy3D::Camera3D camera;
        editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, world);
        const float defaultFov = camera.GetFieldOfView();
        const MouseState wheelUp(
            0, 0, 120, ButtonState::Released, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, wheelUp, 0.0f, 800, 480, camera, world);
        check(camera.GetFieldOfView() < defaultFov,
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
        Easy3D::Camera3D camera;

        const MouseState left(
            400, 200, 0, ButtonState::Pressed, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, left, 0.0f, 800, 480, camera, world);
        check(world.getBlock(placeX, placeY, placeZ).type() ==
                  GalaxyEggbert::BlockTypes::RockPile,
              "left click places the selected block in the preview cell");
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

    {
        World world = slab;
        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        Easy3D::Camera3D camera;
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
        Easy3D::Camera3D camera;
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
        Easy3D::Camera3D camera;
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
        Easy3D::Camera3D camera;

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
                    allFilled &=
                        world.getBlock(x, y, z).type() ==
                        GalaxyEggbert::BlockTypes::RockPile;
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
        Easy3D::Camera3D camera;
        const auto pressKey = [&](Keys key)
        {
            editor.Update(
                KeyboardState{key}, restMouse, 0.0f, 800, 480,
                camera, world);
            editor.Update(
                KeyboardState{}, restMouse, 0.0f, 800, 480,
                camera, world);
        };

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
                               Easy3D::Camera3D& camera)
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
        Easy3D::Camera3D camera;
        clickStop(editor, world, camera);
        check(editor.IsBrowsing(),
              "Stop leaves immediately when the world is unchanged");
    }

    {
        World world;
        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        Easy3D::Camera3D camera;
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

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
