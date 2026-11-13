#include <GalaxyEggbert/Game/GamepadInput.hpp>
#include <GalaxyEggbert/Game/ControlInput.hpp>
#include <GalaxyEggbert/Game/CheatConfig.hpp>
#include <GalaxyEggbert/Game/InputPad.hpp>
#include <GalaxyEggbert/Game/DebugOverlay.hpp>
#include <GalaxyEggbert/Game/FocusNavigator.hpp>
#include <GalaxyEggbert/Game/ScreenKeyboard.hpp>

#include <Microsoft/Xna/Framework/Input/MouseState.hpp>

#include <cmath>
#include <iostream>
#include <string>
#include <utility>
#include <unordered_set>

// Scripted, non-interactive verification of InputPad (2026-07-13,
// plan.md MENU-021..027/028..039) -- proves the on-screen D-pad/Jump/
// Action/Pause hit-testing and Pause-row button hit-testing actually work
// (press/drag/release semantics), not just "compiles and doesn't crash".
// UpdatePlay()/UpdatePause() are deliberately not gated on LoadContent()
// (see InputPad.cpp), so this runs with no GraphicsDevice at all --
// synthetic Microsoft::Xna::Framework::Input::MouseState values drive
// every check, at a 640x480 viewport (scale=1, offsetX=0) so reference-
// space coordinates match screen coordinates directly.
int main()
{
    using namespace GalaxyEggbert::Game;
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

    {
        using Keys = Microsoft::Xna::Framework::Input::Keys;
        using KeyboardState = Microsoft::Xna::Framework::Input::KeyboardState;
        CheatKeyboard k;
        const auto keys = [](std::initializer_list<Keys> down) { return KeyboardState(std::unordered_set<Keys>(down)); };
        const auto tick = [&](const KeyboardState& down = {}, std::vector<CheatKeyboard::Pointer> points = {}, bool allowed = true) {
            return k.Update(allowed,800,360,down,points);
        };
        for (int i = 0; i < 150; ++i) tick();
        check(!k.Visible(), "cheat keyboard: resting cursor without a pressed button never opens");
        for (int i = 0; i < CheatKeyboard::kHoldTicks - 1; ++i) tick({},{{-2,400,180}});
        check(!k.Visible(), "cheat keyboard: pointer hold kHoldTicks-1 ticks stays closed");
        auto out = tick({},{{-2,400,180}});
        check(k.Visible() && out.capture && out.key == Keys::None, "cheat keyboard: opens at kHoldTicks without typing opening press");
        const auto index = [&](Keys code) {
            const auto& list = k.KeysOnScreen();
            for (std::size_t i = 0; i < list.size(); ++i) if (list[i].code == code) return int(i);
            return -1;
        };
        const auto pointer = [&](Keys code, int id = -2) {
            const auto r = k.KeysOnScreen()[index(code)].rect;
            return CheatKeyboard::Pointer{id,(r.x0+r.x1)/2,(r.y0+r.y1)/2};
        };
        tick();
        const auto z = pointer(Keys::Z);
        out = tick({}, {z});
        check(out.capture && out.key == Keys::Z, "cheat keyboard: a fresh tap fires exactly its letter");
        bool once = true;
        for (int i = 0; i < 160; ++i) once &= tick({}, {z}).key == Keys::None;
        check(once, "cheat keyboard: held key never repeats");
        tick();
        check(tick({}, {pointer(Keys::None)}).capture && !k.Visible(), "cheat keyboard: close key closes and consumes click");
        k.Reset();
        for (int i = 0; i < CheatKeyboard::kHoldTicks; ++i) tick({},{{-2,400,180,true}});
        check(!k.Visible(), "cheat keyboard: play-control-claimed press never opens");
        tick();
        for (int i = 0; i < CheatKeyboard::kHoldTicks - 1; ++i) tick({},{{4,400,180}});
        tick({},{{4,450,180}});
        for (int i = 0; i < CheatKeyboard::kHoldTicks; ++i) tick({},{{4,400,180}});
        check(!k.Visible(), "cheat keyboard: leaving circle cancels hold until finger lifts");
        tick();
        for (int i = 0; i < CheatKeyboard::kHoldTicks; ++i) tick({},{{5,400,180},{6,20,350,true}});
        check(k.Visible(), "cheat keyboard: independent centre finger opens while another uses a control");
        k.Close();
        for (int i = 0; i < 150; ++i) tick({},{{5,400,180}});
        check(!k.Visible(), "cheat keyboard: same hold cannot reopen after closing");
        k.Reset();
        for (int i = 0; i < CheatKeyboard::kHoldTicks - 1; ++i) tick(keys({CheatKeyboard::kHoldKey}));
        check(!k.Visible(), "cheat keyboard: LT key kHoldTicks-1 ticks stays closed");
        tick(keys({CheatKeyboard::kHoldKey}));
        check(k.Visible(), "cheat keyboard: LT key opens at kHoldTicks");
        tick();
        tick(keys({Keys::Down})); // first arrow selects F5
        tick(); tick(keys({Keys::Down})); // Q
        tick(); tick(keys({Keys::Down})); // A
        tick(); tick(keys({Keys::Down})); // Z
        check(k.Focused() == index(Keys::Z), "cheat keyboard: d-pad navigates QWERTY highlight");
        tick();
        out = tick(keys({Keys::Enter,Keys::LeftControl}));
        check(out.key == Keys::Z && tick(keys({Keys::Enter,Keys::LeftControl})).key == Keys::None,
              "cheat keyboard: A/Enter fires highlighted key once per press");
        const auto filtered = k.FilterGameplay(keys({Keys::Up,Keys::Left,Keys::LeftControl,Keys::Enter,Keys::Space,Keys::F}));
        check(!filtered.IsKeyDown(Keys::Up) && !filtered.IsKeyDown(Keys::Left) &&
              !filtered.IsKeyDown(Keys::LeftControl) && !filtered.IsKeyDown(Keys::Enter) &&
              !filtered.IsKeyDown(Keys::Space) && filtered.IsKeyDown(Keys::F),
              "cheat keyboard: A and stick give Blupi no input; other physical controls still work");
        tick(); tick(keys({Keys::Space}));
        check(!k.Visible(), "cheat keyboard: B closes");
        tick();
        for (int i = 0; i < CheatKeyboard::kHoldTicks; ++i) tick(keys({CheatKeyboard::kHoldKey}));
        check(k.Visible(), "cheat keyboard: LT opens again after release");
        tick();
        for (int i = 0; i < CheatKeyboard::kHoldTicks; ++i) tick(keys({CheatKeyboard::kHoldKey}));
        check(!k.Visible(), "cheat keyboard: second LT hold closes");
        for (int i = 0; i < CheatKeyboard::kHoldTicks; ++i) tick(keys({CheatKeyboard::kHoldKey}));
        check(!k.Visible(), "cheat keyboard: consumed LT hold cannot reopen");
        tick();
        for (int i = 0; i < 150; ++i) tick(keys({CheatKeyboard::kHoldKey}),{},false);
        tick(keys({CheatKeyboard::kHoldKey}));
        check(!k.Visible(), "cheat keyboard: overlay/phase gate consumes held activation");
        GamepadFrame f; f.connected = true; f.leftTrigger = 1;
        check((GamepadKeys(f) & kPadCheatKeyboard) != 0 && CheatKeyboard::kHoldKey == Keys::OemPipe,
              "gamepad: LT maps to the recorded unused OemPipe key");
        InputPad pad;
        for (int i = 0; i < CheatKeyboard::kHoldTicks; ++i) pad.CheatKeys().Update(true,800,360,keys({CheatKeyboard::kHoldKey}),{});
        InputPad::PlayInput mouseInput, touchInput;
        const bool mouseTaken = pad.UpdatePlay(mouse(20,430,true),800,480,mouseInput);
        const bool touchTaken = pad.UpdatePlayTouches({{1,20,430,true,false},{2,750,430,true,false}},800,480,touchInput);
        check(mouseTaken && touchTaken && mouseInput.moveInput == 0 && mouseInput.turnInput == 0 &&
              !mouseInput.jumpHeld && !mouseInput.actionPressed && !mouseInput.pausePressed &&
              touchInput.moveInput == 0 && touchInput.turnInput == 0 && !touchInput.jumpHeld &&
              !touchInput.actionPressed && !touchInput.pausePressed,
              "cheat keyboard: InputPad consumes mouse and multitouch without play actions while open");
        pad.ResetMissionInput();
        check(!pad.CheatKeys().Visible() && pad.CheatKeys().StateSignature() == CheatKeyboard{}.StateSignature(),
              "cheat keyboard: mission reset clears every replay-relevant field");
        k.Reset(); k.Layout(800,360);
        check(k.KeysOnScreen().size() == 36 && k.KeysOnScreen()[0].rect.y1-k.KeysOnScreen()[0].rect.y0 >= 43,
              "cheat keyboard: all letters, F1, F3, F4-F8, Tab, F12 and close; phone keys at least 43 pixels tall");
        check(k.OrderedLetter('a',Keys::Z,{}) == 'a' && k.OrderedLetter(0,Keys::None,{}) == 'z',
              "cheat keyboard: simultaneous physical and screen letters retain order");
        k.Reset();
        InputPad typed;
        InputPad::TypedCheat result = InputPad::TypedCheat::None;
        for (auto key : {Keys::Z,Keys::O,Keys::O,Keys::M})
        {
            const int letter = k.OrderedLetter(0,key,{});
            result = typed.UpdateTypedGhostCheat({},true,letter).cheat;
        }
        check(result == InputPad::TypedCheat::Zoom, "cheat keyboard: ordered repeated letters reach the existing typed-cheat matcher");
    }


    {
        using Microsoft::Xna::Framework::Input::KeyboardState;
        using Microsoft::Xna::Framework::Input::Keys;
        const auto keys = [](std::initializer_list<Keys> held) {
            return KeyboardState(std::unordered_set<Keys>(held));
        };
        const auto onFoot=VehicleFireKeyboard(keys({Keys::LeftControl}),false);
        const auto armed=VehicleFireKeyboard(keys({Keys::LeftControl,Keys::Up}),true);
        check(onFoot.IsKeyDown(Keys::LeftControl) && !onFoot.IsKeyDown(Keys::F) &&
              armed.IsKeyDown(Keys::F) && !armed.IsKeyDown(Keys::LeftControl) && armed.IsKeyDown(Keys::Up),
              "physical Left Ctrl jumps on foot or climbs a helicopter; only the tank remaps it to glue fire");
        const auto wasd = ReadPlayDirections(keys({Keys::W, Keys::A}));
        const auto arrows = ReadPlayDirections(keys({Keys::Up, Keys::Left}));
        check(wasd.move == arrows.move && wasd.turn == arrows.turn && wasd.move == 1 && wasd.turn == -1,
              "WASD uses the same movement and turning as the arrows");
        const auto reverse = ReadPlayDirections(keys({Keys::S, Keys::D}));
        check(reverse.move == -1 && reverse.turn == 1, "S and D map to Down and Right");
        const auto both = ReadPlayDirections(keys({Keys::W, Keys::Up, Keys::D, Keys::Right}));
        check(both.move == 1 && both.turn == 1, "WASD plus arrows cannot double movement speed");
        const auto opposite = ReadPlayDirections(keys({Keys::W, Keys::Down, Keys::Left, Keys::D}));
        check(opposite.move == 0 && opposite.turn == 0, "opposing keys from either set cancel");
        check(ReadPlayDirections(keys({Keys::X})).backUp &&
                  ReadPlayDirections(keys({Keys::RightControl})).backUp &&
                  !ReadPlayDirections(keys({Keys::S})).backUp,
              "X and Right Ctrl back up; S retains the turn-around command");
        ControlsHelp help;
        check(!help.Update(keys({})), "closed help lets the game update");
        check(help.Update(keys({Keys::F1})) && help.shown, "F1 opens help and freezes the game");
        check(help.Update(keys({Keys::F1})) && help.shown, "holding F1 keeps help open");
        (void)help.Update(keys({}));
        check(help.Update(keys({Keys::Escape})) && !help.shown, "Escape closes help and consumes its frame");
        check(!help.Update(keys({Keys::Escape})), "help releases the game after closing");
        (void)help.Update(keys({}));
        (void)help.Update(keys({Keys::F1}));
        (void)help.Update(keys({}));
        check(help.Update(keys({Keys::F1})) && !help.shown, "a second F1 press closes help");
    }

    {
        using namespace Microsoft::Xna::Framework::Input;
        ControlsHelp help;
        const Keys switching[] = {Keys::Left,Keys::Right,Keys::A,Keys::D,Keys::PageUp,Keys::PageDown,
                                  Keys::LeftShift,Keys::RightControl};
        for (Keys key : switching)
        {
            help.OpenCheats();
            (void)help.Update(KeyboardState{key});
            check(help.shown && help.page == 1, "cheat help: each navigation key changes page");
            (void)help.Update(KeyboardState{key});
            check(help.page == 1, "cheat help: holding navigation does not repeat");
            (void)help.Update({});
            (void)help.Update(KeyboardState{key});
            check(help.page == 0, "cheat help: next press returns to the other page");
        }
        for (const auto size : {std::pair{800,480},std::pair{800,360}})
        {
            const float scale = size.second/480.f, ox = (size.first-640*scale)/2;
            for (float x : {230.f,400.f})
            {
                help.OpenCheats();
                const int arrow = ControlsHelp::PageArrow(ox+x*scale,418*scale,size.first,size.second);
                (void)help.Update({},true,false,arrow);
                check(arrow != 0 && help.page == 1, "cheat help: both scaled pointer arrows change page");
                (void)help.Update({},true,false,arrow);
                check(help.page == 1, "cheat help: held mouse/touch changes page only once");
            }
        }
        help.OpenCheats();
        check(help.page == 0, "opening cheats always starts on page one");
        help.cheats = false;
        (void)help.Update(KeyboardState{Keys::Right});
        check(help.page == 0, "F1 controls help has no page navigation");
    }

    {
        InputPad pad;
        const auto press = pad.UpdateInit(mouse(585, 50, true), kViewportW, kViewportH);
        const auto held = pad.UpdateInit(mouse(585, 50, true), kViewportW, kViewportH);
        const auto release = pad.UpdateInit(mouse(585, 50, false), kViewportW, kViewportH);
        check(!press.exitPressed && !held.exitPressed && release.exitPressed && !release.playPressed &&
                  !release.editorPressed && !release.setupPressed && release.gamerSelected == -1,
              "main-menu Exit fires once on release and cannot trigger other buttons");
        (void)pad.UpdateInit(mouse(1170, 100, true), 1280, 960);
        check(pad.UpdateInit(mouse(1170, 100, false), 1280, 960).exitPressed,
              "Exit hit area scales with the viewport");
    }

    {
        InputPad pad;
        const auto press=pad.UpdateInit(mouse(75,109,true),800,480);
        const auto release=pad.UpdateInit(mouse(75,109,false),800,480);
        check(!press.helpPressed && release.helpPressed && !release.fullscreenPressed && !release.playPressed,
              "main-menu Help opens independently on release");
        // The Demo button stands under Exit (w-130..w-20, 82..136 reference pixels).
        const auto demoPress = pad.UpdateInit(mouse(725, 109, true), 800, 480);
        const auto demoRelease = pad.UpdateInit(mouse(725, 109, false), 800, 480);
        check(!demoPress.demoPressed && demoRelease.demoPressed && !demoRelease.exitPressed &&
                  !demoRelease.playPressed && !demoRelease.editorPressed && !demoRelease.helpPressed &&
                  demoRelease.gamerSelected == -1,
              "main-menu Demo fires once on release and cannot trigger other buttons");
        (void)pad.UpdateInit(mouse(725, 109, true), 800, 480);
        check(!pad.UpdateInit(mouse(400, 300, false), 800, 480).demoPressed,
              "a press on Demo released elsewhere does nothing");
        (void)pad.UpdateInit(mouse(1170, 220, true), 1280, 960);
        check(pad.UpdateInit(mouse(1170, 220, false), 1280, 960).demoPressed && !pad.UpdateInit(
                  mouse(1170, 220, false), 1280, 960).demoPressed,
              "the Demo hit area scales with the viewport");
        (void)pad.UpdateInit(mouse(725, 60, true), 800, 480);
        const auto exitRelease = pad.UpdateInit(mouse(725, 60, false), 800, 480);
        check(exitRelease.exitPressed && !exitRelease.demoPressed, "Exit above it stays Exit");
        // The Galaxy button (BD-0119) stands left of Demo (w-250..w-140, 82..136).
        (void)pad.UpdateInit(mouse(605, 109, true), 800, 480);
        const auto galaxyRelease = pad.UpdateInit(mouse(605, 109, false), 800, 480);
        check(galaxyRelease.galaxyPressed && !galaxyRelease.demoPressed && !galaxyRelease.playPressed &&
                  !galaxyRelease.editorPressed,
              "main-menu Galaxy fires on release and only Galaxy");
        (void)pad.UpdateInit(mouse(605, 109, true), 800, 480);
        check(!pad.UpdateInit(mouse(725, 109, false), 800, 480).galaxyPressed &&
                  !pad.UpdateInit(mouse(725, 109, false), 800, 480).demoPressed,
              "a press on Galaxy released over Demo does neither");
        // The Demos button (the demo positions from the menu) over Galaxy, left of Exit.
        (void)pad.UpdateInit(mouse(605, 42, true), 800, 480);
        const auto demosRelease = pad.UpdateInit(mouse(605, 42, false), 800, 480);
        check(demosRelease.demosPressed && !demosRelease.galaxyPressed && !demosRelease.exitPressed &&
                  !demosRelease.demoPressed && !demosRelease.fullscreenPressed,
              "main-menu Demos fires on release and only Demos");
        // Pause's Record button (BD-0116), top right, only where it is shown.
        {
            InputPad pause;
            (void)pause.UpdatePause(mouse(725, 42, true), 800, 480, true, true, true);
            const auto recorded = pause.UpdatePause(mouse(725, 42, false), 800, 480, true, true, true);
            (void)pause.UpdatePause(mouse(725, 42, true), 800, 480, true, true, false);
            const auto hidden = pause.UpdatePause(mouse(725, 42, false), 800, 480, true, true, false);
            check(recorded.recordPressed && !recorded.continuePressed && !recorded.menuPressed && !hidden.recordPressed,
                  "Pause: Record fires on release where it is shown, and is absent elsewhere");
            std::vector<InputPad::DemoSlotInfo> slots(10);
            slots[4].recorded = true;
            const auto at = [](const QuadBatch::Rect& r) { return std::make_pair(int((r.x0 + r.x1) / 2), int((r.y0 + r.y1) / 2)); };
            const auto [sx, sy] = at(InputPad::DemoSlotRect(2, 800, 480));
            (void)pause.UpdateDemoSlots(mouse(sx, sy, true), 800, 480, slots);
            const auto third = pause.UpdateDemoSlots(mouse(sx, sy, false), 800, 480, slots);
            const auto [rx, ry] = at(InputPad::DemoRestoreRect(4, 800, 480));
            (void)pause.UpdateDemoSlots(mouse(rx, ry, true), 800, 480, slots);
            const auto restore = pause.UpdateDemoSlots(mouse(rx, ry, false), 800, 480, slots);
            const auto [ox, oy] = at(InputPad::DemoRestoreRect(2, 800, 480));
            (void)pause.UpdateDemoSlots(mouse(ox, oy, true), 800, 480, slots);
            const auto noOriginal = pause.UpdateDemoSlots(mouse(ox, oy, false), 800, 480, slots);
            const auto [cx, cy] = at(InputPad::DemoCancelRect(800, 480));
            (void)pause.UpdateDemoSlots(mouse(cx, cy, true), 800, 480, slots);
            const auto cancel = pause.UpdateDemoSlots(mouse(cx, cy, false), 800, 480, slots);
            (void)pause.UpdateDemoSlots(mouse(sx, sy, true), 800, 480, slots);
            const auto moved = pause.UpdateDemoSlots(mouse(cx, cy, false), 800, 480, slots);
            const auto [tx, ty] = at(InputPad::DemoSlotRect(9, 1280, 960));
            (void)pause.UpdateDemoSlots(mouse(tx, ty, true), 1280, 960, slots);
            const auto tenth = pause.UpdateDemoSlots(mouse(tx, ty, false), 1280, 960, slots);
            const auto [ex, ey] = at(InputPad::DemoExportRect(800, 480));
            (void)pause.UpdateDemoSlots(mouse(ex, ey, true), 800, 480, slots);
            const auto exportButton = pause.UpdateDemoSlots(mouse(ex, ey, false), 800, 480, slots);
            const auto [ix, iy] = at(InputPad::DemoImportRect(800, 480));
            (void)pause.UpdateDemoSlots(mouse(ix, iy, true), 800, 480, slots);
            const auto importButton = pause.UpdateDemoSlots(mouse(ix, iy, false), 800, 480, slots);
            const auto cancelBox = InputPad::DemoCancelRect(800, 480);
            const auto exportRect = InputPad::DemoExportRect(800, 480), importRect = InputPad::DemoImportRect(800, 480);
            const auto nameRect = InputPad::DemoNameRect(800, 480);
            const auto [nx, ny] = at(nameRect);
            (void)pause.UpdateDemoSlots(mouse(nx, ny, true), 800, 480, slots);
            const auto nameButton = pause.UpdateDemoSlots(mouse(nx, ny, false), 800, 480, slots);
            check(exportButton.exportPressed && !exportButton.importPressed && !exportButton.cancelPressed &&
                      importButton.importPressed && !importButton.exportPressed && nameButton.namePressed &&
                      !nameButton.cancelPressed && exportRect.x1 <= importRect.x0 && importRect.x1 <= nameRect.x0 &&
                      nameRect.x1 <= cancelBox.x0 && exportRect.x0 >= 400.0f - 320.0f && cancelBox.x1 <= 400.0f + 320.0f,
                  "Demo positions: Export, Import, Name and Cancel in a row, inside the panel");
            {
                const auto [ox, oy] = at(InputPad::PauseOriginalRect(800, 480));
                InputPad pad;
                (void)pad.UpdatePause(mouse(ox, oy, true), 800, 480, false, false, true, true);
                const auto original = pad.UpdatePause(mouse(ox, oy, false), 800, 480, false, false, true, true);
                (void)pad.UpdatePause(mouse(ox, oy, true), 800, 480, false, false, true, false);
                const auto hiddenOriginal = pad.UpdatePause(mouse(ox, oy, false), 800, 480, false, false, true, false);
                const auto o = InputPad::PauseOriginalRect(800, 480), r = InputPad::PauseRecordRect(800, 480);
                check(original.originalPressed && !original.recordPressed && !hiddenOriginal.originalPressed &&
                          o.x1 <= r.x0 && o.y0 == r.y0,
                      "Pause: Original left of Record, only when shown");
            }
            const auto [lx, ly] = at(InputPad::DemoListRowRect(2, 800, 480));
            (void)pause.UpdateDemoSlots(mouse(lx, ly, true), 800, 480, slots, 3);
            const auto row = pause.UpdateDemoSlots(mouse(lx, ly, false), 800, 480, slots, 3);
            const auto [qx, qy] = at(InputPad::DemoListRowRect(5, 800, 480));
            (void)pause.UpdateDemoSlots(mouse(qx, qy, true), 800, 480, slots, 3);
            const auto beyond = pause.UpdateDemoSlots(mouse(qx, qy, false), 800, 480, slots, 3);
            const auto [hx, hy] = at(InputPad::DemoSlotRect(0, 800, 480));
            (void)pause.UpdateDemoSlots(mouse(hx, hy, true), 800, 480, slots, 0);
            const auto underList = pause.UpdateDemoSlots(mouse(hx, hy, false), 800, 480, slots, 0);
            const auto last = InputPad::DemoListRowRect(InputPad::kDemoListRows - 1, 800, 480);
            check(row.importRow == 3 && row.recordSlot == 0 && beyond.importRow == 0 && underList.recordSlot == 0 &&
                      underList.importRow == 0 && last.y1 <= 380.0f && InputPad::DemoListRowRect(0, 800, 480).y0 >= 60.0f,
                  "Demo positions: Import's list in their place, a row chooses its file, the positions hidden under it");
            const auto [bx, by] = at(InputPad::DemoListDeleteRect(1, 800, 480));
            (void)pause.UpdateDemoSlots(mouse(bx, by, true), 800, 480, slots, 3);
            const auto bin = pause.UpdateDemoSlots(mouse(bx, by, false), 800, 480, slots, 3);
            const auto [px, py] = at(InputPad::DemoListNextRect(800, 480));
            (void)pause.UpdateDemoSlots(mouse(px, py, true), 800, 480, slots, 3, true);
            const auto next = pause.UpdateDemoSlots(mouse(px, py, false), 800, 480, slots, 3, true);
            (void)pause.UpdateDemoSlots(mouse(px, py, true), 800, 480, slots, 3, false);
            const auto noArrows = pause.UpdateDemoSlots(mouse(px, py, false), 800, 480, slots, 3, false);
            bool binsIn = true;
            for (int i = 0; i < InputPad::kDemoListRows; ++i)
            {
                const auto r = InputPad::DemoListRowRect(i, 800, 480), d = InputPad::DemoListDeleteRect(i, 800, 480);
                binsIn = binsIn && d.x0 > r.x0 && d.x1 <= r.x1 && d.y0 >= r.y0 && d.y1 <= r.y1;
            }
            const auto prevR = InputPad::DemoListPrevRect(800, 480), nextR = InputPad::DemoListNextRect(800, 480);
            const auto lastRow = InputPad::DemoListRowRect(InputPad::kDemoListRows - 1, 800, 480);
            check(bin.deleteRow == 2 && bin.importRow == 0 && next.nextPage && !next.prevPage && !noArrows.nextPage &&
                      binsIn && prevR.y0 >= lastRow.y1 && nextR.y1 <= 380.0f &&
                      nextR.y1 <= InputPad::DemoImportRect(800, 480).y0,
                  "Demo positions' list: a row's bin is its own control, the arrows under the rows only when paged");
            check(third.recordSlot == 3 && third.restoreSlot == 0 && restore.restoreSlot == 5 && restore.recordSlot == 0 &&
                      noOriginal.restoreSlot == 0 && noOriginal.recordSlot == 0 && cancel.cancelPressed &&
                      cancel.recordSlot == 0 && moved.recordSlot == 0 && !moved.cancelPressed && tenth.recordSlot == 10,
                  "Demo positions: a slot records into it, Original only under a recorded one, Cancel, a press "
                  "released elsewhere does nothing, scaled with the viewport");
            bool apart = true;
            for (int i = 0; i < 10; ++i)
                for (int j = 0; j < 10; ++j)
                {
                    const auto a = InputPad::DemoSlotRect(i, 800, 480), b = InputPad::DemoRestoreRect(j, 800, 480);
                    apart &= a.x1 <= b.x0 || b.x1 <= a.x0 || a.y1 <= b.y0 || b.y1 <= a.y0;
                    if (i != j)
                    {
                        const auto c = InputPad::DemoSlotRect(j, 800, 480);
                        apart &= a.x1 <= c.x0 || c.x1 <= a.x0 || a.y1 <= c.y0 || c.y1 <= a.y0;
                    }
                }
            const auto cancelRect = InputPad::DemoCancelRect(800, 480);
            apart &= InputPad::DemoRestoreRect(9, 800, 480).y1 <= cancelRect.y0 && cancelRect.y1 <= 480.0f &&
                     InputPad::DemoSlotRect(0, 800, 480).x0 >= 0.0f && InputPad::DemoSlotRect(4, 800, 480).x1 <= 800.0f;
            check(apart, "Demo positions: no two controls overlap, all on the screen");
        }
        ControlsHelp help;help.shown=true;
        const bool over=ControlsHelp::OverClose(695,35,800,480);
        check(over && help.Update({},true,over) && !help.shown,
              "the visible Close control dismisses help without a keyboard");
        check(help.Update({},true,false) && help.Update({},false,false) && !help.Update({}),
              "Close consumes the pointer through release so the underlying Exit/Play cannot fire");
    }

    // --- Play: D-pad discrete drag ---
    {
        InputPad pad;
        InputPad::PlayInput in;

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
        InputPad pad;
        InputPad::PlayInput in;

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
        InputPad pad;
        InputPad::PlayInput in;

        // Real PlayAction rect (GetButtonRect(), jump on the right):
        // (W-1.2*96, H-2.6*96)-(W-0.2*96, H-1.6*96) = (524.8,230.4)-(620.8,326.4).
        (void)pad.UpdatePlay(mouse(572, 278, true), kViewportW, kViewportH, in);
        check(!in.actionPressed, "Action: not yet fired on the press frame itself");

        (void)pad.UpdatePlay(mouse(572, 278, true), kViewportW, kViewportH, in);
        check(!in.actionPressed, "Action: not fired while still held");

        // Release, dragged off the button first -- real semantics fire
        // based on where the press STARTED, not the release point.
        (void)pad.UpdatePlay(mouse(10, 10, false), kViewportW, kViewportH, in);
        check(in.actionPressed, "Action: fires on release even though the release point is off the button");

        (void)pad.UpdatePlay(mouse(10, 10, false), kViewportW, kViewportH, in);
        check(!in.actionPressed, "Action: single-fire -- does not refire on a later no-op frame");
    }
    {
        InputPad pad;
        InputPad::PlayInput in;

        // Real PlayPause rect: (580,10)-(630,60).
        (void)pad.UpdatePlay(mouse(600, 30, true), kViewportW, kViewportH, in);
        (void)pad.UpdatePlay(mouse(600, 30, false), kViewportW, kViewportH, in);
        check(in.pausePressed, "Pause (Play-phase button): fires on release");
    }

    // --- Play: UpdatePlay()'s own return value (mouse-claimed-by-a-
    // control signal, used to suppress the camera drag-look) ---
    {
        InputPad pad;
        InputPad::PlayInput in;

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
        InputPad pad;

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
        InputPad pad;
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
        InputPad pad;
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
        InputPad pad;
        const bool pressReturn = pad.UpdateWinLost(mouse(470, 60, true), kViewportW, kViewportH);
        check(!pressReturn, "WinLostReturn: not yet fired on the press frame itself");
        const bool releaseReturn = pad.UpdateWinLost(mouse(470, 60, false), kViewportW, kViewportH);
        check(releaseReturn, "WinLostReturn: fires on release when the press landed inside its rect");
    }
    {
        InputPad pad;
        (void)pad.UpdateWinLost(mouse(10, 10, true), kViewportW, kViewportH);
        const bool releaseOutside = pad.UpdateWinLost(mouse(10, 10, false), kViewportW, kViewportH);
        check(!releaseOutside, "WinLostReturn: does not fire when the press landed outside its rect");
    }

    // --- PlaySetup: real rects, verified against InputPad.cpp's own
    // bsf2=drawBoundsHeight*140/480 formula, which is EXACTLY 140 at this
    // engine's 480 reference height -- used unadapted (see
    // InputPad.hpp's UpdateSetup() class comment). SetupSounds
    // (X 20-90, Y 180-250) and SetupReturn (X 508-620, Y 348-460) are
    // functional, as are SetupJump/Zoom/Accel since OR-0010 (2026-11-02).
    {
        InputPad pad;
        (void)pad.UpdateSetup(mouse(55, 215, true), kViewportW, kViewportH, false);
        const auto release = pad.UpdateSetup(mouse(55, 215, false), kViewportW, kViewportH, false);
        check(release.soundsToggled && !release.returnPressed,
              "Setup: Sounds toggle fires on release, distinct from Return");
    }
    {
        InputPad pad;
        (void)pad.UpdateSetup(mouse(564, 404, true), kViewportW, kViewportH, false);
        const auto release = pad.UpdateSetup(mouse(564, 404, false), kViewportW, kViewportH, false);
        check(release.returnPressed && !release.soundsToggled,
              "Setup: Return fires on release, distinct from Sounds");
    }
    {
        InputPad pad;
        // SetupJump rect: X 20-90, Y 250-320 -> center (55,285).
        (void)pad.UpdateSetup(mouse(55, 285, true), kViewportW, kViewportH, false);
        const auto release = pad.UpdateSetup(mouse(55, 285, false), kViewportW, kViewportH, false);
        check(release.jumpToggled && !release.soundsToggled && !release.returnPressed,
              "Setup: the Jump-side toggle fires on release (OR-0010)");
    }
    {
        // SetupZoom (Y 320-390) and SetupAccel (Y 390-460) toggle too (OR-0010).
        InputPad pad;
        pad.SetAccelerometerSupported(true);
        (void)pad.UpdateSetup(mouse(55, 355, true), kViewportW, kViewportH, false);
        const auto zoom = pad.UpdateSetup(mouse(55, 355, false), kViewportW, kViewportH, false);
        (void)pad.UpdateSetup(mouse(55, 425, true), kViewportW, kViewportH, false);
        const auto accel = pad.UpdateSetup(mouse(55, 425, false), kViewportW, kViewportH, false);
        check(zoom.zoomToggled && !zoom.jumpToggled && accel.accelToggled && !accel.zoomToggled,
              "Setup: the automatic zoom and accelerometer toggles fire on release (OR-0010)");
    }
    {
        // The accelerometer's sensitivity (the original's accelSlider, GameData data[7]): right of
        // "Accelerometer" in its row (Y 390-460), live only while the accelerometer is on.
        InputPad off;
        off.SetAccelerometerSupported(true);
        const auto none = off.UpdateSetup(mouse(342, 439, true), kViewportW, kViewportH, false, false);
        check(none.accelSensitivity < 0, "Setup: no sensitivity slider while the accelerometer is off");
        InputPad on;
        on.SetAccelerometerSupported(true);
        const auto mid = on.UpdateSetup(mouse(342, 439, true), kViewportW, kViewportH, false, true);
        const auto end = on.UpdateSetup(mouse(342, 439, false), kViewportW, kViewportH, false, true);
        check(mid.accelSensitivity >= 45 && mid.accelSensitivity <= 55 && end.volumeReleased && !end.accelToggled,
              "Setup: with the accelerometer on, its sensitivity slider reads about 50 at the track's middle");
    }
    {
        // A phone's saved setting may be on when the save is opened on a sensorless desktop.
        // Both Setup screens must ignore presses/drags in those slots and navigate only visible controls.
        for (const bool showReset : {false, true})
        {
            InputPad pad;
            pad.SetAccelerometerSupported(false);
            for (const bool savedAccel : {false, true})
            {
                (void)pad.UpdateSetup(mouse(55, 425, true), kViewportW, kViewportH, showReset, savedAccel);
                const auto toggle = pad.UpdateSetup(mouse(55, 425, false), kViewportW, kViewportH, showReset, savedAccel);
                const auto drag = pad.UpdateSetup(mouse(342, 439, true), kViewportW, kViewportH, showReset, savedAccel);
                const auto release = pad.UpdateSetup(mouse(430, 439, false), kViewportW, kViewportH, showReset, savedAccel);
                check(!toggle.accelToggled && drag.accelSensitivity < 0 && release.accelSensitivity < 0 &&
                          !release.volumeReleased,
                      "Main/Play Setup without a sensor: hidden toggle and sensitivity ignore pointer input, saved on or off");

                for (const auto [width, height] : {std::pair{640, 480}, std::pair{800, 480}, std::pair{1280, 720}})
                {
                    const float scale = static_cast<float>(height) / 480.0f;
                    const auto targets = pad.SetupTargets(width, height, showReset, savedAccel);
                    bool hiddenTarget = false;
                    for (const auto& r : targets)
                        hiddenTarget |= QuadBatch::InRect(55 * scale, 425 * scale, r) ||
                                        QuadBatch::InRect(342 * scale, 439 * scale, r);
                    check(!hiddenTarget, "Main/Play Setup focus targets exclude both sensor slots at every tested viewport");
                    FocusNavigator navigator;
                    FocusNavigator::Input arrow;
                    arrow.down = true;
                    auto out = navigator.Update(targets, arrow); // Frame rate
                    out = navigator.Update(targets, arrow); // Sounds
                    out = navigator.Update(targets, arrow); // Jump
                    out = navigator.Update(targets, arrow); // Zoom
                    check(out.active && std::fabs(out.x - 55 * scale) < 0.01f &&
                              std::fabs(out.y - 355 * scale) < 0.01f,
                          "Main/Play Setup: Down reaches Automatic zoom on action");
                    out = navigator.Update(targets, arrow);
                    check(out.x > 440 * scale,
                          "Main/Play Setup: next Down skips hidden Accelerometer and Sensitivity to 3D effects");
                }
            }
            pad.SetAccelerometerSupported(true);
            const auto off = pad.SetupTargets(kViewportW, kViewportH, showReset, false);
            const auto on = pad.SetupTargets(kViewportW, kViewportH, showReset, true);
            const auto hasPoint = [](const auto& targets, float x, float y) {
                return std::any_of(targets.begin(), targets.end(), [=](const auto& r) {
                    return QuadBatch::InRect(x, y, r);
                });
            };
            check(hasPoint(off, 55, 425) && !hasPoint(off, 342, 439) &&
                      hasPoint(on, 55, 425) && hasPoint(on, 342, 439),
                  "Main/Play Setup with a sensor: toggle always focusable, sensitivity only with the saved option on");
        }
    }
    {
        // The volume sliders (2026-11-02, the owner's request): right of "Sound effects" in its row
        // (Y 180-250), the effects' then the music's; pressed and dragged they report 0..100 and
        // the release; the Sounds toggle does not fire. Erase progress moved a row lower.
        InputPad pad;
        const auto press = pad.UpdateSetup(mouse(400, 230, true), kViewportW, kViewportH, true);
        const auto dragLeft = pad.UpdateSetup(mouse(100, 230, true), kViewportW, kViewportH, true);
        const auto release = pad.UpdateSetup(mouse(100, 230, false), kViewportW, kViewportH, true);
        check(press.effectsVolume > 0 && press.effectsVolume < 100 && press.musicVolume < 0 &&
                  dragLeft.effectsVolume == 0 && release.volumeReleased && !release.soundsToggled,
              "Setup: the effects' volume slider follows a drag and reports its release");
        InputPad music;
        const auto pressMusic = music.UpdateSetup(mouse(kViewportW - 35, 230, true), kViewportW, kViewportH, true);
        check(pressMusic.musicVolume == 100 && pressMusic.effectsVolume < 0,
              "Setup: the music's slider is right of the effects' (its right end: 100)");
        InputPad reset;
        (void)reset.UpdateSetup(mouse(470, 285, true), kViewportW, kViewportH, true);
        const auto erased = reset.UpdateSetup(mouse(470, 285, false), kViewportW, kViewportH, true);
        check(erased.resetPressed, "Setup: Erase progress is a row lower (the Jump row), right of it");
    }
    {
        // Several fingers (owner 2026-11-02, touch devices): one steers the stick up while another
        // holds Jump; Action fires when its finger lifts; the back-up button just over the stick's ring is
        // held by the finger that pressed it, and a finger dragged over it from the stick does not.
        InputPad pad;
        using TP = InputPad::TouchPoint;
        InputPad::PlayInput in;
        (void)pad.UpdatePlayTouches({TP{1, 100.0f, 380.0f, true, false}, TP{2, 570.0f, 410.0f, true, false}},
                                    kViewportW, kViewportH, in);
        (void)pad.UpdatePlayTouches({TP{1, 100.0f, 330.0f, false, false}, TP{2, 570.0f, 410.0f, false, false}},
                                    kViewportW, kViewportH, in);
        check(in.moveInput == 1.0f && in.turnInput == 0.0f && in.jumpHeld,
              "touch: the stick and Jump held together by two fingers");
        (void)pad.UpdatePlayTouches({TP{1, 100.0f, 270.0f, false, false}}, kViewportW, kViewportH, in);
        check(!in.backUpHeld && in.moveInput == 1.0f, "touch: the stick's finger over the back-up button stays the stick");
        (void)pad.UpdatePlayTouches({TP{1, 100.0f, 270.0f, false, true}}, kViewportW, kViewportH, in);
        (void)pad.UpdatePlayTouches({TP{3, 100.0f, 268.0f, true, false}}, kViewportW, kViewportH, in);
        check(in.backUpHeld && in.moveInput == 0.0f, "touch: a finger pressed on the back-up button holds it");
        (void)pad.UpdatePlayTouches({TP{3, 100.0f, 268.0f, false, true}}, kViewportW, kViewportH, in);
        check(!in.backUpHeld, "touch: lifted, the back-up button lets go");
        // The crouch button (the original's PlayDown) beside Jump, held while a finger is on it,
        // together with Jump under another finger.
        (void)pad.UpdatePlayTouches({TP{4, 457.0f, 413.0f, true, false}, TP{5, 570.0f, 410.0f, true, false}},
                                    kViewportW, kViewportH, in);
        check(in.crouchHeld && in.jumpHeld, "touch: the crouch button beside Jump is held with Jump");
        (void)pad.UpdatePlayTouches({TP{4, 457.0f, 413.0f, false, true}}, kViewportW, kViewportH, in);
        check(!in.crouchHeld, "touch: lifted, the crouch button lets go");
        // The mouse: pressed on it, held while the button stays down.
        InputPad mousePad;
        InputPad::PlayInput mouseIn;
        (void)mousePad.UpdatePlay(mouse(457, 413, true), kViewportW, kViewportH, mouseIn);
        check(mouseIn.crouchHeld, "mouse: pressed on the crouch button, it is held");
        // With the accelerometer steering (SetAccelerometerMode): no stick; Forward and Back in its
        // corner, where Action and Jump stand in the other one; Back backs him up.
        InputPad tilt;
        tilt.SetAccelerometerMode(true);
        (void)tilt.UpdatePlayTouches({TP{1, 67.0f, 278.0f, true, false}}, kViewportW, kViewportH, in);
        check(in.moveInput == 1.0f && in.turnInput == 0.0f && !in.backUpHeld, "accelerometer: Forward held walks him on");
        (void)tilt.UpdatePlayTouches({TP{1, 67.0f, 278.0f, false, true}}, kViewportW, kViewportH, in);
        (void)tilt.UpdatePlayTouches({TP{2, 70.0f, 410.0f, true, false}}, kViewportW, kViewportH, in);
        check(in.backUpHeld && in.moveInput == 0.0f, "accelerometer: Back held backs him up");
        (void)tilt.UpdatePlayTouches({TP{2, 70.0f, 410.0f, false, true}}, kViewportW, kViewportH, in);
        (void)tilt.UpdatePlayTouches({TP{3, 150.0f, 330.0f, true, false}}, kViewportW, kViewportH, in);
        (void)tilt.UpdatePlayTouches({TP{3, 190.0f, 330.0f, false, false}}, kViewportW, kViewportH, in);
        check(in.moveInput == 0.0f && in.turnInput == 0.0f && !in.backUpHeld,
              "accelerometer: no stick where it stood (the tilt turns him)");
    }
    {
        // A gamepad as keys (GamepadInput.hpp, owner 2026-11-02): none when unplugged; the stick past
        // half way or the D-pad walks and turns; A jumps and confirms, B/X act, RB backs up, LB
        // crouches, Y looks up, Start pauses.
        using namespace GalaxyEggbert::Game;
        GamepadFrame f;
        f.a = true;
        check(GamepadKeys(f) == 0u, "a gamepad not connected presses nothing");
        f.connected = true;
        f.leftY = 0.8f;
        f.leftX = -0.3f;
        f.rightShoulder = true;
        const unsigned keys = GamepadKeys(f);
        check((keys & kPadUp) && !(keys & kPadLeft) && (keys & kPadJump) && (keys & kPadConfirm) &&
                  (keys & kPadBackUp) && !(keys & kPadAction),
              "the stick past half way walks, A jumps and confirms, RB backs up");
        GamepadFrame g;
        g.connected = true;
        g.dpadDown = true;
        g.x = true;
        g.start = true;
        g.leftShoulder = true;
        g.y = true;
        const unsigned more = GamepadKeys(g);
        check((more & kPadDown) && (more & kPadAction) && (more & kPadPause) && (more & kPadCrouch) &&
                  (more & kPadLookUp),
              "the D-pad down turns round, X acts, Start pauses, LB crouches, Y looks up");
    }
    {
        // The active cheats' panel (Mobile Eggbert's InputPad.cpp:1290-1316): from (2, 2) at 480 high,
        // 18 px a name plus the padding; wider for a longer name; none when empty.
        const auto one = InputPad::ActiveCheatsRect({"ghost"}, kViewportH);
        const auto two = InputPad::ActiveCheatsRect({"ghost", "opendoors"}, kViewportH);
        check(std::fabs(one.x0 - 2.0f) < 0.01f && std::fabs(one.y0 - 2.0f) < 0.01f &&
                  std::fabs(one.y1 - 26.0f) < 0.01f && std::fabs(two.y1 - 44.0f) < 0.01f && two.x1 > one.x1,
              "the active cheats' panel: top-left, 18 px a name, as wide as the longest");
        // Galaxy's own notice under the training hint's banner (its 40 reference pixels).
        const auto under = InputPad::ActiveCheatsRect({"Mission 11: the world as shipped again."}, kViewportH, 40.0f);
        check(under.y0 >= 40.0f * kViewportH / 480.0f && std::fabs(under.y1 - under.y0 - 24.0f) < 0.01f &&
                  std::fabs(under.x0 - one.x0) < 0.01f,
              "a notice moved under the hint's banner keeps its height and left edge");
    }
    {
        // Jump on the left (the setting off): the stick and the buttons trade sides.
        InputPad pad;
        InputPad::PlayInput right;
        (void)pad.UpdatePlay(mouse(67, 412, true), kViewportW, kViewportH, right);
        (void)pad.UpdatePlay(mouse(67, 412, false), kViewportW, kViewportH, right);
        pad.SetJumpRight(false);
        InputPad::PlayInput stick;
        (void)pad.UpdatePlay(mouse(540, 380, true), kViewportW, kViewportH, stick);
        (void)pad.UpdatePlay(mouse(610, 380, true), kViewportW, kViewportH, stick);
        (void)pad.UpdatePlay(mouse(610, 380, false), kViewportW, kViewportH, stick);
        InputPad::PlayInput jump;
        (void)pad.UpdatePlay(mouse(67, 412, true), kViewportW, kViewportH, jump);
        check(!right.jumpHeld && stick.turnInput == 0.0f && jump.jumpHeld,
              "Play: with the Jump button on the left, Jump is pressed there (the bottom left is the stick otherwise)");
        InputPad::PlayInput drag;
        InputPad mirrored;
        mirrored.SetJumpRight(false);
        (void)mirrored.UpdatePlay(mouse(540, 380, true), kViewportW, kViewportH, drag);
        (void)mirrored.UpdatePlay(mouse(610, 380, true), kViewportW, kViewportH, drag);
        check(drag.turnInput == 1.0f, "Play: and the stick works on the right (mirrored layout, OR-0010)");
    }

    // --- SetupReset (real: MainSetup-only, plan.md MENU-006..020): rect
    // X 450-520, Y 250-320 -> center (485,285) since 2026-11-02 (the original's Y 180-250, the
    // Sounds row, holds the volume sliders now). showReset gates both hit-
    // testing and firing -- confirms PlaySetup's own call (showReset=
    // false) can't ever trigger it even if a press lands in that rect.
    {
        InputPad pad;
        (void)pad.UpdateSetup(mouse(485, 285, true), kViewportW, kViewportH, true);
        const auto release = pad.UpdateSetup(mouse(485, 285, false), kViewportW, kViewportH, true);
        check(release.resetPressed, "Setup: Reset fires on release when showReset=true (MainSetup)");
    }
    {
        InputPad pad;
        (void)pad.UpdateSetup(mouse(485, 285, true), kViewportW, kViewportH, false);
        const auto release = pad.UpdateSetup(mouse(485, 285, false), kViewportW, kViewportH, false);
        check(!release.resetPressed, "Setup: Reset never fires when showReset=false (PlaySetup)");
    }

    // --- Resume: real rects (X 180.6-320.6 Menu / 320.6-460.6 Continue,
    // Y 308-448, same bsf2=140-at-this-reference-height "no adaptation
    // needed" situation as Setup). Both buttons are now functional (Menu
    // -> Init, 2026-07-13, now that Init exists).
    {
        InputPad pad;
        (void)pad.UpdateResume(mouse(390, 378, true), kViewportW, kViewportH);
        const auto release = pad.UpdateResume(mouse(390, 378, false), kViewportW, kViewportH);
        check(release.continuePressed && !release.menuPressed, "Resume: Continue fires on release");
    }
    {
        InputPad pad;
        (void)pad.UpdateResume(mouse(250, 378, true), kViewportW, kViewportH);
        const auto release = pad.UpdateResume(mouse(250, 378, false), kViewportW, kViewportH);
        check(release.menuPressed && !release.continuePressed, "Resume: Menu fires on release, distinct from Continue");
    }

    {
        InputPad pad;
        const auto press = pad.UpdateInit(mouse(105, 42, true), 800, 480);
        const auto release = pad.UpdateInit(mouse(105, 42, false), 800, 480);
        check(!press.fullscreenPressed && release.fullscreenPressed && !release.exitPressed,
              "main menu Fullscreen/Window button toggles on release independently of Exit");
        (void)pad.UpdateInit(mouse(105, 42, true), 800, 480);
        check(!pad.UpdateInit(mouse(300, 100, false), 800, 480).fullscreenPressed,
              "dragging away from display-mode button cancels the click");
    }

    {
        using TP=InputPad::TouchPoint;
        InputPad pad;
        InputPad::PlayInput in;
        (void)pad.UpdatePlay(mouse(618,278,true),800,480,in);
        check(!in.fireHeld,"the vehicle Fire button has no hidden hit area on foot");
        (void)pad.UpdatePlay(mouse(618,278,false),800,480,in);
        pad.SetVehicleFireEnabled(true);
        (void)pad.UpdatePlay(mouse(618,278,true),800,480,in);
        check(in.fireHeld && !in.actionPressed,"holding vehicle Fire does not also enter/leave a vehicle");
        (void)pad.UpdatePlay(mouse(618,278,false),800,480,in);
        check(!in.fireHeld && !in.actionPressed,"Fire release does not issue Action");
        (void)pad.UpdatePlayTouches({TP{7,618,278,false,false}},800,480,in);
        check(in.fireHeld,"a first Moved touch snapshot still holds vehicle Fire");
        (void)pad.UpdatePlayTouches({},800,480,in);
        check(!in.fireHeld,"missing Fire contact releases the trigger");
        (void)pad.UpdatePlayTouches({TP{8,733,278,false,false}},800,480,in);
        (void)pad.UpdatePlayTouches({},800,480,in);
        check(in.actionPressed,"touch Action retains release edges even when the platform skips Pressed/Released snapshots");
        GamepadFrame f; f.connected=true;f.rightTrigger=.8f;
        check((GamepadKeys(f)&kPadFire) && !(GamepadKeys(f)&kPadAction),"gamepad RT fires independently from B/X enter/leave");
    }

    // --- Init / gamer-select menu (plan.md MENU-006..020): real rects,
    // verified against InputPad.cpp's own bsf2=140-at-this-reference-
    // height formula (same "no adaptation needed" situation as Setup/
    // Resume). GamerA X20-90/Y166-236 -> center (55,201); GamerB
    // Y236-306 -> center (55,271); GamerC Y306-376 -> center (55,341);
    // InitSetup Y390-460 -> center (55,425); InitPlay X480-620/Y300-440
    // -> center (550,370).
    {
        InputPad pad;
        (void)pad.UpdateInit(mouse(55, 201, true), kViewportW, kViewportH);
        const auto release = pad.UpdateInit(mouse(55, 201, false), kViewportW, kViewportH);
        check(release.gamerSelected == 0 && !release.playPressed && !release.setupPressed,
              "Init: GamerA fires gamerSelected=0 on release");
    }
    {
        InputPad pad;
        (void)pad.UpdateInit(mouse(55, 271, true), kViewportW, kViewportH);
        const auto release = pad.UpdateInit(mouse(55, 271, false), kViewportW, kViewportH);
        check(release.gamerSelected == 1, "Init: GamerB fires gamerSelected=1 on release");
    }
    {
        InputPad pad;
        (void)pad.UpdateInit(mouse(55, 341, true), kViewportW, kViewportH);
        const auto release = pad.UpdateInit(mouse(55, 341, false), kViewportW, kViewportH);
        check(release.gamerSelected == 2, "Init: GamerC fires gamerSelected=2 on release");
    }
    {
        InputPad pad;
        (void)pad.UpdateInit(mouse(55, 425, true), kViewportW, kViewportH);
        const auto release = pad.UpdateInit(mouse(55, 425, false), kViewportW, kViewportH);
        check(release.setupPressed && release.gamerSelected == -1,
              "Init: InitSetup fires setupPressed on release, not a gamer selection");
    }
    {
        InputPad pad;
        (void)pad.UpdateInit(mouse(550, 370, true), kViewportW, kViewportH);
        const auto release = pad.UpdateInit(mouse(550, 370, false), kViewportW, kViewportH);
        check(release.playPressed && release.gamerSelected == -1,
              "Init: InitPlay fires playPressed on release, not a gamer selection");
    }
    {
        InputPad pad;
        (void)pad.UpdateInit(mouse(550, 370, true), kViewportW, kViewportH);
        const auto stillHeld = pad.UpdateInit(mouse(550, 370, true), kViewportW, kViewportH);
        check(!stillHeld.playPressed, "Init: InitPlay does not fire while still held (only on release)");
    }
    {
        // Not a real mobile-eggbert button (plan.md EDITOR-107) -- opens
        // the in-game 3D world editor's browser. It sits in the original's
        // InitBuy/InitRanking slot: (W-125,166)-(W-55,236), centre (550,201)
        // at this 640-wide viewport.
        InputPad pad;
        (void)pad.UpdateInit(mouse(550, 201, true), kViewportW, kViewportH);
        const auto release = pad.UpdateInit(mouse(550, 201, false), kViewportW, kViewportH);
        check(release.editorPressed && release.gamerSelected == -1 && !release.playPressed,
              "Init: InitEditor fires editorPressed on release, not a gamer selection or Play");
    }
    {
        InputPad pad;
        (void)pad.UpdateInit(mouse(550, 201, true), kViewportW, kViewportH);
        const auto stillHeld = pad.UpdateInit(mouse(550, 201, true), kViewportW, kViewportH);
        check(!stillHeld.editorPressed, "Init: InitEditor does not fire while still held (only on release)");
    }
    {
        // Play controls at 800x480 hug the screen's corners as in the
        // original: Pause (733,19)-(781,67), Jump (685,365)-(781,461).
        InputPad pad;
        InputPad::PlayInput in;
        (void)pad.UpdatePlay(mouse(757, 43, true), 800, 480, in);
        (void)pad.UpdatePlay(mouse(757, 43, false), 800, 480, in);
        const bool paused = in.pausePressed;
        (void)pad.UpdatePlay(mouse(733, 413, true), 800, 480, in);
        check(paused && in.jumpHeld, "Play: Pause and Jump stand in the screen's right corners at 800x480");
        (void)pad.UpdatePlay(mouse(733, 413, false), 800, 480, in);
    }
    {
        // Pause at 800x480: five 140-pixel buttons from the image origin's
        // x - 29.4, so Menu spans (50.6,308)-(190.6,448) and Continue
        // (610.6,308)-(750.6,448), reaching into the side bars.
        InputPad pad;
        auto press = pad.UpdatePause(mouse(70, 380, true), 800, 480, true, true);
        auto release = pad.UpdatePause(mouse(70, 380, false), 800, 480, true, true);
        auto press2 = pad.UpdatePause(mouse(740, 380, true), 800, 480, true, true);
        auto release2 = pad.UpdatePause(mouse(740, 380, false), 800, 480, true, true);
        (void)press;
        (void)press2;
        check(release.menuPressed && release2.continuePressed,
              "Pause: the real 140-pixel buttons span x 50-750 at 800x480");
    }
    {
        // The real buttons stand against the screen's edges, not the centred
        // 640x480 background: at 800x480 GamerA is still at x 20-90 and Play
        // at x 640-780.
        InputPad pad;
        (void)pad.UpdateInit(mouse(55, 201, true), 800, 480);
        const auto gamer = pad.UpdateInit(mouse(55, 201, false), 800, 480);
        (void)pad.UpdateInit(mouse(710, 370, true), 800, 480);
        const auto play = pad.UpdateInit(mouse(710, 370, false), 800, 480);
        check(gamer.gamerSelected == 0 && play.playPressed,
              "Init: buttons are placed from the screen's edges (800x480: GamerA x 20-90, Play x 640-780)");
    }

    if (kCheatsEnabled)
    {
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
        InputPad pad;
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
        InputPad pad;
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
        InputPad pad;
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
        InputPad pad;
        (void)pad.UpdateCheatMenu(mouse(30, 30, true), kViewportW, kViewportH);
        const int pressed = pad.UpdateCheatMenu(mouse(30, 30, false), kViewportW, kViewportH);
        check(pressed == 1, "Cheat menu: pressing the first button fires cheat 1 (OpenDoors) on release");
    }
    {
        InputPad pad;
        (void)pad.UpdateCheatMenu(mouse(600, 30, true), kViewportW, kViewportH);
        const int pressed = pad.UpdateCheatMenu(mouse(600, 30, false), kViewportW, kViewportH);
        check(pressed == 9, "Cheat menu: pressing the last button fires cheat 9 (EndGoal) on release");
    }
    {
        InputPad pad;
        (void)pad.UpdateCheatMenu(mouse(30, 30, true), kViewportW, kViewportH);
        const int pressed = pad.UpdateCheatMenu(mouse(30, 30, true), kViewportW, kViewportH);
        check(pressed == 0, "Cheat menu: not fired while still held (only fires on release)");
    }

    // --- Typed cheat code: "ghost" (plan.md BLUPI-111) ---
    {
        using Keys = Microsoft::Xna::Framework::Input::Keys;
        using KeyboardState = Microsoft::Xna::Framework::Input::KeyboardState;

        InputPad pad;
        bool anyTrue = false;
        // Types "ghost" one key-down-edge at a time, releasing between
        // each letter (matching the real "down-edge appends, held doesn't
        // repeat" semantics) -- only the final 't' completes the suffix
        // match.
        for (Keys k : {Keys::G, Keys::H, Keys::O, Keys::S, Keys::T})
        {
            const bool firedOnPress = pad.UpdateTypedGhostCheat(KeyboardState{k}, /*isPlayPhase=*/true).ghostTyped;
            anyTrue = anyTrue || firedOnPress;
            (void)pad.UpdateTypedGhostCheat(KeyboardState{}, /*isPlayPhase=*/true); // release edge
        }
        check(anyTrue, "Typed cheat: typing \"ghost\" letter by letter fires exactly once (on the final 't')");
    }
    {
        InputPad pad;
        using Keys = Microsoft::Xna::Framework::Input::Keys;
        using KeyboardState = Microsoft::Xna::Framework::Input::KeyboardState;
        bool anyTrue = false;
        for (Keys k : {Keys::G, Keys::H, Keys::O, Keys::S, Keys::T})
        {
            anyTrue = anyTrue || pad.UpdateTypedGhostCheat(KeyboardState{k}, /*isPlayPhase=*/false).ghostTyped;
            (void)pad.UpdateTypedGhostCheat(KeyboardState{}, /*isPlayPhase=*/false);
        }
        check(!anyTrue, "Typed cheat: typing \"ghost\" outside Play phase never fires (real Phase::Play gate)");
    }
    {
        InputPad pad;
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
            anyTrue = anyTrue || pad.UpdateTypedGhostCheat(KeyboardState{k}, true).ghostTyped;
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
        InputPad pad;
        using Keys = Microsoft::Xna::Framework::Input::Keys;
        using KeyboardState = Microsoft::Xna::Framework::Input::KeyboardState;
        bool anyTrue = false;
        for (Keys k : {Keys::X, Keys::Y, Keys::Z, Keys::G, Keys::H, Keys::O, Keys::S, Keys::T})
        {
            anyTrue = anyTrue || pad.UpdateTypedGhostCheat(KeyboardState{k}, true).ghostTyped;
            (void)pad.UpdateTypedGhostCheat(KeyboardState{}, true);
        }
        check(anyTrue, "Typed cheat: an unrelated prefix before \"ghost\" doesn't prevent the suffix match");
    }
    {
        // "quick" toggles quickCheatEnabled_ (SCORE-009/010/011 follow-up,
        // added 2026-07-20) -- same rolling-buffer mechanism as "ghost",
        // sharing the SAME buffer (typing "quickghost" should fire ghost
        // too, matching real source's one-shared-buffer design).
        InputPad pad;
        using Keys = Microsoft::Xna::Framework::Input::Keys;
        using KeyboardState = Microsoft::Xna::Framework::Input::KeyboardState;
        bool anyQuick = false;
        for (Keys k : {Keys::Q, Keys::U, Keys::I, Keys::C, Keys::K})
        {
            anyQuick = anyQuick || pad.UpdateTypedGhostCheat(KeyboardState{k}, true).quickTyped;
            (void)pad.UpdateTypedGhostCheat(KeyboardState{}, true);
        }
        check(anyQuick, "Typed cheat: typing \"quick\" letter by letter fires quickTyped exactly once");
    }
    {
        // OR-0007: the original's whole word table (InputPad.cpp:715-741), each word firing its cheat.
        using Keys = Microsoft::Xna::Framework::Input::Keys;
        using KeyboardState = Microsoft::Xna::Framework::Input::KeyboardState;
        using TC = InputPad::TypedCheat;
        const auto type = [](InputPad& pad, const std::string& word)
        {
            TC fired = TC::None;
            int firings = 0;
            for (const char c : word)
            {
                const auto key = static_cast<Keys>(static_cast<int>(Keys::A) + (c - 'a'));
                const auto result = pad.UpdateTypedGhostCheat(KeyboardState{key}, true);
                if (result.cheat != TC::None)
                {
                    fired = result.cheat;
                    ++firings;
                }
                (void)pad.UpdateTypedGhostCheat(KeyboardState{}, true);
            }
            return std::make_pair(fired, firings);
        };
        int count = 0;
        const auto* words = InputPad::TypedCheatWords(count);
        bool all = count == 48;
        for (int i = 0; i < count; ++i)
        {
            InputPad pad;
            const auto [fired, firings] = type(pad, words[i].word);
            const bool quickInside = words[i].cheat == TC::QuickLollipop; // "quick" fires on the way
            all = all && fired == words[i].cheat && firings == (quickInside ? 2 : 1);
            all = all && words[i].description != nullptr && words[i].description[0] != '\0';
        }
        check(all, "Typed cheats: all 48 words including restart/snap/here/trail/ids/sounds/chunks fire and have an explanation");
        bool distinct = true, origins = true;
        int pages[2]{};
        const std::unordered_set<std::string> galaxy{
            "help", "compass", "fairplay", "fps", "position", "honest", "back", "mark", "recall", "photo", "freeze", "freecam", "xray", "hitbox", "restart", "snap", "here", "trail", "ids", "sounds", "chunks"};
        const std::unordered_set<std::string> mobile{"quick", "ghost", "zoom", "debug", "cheats"};
        for (int i = 0; i < count; ++i)
        {
            const std::string word = words[i].word;
            ++pages[words[i].HelpPage()];
            origins &= (words[i].HelpPage() == 1) == galaxy.contains(word);
            origins &= (words[i].origin == InputPad::CheatOrigin::MobileEggbert) == mobile.contains(word);
            if (words[i].cheat != TC::Back && words[i].cheat != TC::Mark && words[i].cheat != TC::Recall &&
                words[i].cheat != TC::Photo && words[i].cheat != TC::Freeze && words[i].cheat != TC::Freecam &&
                words[i].cheat != TC::Xray && words[i].cheat != TC::Hitbox &&
                words[i].cheat != TC::Restart && words[i].cheat != TC::Snap && words[i].cheat != TC::Here &&
                words[i].cheat != TC::Trail && words[i].cheat != TC::Ids && words[i].cheat != TC::Sounds &&
                words[i].cheat != TC::Chunks) continue;
            for (int j = 0; j < count; ++j)
            {
                if (i == j) continue;
                const std::string other = words[j].word;
                distinct &= !other.starts_with(word) && !word.starts_with(other) &&
                    !other.ends_with(word) && !word.ends_with(other);
            }
        }
        bool helpFits = true;
        for (int page = 0; page < 2; ++page)
        {
            const auto layout = InputPad::LayoutCheatHelp(pages[page]);
            for (const int height : {480, 360})
                for (int i = 0; i < pages[page]; ++i)
                    helpFits &= (layout.RowTop(i) + layout.DescriptionOffset() + 32 * layout.DescriptionSize()) * height / 480
                              < 416.0f * height / 480;
        }
        check(helpFits, "cheat help descriptions end above page arrows at both required heights");
        check(distinct, "new cheats neither prefix nor share matched suffixes with another word");
        check(origins && pages[0] == 27 && pages[1] == 21,
              "every word belongs to exactly one origin/page; original and Mobile words fill page one");
        {
            InputPad reset;
            type(reset, "ba");
            reset.ResetMissionInput();
            check(type(reset, "ck").second == 0 && type(reset, "back").first == TC::Back,
                  "mission input reset clears a partial back word and its held-letter companions");
        }
        {
            InputPad xpad;
            check(type(xpad, "xnjttjpo").first == TC::XMission && type(xpad, "xmission").first == TC::None,
                  "Typed cheats: xnjttjpo (Speedy Blupi's XMISSION, each letter after the first one more) fires, "
                  "the plain word does not");
        }
        InputPad pad;
        check(type(pad, "givecopter").first == TC::GiveCopter && type(pad, "copter").first == TC::None,
              "Typed cheats: givecopter fires; the buffer is cleared after a word (\"copter\" alone is nothing)");
        check(type(pad, "xxjeepdrive").first == TC::JeepDrive, "Typed cheats: a word after other letters still fires");
        InputPad locked;
        locked.SetCheatEntryLocked(true);
        bool blocked=true;
        for (int i=0;i<count;++i)
            if (words[i].cheat!=TC::FairPlay)
                blocked &= type(locked,words[i].word).second==0;
        check(blocked && type(locked,"fairplay").first==TC::FairPlay,
              "fairplay blocks every other typed cheat including ghost/quick flags, but can always unlock itself");
        locked.ResetMissionInput();
        check(locked.IsCheatEntryLocked(),"fairplay lock survives mission input reset");
        locked.SetCheatEntryLocked(false);
        check(type(locked,"megablupi").first==TC::MegaBlupi && type(locked,"compass").first==TC::Compass &&
                  type(locked,"debug").first==TC::Debug,
              "unlocking fairplay permits other cheats again");
        InputPad run;
        check(type(run,"honest").first==TC::Honest,"Typed cheats: honest fires while cheats are open");
        run.LockCheatEntryForRun();
        bool runBlocked=true;
        for (int i=0;i<count;++i)
            runBlocked &= type(run,words[i].word).second==0;
        run.SetCheatEntryLocked(false);
        run.ResetMissionInput();
        check(runBlocked && run.IsCheatEntryLocked() && run.IsCheatEntryLockedForRun() &&
                  type(run,"fairplay").second==0 && type(run,"honest").second==0 && type(run,"debug").second==0,
              "honest: every word refused (fairplay and honest too) and the lock cannot be lifted");
        run.RestoreCheatEntryLock(false,false);
        check(type(run,"debug").first==TC::Debug,"a replay restores the recorded lock exactly");
    }

    {
        using namespace Microsoft::Xna::Framework::Input;
        InputPad pad;
        for (char c : std::string("weelkeys"))
        {
            const auto key = static_cast<Keys>(int(Keys::A) + c - 'a');
            const auto result = pad.UpdateTypedGhostCheat(KeyboardState{key}, true);
            if (c == 's')
            {
                check(result.cheat == InputPad::TypedCheat::WeelKeys &&
                          !pad.FilterGameplayKeyboard(KeyboardState{Keys::S}).IsKeyDown(Keys::S),
                      "typing weelkeys consumes its final S rather than turning Blupi around");
                (void)pad.UpdateTypedGhostCheat(KeyboardState{Keys::S}, true);
                check(!pad.FilterGameplayKeyboard(KeyboardState{Keys::S}).IsKeyDown(Keys::S),
                      "the cheat's final S stays suppressed until released");
            }
            (void)pad.UpdateTypedGhostCheat(KeyboardState{}, true);
        }
        (void)pad.UpdateTypedGhostCheat(KeyboardState{Keys::S}, true);
        check(pad.FilterGameplayKeyboard(KeyboardState{Keys::S}).IsKeyDown(Keys::S),
              "a fresh S press after the cheat still turns normally");
        InputPad slow;
        auto slowCheat=InputPad::TypedCheat::None;
        for (char c : std::string("weelkeys"))
        {
            const auto key=static_cast<Keys>(int(Keys::A)+c-'a');
            slowCheat=slow.UpdateTypedGhostCheat(KeyboardState{key},true).cheat;
            for (int tick=0;tick<120;++tick) (void)slow.UpdateTypedGhostCheat(KeyboardState{},true);
        }
        check(slowCheat==InputPad::TypedCheat::WeelKeys, "cheat recognition retains the original buffer even with two-second typing pauses");
        InputPad walking;
        for (int i=0; i<120; ++i) (void)walking.UpdateTypedGhostCheat(KeyboardState{Keys::W}, true);
        check(walking.FilterGameplayKeyboard(KeyboardState{Keys::W}).IsKeyDown(Keys::W),
              "ordinary held movement does not become cheat typing");
        int eventWordCount=0;
        const auto* eventWords=InputPad::TypedCheatWords(eventWordCount);
        for (int i=0;i<eventWordCount;++i)
        {
            InputPad eventPad;
            auto result=InputPad::TypedCheat::None;
            // Every physical key has already been released before the next Update.
            for (char c:std::string(eventWords[i].word))
                result=eventPad.UpdateTypedGhostCheat(KeyboardState{},true,c).cheat;
            check(result==eventWords[i].cheat, "ordered text events recognize a cheat without any held keys");
        }
        InputPad events;
        check(events.UpdateTypedGhostCheat(KeyboardState{Keys::G},true,0).cheat==InputPad::TypedCheat::None,
              "text mode does not also feed the polled keyboard into the cheat buffer");
        auto copter=InputPad::TypedCheat::None;
        for (char c:std::string("givecopter"))
            copter=events.UpdateTypedGhostCheat(KeyboardState{Keys::G,Keys::R},true,c).cheat;
        check(copter==InputPad::TypedCheat::GiveCopter,
              "ordered givecopter ignores simultaneous snapshot letters and repeated E is preserved");
        events.SetCheatEntryLocked(true);
        for (char c:std::string("givecopter"))
            copter=events.UpdateTypedGhostCheat(KeyboardState{},true,c).cheat;
        check(copter==InputPad::TypedCheat::None,"fairplay also blocks queued text cheats");
        for (char c:std::string("fairplay"))
            copter=events.UpdateTypedGhostCheat(KeyboardState{},true,c).cheat;
        check(copter==InputPad::TypedCheat::FairPlay,"queued text can still unlock fairplay");
    }
    }
    else
    {
        using namespace Microsoft::Xna::Framework::Input;
        InputPad pad;
        int count=1;
        (void)InputPad::TypedCheatWords(count);
        bool none=count==0;
        for(char c:std::string("ghostcompasshelpweelkeysfairplaydebug"))
        {
            const auto key=static_cast<Keys>(int(Keys::A)+c-'a');
            none &= pad.UpdateTypedGhostCheat(KeyboardState{key},true).cheat==InputPad::TypedCheat::None;
            (void)pad.UpdateTypedGhostCheat(KeyboardState{},true);
        }
        check(none, "cheats-OFF build exposes and recognizes no typed cheats");
        for(char c:std::string("givecopterfairplaydebug"))
            none &= pad.UpdateTypedGhostCheat(KeyboardState{},true,c).cheat==InputPad::TypedCheat::None;
        check(none,"cheats-OFF build also ignores ordered text events");
        check(!pad.UpdateCheatGesture(mouse(70,200,true),640,480) &&
                  pad.UpdateCheatMenu(mouse(50,240,false),640,480)==0,
              "cheats-OFF build cannot unlock or activate the hidden cheat menu");
        check(pad.FilterGameplayKeyboard(KeyboardState{Keys::S}).IsKeyDown(Keys::S),
              "cheats-OFF build leaves normal movement keys alone");
    }
    {
        DebugOverlayInfo info;
        info.ticks=1250; info.mission=20; info.region=3; info.lives=7;
        info.position={49.49f,1.5f,-.51f}; info.velocity={-2.5f,0,1.25f};
        info.yawDegrees=-90; info.speed=.5f; info.mode="air heli";
        info.keys=2; info.touches=3; info.gamepad=true;
        const auto lines=DebugOverlayLines(info);
        const auto has=[&](const char* value) { return std::find(lines.begin(),lines.end(),value)!=lines.end(); };
        check(has("time: 01:02 (1250 ticks)"),"debug time uses the world's 20Hz clock");
        check(has("pos: 49.49 1.50 -0.51") && has("cell: 49 2 -1"),
              "debug exposes all three raw coordinates and centre-based cell boundaries");
        check(has("vel: -2.50 0.00 1.25 blocks/s") && has("yaw: 270.0 deg") && has("speed: 0.5x"),
              "debug preserves three velocity components, wraps yaw and labels slow speed accurately");
        check(has("mode: air heli") && has("keys: 2  touches: 3") && has("gamepad: on  accel: off"),
              "debug reports current modes and input devices");
    }
    {
        // The keyboard on the screen (2026-11-10): five rows, Shift for a capital at the start,
        // Del, OK and Cancel; the arrows' navigator over its keys.
        using GalaxyEggbert::Game::ScreenKeyboard;
        using GalaxyEggbert::Game::FocusNavigator;
        using Microsoft::Xna::Framework::Input::Keys;
        ScreenKeyboard keyboard;
        keyboard.Layout({100.0f, 118.0f, 700.0f, 372.0f});
        check(keyboard.Keys().size() == 42, "the screen's keyboard has 42 keys (10+10+10+9+3)");
        const auto keyAt = [&](char c) {
            for (std::size_t i = 0; i < keyboard.Keys().size(); ++i)
                if (keyboard.Keys()[i].kind == ScreenKeyboard::Kind::Char && keyboard.Keys()[i].c == c) return static_cast<int>(i);
            return -1;
        };
        const auto kindAt = [&](ScreenKeyboard::Kind kind) {
            for (std::size_t i = 0; i < keyboard.Keys().size(); ++i)
                if (keyboard.Keys()[i].kind == kind) return static_cast<int>(i);
            return -1;
        };
        std::string text;
        keyboard.Begin(text);
        keyboard.Press(keyAt('s'), text, 24);
        keyboard.Press(keyAt('k'), text, 24);
        keyboard.Press(kindAt(ScreenKeyboard::Kind::Space), text, 24);
        keyboard.Press(keyAt('7'), text, 24);
        check(text == "Sk 7", "Shift is on at the start, a letter turns it off: Sk 7");
        keyboard.Press(kindAt(ScreenKeyboard::Kind::Backspace), text, 24);
        check(text == "Sk ", "Del deletes the last character");
        check(keyboard.Press(kindAt(ScreenKeyboard::Kind::Done), text, 24) == ScreenKeyboard::Result::Done &&
                  keyboard.Press(kindAt(ScreenKeyboard::Kind::Cancel), text, 24) == ScreenKeyboard::Result::Cancel,
              "OK is Done, Cancel is Cancel");
        std::string full(24, 'a');
        keyboard.Press(keyAt('b'), full, 24);
        check(full.size() == 24, "a full name takes no more");
        const auto& r = keyboard.Keys()[static_cast<std::size_t>(keyAt('g'))].rect;
        check(keyboard.KeyAt((r.x0 + r.x1) * 0.5f, (r.y0 + r.y1) * 0.5f) == keyAt('g') && keyboard.KeyAt(10, 10) < 0 &&
                  !keyboard.Holds(10, 10),
              "a key is found under its centre, nothing off the keyboard");
        std::string typed;
        ScreenKeyboard::TypeKey(Keys::R, true, typed, 24);
        ScreenKeyboard::TypeKey(Keys::D7, false, typed, 24);
        ScreenKeyboard::TypeKey(Keys::OemMinus, false, typed, 24);
        check(typed == "R7-" && ScreenKeyboard::TypeKey(Keys::Enter, false, typed, 24) == ScreenKeyboard::Result::Done,
              "real keys type letters, digits and '-'; Enter is Done");

        const auto targets = keyboard.Rects();
        const int space = kindAt(ScreenKeyboard::Kind::Space), ok = kindAt(ScreenKeyboard::Kind::Done);
        check(FocusNavigator::Neighbour(targets, space, 1, 0) == ok,
              "Right from Space is OK in the same row, not a letter just above it");
        check(FocusNavigator::Neighbour(targets, keyAt('y'), 0, 1) == keyAt('h') &&
                  FocusNavigator::Neighbour(targets, keyAt('h'), 0, -1) == keyAt('y'),
              "Down and Up keep the column");
        check(FocusNavigator::Neighbour(targets, keyAt('1'), -1, 0) == keyAt('1'), "no key left of the first: it stays");
        FocusNavigator navigator;
        FocusNavigator::Input in;
        in.right = true;
        auto out = navigator.Update(targets, in);
        check(out.active && out.focused == 0 && !out.pressed, "the first arrow only shows the choice, on the first target");
        out = navigator.Update(targets, in);
        check(out.focused == 1, "the next one moves it");
        in = {};
        in.confirm = true;
        out = navigator.Update(targets, in);
        const auto& two = targets[1];
        check(out.pressed && out.x == (two.x0 + two.x1) * 0.5f, "A presses the chosen key at its centre");
        out = navigator.Update(targets, FocusNavigator::Input{});
        check(out.active && !out.pressed, "and lets it go the next update");
        in = {};
        in.pointer = true;
        out = navigator.Update(targets, in);
        check(!out.active && !navigator.Active(), "the mouse moved: the choice is gone, the real mouse again");
    }
    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
