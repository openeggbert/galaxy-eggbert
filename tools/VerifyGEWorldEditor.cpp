#include "Editor/GEBoxRegion.hpp"
#include "Editor/GECustomWorldStorage.hpp"
#include "Editor/GEEditCommandStack.hpp"
#include "Editor/GEEditorBrowserScreen.hpp"
#include "Editor/GEEditorPalette.hpp"
#include "Editor/GEPaletteCategories.hpp"
#include "Editor/GEVoxelRaycast.hpp"
#include "Editor/GEWorldEditor.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <cmath>
#include <cstdio>
#include <iostream>
#include <set>

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
//   - EDITOR-105: GEBoxRegion::NormalizeAndClamp directly (unordered
//     corners, world-bounds clamping), plus GEWorldEditor's F-key box-fill
//     tool filling an exact expected volume as one undo command through
//     the same synthetic-input path.
//   - EDITOR-106: GEPaletteCategories data sanity (full 1..440 coverage,
//     every curated id in range), plus GEEditorPalette::Update()'s
//     click-hit-testing (toolbar buttons, tab toggle, icon selection,
//     clickConsumed) driven with synthetic MouseState values -- still no
//     GraphicsDevice needed (only Draw() touches the GPU).
//   - EDITOR-107: GECustomWorldStorage's real filesystem behavior (dir
//     naming, listing, NextNewWorldPath's numbering) against a throwaway
//     gamer-slot fixture directory cleaned up before and after; plus
//     GEEditorBrowserScreen's New/Open/delete-confirm click-hit-testing
//     and GEWorldEditor's browsing-state transitions (EnterBrowser/
//     IsBrowsing/UpdateBrowsing/EnterEditing/ExitBrowser) -- none of this
//     needs a GraphicsDevice either (only Draw()/DrawBrowsing() do).
//   - EDITOR-108: GEWorldEditor's Play-Test toolbar button -- clicking it
//     saves the world to GetWorldPath() and sets ConsumePlayTestRequested(),
//     driven through the same synthetic-input path as earlier sections
//     (GalaxyEggbertCnaGame's own phase-switch/checkpoint-skip side of
//     this needs a real GraphicsDevice/game loop, live-verified instead).
//   - EDITOR-109: ConfirmedObjectCategories()/AllObjectTypeIdsInOrder()
//     data sanity (full 1..203 coverage, curated ids in range, no
//     duplicates, effect-only types excluded), GEEditorPalette's Blocks/
//     Objects mode toggle and independent per-mode selections,
//     GEEditCommandStack's MoveObjectEdit undo/redo (place AND overwrite,
//     where undo must restore the previous record rather than empty the
//     cell), and a real left click in Objects mode placing a stationary
//     MoveObjectRecord through GEWorldEditor::Update() -- all still
//     GraphicsDevice-free.
//   - EDITOR-110: nearest-billboard object picking (G, reusing
//     GEHud::ProjectWorldToHudSpace() -- so this section links GEHud.cpp
//     too, see CMakeLists.txt), patrol-target assignment (T), the 5-field
//     cycle + adjust tool (Tab/OemPlus/OemMinus), and object removal
//     (Delete), each as a real GEEditCommand::Kind::MoveObjectEdit pushed
//     through GEWorldEditor::Update() -- still GraphicsDevice-free (only
//     Draw()'s new selection-highlight cube needs one).
// Later milestones (sky-region round-trips, the hardening pass) add their
// own sections here.
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

    // --- GEBoxRegion::NormalizeAndClamp: unordered corners + world-bounds clamping ---
    {
        const BoxRegion region = NormalizeAndClamp(5, 20, 8, 2, 10, 30, 100);
        check(region.minX == 2 && region.maxX == 5, "X is normalized regardless of which corner is larger");
        check(region.minY == 10 && region.maxY == 20, "Y is normalized regardless of which corner is larger");
        check(region.minZ == 8 && region.maxZ == 30, "Z is normalized regardless of which corner is larger");
    }
    {
        const BoxRegion region = NormalizeAndClamp(-5, 50, 105, 3, 200, 99, 100);
        check(region.minX == 0, "a negative corner clamps to 0, not wrapping/underflowing");
        check(region.maxY == 99, "a corner beyond blocksPerAxis clamps to the last valid index");
        check(region.minZ == 99 && region.maxZ == 99,
              "both corners already at/beyond the last valid index still clamp to a valid single-cell range");
    }

    // --- GEWorldEditor: F-key box-fill fills an exact expected volume as one undo command ---
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
        constexpr float kDefaultFlySpeed = 15.0f; // GEWorldEditor's own documented default
        const float cosPitch = std::cos(kDefaultPitch);
        const float fwdX = std::sin(kDefaultYaw) * cosPitch;
        const float fwdY = std::sin(kDefaultPitch);
        const float fwdZ = -std::cos(kDefaultYaw) * cosPitch;

        const RaycastHit cornerAHit = Raycast(world, 50.0f, 10.0f, 50.0f, fwdX, fwdY, fwdZ, 200.0f);
        check(cornerAHit.hit, "test setup sanity: corner A raycast hits the slab");

        const KeyboardState fKeys{Keys::F};
        editor.Update(fKeys, restMouse, 0.0f, 800, 480, camera, world); // press F: picks corner A

        // Move the camera (F released, so the edge-trigger re-arms) before
        // picking corner B, so the two corners are genuinely different
        // cells. Space (world-up), not W: moving straight up shifts the
        // ray's ORIGIN off the diagonal look line without changing its
        // direction, so the new ray is a parallel shift that crosses the
        // slab's top face at a genuinely different Z -- moving along the
        // look direction itself (W) would retrace the exact same ray and
        // hit the exact same point, since it's already the direction being
        // raycast along.
        constexpr float kMoveDt = 0.3f;
        editor.Update(KeyboardState{Keys::Space}, restMouse, kMoveDt, 800, 480, camera, world);

        // Independently compute where the camera/ray ended up, using the
        // exact same movement math GEWorldEditor::Update() applies.
        const float camYAfterMove = 10.0f + 1.0f * kDefaultFlySpeed * kMoveDt; // Space moves along world-up (0,1,0)
        const RaycastHit cornerBHit = Raycast(world, 50.0f, camYAfterMove, 50.0f,
                                               fwdX, fwdY, fwdZ, 200.0f);
        check(cornerBHit.hit, "test setup sanity: corner B raycast hits the slab at the new position");
        check(cornerBHit.z != cornerAHit.z,
              "test setup sanity: corner B is a genuinely different cell than corner A");

        editor.Update(fKeys, restMouse, 0.0f, 800, 480, camera, world); // press F again: fills

        const auto expectedRegion = NormalizeAndClamp(cornerAHit.x, cornerAHit.y, cornerAHit.z,
                                                        cornerBHit.x, cornerBHit.y, cornerBHit.z,
                                                        static_cast<int>(world.blocksPerAxis()));
        bool allFilled = true;
        int expectedVolume = 0;
        for (int x = expectedRegion.minX; x <= expectedRegion.maxX; ++x)
        {
            for (int y = expectedRegion.minY; y <= expectedRegion.maxY; ++y)
            {
                for (int z = expectedRegion.minZ; z <= expectedRegion.maxZ; ++z)
                {
                    ++expectedVolume;
                    if (world.getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                        static_cast<std::uint16_t>(z))
                            .type() != GalaxyEggbert::BlockTypes::RockPile)
                    {
                        allFilled = false;
                    }
                }
            }
        }
        check(expectedVolume > 1, "test setup sanity: the box spans more than a single cell");
        check(allFilled, "the second F press fills every cell in the expected region with RockPile");
        check(editor.ConsumeNeedsPresentationRebuild(), "box-fill requests a presentation rebuild");

        editor.Update(KeyboardState{Keys::U}, restMouse, 0.0f, 800, 480, camera, world);
        check(world.getBlock(cornerAHit.x, cornerAHit.y, cornerAHit.z).type() == 1,
              "undo restores corner A's real original block in one step");
        check(world.getBlock(cornerBHit.x, cornerBHit.y, cornerBHit.z).type() == 1,
              "undo restores corner B's real original block in the SAME undo step (one fill == one command)");
        check(editor.ConsumeNeedsPresentationRebuild(), "undoing the fill requests a presentation rebuild");
    }

    // --- GEPaletteCategories: full numeric coverage + curated-id sanity ---
    {
        const auto allIds = AllBlockIconIdsInOrder();
        check(allIds.size() == 440, "AllBlockIconIdsInOrder() covers all 440 valid non-Air block types");
        check(allIds.front() == 1 && allIds.back() == 440,
              "AllBlockIconIdsInOrder() is in order, starting at 1 (Air excluded) through 440");

        bool allCuratedInRange = true;
        int curatedCount = 0;
        for (const auto& category : ConfirmedBlockCategories())
        {
            check(!category.name.empty(), "every curated category has a non-empty name");
            for (const int iconId : category.iconIds)
            {
                ++curatedCount;
                if (iconId < 1 || iconId > 440)
                {
                    allCuratedInRange = false;
                }
            }
        }
        check(allCuratedInRange, "every curated category's icon ids fall within the valid 1..440 range");
        check(curatedCount > 0, "test setup sanity: at least one curated category icon exists");
    }

    // --- GEEditorPalette: click hit-testing (toolbar/tab/icon selection/clickConsumed) ---
    {
        constexpr int kViewportW = 800;
        constexpr int kViewportH = 480;

        const auto click = [&](GEEditorPalette& palette, float x, float y)
        {
            const MouseState down(static_cast<int>(x), static_cast<int>(y), 0, ButtonState::Pressed,
                                  ButtonState::Released, ButtonState::Released, ButtonState::Released,
                                  ButtonState::Released);
            const MouseState up(static_cast<int>(x), static_cast<int>(y), 0, ButtonState::Released,
                                ButtonState::Released, ButtonState::Released, ButtonState::Released,
                                ButtonState::Released);
            (void)palette.Update(down, kViewportW, kViewportH);
            return palette.Update(up, kViewportW, kViewportH);
        };

        // Layout constants mirroring GEEditorPalette.cpp's own private
        // anonymous-namespace geometry (kToolbarX/Y0/ButtonSize/Gap,
        // kPaletteIconSize/Gap/Cols/Rows, kGridMargin, kTabToggle
        // Width/Height, kControlGap) -- a white-box test of this specific
        // layout, matching this project's own precedent of pinning exact
        // internal geometry (e.g. VerifyGEInputPad's real button rects).
        constexpr float kGridX0 = 800.0f - (8 * (40.0f + 4.0f) - 4.0f) - 10.0f; // 442
        constexpr float kGridY0 = 480.0f - (4 * (40.0f + 4.0f) - 4.0f) - 10.0f; // 298

        {
            GEEditorPalette palette;
            check(palette.SelectedBlockType() == GalaxyEggbert::BlockTypes::RockPile,
                  "GEEditorPalette starts with RockPile selected by default");

            const auto result = click(palette, 30.0f, 30.0f); // toolbar button 0 (Undo)
            check(result.action == GEEditorPalette::ToolbarAction::Undo,
                  "clicking the first toolbar button reports Undo");
            check(result.clickConsumed, "a toolbar-button click reports clickConsumed");
        }
        {
            GEEditorPalette palette;
            const auto result = click(palette, 30.0f, 90.0f); // toolbar button 1 (Redo)
            check(result.action == GEEditorPalette::ToolbarAction::Redo,
                  "clicking the second toolbar button reports Redo");
        }
        {
            GEEditorPalette palette;
            const auto result = click(palette, 30.0f, 146.0f); // toolbar button 2 (Save)
            check(result.action == GEEditorPalette::ToolbarAction::Save,
                  "clicking the third toolbar button reports Save");
        }
        {
            GEEditorPalette palette;
            const auto result = click(palette, 400.0f, 200.0f); // empty space, no UI there
            check(result.action == GEEditorPalette::ToolbarAction::None &&
                      !result.clickConsumed,
                  "clicking empty space away from any control reports no action and doesn't consume the click");
        }
        {
            GEEditorPalette palette;
            // Confirmed tab, cell index 1 (col=1,row=0): the second icon in
            // the first curated category ("Terrain"), BrickWall.
            const float cellX = kGridX0 + 1.0f * (40.0f + 4.0f) + 20.0f;
            const float cellY = kGridY0 + 20.0f;
            const auto result = click(palette, cellX, cellY);
            check(result.clickConsumed, "clicking a palette icon cell reports clickConsumed");
            check(palette.SelectedBlockType() == GalaxyEggbert::BlockTypes::BrickWall,
                  "clicking the Confirmed tab's second icon selects BrickWall");
        }
        {
            GEEditorPalette palette;
            // Toggle to the All-Icons tab, then click cell index 0 -- should
            // now select icon id 1 (AllBlockIconIdsInOrder()'s own first
            // entry), proving the tab toggle actually changed the active list.
            const float tabX = kGridX0 + 10.0f;
            const float tabY = kGridY0 - 24.0f - 4.0f + 10.0f;
            (void)click(palette, tabX, tabY);
            const float cellX = kGridX0 + 20.0f;
            const float cellY = kGridY0 + 20.0f;
            const auto result = click(palette, cellX, cellY);
            check(result.clickConsumed, "clicking a palette icon cell on the All-Icons tab reports clickConsumed");
            check(palette.SelectedBlockType() == 1,
                  "after toggling to the All-Icons tab, the first cell selects icon id 1");
        }
    }

    // --- GECustomWorldStorage: dir naming, listing, NextNewWorldPath numbering ---
    {
        // A gamer slot number unlikely to collide with anything else this
        // test binary (or the real game) touches; cleaned up before and
        // after so repeat runs never accumulate or see stale state.
        constexpr int kTestGamerSlot = 77;
        std::error_code ec;
        std::filesystem::remove_all(CustomWorldsDir(kTestGamerSlot), ec);

        check(CustomWorldsDir(kTestGamerSlot) == std::filesystem::path("customworlds") / "gamer77",
              "CustomWorldsDir() builds the expected per-gamer-slot path");
        check(ListCustomWorlds(kTestGamerSlot).empty(),
              "ListCustomWorlds() is empty for a gamer slot with no directory yet");

        const auto first = NextNewWorldPath(kTestGamerSlot);
        check(first.filename() == "custom_001.vwr",
              "NextNewWorldPath() picks custom_001.vwr when the directory is empty");
        check(std::filesystem::exists(CustomWorldsDir(kTestGamerSlot)),
              "NextNewWorldPath() creates the gamer slot's directory");

        // NextNewWorldPath() only reserves a name -- matches how the real
        // browser's New World action uses it (saving a real World there
        // is a separate step).
        World().saveToFile(first);
        const auto second = NextNewWorldPath(kTestGamerSlot);
        check(second.filename() == "custom_002.vwr",
              "NextNewWorldPath() picks the next unused number once the first exists");

        World().saveToFile(second);
        check(ListCustomWorlds(kTestGamerSlot).size() == 2, "ListCustomWorlds() finds both created worlds");

        std::filesystem::remove_all(CustomWorldsDir(kTestGamerSlot), ec);
    }

    // --- GEEditorBrowserScreen: New/Open/delete-confirm click hit-testing ---
    {
        constexpr int kTestGamerSlot = 78;
        std::error_code ec;
        std::filesystem::remove_all(CustomWorldsDir(kTestGamerSlot), ec);
        World().saveToFile(NextNewWorldPath(kTestGamerSlot)); // custom_001.vwr

        GEEditorBrowserScreen browser;
        browser.Refresh(kTestGamerSlot);

        const auto click = [](GEEditorBrowserScreen& b, float x, float y)
        {
            const MouseState down(static_cast<int>(x), static_cast<int>(y), 0, ButtonState::Pressed,
                                  ButtonState::Released, ButtonState::Released, ButtonState::Released,
                                  ButtonState::Released);
            const MouseState up(static_cast<int>(x), static_cast<int>(y), 0, ButtonState::Released,
                                ButtonState::Released, ButtonState::Released, ButtonState::Released,
                                ButtonState::Released);
            (void)b.Update(down, 800, 480);
            return b.Update(up, 800, 480);
        };

        {
            // Row 0 ("+ New World"): (200,20)-(520,56).
            const auto result = click(browser, 300.0f, 38.0f);
            check(result.action == GEEditorBrowserScreen::Action::New,
                  "clicking the New World row reports Action::New");
        }
        {
            // Row 1 (the one created world): y0 = 20 + 1*(36+6) = 62, so (200,62)-(520,98).
            const auto result = click(browser, 300.0f, 80.0f);
            check(result.action == GEEditorBrowserScreen::Action::Open,
                  "clicking a world row reports Action::Open");
            check(!result.path.empty() && result.path.extension() == ".vwr",
                  "the Open action reports a real .vwr path");
        }
        {
            // Row 1's delete "X": (488,66)-(516,94).
            const auto firstClick = click(browser, 500.0f, 80.0f);
            check(firstClick.action == GEEditorBrowserScreen::Action::None,
                  "the first click on a row's delete X doesn't report an action (just arms the confirm)");
            check(!ListCustomWorlds(kTestGamerSlot).empty(),
                  "the world file still exists after only arming delete");

            (void)click(browser, 500.0f, 80.0f);
            check(ListCustomWorlds(kTestGamerSlot).empty(),
                  "a second click on the SAME delete X actually deletes the file");
        }

        std::filesystem::remove_all(CustomWorldsDir(kTestGamerSlot), ec);
    }

    // --- GEWorldEditor: browsing-state transitions ---
    {
        constexpr int kTestGamerSlot = 79;
        std::error_code ec;
        std::filesystem::remove_all(CustomWorldsDir(kTestGamerSlot), ec);

        GEWorldEditor editor;
        check(editor.IsBrowsing(), "GEWorldEditor starts in browsing mode by default");

        editor.EnterBrowser(kTestGamerSlot);
        check(editor.IsBrowsing(), "EnterBrowser() stays in browsing mode");

        const MouseState down(300, 38, 0, ButtonState::Pressed, ButtonState::Released, ButtonState::Released,
                              ButtonState::Released, ButtonState::Released);
        const MouseState up(300, 38, 0, ButtonState::Released, ButtonState::Released, ButtonState::Released,
                            ButtonState::Released, ButtonState::Released);
        (void)editor.UpdateBrowsing(down, 800, 480);
        const auto request = editor.UpdateBrowsing(up, 800, 480);
        check(request.shouldCreateNew,
              "clicking New World reports shouldCreateNew via GEWorldEditor::UpdateBrowsing()");

        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        check(!editor.IsBrowsing(), "EnterEditing() leaves browsing mode");

        std::filesystem::remove_all(CustomWorldsDir(kTestGamerSlot), ec);
    }
    {
        GEWorldEditor editor;
        editor.EnterBrowser(0);
        editor.ExitBrowser();
        check(!editor.IsBrowsing(), "ExitBrowser() leaves browsing mode");
    }

    // --- GEWorldEditor: Play-Test toolbar button saves + requests a play-test session ---
    {
        World world;
        world.setBlock(10, 5, 10, Block::make(42));
        GEWorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        editor.SetWorldPath("verify_ge_world_editor_playtest_scratch.vwr");
        Easy3D::Camera3D camera;

        check(!editor.ConsumePlayTestRequested(),
              "test setup sanity: no play-test requested before any click");

        // Toolbar button 4 (Play-Test): y0 = 10 + 4*(48+8) = 234, so (10,234)-(58,282).
        const MouseState down(30, 258, 0, ButtonState::Pressed, ButtonState::Released,
                              ButtonState::Released, ButtonState::Released, ButtonState::Released);
        const MouseState up(30, 258, 0, ButtonState::Released, ButtonState::Released,
                            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, down, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{}, up, 0.0f, 800, 480, camera, world);

        check(editor.GetWorldPath() == std::filesystem::path("verify_ge_world_editor_playtest_scratch.vwr"),
              "GetWorldPath() returns the path set via SetWorldPath()");
        check(editor.ConsumePlayTestRequested(),
              "clicking the Play-Test toolbar button sets ConsumePlayTestRequested()");
        check(!editor.ConsumePlayTestRequested(),
              "ConsumePlayTestRequested() clears back to false once read");

        const World reloaded = World::loadFromFile("verify_ge_world_editor_playtest_scratch.vwr");
        check(reloaded.getBlock(10, 5, 10).type() == 42,
              "the Play-Test button saves the world before requesting the session, and it round-trips");
        std::remove("verify_ge_world_editor_playtest_scratch.vwr");
    }

    // --- Regression: a palette click must not ALSO edit the world behind it ---
    // The palette fires its action on mouse RELEASE, but GEWorldEditor's own
    // place/remove clicks are edge-triggered on PRESS -- so before this was
    // fixed (found while wiring EDITOR-109, but present since EDITOR-106),
    // every palette icon/toolbar click also placed a block at the crosshair
    // on the way down. Exercised through GEWorldEditor::Update() rather than
    // GEEditorPalette::Update() in isolation, because only the two together
    // show the mismatch.
    {
        World world;
        for (int z = 0; z < 100; ++z)
        {
            for (int y = 0; y < 4; ++y)
            {
                world.setBlock(50, static_cast<std::uint16_t>(y), static_cast<std::uint16_t>(z), Block::make(1));
            }
        }
        // Ground truth for "the world is unchanged": the solid-block count
        // over the slab's own X plane, which is all a crosshair place/remove
        // from EnterEditing(0,10,0)'s default look could possibly touch.
        const auto solidCountOnSlabPlane = [&world]()
        {
            int count = 0;
            for (int y = 0; y < 100; ++y)
            {
                for (int z = 0; z < 100; ++z)
                {
                    if (!world.getBlock(50, static_cast<std::uint16_t>(y),
                                        static_cast<std::uint16_t>(z)).isAir())
                    {
                        ++count;
                    }
                }
            }
            return count;
        };
        const int before = solidCountOnSlabPlane();

        GEWorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        Easy3D::Camera3D camera;

        constexpr float kGridX0 = 800.0f - (8 * (40.0f + 4.0f) - 4.0f) - 10.0f;
        constexpr float kGridY0 = 480.0f - (4 * (40.0f + 4.0f) - 4.0f) - 10.0f;
        const auto pressAndRelease = [&](float x, float y)
        {
            const MouseState down(static_cast<int>(x), static_cast<int>(y), 0, ButtonState::Pressed,
                                  ButtonState::Released, ButtonState::Released, ButtonState::Released,
                                  ButtonState::Released);
            const MouseState up(static_cast<int>(x), static_cast<int>(y), 0, ButtonState::Released,
                                ButtonState::Released, ButtonState::Released, ButtonState::Released,
                                ButtonState::Released);
            editor.Update(KeyboardState{}, down, 0.0f, 800, 480, camera, world);
            editor.Update(KeyboardState{}, up, 0.0f, 800, 480, camera, world);
        };

        pressAndRelease(kGridX0 + 20.0f, kGridY0 + 20.0f); // a block-palette icon cell
        check(solidCountOnSlabPlane() == before,
              "clicking a palette icon in Blocks mode doesn't also place a block at the crosshair");
        check(!editor.ConsumeNeedsPresentationRebuild(),
              "a palette icon click alone doesn't request a presentation rebuild");

        pressAndRelease(30.0f, 30.0f); // toolbar button 0 (Undo) -- nothing to undo
        check(solidCountOnSlabPlane() == before,
              "clicking a toolbar button doesn't also place a block at the crosshair");

        // A press that STARTS in the 3D view still edits normally, even
        // though the release lands on the palette -- the claim is per-press,
        // not "the cursor is over the palette right now".
        const MouseState downInView(400, 200, 0, ButtonState::Pressed, ButtonState::Released,
                                    ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, downInView, 0.0f, 800, 480, camera, world);
        check(solidCountOnSlabPlane() == before + 1,
              "a left click out in the 3D view still places a block normally");
        check(editor.ConsumeNeedsPresentationRebuild(),
              "a real 3D-view place still requests a presentation rebuild");
    }

    // --- GEPaletteCategories: object-type coverage + curated-id sanity ---
    {
        const auto allTypes = AllObjectTypeIdsInOrder();
        check(allTypes.size() == 203,
              "AllObjectTypeIdsInOrder() covers all 203 placeable ObjectType ids (ObjectType0 excluded)");
        check(allTypes.front() == 1 && allTypes.back() == 203,
              "AllObjectTypeIdsInOrder() is in order, starting at 1 (the null slot excluded) through 203");

        bool allCuratedInRange = true;
        int curatedCount = 0;
        std::set<int> seenTypeIds;
        bool noDuplicatesAcrossCategories = true;
        for (const auto& category : ConfirmedObjectCategories())
        {
            check(!category.name.empty(), "every curated object category has a non-empty name");
            for (const int typeId : category.iconIds)
            {
                ++curatedCount;
                if (typeId < 1 || typeId > 203)
                {
                    allCuratedInRange = false;
                }
                if (!seenTypeIds.insert(typeId).second)
                {
                    noDuplicatesAcrossCategories = false;
                }
            }
        }
        check(allCuratedInRange, "every curated object category's ids fall within the valid 1..203 range");
        check(curatedCount > 0, "test setup sanity: at least one curated object category id exists");
        check(noDuplicatesAcrossCategories,
              "no ObjectType id appears in more than one curated object category");

        // Spot-check that the curated groups actually match ObjectType.hpp's
        // own documented comment groups -- the whole point of the "don't
        // invent semantics" rule is that these ids are copied from there,
        // not guessed.
        const auto categories = ConfirmedObjectCategories();
        check(categories.front().name == "Platform Lifts" &&
                  categories.front().iconIds == std::vector<int>{1, 47, 48},
              "the Platform Lifts category matches ObjectType.hpp's own lift group (1, 47, 48)");
        check(seenTypeIds.count(6) == 1, "the extra-life egg (ObjectType6) is a curated, placeable object");
        check(seenTypeIds.count(39) == 0,
              "the sparkle trail (ObjectType39) is excluded -- an effect the game spawns, not a placement");
        check(seenTypeIds.count(8) == 0,
              "explosions (ObjectType8) are excluded -- transient effects, not placements");
    }

    // --- GEEditorPalette: Blocks/Objects mode toggle + independent selections ---
    {
        constexpr int kViewportW = 800;
        constexpr int kViewportH = 480;
        const auto click = [&](GEEditorPalette& palette, float x, float y)
        {
            const MouseState down(static_cast<int>(x), static_cast<int>(y), 0, ButtonState::Pressed,
                                  ButtonState::Released, ButtonState::Released, ButtonState::Released,
                                  ButtonState::Released);
            const MouseState up(static_cast<int>(x), static_cast<int>(y), 0, ButtonState::Released,
                                ButtonState::Released, ButtonState::Released, ButtonState::Released,
                                ButtonState::Released);
            (void)palette.Update(down, kViewportW, kViewportH);
            return palette.Update(up, kViewportW, kViewportH);
        };
        // Toolbar button 5 (mode toggle): y0 = 10 + 5*(48+8) = 290, so (10,290)-(58,338).
        constexpr float kModeButtonX = 30.0f;
        constexpr float kModeButtonY = 314.0f;
        constexpr float kGridX0 = 800.0f - (8 * (40.0f + 4.0f) - 4.0f) - 10.0f; // 442
        constexpr float kGridY0 = 480.0f - (4 * (40.0f + 4.0f) - 4.0f) - 10.0f; // 298

        GEEditorPalette palette;
        check(!palette.IsObjectMode(), "GEEditorPalette starts in Blocks mode");
        check(palette.SelectedObjectType() == GalaxyEggbert::ObjectType::ObjectType6,
              "GEEditorPalette starts with the extra-life egg selected as its default object type");

        const auto toggleResult = click(palette, kModeButtonX, kModeButtonY);
        check(toggleResult.clickConsumed, "clicking the mode-toggle button reports clickConsumed");
        check(toggleResult.action == GEEditorPalette::ToolbarAction::None,
              "the mode toggle is handled internally -- it reports no ToolbarAction for the caller");
        check(palette.IsObjectMode(), "clicking the 6th toolbar button switches to Objects mode");

        // Cell index 0 of the Confirmed tab in Objects mode = the first
        // curated object category's first id (Platform Lifts -> ObjectType1).
        const auto cellResult = click(palette, kGridX0 + 20.0f, kGridY0 + 20.0f);
        check(cellResult.clickConsumed, "clicking an object cell reports clickConsumed");
        check(palette.SelectedObjectType() == GalaxyEggbert::ObjectType::ObjectType1,
              "clicking the Objects tab's first cell selects the standard platform lift (ObjectType1)");
        check(palette.SelectedBlockType() == GalaxyEggbert::BlockTypes::RockPile,
              "selecting an object leaves the block selection untouched");

        (void)click(palette, kModeButtonX, kModeButtonY);
        check(!palette.IsObjectMode(), "clicking the mode-toggle button again switches back to Blocks mode");
        const auto blockCellResult = click(palette, kGridX0 + (40.0f + 4.0f) + 20.0f, kGridY0 + 20.0f);
        check(blockCellResult.clickConsumed, "clicking a block cell after switching back reports clickConsumed");
        check(palette.SelectedBlockType() == GalaxyEggbert::BlockTypes::BrickWall,
              "back in Blocks mode, the icon grid selects block types again");
        check(palette.SelectedObjectType() == GalaxyEggbert::ObjectType::ObjectType1,
              "selecting a block leaves the object selection untouched");
    }

    // --- GEEditCommandStack: MoveObjectEdit undo/redo (place, remove, overwrite) ---
    {
        using GalaxyEggbert::CollectMoveObjects;
        using GalaxyEggbert::MoveObjectRecord;
        using GalaxyEggbert::PlaceMoveObject;

        const auto makeRecord = [](GalaxyEggbert::ObjectType type, float x, float y, float z)
        {
            MoveObjectRecord record;
            record.type = type;
            record.posStartX = x;
            record.posStartY = y;
            record.posStartZ = z;
            record.posEndX = x;
            record.posEndY = y;
            record.posEndZ = z;
            return record;
        };

        // Place: no "before", a record "after".
        {
            World world;
            GEEditCommandStack stack;
            const MoveObjectRecord record = makeRecord(GalaxyEggbert::ObjectType::ObjectType6, 12.0f, 3.0f, 20.0f);
            PlaceMoveObject(world, record);

            GEEditCommand command;
            command.kind = GEEditCommand::Kind::MoveObjectEdit;
            command.objectAnchorX = 12;
            command.objectAnchorY = 3;
            command.objectAnchorZ = 20;
            command.objectAfter = record;
            stack.Push(std::move(command));
            check(CollectMoveObjects(world).size() == 1,
                  "test setup sanity: the placed MoveObject is in the world before undo");

            check(stack.Undo(world), "undoing a MoveObject placement reports success");
            check(CollectMoveObjects(world).empty(), "undoing a MoveObject placement removes it from the world");
            check(stack.Redo(world), "redoing a MoveObject placement reports success");
            const auto afterRedo = CollectMoveObjects(world);
            check(afterRedo.size() == 1 && afterRedo[0].type == GalaxyEggbert::ObjectType::ObjectType6,
                  "redoing a MoveObject placement restores the exact record");
        }

        // Overwrite: a record "before" AND a different record "after" --
        // undo must restore the original type, not just clear the cell.
        {
            World world;
            GEEditCommandStack stack;
            const MoveObjectRecord original = makeRecord(GalaxyEggbert::ObjectType::ObjectType6, 12.0f, 3.0f, 20.0f);
            PlaceMoveObject(world, original);
            const MoveObjectRecord replacement =
                makeRecord(GalaxyEggbert::ObjectType::ObjectType44, 12.0f, 3.0f, 20.0f);

            GEEditCommand command;
            command.kind = GEEditCommand::Kind::MoveObjectEdit;
            command.objectAnchorX = 12;
            command.objectAnchorY = 3;
            command.objectAnchorZ = 20;
            command.objectBefore = original;
            command.objectAfter = replacement;
            PlaceMoveObject(world, replacement);
            stack.Push(std::move(command));

            const auto afterPlace = CollectMoveObjects(world);
            check(afterPlace.size() == 1 && afterPlace[0].type == GalaxyEggbert::ObjectType::ObjectType44,
                  "test setup sanity: placing onto an occupied anchor replaces the record there");

            check(stack.Undo(world), "undoing a MoveObject overwrite reports success");
            const auto afterUndo = CollectMoveObjects(world);
            check(afterUndo.size() == 1 && afterUndo[0].type == GalaxyEggbert::ObjectType::ObjectType6,
                  "undoing an overwrite restores the object that was there before, not just empty space");

            check(stack.Redo(world), "redoing a MoveObject overwrite reports success");
            const auto afterRedo = CollectMoveObjects(world);
            check(afterRedo.size() == 1 && afterRedo[0].type == GalaxyEggbert::ObjectType::ObjectType44,
                  "redoing an overwrite re-applies the replacement record");
        }
    }

    // --- GEWorldEditor: left click in Objects mode places a real MoveObject ---
    {
        using GalaxyEggbert::CollectMoveObjects;

        // Same slab/camera setup as the block place/remove section above, so
        // the raycast target is already known-good ground truth.
        World world;
        for (int z = 0; z < 100; ++z)
        {
            for (int y = 0; y < 4; ++y)
            {
                world.setBlock(50, static_cast<std::uint16_t>(y), static_cast<std::uint16_t>(z), Block::make(1));
            }
        }
        constexpr float kDefaultYaw = 0.0f;
        constexpr float kDefaultPitch = -0.35f;
        const float cosPitch = std::cos(kDefaultPitch);
        const RaycastHit expected = Raycast(world, 50.0f, 10.0f, 50.0f,
                                             std::sin(kDefaultYaw) * cosPitch, std::sin(kDefaultPitch),
                                             -std::cos(kDefaultYaw) * cosPitch, 200.0f);
        check(expected.hit, "test setup sanity: the reference raycast for object placement hits the slab");
        const auto placeX = static_cast<std::uint16_t>(static_cast<int>(expected.x) + expected.normalX);
        const auto placeY = static_cast<std::uint16_t>(static_cast<int>(expected.y) + expected.normalY);
        const auto placeZ = static_cast<std::uint16_t>(static_cast<int>(expected.z) + expected.normalZ);

        GEWorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        Easy3D::Camera3D camera;

        // Switch the palette into Objects mode through the real UI path
        // (toolbar button 5), then select the first curated object cell
        // (Platform Lifts -> ObjectType1) -- both are palette clicks, so
        // each is consumed and places nothing in the world.
        const auto paletteClick = [&](float x, float y)
        {
            const MouseState down(static_cast<int>(x), static_cast<int>(y), 0, ButtonState::Pressed,
                                  ButtonState::Released, ButtonState::Released, ButtonState::Released,
                                  ButtonState::Released);
            const MouseState up(static_cast<int>(x), static_cast<int>(y), 0, ButtonState::Released,
                                ButtonState::Released, ButtonState::Released, ButtonState::Released,
                                ButtonState::Released);
            editor.Update(KeyboardState{}, down, 0.0f, 800, 480, camera, world);
            editor.Update(KeyboardState{}, up, 0.0f, 800, 480, camera, world);
        };
        paletteClick(30.0f, 314.0f); // mode toggle -> Objects
        constexpr float kGridX0 = 800.0f - (8 * (40.0f + 4.0f) - 4.0f) - 10.0f;
        constexpr float kGridY0 = 480.0f - (4 * (40.0f + 4.0f) - 4.0f) - 10.0f;
        paletteClick(kGridX0 + 20.0f, kGridY0 + 20.0f); // first object cell -> ObjectType1
        (void)editor.ConsumeNeedsPresentationRebuild();
        check(CollectMoveObjects(world).empty(),
              "palette clicks in Objects mode are consumed -- they don't place an object in the world");

        // A real left click out in the 3D view (away from any palette
        // geometry) now places the selected object.
        const MouseState leftMouse(400, 200, 0, ButtonState::Pressed, ButtonState::Released,
                                    ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, leftMouse, 0.0f, 800, 480, camera, world);

        const auto placed = CollectMoveObjects(world);
        check(placed.size() == 1, "a left click in Objects mode places exactly one MoveObject");
        check(!placed.empty() && placed[0].type == GalaxyEggbert::ObjectType::ObjectType1,
              "the placed MoveObject carries the palette's selected ObjectType");
        check(!placed.empty() && placed[0].posStartX == static_cast<float>(placeX) &&
                  placed[0].posStartY == static_cast<float>(placeY) &&
                  placed[0].posStartZ == static_cast<float>(placeZ),
              "the placed MoveObject sits at the cell adjacent to the aimed-at face");
        check(!placed.empty() && placed[0].posEndX == placed[0].posStartX &&
                  placed[0].posEndY == placed[0].posStartY &&
                  placed[0].posEndZ == placed[0].posStartZ,
              "a freshly placed MoveObject is stationary (posEnd == posStart)");
        check(world.getBlock(placeX, placeY, placeZ).isAir(),
              "placing an object does NOT also place a block in that cell");
        check(editor.ConsumeNeedsPresentationRebuild(),
              "placing an object requests a presentation rebuild");

        // U undoes the placement through the same key path block edits use.
        editor.Update(KeyboardState{Keys::U}, restMouse, 0.0f, 800, 480, camera, world);
        check(CollectMoveObjects(world).empty(), "U undoes an object placement");
        check(editor.ConsumeNeedsPresentationRebuild(), "undoing an object placement requests a rebuild");

        editor.Update(KeyboardState{Keys::R}, restMouse, 0.0f, 800, 480, camera, world);
        const auto redone = CollectMoveObjects(world);
        check(redone.size() == 1 && redone[0].type == GalaxyEggbert::ObjectType::ObjectType1,
              "R redoes an object placement, restoring the exact record");
        check(editor.ConsumeNeedsPresentationRebuild(), "redoing an object placement requests a rebuild");
    }

    // --- GEWorldEditor: G/T/Tab/OemPlus/OemMinus/Delete select-and-edit tool (plan.md EDITOR-110) ---
    {
        using GalaxyEggbert::CollectMoveObjects;

        // Same slab/camera setup as the object-placement section above, so
        // the raycast target (and so the placed object's cell) is already
        // known-good ground truth.
        World world;
        for (int z = 0; z < 100; ++z)
        {
            for (int y = 0; y < 4; ++y)
            {
                world.setBlock(50, static_cast<std::uint16_t>(y), static_cast<std::uint16_t>(z), Block::make(1));
            }
        }
        constexpr float kDefaultYaw = 0.0f;
        constexpr float kDefaultPitch = -0.35f;
        const float cosPitch = std::cos(kDefaultPitch);
        const RaycastHit expected = Raycast(world, 50.0f, 10.0f, 50.0f,
                                             std::sin(kDefaultYaw) * cosPitch, std::sin(kDefaultPitch),
                                             -std::cos(kDefaultYaw) * cosPitch, 200.0f);
        check(expected.hit, "test setup sanity: the reference raycast for object editing hits the slab");

        GEWorldEditor editor;
        editor.EnterEditing(0.0f, 10.0f, 0.0f);
        Easy3D::Camera3D camera;

        // Switch to Objects mode, select ObjectType1, and place a
        // stationary object at the aimed-at cell -- same real-UI-path
        // setup as the placement section above, so GEEditCommandStack has
        // a real place command underneath everything this section does.
        const auto paletteClick = [&](float x, float y)
        {
            const MouseState down(static_cast<int>(x), static_cast<int>(y), 0, ButtonState::Pressed,
                                  ButtonState::Released, ButtonState::Released, ButtonState::Released,
                                  ButtonState::Released);
            const MouseState up(static_cast<int>(x), static_cast<int>(y), 0, ButtonState::Released,
                                ButtonState::Released, ButtonState::Released, ButtonState::Released,
                                ButtonState::Released);
            editor.Update(KeyboardState{}, down, 0.0f, 800, 480, camera, world);
            editor.Update(KeyboardState{}, up, 0.0f, 800, 480, camera, world);
        };
        paletteClick(30.0f, 314.0f); // mode toggle -> Objects
        constexpr float kGridX0 = 800.0f - (8 * (40.0f + 4.0f) - 4.0f) - 10.0f;
        constexpr float kGridY0 = 480.0f - (4 * (40.0f + 4.0f) - 4.0f) - 10.0f;
        paletteClick(kGridX0 + 20.0f, kGridY0 + 20.0f); // first object cell -> ObjectType1
        (void)editor.ConsumeNeedsPresentationRebuild();

        const MouseState leftMouse(400, 200, 0, ButtonState::Pressed, ButtonState::Released,
                                    ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, leftMouse, 0.0f, 800, 480, camera, world);
        (void)editor.ConsumeNeedsPresentationRebuild();
        check(CollectMoveObjects(world).size() == 1, "test setup sanity: an object is placed before editing it");

        // Presses a single key for exactly one edge-triggered frame, then
        // releases it -- every G/T/Tab/OemPlus/OemMinus/U/R/Delete key in
        // GEWorldEditor::Update() is edge-triggered on the press, so
        // driving the SAME key multiple times in a row (as several checks
        // below do) needs a released frame in between to re-arm it, same
        // requirement as a real keyboard's up/down events.
        const auto pressKey = [&](Microsoft::Xna::Framework::Input::Keys key)
        {
            editor.Update(KeyboardState{key}, restMouse, 0.0f, 800, 480, camera, world);
            editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, world);
        };

        // G selects the placed object -- it sits right where the crosshair
        // just aimed, so its billboard projects near screen center. G
        // itself never mutates the world.
        pressKey(Keys::G);
        check(!editor.ConsumeNeedsPresentationRebuild(), "G alone doesn't request a presentation rebuild");

        // T gives the selected object a real patrol path: posEnd becomes
        // the current raycast aim cell (still the same slab surface).
        pressKey(Keys::T);
        check(editor.ConsumeNeedsPresentationRebuild(), "T requests a presentation rebuild");
        {
            const auto afterTarget = CollectMoveObjects(world);
            check(afterTarget.size() == 1, "T doesn't change the object count");
            check(!afterTarget.empty() &&
                      (afterTarget[0].posEndX != afterTarget[0].posStartX ||
                       afterTarget[0].posEndY != afterTarget[0].posStartY ||
                       afterTarget[0].posEndZ != afterTarget[0].posStartZ),
                  "T gives the selected object a real patrol path (posEnd != posStart) -- "
                  "this ALSO confirms G actually selected the object above (T is a no-op otherwise)");
            check(!afterTarget.empty() && afterTarget[0].posEndX == static_cast<float>(expected.x) &&
                      afterTarget[0].posEndY == static_cast<float>(expected.y) &&
                      afterTarget[0].posEndZ == static_cast<float>(expected.z),
                  "T's new posEnd is exactly the current raycast hit cell");
        }

        // OemPlus/OemMinus adjust the active field, which defaults to Speed.
        const float speedBefore = CollectMoveObjects(world)[0].speed;
        pressKey(Keys::OemPlus);
        check(editor.ConsumeNeedsPresentationRebuild(), "OemPlus (speed) requests a presentation rebuild");
        check(CollectMoveObjects(world)[0].speed > speedBefore, "OemPlus increases the active field (speed)");

        pressKey(Keys::OemMinus);
        check(editor.ConsumeNeedsPresentationRebuild(), "OemMinus (speed) requests a presentation rebuild");
        check(std::fabs(CollectMoveObjects(world)[0].speed - speedBefore) < 0.0001f,
              "OemMinus undoes OemPlus's own speed nudge back to the original value");

        // Tab cycles to the next field (StepAdvanceTicks) -- OemPlus now
        // touches that field instead of speed.
        pressKey(Keys::Tab);
        check(!editor.ConsumeNeedsPresentationRebuild(), "Tab alone doesn't request a presentation rebuild");
        const float stepAdvanceBefore = CollectMoveObjects(world)[0].stepAdvanceTicks;
        pressKey(Keys::OemPlus);
        check(editor.ConsumeNeedsPresentationRebuild(), "OemPlus (stepAdvanceTicks) requests a presentation rebuild");
        {
            const auto afterFieldEdit = CollectMoveObjects(world)[0];
            check(afterFieldEdit.stepAdvanceTicks > stepAdvanceBefore,
                  "after Tab, OemPlus adjusts stepAdvanceTicks instead of speed");
            check(afterFieldEdit.speed == speedBefore, "adjusting stepAdvanceTicks leaves speed untouched");
        }

        // U undoes all 4 edits above, one at a time, in exact reverse order:
        // the stepAdvanceTicks bump, the OemPlus/OemMinus speed round-trip
        // (2 distinct commands even though they net back to the same
        // value), then T's patrol target -- landing back on the original
        // stationary placement.
        pressKey(Keys::U);
        check(CollectMoveObjects(world)[0].stepAdvanceTicks == stepAdvanceBefore,
              "U undoes the stepAdvanceTicks edit");
        pressKey(Keys::U);
        check(CollectMoveObjects(world)[0].speed > speedBefore,
              "U undoes the OemMinus speed edit, landing back on the OemPlus-bumped speed");
        pressKey(Keys::U);
        check(std::fabs(CollectMoveObjects(world)[0].speed - speedBefore) < 0.0001f,
              "U undoes the OemPlus speed edit, landing back on the original speed");
        pressKey(Keys::U);
        {
            const auto afterAllUndo = CollectMoveObjects(world);
            check(afterAllUndo.size() == 1 &&
                      afterAllUndo[0].posEndX == afterAllUndo[0].posStartX &&
                      afterAllUndo[0].posEndY == afterAllUndo[0].posStartY &&
                      afterAllUndo[0].posEndZ == afterAllUndo[0].posStartZ,
                  "undoing all 4 edits restores the original stationary placement");
        }

        // R redoes all 4 edits back forward, exactly.
        pressKey(Keys::R);
        pressKey(Keys::R);
        pressKey(Keys::R);
        pressKey(Keys::R);
        {
            const auto afterAllRedo = CollectMoveObjects(world);
            check(afterAllRedo.size() == 1 &&
                      (afterAllRedo[0].posEndX != afterAllRedo[0].posStartX ||
                       afterAllRedo[0].posEndY != afterAllRedo[0].posStartY ||
                       afterAllRedo[0].posEndZ != afterAllRedo[0].posStartZ) &&
                      std::fabs(afterAllRedo[0].speed - speedBefore) < 0.0001f &&
                      afterAllRedo[0].stepAdvanceTicks > stepAdvanceBefore,
                  "redoing all 4 edits restores the exact post-edit state");
        }

        // Delete removes the selected object entirely.
        pressKey(Keys::Delete);
        check(editor.ConsumeNeedsPresentationRebuild(), "Delete requests a presentation rebuild");
        check(CollectMoveObjects(world).empty(), "Delete removes the selected object from the world");

        // U undoes the delete, restoring the object with its full edited
        // state intact (not just an empty placeholder).
        pressKey(Keys::U);
        {
            const auto afterUndoDelete = CollectMoveObjects(world);
            check(afterUndoDelete.size() == 1 &&
                      std::fabs(afterUndoDelete[0].speed - speedBefore) < 0.0001f &&
                      afterUndoDelete[0].stepAdvanceTicks > stepAdvanceBefore,
                  "U undoes the delete, restoring the object with its full edited state intact");
        }

        // A fresh editor/world pair with no MoveObjects placed anywhere --
        // G has nothing to select, and a Delete right after it is
        // confirmed to be a harmless no-op (no crash, no accidental
        // mutation of an unrelated cell).
        {
            World emptyObjWorld;
            GEWorldEditor freshEditor;
            freshEditor.EnterEditing(0.0f, 10.0f, 0.0f);
            Easy3D::Camera3D freshCamera;
            freshEditor.Update(KeyboardState{Keys::G}, restMouse, 0.0f, 800, 480, freshCamera, emptyObjWorld);
            freshEditor.Update(KeyboardState{Keys::Delete}, restMouse, 0.0f, 800, 480, freshCamera, emptyObjWorld);
            check(CollectMoveObjects(emptyObjWorld).empty(),
                  "G with no MoveObjects anywhere selects nothing; Delete afterward is a safe no-op");
        }
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
