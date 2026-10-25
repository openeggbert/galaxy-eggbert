#include "InputPad.hpp"
#include "TextMetrics.hpp"

#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

namespace GalaxyEggbert::Game
{
    namespace
    {
        constexpr float kRefW = 640.0f;
        constexpr float kRefH = 480.0f;

        // pad.png: 140px cells, 8 columns (sheet 1120x420) -- same
        // convention Hud.cpp's own kPadCellPx/kPadCols already documents
        // (Pixmap.cpp's real PixmapChannel::Pad case).
        constexpr float kPadCellPx = 140.0f;
        constexpr int kPadCols = 8;

        // Real icon indices confirmed against InputPad.cpp / Pixmap.cpp
        // (see this session's research notes): D-pad ring=0, thumb=1,
        // PlayJump=2, PlayPause=3, PlayAction=12; PauseBack=8,
        // PauseRestart=9, PauseContinue=10, PauseMenu=11, PauseSetup=19.
        constexpr int kIconDPadRing = 0;
        constexpr int kIconDPadThumb = 1;
        constexpr int kIconPlayJump = 2;
        constexpr int kIconPlayPause = 3;
        constexpr int kIconPlayAction = 12;
        constexpr int kIconPauseBack = 8;
        constexpr int kIconPauseRestart = 9;
        constexpr int kIconPauseContinue = 10;
        constexpr int kIconPauseMenu = 11;
        constexpr int kIconPauseSetup = 19;

        // Real "pressed" visual is the same icon at reduced opacity, not
        // an icon swap -- 0.8 for menu-style (Pause row) buttons, 0.6 for
        // Play action buttons (this session's InputPad research notes).
        constexpr float kPausePressedAlpha = 0.8f;
        constexpr float kPlayPressedAlpha = 0.6f;

        // Real DrawInfo/Init panel opacity is 0.6 (same value as Hud.cpp's
        // own kPanelOpacity and the same real panel icon).
        constexpr float kInitPanelOpacity = 0.3f; // real DrawButtonsBackground() opacity

        // Play on-screen control layout (proportionally adapted into the
        // existing 640x480 reference space -- see InputPad.hpp's class
        // comment for why a literal drawBounds-relative port isn't
        // possible). D-pad hit-square half-extent 70 happens to equal the
        // real source's own 140px hit-radius/2 coincidentally (both are
        // "70" in their respective unit systems, not a derived value).
        // Rect/InRect (plan.md EDITOR-106): the shared QuadBatch hit-testing
        // primitive, also used by the editor UI.
        using Rect = QuadBatch::Rect;
        using QuadBatch::InRect;

        // Real InputPad::GetButtonRect() positions every button in SCREEN
        // pixels, from buttonSizeFactor1 = height / 5 and buttonSizeFactor2 =
        // height * 140 / 480: the Play controls and Setup against the screen's
        // edges, Pause/Resume/WinLostReturn from the 640x480 image's origin.
        // These were approximations in the centred 640x480 space until
        // 2026-10-25 (90-pixel Pause buttons from x 55, the Play controls 70-80
        // pixels inside the screen's corners on an 800-wide window); every rect
        // below is now the original's, in screen pixels, scaled with the
        // window's height like the rest of this class.
        struct ScreenMetrics
        {
            float w, h, s, f1, f2, originX;
        };

        ScreenMetrics Metrics(int viewportW, int viewportH) noexcept
        {
            const float w = static_cast<float>(viewportW);
            const float h = static_cast<float>(viewportH);
            const float s = h / kRefH;
            return ScreenMetrics{w, h, s, h / 5.0f, h * 140.0f / 480.0f, (w - kRefW * s) * 0.5f};
        }

        // Play (the original's default "jump button on the right" layout): the
        // stick ring at (100, height-100), touch area GetPadBounds(center, 140),
        // ring and knob drawn GetPadBounds(center, 70); Jump and Action in the
        // bottom-right corner; Pause in the top-right one.
        constexpr float kDPadRadius = 140.0f;      // real padRadius: the touch area's half size
        constexpr float kDPadDrawRadius = 70.0f;   // real padRadius / 2: ring and knob
        constexpr float kDPadThreshold = 20.0f;    // pixels per axis for the discrete -1/0/+1 read

        Microsoft::Xna::Framework::Vector2 DPadCenter(const ScreenMetrics& m) noexcept
        {
            return {100.0f * m.s, m.h - 100.0f * m.s};
        }

        Rect PlayJumpRect(const ScreenMetrics& m) noexcept
        {
            return Rect{m.w - m.f1 * 1.2f, m.h - m.f1 * 1.2f, m.w - m.f1 * 0.2f, m.h - m.f1 * 0.2f};
        }

        Rect PlayActionRect(const ScreenMetrics& m) noexcept
        {
            return Rect{m.w - m.f1 * 1.2f, m.h - m.f1 * 2.6f, m.w - m.f1 * 0.2f, m.h - m.f1 * 1.6f};
        }

        Rect PlayPauseRect(const ScreenMetrics& m) noexcept
        {
            return Rect{m.w - m.f1 * 0.7f, m.f1 * 0.2f, m.w - m.f1 * 0.2f, m.f1 * 0.7f};
        }

        // Pause: Menu, Back, Setup, Restart, Continue, one buttonSizeFactor2
        // wide each, from the image origin's x - 0.21 * buttonSizeFactor2.
        Rect PauseButtonRect(int index, const ScreenMetrics& m) noexcept
        {
            const float x0 = m.originX + m.f2 * (-0.21f + static_cast<float>(index));
            return Rect{x0, m.f2 * 2.2f, x0 + m.f2, m.f2 * 3.2f};
        }

        // Resume: Menu and Continue, from x + 1.29 * buttonSizeFactor2.
        Rect ResumeButtonRect(int index, const ScreenMetrics& m) noexcept
        {
            const float x0 = m.originX + m.f2 * (1.29f + static_cast<float>(index));
            return Rect{x0, m.f2 * 2.2f, x0 + m.f2, m.f2 * 3.2f};
        }

        Rect WinLostReturnRect(const ScreenMetrics& m) noexcept
        {
            return Rect{m.originX + m.w - m.f1 * 2.2f, m.f1 * 0.2f, m.originX + m.w - m.f1 * 1.2f, m.f1 * 1.2f};
        }

        // Setup: Sounds/Jump/Zoom/Accel stacked in the left column, Reset at
        // x 450 on Sounds' row, Return in the bottom-right corner.
        Rect SetupColumnRect(float top, float bottom, const ScreenMetrics& m) noexcept
        {
            return Rect{20.0f * m.s, m.h - 20.0f * m.s - m.f2 * top, 20.0f * m.s + m.f2 * 0.5f,
                        m.h - 20.0f * m.s - m.f2 * bottom};
        }
        Rect SetupSoundsRect(const ScreenMetrics& m) noexcept { return SetupColumnRect(2.0f, 1.5f, m); }
        Rect SetupJumpRect(const ScreenMetrics& m) noexcept { return SetupColumnRect(1.5f, 1.0f, m); }
        Rect SetupZoomRect(const ScreenMetrics& m) noexcept { return SetupColumnRect(1.0f, 0.5f, m); }
        Rect SetupAccelRect(const ScreenMetrics& m) noexcept { return SetupColumnRect(0.5f, 0.0f, m); }
        Rect SetupResetRect(const ScreenMetrics& m) noexcept
        {
            const Rect row = SetupSoundsRect(m);
            return Rect{450.0f * m.s, row.y0, 450.0f * m.s + m.f2 * 0.5f, row.y1};
        }
        Rect SetupReturnRect(const ScreenMetrics& m) noexcept
        {
            return Rect{m.w - 20.0f * m.s - m.f2 * 0.8f, m.h - 20.0f * m.s - m.f2 * 0.8f, m.w - 20.0f * m.s,
                        m.h - 20.0f * m.s};
        }

        // Real Pause background/character (pause.png, blupiyoupie.png):
        // pause.png is an exact 640x480 match for the reference space;
        // blupiyoupie.png (410x380) is centered at real position (418,190).
        constexpr float kCharacterCenterX = 418.0f, kCharacterCenterY = 190.0f;

        // Win/Lost screens (plan.md MENU-046..057), verified directly
        // against `Game1.cpp`'s real `Draw()` phase branches: both center
        // blupiyoupie.png at real (418,238) -- a DIFFERENT Y than Pause's
        // 190 above -- scaling its native 410x380 half-size (205,190) by
        // `num`. Win: `num = sin(phaseTime/ScaleTime(3))/2+1`, i.e.
        // `sin(t/0.15s)/2+1` at the real 20fps base rate this class's
        // phaseTimeSeconds parameter is expressed in -- a perpetual pulse
        // between 0.5x/1.5x native size, no rotation. Lost: `num =
        // min(phaseTime/ScaleTime(100),1)` = `min(t/5s,1)` -- grows from
        // nothing to native size once over 5s, with a decaying spin
        // (`rotation = (1-num)^2 * 360*6` degrees while num<1, converging
        // to 0 rotation exactly as it reaches full size).
        constexpr float kWinLostCharacterCenterY = 238.0f;
        constexpr float kWinPulsePeriodSeconds = 0.15f;
        constexpr float kLostGrowDurationSeconds = 5.0f;
        constexpr float kLostSpinMaxDegrees = 360.0f * 6.0f;

        // text.png: 32px glyph cells, 16 columns, glyph index == ASCII
        // code for the printable range (same convention Hud.cpp already
        // documents); the advances are the real proportional widths
        // (TextMetrics.hpp).
        constexpr float kGlyphCellPx = 32.0f;
        constexpr int kGlyphCols = 16;
        constexpr float kPauseLabelScale = 0.7f; // real DrawTextUnderButton() scale
        constexpr float kPauseLabelYOffset = 2.0f; // real "buttonRect.Bottom + 2"
        constexpr float kSetupLabelScale = 0.7f; // real DrawTextRightButton() scale
        constexpr float kSetupLabelXOffset = 10.0f; // real "buttonRect.Right + 10"
        constexpr float kSetupLabelYNudge = 8.0f; // real "(Top+Bottom)/2 - 8" single-line case

        // Real icon indices (Pixmap.cpp): Sounds/Jump/Zoom/Accel really
        // SWAP icon 13 (selected/on) vs 21 (not selected/off) -- a
        // DIFFERENT "state" convention from every other button in this
        // class (which only ever change opacity, never icon, when
        // pressed). Reset=20, Return=8 (same value as PauseBack's icon,
        // but a distinct named constant since it's a different real
        // button that happens to share an icon).
        constexpr int kIconSetupToggleOn = 13;
        constexpr int kIconSetupToggleOff = 21;
        constexpr int kIconSetupReset = 20;
        constexpr int kIconSetupReturn = 8;

        // Logical control indices, local to this file -- Play and Pause
        // reuse the same activeControl_ member but are never both live in
        // the same frame (see InputPad.hpp's ResetTouchState() comment).
        constexpr int kPlayControlDPad = 0;
        constexpr int kPlayControlJump = 1;
        constexpr int kPlayControlAction = 2;
        constexpr int kPlayControlPause = 3;

        constexpr int kPauseControlMenu = 0;
        constexpr int kPauseControlBack = 1;
        constexpr int kPauseControlSetup = 2;
        constexpr int kPauseControlRestart = 3;
        constexpr int kPauseControlContinue = 4;

        constexpr int kWinLostControlReturn = 0;

        constexpr int kSetupControlSounds = 0;
        constexpr int kSetupControlJump = 1;
        constexpr int kSetupControlZoom = 2;
        constexpr int kSetupControlAccel = 3;
        constexpr int kSetupControlReset = 4;
        constexpr int kSetupControlReturn = 5;

        constexpr int kResumeControlMenu = 0;
        constexpr int kResumeControlContinue = 1;

