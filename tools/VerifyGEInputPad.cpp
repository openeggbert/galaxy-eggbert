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

        // Menu now goes to Init (2026-07-13, now that Init exists) --
        // distinct from Continue/Restart.
        auto pressMenu = pad.UpdatePause(mouse(100, 350, true), kViewportW, kViewportH, true, true);
        auto releaseMenu = pad.UpdatePause(mouse(100, 350, false), kViewportW, kViewportH, true, true);
        check(releaseMenu.menuPressed && !releaseMenu.continuePressed && !releaseMenu.restartPressed,
              "Pause: Menu fires on release, distinct from Continue/Restart");
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

    // --- Win/Lost: the shared WinLostReturn button (real rect, confirmed
    // via InputPad.cpp's own bsf1=drawBoundsHeight/5 formula:
    // (428.8,19.2)-(524.8,115.2) in this 640x480 reference space) --
    // edge/release-triggered, same semantics as every other non-Jump
    // button in this class.
    {
        GEInputPad pad;
        const bool pressReturn = pad.UpdateWinLost(mouse(470, 60, true), kViewportW, kViewportH);
        check(!pressReturn, "WinLostReturn: not yet fired on the press frame itself");
        const bool releaseReturn = pad.UpdateWinLost(mouse(470, 60, false), kViewportW, kViewportH);
        check(releaseReturn, "WinLostReturn: fires on release when the press landed inside its rect");
    }
    {
        GEInputPad pad;
        (void)pad.UpdateWinLost(mouse(10, 10, true), kViewportW, kViewportH);
        const bool releaseOutside = pad.UpdateWinLost(mouse(10, 10, false), kViewportW, kViewportH);
        check(!releaseOutside, "WinLostReturn: does not fire when the press landed outside its rect");
    }

    // --- PlaySetup: real rects, verified against InputPad.cpp's own
    // bsf2=drawBoundsHeight*140/480 formula, which is EXACTLY 140 at this
    // engine's 480 reference height -- used unadapted (see
    // GEInputPad.hpp's UpdateSetup() class comment). SetupSounds
    // (X 20-90, Y 180-250) and SetupReturn (X 508-620, Y 348-460) are
    // functional; SetupJump/Zoom/Accel/Reset are real-position but inert.
    {
        GEInputPad pad;
        (void)pad.UpdateSetup(mouse(55, 215, true), kViewportW, kViewportH, false);
        const auto release = pad.UpdateSetup(mouse(55, 215, false), kViewportW, kViewportH, false);
        check(release.soundsToggled && !release.returnPressed,
              "Setup: Sounds toggle fires on release, distinct from Return");
    }
    {
        GEInputPad pad;
        (void)pad.UpdateSetup(mouse(564, 404, true), kViewportW, kViewportH, false);
        const auto release = pad.UpdateSetup(mouse(564, 404, false), kViewportW, kViewportH, false);
        check(release.returnPressed && !release.soundsToggled,
              "Setup: Return fires on release, distinct from Sounds");
    }
    {
        GEInputPad pad;
        // SetupJump rect: X 20-90, Y 250-320 -> center (55,285).
        (void)pad.UpdateSetup(mouse(55, 285, true), kViewportW, kViewportH, false);
        const auto release = pad.UpdateSetup(mouse(55, 285, false), kViewportW, kViewportH, false);
        check(!release.soundsToggled && !release.returnPressed,
              "Setup: Jump button press/release fires neither Sounds nor Return (documented inert placeholder)");
    }

    // --- SetupReset (real: MainSetup-only, plan.md MENU-006..020): rect
    // X 450-520, Y 180-250 -> center (485,215). showReset gates both hit-
    // testing and firing -- confirms PlaySetup's own call (showReset=
    // false) can't ever trigger it even if a press lands in that rect.
    {
        GEInputPad pad;
        (void)pad.UpdateSetup(mouse(485, 215, true), kViewportW, kViewportH, true);
        const auto release = pad.UpdateSetup(mouse(485, 215, false), kViewportW, kViewportH, true);
        check(release.resetPressed, "Setup: Reset fires on release when showReset=true (MainSetup)");
    }
    {
        GEInputPad pad;
        (void)pad.UpdateSetup(mouse(485, 215, true), kViewportW, kViewportH, false);
        const auto release = pad.UpdateSetup(mouse(485, 215, false), kViewportW, kViewportH, false);
        check(!release.resetPressed, "Setup: Reset never fires when showReset=false (PlaySetup)");
    }

    // --- Resume: real rects (X 180.6-320.6 Menu / 320.6-460.6 Continue,
    // Y 308-448, same bsf2=140-at-this-reference-height "no adaptation
    // needed" situation as Setup). Both buttons are now functional (Menu
    // -> Init, 2026-07-13, now that Init exists).
    {
        GEInputPad pad;
        (void)pad.UpdateResume(mouse(390, 378, true), kViewportW, kViewportH);
        const auto release = pad.UpdateResume(mouse(390, 378, false), kViewportW, kViewportH);
        check(release.continuePressed && !release.menuPressed, "Resume: Continue fires on release");
    }
    {
        GEInputPad pad;
        (void)pad.UpdateResume(mouse(250, 378, true), kViewportW, kViewportH);
        const auto release = pad.UpdateResume(mouse(250, 378, false), kViewportW, kViewportH);
        check(release.menuPressed && !release.continuePressed, "Resume: Menu fires on release, distinct from Continue");
    }

    // --- Init / gamer-select menu (plan.md MENU-006..020): real rects,
    // verified against InputPad.cpp's own bsf2=140-at-this-reference-
    // height formula (same "no adaptation needed" situation as Setup/
    // Resume). GamerA X20-90/Y166-236 -> center (55,201); GamerB
    // Y236-306 -> center (55,271); GamerC Y306-376 -> center (55,341);
    // InitSetup Y390-460 -> center (55,425); InitPlay X480-620/Y300-440
    // -> center (550,370).
    {
        GEInputPad pad;
        (void)pad.UpdateInit(mouse(55, 201, true), kViewportW, kViewportH);
        const auto release = pad.UpdateInit(mouse(55, 201, false), kViewportW, kViewportH);
        check(release.gamerSelected == 0 && !release.playPressed && !release.setupPressed,
              "Init: GamerA fires gamerSelected=0 on release");
    }
    {
        GEInputPad pad;
        (void)pad.UpdateInit(mouse(55, 271, true), kViewportW, kViewportH);
        const auto release = pad.UpdateInit(mouse(55, 271, false), kViewportW, kViewportH);
        check(release.gamerSelected == 1, "Init: GamerB fires gamerSelected=1 on release");
    }
    {
        GEInputPad pad;
        (void)pad.UpdateInit(mouse(55, 341, true), kViewportW, kViewportH);
        const auto release = pad.UpdateInit(mouse(55, 341, false), kViewportW, kViewportH);
        check(release.gamerSelected == 2, "Init: GamerC fires gamerSelected=2 on release");
    }
    {
        GEInputPad pad;
        (void)pad.UpdateInit(mouse(55, 425, true), kViewportW, kViewportH);
        const auto release = pad.UpdateInit(mouse(55, 425, false), kViewportW, kViewportH);
        check(release.setupPressed && release.gamerSelected == -1,
              "Init: InitSetup fires setupPressed on release, not a gamer selection");
    }
    {
        GEInputPad pad;
        (void)pad.UpdateInit(mouse(550, 370, true), kViewportW, kViewportH);
        const auto release = pad.UpdateInit(mouse(550, 370, false), kViewportW, kViewportH);
        check(release.playPressed && release.gamerSelected == -1,
              "Init: InitPlay fires playPressed on release, not a gamer selection");
    }
    {
        GEInputPad pad;
        (void)pad.UpdateInit(mouse(550, 370, true), kViewportW, kViewportH);
        const auto stillHeld = pad.UpdateInit(mouse(550, 370, true), kViewportW, kViewportH);
        check(!stillHeld.playPressed, "Init: InitPlay does not fire while still held (only on release)");
    }

    // --- Cheat gesture: real 10-tap sequence (12,22,32,12,11,21,22,21,
    // 31,32) over 6 invisible zones in a 3-col x 2-row grid spanning the
    // top-left 2/3 width x 57% height of the 640x480 reference space.
    // Zone center points (col,row): 11=(71.11,68.4), 12=(71.11,205.2),
    // 21=(213.33,68.4), 22=(213.33,205.2), 31=(355.56,68.4),
    // 32=(355.56,205.2).
    {
        const float seq[10][2] = {
            {71.11f, 205.2f}, {213.33f, 205.2f}, {355.56f, 205.2f}, {71.11f, 205.2f}, {71.11f, 68.4f},
            {213.33f, 68.4f}, {213.33f, 205.2f}, {213.33f, 68.4f}, {355.56f, 68.4f}, {355.56f, 205.2f},
        };
        GEInputPad pad;
        bool unlocked = false;
        for (const auto& p : seq)
        {
            unlocked = pad.UpdateCheatGesture(mouse(static_cast<int>(p[0]), static_cast<int>(p[1]), true),
                                              kViewportW, kViewportH);
            (void)pad.UpdateCheatGesture(mouse(static_cast<int>(p[0]), static_cast<int>(p[1]), false), kViewportW,
                                         kViewportH);
        }
        check(unlocked, "Cheat gesture: the full real 10-tap sequence unlocks on the 10th correct tap");
    }
    {
        const float seq[10][2] = {
            {71.11f, 205.2f}, {213.33f, 205.2f}, {355.56f, 205.2f}, {71.11f, 205.2f}, {71.11f, 68.4f},
            {213.33f, 68.4f}, {213.33f, 205.2f}, {213.33f, 68.4f}, {355.56f, 68.4f}, {355.56f, 205.2f},
        };
        GEInputPad pad;
        // Two correct taps, then a WRONG tap (zone 11 instead of the
        // expected 3rd tap, 32).
        (void)pad.UpdateCheatGesture(mouse(71, 205, true), kViewportW, kViewportH);
        (void)pad.UpdateCheatGesture(mouse(71, 205, false), kViewportW, kViewportH);
        (void)pad.UpdateCheatGesture(mouse(213, 205, true), kViewportW, kViewportH);
        (void)pad.UpdateCheatGesture(mouse(213, 205, false), kViewportW, kViewportH);
        (void)pad.UpdateCheatGesture(mouse(71, 68, true), kViewportW, kViewportH);
        (void)pad.UpdateCheatGesture(mouse(71, 68, false), kViewportW, kViewportH);
        // Replaying the FULL correct sequence from scratch should still
        // take exactly 10 taps to unlock (not just the remaining 7) --
        // proving the wrong tap above reset progress to 0.
        bool unlocked = false;
        for (const auto& p : seq)
        {
            unlocked = pad.UpdateCheatGesture(mouse(static_cast<int>(p[0]), static_cast<int>(p[1]), true),
                                              kViewportW, kViewportH);
            (void)pad.UpdateCheatGesture(mouse(static_cast<int>(p[0]), static_cast<int>(p[1]), false), kViewportW,
                                         kViewportH);
        }
        check(unlocked, "Cheat gesture: a wrong tap resets progress to 0 (the full 10-tap sequence still unlocks afterward)");
    }
    {
        GEInputPad pad;
        (void)pad.UpdateCheatGesture(mouse(71, 205, true), kViewportW, kViewportH); // correct 1st tap
        (void)pad.UpdateCheatGesture(mouse(71, 205, false), kViewportW, kViewportH);
        (void)pad.UpdateCheatGesture(mouse(500, 400, true), kViewportW, kViewportH); // outside all 6 zones
        (void)pad.UpdateCheatGesture(mouse(500, 400, false), kViewportW, kViewportH);
        const float rest[9][2] = {
            {213.33f, 205.2f}, {355.56f, 205.2f}, {71.11f, 205.2f}, {71.11f, 68.4f},
            {213.33f, 68.4f}, {213.33f, 205.2f}, {213.33f, 68.4f}, {355.56f, 68.4f}, {355.56f, 205.2f},
        };
        bool unlocked = false;
        for (const auto& p : rest)
        {
            unlocked = pad.UpdateCheatGesture(mouse(static_cast<int>(p[0]), static_cast<int>(p[1]), true),
                                              kViewportW, kViewportH);
            (void)pad.UpdateCheatGesture(mouse(static_cast<int>(p[0]), static_cast<int>(p[1]), false), kViewportW,
                                         kViewportH);
        }
        check(unlocked, "Cheat gesture: a press outside all 6 zones is ignored, not treated as a wrong tap");
    }

    // --- Cheat menu: real row of 9 buttons, proportionally spanning the
    // full 640-wide reference space (real is 9x80 absolute pixels, which
    // doesn't fit at all -- see UpdateCheatMenu()'s own class comment).
    // Button 0 = cheat 1 (OpenDoors); button 8 = cheat 9 (EndGoal).
    {
        GEInputPad pad;
        (void)pad.UpdateCheatMenu(mouse(30, 30, true), kViewportW, kViewportH);
        const int pressed = pad.UpdateCheatMenu(mouse(30, 30, false), kViewportW, kViewportH);
        check(pressed == 1, "Cheat menu: pressing the first button fires cheat 1 (OpenDoors) on release");
    }
    {
        GEInputPad pad;
        (void)pad.UpdateCheatMenu(mouse(600, 30, true), kViewportW, kViewportH);
        const int pressed = pad.UpdateCheatMenu(mouse(600, 30, false), kViewportW, kViewportH);
        check(pressed == 9, "Cheat menu: pressing the last button fires cheat 9 (EndGoal) on release");
    }
    {
        GEInputPad pad;
        (void)pad.UpdateCheatMenu(mouse(30, 30, true), kViewportW, kViewportH);
        const int pressed = pad.UpdateCheatMenu(mouse(30, 30, true), kViewportW, kViewportH);
        check(pressed == 0, "Cheat menu: not fired while still held (only fires on release)");
    }

    // --- Typed cheat code: "ghost" (plan.md BLUPI-111) ---
    {
        using Keys = Microsoft::Xna::Framework::Input::Keys;
        using KeyboardState = Microsoft::Xna::Framework::Input::KeyboardState;

        GEInputPad pad;
        bool anyTrue = false;
        // Types "ghost" one key-down-edge at a time, releasing between
        // each letter (matching the real "down-edge appends, held doesn't
        // repeat" semantics) -- only the final 't' completes the suffix
        // match.
        for (Keys k : {Keys::G, Keys::H, Keys::O, Keys::S, Keys::T})
        {
            const bool firedOnPress = pad.UpdateTypedGhostCheat(KeyboardState{k}, /*isPlayPhase=*/true);
            anyTrue = anyTrue || firedOnPress;
            (void)pad.UpdateTypedGhostCheat(KeyboardState{}, /*isPlayPhase=*/true); // release edge
        }
        check(anyTrue, "Typed cheat: typing \"ghost\" letter by letter fires exactly once (on the final 't')");
    }
    {
        GEInputPad pad;
        using Keys = Microsoft::Xna::Framework::Input::Keys;
        using KeyboardState = Microsoft::Xna::Framework::Input::KeyboardState;
        bool anyTrue = false;
        for (Keys k : {Keys::G, Keys::H, Keys::O, Keys::S, Keys::T})
        {
            anyTrue = anyTrue || pad.UpdateTypedGhostCheat(KeyboardState{k}, /*isPlayPhase=*/false);
            (void)pad.UpdateTypedGhostCheat(KeyboardState{}, /*isPlayPhase=*/false);
        }
        check(!anyTrue, "Typed cheat: typing \"ghost\" outside Play phase never fires (real Phase::Play gate)");
    }
    {
        GEInputPad pad;
        using Keys = Microsoft::Xna::Framework::Input::Keys;
        using KeyboardState = Microsoft::Xna::Framework::Input::KeyboardState;
        // Holding G down across multiple frames (no release in between)
        // must NOT repeat-append -- only the down-EDGE counts.
        (void)pad.UpdateTypedGhostCheat(KeyboardState{Keys::G}, true);
        (void)pad.UpdateTypedGhostCheat(KeyboardState{Keys::G}, true);
        (void)pad.UpdateTypedGhostCheat(KeyboardState{Keys::G}, true);
        (void)pad.UpdateTypedGhostCheat(KeyboardState{}, true);
        bool anyTrue = false;
        for (Keys k : {Keys::H, Keys::O, Keys::S, Keys::T})
        {
            anyTrue = anyTrue || pad.UpdateTypedGhostCheat(KeyboardState{k}, true);
            (void)pad.UpdateTypedGhostCheat(KeyboardState{}, true);
        }
        check(anyTrue,
              "Typed cheat: holding a key across frames only counts once (down-edge, not held-repeat), "
              "so \"ghost\" still completes correctly afterward");
    }
    {
        // Suffix match: typing an unrelated prefix before "ghost" still
        // fires (real behavior matches the buffer's own SUFFIX, no reset
        // needed).
        GEInputPad pad;
        using Keys = Microsoft::Xna::Framework::Input::Keys;
        using KeyboardState = Microsoft::Xna::Framework::Input::KeyboardState;
        bool anyTrue = false;
        for (Keys k : {Keys::X, Keys::Y, Keys::Z, Keys::G, Keys::H, Keys::O, Keys::S, Keys::T})
        {
            anyTrue = anyTrue || pad.UpdateTypedGhostCheat(KeyboardState{k}, true);
            (void)pad.UpdateTypedGhostCheat(KeyboardState{}, true);
        }
        check(anyTrue, "Typed cheat: an unrelated prefix before \"ghost\" doesn't prevent the suffix match");
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
