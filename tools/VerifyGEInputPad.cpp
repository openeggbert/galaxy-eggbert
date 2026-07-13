#include "Game/GEInputPad.hpp"

#include <Microsoft/Xna/Framework/Input/MouseState.hpp>

#include <iostream>

// Scripted, non-interactive verification of GEInputPad (2026-07-13,
// plan.md MENU-021..027/028..039) -- proves the on-screen D-pad/Jump/
// Action/Pause hit-testing and Pause-row button hit-testing actually work
// (press/drag/release semantics), not just "compiles and doesn't crash".
// UpdatePlay()/UpdatePause() are deliberately not gated on LoadContent()
// (see GEInputPad.cpp), so this runs with no GraphicsDevice at all --
// synthetic Microsoft::Xna::Framework::Input::MouseState values drive
// every check, at a 640x480 viewport (scale=1, offsetX=0) so reference-
// space coordinates match screen coordinates directly.
int main()
{
    using namespace GalaxyEggbert::CNA;
    using Microsoft::Xna::Framework::Input::ButtonState;
    using Microsoft::Xna::Framework::Input::MouseState;

    constexpr int kViewportW = 640;
    constexpr int kViewportH = 480;

    bool allOk = true;
    const auto check = [&allOk](bool cond, const char* what)
    {
        std::cout << (cond ? "PASS" : "FAIL") << ": " << what << std::endl;
        if (!cond) allOk = false;
    };

    const auto mouse = [](int x, int y, bool down)
    {
        const ButtonState left = down ? ButtonState::Pressed : ButtonState::Released;
        return MouseState(x, y, 0, left, ButtonState::Released, ButtonState::Released,
                           ButtonState::Released, ButtonState::Released);
    };

    // --- Play: D-pad discrete drag ---
    {
        GEInputPad pad;
        GEInputPad::PlayInput in;

        // Press inside the D-pad hit-square (center (80,400)) with no
        // offset yet -> both axes still 0.
        (void)pad.UpdatePlay(mouse(80, 400, true), kViewportW, kViewportH, in);
        check(in.turnInput == 0.0f && in.moveInput == 0.0f, "D-pad: centered press reads (0,0)");

        // Drag right past the +20 threshold -> turnInput=+1, moveInput
        // still 0 (dy still 0).
        (void)pad.UpdatePlay(mouse(150, 400, true), kViewportW, kViewportH, in);
        check(in.turnInput == 1.0f && in.moveInput == 0.0f, "D-pad: drag right past threshold reads turnInput=+1");

        // Drag up (screen Y decreases) past threshold -> moveInput=+1
        // ("forward"), matching a physical D-pad's up=forward mapping.
        (void)pad.UpdatePlay(mouse(80, 350, true), kViewportW, kViewportH, in);
        check(in.moveInput == 1.0f && in.turnInput == 0.0f, "D-pad: drag up past threshold reads moveInput=+1 (forward)");

        // Drag down past threshold -> moveInput=-1.
        (void)pad.UpdatePlay(mouse(80, 450, true), kViewportW, kViewportH, in);
        check(in.moveInput == -1.0f, "D-pad: drag down past threshold reads moveInput=-1");

        // Drag left past threshold -> turnInput=-1.
        (void)pad.UpdatePlay(mouse(50, 400, true), kViewportW, kViewportH, in);
        check(in.turnInput == -1.0f, "D-pad: drag left past threshold reads turnInput=-1");

        // Release -> both axes snap back to 0 (no drag in progress).
        (void)pad.UpdatePlay(mouse(50, 400, false), kViewportW, kViewportH, in);
        check(in.turnInput == 0.0f && in.moveInput == 0.0f, "D-pad: releasing clears both axes");
    }

    // --- Play: Jump is level-triggered (no release needed, current
    // position each frame) ---
    {
        GEInputPad pad;
        GEInputPad::PlayInput in;

        // Real PlayJump rect: (550,390)-(620,460).
        (void)pad.UpdatePlay(mouse(300, 300, false), kViewportW, kViewportH, in);
        check(!in.jumpHeld, "Jump: not held when mouse is up");

        (void)pad.UpdatePlay(mouse(580, 420, true), kViewportW, kViewportH, in);
        check(in.jumpHeld, "Jump: held true the instant press lands inside its rect");

        // Still held next frame, same position -> still true (level-
        // triggered, no release needed).
        (void)pad.UpdatePlay(mouse(580, 420, true), kViewportW, kViewportH, in);
        check(in.jumpHeld, "Jump: stays held across frames while still down and still inside rect");

        // Drag out of the rect while still down -> jumpHeld goes false
        // immediately (level trigger reads CURRENT position only).
        (void)pad.UpdatePlay(mouse(300, 300, true), kViewportW, kViewportH, in);
        check(!in.jumpHeld, "Jump: releases (input-wise) the instant the pointer leaves its rect, even mid-press");
    }

    // --- Play: Action/Pause are edge/release-triggered (single-fire on
    // release, regardless of where the release lands) ---
    {
        GEInputPad pad;
        GEInputPad::PlayInput in;

        // Real PlayAction rect: (550,310)-(620,380).
        (void)pad.UpdatePlay(mouse(580, 340, true), kViewportW, kViewportH, in);
        check(!in.actionPressed, "Action: not yet fired on the press frame itself");

        (void)pad.UpdatePlay(mouse(580, 340, true), kViewportW, kViewportH, in);
        check(!in.actionPressed, "Action: not fired while still held");

        // Release, dragged off the button first -- real semantics fire
        // based on where the press STARTED, not the release point.
        (void)pad.UpdatePlay(mouse(10, 10, false), kViewportW, kViewportH, in);
        check(in.actionPressed, "Action: fires on release even though the release point is off the button");

        (void)pad.UpdatePlay(mouse(10, 10, false), kViewportW, kViewportH, in);
        check(!in.actionPressed, "Action: single-fire -- does not refire on a later no-op frame");
    }
    {
        GEInputPad pad;
        GEInputPad::PlayInput in;

        // Real PlayPause rect: (580,10)-(630,60).
        (void)pad.UpdatePlay(mouse(600, 30, true), kViewportW, kViewportH, in);
        (void)pad.UpdatePlay(mouse(600, 30, false), kViewportW, kViewportH, in);
        check(in.pausePressed, "Pause (Play-phase button): fires on release");
    }

    // --- Play: UpdatePlay()'s own return value (mouse-claimed-by-a-
    // control signal, used to suppress the camera drag-look) ---
    {
        GEInputPad pad;
        GEInputPad::PlayInput in;

        const bool claimedEmptySpace = pad.UpdatePlay(mouse(300, 200, true), kViewportW, kViewportH, in);
        check(!claimedEmptySpace, "UpdatePlay: pressing empty space does not claim the mouse");

        // Release before the next press -- otherwise this would read as
        // the SAME continuous drag that started on empty space (correctly
        // still unclaimed), not a fresh press on the D-pad.
        (void)pad.UpdatePlay(mouse(300, 200, false), kViewportW, kViewportH, in);

        const bool claimedDPad = pad.UpdatePlay(mouse(80, 400, true), kViewportW, kViewportH, in);
        check(claimedDPad, "UpdatePlay: pressing the D-pad claims the mouse (for camera-look suppression)");
    }

    // --- Pause: unconditional buttons (Menu/Setup/Continue) vs
    // conditional (Back/Restart) ---
    {
        GEInputPad pad;

        // Real Pause row (index 0..4): Menu[55,145], Back[165,255],
        // Setup[275,365], Restart[385,475], Continue[495,585], Y[310,400].
        // showBack=false, showRestart=false (mission==1 case).
        auto in = pad.UpdatePause(mouse(200, 350, true), kViewportW, kViewportH, false, false);
        in = pad.UpdatePause(mouse(200, 350, false), kViewportW, kViewportH, false, false);
        check(!in.continuePressed && !in.restartPressed,
              "Pause: clicking a hidden Back button (mission==1) does not fire anything");

        auto in2 = pad.UpdatePause(mouse(420, 350, true), kViewportW, kViewportH, false, false);
        in2 = pad.UpdatePause(mouse(420, 350, false), kViewportW, kViewportH, false, false);
        check(!in2.continuePressed && !in2.restartPressed,
              "Pause: clicking a hidden Restart button (mission==1) does not fire anything");
    }
    {
        GEInputPad pad;
        // showBack=true, showRestart=true (a normal, non-mission-1, non-
        // decade-boundary mission).
        auto press = pad.UpdatePause(mouse(540, 350, true), kViewportW, kViewportH, true, true);
        auto release = pad.UpdatePause(mouse(540, 350, false), kViewportW, kViewportH, true, true);
        check(!press.continuePressed, "Pause: Continue does not fire on the press frame");
        check(release.continuePressed, "Pause: Continue fires on release");

        auto press2 = pad.UpdatePause(mouse(420, 350, true), kViewportW, kViewportH, true, true);
        auto release2 = pad.UpdatePause(mouse(420, 350, false), kViewportW, kViewportH, true, true);
        check(release2.restartPressed && !release2.continuePressed,
              "Pause: Restart fires on release, distinct from Continue");

        // Menu/Setup are real positions/icons but intentionally inert.
        auto pressMenu = pad.UpdatePause(mouse(100, 350, true), kViewportW, kViewportH, true, true);
        auto releaseMenu = pad.UpdatePause(mouse(100, 350, false), kViewportW, kViewportH, true, true);
        check(!releaseMenu.continuePressed && !releaseMenu.restartPressed,
              "Pause: Menu button press/release fires neither Continue nor Restart (documented inert placeholder)");
    }

    // --- ResetTouchState() clears an in-flight press so a keyboard-driven
    // phase change mid-drag can't leak into the next phase's own reading.
    {
        GEInputPad pad;
        auto press = pad.UpdatePause(mouse(540, 350, true), kViewportW, kViewportH, true, true);
        pad.ResetTouchState();
        auto release = pad.UpdatePause(mouse(540, 350, false), kViewportW, kViewportH, true, true);
        check(!release.continuePressed, "ResetTouchState: clears a latched press before its release fires");
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