        // Cheat gesture zones (plan.md CHEAT-001..009): real is a 3-col x
        // 2-row grid of 6 INVISIBLE zones spanning the top-left ~2/3
        // width x ~57% height of the screen (`InputPad.cpp`'s own real
        // `GetButtonRect()` cases for `Cheat11..Cheat32`, real
        // `cheatButtonSizeFactor = drawBoundsHeight/3.5`). Zone naming:
        // first digit = column (1-3), second digit = row (1-2) --
        // matches the real `Cheat11/12/21/22/31/32` glyph names read as
        // (col,row). Ported here directly in reference-space fractions of
        // 640x480 (no drawBounds-relative formula needed, unlike every
        // other screen this session -- these zones are invisible, so
        // there's no real pixel size to match visually, only real
        // relative coverage).
        constexpr float kGestureAreaW = kRefW * 2.0f / 3.0f;
        constexpr float kGestureAreaH = kRefH * 0.57f;
        constexpr float kGestureCellW = kGestureAreaW / 3.0f;
        constexpr float kGestureCellH = kGestureAreaH / 2.0f;

        // Real 10-tap sequence (`Game1.hpp`'s own `cheatGesteLength=10`
        // constant and its real tap-order array -- an earlier plan.md
        // draft said 6 taps, which was wrong, confusing the tap-COUNT
        // with the 6 distinct ZONE names).
        constexpr int kCheatGestureLength = 10;
        constexpr int kCheatGestureSequence[kCheatGestureLength] = {12, 22, 32, 12, 11, 21, 22, 21, 31, 32};

        // Cheat menu overlay: real is a row of nine 80x80 ABSOLUTE-pixel
        // boxes at the literal top-left (`InputPad.cpp` special-cases
        // this range before its normal per-button switch -- a genuine
        // real inconsistency vs. every other button in the game, which
        // all use a drawBounds-relative formula). 9*80=720 exceeds even
        // this engine's 640-wide reference space, so (same situation as
        // the Pause row) this spans the full reference width in 9 equal
        // columns instead.
        constexpr int kCheatButtonCount = 9;
        constexpr float kCheatButtonW = kRefW / static_cast<float>(kCheatButtonCount);
        constexpr float kCheatButtonH = kCheatButtonW; // real boxes are square
        constexpr float kCheatLabelYOffset = 2.0f; // same real "Bottom + 2" convention as the Pause row

        // Real single-letter labels (`Decor::GetCheatTinyText()`) --
        // cheats 6/8 really do share "T" (Trial/Treasure) in the real
        // source, not a transcription mistake here.
        constexpr const char* kCheatLetters[kCheatButtonCount] = {"D", "B", "S", "E", "R", "T", "C", "T", "G"};

        // Wait phase (plan.md MENU-001..005): real jauge.png progress
        // gauge, position (196,426), zoom 2.0, mode Yellow. Sheet is
        // 124x88 (4 rows of 22px: row 0 = empty-gauge background, rows
        // 1-3 = Red/Blue/Yellow fill -- confirmed directly against real
        // `Jauge::Draw()`). Real fill formula: `filledWidth =
        // level*114/100`, fill rect X spans [0, 6+filledWidth]. `level`
        // (0-100) comes from the real NON-LINEAR `waitTable` lookup curve
        // (`Game1.hpp`), picking the first entry whose threshold >= the
        // elapsed-time fraction -- NOT a linear ramp. Real minimum
        // duration is a fixed 5.0s wall-clock timer (`Game1.cpp`'s real
        // `waitProgress = ticks/50,000,000`, decoupled from actual asset
        // loading, which already finished synchronously one frame
        // earlier in this engine's own `LoadContent()`).
        constexpr float kJaugePosX = 196.0f, kJaugePosY = 426.0f;
        constexpr float kJaugeZoom = 2.0f;
        constexpr float kJaugeCellW = 124.0f, kJaugeCellH = 22.0f;
        constexpr int kJaugeModeYellow = 3;
        constexpr float kWaitDurationSeconds = 5.0f;
        struct WaitTableEntry
        {
            float threshold;
            int level;
        };
        constexpr WaitTableEntry kWaitTable[12] = {
            {0.10f, 7}, {0.20f, 20}, {0.25f, 22}, {0.45f, 50}, {0.60f, 53}, {0.65f, 58},
            {0.68f, 60}, {0.80f, 70}, {0.84f, 75}, {0.90f, 84}, {0.94f, 91}, {1.00f, 100},
        };

        // Init phase / gamer-select menu (plan.md MENU-006..020): real
        // speedyblupi.png title logo (640x160) -- Left/Right FIXED at
        // 80/720, only Top/Bottom animate during the real 1.0s entry
        // slide (`num=1-(1-t)^2`, confirmed via research this is
        // VERTICAL, not the horizontal "slides in from the right" an
        // earlier doc-comment in the real source itself incorrectly
        // claims). Real blupiyoupie.png entry: centered at real (468,280)
        // -- different from Pause's (418,190)/WinLost's (418,238) --
        // scaling 50%->100% (`num=0.5+t/2`) while fading in 0.25->1.0
        // opacity (`min(num^2,1)`), no rotation. Real exit-fade
        // animations are NOT ported (every phase transition in this
        // engine is instant, plan.md MENU-088/089).
        constexpr float kInitTitleLeft = 80.0f, kInitTitleRight = 720.0f;
        constexpr float kInitEntryDurationSeconds = 1.0f;
        constexpr float kInitCharacterCenterX = 468.0f, kInitCharacterCenterY = 280.0f;

        // Real Init layout (InputPad::GetButtonRect() and
        // Game1::DrawButtonsBackground()): unlike the 640x480 backgrounds,
        // the buttons stand against the SCREEN's edges -- the gamer column 20
        // pixels from its left, Play 20 from its right -- with
        // buttonSizeFactor2 = height * 140 / 480 (70-pixel gamer/Setup
        // buttons, a 140-pixel Play button), over two pad.png icon-15 panels
        // at 0.3 opacity. They were laid out in the centred 640x480 space
        // until 2026-10-25, 80 pixels right of the original on an 800-wide
        // window. The 3D world editor's button (not a mobile-eggbert button,
        // plan.md EDITOR-107) takes the slot the original gives
        // InitBuy/InitRanking, above Play, with the right panel at the
        // height the original uses when that slot is shown.
        struct InitLayout
        {
            Rect gamer[3];
            Rect setup, play, editor;
            Rect leftPanel, rightPanel;
            float scale;
        };

        InitLayout ComputeInitLayout(int viewportW, int viewportH) noexcept
        {
            const float w = static_cast<float>(viewportW);
            const float h = static_cast<float>(viewportH);
            const float s = h / kRefH;
            const float f2 = 140.0f * s;
            const float left0 = 20.0f * s;
            const float left1 = left0 + f2 * 0.5f;
            const auto column = [&](float top, float bottom)
            {
                return Rect{left0, h - 20.0f * s - f2 * top, left1, h - 20.0f * s - f2 * bottom};
            };
            InitLayout l{};
            l.gamer[0] = column(2.1f, 1.6f);
            l.gamer[1] = column(1.6f, 1.1f);
            l.gamer[2] = column(1.1f, 0.6f);
            l.setup = column(0.5f, 0.0f);
            l.play = Rect{w - 20.0f * s - f2, h - 40.0f * s - f2, w - 20.0f * s, h - 40.0f * s};
            l.editor = Rect{w - 20.0f * s - f2 * 0.75f, h - 20.0f * s - f2 * 2.1f,
                            w - 20.0f * s - f2 * 0.25f, h - 20.0f * s - f2 * 1.6f};
            l.leftPanel = Rect{10.0f * s, h - 325.0f * s, 260.0f * s, h - 10.0f * s};
            l.rightPanel = Rect{w - 170.0f * s, h - 325.0f * s, w - 10.0f * s, h - 10.0f * s};
            l.scale = s;
            return l;
        }

        constexpr int kInitPanelIcon = 15; // pad.png, same convention as Hud's DrawInfo panel

        // Real icon indices (`Pixmap.cpp`'s pad.png dispatch): GamerA=4/16
        // (unselected/selected), GamerB=5/17, GamerC=6/18, InitSetup=19
        // (same icon as PauseSetup), InitPlay=7. InitRanking/InitBuy are
        // NOT ported: real visibility gate (`getIsTrialModeProperty()`/
        // `getIsRankingModeProperty()`) resolves to "never shown by
        // default" in this port (hardcoded false / QA-cheat-only,
        // confirmed via research) -- same "unreachable in this port"
        // precedent already established for the Trial phase itself.
        constexpr int kIconInitGamerAOff = 4, kIconInitGamerASel = 16;
        constexpr int kIconInitGamerBOff = 5, kIconInitGamerBSel = 17;
        constexpr int kIconInitGamerCOff = 6, kIconInitGamerCSel = 18;
        constexpr int kIconInitSetup = 19;
        constexpr int kIconInitPlay = 7;
        // The editor's button (plan.md EDITOR-107): pad.png's blank button
        // face (icon 21) with the Eggbert editor's own tools icon (button.png
        // icon 55, the crossed wrench and screwdriver) on it, labelled
        // "3D World Editor". It was pad.png's X (icon 14) until 2026-10-25,
        // which read as "close" (owner).
        constexpr int kIconInitEditorFace = 21;
        constexpr int kButtonIconInitEditor = 55;
        constexpr float kButtonCellPx = 40.0f; // button.png: 40-pixel cells, 6 columns
        constexpr int kButtonCols = 6;
        constexpr float kInitEditorIconFraction = 0.6f;

        // Real per-slot text (`Game1::DrawButtonGamerText()`/
        // `MyResource`): "Player {letter}" (scale 0.7) + "Main gates :
        // {n}/12" + "Secondary gates : {n}/52" + "Blupi : {lives}" (scale
        // 0.45), offset from the button's own top-right corner. The gate
        // counts are the gamer's own (GameData::GetGamerInfo(): doors 0-179
        // secondary, 180-199 main; SaveData::GetGateCountsForGamer()).
        constexpr float kGamerTitleScale = 0.7f;
        constexpr float kGamerBodyScale = 0.45f;
        constexpr float kGamerTextXOffset = 5.0f;
        constexpr float kGamerTitleYOffset = 3.0f;
        constexpr float kGamerMDoorsYOffset = 25.0f;
        constexpr float kGamerSDoorsYOffset = 39.0f;
        constexpr float kGamerLivesYOffset = 53.0f;

        constexpr int kInitControlGamerA = 0;
        constexpr int kInitControlGamerB = 1;
        constexpr int kInitControlGamerC = 2;
        constexpr int kInitControlSetup = 3;
        constexpr int kInitControlPlay = 4;
        constexpr int kInitControlEditor = 5;

        // Real fade-out transitions (plan.md MENU-088/089, 2026-07-13,
        // dedicated research pass into the real `fadeOutPhase` mechanic):
        // the generic real commit timer is `Config::ScaleTime(20)` = 1.0s
        // at this build's pinned 20fps -- matches this engine's own
        // kWaitDurationSeconds-style cross-file convention (owned here,
        // GalaxyEggbertGame.cpp's own copy drives the actual phase
        // commit). Pause/Resume's own entrance flourish is a SEPARATE,
        // shorter real 0.75s (`ScaleTime(15)`).
        constexpr float kFadeDurationSeconds = 1.0f;
        constexpr float kPauseEntryDurationSeconds = 0.75f;

        // Real MainSetup/PlaySetup gear.png decorations (native 226x226,
        // confirmed via research): two FIXED real rects, a genuine
        // intentional size asymmetry (gear2 is literally 2x native size),
        // not a mistake.
        constexpr float kGear1CenterX = 600.0f, kGear1CenterY = 261.0f, kGear1Half = 113.0f; // (487,148)-(713,374)
        constexpr float kGear2CenterX = 344.0f, kGear2CenterY = 494.0f, kGear2Half = 226.0f; // (118,268)-(570,720)

        // AppendQuadUv (plan.md EDITOR-106): moved to the shared QuadBatch.
        using QuadBatch::AppendQuadUv;

