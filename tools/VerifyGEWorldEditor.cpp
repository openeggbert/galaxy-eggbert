#include <GalaxyEggbert/Editor/AutoTile.hpp>
#include <GalaxyEggbert/Editor/BoxRegion.hpp>
#include <GalaxyEggbert/Editor/Clipboard.hpp>
#include <GalaxyEggbert/Editor/PaletteCategories.hpp>
#include <GalaxyEggbert/Game/PlateRotationMetadata.hpp>
#include <GalaxyEggbert/BigDecorRecord.hpp>
#include <GalaxyEggbert/MoveObjectRecord.hpp>
#include <GalaxyEggbert/Editor/EditCommandStack.hpp>
#include <GalaxyEggbert/Editor/VoxelRaycast.hpp>
#include <GalaxyEggbert/Editor/WorldEditor.hpp>
#include <GalaxyEggbert/Editor/EditorCommandKeys.hpp>
#include <GalaxyEggbert/Game/DirectionalCubeTiles.hpp>

#include <GalaxyEggbert/BigDecorRecord.hpp>
#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <algorithm>
#include <cmath>
#include <optional>
#include <random>
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
        400, 240, 0, ButtonState::Released, ButtonState::Released,
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

        editor.Update(KeyboardState{Keys::LeftControl}, restMouse, 1.0f, 800, 480, camera, world);
        check(camera.Position().Y > 25.0f,
              "Ctrl moves the editor camera upward");
    }

    {
        World world;
        WorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        CnaEngine::Graphics::Camera camera;
        editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, world);
        const float defaultFov = camera.FieldOfView();
        const MouseState wheelUp(
            400, 240, 120, ButtonState::Released, ButtonState::Released,
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
        editor.EnterEditing(0, 10, 0);
        CnaEngine::Graphics::Camera camera;
        editor.Update(KeyboardState{}, restMouse, 0, 800, 480, camera, world);
        const auto extend = [&](Keys key) {
            editor.Update(KeyboardState{Keys::LeftShift, key}, restMouse, 0, 800, 480, camera, world);
            editor.Update(KeyboardState{}, restMouse, 0, 800, 480, camera, world);
        };
        extend(Keys::NumPad6); extend(Keys::NumPad6);
        extend(Keys::NumPad9);
        for (int i = 0; i < 7; ++i) extend(Keys::NumPad2);
        int before = 0;
        for (int x = placeX; x < placeX + 3; ++x)
            for (int y = placeY; y < placeY + 2; ++y)
                for (int z = placeZ; z < placeZ + 8; ++z) before += !world.getBlock(x,y,z).isAir();
        check(before == 0, "Shift+NumPad previews a 3x2x8 selection without modifying the world");
        const auto eye = camera.Position();
        editor.Update(KeyboardState{Keys::Space}, restMouse, 0, 800, 480, camera, world);
        int filled = 0;
        for (int x = placeX; x < placeX + 3; ++x)
            for (int y = placeY; y < placeY + 2; ++y)
                for (int z = placeZ; z < placeZ + 8; ++z) filled += !world.getBlock(x,y,z).isAir();
        check(filled == 48 && camera.Position().Y == eye.Y,
              "Space fills the complete 3x2x8 selection and does not raise the camera");
        editor.Update(KeyboardState{}, restMouse, 0, 800, 480, camera, world);
        editor.Update(KeyboardState{Keys::U}, restMouse, 0, 800, 480, camera, world);
        int remaining = 0;
        for (int x = placeX; x < placeX + 3; ++x)
            for (int y = placeY; y < placeY + 2; ++y)
                for (int z = placeZ; z < placeZ + 8; ++z) remaining += !world.getBlock(x,y,z).isAir();
        check(remaining == 0, "one undo restores the entire extended selection");
    }

    {
        World world = slab;
        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        CnaEngine::Graphics::Camera camera;

        const MouseState left(
            400, 240, 0, ButtonState::Pressed, ButtonState::Released,
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
            400, 240, 0, ButtonState::Released, ButtonState::Pressed,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, middle, 0.0f, 800, 480, camera, world);
        check(world.getBlock(placeX, placeY, placeZ).isAir(),
              "middle click removes the aimed-at block");
        check(!world.getBlock(slabHit.x, slabHit.y, slabHit.z).isAir(),
              "removing the nearest block preserves the slab below it");
    }

    // OR-0070: V turns the aimed block to its next variant: its family's (a fan), else its auto-tile
    // row's (the grass's ends 157, 109, 108, 107). Undoable.
    {
        World world = slab;
        world.setBlock(slabHit.x, slabHit.y, slabHit.z, Block::make(108));
        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        CnaEngine::Graphics::Camera camera;
        const MouseState rest(
            400, 240, 0, ButtonState::Released, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, rest, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{Keys::V}, rest, 0.0f, 800, 480, camera, world);
        check(world.getBlock(slabHit.x, slabHit.y, slabHit.z).type() == 107,
              "V turns the aimed grass end to the next of its row (OR-0070)");
        editor.Update(KeyboardState{}, rest, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{Keys::V}, rest, 0.0f, 800, 480, camera, world);
        check(world.getBlock(slabHit.x, slabHit.y, slabHit.z).type() == 157, "and on, wrapping round its row");
        world.setBlock(slabHit.x, slabHit.y, slabHit.z, Block::make(126));
        editor.Update(KeyboardState{}, rest, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{Keys::V}, rest, 0.0f, 800, 480, camera, world);
        check(world.getBlock(slabHit.x, slabHit.y, slabHit.z).type() == 129, "a fan to the next of its family");
        editor.Update(KeyboardState{}, rest, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{Keys::U}, rest, 0.0f, 800, 480, camera, world);
        check(world.getBlock(slabHit.x, slabHit.y, slabHit.z).type() == 126, "undone, the fan again");
    }

    // Three short native V presses, all released before Update, must remain three undoable edits.
    {
        World world = slab;
        world.setBlock(slabHit.x, slabHit.y, slabHit.z, Block::make(126));
        WorldEditor editor;
        editor.EnterEditing(0, 10, 0);
        CnaEngine::Graphics::Camera camera;
        editor.Update(KeyboardState{}, restMouse, 0, 800, 480, camera, world);
        EditorCommandKeys presses;
        presses.Record(Keys::V); presses.Record(Keys::V); presses.Record(Keys::V);
        for (int tick = 0; tick < 6; ++tick)
            editor.Update(presses.Next(KeyboardState{}), restMouse, 0, 800, 480, camera, world);
        check(world.getBlock(slabHit.x, slabHit.y, slabHit.z).type() == 135,
              "three between-frame V presses advance a fan through three variants");
        for (int i = 0; i < 3; ++i)
        {
            editor.Update(KeyboardState{}, restMouse, 0, 800, 480, camera, world);
            editor.Update(KeyboardState{Keys::U}, restMouse, 0, 800, 480, camera, world);
        }
        check(world.getBlock(slabHit.x, slabHit.y, slabHit.z).type() == 126,
              "each queued V press is a separate undoable change");
        EditorCommandKeys held;
        held.Record(Keys::V);
        int edges = 0;
        for (int tick = 0; tick < 6; ++tick)
        {
            const auto keys = held.Next(KeyboardState{Keys::V, Keys::LeftControl});
            edges += keys.IsKeyDown(Keys::V) ? 1 : 0;
            check(keys.IsKeyDown(Keys::LeftControl), "V capture preserves other editor keys");
        }
        check(edges == 1, "a held physical V does not duplicate its recorded press");
        held.Record(Keys::V); held.Reset();
        check(!held.Next(KeyboardState{}).IsKeyDown(Keys::V) &&
              !held.Next(KeyboardState{}).IsKeyDown(Keys::V), "focus/phase reset discards pending variants");
    }

    // Exercise the public editor workflow for every multi-variant terrain family, plus
    // auto-tiled materials. This checks input/target/history together, not NextVariant itself.
    {
        std::vector<std::uint16_t> icons{147,108,261,341,386,250};
        for (const auto& family : VariantFamilies())
            if (family.icons.size() > 1) icons.push_back(family.icons.front());
        bool responds = true;
        for (const auto icon : icons)
        {
            World world = slab;
            world.setBlock(slabHit.x, slabHit.y, slabHit.z, Block::make(icon));
            WorldEditor editor;
            editor.EnterEditing(0, 10, 0);
            CnaEngine::Graphics::Camera camera;
            editor.Update(KeyboardState{}, restMouse, 0, 800, 480, camera, world);
            EditorCommandKeys presses;
            presses.Record(Keys::V);
            editor.Update(presses.Next(KeyboardState{}), restMouse, 0, 800, 480, camera, world);
            const bool changed = world.getBlock(slabHit.x, slabHit.y, slabHit.z).type() != icon;
            editor.Update(presses.Next(KeyboardState{}), restMouse, 0, 800, 480, camera, world);
            editor.Update(KeyboardState{Keys::U}, restMouse, 0, 800, 480, camera, world);
            const bool restored = world.getBlock(slabHit.x, slabHit.y, slabHit.z).type() == icon;
            if (!changed || !restored) std::cout << "variant workflow failed for icon " << icon << std::endl;
            responds &= changed && restored;
        }
        check(responds,"every multi-variant terrain family accepts a short V press and Undo");
    }

    // OR-0071: O turns a model with a front a quarter at a time; a fan only between its two axes.
    {
        World world = slab;
        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        CnaEngine::Graphics::Camera camera;
        const MouseState rest(
            400, 240, 0, ButtonState::Released, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        world.setBlock(slabHit.x, slabHit.y, slabHit.z, Block::make(330)); // a teleporter
        int turns = -1;
        bool quarters = true;
        for (int expected : {1, 2, 3, 0})
        {
            editor.Update(KeyboardState{}, rest, 0.0f, 800, 480, camera, world);
            editor.Update(KeyboardState{Keys::O}, rest, 0.0f, 800, 480, camera, world);
            turns = ObjectQuarterTurns(world, slabHit.x, slabHit.y, slabHit.z);
            quarters = quarters && turns == expected && QuarterTurns(world, slabHit.x, slabHit.y, slabHit.z) == 0;
        }
        check(quarters, "O turns a teleporter a quarter at a time, round to where it began (its object turns, "
                        "not the plate bit the hub signs carry)");
        world.setBlock(slabHit.x, slabHit.y, slabHit.z, Block::make(126));
        editor.Update(KeyboardState{}, rest, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{Keys::O}, rest, 0.0f, 800, 480, camera, world);
        const bool one = QuarterTurns(world, slabHit.x, slabHit.y, slabHit.z) == 1 &&
                         IsPlateRotated(world, slabHit.x, slabHit.y, slabHit.z);
        editor.Update(KeyboardState{}, rest, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{Keys::O}, rest, 0.0f, 800, 480, camera, world);
        check(one && QuarterTurns(world, slabHit.x, slabHit.y, slabHit.z) == 0, "a fan swaps its axis, back and forth");
        // An object in the preview's cell turns a quarter, its cell's turns (a treasure).
        GalaxyEggbert::MoveObjectRecord treasure;
        treasure.type = GalaxyEggbert::Def::ObjectType::ObjectType5;
        treasure.posStartX = treasure.posEndX = static_cast<float>(placeX);
        treasure.posStartY = treasure.posEndY = static_cast<float>(placeY);
        treasure.posStartZ = treasure.posEndZ = static_cast<float>(placeZ);
        GalaxyEggbert::PlaceMoveObject(world, treasure);
        editor.Update(KeyboardState{}, rest, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{Keys::O}, rest, 0.0f, 800, 480, camera, world);
        check(ObjectQuarterTurns(world, placeX, placeY, placeZ) == 1 && QuarterTurns(world, placeX, placeY, placeZ) == 0 &&
                  QuarterTurns(world, slabHit.x, slabHit.y, slabHit.z) == 0,
              "O turns the object in the preview's cell (its own turns, not the cell's wind), not the block under it (OR-0071)");
        editor.Update(KeyboardState{}, rest, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{Keys::U}, rest, 0.0f, 800, 480, camera, world);
        check(ObjectQuarterTurns(world, placeX, placeY, placeZ) == 0, "undone, the object faces as before");
    }

    // The NumPad coordinate is explicit: Turn must not silently edit the ray's old voxel.
    {
        World world = slab;
        WorldEditor editor;
        editor.EnterEditing(0,10,0);
        CnaEngine::Graphics::Camera camera;
        editor.Update(KeyboardState{},restMouse,0,800,480,camera,world);
        const auto x = static_cast<std::uint16_t>(placeX+1);
        world.setBlock(x,placeY,placeZ,Block::make(190)); // one window, five plain walls
        WorldEditor::Navigation keep;
        keep.keepAim = true;
        editor.Update(KeyboardState{Keys::NumPad6},restMouse,0,800,480,camera,world,keep);
        const auto target = editor.CurrentTurnTarget(world);
        check(target && target->kind == WorldEditor::EditTarget::Kind::Block &&
              target->x == x && target->y == placeY && target->z == placeZ,
              "the red selection and Turn share the explicit NumPad coordinate");
        bool rotated = true;
        constexpr int room[4] = {8,8,8,8};
        const int originalSide = SingleTexturedSide(190,0,room);
        for (int quarter : {1,2,3,0})
        {
            editor.Update(KeyboardState{},restMouse,0,800,480,camera,world,keep);
            editor.Update(KeyboardState{Keys::O},restMouse,0,800,480,camera,world,keep);
            const auto turns = ObjectQuarterTurns(world,x,placeY,placeZ);
            rotated &= turns == quarter && TurnTexturedSide(originalSide,turns) ==
                (quarter == 0 ? originalSide : quarter == 1 ? 2 : quarter == 2 ? 1 : 3);
        }
        check(rotated && QuarterTurns(world,slabHit.x,slabHit.y,slabHit.z)==0,
              "O turns a house window through all four sides, leaving the ray's ground unchanged");
        editor.Update(KeyboardState{},restMouse,0,800,480,camera,world,keep);
        editor.Update(KeyboardState{Keys::U},restMouse,0,800,480,camera,world,keep);
        check(ObjectQuarterTurns(world,x,placeY,placeZ)==3,"window turn Undo restores its previous side");
        world.setBlock(x,placeY,placeZ,Block::air());
        editor.Update(KeyboardState{},restMouse,0,800,480,camera,world,keep);
        editor.Update(KeyboardState{Keys::O},restMouse,0,800,480,camera,world,keep);
        check(!editor.CurrentTurnTarget(world) && QuarterTurns(world,slabHit.x,slabHit.y,slabHit.z)==0,
              "an empty NumPad coordinate does not turn a different object behind it");
        EditorCommandKeys presses;
        presses.Record(Keys::V); presses.Record(Keys::O); presses.Record(Keys::V);
        std::vector<Keys> commands;
        for (int tick=0; tick<6; ++tick)
            for (auto key : presses.Next(KeyboardState{Keys::O}).GetPressedKeys())
                if (key==Keys::V || key==Keys::O) commands.push_back(key);
        check(commands==std::vector<Keys>{Keys::V,Keys::O,Keys::V},
              "rapid V/O commands retain their arrival order and a held O is not duplicated");
    }

    // OR-0072: a lift placed, the next click elsewhere is its end (the original's m_liftEnd).
    {
        const auto FindMoveObjectAt = [](const World& w, std::uint16_t x, std::uint16_t y, std::uint16_t z)
            -> std::optional<GalaxyEggbert::MoveObjectRecord>
        {
            for (const auto& record : GalaxyEggbert::CollectMoveObjects(w))
            {
                if (record.posStartX == static_cast<float>(x) && record.posStartY == static_cast<float>(y) &&
                    record.posStartZ == static_cast<float>(z))
                {
                    return record;
                }
            }
            return std::nullopt;
        };
        World world = slab;
        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        CnaEngine::Graphics::Camera camera;
        editor.Palette().SelectObject(GalaxyEggbert::Def::ObjectType::ObjectType1);
        const MouseState press(
            400, 240, 0, ButtonState::Pressed, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        const MouseState rest(
            400, 240, 0, ButtonState::Released, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, rest, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{}, press, 0.0f, 800, 480, camera, world);
        const auto placed = FindMoveObjectAt(world, placeX, placeY, placeZ);
        check(placed.has_value(), "a lift is placed in the preview cell");
        editor.Update(KeyboardState{}, rest, 0.0f, 800, 480, camera, world);
        // The preview moved two up (NumPad 9) and a click there: the lift's end.
        editor.Update(KeyboardState{Keys::NumPad9}, rest, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{}, rest, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{Keys::NumPad9}, rest, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{}, rest, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{}, press, 0.0f, 800, 480, camera, world);
        const auto ended = FindMoveObjectAt(world, placeX, placeY, placeZ);
        check(ended.has_value() && ended->posEndY == static_cast<float>(placeY + 2) &&
                  ended->posEndX == static_cast<float>(placeX) && ended->stepAdvanceTicks > placed->stepAdvanceTicks,
              "the next click sets its end two up, its step scaled to the cells (OR-0072)");
        check(!FindMoveObjectAt(world, placeX, static_cast<std::uint16_t>(placeY + 2), placeZ).has_value(),
              "and places no second lift");
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
            400, 240, 0, ButtonState::Pressed, ButtonState::Released,
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
            400, 240, 0, ButtonState::Pressed, ButtonState::Released,
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
            400, 240, 0, ButtonState::Pressed, ButtonState::Released,
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
            KeyboardState{Keys::LeftControl}, restMouse, 0.3f, 800, 480,
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

    // ED-0010: copy a box, turn it, mirror it, stamp it; one undo takes the stamp back.
    {
        using GalaxyEggbert::CollectBigDecor;
        using GalaxyEggbert::CollectMoveObjects;
        using GalaxyEggbert::MoveObjectRecord;
        using GalaxyEggbert::PlaceBigDecor;
        using GalaxyEggbert::PlaceMoveObject;
        using GalaxyEggbert::Def::ObjectType;
        World world;
        world.setBlock(10, 5, 20, Block::make(1));
        world.setBlock(11, 5, 20, Block::make(129)); // a fan blowing +X
        world.setBlock(10, 5, 21, Block::make(174)); // a sign, turned a quarter
        SetObjectQuarterTurns(world, 10, 5, 21, 1);
        world.setBlock(11, 5, 21, Block::make(114)); // its wind, +X
        world.setBlock(12, 5, 21, Block::make(1));
        GalaxyEggbert::MoveObjectRecord lift;
        lift.type = ObjectType::ObjectType1;
        lift.posStartX = 12.0f;
        lift.posStartY = 5.0f;
        lift.posStartZ = 20.0f;
        lift.posEndX = 12.0f;
        lift.posEndY = 5.0f;
        lift.posEndZ = 23.0f;
        PlaceMoveObject(world, lift);
        PlaceBigDecor(world, GalaxyEggbert::BigDecorRecord{20, 11, 6, 21});
        MoveObjectRecord outside = lift;
        outside.posStartX = outside.posEndX = 30.0f;
        PlaceMoveObject(world, outside);

        const Clipboard copied = CopyBox(world, NormalizeAndClamp(12, 6, 21, 10, 5, 20, 100));
        check(copied.sizeX == 3 && copied.sizeY == 2 && copied.sizeZ == 2 && copied.objects.size() == 1 &&
                  copied.bigDecor.size() == 1 && copied.cells[copied.Index(1, 0, 0)].block.type() == 129 &&
                  copied.cells[copied.Index(0, 0, 1)].objectTurns == 1 && copied.objects[0].posStartX == 2.0f &&
                  copied.objects[0].posEndZ == 3.0f,
              "C copies a box: its blocks, turns, the object and BigDecor anchored in it, not one outside");

        const Clipboard turned = TurnedQuarter(copied);
        const auto& fan = turned.cells[turned.Index(0, 0, 1)];
        const auto& wind = turned.cells[turned.Index(1, 0, 1)];
        const auto& sign = turned.cells[turned.Index(1, 0, 2)];
        check(turned.sizeX == 2 && turned.sizeZ == 3 && fan.block.type() == 126 && fan.plateTurns == 1 &&
                  wind.block.type() == 110 && wind.plateTurns == 1,
              "a quarter turn takes a fan blowing +X and its wind to -Z: the -X twin turned onto Z");
        check(sign.block.type() == 174 && sign.objectTurns == 1,
              "a sign keeps its turns (it faces its cell's open side by itself, which turns with the copy)");
        check(turned.objects[0].posStartX == 0.0f && turned.objects[0].posStartZ == 0.0f &&
                  turned.objects[0].posEndX == 3.0f && turned.objects[0].posEndZ == 0.0f &&
                  turned.cells[turned.Index(0, 0, 0)].objectTurns == 1 && turned.bigDecor[0].x == 1 &&
                  turned.bigDecor[0].z == 1 && turned.cells[turned.Index(1, 1, 1)].objectTurns == 1,
              "a quarter turn turns a lift's path and turns the lift and the BigDecor a quarter");
        Clipboard round = copied;
        for (int i = 0; i < 4; ++i)
        {
            round = TurnedQuarter(round);
        }
        bool same = round.sizeX == copied.sizeX && round.sizeZ == copied.sizeZ && !round.Transformed();
        for (std::size_t i = 0; same && i < copied.cells.size(); ++i)
        {
            same = round.cells[i].block == copied.cells[i].block &&
                   std::max(round.cells[i].plateTurns, 0) % 2 == std::max(copied.cells[i].plateTurns, 0) % 2;
        }
        check(same && round.objects[0].posEndZ == 3.0f, "four quarter turns give the copy back");

        const Clipboard mirrored = MirroredX(copied);
        check(mirrored.cells[mirrored.Index(1, 0, 0)].block.type() == 126 &&
                  mirrored.cells[mirrored.Index(1, 0, 1)].block.type() == 110 &&
                  mirrored.cells[mirrored.Index(2, 0, 1)].objectTurns == 3 && mirrored.objects[0].posStartX == 0.0f &&
                  mirrored.objects[0].posEndZ == 3.0f && mirrored.mirrored,
              "a mirror swaps the fan and wind to -X, reverses the sign's turn, moves the lift to the other side");

        World target;
        target.setBlock(42, 5, 40, Block::make(1)); // under the copy's empty cell
        MoveObjectRecord old = lift;
        old.type = ObjectType::ObjectType4;
        old.posStartX = old.posEndX = 40.0f;
        old.posStartY = old.posEndY = 6.0f;
        old.posStartZ = old.posEndZ = 41.0f;
        PlaceMoveObject(target, old);
        PlaceBigDecor(target, GalaxyEggbert::BigDecorRecord{21, 41, 5, 40});
        World merged = target;
        std::minstd_rand dice(7u);
        const auto next = [&dice]() { return static_cast<int>(dice() & 0x7fffffff); };
        EditCommandStack stack;
        stack.Push(PasteBox(target, copied, 40, 5, 40, false, next));
        const auto objects = CollectMoveObjects(target);
        const auto decor = CollectBigDecor(target);
        check(target.getBlock(41, 5, 40).type() == 129 && target.getBlock(42, 5, 40).isAir() &&
                  target.getBlock(40, 5, 41).type() == 174 && ObjectQuarterTurns(target, 40, 5, 41) == 1 &&
                  objects.size() == 1 && objects[0].posStartX == 42.0f && objects[0].posEndZ == 43.0f &&
                  decor.size() == 1 && decor[0].x == 41 && decor[0].y == 6 && decor[0].z == 41,
              "a stamp replaces its box: the copy's blocks and turns, its lift and BigDecor, the old ones gone");
        const bool undone = stack.Undo(target) && target.getBlock(42, 5, 40).type() == 1 &&
                            target.getBlock(41, 5, 40).isAir() && target.getBlock(40, 5, 41).isAir() &&
                            ObjectQuarterTurns(target, 40, 5, 41) == 0 && CollectMoveObjects(target).size() == 1 &&
                            CollectMoveObjects(target)[0].type == ObjectType::ObjectType4 &&
                            CollectBigDecor(target).size() == 1 && CollectBigDecor(target)[0].icon == 21;
        check(undone, "one undo takes the whole stamp back: blocks, turns, objects, BigDecor");
        check(stack.Redo(target) && target.getBlock(41, 5, 40).type() == 129 && CollectMoveObjects(target).size() == 1 &&
                  CollectMoveObjects(target)[0].posStartX == 42.0f,
              "redo stamps it again");
        (void)PasteBox(merged, copied, 40, 5, 40, true, next);
        check(merged.getBlock(42, 5, 40).type() == 1 && merged.getBlock(41, 5, 40).type() == 129 &&
                  CollectMoveObjects(merged).size() == 2 && CollectBigDecor(merged).size() == 2,
              "CTRL+click stamps over: what is under the copy's empty cells stays");
    }

    // The keys: C on two corners copies, P pastes on and off, Q turns, a click stamps, U takes it back.
    {
        World world = slab;
        WorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        CnaEngine::Graphics::Camera camera;
        const auto press = [&](Keys key, float dt = 0.0f) {
            editor.Update(KeyboardState{key}, restMouse, dt, 800, 480, camera, world);
            editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, world);
        };
        const auto blocks = [&]() {
            int count = 0;
            for (std::uint16_t y = 0; y < 12; ++y)
            {
                for (std::uint16_t z = 0; z < 100; ++z)
                {
                    count += world.getBlock(50, y, z).isAir() ? 0 : 1;
                }
            }
            return count;
        };
        const int before = blocks();
        press(Keys::C);
        editor.Update(KeyboardState{Keys::LeftControl}, restMouse, 0.3f, 800, 480, camera, world);
        press(Keys::C);
        const Clipboard* clip = editor.CurrentClipboard();
        check(clip != nullptr && clip->sizeX == 1 && clip->sizeY == 1 && clip->sizeZ > 1 && !editor.IsPasting(),
              "C on two corners copies the box between them (a row of the slab's top)");
        const int row = clip != nullptr ? clip->sizeZ : 0;
        press(Keys::P);
        check(editor.IsPasting(), "P starts pasting the copy");
        press(Keys::Q);
        check(editor.CurrentClipboard()->sizeX == row && editor.CurrentClipboard()->sizeZ == 1, "Q turns it a quarter");
        press(Keys::Q);
        press(Keys::Q);
        press(Keys::Q);
        const MouseState left(400, 240, 0, ButtonState::Pressed, ButtonState::Released, ButtonState::Released,
                              ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, left, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, world);
        check(blocks() == before + row, "a click stamps the copy at the red preview");
        check(editor.IsPasting(), "the pasting stays on for the next stamp");
        press(Keys::U);
        check(blocks() == before, "U takes the whole stamp back");
        press(Keys::P);
        check(!editor.IsPasting(), "P again stops pasting");
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
        GalaxyEggbert::MoveObjectRecord lift;
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
        turn.turnBefore = 0;
        turn.turnAfter = 1;
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

    // OR-0076: the editor aims through the cursor (or the touch point: SDL turns a touch into mouse
    // events), not the screen centre; over the UI the last aimed cell stays; the right button turns
    // the view and aims at the centre.
    {
        World floor;
        for (std::uint16_t x = 20; x < 80; ++x)
        {
            for (std::uint16_t z = 0; z < 100; ++z)
            {
                floor.setBlock(x, 0, z, Block::make(1));
            }
        }
        struct Cell
        {
            int x, y, z;
            bool found;
        };
        const auto countSolid = [](const World& w)
        {
            int count = 0;
            for (std::uint16_t x = 0; x < 100; ++x)
                for (std::uint16_t y = 0; y < 3; ++y)
                    for (std::uint16_t z = 0; z < 100; ++z)
                        if (!w.getBlock(x, y, z).isAir()) ++count;
            return count;
        };
        // Taps (cx, cy) on a fresh editor and returns the one cell it laid at height 1.
        const auto tap = [&](int cx, int cy, ButtonState right = ButtonState::Released)
        {
            World world = floor;
            WorldEditor editor;
            editor.EnterEditing(0.0f, 10.0f, 0.0f);
            CnaEngine::Graphics::Camera camera;
            const MouseState aim(
                cx, cy, 0, ButtonState::Released, ButtonState::Released, right,
                ButtonState::Released, ButtonState::Released);
            editor.Update(KeyboardState{}, aim, 0.0f, 800, 480, camera, world);
            const MouseState press(
                cx, cy, 0, ButtonState::Pressed, ButtonState::Released, right,
                ButtonState::Released, ButtonState::Released);
            editor.Update(KeyboardState{}, press, 0.0f, 800, 480, camera, world);
            Cell cell{0, 0, 0, false};
            for (std::uint16_t x = 0; x < 100; ++x)
                for (std::uint16_t z = 0; z < 100; ++z)
                    if (!world.getBlock(x, 1, z).isAir()) cell = {x, 1, z, true};
            return cell;
        };
        const Cell centre = tap(400, 240);
        const Cell rightOf = tap(640, 240);
        const Cell leftOf = tap(160, 240);
        const Cell farther = tap(400, 190);
        const Cell nearer = tap(400, 340);
        check(centre.found && rightOf.found && leftOf.found && farther.found && nearer.found,
              "a click through the cursor lays a block wherever the cursor points at the floor");
        check(rightOf.x > centre.x && leftOf.x < centre.x,
              "a cursor right/left of the centre aims right/left of the centre's cell");
        check(farther.z < centre.z && nearer.z > centre.z,
              "a cursor above/below the centre aims farther/nearer than the centre's cell");
        const Cell lookingRight = tap(0, 0, ButtonState::Pressed); // relative mode: x, y are deltas
        check(lookingRight.found && lookingRight.x == centre.x && lookingRight.z == centre.z,
              "while the right button turns the view the centre aims");

        // Over the UI the aim keeps its last cell: aim right of the centre, then PLACE (500, 450).
        {
            World world = floor;
            WorldEditor editor;
            editor.EnterEditing(0.0f, 10.0f, 0.0f);
            CnaEngine::Graphics::Camera camera;
            const auto state = [](int x, int y, bool down)
            {
                return MouseState(
                    x, y, 0, down ? ButtonState::Pressed : ButtonState::Released, ButtonState::Released,
                    ButtonState::Released, ButtonState::Released, ButtonState::Released);
            };
            editor.Update(KeyboardState{}, state(640, 240, false), 0.0f, 800, 480, camera, world);
            editor.Update(KeyboardState{}, state(500, 450, true), 0.0f, 800, 480, camera, world);
            editor.Update(KeyboardState{}, state(500, 450, false), 0.0f, 800, 480, camera, world);
            Cell placed{0, 0, 0, false};
            for (std::uint16_t x = 0; x < 100; ++x)
                for (std::uint16_t z = 0; z < 100; ++z)
                    if (!world.getBlock(x, 1, z).isAir()) placed = {x, 1, z, true};
            check(countSolid(world) == countSolid(floor) + 1 && placed.found && placed.x == rightOf.x &&
                      placed.z == rightOf.z,
                  "PLACE under the cursor places at the last cell aimed in the view, not under the button");
        }
    }

    {
        GalaxyEggbert::MoveObjectRecord lift;
        lift.type=GalaxyEggbert::Def::ObjectType::ObjectType1;
        lift.posStartX=2;lift.posStartY=3;lift.posStartZ=4;
        lift.posEndX=8;lift.posEndY=9;lift.posEndZ=10;
        auto still=lift;still.type=GalaxyEggbert::Def::ObjectType::ObjectType47;
        still.posEndX=2;still.posEndY=3;still.posEndZ=4;
        auto chest=lift;chest.type=GalaxyEggbert::Def::ObjectType::ObjectType5;
        const auto markers=GalaxyEggbert::Editor::BuildLiftEndpointMarkers({lift,still,chest});
        check(markers.size()==3 && markers[0].icon==30 && markers[0].x==2 &&
              markers[1].icon==31 && markers[1].y==9 && markers[1].z==10 && markers[2].icon==30,
              "moving lifts show marker1 at start and marker2 at end; still lifts avoid overlapping labels and chests get none");
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
