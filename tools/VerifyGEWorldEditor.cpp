#include "Editor/GEEditCommandStack.hpp"
#include "Editor/GEVoxelRaycast.hpp"
#include "Editor/GEWorldEditor.hpp"

#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <cmath>
#include <cstdio>
#include <iostream>

// Scripted verification for the in-game 3D world editor (plan.md section 6,
// EDITOR-1xx tasks). Sections so far:
//   - EDITOR-101: the free-fly camera's pure movement math (WASD +
//     scroll-wheel fly-speed, driven by synthetic KeyboardState/MouseState
//     values -- no GraphicsDevice/window needed at runtime, same "no
//     graphics context needed" shape as VerifyGEInputPad). Mouse-look
//     (holding the right button) is deliberately NOT exercised here: it
//     calls real Mouse::SetCaptureEXT()/setIsRelativeMouseModeEXTProperty(),
//     which need an actual SDL window to mean anything -- camera feel from
//     mouse-look is a live/screenshot check, same as every other "visual
//     judgment" item in this project.
//   - EDITOR-102: the Amanatides-Woo voxel raycast (GEVoxelRaycast.hpp)
//     against a synthetic World with known blocks placed -- pure grid math,
//     no GraphicsDevice needed either.
//   - EDITOR-103: single block place/remove + ConsumeNeedsPresentationRebuild(),
//     driven through GEWorldEditor::Update() with synthetic left/middle-click
//     MouseState values (both still no GraphicsDevice needed -- place/remove
//     only touches the World, not the GPU-side highlight mesh).
//   - EDITOR-104: GEEditCommandStack's push/undo/redo/truncate/depth-cap
//     behavior directly, plus GEWorldEditor's U/R undo/redo keys undoing/
//     redoing a real place through the same synthetic-input path as
//     EDITOR-103's section.
// Later milestones (box-fill, palette data, MoveObject/sky-region
// round-trips) add their own sections here.
int main()
{
    using namespace GalaxyEggbert::CNA;
    using GalaxyEggbert::Worlds::Block;
    using GalaxyEggbert::Worlds::World;
    using Microsoft::Xna::Framework::Input::ButtonState;
    using Microsoft::Xna::Framework::Input::KeyboardState;
    using Microsoft::Xna::Framework::Input::Keys;
    using Microsoft::Xna::Framework::Input::MouseState;

    bool allOk = true;
    const auto check = [&allOk](bool cond, const char* what)
    {
        std::cout << (cond ? "PASS" : "FAIL") << ": " << what << std::endl;
        if (!cond) allOk = false;
    };

    // MouseState with both buttons at rest -- every GEWorldEditor::Update()
    // check below uses this so it never takes the mouse-look/SDL-capture
    // branches (see the top-of-file comment for why those are excluded).
    const MouseState restMouse(0, 0, 0, ButtonState::Released, ButtonState::Released,
                               ButtonState::Released, ButtonState::Released, ButtonState::Released);
    // An all-air world -- fine for the camera-movement checks below, which
    // don't depend on what Update()'s own internal raycast happens to hit.
    // Non-const: EDITOR-103 made GEWorldEditor::Update() take a mutable
    // World& (it now places/removes blocks); none of the movement-only
    // checks below actually mutate it (both mouse buttons stay at rest).
    World emptyWorld;

    // --- EnterEditing() sets the starting position and a level-ish default look ---
    {
        GEWorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        Easy3D::Camera3D camera;
        editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, emptyWorld);
        check(camera.GetPosition().X == 50.0f && camera.GetPosition().Y == 15.0f &&
                  camera.GetPosition().Z == 50.0f,
              "EnterEditing() starts the camera at the given position");
        check(camera.GetTarget().Z < camera.GetPosition().Z,
              "default yaw (0) looks toward -Z");
        check(camera.GetTarget().Y < camera.GetPosition().Y,
              "default pitch looks slightly downward");
    }

    // --- W moves forward along yaw 0 (-Z), A/D strafe along +/-X, dt scales distance ---
    {
        GEWorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        Easy3D::Camera3D camera;
        editor.Update(KeyboardState{Keys::W}, restMouse, 1.0f, 800, 480, camera, emptyWorld);
        check(std::fabs(camera.GetPosition().X - 50.0f) < 0.001f,
              "holding W at yaw 0 doesn't drift sideways in X");
        check(camera.GetPosition().Z < 50.0f - 10.0f,
              "holding W for 1s at the default fly speed moves well forward (-Z)");
    }
    {
        GEWorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        Easy3D::Camera3D camera;
        editor.Update(KeyboardState{Keys::S}, restMouse, 1.0f, 800, 480, camera, emptyWorld);
        check(camera.GetPosition().Z > 50.0f + 10.0f,
              "holding S at yaw 0 moves well backward (+Z), opposite of W");
    }
    {
        GEWorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        Easy3D::Camera3D camera;
        editor.Update(KeyboardState{Keys::D}, restMouse, 1.0f, 800, 480, camera, emptyWorld);
        check(camera.GetPosition().X > 50.0f + 10.0f,
              "holding D at yaw 0 strafes well right (+X)");
        check(std::fabs(camera.GetPosition().Z - 50.0f) < 0.001f,
              "holding D at yaw 0 doesn't drift in Z");
    }
    {
        GEWorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        Easy3D::Camera3D camera;
        editor.Update(KeyboardState{Keys::A}, restMouse, 1.0f, 800, 480, camera, emptyWorld);
        check(camera.GetPosition().X < 50.0f - 10.0f,
              "holding A at yaw 0 strafes well left (-X), opposite of D");
    }

    // --- Space/LeftControl move purely along world Y ---
    {
        GEWorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        Easy3D::Camera3D camera;
        editor.Update(KeyboardState{Keys::Space}, restMouse, 1.0f, 800, 480, camera, emptyWorld);
        check(camera.GetPosition().Y > 15.0f + 10.0f, "holding Space rises well above the start height");
        check(std::fabs(camera.GetPosition().X - 50.0f) < 0.001f &&
                  std::fabs(camera.GetPosition().Z - 50.0f) < 0.001f,
              "holding Space alone doesn't drift horizontally");
    }
    {
        GEWorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        Easy3D::Camera3D camera;
        editor.Update(KeyboardState{Keys::LeftControl}, restMouse, 1.0f, 800, 480, camera, emptyWorld);
        check(camera.GetPosition().Y < 15.0f - 10.0f,
              "holding Left Ctrl descends well below the start height, opposite of Space");
    }

    // --- No keys held is a no-op (position unchanged across a real dt) ---
    {
        GEWorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        Easy3D::Camera3D camera;
        editor.Update(KeyboardState{}, restMouse, 1.0f, 800, 480, camera, emptyWorld);
        check(camera.GetPosition().X == 50.0f && camera.GetPosition().Y == 15.0f &&
                  camera.GetPosition().Z == 50.0f,
              "no keys held leaves the camera position unchanged");
    }

    // --- dt scales distance linearly (half the time, roughly half the distance) ---
    {
        GEWorldEditor editorFull;
        editorFull.EnterEditing(0.0f, 0.0f, 0.0f);
        Easy3D::Camera3D cameraFull;
        editorFull.Update(KeyboardState{Keys::W}, restMouse, 1.0f, 800, 480, cameraFull, emptyWorld);
        const float fullDistanceZ = cameraFull.GetPosition().Z;

        GEWorldEditor editorHalf;
        editorHalf.EnterEditing(0.0f, 0.0f, 0.0f);
        Easy3D::Camera3D cameraHalf;
        editorHalf.Update(KeyboardState{Keys::W}, restMouse, 0.5f, 800, 480, cameraHalf, emptyWorld);
        const float halfDistanceZ = cameraHalf.GetPosition().Z;

        check(std::fabs(halfDistanceZ - fullDistanceZ * 0.5f) < 0.001f,
              "movement distance scales linearly with dt");
    }

    // --- Raycast: straight down hits a single floor block directly below ---
    {
        World world;
        world.setBlock(50, 4, 50, Block::make(1));
        const RaycastHit hit = Raycast(world, 50.0f, 10.0f, 50.0f, 0.0f, -1.0f, 0.0f, 200.0f);
        check(hit.hit, "straight-down ray hits the floor block");
        check(hit.x == 50 && hit.y == 4 && hit.z == 50, "hit reports the correct cell (50,4,50)");
        check(hit.normalY == 1 && hit.normalX == 0 && hit.normalZ == 0,
              "hit face normal points straight up (+Y), the side the ray arrived from");
        check(std::fabs(hit.distance - 5.5f) < 0.001f,
              "hit distance is exact: origin y=10 down to the block's real top face at y=4.5 "
              "(block index 4 spans [3.5,4.5), the block-centering convention)");
    }

    // --- Raycast: a ray that never reaches any block is a clean miss ---
    {
        World world;
        world.setBlock(50, 4, 50, Block::make(1));
        const RaycastHit hit = Raycast(world, 50.0f, 10.0f, 50.0f, 0.0f, 1.0f, 0.0f, 200.0f);
        check(!hit.hit, "a ray aimed away from the only block in the world misses");
    }

    // --- Raycast: horizontal hit reports a horizontal face normal ---
    {
        World world;
        world.setBlock(60, 5, 50, Block::make(1));
        const RaycastHit hit = Raycast(world, 50.0f, 5.0f, 50.0f, 1.0f, 0.0f, 0.0f, 200.0f);
        check(hit.hit && hit.x == 60 && hit.y == 5 && hit.z == 50,
              "a ray fired along +X hits the block placed 10 cells east");
        check(hit.normalX == -1 && hit.normalY == 0 && hit.normalZ == 0,
              "hit face normal points back toward -X, the side the ray arrived from");
    }

    // --- Raycast: origin already inside a solid block reports no crossed face ---
    {
        World world;
        world.setBlock(50, 5, 50, Block::make(1));
        const RaycastHit hit = Raycast(world, 50.0f, 5.0f, 50.0f, 0.0f, -1.0f, 0.0f, 200.0f);
        check(hit.hit && hit.x == 50 && hit.y == 5 && hit.z == 50,
              "a ray whose origin starts inside a solid block reports that same block as the hit");
        check(hit.normalX == 0 && hit.normalY == 0 && hit.normalZ == 0,
              "no face normal is reported when the origin starts inside the hit block");
        check(hit.distance == 0.0f, "distance is zero when the origin starts inside the hit block");
    }

    // --- Raycast: maxDistance stops the search before it reaches a farther block ---
    {
        World world;
        world.setBlock(90, 5, 50, Block::make(1));
        const RaycastHit hit = Raycast(world, 50.0f, 5.0f, 50.0f, 1.0f, 0.0f, 0.0f, 10.0f);
        check(!hit.hit, "a block far beyond maxDistance is correctly not reached");
    }

    // --- Raycast: out-of-bounds origin still finds a block once the ray enters the grid ---
    {
        World world;
        world.setBlock(5, 5, 50, Block::make(1));
        const RaycastHit hit = Raycast(world, -20.0f, 5.0f, 50.0f, 1.0f, 0.0f, 0.0f, 200.0f);
        check(hit.hit && hit.x == 5 && hit.y == 5 && hit.z == 50,
              "a ray starting outside the world bounds still finds a block once it enters the grid");
    }

    // --- Block place/remove + ConsumeNeedsPresentationRebuild() ---
    // A thick slab at raw grid X=50 spanning the whole Z range and y=0..3 --
    // EnterEditing(0,10,0) puts the camera at raw grid (50,10,50) with its
    // documented default look (yaw=0, pitch=-0.35), so forward.X is exactly
    // 0 (sin(0)==0) and the ray never drifts off this X=50 plane, guaranteed
    // to hit the slab somewhere in Z regardless of the exact descent rate.
    {
        World world;
        for (int z = 0; z < 100; ++z)
        {
            for (int y = 0; y < 4; ++y)
            {
                world.setBlock(50, static_cast<std::uint16_t>(y), static_cast<std::uint16_t>(z), Block::make(1));
            }
        }

        // Ground truth: the exact same raycast GEWorldEditor::Update() will
        // perform internally, computed directly against GEVoxelRaycast
        // (already exhaustively verified above) using EnterEditing()'s own
        // documented default yaw/pitch.
        constexpr float kDefaultYaw = 0.0f;
        constexpr float kDefaultPitch = -0.35f;
        const float cosPitch = std::cos(kDefaultPitch);
        const RaycastHit expected = Raycast(world, 50.0f, 10.0f, 50.0f,
                                             std::sin(kDefaultYaw) * cosPitch, std::sin(kDefaultPitch),
                                             -std::cos(kDefaultYaw) * cosPitch, 200.0f);
        check(expected.hit, "test setup sanity: the reference raycast against the slab hits");
        const int placeX = static_cast<int>(expected.x) + expected.normalX;
        const int placeY = static_cast<int>(expected.y) + expected.normalY;
        const int placeZ = static_cast<int>(expected.z) + expected.normalZ;
        check(world.getBlock(static_cast<std::uint16_t>(placeX), static_cast<std::uint16_t>(placeY),
                              static_cast<std::uint16_t>(placeZ)).isAir(),
              "test setup sanity: the cell a left-click should place into starts as air");

        GEWorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        Easy3D::Camera3D camera;

        // A plain Update() with no clicks performs the raycast but doesn't
        // mutate anything.
        editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, world);
        check(!editor.ConsumeNeedsPresentationRebuild(),
              "a plain Update() with no clicks doesn't request a rebuild");

        // Left click places a block at the expected adjacent cell.
        const MouseState leftMouse(0, 0, 0, ButtonState::Pressed, ButtonState::Released,
                                    ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, leftMouse, 0.0f, 800, 480, camera, world);
        check(!world.getBlock(static_cast<std::uint16_t>(placeX), static_cast<std::uint16_t>(placeY),
                               static_cast<std::uint16_t>(placeZ)).isAir(),
              "left click places a block at the cell adjacent to the hit face");
        check(editor.ConsumeNeedsPresentationRebuild(),
              "placing a block requests a presentation rebuild");
        check(!editor.ConsumeNeedsPresentationRebuild(),
              "ConsumeNeedsPresentationRebuild() clears back to false once read");

        // Holding left across a second frame doesn't place a second time
        // (edge-triggered, not fired every frame while held).
        editor.Update(KeyboardState{}, leftMouse, 0.0f, 800, 480, camera, world);
        check(!editor.ConsumeNeedsPresentationRebuild(),
              "holding the left button across frames only places once (edge-triggered)");

        // Middle click removes the aimed-at block -- by now that's the
        // block JUST placed above the original slab surface (closer to
        // the camera, so the fresh raycast this Update() call performs
        // hits it first, not the original `expected` slab cell underneath).
        const MouseState middleMouse(0, 0, 0, ButtonState::Released, ButtonState::Pressed,
                                      ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, middleMouse, 0.0f, 800, 480, camera, world);
        check(world.getBlock(static_cast<std::uint16_t>(placeX), static_cast<std::uint16_t>(placeY),
                              static_cast<std::uint16_t>(placeZ)).isAir(),
              "middle click removes the aimed-at block (the just-placed one, now the nearest hit)");
        check(!world.getBlock(expected.x, expected.y, expected.z).isAir(),
              "the original slab surface underneath is untouched by the removal");
        check(editor.ConsumeNeedsPresentationRebuild(),
              "removing a block requests a presentation rebuild");
    }

    // --- Save/load round-trip via Enter ---
    {
        World world;
        world.setBlock(50, 4, 50, Block::make(42));
        GEWorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        editor.SetWorldPath("verify_ge_world_editor_scratch.vwr");
        Easy3D::Camera3D camera;

        const KeyboardState enterKeys{Keys::Enter};
        editor.Update(enterKeys, restMouse, 0.0f, 800, 480, camera, world);

        const World reloaded = World::loadFromFile("verify_ge_world_editor_scratch.vwr");
        check(reloaded.getBlock(50, 4, 50).type() == 42,
              "Enter saves the world to the path set via SetWorldPath(), and it round-trips");
        std::remove("verify_ge_world_editor_scratch.vwr");
    }

    // --- GEEditCommandStack: push/undo/redo, replaying real before/after values ---
    {
        World world;
        world.setBlock(10, 5, 10, Block::make(1));

        GEEditCommandStack stack;
        GEEditCommand place;
        place.kind = GEEditCommand::Kind::BlockEdit;
        place.blockChanges.push_back({10, 5, 10, Block::make(1), Block::make(2)});
        world.setBlock(10, 5, 10, Block::make(2));
        stack.Push(place);

        check(stack.UndoCount() == 1 && stack.RedoCount() == 0,
              "Push() records one undo entry and starts with an empty redo stack");

        check(stack.Undo(world), "Undo() reports success when there's something to undo");
        check(world.getBlock(10, 5, 10).type() == 1, "Undo() restores the real \"before\" value");
        check(stack.UndoCount() == 0 && stack.RedoCount() == 1,
              "Undo() moves the entry from the undo stack to the redo stack");

        check(stack.Redo(world), "Redo() reports success when there's something to redo");
        check(world.getBlock(10, 5, 10).type() == 2, "Redo() re-applies the real \"after\" value");
        check(stack.UndoCount() == 1 && stack.RedoCount() == 0,
              "Redo() moves the entry back onto the undo stack");
    }
    {
        World world;
        GEEditCommandStack stack;
        check(!stack.Undo(world), "Undo() on an empty stack is a no-op returning false");
        check(!stack.Redo(world), "Redo() on an empty stack is a no-op returning false");
    }
    {
        // Pushing a new command after an Undo() discards the stale redo
        // entry -- standard undo/redo semantics, not "branching" history.
        World world;
        GEEditCommandStack stack;
        GEEditCommand first;
        first.blockChanges.push_back({1, 1, 1, Block::air(), Block::make(1)});
        stack.Push(first);
        stack.Undo(world);
        check(stack.RedoCount() == 1, "test setup sanity: one entry is on the redo stack after Undo()");

        GEEditCommand second;
        second.blockChanges.push_back({2, 2, 2, Block::air(), Block::make(2)});
        stack.Push(second);
        check(stack.RedoCount() == 0,
              "pushing a new command after an Undo() clears the now-stale redo stack");
        check(stack.UndoCount() == 1, "the newly pushed command is the only undo entry");
    }
    {
        // Depth cap: pushing well beyond the cap evicts the OLDEST entries
        // first, keeping the stack bounded.
        World world;
        GEEditCommandStack stack;
        constexpr int kPushCount = 250; // > the 200 documented cap
        for (int i = 0; i < kPushCount; ++i)
        {
            GEEditCommand command;
            command.blockChanges.push_back(
                {0, 0, 0, Block::air(), Block::make(static_cast<std::uint16_t>((i % 4000) + 1))});
            stack.Push(command);
        }
        check(stack.UndoCount() == 200, "the undo stack is capped at its documented 200-entry depth");
    }

    // --- GEWorldEditor: U/R undo/redo a real place through the same synthetic-input path ---
    {
        World world;
        for (int z = 0; z < 100; ++z)
        {
            for (int y = 0; y < 4; ++y)
            {
                world.setBlock(50, static_cast<std::uint16_t>(y), static_cast<std::uint16_t>(z), Block::make(1));
            }
        }
        GEWorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        Easy3D::Camera3D camera;

        constexpr float kDefaultYaw = 0.0f;
        constexpr float kDefaultPitch = -0.35f;
        const float cosPitch = std::cos(kDefaultPitch);
        const RaycastHit expected = Raycast(world, 50.0f, 10.0f, 50.0f,
                                             std::sin(kDefaultYaw) * cosPitch, std::sin(kDefaultPitch),
                                             -std::cos(kDefaultYaw) * cosPitch, 200.0f);
        const std::uint16_t placeX = static_cast<std::uint16_t>(expected.x + expected.normalX);
        const std::uint16_t placeY = static_cast<std::uint16_t>(expected.y + expected.normalY);
        const std::uint16_t placeZ = static_cast<std::uint16_t>(expected.z + expected.normalZ);

        const MouseState leftMouse(0, 0, 0, ButtonState::Pressed, ButtonState::Released,
                                    ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, leftMouse, 0.0f, 800, 480, camera, world);
        check(!world.getBlock(placeX, placeY, placeZ).isAir(), "test setup sanity: the left click placed a block");
        (void)editor.ConsumeNeedsPresentationRebuild();

        const KeyboardState undoKeys{Keys::U};
        editor.Update(undoKeys, restMouse, 0.0f, 800, 480, camera, world);
        check(world.getBlock(placeX, placeY, placeZ).isAir(),
              "the U key undoes the real placement made through Update()");
        check(editor.ConsumeNeedsPresentationRebuild(), "a successful undo requests a presentation rebuild");

        const KeyboardState redoKeys{Keys::R};
        editor.Update(redoKeys, restMouse, 0.0f, 800, 480, camera, world);
        check(!world.getBlock(placeX, placeY, placeZ).isAir(),
              "the R key redoes the placement back");
        check(editor.ConsumeNeedsPresentationRebuild(), "a successful redo requests a presentation rebuild");
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