        void PadIconUv(int icon, float sheetW, float sheetH, float& u0, float& v0, float& u1, float& v1)
        {
            const int col = icon % kPadCols;
            const int row = icon / kPadCols;
            u0 = (static_cast<float>(col) * kPadCellPx) / sheetW;
            v0 = (static_cast<float>(row) * kPadCellPx) / sheetH;
            u1 = (static_cast<float>(col + 1) * kPadCellPx) / sheetW;
            v1 = (static_cast<float>(row + 1) * kPadCellPx) / sheetH;
        }

        // Real Lost-screen spin: the sprite rotates about its own rect
        // CENTER (confirmed via mobile-eggbert's `Misc::RotateAdjust` --
        // it compensates SpriteBatch's real top-left rotation origin by
        // shifting the rect so the visual pivot lands on the center
        // instead). Builds one standalone quad directly rather than going
        // through the axis-aligned Quad/FlushQuads path above, since a
        // rotated quad's 4 corners aren't expressible as a single (x0,y0)-
        // (x1,y1) rect. Positive rotationDegrees is clockwise on screen
        // (standard XNA SpriteBatch convention in this Y-down space).
        // AppendRotatedQuadUv (plan.md EDITOR-106): moved to the shared QuadBatch.
        using QuadBatch::AppendRotatedQuadUv;

        // Real Pixmap::DrawBackground(): before the centred 640x480 image the
        // original stretches the image's top-left 10x10 corner
        // (`GetSrcRectangle(bitmap, 10, 10, 10, 10, 0, 0)`) over the whole
        // viewport, so the bars beside a 4:3 image on a wider screen take the
        // image's own edge colour instead of showing what lies under it (the
        // paused 3D world, user report 2026-10-25). Every background drawn
        // here is 640x480, the reference space.
        QuadBatch::Quad BackgroundBarsQuad(int viewportW, int viewportH)
        {
            QuadBatch::Quad q;
            q.x0 = 0.0f;
            q.y0 = 0.0f;
            q.x1 = static_cast<float>(viewportW);
            q.y1 = static_cast<float>(viewportH);
            q.u0 = 0.0f;
            q.v0 = 0.0f;
            q.u1 = 10.0f / kRefW;
            q.v1 = 10.0f / kRefH;
            return q;
        }
    }

    void InputPad::LoadContent(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        using Microsoft::Xna::Framework::Graphics::BasicEffect;
        using Microsoft::Xna::Framework::Graphics::Texture2D;

        const char* kPaths[13] = {
            "Content/icons/pad.png",
            "Content/backgrounds/pause.png",
            "Content/backgrounds/blupiyoupie.png",
            "Content/backgrounds/win.png",
            "Content/backgrounds/lost.png",
            "Content/backgrounds/setup.png",
            "Content/icons/text.png",
            "Content/backgrounds/wait.png",
            "Content/icons/jauge.png",
            "Content/backgrounds/init.png",
            "Content/backgrounds/speedyblupi.png",
            "Content/backgrounds/gear.png",
            "Content/icons/button.png",
        };
        for (const char* path : kPaths)
        {
            if (!std::filesystem::exists(path))
            {
                std::printf("InputPad: %s not found -- on-screen controls disabled.\n", path);
                return;
            }
        }

        padTexture_ = Texture2D(kPaths[0], device);
        pauseBgTexture_ = Texture2D(kPaths[1], device);
        blupiyoupieTexture_ = Texture2D(kPaths[2], device);
        winBgTexture_ = Texture2D(kPaths[3], device);
        lostBgTexture_ = Texture2D(kPaths[4], device);
        setupBgTexture_ = Texture2D(kPaths[5], device);
        textTexture_ = Texture2D(kPaths[6], device);
        waitBgTexture_ = Texture2D(kPaths[7], device);
        jaugeTexture_ = Texture2D(kPaths[8], device);
        initBgTexture_ = Texture2D(kPaths[9], device);
        speedyblupiTexture_ = Texture2D(kPaths[10], device);
        gearTexture_ = Texture2D(kPaths[11], device);
        buttonTexture_ = Texture2D(kPaths[12], device);

        const auto makeEffect = [&device](Texture2D& texture)
        {
            auto effect = std::make_unique<BasicEffect>(device);
            effect->VertexColorEnabled = false;
            effect->setTextureEnabledProperty(true);
            effect->setTextureProperty(&texture);
            return effect;
        };
        padEffect_ = makeEffect(padTexture_);
        pauseBgEffect_ = makeEffect(pauseBgTexture_);
        blupiyoupieEffect_ = makeEffect(blupiyoupieTexture_);
        winBgEffect_ = makeEffect(winBgTexture_);
        lostBgEffect_ = makeEffect(lostBgTexture_);
        setupBgEffect_ = makeEffect(setupBgTexture_);
        textEffect_ = makeEffect(textTexture_);
        waitBgEffect_ = makeEffect(waitBgTexture_);
        jaugeEffect_ = makeEffect(jaugeTexture_);
        initBgEffect_ = makeEffect(initBgTexture_);
        speedyblupiEffect_ = makeEffect(speedyblupiTexture_);
        gearEffect_ = makeEffect(gearTexture_);
        buttonEffect_ = makeEffect(buttonTexture_);
        loaded_ = true;
    }

    void InputPad::ResetTouchState() noexcept
    {
        activeControl_ = -1;
        mouseWasDown_ = false;
        dpadDragOffsetX_ = 0.0f;
        dpadDragOffsetY_ = 0.0f;
    }

    void InputPad::FlushQuads(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                Microsoft::Xna::Framework::Graphics::BasicEffect& effect,
                                std::unique_ptr<CnaEngine::Graphics::GpuMesh>& renderer,
                                const std::vector<Quad>& quads, int viewportW, int viewportH,
                                float alpha)
    {
        QuadBatch::FlushQuads(device, effect, renderer, quads, viewportW, viewportH, alpha);
    }

    void InputPad::AppendCenteredLabel(std::vector<Quad>& quads, const std::string& text,
                                        float centerX, float topY, float viewportScale) const
    {
        if (text.empty())
        {
            return;
        }
        const float textSheetW = static_cast<float>(textTexture_.getWidthProperty());
        const float textSheetH = static_cast<float>(textTexture_.getHeightProperty());
        const float glyphScale = kPauseLabelScale * viewportScale;
        const float cellPx = kGlyphCellPx * glyphScale;
        float penX = centerX - TextMetrics::Width(text) * glyphScale * 0.5f;
        for (const char c : text)
        {
            const int rank = TextMetrics::GlyphIndex(c);
            const int gcol = rank % kGlyphCols;
            const int grow = rank / kGlyphCols;
            Quad q;
            q.x0 = penX;
            q.y0 = topY;
            q.x1 = penX + cellPx;
            q.y1 = topY + cellPx;
            q.u0 = (static_cast<float>(gcol) * kGlyphCellPx) / textSheetW;
            q.v0 = (static_cast<float>(grow) * kGlyphCellPx) / textSheetH;
            q.u1 = (static_cast<float>(gcol + 1) * kGlyphCellPx) / textSheetW;
            q.v1 = (static_cast<float>(grow + 1) * kGlyphCellPx) / textSheetH;
            quads.push_back(q);
            penX += TextMetrics::Advance(c) * glyphScale;
        }
    }

    void InputPad::AppendLeftAlignedLabel(std::vector<Quad>& quads, const std::string& text,
                                            float leftX, float centerY, float viewportScale) const
    {
        if (text.empty())
        {
            return;
        }
        const float textSheetW = static_cast<float>(textTexture_.getWidthProperty());
        const float textSheetH = static_cast<float>(textTexture_.getHeightProperty());
        const float glyphScale = kSetupLabelScale * viewportScale;
        const float cellPx = kGlyphCellPx * glyphScale;
        const float topY = centerY - kSetupLabelYNudge * viewportScale;
        float penX = leftX;
        for (const char c : text)
        {
            const int rank = TextMetrics::GlyphIndex(c);
            const int gcol = rank % kGlyphCols;
            const int grow = rank / kGlyphCols;
            Quad q;
            q.x0 = penX;
            q.y0 = topY;
            q.x1 = penX + cellPx;
            q.y1 = topY + cellPx;
            q.u0 = (static_cast<float>(gcol) * kGlyphCellPx) / textSheetW;
            q.v0 = (static_cast<float>(grow) * kGlyphCellPx) / textSheetH;
            q.u1 = (static_cast<float>(gcol + 1) * kGlyphCellPx) / textSheetW;
            q.v1 = (static_cast<float>(grow + 1) * kGlyphCellPx) / textSheetH;
            quads.push_back(q);
            penX += TextMetrics::Advance(c) * glyphScale;
        }
    }

    void InputPad::AppendGamerLabel(std::vector<Quad>& quads, const std::string& text, float leftX, float topY,
                                      float labelScale, float viewportScale) const
    {
        if (text.empty())
        {
            return;
        }
        const float textSheetW = static_cast<float>(textTexture_.getWidthProperty());
        const float textSheetH = static_cast<float>(textTexture_.getHeightProperty());
        const float glyphScale = labelScale * viewportScale;
        const float cellPx = kGlyphCellPx * glyphScale;
        float penX = leftX;
        for (const char c : text)
        {
            const int rank = TextMetrics::GlyphIndex(c);
            const int gcol = rank % kGlyphCols;
            const int grow = rank / kGlyphCols;
            Quad q;
            q.x0 = penX;
            q.y0 = topY;
            q.x1 = penX + cellPx;
            q.y1 = topY + cellPx;
            q.u0 = (static_cast<float>(gcol) * kGlyphCellPx) / textSheetW;
            q.v0 = (static_cast<float>(grow) * kGlyphCellPx) / textSheetH;
            q.u1 = (static_cast<float>(gcol + 1) * kGlyphCellPx) / textSheetW;
            q.v1 = (static_cast<float>(grow + 1) * kGlyphCellPx) / textSheetH;
            quads.push_back(q);
            penX += TextMetrics::Advance(c) * glyphScale;
        }
    }

    InputPad::CharacterAnim InputPad::ComputePauseResumeCharacterAnim(
        float phaseTimeSeconds, GalaxyEggbert::Def::GamePhase fadeOutPhase) const
    {
        using GalaxyEggbert::Def::GamePhase;

        CharacterAnim anim{};
        anim.centerX = kCharacterCenterX;
        anim.centerY = kCharacterCenterY;
        anim.opacity = 1.0f;

        const float charW = static_cast<float>(blupiyoupieTexture_.getWidthProperty());
        const float charH = static_cast<float>(blupiyoupieTexture_.getHeightProperty());

        if (fadeOutPhase == GalaxyEggbert::Def::GamePhase::None)
        {
            // Real entrance flourish (NOT part of the generic commit
            // fade): grow from a point + decelerating 360 degree spin
            // over a real 0.75s.
            const float t = std::min(phaseTimeSeconds / kPauseEntryDurationSeconds, 1.0f);
            anim.halfW = (charW * 0.5f) * t;
            anim.halfH = (charH * 0.5f) * t;
            anim.rotationDegrees = (1.0f - t) * (1.0f - t) * 360.0f;
        }
        else if (fadeOutPhase == GalaxyEggbert::Def::GamePhase::Play)
        {
            // Real exit-to-Play: blow up to 11x native size while
            // linearly fading out, no rotation (same idiom as Init->Play).
            const float t = std::min(phaseTimeSeconds / kFadeDurationSeconds, 1.0f);
            const float num = 1.0f + t * 10.0f;
            anim.halfW = (charW * 0.5f) * num;
            anim.halfH = (charH * 0.5f) * num;
            anim.rotationDegrees = 0.0f;
            anim.opacity = 1.0f - t;
        }
        else if (fadeOutPhase == GalaxyEggbert::Def::GamePhase::PlaySetup)
        {
            // Real exit-to-PlaySetup (Pause only): fixed native size/
            // opacity, slides horizontally off to the right, quadratic
            // ease-in, no rotation.
            const float t = std::min(phaseTimeSeconds / kFadeDurationSeconds, 1.0f);
            const float num = t * t;
            anim.centerX = kCharacterCenterX + 800.0f * num;
            anim.halfW = charW * 0.5f;
            anim.halfH = charH * 0.5f;
            anim.rotationDegrees = 0.0f;
        }
        else
        {
            // Real exit-to-Init (or any other destination): the entrance
            // formula run in reverse -- shrinks to nothing while spinning
            // UP into a full 360, over the first 0.75s of the real 1.0s
            // commit window (leaving a real ~0.25s "dead"/invisible
            // window before the phase actually commits -- reproduced
            // faithfully, not "fixed").
            const float shrinkT = std::min(phaseTimeSeconds / kPauseEntryDurationSeconds, 1.0f);
            const float numShrink = 1.0f - shrinkT;
            anim.halfW = (charW * 0.5f) * numShrink;
            anim.halfH = (charH * 0.5f) * numShrink;
            anim.rotationDegrees = shrinkT * shrinkT * 360.0f;
        }

        anim.visible = anim.halfW > 0.0f && anim.halfH > 0.0f;
        return anim;
    }

