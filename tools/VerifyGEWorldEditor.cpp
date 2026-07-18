#include "Editor/GEVoxelRaycast.hpp"
#include "Editor/GEWorldEditor.hpp"

#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <cmath>
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
// Later milestones (box-fill, undo/redo, palette data, MoveObject/
// sky-region round-trips) add their own sections here.
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
    const World emptyWorld;

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

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
