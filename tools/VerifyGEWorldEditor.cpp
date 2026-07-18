#include "Editor/GEWorldEditor.hpp"

#include <cmath>
#include <iostream>

// Scripted verification for the in-game 3D world editor (plan.md section 6,
// EDITOR-1xx tasks). This milestone (EDITOR-101) covers the free-fly
// camera's pure movement math (WASD + scroll-wheel fly-speed, driven by
// synthetic KeyboardState/MouseState values -- no GraphicsDevice/window
// needed at runtime, same "no graphics context needed" shape as
// VerifyGEInputPad). Mouse-look (holding the right button) is deliberately
// NOT exercised here: it calls real Mouse::SetCaptureEXT()/
// setIsRelativeMouseModeEXTProperty(), which need an actual SDL window to
// mean anything -- camera feel from mouse-look is a live/screenshot check,
// same as every other "visual judgment" item in this project. Later
// milestones (raycast, box-fill, undo/redo, palette data, MoveObject/
// sky-region round-trips) add their own sections here.
int main()
{
    using namespace GalaxyEggbert::CNA;
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

    // MouseState with both buttons at rest -- every check below uses this so
    // Update() never takes the mouse-look/SDL-capture branches (see the
    // top-of-file comment for why those are excluded from this tool).
    const MouseState restMouse(0, 0, 0, ButtonState::Released, ButtonState::Released,
                               ButtonState::Released, ButtonState::Released, ButtonState::Released);

    // --- EnterEditing() sets the starting position and a level-ish default look ---
    {
        GEWorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        Easy3D::Camera3D camera;
        editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera);
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
        editor.Update(KeyboardState{Keys::W}, restMouse, 1.0f, 800, 480, camera);
        check(std::fabs(camera.GetPosition().X - 50.0f) < 0.001f,
              "holding W at yaw 0 doesn't drift sideways in X");
        check(camera.GetPosition().Z < 50.0f - 10.0f,
              "holding W for 1s at the default fly speed moves well forward (-Z)");
    }
    {
        GEWorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        Easy3D::Camera3D camera;
        editor.Update(KeyboardState{Keys::S}, restMouse, 1.0f, 800, 480, camera);
        check(camera.GetPosition().Z > 50.0f + 10.0f,
              "holding S at yaw 0 moves well backward (+Z), opposite of W");
    }
    {
        GEWorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        Easy3D::Camera3D camera;
        editor.Update(KeyboardState{Keys::D}, restMouse, 1.0f, 800, 480, camera);
        check(camera.GetPosition().X > 50.0f + 10.0f,
              "holding D at yaw 0 strafes well right (+X)");
        check(std::fabs(camera.GetPosition().Z - 50.0f) < 0.001f,
              "holding D at yaw 0 doesn't drift in Z");
    }
    {
        GEWorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        Easy3D::Camera3D camera;
        editor.Update(KeyboardState{Keys::A}, restMouse, 1.0f, 800, 480, camera);
        check(camera.GetPosition().X < 50.0f - 10.0f,
              "holding A at yaw 0 strafes well left (-X), opposite of D");
    }

    // --- Space/LeftControl move purely along world Y ---
    {
        GEWorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        Easy3D::Camera3D camera;
        editor.Update(KeyboardState{Keys::Space}, restMouse, 1.0f, 800, 480, camera);
        check(camera.GetPosition().Y > 15.0f + 10.0f, "holding Space rises well above the start height");
        check(std::fabs(camera.GetPosition().X - 50.0f) < 0.001f &&
                  std::fabs(camera.GetPosition().Z - 50.0f) < 0.001f,
              "holding Space alone doesn't drift horizontally");
    }
    {
        GEWorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        Easy3D::Camera3D camera;
        editor.Update(KeyboardState{Keys::LeftControl}, restMouse, 1.0f, 800, 480, camera);
        check(camera.GetPosition().Y < 15.0f - 10.0f,
              "holding Left Ctrl descends well below the start height, opposite of Space");
    }

    // --- No keys held is a no-op (position unchanged across a real dt) ---
    {
        GEWorldEditor editor;
        editor.EnterEditing(50.0f, 15.0f, 50.0f);
        Easy3D::Camera3D camera;
        editor.Update(KeyboardState{}, restMouse, 1.0f, 800, 480, camera);
        check(camera.GetPosition().X == 50.0f && camera.GetPosition().Y == 15.0f &&
                  camera.GetPosition().Z == 50.0f,
              "no keys held leaves the camera position unchanged");
    }

    // --- dt scales distance linearly (half the time, roughly half the distance) ---
    {
        GEWorldEditor editorFull;
        editorFull.EnterEditing(0.0f, 0.0f, 0.0f);
        Easy3D::Camera3D cameraFull;
        editorFull.Update(KeyboardState{Keys::W}, restMouse, 1.0f, 800, 480, cameraFull);
        const float fullDistanceZ = cameraFull.GetPosition().Z;

        GEWorldEditor editorHalf;
        editorHalf.EnterEditing(0.0f, 0.0f, 0.0f);
        Easy3D::Camera3D cameraHalf;
        editorHalf.Update(KeyboardState{Keys::W}, restMouse, 0.5f, 800, 480, cameraHalf);
        const float halfDistanceZ = cameraHalf.GetPosition().Z;

        check(std::fabs(halfDistanceZ - fullDistanceZ * 0.5f) < 0.001f,
              "movement distance scales linearly with dt");
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