    InputPad::SetupFadeAnim InputPad::ComputeSetupFadeAnim(float phaseTimeSeconds, bool exiting) const
    {
        const float t = std::min(phaseTimeSeconds / kFadeDurationSeconds, 1.0f);
        const float enteringNum = 1.0f - (1.0f - t) * (1.0f - t);

        float num;
        float num2;
        if (!exiting)
        {
            num = enteringNum;
            // Real: once past the first 1.0s (i.e. genuinely idle/
            // settled, not merely mid-entry), the gears keep slowly
            // rotating forever (400 frames = 20s per unit) rather than
            // freezing -- confirmed via research.
            num2 = phaseTimeSeconds < kFadeDurationSeconds
                       ? enteringNum
                       : 1.0f + (phaseTimeSeconds - kFadeDurationSeconds) / 20.0f;
        }
        else
        {
            // Real exit: both num and num2 inverted -- an exit fade never
            // outlasts the bounded 1.0s commit window, so num2's own
            // "settled" branch above never applies here.
            num = 1.0f - enteringNum;
            num2 = 1.0f - enteringNum;
        }

        SetupFadeAnim anim{};
        anim.speedyLeft = 720.0f - 640.0f * num;
        anim.speedyRight = 1360.0f - 640.0f * num;
        anim.speedyOpacity = num * num;
        anim.gearOpacity = 0.5f - num * 0.4f;
        anim.gearRotation1 = -num2 * 250.0f;
        anim.gearRotation2 = num2 * 125.0f;
        return anim;
    }

    bool InputPad::UpdatePlay(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                int viewportW, int viewportH, PlayInput& outInput) noexcept
    {
        outInput = PlayInput{};

        // Hit-testing is pure layout math -- deliberately NOT gated on
        // loaded_ (unlike DrawPlay()/DrawPause() below), so controls still
        // respond even if pad.png somehow failed to load, and so this
        // logic is testable headless without a GraphicsDevice (see
        // tools/VerifyGEInputPad.cpp).
        using Microsoft::Xna::Framework::Input::ButtonState;

        const ScreenMetrics m = Metrics(viewportW, viewportH);
        const float mouseX = static_cast<float>(mouse.getXProperty());
        const float mouseY = static_cast<float>(mouse.getYProperty());
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;

        const auto padCenter = DPadCenter(m);
        const float padHalf = kDPadRadius * m.s;
        const bool overDPad = InRect(mouseX, mouseY, Rect{padCenter.X - padHalf, padCenter.Y - padHalf,
                                                          padCenter.X + padHalf, padCenter.Y + padHalf});
        const bool overJump = InRect(mouseX, mouseY, PlayJumpRect(m));
        const bool overAction = InRect(mouseX, mouseY, PlayActionRect(m));
        const bool overPause = InRect(mouseX, mouseY, PlayPauseRect(m));

        if (mouseDown && !mouseWasDown_)
        {
            if (overDPad) activeControl_ = kPlayControlDPad;
            else if (overJump) activeControl_ = kPlayControlJump;
            else if (overAction) activeControl_ = kPlayControlAction;
            else if (overPause) activeControl_ = kPlayControlPause;
            else activeControl_ = -1;
        }

        // Real level-triggered Jump: fires every frame the pointer is
        // currently inside its rect while the mouse is down, independent
        // of where the press started (see InputPad.hpp's class comment).
        outInput.jumpHeld = mouseDown && overJump;

        // Real discrete D-pad: only active while the drag that STARTED on
        // the pad continues, using the CURRENT drag point each frame (not
        // a fixed initial grab offset).
        if (mouseDown && activeControl_ == kPlayControlDPad)
        {
            const float dx = mouseX - padCenter.X;
            const float dy = mouseY - padCenter.Y;
            const float threshold = kDPadThreshold * m.s;
            outInput.turnInput = dx > threshold ? 1.0f : (dx < -threshold ? -1.0f : 0.0f);
            // Screen Y increases downward; dragging the thumb UP (negative
            // dy) should read as "forward" (+1), matching a physical D-pad.
            outInput.moveInput = dy < -threshold ? 1.0f : (dy > threshold ? -1.0f : 0.0f);
            // Real knob: drawn at the touch point (padTouchPos), which stays
            // within the touch area. Stored in reference pixels.
            dpadDragOffsetX_ = std::clamp(dx, -padHalf, padHalf) / m.s;
            dpadDragOffsetY_ = std::clamp(dy, -padHalf, padHalf) / m.s;
        }
        else
        {
            dpadDragOffsetX_ = 0.0f;
            dpadDragOffsetY_ = 0.0f;
        }

        if (!mouseDown && mouseWasDown_)
        {
            // Real edge/release-triggered Action and Pause: single-fire on
            // release, regardless of current pointer position, as long as
            // the press started on that control.
            if (activeControl_ == kPlayControlAction) outInput.actionPressed = true;
            else if (activeControl_ == kPlayControlPause) outInput.pausePressed = true;
            activeControl_ = -1;
        }

        mouseWasDown_ = mouseDown;
        return activeControl_ != -1;
    }

    void InputPad::DrawPlay(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                              int viewportW, int viewportH)
    {
        if (!loaded_)
        {
            return;
        }

        const ScreenMetrics m = Metrics(viewportW, viewportH);

        const float padSheetW = static_cast<float>(padTexture_.getWidthProperty());
        const float padSheetH = static_cast<float>(padTexture_.getHeightProperty());

        std::vector<Quad> normalQuads;
        std::vector<Quad> pressedQuads;

        const auto appendIconQuad = [&](std::vector<Quad>& bucket, float x0, float y0, float x1, float y1, int icon)
        {
            Quad q;
            q.x0 = x0;
            q.y0 = y0;
            q.x1 = x1;
            q.y1 = y1;
            PadIconUv(icon, padSheetW, padSheetH, q.u0, q.v0, q.u1, q.v1);
            bucket.push_back(q);
        };

        // D-pad: ring always centered; thumb offset by the live drag (zero
        // when not actively dragging, snapping back to center).
        const auto padCenter = DPadCenter(m);
        const float drawHalf = kDPadDrawRadius * m.s;
        appendIconQuad(normalQuads, padCenter.X - drawHalf, padCenter.Y - drawHalf, padCenter.X + drawHalf,
                       padCenter.Y + drawHalf, kIconDPadRing);
        const float thumbCx = padCenter.X + dpadDragOffsetX_ * m.s;
        const float thumbCy = padCenter.Y + dpadDragOffsetY_ * m.s;
        appendIconQuad(activeControl_ == kPlayControlDPad ? pressedQuads : normalQuads,
                       thumbCx - drawHalf, thumbCy - drawHalf, thumbCx + drawHalf, thumbCy + drawHalf,
                       kIconDPadThumb);

        const auto appendRect = [&](std::vector<Quad>& bucket, const Rect& r, int icon)
        {
            appendIconQuad(bucket, r.x0, r.y0, r.x1, r.y1, icon);
        };
        appendRect(activeControl_ == kPlayControlJump ? pressedQuads : normalQuads, PlayJumpRect(m), kIconPlayJump);
        appendRect(activeControl_ == kPlayControlAction ? pressedQuads : normalQuads, PlayActionRect(m),
                   kIconPlayAction);
        appendRect(activeControl_ == kPlayControlPause ? pressedQuads : normalQuads, PlayPauseRect(m),
                   kIconPlayPause);

        // NonPremultiplied, not AlphaBlend: Texture2D(path) loads PNGs with
        // straight alpha, while AlphaBlend expects the premultiplied colours
        // CNA's (and XNA's) content pipeline produces, which Mobile Eggbert
        // draws. With AlphaBlend every translucent pixel came out too bright:
        // the menu's dark blue button panel showed whitish (user report,
        // 2026-10-25).
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        FlushQuads(device, *padEffect_, padRenderer_, normalQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressedQuads, viewportW, viewportH, kPlayPressedAlpha);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    InputPad::PauseInput InputPad::UpdatePause(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                                    int viewportW, int viewportH,
                                                    bool showBack, bool showRestart) noexcept
    {
        PauseInput result;

        // Same reasoning as UpdatePlay() above -- not gated on loaded_.
        using Microsoft::Xna::Framework::Input::ButtonState;

        const ScreenMetrics m = Metrics(viewportW, viewportH);
        const float mouseX = static_cast<float>(mouse.getXProperty());
        const float mouseY = static_cast<float>(mouse.getYProperty());
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;

        // Unconditional: Menu/Setup/Continue. Conditional: Back/Restart --
        // real visibility rules (mission!=1; mission!=1 && mission%10!=0).
        const bool overMenu = InRect(mouseX, mouseY, PauseButtonRect(0, m));
        const bool overBack = showBack && InRect(mouseX, mouseY, PauseButtonRect(1, m));
        const bool overSetup = InRect(mouseX, mouseY, PauseButtonRect(2, m));
        const bool overRestart = showRestart && InRect(mouseX, mouseY, PauseButtonRect(3, m));
        const bool overContinue = InRect(mouseX, mouseY, PauseButtonRect(4, m));

        if (mouseDown && !mouseWasDown_)
        {
            if (overMenu) activeControl_ = kPauseControlMenu;
            else if (overBack) activeControl_ = kPauseControlBack;
            else if (overSetup) activeControl_ = kPauseControlSetup;
            else if (overRestart) activeControl_ = kPauseControlRestart;
            else if (overContinue) activeControl_ = kPauseControlContinue;
            else activeControl_ = -1;
        }

        if (!mouseDown && mouseWasDown_)
        {
            // Continue/Restart/Setup/Menu/Back are all wired to real
            // behavior (see InputPad.hpp's UpdatePause() comment -- Menu
            // goes to Init, Back goes to the mission's own hub, 2026-07-17).
            if (activeControl_ == kPauseControlContinue) result.continuePressed = true;
            else if (activeControl_ == kPauseControlRestart) result.restartPressed = true;
            else if (activeControl_ == kPauseControlSetup) result.setupPressed = true;
            else if (activeControl_ == kPauseControlMenu) result.menuPressed = true;
            else if (activeControl_ == kPauseControlBack) result.backPressed = true;
            activeControl_ = -1;
        }

        mouseWasDown_ = mouseDown;
        return result;
    }

    void InputPad::DrawPause(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                              int viewportW, int viewportH, bool showBack, bool showRestart,
                              float phaseTimeSeconds, GalaxyEggbert::Def::GamePhase fadeOutPhase)
    {
        if (!loaded_)
        {
            return;
        }

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const auto refToScreenX = [&](float x) { return offsetX + x * scale; };
        const auto refToScreenY = [&](float y) { return y * scale; };
        const ScreenMetrics m = Metrics(viewportW, viewportH);

        // Real pause.png is an exact 640x480 match for the reference
        // space -- a direct full-screen quad, no cropping/UV math needed.
        // The background never fades -- only the character does.
        Quad background;
        background.x0 = refToScreenX(0.0f);
        background.y0 = refToScreenY(0.0f);
        background.x1 = refToScreenX(kRefW);
        background.y1 = refToScreenY(kRefH);
        background.u0 = 0.0f;
        background.v0 = 0.0f;
        background.u1 = 1.0f;
        background.v1 = 1.0f;
        std::vector<Quad> backgroundQuads{BackgroundBarsQuad(viewportW, viewportH), background};

        // Real entrance flourish / exit fades (plan.md MENU-088/089) --
        // see ComputePauseResumeCharacterAnim()'s own comment for the
        // per-`fadeOutPhase` formulas.
        const auto anim = ComputePauseResumeCharacterAnim(phaseTimeSeconds, fadeOutPhase);
        auto charMesh = MakeTexturedMeshBuilder();
        if (anim.visible)
        {
            // Real Pixmap::DrawIcon() adds no image origin: the character's
            // centre is in screen pixels.
            AppendRotatedQuadUv(charMesh, anim.centerX * scale, refToScreenY(anim.centerY),
                                anim.halfW * scale, anim.halfH * scale, anim.rotationDegrees, 0.0f, 0.0f, 1.0f, 1.0f);
        }

        const float padSheetW = static_cast<float>(padTexture_.getWidthProperty());
        const float padSheetH = static_cast<float>(padTexture_.getHeightProperty());
        std::vector<Quad> normalQuads;
        std::vector<Quad> pressedQuads;
        std::vector<Quad> labelQuads;

        // Real: buttons/labels are hidden entirely while an exit fade is
        // active (`fadeOutPhase != None`), confirmed via research -- NOT
        // during the entrance flourish (`fadeOutPhase == None` covers
        // both a fresh entry and the settled idle state).
        if (fadeOutPhase == GalaxyEggbert::Def::GamePhase::None)
        {
            // Real button labels (confirmed 2026-07-13 against `Game1::
            // DrawButtonsText()`'s real `DrawTextUnderButton()` calls for
            // `Phase::Pause`): centered under each VISIBLE button, at real
            // scale 0.7, Y = button's real bottom edge + 2. Real EN
            // strings -- note PauseMenu's real text is "Home", not "Menu".
            const auto appendLabel = [&](int index, bool visible, const char* text)
            {
                if (!visible)
                {
                    return;
                }
                const Rect r = PauseButtonRect(index, m);
                const float centerX = (r.x0 + r.x1) * 0.5f;
                const float topY = r.y1 + kPauseLabelYOffset * scale;
                AppendCenteredLabel(labelQuads, text, centerX, topY, scale);
            };

            const auto appendButton = [&](int index, bool visible, int icon, int controlId)
            {
                if (!visible)
                {
                    return;
                }
                const Rect r = PauseButtonRect(index, m);
                Quad q;
                q.x0 = r.x0;
                q.y0 = r.y0;
                q.x1 = r.x1;
                q.y1 = r.y1;
                PadIconUv(icon, padSheetW, padSheetH, q.u0, q.v0, q.u1, q.v1);
                (activeControl_ == controlId ? pressedQuads : normalQuads).push_back(q);
            };
            appendButton(0, true, kIconPauseMenu, kPauseControlMenu);
            appendButton(1, showBack, kIconPauseBack, kPauseControlBack);
            appendButton(2, true, kIconPauseSetup, kPauseControlSetup);
            appendButton(3, showRestart, kIconPauseRestart, kPauseControlRestart);
            appendButton(4, true, kIconPauseContinue, kPauseControlContinue);
            appendLabel(0, true, "Home");
            appendLabel(1, showBack, "Back");
            appendLabel(2, true, "Setup");
            appendLabel(3, showRestart, "Restart");
            appendLabel(4, true, "Continue");
        }

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        FlushQuads(device, *pauseBgEffect_, pauseBgRenderer_, backgroundQuads, viewportW, viewportH, 1.0f);
        if (!charMesh.Mesh().Empty())
        {
            blupiyoupieEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            blupiyoupieEffect_->View = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            blupiyoupieEffect_->Projection = Microsoft::Xna::Framework::Matrix::CreateOrthographicOffCenter(
                0.0f, static_cast<float>(viewportW), static_cast<float>(viewportH), 0.0f, 0.0f, 1.0f);
            blupiyoupieEffect_->setAlphaProperty(anim.opacity);
            blupiyoupieRenderer_ = UploadTexturedMesh(device, charMesh.Mesh());
            blupiyoupieRenderer_->Draw(device, *blupiyoupieEffect_);
            blupiyoupieEffect_->setAlphaProperty(1.0f);
        }
        FlushQuads(device, *padEffect_, padRenderer_, normalQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressedQuads, viewportW, viewportH, kPausePressedAlpha);
        FlushQuads(device, *textEffect_, textRenderer_, labelQuads, viewportW, viewportH, 1.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    bool InputPad::UpdateWinLost(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                   int viewportW, int viewportH) noexcept
    {
        using Microsoft::Xna::Framework::Input::ButtonState;

        const ScreenMetrics m = Metrics(viewportW, viewportH);
        const float mouseX = static_cast<float>(mouse.getXProperty());
        const float mouseY = static_cast<float>(mouse.getYProperty());
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;

        const bool overReturn = InRect(mouseX, mouseY, WinLostReturnRect(m));

        if (mouseDown && !mouseWasDown_)
        {
            activeControl_ = overReturn ? kWinLostControlReturn : -1;
        }

        bool returnPressed = false;
        if (!mouseDown && mouseWasDown_)
        {
            // Real edge/release-triggered semantics, same as every other
            // non-Jump button in this class.
            returnPressed = (activeControl_ == kWinLostControlReturn);
            activeControl_ = -1;
        }

        mouseWasDown_ = mouseDown;
        return returnPressed;
    }

    void InputPad::DrawWinLost(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                int viewportW, int viewportH, bool won, float phaseTimeSeconds)
    {
        if (!loaded_)
        {
            return;
        }

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const auto refToScreenX = [&](float x) { return offsetX + x * scale; };
        const auto refToScreenY = [&](float y) { return y * scale; };
        const ScreenMetrics m = Metrics(viewportW, viewportH);

        // Real win.png/lost.png are exact 640x480 matches for the
        // reference space, same as pause.png.
        Quad background;
        background.x0 = refToScreenX(0.0f);
        background.y0 = refToScreenY(0.0f);
        background.x1 = refToScreenX(kRefW);
        background.y1 = refToScreenY(kRefH);
        background.u0 = 0.0f;
        background.v0 = 0.0f;
        background.u1 = 1.0f;
        background.v1 = 1.0f;
        std::vector<Quad> backgroundQuads{BackgroundBarsQuad(viewportW, viewportH), background};

        // Real per-phase animation (see InputPad.hpp's class comment for
        // the exact real formulas this ports): Win pulses forever between
        // 0.5x/1.5x native size, no rotation; Lost grows from nothing to
        // native size once over 5s with a decaying 6-turn spin.
        const float charW = static_cast<float>(blupiyoupieTexture_.getWidthProperty());
        const float charH = static_cast<float>(blupiyoupieTexture_.getHeightProperty());
        float num;
        float rotationDegrees = 0.0f;
        if (won)
        {
            num = std::sin(phaseTimeSeconds / kWinPulsePeriodSeconds) * 0.5f + 1.0f;
        }
        else
        {
            num = std::min(phaseTimeSeconds / kLostGrowDurationSeconds, 1.0f);
            if (num < 1.0f)
            {
                const float settle = 1.0f - num;
                rotationDegrees = settle * settle * kLostSpinMaxDegrees;
            }
        }
        const float halfWRef = (charW * 0.5f) * num;
        const float halfHRef = (charH * 0.5f) * num;

        auto charMesh = MakeTexturedMeshBuilder();
        if (halfWRef > 0.0f && halfHRef > 0.0f)
        {
            AppendRotatedQuadUv(charMesh,
                                kCharacterCenterX * scale, refToScreenY(kWinLostCharacterCenterY),
                                halfWRef * scale, halfHRef * scale, rotationDegrees,
                                0.0f, 0.0f, 1.0f, 1.0f);
        }

        const float padSheetW = static_cast<float>(padTexture_.getWidthProperty());
        const float padSheetH = static_cast<float>(padTexture_.getHeightProperty());
        std::vector<Quad> normalQuads;
        std::vector<Quad> pressedQuads;
        {
            const Rect r = WinLostReturnRect(m);
            Quad q;
            q.x0 = r.x0;
            q.y0 = r.y0;
            q.x1 = r.x1;
            q.y1 = r.y1;
            PadIconUv(kIconPlayPause, padSheetW, padSheetH, q.u0, q.v0, q.u1, q.v1);
            (activeControl_ == kWinLostControlReturn ? pressedQuads : normalQuads).push_back(q);
        }

        auto& bgEffect = won ? *winBgEffect_ : *lostBgEffect_;
        auto& bgRenderer = won ? winBgRenderer_ : lostBgRenderer_;

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        FlushQuads(device, bgEffect, bgRenderer, backgroundQuads, viewportW, viewportH, 1.0f);
        if (!charMesh.Mesh().Empty())
        {
            blupiyoupieEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            blupiyoupieEffect_->View = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            blupiyoupieEffect_->Projection = Microsoft::Xna::Framework::Matrix::CreateOrthographicOffCenter(
                0.0f, static_cast<float>(viewportW), static_cast<float>(viewportH), 0.0f, 0.0f, 1.0f);
            blupiyoupieRenderer_ = UploadTexturedMesh(device, charMesh.Mesh());
            blupiyoupieRenderer_->Draw(device, *blupiyoupieEffect_);
        }
        FlushQuads(device, *padEffect_, padRenderer_, normalQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressedQuads, viewportW, viewportH, kPlayPressedAlpha);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    InputPad::SetupInput InputPad::UpdateSetup(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                                    int viewportW, int viewportH, bool showReset) noexcept
    {
        SetupInput result;

        using Microsoft::Xna::Framework::Input::ButtonState;

        const ScreenMetrics m = Metrics(viewportW, viewportH);
        const float mouseX = static_cast<float>(mouse.getXProperty());
        const float mouseY = static_cast<float>(mouse.getYProperty());
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;

        const bool overSounds = InRect(mouseX, mouseY, SetupSoundsRect(m));
        const bool overJump = InRect(mouseX, mouseY, SetupJumpRect(m));
        const bool overZoom = InRect(mouseX, mouseY, SetupZoomRect(m));
        const bool overAccel = InRect(mouseX, mouseY, SetupAccelRect(m));
        const bool overReset = showReset && InRect(mouseX, mouseY, SetupResetRect(m));
        const bool overReturn = InRect(mouseX, mouseY, SetupReturnRect(m));

        if (mouseDown && !mouseWasDown_)
        {
            if (overSounds) activeControl_ = kSetupControlSounds;
            else if (overJump) activeControl_ = kSetupControlJump;
            else if (overZoom) activeControl_ = kSetupControlZoom;
            else if (overAccel) activeControl_ = kSetupControlAccel;
            else if (overReset) activeControl_ = kSetupControlReset;
            else if (overReturn) activeControl_ = kSetupControlReturn;
            else activeControl_ = -1;
        }

        if (!mouseDown && mouseWasDown_)
        {
            // Sounds/Reset (both real, meaningful desktop behavior now)
            // and Return are wired -- Jump/Zoom/Accel render at their
            // real position/icon/label but stay intentionally inert (see
            // InputPad.hpp's UpdateSetup() class comment).
            if (activeControl_ == kSetupControlSounds) result.soundsToggled = true;
            else if (activeControl_ == kSetupControlReset) result.resetPressed = true;
            else if (activeControl_ == kSetupControlReturn) result.returnPressed = true;
            activeControl_ = -1;
        }

        mouseWasDown_ = mouseDown;
        return result;
    }

    void InputPad::DrawSetup(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                              int viewportW, int viewportH, bool soundsOn, bool showReset, int selectedGamer,
                              float phaseTimeSeconds, GalaxyEggbert::Def::GamePhase fadeOutPhase)
    {
        if (!loaded_)
        {
            return;
        }

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const auto refToScreenX = [&](float x) { return offsetX + x * scale; };
        const auto refToScreenY = [&](float y) { return y * scale; };
        const ScreenMetrics m = Metrics(viewportW, viewportH);
        // Real Pixmap::DrawIcon() adds no image origin: the title and gears
        // are in screen pixels.
        const auto screenX = [&](float x) { return x * scale; };

        // Real setup.png is an exact 640x480 match for the reference
        // space, same as pause.png/win.png/lost.png. Never fades.
        Quad background;
        background.x0 = refToScreenX(0.0f);
        background.y0 = refToScreenY(0.0f);
        background.x1 = refToScreenX(kRefW);
        background.y1 = refToScreenY(kRefH);
        background.u0 = 0.0f;
        background.v0 = 0.0f;
        background.u1 = 1.0f;
        background.v1 = 1.0f;
        std::vector<Quad> backgroundQuads{BackgroundBarsQuad(viewportW, viewportH), background};

        // Real speedyblupi.png slide-in + 2 rotating gear.png decorations
        // (plan.md MENU-088/089, MENU-059/060) -- see
        // ComputeSetupFadeAnim()'s own comment for the exact real formula,
        // shared verbatim by entry and exit (num/num2 inverted on exit).
        const bool exiting = fadeOutPhase != GalaxyEggbert::Def::GamePhase::None;
        const auto anim = ComputeSetupFadeAnim(phaseTimeSeconds, exiting);

        Quad speedyQuad;
        speedyQuad.x0 = screenX(anim.speedyLeft);
        speedyQuad.y0 = refToScreenY(0.0f);
        speedyQuad.x1 = screenX(anim.speedyRight);
        speedyQuad.y1 = refToScreenY(160.0f);
        speedyQuad.u0 = 0.0f;
        speedyQuad.v0 = 0.0f;
        speedyQuad.u1 = 1.0f;
        speedyQuad.v1 = 1.0f;
        std::vector<Quad> speedyQuads{speedyQuad};

        auto gearMesh = MakeTexturedMeshBuilder();
        AppendRotatedQuadUv(gearMesh, screenX(kGear1CenterX), refToScreenY(kGear1CenterY),
                            kGear1Half * scale, kGear1Half * scale, anim.gearRotation1, 0.0f, 0.0f, 1.0f, 1.0f);
        AppendRotatedQuadUv(gearMesh, screenX(kGear2CenterX), refToScreenY(kGear2CenterY),
                            kGear2Half * scale, kGear2Half * scale, anim.gearRotation2, 0.0f, 0.0f, 1.0f, 1.0f);

        const float padSheetW = static_cast<float>(padTexture_.getWidthProperty());
        const float padSheetH = static_cast<float>(padTexture_.getHeightProperty());
        std::vector<Quad> normalQuads;
        std::vector<Quad> pressedQuads;
        std::vector<Quad> labelQuads;

        // Real: buttons/labels are hidden entirely while an exit fade is
        // active, not during the entry decoration (which plays as a
        // purely decorative overlay atop an already-interactive screen,
        // confirmed via research).
        if (!exiting)
        {
            const auto appendButton = [&](const Rect& r, int icon, int controlId)
            {
                Quad q;
                q.x0 = r.x0;
                q.y0 = r.y0;
                q.x1 = r.x1;
                q.y1 = r.y1;
                PadIconUv(icon, padSheetW, padSheetH, q.u0, q.v0, q.u1, q.v1);
                (activeControl_ == controlId ? pressedQuads : normalQuads).push_back(q);
            };
            const auto appendLabel = [&](const Rect& r, const std::string& text)
            {
                const float leftX = r.x1 + kSetupLabelXOffset * scale;
                const float centerY = (r.y0 + r.y1) * 0.5f;
                // Real DrawTextRightButton(): a two-line label starts 20
                // above the button's centre (12 above the one-line
                // position) and its second line is 24 lower.
                const auto newline = text.find('\n');
                if (newline == std::string::npos)
                {
                    AppendLeftAlignedLabel(labelQuads, text, leftX, centerY, scale);
                    return;
                }
                AppendLeftAlignedLabel(labelQuads, text.substr(0, newline), leftX, centerY - 12.0f * scale, scale);
                AppendLeftAlignedLabel(labelQuads, text.substr(newline + 1), leftX, centerY + 12.0f * scale, scale);
            };

            // Real icon SWAP (not just opacity) for the 3 toggle-style
            // buttons -- Jump/Zoom/Accel have no real state tracked in
            // this engine (no meaningful desktop equivalent, see class
            // comment), so they always render the "off" icon.
            appendButton(SetupSoundsRect(m), soundsOn ? kIconSetupToggleOn : kIconSetupToggleOff, kSetupControlSounds);
            appendButton(SetupJumpRect(m), kIconSetupToggleOff, kSetupControlJump);
            appendButton(SetupZoomRect(m), kIconSetupToggleOff, kSetupControlZoom);
            appendButton(SetupAccelRect(m), kIconSetupToggleOff, kSetupControlAccel);
            if (showReset)
            {
                appendButton(SetupResetRect(m), kIconSetupReset, kSetupControlReset);
            }
            appendButton(SetupReturnRect(m), kIconSetupReturn, kSetupControlReturn);

            appendLabel(SetupSoundsRect(m), "Sound effects");
            appendLabel(SetupJumpRect(m), "Jump button on the right");
            appendLabel(SetupZoomRect(m), "Automatic zoom on action");
            appendLabel(SetupAccelRect(m), "Accelerometer");
            if (showReset)
            {
                // Real TX_BUTTON_SETUP_RESET, two lines ("Player {0} :\nErase
                // progress"); it was collapsed to one line, which ran off the
                // screen (2026-10-25).
                const std::string resetLabel =
                    "Player " + std::string(1, static_cast<char>('A' + selectedGamer)) + " :\nErase progress";
                appendLabel(SetupResetRect(m), resetLabel);
            }
            // SetupReturn has no real label at all in the source (same as WinLostReturn).
        }

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        FlushQuads(device, *setupBgEffect_, setupBgRenderer_, backgroundQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *speedyblupiEffect_, speedyblupiRenderer_, speedyQuads, viewportW, viewportH,
                   anim.speedyOpacity);
        gearEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
        gearEffect_->View = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
        gearEffect_->Projection = Microsoft::Xna::Framework::Matrix::CreateOrthographicOffCenter(
            0.0f, static_cast<float>(viewportW), static_cast<float>(viewportH), 0.0f, 0.0f, 1.0f);
        gearEffect_->setAlphaProperty(anim.gearOpacity);
        gearRenderer_ = UploadTexturedMesh(device, gearMesh.Mesh());
        gearRenderer_->Draw(device, *gearEffect_);
        gearEffect_->setAlphaProperty(1.0f);
        FlushQuads(device, *padEffect_, padRenderer_, normalQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressedQuads, viewportW, viewportH, kPausePressedAlpha);
        FlushQuads(device, *textEffect_, textRenderer_, labelQuads, viewportW, viewportH, 1.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    InputPad::ResumeInput InputPad::UpdateResume(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                                      int viewportW, int viewportH) noexcept
    {
        ResumeInput result;

        using Microsoft::Xna::Framework::Input::ButtonState;

        const ScreenMetrics m = Metrics(viewportW, viewportH);
        const float mouseX = static_cast<float>(mouse.getXProperty());
        const float mouseY = static_cast<float>(mouse.getYProperty());
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;

        const bool overMenu = InRect(mouseX, mouseY, ResumeButtonRect(0, m));
        const bool overContinue = InRect(mouseX, mouseY, ResumeButtonRect(1, m));

        if (mouseDown && !mouseWasDown_)
        {
            if (overMenu) activeControl_ = kResumeControlMenu;
            else if (overContinue) activeControl_ = kResumeControlContinue;
            else activeControl_ = -1;
        }

        if (!mouseDown && mouseWasDown_)
        {
            // Both buttons are now wired to real behavior (see
            // InputPad.hpp's UpdateResume() class comment -- Menu now
            // goes to Init, 2026-07-13, now that Init exists).
            if (activeControl_ == kResumeControlContinue) result.continuePressed = true;
            else if (activeControl_ == kResumeControlMenu) result.menuPressed = true;
            activeControl_ = -1;
        }

        mouseWasDown_ = mouseDown;
        return result;
    }

    void InputPad::DrawResume(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                               int viewportW, int viewportH, float phaseTimeSeconds,
                               GalaxyEggbert::Def::GamePhase fadeOutPhase)
    {
        if (!loaded_)
        {
            return;
        }

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const auto refToScreenX = [&](float x) { return offsetX + x * scale; };
        const auto refToScreenY = [&](float y) { return y * scale; };
        const ScreenMetrics m = Metrics(viewportW, viewportH);

        // Real background is pause.png -- the SAME image as Pause
        // (confirmed via `Game1.cpp`'s shared `case Phase::Pause: case
        // Phase::Resume:` background dispatch). Never fades.
        Quad background;
        background.x0 = refToScreenX(0.0f);
        background.y0 = refToScreenY(0.0f);
        background.x1 = refToScreenX(kRefW);
        background.y1 = refToScreenY(kRefH);
        background.u0 = 0.0f;
        background.v0 = 0.0f;
        background.u1 = 1.0f;
        background.v1 = 1.0f;
        std::vector<Quad> backgroundQuads{BackgroundBarsQuad(viewportW, viewportH), background};

        // Real: Resume's entry flourish/exit fade is bit-for-bit Pause's
        // own (confirmed via research) -- Resume->Play (Continue) never
        // actually reaches this as an exit fade at all (the real `-2`
        // mission sentinel bypasses the defer mechanism entirely, see
        // this method's own class comment), so in practice fadeOutPhase
        // here is only ever None or Init.
        const auto anim = ComputePauseResumeCharacterAnim(phaseTimeSeconds, fadeOutPhase);
        auto charMesh = MakeTexturedMeshBuilder();
        if (anim.visible)
        {
            AppendRotatedQuadUv(charMesh, anim.centerX * scale, refToScreenY(anim.centerY),
                                anim.halfW * scale, anim.halfH * scale, anim.rotationDegrees, 0.0f, 0.0f, 1.0f, 1.0f);
        }

        const float padSheetW = static_cast<float>(padTexture_.getWidthProperty());
        const float padSheetH = static_cast<float>(padTexture_.getHeightProperty());
        std::vector<Quad> normalQuads;
        std::vector<Quad> pressedQuads;
        std::vector<Quad> labelQuads;

        if (fadeOutPhase == GalaxyEggbert::Def::GamePhase::None)
        {
            const auto appendButton = [&](const Rect& r, int icon, int controlId)
            {
                Quad q;
                q.x0 = r.x0;
                q.y0 = r.y0;
                q.x1 = r.x1;
                q.y1 = r.y1;
                PadIconUv(icon, padSheetW, padSheetH, q.u0, q.v0, q.u1, q.v1);
                (activeControl_ == controlId ? pressedQuads : normalQuads).push_back(q);
            };
            const auto appendLabel = [&](const Rect& r, const char* text)
            {
                const float centerX = (r.x0 + r.x1) * 0.5f;
                const float topY = r.y1 + kPauseLabelYOffset * scale;
                AppendCenteredLabel(labelQuads, text, centerX, topY, scale);
            };

            appendButton(ResumeButtonRect(0, m), kIconPauseMenu, kResumeControlMenu);
            appendButton(ResumeButtonRect(1, m), kIconPauseContinue, kResumeControlContinue);
            appendLabel(ResumeButtonRect(0, m), "Home");
            appendLabel(ResumeButtonRect(1, m), "Continue");
        }

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        FlushQuads(device, *pauseBgEffect_, pauseBgRenderer_, backgroundQuads, viewportW, viewportH, 1.0f);
        if (!charMesh.Mesh().Empty())
        {
            blupiyoupieEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            blupiyoupieEffect_->View = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            blupiyoupieEffect_->Projection = Microsoft::Xna::Framework::Matrix::CreateOrthographicOffCenter(
                0.0f, static_cast<float>(viewportW), static_cast<float>(viewportH), 0.0f, 0.0f, 1.0f);
            blupiyoupieEffect_->setAlphaProperty(anim.opacity);
            blupiyoupieRenderer_ = UploadTexturedMesh(device, charMesh.Mesh());
            blupiyoupieRenderer_->Draw(device, *blupiyoupieEffect_);
            blupiyoupieEffect_->setAlphaProperty(1.0f);
        }
        FlushQuads(device, *padEffect_, padRenderer_, normalQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressedQuads, viewportW, viewportH, kPausePressedAlpha);
        FlushQuads(device, *textEffect_, textRenderer_, labelQuads, viewportW, viewportH, 1.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    bool InputPad::UpdateCheatGesture(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                        int viewportW, int viewportH) noexcept
    {
        using Microsoft::Xna::Framework::Input::ButtonState;

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const float mouseRefX = (static_cast<float>(mouse.getXProperty()) - offsetX) / scale;
        const float mouseRefY = static_cast<float>(mouse.getYProperty()) / scale;
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;

        bool unlocked = false;
        if (mouseDown && !cheatMouseWasDown_)
        {
            int tappedZone = -1;
            if (mouseRefX >= 0.0f && mouseRefX < kGestureAreaW && mouseRefY >= 0.0f && mouseRefY < kGestureAreaH)
            {
                const int col = static_cast<int>(mouseRefX / kGestureCellW) + 1;
                const int row = static_cast<int>(mouseRefY / kGestureCellH) + 1;
                tappedZone = col * 10 + row;
            }
            if (tappedZone == kCheatGestureSequence[cheatGestureIndex_])
            {
                ++cheatGestureIndex_;
                if (cheatGestureIndex_ >= kCheatGestureLength)
                {
                    cheatGestureIndex_ = 0;
                    unlocked = true;
                }
            }
            else if (tappedZone != -1)
            {
                // Real behavior: any wrong tap resets progress to 0. A
                // press outside all 6 zones is simply ignored (see this
                // method's own class-comment for why) -- so this only
                // fires when a DIFFERENT one of the 6 zones was tapped.
                cheatGestureIndex_ = 0;
            }
        }

        cheatMouseWasDown_ = mouseDown;
        return unlocked;
    }

    int InputPad::UpdateCheatMenu(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                    int viewportW, int viewportH) noexcept
    {
        using Microsoft::Xna::Framework::Input::ButtonState;

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const float mouseRefX = (static_cast<float>(mouse.getXProperty()) - offsetX) / scale;
        const float mouseRefY = static_cast<float>(mouse.getYProperty()) / scale;
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;

        const bool overRow = mouseRefY >= 0.0f && mouseRefY < kCheatButtonH;
        const int hoveredIndex = overRow ? static_cast<int>(mouseRefX / kCheatButtonW) : -1;
        const bool overButton = overRow && hoveredIndex >= 0 && hoveredIndex < kCheatButtonCount;

        if (mouseDown && !cheatMouseWasDown_)
        {
            cheatActiveControl_ = overButton ? hoveredIndex : -1;
        }

        int pressedCheat = 0;
        if (!mouseDown && cheatMouseWasDown_)
        {
            if (cheatActiveControl_ >= 0)
            {
                pressedCheat = cheatActiveControl_ + 1;
            }
            cheatActiveControl_ = -1;
        }

        cheatMouseWasDown_ = mouseDown;
        return pressedCheat;
    }

    InputPad::TypedCheatResult InputPad::UpdateTypedGhostCheat(
        const Microsoft::Xna::Framework::Input::KeyboardState& keyboard, bool isPlayPhase) noexcept
    {
        using Microsoft::Xna::Framework::Input::Keys;

        TypedCheatResult result;
        if (!isPlayPhase)
        {
            return result;
        }

        static constexpr Keys kLetterKeys[26] = {
            Keys::A, Keys::B, Keys::C, Keys::D, Keys::E, Keys::F, Keys::G, Keys::H,
            Keys::I, Keys::J, Keys::K, Keys::L, Keys::M, Keys::N, Keys::O, Keys::P,
            Keys::Q, Keys::R, Keys::S, Keys::T, Keys::U, Keys::V, Keys::W, Keys::X,
            Keys::Y, Keys::Z};

        for (int li = 0; li < 26; ++li)
        {
            const bool down = keyboard.IsKeyDown(kLetterKeys[li]);
            if (down && !letterKeyWasDown_[li])
            {
                typedCheatBuffer_ += static_cast<char>('a' + li);
                if (typedCheatBuffer_.size() > 32)
                {
                    typedCheatBuffer_ = typedCheatBuffer_.substr(typedCheatBuffer_.size() - 32);
                }

                static const std::string kGhostName = "ghost";
                if (typedCheatBuffer_.size() >= kGhostName.size() &&
                    typedCheatBuffer_.substr(typedCheatBuffer_.size() - kGhostName.size()) == kGhostName)
                {
                    result.ghostTyped = true;
                }
                static const std::string kQuickName = "quick";
                if (typedCheatBuffer_.size() >= kQuickName.size() &&
                    typedCheatBuffer_.substr(typedCheatBuffer_.size() - kQuickName.size()) == kQuickName)
                {
                    result.quickTyped = true;
                }
            }
            letterKeyWasDown_[li] = down;
        }
        return result;
    }

    void InputPad::DrawCheatMenu(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                  int viewportW, int viewportH)
    {
        if (!loaded_)
        {
            return;
        }

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const auto refToScreenX = [&](float x) { return offsetX + x * scale; };
        const auto refToScreenY = [&](float y) { return y * scale; };

        const float padSheetW = static_cast<float>(padTexture_.getWidthProperty());
        const float padSheetH = static_cast<float>(padTexture_.getHeightProperty());
        std::vector<Quad> normalQuads;
        std::vector<Quad> pressedQuads;
        std::vector<Quad> labelQuads;

        for (int i = 0; i < kCheatButtonCount; ++i)
        {
            const float x0 = static_cast<float>(i) * kCheatButtonW;
            const float x1 = x0 + kCheatButtonW;
            Quad q;
            q.x0 = refToScreenX(x0);
            q.y0 = refToScreenY(0.0f);
            q.x1 = refToScreenX(x1);
            q.y1 = refToScreenY(kCheatButtonH);
            PadIconUv(kIconDPadRing, padSheetW, padSheetH, q.u0, q.v0, q.u1, q.v1);
            (cheatActiveControl_ == i ? pressedQuads : normalQuads).push_back(q);

            AppendCenteredLabel(labelQuads, kCheatLetters[i], refToScreenX((x0 + x1) * 0.5f),
                                refToScreenY(kCheatButtonH + kCheatLabelYOffset), scale);
        }

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        FlushQuads(device, *padEffect_, padRenderer_, normalQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressedQuads, viewportW, viewportH, kPausePressedAlpha);
        FlushQuads(device, *textEffect_, textRenderer_, labelQuads, viewportW, viewportH, 1.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    void InputPad::DrawWait(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW,
                              int viewportH, float phaseTimeSeconds)
    {
        if (!loaded_)
        {
            return;
        }

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const auto refToScreenX = [&](float x) { return offsetX + x * scale; };
        const auto refToScreenY = [&](float y) { return y * scale; };

        Quad background;
        background.x0 = refToScreenX(0.0f);
        background.y0 = refToScreenY(0.0f);
        background.x1 = refToScreenX(kRefW);
        background.y1 = refToScreenY(kRefH);
        background.u0 = 0.0f;
        background.v0 = 0.0f;
        background.u1 = 1.0f;
        background.v1 = 1.0f;
        std::vector<Quad> backgroundQuads{BackgroundBarsQuad(viewportW, viewportH), background};

        // Real non-linear waitTable lookup, not a linear ramp -- see this
        // file's own kWaitTable comment.
        const float progress = std::clamp(phaseTimeSeconds / kWaitDurationSeconds, 0.0f, 1.0f);
        int level = 100;
        for (const auto& entry : kWaitTable)
        {
            if (progress <= entry.threshold)
            {
                level = entry.level;
                break;
            }
        }
        const int filledWidth = level * 114 / 100;

        const float jaugeSheetW = static_cast<float>(jaugeTexture_.getWidthProperty());
        const float jaugeSheetH = static_cast<float>(jaugeTexture_.getHeightProperty());
        std::vector<Quad> jaugeQuads;
        {
            // Row 0: always-visible empty-gauge background.
            Quad q;
            q.x0 = refToScreenX(kJaugePosX);
            q.y0 = refToScreenY(kJaugePosY);
            q.x1 = refToScreenX(kJaugePosX + kJaugeCellW * kJaugeZoom);
            q.y1 = refToScreenY(kJaugePosY + kJaugeCellH * kJaugeZoom);
            q.u0 = 0.0f;
            q.v0 = 0.0f;
            q.u1 = kJaugeCellW / jaugeSheetW;
            q.v1 = kJaugeCellH / jaugeSheetH;
            jaugeQuads.push_back(q);
        }
        if (filledWidth > 0)
        {
            const float fillW = 6.0f + static_cast<float>(filledWidth);
            Quad q;
            q.x0 = refToScreenX(kJaugePosX);
            q.y0 = refToScreenY(kJaugePosY);
            q.x1 = refToScreenX(kJaugePosX + fillW * kJaugeZoom);
            q.y1 = refToScreenY(kJaugePosY + kJaugeCellH * kJaugeZoom);
            q.u0 = 0.0f;
            q.v0 = (kJaugeCellH * static_cast<float>(kJaugeModeYellow)) / jaugeSheetH;
            q.u1 = fillW / jaugeSheetW;
            q.v1 = (kJaugeCellH * static_cast<float>(kJaugeModeYellow + 1)) / jaugeSheetH;
            jaugeQuads.push_back(q);
        }

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        FlushQuads(device, *waitBgEffect_, waitBgRenderer_, backgroundQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *jaugeEffect_, jaugeRenderer_, jaugeQuads, viewportW, viewportH, 1.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    InputPad::InitInput InputPad::UpdateInit(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                                  int viewportW, int viewportH) noexcept
    {
        InitInput result;

        using Microsoft::Xna::Framework::Input::ButtonState;

        const InitLayout layout = ComputeInitLayout(viewportW, viewportH);
        const float mouseX = static_cast<float>(mouse.getXProperty());
        const float mouseY = static_cast<float>(mouse.getYProperty());
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;

        const bool overGamerA = InRect(mouseX, mouseY, layout.gamer[0]);
        const bool overGamerB = InRect(mouseX, mouseY, layout.gamer[1]);
        const bool overGamerC = InRect(mouseX, mouseY, layout.gamer[2]);
        const bool overSetup = InRect(mouseX, mouseY, layout.setup);
        const bool overPlay = InRect(mouseX, mouseY, layout.play);
        const bool overEditor = InRect(mouseX, mouseY, layout.editor);

        if (mouseDown && !mouseWasDown_)
        {
            if (overGamerA) activeControl_ = kInitControlGamerA;
            else if (overGamerB) activeControl_ = kInitControlGamerB;
            else if (overGamerC) activeControl_ = kInitControlGamerC;
            else if (overSetup) activeControl_ = kInitControlSetup;
            else if (overPlay) activeControl_ = kInitControlPlay;
            else if (overEditor) activeControl_ = kInitControlEditor;
            else activeControl_ = -1;
        }

        if (!mouseDown && mouseWasDown_)
        {
            // Real edge/release-triggered semantics, same as every other
            // non-Jump button in this class.
            switch (activeControl_)
            {
                case kInitControlGamerA: result.gamerSelected = 0; break;
                case kInitControlGamerB: result.gamerSelected = 1; break;
                case kInitControlGamerC: result.gamerSelected = 2; break;
                case kInitControlSetup: result.setupPressed = true; break;
                case kInitControlPlay: result.playPressed = true; break;
                case kInitControlEditor: result.editorPressed = true; break;
                default: break;
            }
            activeControl_ = -1;
        }

        mouseWasDown_ = mouseDown;
        return result;
    }

    void InputPad::DrawInit(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW,
                             int viewportH, float phaseTimeSeconds, int selectedGamer,
                             const std::array<InitGamerInfo, 3>& gamers, GalaxyEggbert::Def::GamePhase fadeOutPhase,
                             bool overTitleScene)
    {
        if (!loaded_)
        {
            return;
        }

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const auto refToScreenX = [&](float x) { return offsetX + x * scale; };
        const auto refToScreenY = [&](float y) { return y * scale; };

        Quad background;
        background.x0 = refToScreenX(0.0f);
        background.y0 = refToScreenY(0.0f);
        background.x1 = refToScreenX(kRefW);
        background.y1 = refToScreenY(kRefH);
        background.u0 = 0.0f;
        background.v0 = 0.0f;
        background.u1 = 1.0f;
        background.v1 = 1.0f;
        std::vector<Quad> backgroundQuads{BackgroundBarsQuad(viewportW, viewportH), background};

        const float t = std::min(phaseTimeSeconds / kInitEntryDurationSeconds, 1.0f);
        const float charW = static_cast<float>(blupiyoupieTexture_.getWidthProperty());
        const float charH = static_cast<float>(blupiyoupieTexture_.getHeightProperty());

        float titleLeft = kInitTitleLeft, titleRight = kInitTitleRight, titleTop, titleBottom;
        float titleOpacity = 1.0f;
        float charHalfW, charHalfH, charOpacity, charRotation = 0.0f;

        using GalaxyEggbert::Def::GamePhase;
        if (fadeOutPhase == GalaxyEggbert::Def::GamePhase::None)
        {
            // Real title-logo entry: vertical ease-out slide-down from
            // above the screen, Left/Right fixed.
            const float titleNum = 1.0f - (1.0f - t) * (1.0f - t);
            titleTop = -160.0f + 160.0f * titleNum;
            titleBottom = 160.0f * titleNum;

            // Real blupiyoupie entry: scale 50%->100%, fade 0.25->1.0, no
            // rotation.
            const float charNum = 0.5f + t * 0.5f;
            charOpacity = std::min(charNum * charNum, 1.0f);
            charHalfW = (charW * 0.5f) * charNum;
            charHalfH = (charH * 0.5f) * charNum;
        }
        else if (fadeOutPhase == GalaxyEggbert::Def::GamePhase::Play)
        {
            // Real exit-to-Play: title reverses at 2x speed (already
            // off-screen again by 0.5s, keeps going); blupiyoupie blows up
            // to 11x while linearly fading out, no rotation -- same idiom
            // as Pause->Play, just at Init's own (468,280).
            const float num = 1.0f - 2.0f * t;
            titleTop = -160.0f + 160.0f * num;
            titleBottom = 160.0f * num;

            const float charNum = 1.0f + t * 10.0f;
            charHalfW = (charW * 0.5f) * charNum;
            charHalfH = (charH * 0.5f) * charNum;
            charOpacity = 1.0f - t;
        }
        else
        {
            // Real exit-to-MainSetup: title slides back off to the right
            // while fading (`num=(1-t)^2`); blupiyoupie stays FIXED size/
            // position and just fades out -- confirmed via research NO
            // zoom happens here despite an earlier doc-comment claiming
            // one.
            const float num = (1.0f - t) * (1.0f - t);
            titleLeft = 720.0f - 640.0f * num;
            titleRight = 1360.0f - 640.0f * num;
            titleTop = 0.0f;
            titleBottom = 160.0f;
            titleOpacity = num * num;

            charHalfW = charW * 0.5f;
            charHalfH = charH * 0.5f;
            charOpacity = (1.0f - t) * (1.0f - t);
        }

        // Real Pixmap::DrawIcon() scales its rect by the zoom but does not
        // add the 640x480 image's origin (only text does): the title's 80-720
        // and the character's centre 468 are screen pixels, so screenX() here,
        // not refToScreenX() (they stood 80 pixels too far right on an
        // 800-wide window until 2026-10-25).
        const auto screenX = [&](float x) { return x * scale; };
        Quad titleQuad;
        titleQuad.x0 = screenX(titleLeft);
        titleQuad.y0 = refToScreenY(titleTop);
        titleQuad.x1 = screenX(titleRight);
        titleQuad.y1 = refToScreenY(titleBottom);
        titleQuad.u0 = 0.0f;
        titleQuad.v0 = 0.0f;
        titleQuad.u1 = 1.0f;
        titleQuad.v1 = 1.0f;
        std::vector<Quad> titleQuads{titleQuad};

        auto charMesh = MakeTexturedMeshBuilder();
        if (charHalfW > 0.0f && charHalfH > 0.0f)
        {
            AppendRotatedQuadUv(charMesh,
                                screenX(kInitCharacterCenterX), refToScreenY(kInitCharacterCenterY),
                                charHalfW * scale, charHalfH * scale, charRotation, 0.0f, 0.0f, 1.0f, 1.0f);
        }

        const float padSheetW = static_cast<float>(padTexture_.getWidthProperty());
        const float padSheetH = static_cast<float>(padTexture_.getHeightProperty());
        std::vector<Quad> normalQuads;
        std::vector<Quad> pressedQuads;
        std::vector<Quad> labelQuads;
        std::vector<Quad> panelQuads;

        std::vector<Quad> editorIconQuads;

        // Real: buttons are hidden entirely while an exit fade is active.
        if (fadeOutPhase == GalaxyEggbert::Def::GamePhase::None)
        {
            const InitLayout layout = ComputeInitLayout(viewportW, viewportH);
            const float s = layout.scale;
            const auto appendIconQuad = [&](std::vector<Quad>& bucket, const Rect& r, int icon)
            {
                Quad q{r.x0, r.y0, r.x1, r.y1, 0.0f, 0.0f, 0.0f, 0.0f};
                PadIconUv(icon, padSheetW, padSheetH, q.u0, q.v0, q.u1, q.v1);
                bucket.push_back(q);
            };
            for (const Rect& r : {layout.leftPanel, layout.rightPanel})
            {
                appendIconQuad(panelQuads, r, kInitPanelIcon);
            }

            const auto appendGamerRow = [&](int gamer, int control, int iconOff, int iconSel, char letter)
            {
                const Rect& r = layout.gamer[gamer];
                appendIconQuad(activeControl_ == control ? pressedQuads : normalQuads, r,
                              selectedGamer == gamer ? iconSel : iconOff);

                const float textLeft = r.x1 + kGamerTextXOffset * s;
                const InitGamerInfo& info = gamers[static_cast<std::size_t>(gamer)];
                std::string title = "Player ";
                title += letter;
                AppendGamerLabel(labelQuads, title, textLeft, r.y0 + kGamerTitleYOffset * s, kGamerTitleScale, s);
                AppendGamerLabel(labelQuads, "Main gates : " + std::to_string(info.mainGates) + "/12", textLeft,
                                r.y0 + kGamerMDoorsYOffset * s, kGamerBodyScale, s);
                AppendGamerLabel(labelQuads, "Secondary gates : " + std::to_string(info.secondaryGates) + "/52",
                                textLeft, r.y0 + kGamerSDoorsYOffset * s, kGamerBodyScale, s);
                AppendGamerLabel(labelQuads, "Blupi : " + std::to_string(info.lives), textLeft,
                                r.y0 + kGamerLivesYOffset * s, kGamerBodyScale, s);
            };
            appendGamerRow(0, kInitControlGamerA, kIconInitGamerAOff, kIconInitGamerASel, 'A');
            appendGamerRow(1, kInitControlGamerB, kIconInitGamerBOff, kIconInitGamerBSel, 'B');
            appendGamerRow(2, kInitControlGamerC, kIconInitGamerCOff, kIconInitGamerCSel, 'C');

            appendIconQuad(activeControl_ == kInitControlSetup ? pressedQuads : normalQuads, layout.setup,
                          kIconInitSetup);
            appendIconQuad(activeControl_ == kInitControlPlay ? pressedQuads : normalQuads, layout.play,
                          kIconInitPlay);
            appendIconQuad(activeControl_ == kInitControlEditor ? pressedQuads : normalQuads, layout.editor,
                          kIconInitEditorFace);

            // Real labels: TX_BUTTON_PLAY under Play, TX_BUTTON_SETUP right of
            // Setup (Game1::DrawButtonsText()); the editor's label under its
            // button, on two lines like the original's two-line labels.
            AppendCenteredLabel(labelQuads, "Play", (layout.play.x0 + layout.play.x1) * 0.5f,
                                layout.play.y1 + kPauseLabelYOffset * s, s);
            AppendLeftAlignedLabel(labelQuads, "Setup", layout.setup.x1 + kSetupLabelXOffset * s,
                                   (layout.setup.y0 + layout.setup.y1) * 0.5f, s);
            const float editorCenterX = (layout.editor.x0 + layout.editor.x1) * 0.5f;
            AppendCenteredLabel(labelQuads, "3D World", editorCenterX, layout.editor.y1 + kPauseLabelYOffset * s, s);
            AppendCenteredLabel(labelQuads, "Editor", editorCenterX, layout.editor.y1 + (kPauseLabelYOffset + 22.0f) * s,
                                s);

            const float iconHalf = (layout.editor.x1 - layout.editor.x0) * kInitEditorIconFraction * 0.5f;
            const float iconCenterY = (layout.editor.y0 + layout.editor.y1) * 0.5f;
            const float buttonSheetW = static_cast<float>(buttonTexture_.getWidthProperty());
            const float buttonSheetH = static_cast<float>(buttonTexture_.getHeightProperty());
            const int col = kButtonIconInitEditor % kButtonCols;
            const int row = kButtonIconInitEditor / kButtonCols;
            editorIconQuads.push_back(Quad{editorCenterX - iconHalf, iconCenterY - iconHalf,
                                           editorCenterX + iconHalf, iconCenterY + iconHalf,
                                           static_cast<float>(col) * kButtonCellPx / buttonSheetW,
                                           static_cast<float>(row) * kButtonCellPx / buttonSheetH,
                                           static_cast<float>(col + 1) * kButtonCellPx / buttonSheetW,
                                           static_cast<float>(row + 1) * kButtonCellPx / buttonSheetH});
        }

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        // Over Galaxy's 3D title scene the menu is only its panels, buttons
        // and labels; Mobile Eggbert's init.png, title and blupiyoupie art
        // remain the fallback without the scene.
        if (!overTitleScene)
        {
            FlushQuads(device, *initBgEffect_, initBgRenderer_, backgroundQuads, viewportW, viewportH, 1.0f);
            FlushQuads(device, *speedyblupiEffect_, speedyblupiRenderer_, titleQuads, viewportW, viewportH,
                       titleOpacity);
        }
        if (!overTitleScene && !charMesh.Mesh().Empty())
        {
            blupiyoupieEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            blupiyoupieEffect_->View = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            blupiyoupieEffect_->Projection = Microsoft::Xna::Framework::Matrix::CreateOrthographicOffCenter(
                0.0f, static_cast<float>(viewportW), static_cast<float>(viewportH), 0.0f, 0.0f, 1.0f);
            blupiyoupieEffect_->setAlphaProperty(charOpacity);
            blupiyoupieRenderer_ = UploadTexturedMesh(device, charMesh.Mesh());
            blupiyoupieRenderer_->Draw(device, *blupiyoupieEffect_);
            blupiyoupieEffect_->setAlphaProperty(1.0f);
        }
        FlushQuads(device, *padEffect_, padRenderer_, panelQuads, viewportW, viewportH, kInitPanelOpacity);
        FlushQuads(device, *padEffect_, padRenderer_, normalQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressedQuads, viewportW, viewportH, kPausePressedAlpha);
        FlushQuads(device, *buttonEffect_, buttonRenderer_, editorIconQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *textEffect_, textRenderer_, labelQuads, viewportW, viewportH, 1.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }
}
