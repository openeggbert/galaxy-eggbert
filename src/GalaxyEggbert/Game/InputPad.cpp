#include "InputPad.hpp"

#include <tuple>
#include <sstream>
#include "Localization.hpp"
#include "TextMetrics.hpp"
#include "TextureScale.hpp"
#include "CheatConfig.hpp"

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
#include <unordered_set>

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
        constexpr int kIconPlayDown = 23; // Pixmap.cpp: PlayDown
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

        // The Setup's "Jump button on the right" (GameData data[4], on by default): off, the stick
        // goes right and Jump and Action left (InputPad.cpp:232-240, 1718-1760: mirrored).
        Rect MirrorX(const Rect& r, const ScreenMetrics& m) noexcept
        {
            return Rect{m.w - r.x1, r.y0, m.w - r.x0, r.y1};
        }

        Microsoft::Xna::Framework::Vector2 DPadCenter(const ScreenMetrics& m, bool jumpRight) noexcept
        {
            return {jumpRight ? 100.0f * m.s : m.w - 100.0f * m.s, m.h - 100.0f * m.s};
        }

        Rect PlayJumpRect(const ScreenMetrics& m, bool jumpRight) noexcept
        {
            const Rect right{m.w - m.f1 * 1.2f, m.h - m.f1 * 1.2f, m.w - m.f1 * 0.2f, m.h - m.f1 * 0.2f};
            return jumpRight ? right : MirrorX(right, m);
        }

        Rect PlayActionRect(const ScreenMetrics& m, bool jumpRight) noexcept
        {
            const Rect right{m.w - m.f1 * 1.2f, m.h - m.f1 * 2.6f, m.w - m.f1 * 0.2f, m.h - m.f1 * 1.6f};
            return jumpRight ? right : MirrorX(right, m);
        }

        // The back-up button (Galaxy's, 2026-11-02): just over the stick's ring (owner: it was
        // too high), inside the stick's touch area, which it takes precedence over.
        Rect PlayBackUpRect(const ScreenMetrics& m, bool jumpRight) noexcept
        {
            const auto c = DPadCenter(m, jumpRight);
            const float half = 28.0f * m.s;
            const float cy = c.Y - (kDPadDrawRadius + 14.0f) * m.s - half;
            return Rect{c.X - half, cy - half, c.X + half, cy + half};
        }

        // The crouch button (the original's PlayDown): in the original it takes the stick's corner
        // when the accelerometer hides the stick (InputPad.cpp:1754-1771); Galaxy keeps its stick,
        // so the button stands beside Jump, one button further in, the same size.
        Rect PlayCrouchRect(const ScreenMetrics& m, bool jumpRight) noexcept
        {
            const Rect right{m.w - m.f1 * 2.4f, m.h - m.f1 * 1.2f, m.w - m.f1 * 1.4f, m.h - m.f1 * 0.2f};
            return jumpRight ? right : MirrorX(right, m);
        }

        // With the accelerometer (SetAccelerometerMode): Forward and Back in the stick's corner, as
        // Action and Jump stand in the other one (Back in the corner, Forward over it).
        Rect PlayForwardRect(const ScreenMetrics& m, bool jumpRight) noexcept { return PlayActionRect(m, !jumpRight); }
        Rect PlayBackRect(const ScreenMetrics& m, bool jumpRight) noexcept { return PlayJumpRect(m, !jumpRight); }

        Rect PlayFireRect(const ScreenMetrics& m, bool jumpRight) noexcept
        {
            const Rect right{m.w-m.f1*2.4f,m.h-m.f1*2.6f,m.w-m.f1*1.4f,m.h-m.f1*1.6f};
            return jumpRight ? right : MirrorX(right,m);
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
        // The original puts SetupReset right of SetupSounds (its row); since 2026-11-02 that row
        // holds the volume sliders (the owner's request), so Erase progress is a row lower.
        Rect SetupResetRect(const ScreenMetrics& m) noexcept
        {
            const Rect row = SetupJumpRect(m);
            return Rect{450.0f * m.s, row.y0, 450.0f * m.s + m.f2 * 0.5f, row.y1};
        }
        // "3D terrain" (a Galaxy setting, 2026-11-04: the reliefs or flat textures), under Erase
        // progress, at x 450 on the Zoom row.
        Rect SetupTerrainRect(const ScreenMetrics& m) noexcept
        {
            const Rect row = SetupZoomRect(m);
            return Rect{450.0f * m.s, row.y0, 450.0f * m.s + m.f2 * 0.5f, row.y1};
        }
        // "3D effects" (a Galaxy setting, 2026-11-09: fog, shadows, shaders, particles), under 3D terrain,
        // at x 450 on the Accelerometer row.
        Rect SetupEffectsRect(const ScreenMetrics& m) noexcept
        {
            const Rect row = SetupAccelRect(m);
            return Rect{450.0f * m.s, row.y0, 450.0f * m.s + m.f2 * 0.5f, row.y1};
        }
        // "Frame rate" (a Galaxy setting, 2026-11-09: 60, 120, 144, 240 frames a second): a row of the
        // left column of its own, over Sound effects, under the title. It stood in the Accelerometer row,
        // where the sensitivity slider took its place with the accelerometer on (a phone's, reported).
        Rect SetupFrameRateRect(const ScreenMetrics& m) noexcept { return SetupColumnRect(2.5f, 2.0f, m); }
        // "Textures" (a Galaxy setting, 2026-11-09, ../ray.md: auto, 1x, 2x; TextureScale.hpp), at x 450 on
        // the Frame rate row, over Erase progress; applies at the next start.
        Rect SetupTexturesRect(const ScreenMetrics& m) noexcept
        {
            const Rect row = SetupFrameRateRect(m);
            return Rect{450.0f * m.s, row.y0, 450.0f * m.s + m.f2 * 0.5f, row.y1};
        }
        // "Nickname" (a Galaxy addition, the owner, 2026-11-11): the selected player's nickname, in the
        // top-left corner as the menu's Fullscreen (the title's logo starts right of it), in the
        // main Setup with Erase progress.
        Rect SetupNicknameRect(const ScreenMetrics& m) noexcept
        {
            return Rect{20.0f * m.s, 15.0f * m.s, 150.0f * m.s, 69.0f * m.s};
        }
        // "Language" (a Galaxy addition, the owner's Q-023, 2026-11-11; BD-0126): the language's own
        // name on the blank face, a press the next language; in the top-right corner, Nickname's
        // mirror, in every Setup (the pause's too).
        Rect SetupLanguageRect(const ScreenMetrics& m) noexcept
        {
            return Rect{m.w - 150.0f * m.s, 15.0f * m.s, m.w - 20.0f * m.s, 69.0f * m.s};
        }
        // The volume sliders (a Galaxy addition, 2026-11-02): side by side in the Sounds row, right
        // of its label, the effects' (0) then the music's (1); each its label over a track drawn
        // as the original's Slider (Slider.cpp: jauge.png's empty cell, pad icon 1 for the knob).
        // which 2: the accelerometer's sensitivity, the original's accelSlider, in the Accelerometer
        // row right of its label where the effects' slider stands in the Sounds row.
        Rect SetupSliderRect(int which, const ScreenMetrics& m) noexcept
        {
            const Rect row = which == 2 ? SetupAccelRect(m) : SetupSoundsRect(m);
            const float left = 250.0f * m.s;
            const float width = (m.w - 20.0f * m.s - left) * 0.5f;
            const float x0 = left + width * static_cast<float>(which == 2 ? 0 : which);
            // The sensitivity slider ends short of the 3D effects toggle in its row.
            const float x1 = which == 2 ? std::min(x0 + width, 440.0f * m.s) : x0 + width;
            return Rect{x0 + 10.0f * m.s, row.y0, x1 - 10.0f * m.s, row.y1};
        }
        Rect SetupSliderTrackRect(int which, const ScreenMetrics& m) noexcept
        {
            const Rect block = SetupSliderRect(which, m);
            const float cy = block.y0 + (block.y1 - block.y0) * 0.70f;
            const float half = 13.0f * m.s;
            return Rect{block.x0 + 16.0f * m.s, cy - half, block.x1 - 16.0f * m.s, cy + half};
        }
        float SetupSliderValueAt(int which, float x, const ScreenMetrics& m) noexcept
        {
            const Rect track = SetupSliderTrackRect(which, m);
            const float v = (x - track.x0) / (track.x1 - track.x0);
            return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
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
        constexpr int kPlayControlBackUp = 4;
        constexpr int kPlayControlCrouch = 5;
        constexpr int kPlayControlFire = 6;
        constexpr int kPlayControlForward = 7; // the accelerometer's Forward (its Back is kPlayControlBackUp)
        constexpr int kIconPlayArrow = 7;      // pad.png's arrow (it points right): turned up and down

        constexpr int kPauseControlMenu = 0;
        constexpr int kPauseControlBack = 1;
        constexpr int kPauseControlSetup = 2;
        constexpr int kPauseControlRestart = 3;
        constexpr int kPauseControlContinue = 4;
        constexpr int kPauseControlRecord = 5;
        constexpr int kPauseControlOriginal = 6;

        constexpr int kWinLostControlReturn = 0;

        constexpr int kSetupControlSounds = 0;
        constexpr int kSetupControlJump = 1;
        constexpr int kSetupControlZoom = 2;
        constexpr int kSetupControlAccel = 3;
        constexpr int kSetupControlReset = 4;
        constexpr int kSetupControlReturn = 5;
        constexpr int kSetupControlEffectsVolume = 6;
        constexpr int kSetupControlMusicVolume = 7;
        constexpr int kSetupControlAccelSensitivity = 8;
        constexpr int kSetupControlTerrain = 9;
        constexpr int kSetupControlEffects = 10;
        constexpr int kSetupControlFrameRate = 11;
        constexpr int kSetupControlTextures = 12;
        constexpr int kSetupControlNickname = 13;
        constexpr int kSetupControlLanguage = 14;
        constexpr int kSetupSliderKnobIcon = 1; // Slider::Draw's DrawIcon(Pad, 1, ...)

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
            Rect setup, play, editor, exit, fullscreen, help, demo, galaxy, demos, multiplayer, language;
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
            l.help = Rect{20.f*s,82.f*s,130.f*s,136.f*s};
            // The language's flag (the owner, 2026-11-12): right of Help, under Fullscreen's end.
            l.language = Rect{140.0f * s, 82.0f * s, 190.0f * s, 136.0f * s};
            l.fullscreen = Rect{20.0f * s, 15.0f * s, 190.0f * s, 69.0f * s};
            l.exit = Rect{w - 130.0f * s, 15.0f * s, w - 20.0f * s, 69.0f * s};
            // Galaxy's Demo button (Speedy Blupi's demos, DemoReplay.hpp): under Exit, as Help is
            // under Fullscreen on the other side.
            l.demo = Rect{w - 130.0f * s, 82.0f * s, w - 20.0f * s, 136.0f * s};
            // The player's own galaxy (BD-0119), left of Demo.
            l.galaxy = Rect{w - 250.0f * s, 82.0f * s, w - 140.0f * s, 136.0f * s};
            // The demo positions (Export, Import, Original) without a mission first (2026-11-09):
            // left of Exit, over Galaxy.
            l.demos = Rect{w - 250.0f * s, 15.0f * s, w - 140.0f * s, 69.0f * s};
            // The network game (2026-11-10): in the left panel, right of Setup's label (the free
            // corner; over the title's letters it hid them).
            l.multiplayer = Rect{165.0f * s, h - 20.0f * s - f2 * 0.5f, 255.0f * s, h - 20.0f * s};
            l.leftPanel = Rect{10.0f * s, h - 325.0f * s, 260.0f * s, h - 10.0f * s};
            l.rightPanel = Rect{w - 170.0f * s, h - 325.0f * s, w - 10.0f * s, h - 10.0f * s};
            l.scale = s;
            return l;
        }

        constexpr int kInitPanelIcon = 15; // pad.png, same convention as Hud's DrawInfo panel

        // The version left of Play and the build's date and time under it (Galaxy's own, the owner,
        // 2026-11-11): the gamer rows' body size, the version a little larger.
        constexpr float kInitVersionScale = 0.55f;
        constexpr float kInitBuildScale = 0.45f;
        constexpr float kInitVersionGap = 10.0f;     // reference pixels left of the right panel
        constexpr float kInitVersionLineGap = 20.0f; // the version's top above Play's middle

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
        constexpr int kInitControlExit = 6;
        constexpr int kInitControlFullscreen = 7;
        constexpr int kInitControlHelp = 8;
        constexpr int kInitControlDemo = 9;
        constexpr int kInitControlGalaxy = 10;
        constexpr int kInitControlDemos = 11;
        constexpr int kInitControlMultiplayer = 12;
        constexpr int kInitControlLanguage = 13;

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
            "Content/backgrounds/galaxyeggbert.png",
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

        // 2x sheets when wanted (TextureScale.hpp); each texture remembers the scale it was loaded at.
        const auto LoadScaled = [&device](Texture2D& target, const char* path)
        {
            int scale = 1;
            target = Texture2D(ScaledContentPath(path, &scale), device);
            SetTextureScale(&target, scale);
        };
        LoadScaled(padTexture_, kPaths[0]);
        LoadScaled(pauseBgTexture_, kPaths[1]);
        LoadScaled(blupiyoupieTexture_, kPaths[2]);
        LoadScaled(winBgTexture_, kPaths[3]);
        LoadScaled(lostBgTexture_, kPaths[4]);
        LoadScaled(setupBgTexture_, kPaths[5]);
        LoadScaled(textTexture_, kPaths[6]);
        LoadScaled(waitBgTexture_, kPaths[7]);
        LoadScaled(jaugeTexture_, kPaths[8]);
        LoadScaled(initBgTexture_, kPaths[9]);
        LoadScaled(speedyblupiTexture_, kPaths[10]);
        LoadScaled(gearTexture_, kPaths[11]);
        LoadScaled(buttonTexture_, kPaths[12]);

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
        if (std::filesystem::exists("Content/icons/flags.png"))
        {
            LoadScaled(flagsTexture_, "Content/icons/flags.png");
            flagsEffect_ = makeEffect(flagsTexture_);
            flagsLoaded_ = true;
        }
        helpPanelEffect_ = std::make_unique<BasicEffect>(device);
        helpPanelEffect_->VertexColorEnabled = false;
        helpPanelEffect_->setTextureEnabledProperty(false);
        loaded_ = true;
    }

    void InputPad::ResetMissionInput() noexcept
    {
        ResetTouchState();
        dpadTouchId_ = actionTouchId_ = pauseTouchId_ = backUpTouchId_ = crouchTouchId_ = forwardTouchId_ = -1;
        cheatActiveControl_ = -1;
        cheatMouseWasDown_ = false;
        cheatGestureIndex_ = 0;
        typedCheatBuffer_.clear();
        suppressedGameplayLetters_ = 0;
        typedIdleSeconds_ = 0;
        typingCheat_ = false;
        for (auto& down : letterKeyWasDown_) down = false;
    }

    void InputPad::ResetTouchState() noexcept
    {
        activeControl_ = -1;
        previousPlayTouches_.clear();
        fireTouchHeld_=false;
        dpadTouchId_=actionTouchId_=pauseTouchId_=backUpTouchId_=crouchTouchId_=forwardTouchId_=-1;
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

    // The scale (at most @p scale) at which @p text, drawn as AppendCenteredLabel draws it, fits the
    // blank face @p face with a little margin: a translation can be wider than the English.
    float InputPad::FaceLabelScale(std::string_view text, const Rect& face, float scale) noexcept
    {
        const float room = face.x1 - face.x0 - 8.0f * scale;
        const float width = TextMetrics::Width(text) * kPauseLabelScale * scale;
        return width > room && width > 0.0f ? scale * room / width : scale;
    }

    void InputPad::AppendCenteredLabel(std::vector<Quad>& quads, std::string_view text,
                                        float centerX, float topY, float viewportScale) const
    {
        if (text.empty())
        {
            return;
        }
        const float textSheetW = LogicalWidth(textTexture_);
        const float textSheetH = LogicalHeight(textTexture_);
        const float glyphScale = kPauseLabelScale * viewportScale;
        const float cellPx = kGlyphCellPx * glyphScale;
        float penX = centerX - TextMetrics::Width(text) * glyphScale * 0.5f;
        for (std::size_t at = 0; at < text.size();)
        {
            const int rank = TextMetrics::NextGlyph(text, at);
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
            penX += TextMetrics::GlyphAdvance(rank) * glyphScale;
        }
    }

    void InputPad::AppendLeftAlignedLabel(std::vector<Quad>& quads, std::string_view text,
                                            float leftX, float centerY, float viewportScale) const
    {
        if (text.empty())
        {
            return;
        }
        const float textSheetW = LogicalWidth(textTexture_);
        const float textSheetH = LogicalHeight(textTexture_);
        const float glyphScale = kSetupLabelScale * viewportScale;
        const float cellPx = kGlyphCellPx * glyphScale;
        const float topY = centerY - kSetupLabelYNudge * viewportScale;
        float penX = leftX;
        for (std::size_t at = 0; at < text.size();)
        {
            const int rank = TextMetrics::NextGlyph(text, at);
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
            penX += TextMetrics::GlyphAdvance(rank) * glyphScale;
        }
    }

    InputPad::CheatListRect InputPad::ActiveCheatsRect(const std::vector<std::string>& names, int viewportH,
                                                       float top) noexcept
    {
        constexpr float kScale = 0.55f, kLine = 18.0f, kPad = 3.0f, kAt = 5.0f;
        const float s = static_cast<float>(viewportH) / kRefH;
        float maxWidth = 0.0f;
        for (const auto& name : names)
        {
            maxWidth = std::max(maxWidth, TextMetrics::Width(name) * kScale);
        }
        return CheatListRect{(kAt - kPad) * s, (top + kAt - kPad) * s, (kAt + maxWidth + kPad) * s,
                             (top + kAt + kLine * static_cast<float>(names.size()) + kPad) * s};
    }

    void InputPad::DrawActiveCheats(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW,
                                     int viewportH, const std::vector<std::string>& names, float top)
    {
        if (!loaded_ || names.empty())
        {
            return;
        }
        constexpr float kScale = 0.55f, kLine = 18.0f, kAt = 5.0f;
        const float s = static_cast<float>(viewportH) / kRefH;
        const CheatListRect r = ActiveCheatsRect(names, viewportH, top);
        std::vector<Quad> panel;
        {
            Quad q{r.x0, r.y0, r.x1, r.y1, 0.0f, 0.0f, 0.0f, 0.0f};
            PadIconUv(kInitPanelIcon, LogicalWidth(padTexture_),
                      LogicalHeight(padTexture_), q.u0, q.v0, q.u1, q.v1);
            panel.push_back(q);
        }
        std::vector<Quad> labels;
        float y = top + kAt;
        for (const auto& name : names)
        {
            AppendGamerLabel(labels, name, kAt * s, y * s, kScale, s);
            y += kLine;
        }
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        FlushQuads(device, *padEffect_, padRenderer_, panel, viewportW, viewportH, 0.6f);
        FlushQuads(device, *textEffect_, textRenderer_, labels, viewportW, viewportH, 1.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    void InputPad::DrawLabels(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW, int viewportH,
                              const std::vector<ScreenLabel>& labels)
    {
        if (!loaded_ || labels.empty() || viewportH <= 0)
        {
            return;
        }
        if (!whiteTextEffect_)
        {
            // text.png's glyphs are black: a white copy takes any colour from the effect's diffuse.
            const int w = textTexture_.getWidthProperty();
            const int h = textTexture_.getHeightProperty();
            std::vector<Microsoft::Xna::Framework::Color> texels(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
            textTexture_.GetData(texels.data(), static_cast<int>(texels.size()));
            for (auto& texel : texels)
            {
                texel = Microsoft::Xna::Framework::Color(255, 255, 255, static_cast<int>(texel.getAProperty()));
            }
            whiteTextTexture_ = std::make_unique<Microsoft::Xna::Framework::Graphics::Texture2D>(device, w, h);
            whiteTextTexture_->SetData(texels.data(), static_cast<int>(texels.size()));
            SetTextureScale(whiteTextTexture_.get(), TextureScaleOf(&textTexture_));
            whiteTextEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
            whiteTextEffect_->VertexColorEnabled = false;
            whiteTextEffect_->setTextureEnabledProperty(true);
            whiteTextEffect_->setTextureProperty(whiteTextTexture_.get());
        }
        constexpr float kScale = 0.55f;
        const float s = static_cast<float>(viewportH) / kRefH;
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        for (const auto& label : labels)
        {
            const float width = static_cast<float>(TextMetrics::Width(label.text)) * kScale * s;
            const float height = kGlyphCellPx * kScale * s;
            std::vector<Quad> glyphs;
            AppendGamerLabel(glyphs, label.text, label.x - width * 0.5f, label.y - height * 0.5f, kScale, s);
            whiteTextEffect_->setDiffuseColorProperty(label.colour);
            FlushQuads(device, *whiteTextEffect_, textRenderer_, glyphs, viewportW, viewportH, 1.0f);
        }
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    void InputPad::DrawDebug(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW,
                             int viewportH, const std::vector<std::string>& lines, bool touchControls)
    {
        if (!loaded_ || !kCheatsEnabled || lines.empty() || viewportW <= 0 || viewportH <= 0) return;
        const float s = std::min(float(viewportW) / kRefW, float(viewportH) / kRefH);
        constexpr float font = .5f, lineHeight = 16.f, padding = 5.f;
        constexpr float width = 316.f; // fixed anchor, independent of changing digits/modes
        // Centre the touch panel between the side controls, clear of Action as well as Pause.
        const float right = touchControls ? viewportW * .5f + width * s * .5f : viewportW - padding * s;
        const float left = right - width * s;
        const float top = touchControls ? 78.f * float(viewportH) / kRefH : 5.f * s;
        const std::vector<Quad> panel{{left - padding * s, top - 3 * s, right + 3 * s,
            top + (lines.size() * lineHeight + 3) * s, 0, 0, 1, 1}};
        std::vector<Quad> text;
        const auto numeric = [&](const std::string& value,float x,float y) {
            for (std::size_t i=0;i<value.size();++i)
                AppendGamerLabel(text,std::string(1,value[i]),x+float(i)*9.5f*s,y,font,s);
        };
        for (std::size_t i=0;i<lines.size();++i)
        {
            const float y=top+float(i)*lineHeight*s;
            const auto colon=lines[i].find(':');
            if (colon==std::string::npos) { AppendGamerLabel(text,lines[i],left,y,font,s); continue; }
            const auto label=lines[i].substr(0,colon);
            auto value=lines[i].substr(colon+2);
            AppendGamerLabel(text,label=="vel" ? "vel b/s:" : label+":",left,y,font,s);
            if (label=="pos" || label=="cell" || label=="vel")
            {
                std::istringstream parts(value);
                for (int column=0;column<3;++column)
                {
                    std::string token;
                    parts>>token;
                    numeric(token,left+(70.f+80.f*column)*s,y);
                }
                continue;
            }
            std::string secondLabel,secondValue;
            if (label=="time")
            {
                const auto ticks=value.find(" (");
                if (ticks!=std::string::npos)
                {
                    secondLabel="ticks:";
                    secondValue=value.substr(ticks+2,value.find(" ticks",ticks)-ticks-2);
                    value.resize(ticks);
                }
            }
            else if (const auto next=value.find("  "); next!=std::string::npos)
            {
                const auto nextColon=value.find(':',next+2);
                if (nextColon!=std::string::npos)
                {
                    secondLabel=value.substr(next+2,nextColon-next-1);
                    secondValue=value.substr(nextColon+2);
                    value.resize(next);
                }
            }
            if (label=="mode" || label=="gamepad") AppendGamerLabel(text,value,left+70.f*s,y,font,s);
            else numeric(value,left+70.f*s,y);
            if (!secondLabel.empty())
            {
                AppendGamerLabel(text,secondLabel,left+160.f*s,y,font,s);
                if (label=="gamepad") AppendGamerLabel(text,secondValue,left+218.f*s,y,font,s);
                else numeric(secondValue,left+218.f*s,y);
            }
        }
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        FlushQuads(device, *helpPanelEffect_, helpPanelRenderer_, panel, viewportW, viewportH, .72f);
        const auto diffuse = textEffect_->getDiffuseColorProperty();
        textEffect_->setDiffuseColorProperty({0.f, 0.f, 0.f});
        FlushQuads(device, *textEffect_, textRenderer_, text, viewportW, viewportH, 1.f);
        textEffect_->setDiffuseColorProperty(diffuse);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    void InputPad::DrawControlsHelp(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                    int viewportW, int viewportH, bool cheats)
    {
        if (!loaded_ || viewportW <= 0 || viewportH <= 0) return;
        const float s = std::min(float(viewportW) / kRefW, float(viewportH) / kRefH);
        const float ox = (float(viewportW) - kRefW * s) * 0.5f;
        const float oy = (float(viewportH) - kRefH * s) * 0.5f;
        const std::vector<Quad> panel{{ox + 10 * s, oy + 10 * s, ox + 630 * s, oy + 470 * s,
                                       0, 0, 1, 1}};
        std::vector<Quad> text;
        // A line narrower than @p room (reference pixels) when one is given: a translation longer
        // than the English is drawn smaller rather than over the other column.
        const auto line = [&](std::string_view label, float x, float y, float size = 0.48f, float room = 0.0f) {
            const float width = TextMetrics::Width(label);
            if (room > 0.0f && width * size > room) size = room / width;
            AppendGamerLabel(text, label, ox + x * s, oy + y * s, size, s);
        };
        constexpr float kLeftRoom = 300.0f;  // 28 .. 328, the right column starting at 335
        constexpr float kRightRoom = 285.0f; // 335 .. 620, the panel's right edge at 630
        if (cheats)
        {
            // The cheat list stays English (the owner's Q-023: a tester's tool, as the debug panel).
            line("Galaxy Eggbert - Cheats", 28, 22, 0.85f);
            line("Type a word during play (no Enter needed).", 28, 54, 0.45f);
            int count = 0;
            const auto* words = TypedCheatWords(count);
            const int perColumn = (count + 1) / 2;
            for (int i = 0; i < count; ++i)
            {
                const float x = i < perColumn ? 28.0f : 335.0f;
                const float y = 78.0f + float(i % perColumn) * std::min(27.0f, 350.0f / perColumn);
                line(words[i].word, x, y, perColumn > 13 ? 0.44f : 0.48f);
                line(words[i].description, x, y + 13.0f, perColumn > 13 ? 0.37f : 0.4f);
            }
        }
        else
        {
            line(Tr("Galaxy Eggbert - Controls"), 28, 22, 0.85f, 500.0f);
            line(Tr("KEYBOARD"), 28, 62, 0.65f, kLeftRoom);
            const char* keyboard[] = {
                TrMark("W / Up: walk forward"),
                TrMark("A D / Left Right: turn"),
                TrMark("S / Down: turn around, then walk"),
                TrMark("Left Ctrl: jump / helicopter up"),
                TrMark("Space: action / enter / leave"),
                TrMark("F: vehicle glue; Ctrl: tank glue"),
                TrMark("X / Right Ctrl: back up"),
                TrMark("Left Shift: crouch / dive"),
                TrMark("Crouch + S / Down: pull crate"),
                TrMark("Right Shift: look / helicopter up"),
                TrMark("Esc: pause / continue"),
                TrMark("Enter: Play / Continue"),
                TrMark("F1: show / hide this help"),
                TrMark("F3: first / third person"),
                TrMark("F11: full screen"),
                TrMark("F5 / F4 / F6: speed 1x / 1.5x / 2x"),
                TrMark("Tab: slow / normal speed"),
                TrMark("F12: cheat menu"),
                TrMark("F7 / F8: faster (quick cheat)")};
            float y = 90;
            for (const char* label : keyboard)
            {
                if (!kCheatsEnabled && (std::string_view(label).starts_with("F12") || std::string_view(label).starts_with("F7"))) continue;
                line(Tr(label), 28, y, 0.48f, kLeftRoom); y += 17;
            }
            line(Tr("GAMEPAD"), 335, 62, 0.65f, kRightRoom);
            const char* gamepad[] = {
                TrMark("Left stick / D-pad: walk, turn"),
                TrMark("Down: turn around, then walk"),
                TrMark("A: jump / helicopter up"),
                TrMark("A: Play / Continue in menus"),
                TrMark("B / X: action / enter / leave"),
                TrMark("RT: vehicle glue shot / drop"),
                TrMark("RB: back up"),
                TrMark("LB: crouch / dive"),
                TrMark("LB + Down: pull crate"),
                TrMark("Y: look up"),
                TrMark("Start / Back: pause / continue")};
            y = 90;
            for (const char* label : gamepad) { line(Tr(label), 335, y, 0.48f, kRightRoom); y += 17; }
            line(Tr("MOUSE"), 335, 285, 0.65f, kRightRoom);
            line(Tr("Left click: menu buttons"), 335, 313, 0.48f, kRightRoom);
            line(Tr("Hold left + drag: look around"), 335, 330, 0.48f, kRightRoom);
            line(Tr("Walking returns the camera"), 335, 347, 0.48f, kRightRoom);
            line(Tr("Touch Fire: tank / helicopter glue"), 335, 366, 0.48f, kRightRoom);
            line(Tr("Touch controls: touch screens only"), 335, 383, 0.48f, kRightRoom);
            line(kCheatsEnabled ? Tr("Type help: cheats explained") : Tr("Cheats disabled"), 335, 405, 0.48f, kRightRoom);
        }
        line(Tr("[Close]"), 550, 25, .55f, 75.0f);
        line(Tr("Game paused - F1 or Esc to close"), 28, 439, 0.6f, 590.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        FlushQuads(device, *helpPanelEffect_, helpPanelRenderer_, panel, viewportW, viewportH, 0.87f);
        const auto diffuse = textEffect_->getDiffuseColorProperty();
        textEffect_->setDiffuseColorProperty({0.0f, 0.0f, 0.0f});
        FlushQuads(device, *textEffect_, textRenderer_, text, viewportW, viewportH, 1.0f);
        textEffect_->setDiffuseColorProperty(diffuse);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    void InputPad::AppendGamerLabel(std::vector<Quad>& quads, std::string_view text, float leftX, float topY,
                                      float labelScale, float viewportScale) const
    {
        if (text.empty())
        {
            return;
        }
        const float textSheetW = LogicalWidth(textTexture_);
        const float textSheetH = LogicalHeight(textTexture_);
        const float glyphScale = labelScale * viewportScale;
        const float cellPx = kGlyphCellPx * glyphScale;
        float penX = leftX;
        for (std::size_t at = 0; at < text.size();)
        {
            const int rank = TextMetrics::NextGlyph(text, at);
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
            penX += TextMetrics::GlyphAdvance(rank) * glyphScale;
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

        const float charW = LogicalWidth(blupiyoupieTexture_);
        const float charH = LogicalHeight(blupiyoupieTexture_);

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

        const auto padCenter = DPadCenter(m, jumpRight_);
        const float padHalf = kDPadRadius * m.s;
        const bool overDPad = !accelMode_ && InRect(mouseX, mouseY, Rect{padCenter.X - padHalf, padCenter.Y - padHalf,
                                                                         padCenter.X + padHalf, padCenter.Y + padHalf});
        const bool overJump = InRect(mouseX, mouseY, PlayJumpRect(m, jumpRight_));
        const bool overAction = InRect(mouseX, mouseY, PlayActionRect(m, jumpRight_));
        const bool overPause = InRect(mouseX, mouseY, PlayPauseRect(m));
        const bool overBackUp = InRect(mouseX, mouseY, accelMode_ ? PlayBackRect(m, jumpRight_) : PlayBackUpRect(m, jumpRight_));
        const bool overForward = accelMode_ && InRect(mouseX, mouseY, PlayForwardRect(m, jumpRight_));
        const bool overCrouch = InRect(mouseX, mouseY, PlayCrouchRect(m, jumpRight_));
        const bool overFire = vehicleFireEnabled_ && InRect(mouseX,mouseY,PlayFireRect(m,jumpRight_));

        if (mouseDown && !mouseWasDown_)
        {
            if (overFire) activeControl_ = kPlayControlFire;
            else if (overBackUp) activeControl_ = kPlayControlBackUp;
            else if (overForward) activeControl_ = kPlayControlForward;
            else if (overCrouch) activeControl_ = kPlayControlCrouch;
            else if (overDPad) activeControl_ = kPlayControlDPad;
            else if (overJump) activeControl_ = kPlayControlJump;
            else if (overAction) activeControl_ = kPlayControlAction;
            else if (overPause) activeControl_ = kPlayControlPause;
            else activeControl_ = -1;
        }

        // Real level-triggered Jump: fires every frame the pointer is
        // currently inside its rect while the mouse is down, independent
        // of where the press started (see InputPad.hpp's class comment).
        outInput.jumpHeld = mouseDown && overJump;
        outInput.fireHeld = mouseDown && overFire;
        outInput.backUpHeld = mouseDown && activeControl_ == kPlayControlBackUp;
        outInput.crouchHeld = mouseDown && activeControl_ == kPlayControlCrouch;
        if (mouseDown && activeControl_ == kPlayControlForward) outInput.moveInput = 1.0f;

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

    bool InputPad::UpdatePlayTouches(const std::vector<TouchPoint>& contacts, int viewportW, int viewportH,
                                       PlayInput& outInput) noexcept
    {
        auto touches=contacts;
        for (auto& t : touches)
            if (!t.released && std::none_of(previousPlayTouches_.begin(),previousPlayTouches_.end(),
                [&](const auto& previous) { return previous.id==t.id; })) t.pressed=true;
        for (auto t : previousPlayTouches_)
            if (std::none_of(contacts.begin(),contacts.end(),[&](const auto& now) { return now.id==t.id; }))
            { t.pressed=false;t.released=true;touches.push_back(t); }
        previousPlayTouches_.clear();
        for (const auto& t : contacts) if (!t.released) previousPlayTouches_.push_back(t);
        outInput = PlayInput{};
        const ScreenMetrics m = Metrics(viewportW, viewportH);
        const auto padCenter = DPadCenter(m, jumpRight_);
        const float padHalf = kDPadRadius * m.s;
        const Rect padRect{padCenter.X - padHalf, padCenter.Y - padHalf, padCenter.X + padHalf, padCenter.Y + padHalf};
        const Rect jumpRect = PlayJumpRect(m, jumpRight_);
        const Rect actionRect = PlayActionRect(m, jumpRight_);
        const Rect pauseRect = PlayPauseRect(m);
        const Rect backRect = accelMode_ ? PlayBackRect(m, jumpRight_) : PlayBackUpRect(m, jumpRight_);
        const Rect forwardRect = PlayForwardRect(m, jumpRight_);
        const Rect crouchRect = PlayCrouchRect(m, jumpRight_);
        bool onControl = false;
        bool dpadSeen = false;
        for (const auto& t : touches)
        {
            if (t.pressed)
            {
                if (InRect(t.x, t.y, backRect) && backUpTouchId_ < 0) backUpTouchId_ = t.id;
                else if (accelMode_ && InRect(t.x, t.y, forwardRect) && forwardTouchId_ < 0) forwardTouchId_ = t.id;
                else if (InRect(t.x, t.y, crouchRect) && crouchTouchId_ < 0) crouchTouchId_ = t.id;
                else if (!accelMode_ && InRect(t.x, t.y, padRect) && dpadTouchId_ < 0) dpadTouchId_ = t.id;
                else if (InRect(t.x, t.y, actionRect)) actionTouchId_ = t.id;
                else if (InRect(t.x, t.y, pauseRect)) pauseTouchId_ = t.id;
            }
            const bool down = !t.released;
            if (down && InRect(t.x, t.y, jumpRect))
            {
                outInput.jumpHeld = true;
                onControl = true;
            }
            if (vehicleFireEnabled_ && down && InRect(t.x,t.y,PlayFireRect(m,jumpRight_)))
            { outInput.fireHeld=true;onControl=true; }
            if (t.id == dpadTouchId_)
            {
                dpadSeen = true;
                onControl = true;
                if (down)
                {
                    const float dx = t.x - padCenter.X;
                    const float dy = t.y - padCenter.Y;
                    const float threshold = kDPadThreshold * m.s;
                    outInput.turnInput = dx > threshold ? 1.0f : (dx < -threshold ? -1.0f : 0.0f);
                    outInput.moveInput = dy < -threshold ? 1.0f : (dy > threshold ? -1.0f : 0.0f);
                    dpadDragOffsetX_ = std::clamp(dx, -padHalf, padHalf) / m.s;
                    dpadDragOffsetY_ = std::clamp(dy, -padHalf, padHalf) / m.s;
                }
                else
                {
                    dpadTouchId_ = -1;
                }
            }
            if (t.id == backUpTouchId_)
            {
                onControl = true;
                if (down) outInput.backUpHeld = true;
                else backUpTouchId_ = -1;
            }
            if (t.id == crouchTouchId_)
            {
                onControl = true;
                if (down) outInput.crouchHeld = true;
                else crouchTouchId_ = -1;
            }
            if (t.id == forwardTouchId_)
            {
                onControl = true;
                if (down) outInput.moveInput = 1.0f;
                else forwardTouchId_ = -1;
            }
            if (t.id == actionTouchId_)
            {
                onControl = true;
                if (t.released)
                {
                    outInput.actionPressed = true;
                    actionTouchId_ = -1;
                }
            }
            if (t.id == pauseTouchId_)
            {
                onControl = true;
                if (t.released)
                {
                    outInput.pausePressed = true;
                    pauseTouchId_ = -1;
                }
            }
        }
        if (!dpadSeen)
        {
            // Its finger is gone (no release reported): let go of the stick.
            dpadTouchId_ = -1;
            dpadDragOffsetX_ = 0.0f;
            dpadDragOffsetY_ = 0.0f;
        }
        const auto present = [&touches](int id) {
            return std::any_of(touches.begin(), touches.end(), [id](const TouchPoint& t) { return t.id == id; });
        };
        if (backUpTouchId_ >= 0 && !present(backUpTouchId_)) backUpTouchId_ = -1;
        if (crouchTouchId_ >= 0 && !present(crouchTouchId_)) crouchTouchId_ = -1;
        if (forwardTouchId_ >= 0 && (!accelMode_ || !present(forwardTouchId_))) forwardTouchId_ = -1;
        if (actionTouchId_ >= 0 && !present(actionTouchId_)) actionTouchId_ = -1;
        if (pauseTouchId_ >= 0 && !present(pauseTouchId_)) pauseTouchId_ = -1;
        fireTouchHeld_=outInput.fireHeld;
        return onControl;
    }

    void InputPad::DrawPlay(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                              int viewportW, int viewportH)
    {
        if (!loaded_)
        {
            return;
        }

        const ScreenMetrics m = Metrics(viewportW, viewportH);

        const float padSheetW = LogicalWidth(padTexture_);
        const float padSheetH = LogicalHeight(padTexture_);

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
        const auto padCenter = DPadCenter(m, jumpRight_);
        const float drawHalf = kDPadDrawRadius * m.s;
        if (!accelMode_)
        {
            appendIconQuad(normalQuads, padCenter.X - drawHalf, padCenter.Y - drawHalf, padCenter.X + drawHalf,
                           padCenter.Y + drawHalf, kIconDPadRing);
            const float thumbCx = padCenter.X + dpadDragOffsetX_ * m.s;
            const float thumbCy = padCenter.Y + dpadDragOffsetY_ * m.s;
            appendIconQuad(activeControl_ == kPlayControlDPad ? pressedQuads : normalQuads,
                           thumbCx - drawHalf, thumbCy - drawHalf, thumbCx + drawHalf, thumbCy + drawHalf,
                           kIconDPadThumb);
        }
        else
        {
            // Forward (the arrow turned up) and Back (turned down) in the stick's corner.
            const auto appendArrow = [&](bool pressed, const Rect& r, float rotation)
            {
                appendIconQuad(pressed ? pressedQuads : normalQuads, r.x0, r.y0, r.x1, r.y1, kIconPlayArrow);
                (pressed ? pressedQuads : normalQuads).back().rotation = rotation;
            };
            appendArrow(activeControl_ == kPlayControlForward || forwardTouchId_ >= 0, PlayForwardRect(m, jumpRight_), -90.0f);
            appendArrow(activeControl_ == kPlayControlBackUp || backUpTouchId_ >= 0, PlayBackRect(m, jumpRight_), 90.0f);
        }

        const auto appendRect = [&](std::vector<Quad>& bucket, const Rect& r, int icon)
        {
            appendIconQuad(bucket, r.x0, r.y0, r.x1, r.y1, icon);
        };
        appendRect(activeControl_ == kPlayControlJump ? pressedQuads : normalQuads, PlayJumpRect(m, jumpRight_), kIconPlayJump);
        appendRect(activeControl_ == kPlayControlAction ? pressedQuads : normalQuads, PlayActionRect(m, jumpRight_),
                   kIconPlayAction);
        appendRect(activeControl_ == kPlayControlPause ? pressedQuads : normalQuads, PlayPauseRect(m),
                   kIconPlayPause);
        // The back-up button (Galaxy's, 2026-11-02), the Setup's return arrow over the stick; with the
        // accelerometer the Back button is it.
        if (!accelMode_)
            appendRect(activeControl_ == kPlayControlBackUp || backUpTouchId_ >= 0 ? pressedQuads : normalQuads,
                       PlayBackUpRect(m, jumpRight_), kIconSetupReturn);
        appendRect(activeControl_ == kPlayControlCrouch || crouchTouchId_ >= 0 ? pressedQuads : normalQuads,
                   PlayCrouchRect(m, jumpRight_), kIconPlayDown);

        if (vehicleFireEnabled_)
            appendRect(activeControl_==kPlayControlFire || fireTouchHeld_ ? pressedQuads : normalQuads,
                       PlayFireRect(m,jumpRight_),kIconPlayAction);
        // NonPremultiplied, not AlphaBlend: Texture2D(path) loads PNGs with
        // straight alpha, while AlphaBlend expects the premultiplied colours
        // CNA's (and XNA's) content pipeline produces, which Mobile Eggbert
        // draws. With AlphaBlend every translucent pixel came out too bright:
        // the menu's dark blue button panel showed whitish (user report,
        // 2026-10-25).
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        FlushQuads(device, *padEffect_, padRenderer_, normalQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressedQuads, viewportW, viewportH, kPlayPressedAlpha);
        if (vehicleFireEnabled_)
        {
            const auto r=PlayFireRect(m,jumpRight_);
            std::vector<Quad> labels;
            const float font=.55f;
            const std::string_view fire=Tr("Fire");
            AppendGamerLabel(labels,fire,(r.x0+r.x1)*.5f-TextMetrics::Width(fire)*font*m.s*.5f,
                             (r.y0+r.y1)*.5f-12*font*m.s,font,m.s);
            FlushQuads(device,*textEffect_,textRenderer_,labels,viewportW,viewportH,1);
        }
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    QuadBatch::Rect InputPad::PauseRecordRect(int viewportW, int viewportH) noexcept
    {
        const float w = static_cast<float>(viewportW);
        const float s = static_cast<float>(viewportH) / kRefH;
        return Rect{w - 130.0f * s, 15.0f * s, w - 20.0f * s, 69.0f * s};
    }

    QuadBatch::Rect InputPad::PauseOriginalRect(int viewportW, int viewportH) noexcept
    {
        const float w = static_cast<float>(viewportW);
        const float s = static_cast<float>(viewportH) / kRefH;
        return Rect{w - 250.0f * s, 15.0f * s, w - 140.0f * s, 69.0f * s};
    }

    QuadBatch::Rect InputPad::DemoSlotRect(int index, int viewportW, int viewportH) noexcept
    {
        // Two rows of five, centred: the number on a blank button, two lines under it.
        const float s = static_cast<float>(viewportH) / kRefH;
        const float cx = static_cast<float>(viewportW) * 0.5f;
        const int col = index % 5;
        const int row = index / 5;
        const float x0 = cx + (static_cast<float>(col) * 124.0f - 302.0f) * s;
        const float y0 = (row == 0 ? 78.0f : 232.0f) * s;
        return Rect{x0, y0, x0 + 108.0f * s, y0 + 64.0f * s};
    }

    QuadBatch::Rect InputPad::DemoRestoreRect(int index, int viewportW, int viewportH) noexcept
    {
        const Rect slot = DemoSlotRect(index, viewportW, viewportH);
        const float s = static_cast<float>(viewportH) / kRefH;
        return Rect{slot.x0, slot.y1 + 46.0f * s, slot.x1, slot.y1 + 76.0f * s};
    }

    // The bottom row (2026-11-10, four buttons): Export, Import, Name, Cancel.
    QuadBatch::Rect InputPad::DemoCancelRect(int viewportW, int viewportH) noexcept
    {
        const float s = static_cast<float>(viewportH) / kRefH;
        const float cx = static_cast<float>(viewportW) * 0.5f;
        return Rect{cx + 170.0f * s, 412.0f * s, cx + 300.0f * s, 466.0f * s};
    }

    QuadBatch::Rect InputPad::DemoExportRect(int viewportW, int viewportH) noexcept
    {
        const float s = static_cast<float>(viewportH) / kRefH;
        const float cx = static_cast<float>(viewportW) * 0.5f;
        return Rect{cx - 300.0f * s, 412.0f * s, cx - 170.0f * s, 466.0f * s};
    }

    QuadBatch::Rect InputPad::DemoNameRect(int viewportW, int viewportH) noexcept
    {
        const float s = static_cast<float>(viewportH) / kRefH;
        const float cx = static_cast<float>(viewportW) * 0.5f;
        return Rect{cx + 20.0f * s, 412.0f * s, cx + 150.0f * s, 466.0f * s};
    }

    QuadBatch::Rect InputPad::DemoKeyboardRect(int viewportW, int viewportH) noexcept
    {
        const float s = static_cast<float>(viewportH) / kRefH;
        const float cx = static_cast<float>(viewportW) * 0.5f;
        return Rect{cx - 300.0f * s, 118.0f * s, cx + 300.0f * s, 372.0f * s};
    }

    QuadBatch::Rect InputPad::DemoListRowRect(int index, int viewportW, int viewportH) noexcept
    {
        const float s = static_cast<float>(viewportH) / kRefH;
        const float cx = static_cast<float>(viewportW) * 0.5f;
        const float y0 = (72.0f + static_cast<float>(index) * 38.0f) * s;
        return Rect{cx - 300.0f * s, y0, cx + 300.0f * s, y0 + 32.0f * s};
    }

    QuadBatch::Rect InputPad::DemoListDeleteRect(int index, int viewportW, int viewportH) noexcept
    {
        const float s = static_cast<float>(viewportH) / kRefH;
        const Rect row = DemoListRowRect(index, viewportW, viewportH);
        return Rect{row.x1 - 34.0f * s, row.y0 + 3.0f * s, row.x1 - 8.0f * s, row.y1 - 3.0f * s};
    }

    QuadBatch::Rect InputPad::DemoListPrevRect(int viewportW, int viewportH) noexcept
    {
        const float s = static_cast<float>(viewportH) / kRefH;
        const float cx = static_cast<float>(viewportW) * 0.5f;
        return Rect{cx - 300.0f * s, 342.0f * s, cx - 230.0f * s, 374.0f * s};
    }

    QuadBatch::Rect InputPad::DemoListNextRect(int viewportW, int viewportH) noexcept
    {
        const float s = static_cast<float>(viewportH) / kRefH;
        const float cx = static_cast<float>(viewportW) * 0.5f;
        return Rect{cx + 230.0f * s, 342.0f * s, cx + 300.0f * s, 374.0f * s};
    }

    QuadBatch::Rect InputPad::DemoImportRect(int viewportW, int viewportH) noexcept
    {
        const float s = static_cast<float>(viewportH) / kRefH;
        const float cx = static_cast<float>(viewportW) * 0.5f;
        return Rect{cx - 150.0f * s, 412.0f * s, cx - 20.0f * s, 466.0f * s};
    }

    InputPad::DemoSlotsInput InputPad::UpdateDemoSlots(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                                       int viewportW, int viewportH,
                                                       const std::vector<DemoSlotInfo>& slots, int listRows,
                                                       bool paged, const ScreenKeyboard* keyboard) noexcept
    {
        using Microsoft::Xna::Framework::Input::ButtonState;
        DemoSlotsInput result;
        const float x = static_cast<float>(mouse.getXProperty());
        const float y = static_cast<float>(mouse.getYProperty());
        const bool down = mouse.getLeftButtonProperty() == ButtonState::Pressed;
        // Controls: 0-9 the slots, 10-19 their Original buttons, 20 Cancel, 21 Export, 22 Import,
        // 23 Name, 30-36 the list's rows, 40-46 their bins (the list replaces the slots), 50/51 its
        // arrows; while naming only the keyboard's keys, 100 and up.
        if (keyboard != nullptr)
        {
            const int key = keyboard->KeyAt(x, y);
            if (down && !mouseWasDown_) demoSlotsControl_ = key >= 0 ? 100 + key : -1;
            if (!down && mouseWasDown_)
            {
                if (demoSlotsControl_ >= 100 && demoSlotsControl_ == 100 + key) result.keyboardKey = key;
                else if (demoSlotsControl_ < 0 && !keyboard->Holds(x, y)) result.outsideClick = true;
                demoSlotsControl_ = -1;
            }
            mouseWasDown_ = down;
            return result;
        }
        const auto over = [&]() {
            for (int i = 0; i < listRows && i < kDemoListRows; ++i)
                if (InRect(x, y, DemoListDeleteRect(i, viewportW, viewportH))) return 40 + i;
            for (int i = 0; i < listRows && i < kDemoListRows; ++i)
                if (InRect(x, y, DemoListRowRect(i, viewportW, viewportH))) return 30 + i;
            if (paged && InRect(x, y, DemoListPrevRect(viewportW, viewportH))) return 50;
            if (paged && InRect(x, y, DemoListNextRect(viewportW, viewportH))) return 51;
            for (int i = 0; listRows < 0 && i < static_cast<int>(slots.size()) && i < 10; ++i)
            {
                if (InRect(x, y, DemoSlotRect(i, viewportW, viewportH))) return i;
                if (slots[static_cast<std::size_t>(i)].recorded && InRect(x, y, DemoRestoreRect(i, viewportW, viewportH)))
                    return 10 + i;
            }
            if (InRect(x, y, DemoCancelRect(viewportW, viewportH))) return 20;
            if (InRect(x, y, DemoExportRect(viewportW, viewportH))) return 21;
            if (InRect(x, y, DemoNameRect(viewportW, viewportH))) return 23;
            return InRect(x, y, DemoImportRect(viewportW, viewportH)) ? 22 : -1;
        };
        if (down && !mouseWasDown_) demoSlotsControl_ = over();
        if (!down && mouseWasDown_)
        {
            if (demoSlotsControl_ >= 0 && demoSlotsControl_ == over())
            {
                if (demoSlotsControl_ < 10) result.recordSlot = demoSlotsControl_ + 1;
                else if (demoSlotsControl_ < 20) result.restoreSlot = demoSlotsControl_ - 9;
                else if (demoSlotsControl_ == 20) result.cancelPressed = true;
                else if (demoSlotsControl_ == 21) result.exportPressed = true;
                else if (demoSlotsControl_ == 22) result.importPressed = true;
                else if (demoSlotsControl_ == 23) result.namePressed = true;
                else if (demoSlotsControl_ < 40) result.importRow = demoSlotsControl_ - 29;
                else if (demoSlotsControl_ < 50) result.deleteRow = demoSlotsControl_ - 39;
                else if (demoSlotsControl_ == 50) result.prevPage = true;
                else result.nextPage = true;
            }
            demoSlotsControl_ = -1;
        }
        mouseWasDown_ = down;
        return result;
    }

    void InputPad::DrawDemoSlots(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW,
                                 int viewportH, const std::vector<DemoSlotInfo>& slots, const std::string& message,
                                 const std::string& title, int activeMode, const std::vector<std::string>* listRows,
                                 int armedRow, const std::string& pageLabel, const DemoSlotsExtras& extras)
    {
        if (!loaded_)
        {
            return;
        }
        const float s = static_cast<float>(viewportH) / kRefH;
        const float w = static_cast<float>(viewportW);
        const float padSheetW = LogicalWidth(padTexture_);
        const float padSheetH = LogicalHeight(padTexture_);
        std::vector<Quad> panels, normal, pressed, labels, faint;
        const auto icon = [&](std::vector<Quad>& bucket, const Rect& r, int index) {
            Quad q{r.x0, r.y0, r.x1, r.y1, 0.0f, 0.0f, 0.0f, 0.0f};
            PadIconUv(index, padSheetW, padSheetH, q.u0, q.v0, q.u1, q.v1);
            bucket.push_back(q);
        };
        const auto centred = [&](std::vector<Quad>& bucket, std::string_view text, float cx, float top, float size) {
            AppendGamerLabel(bucket, text, cx - TextMetrics::Width(text) * size * s * 0.5f, top, size, s);
        };
        icon(panels, Rect{w * 0.5f - 320.0f * s, 20.0f * s, w * 0.5f + 320.0f * s, 472.0f * s}, kInitPanelIcon);
        centred(labels, title, w * 0.5f, 34.0f * s, 0.7f);
        // A text the width of @p room at most (cut, "..." at its end).
        const auto fit = [&](std::string text, float size, float room) {
            if (TextMetrics::Width(text) * size * s <= room) return text;
            while (!text.empty() && TextMetrics::Width(text + "...") * size * s > room) TextMetrics::PopBack(text);
            return text + "...";
        };
        if (extras.keyboard != nullptr)
        {
            // Naming a recording: the name over the keyboard, in place of the positions.
            const Rect field{w * 0.5f - 300.0f * s, 74.0f * s, w * 0.5f + 300.0f * s, 110.0f * s};
            icon(normal, field, kInitPanelIcon);
            AppendGamerLabel(labels, fit(TrFormat("Name: {0}", {extras.draft + "_"}), 0.55f, field.x1 - field.x0 - 20.0f * s),
                             field.x0 + 10.0f * s, field.y0 + 7.0f * s, 0.55f, s);
            for (std::size_t i = 0; i < extras.keyboard->Keys().size(); ++i)
            {
                const auto& key = extras.keyboard->Keys()[i];
                const bool lit = demoSlotsControl_ == 100 + static_cast<int>(i) ||
                                 (key.kind == ScreenKeyboard::Kind::Shift && extras.keyboard->Shifted());
                icon(lit ? pressed : normal, key.rect, 21);
                const std::string face = extras.keyboard->Label(key);
                const float size = face.size() == 1 ? 0.6f : 0.42f;
                centred(labels, face, (key.rect.x0 + key.rect.x1) * 0.5f,
                        (key.rect.y0 + key.rect.y1) * 0.5f - 16.0f * size * s, size);
            }
        }
        std::vector<Quad> rows; // a calm ground under each row (the panel's own picture)
        for (int i = 0; listRows != nullptr && i < static_cast<int>(listRows->size()) && i < kDemoListRows; ++i)
        {
            const Rect r = DemoListRowRect(i, viewportW, viewportH);
            icon(demoSlotsControl_ == 30 + i ? pressed : rows, r, demoSlotsControl_ == 30 + i ? 21 : kInitPanelIcon);
            icon(demoSlotsControl_ == 40 + i ? pressed : normal, DemoListDeleteRect(i, viewportW, viewportH),
                 armedRow == i ? 13 : 20); // the tick, the bin
            std::string row = armedRow == i ? std::string(Tr("Delete it? Click its bin again.")) : (*listRows)[static_cast<std::size_t>(i)];
            while (!row.empty() && TextMetrics::Width(row) * 0.45f * s > r.x1 - r.x0 - 52.0f * s) TextMetrics::PopBack(row);
            AppendGamerLabel(labels, row, r.x0 + 10.0f * s, r.y0 + 6.0f * s, 0.45f, s);
        }
        if (listRows != nullptr && !pageLabel.empty())
        {
            const Rect prev = DemoListPrevRect(viewportW, viewportH), next = DemoListNextRect(viewportW, viewportH);
            icon(demoSlotsControl_ == 50 ? pressed : normal, prev, 21);
            icon(demoSlotsControl_ == 51 ? pressed : normal, next, 21);
            centred(labels, "<", (prev.x0 + prev.x1) * 0.5f, prev.y0 + 6.0f * s, 0.6f);
            centred(labels, ">", (next.x0 + next.x1) * 0.5f, next.y0 + 6.0f * s, 0.6f);
            centred(labels, pageLabel, w * 0.5f, prev.y0 + 8.0f * s, 0.5f);
        }
        for (int i = 0; listRows == nullptr && extras.keyboard == nullptr && i < static_cast<int>(slots.size()) && i < 10; ++i)
        {
            const auto& slot = slots[static_cast<std::size_t>(i)];
            const Rect r = DemoSlotRect(i, viewportW, viewportH);
            icon(demoSlotsControl_ == i ? pressed : normal, r, 21);
            const float cx = (r.x0 + r.x1) * 0.5f;
            centred(labels, std::to_string(i + 1), cx, r.y0 + 14.0f * s, 0.9f);
            const float room = r.x1 - r.x0 + 12.0f * s;
            centred(slot.recorded ? labels : faint, fit(slot.line1, 0.42f, room), cx, r.y1 + 4.0f * s, 0.42f);
            centred(slot.recorded ? labels : faint, fit(slot.line2, 0.42f, room), cx, r.y1 + 22.0f * s, 0.42f);
            if (slot.recorded)
            {
                const Rect o = DemoRestoreRect(i, viewportW, viewportH);
                icon(demoSlotsControl_ == 10 + i ? pressed : normal, o, 21);
                centred(labels, Tr("Original"), cx, o.y0 + 6.0f * s, 0.42f);
            }
        }
        const Rect cancel = DemoCancelRect(viewportW, viewportH);
        if (extras.keyboard == nullptr)
        {
            icon(demoSlotsControl_ == 20 ? pressed : normal, cancel, 21);
            centred(labels, Tr("Cancel"), (cancel.x0 + cancel.x1) * 0.5f, cancel.y0 + 16.0f * s, 0.6f);
            // activeMode: 1 Export, 2 Import, 3 Name drawn held.
            for (const auto& [control, rect, caption] :
                 {std::tuple<int, Rect, const char*>{21, DemoExportRect(viewportW, viewportH), TrMark("Export")},
                  std::tuple<int, Rect, const char*>{22, DemoImportRect(viewportW, viewportH), TrMark("Import")},
                  std::tuple<int, Rect, const char*>{23, DemoNameRect(viewportW, viewportH), TrMark("Name")}})
            {
                icon(demoSlotsControl_ == control || activeMode == control - 20 ? pressed : normal, rect, 21);
                centred(labels, Tr(caption), (rect.x0 + rect.x1) * 0.5f, rect.y0 + 16.0f * s, 0.6f);
            }
        }
        if (extras.focus)
        {
            const Rect f = *extras.focus;
            icon(pressed, Rect{f.x0 - 4.0f * s, f.y0 - 4.0f * s, f.x1 + 4.0f * s, f.y1 + 4.0f * s}, 21);
        }
        if (!message.empty())
        {
            // Smaller when it is wider than the panel (a long file name, a longer translation).
            const float width = TextMetrics::Width(message) * s;
            const float size = width * 0.45f > 620.0f * s ? 620.0f * s / width : 0.45f;
            centred(labels, message, w * 0.5f, 384.0f * s, size);
        }
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        FlushQuads(device, *padEffect_, padRenderer_, panels, viewportW, viewportH, 0.75f);
        FlushQuads(device, *padEffect_, padRenderer_, rows, viewportW, viewportH, 0.6f);
        FlushQuads(device, *padEffect_, padRenderer_, normal, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressed, viewportW, viewportH, kPausePressedAlpha);
        FlushQuads(device, *textEffect_, textRenderer_, labels, viewportW, viewportH, 1.0f);
        FlushQuads(device, *textEffect_, textRenderer_, faint, viewportW, viewportH, 0.6f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    QuadBatch::Rect InputPad::PanelRect(float x0, float y0, float x1, float y1, int viewportW, int viewportH) noexcept
    {
        const float s = static_cast<float>(viewportH) / kRefH;
        const float cx = static_cast<float>(viewportW) * 0.5f;
        return Rect{cx + (x0 - 320.0f) * s, y0 * s, cx + (x1 - 320.0f) * s, y1 * s};
    }

    float InputPad::PanelTextWidth(const std::string& text, float size, int viewportH) noexcept
    {
        return TextMetrics::Width(text) * size * static_cast<float>(viewportH) / kRefH;
    }

    PanelInput InputPad::UpdatePanel(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                     const std::vector<PanelButton>& buttons, const ScreenKeyboard* keyboard) noexcept
    {
        using Microsoft::Xna::Framework::Input::ButtonState;
        PanelInput result;
        const float x = static_cast<float>(mouse.getXProperty());
        const float y = static_cast<float>(mouse.getYProperty());
        const bool down = mouse.getLeftButtonProperty() == ButtonState::Pressed;
        if (keyboard != nullptr)
        {
            // A line being typed: only the keyboard's keys (100 and up), a click off it is the screen's.
            const int key = keyboard->KeyAt(x, y);
            if (down && !mouseWasDown_) panelControl_ = key >= 0 ? 100 + key : -1;
            if (!down && mouseWasDown_)
            {
                if (panelControl_ >= 100 && panelControl_ == 100 + key) result.keyboardKey = key;
                else if (panelControl_ < 0 && !keyboard->Holds(x, y)) result.outsideClick = true;
                panelControl_ = -1;
            }
            mouseWasDown_ = down;
            return result;
        }
        const auto over = [&]() {
            for (std::size_t i = 0; i < buttons.size(); ++i)
                if (buttons[i].enabled && InRect(x, y, buttons[i].rect)) return static_cast<int>(i);
            return -1;
        };
        if (down && !mouseWasDown_) panelControl_ = over();
        if (!down && mouseWasDown_)
        {
            if (panelControl_ >= 0 && panelControl_ == over()) result.button = panelControl_;
            panelControl_ = -1;
        }
        mouseWasDown_ = down;
        return result;
    }

    void InputPad::DrawPanel(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW, int viewportH,
                             const std::string& title, const std::vector<PanelButton>& buttons,
                             const std::vector<PanelLine>& lines, const ScreenKeyboard* keyboard,
                             std::optional<QuadBatch::Rect> focus)
    {
        if (!loaded_)
        {
            return;
        }
        const float s = static_cast<float>(viewportH) / kRefH;
        const float w = static_cast<float>(viewportW);
        const float padSheetW = LogicalWidth(padTexture_);
        const float padSheetH = LogicalHeight(padTexture_);
        std::vector<Quad> panels, rows, normal, pressed, dead, labels, faint;
        const auto icon = [&](std::vector<Quad>& bucket, const Rect& r, int index) {
            Quad q{r.x0, r.y0, r.x1, r.y1, 0.0f, 0.0f, 0.0f, 0.0f};
            PadIconUv(index, padSheetW, padSheetH, q.u0, q.v0, q.u1, q.v1);
            bucket.push_back(q);
        };
        // A text the width of @p room at most (cut, "..." at its end).
        const auto fit = [&](std::string text, float size, float room) {
            if (TextMetrics::Width(text) * size * s <= room) return text;
            while (!text.empty() && TextMetrics::Width(text + "...") * size * s > room) TextMetrics::PopBack(text);
            return text + "...";
        };
        const auto centred = [&](std::vector<Quad>& bucket, std::string_view text, float cx, float top, float size) {
            AppendGamerLabel(bucket, text, cx - TextMetrics::Width(text) * size * s * 0.5f, top, size, s);
        };
        icon(panels, Rect{w * 0.5f - 320.0f * s, 20.0f * s, w * 0.5f + 320.0f * s, 472.0f * s}, kInitPanelIcon);
        centred(labels, fit(title, 0.7f, 600.0f * s), w * 0.5f, 34.0f * s, 0.7f);
        for (std::size_t i = 0; i < buttons.size(); ++i)
        {
            const auto& b = buttons[i];
            const bool held = b.held || (keyboard == nullptr && panelControl_ == static_cast<int>(i));
            auto& text = b.enabled ? labels : faint;
            if (b.row)
            {
                icon(held ? pressed : rows, b.rect, held ? 21 : kInitPanelIcon);
                AppendGamerLabel(text, fit(b.label, b.size, b.rect.x1 - b.rect.x0 - 20.0f * s), b.rect.x0 + 10.0f * s,
                                 (b.rect.y0 + b.rect.y1) * 0.5f - 15.0f * b.size * s, b.size, s);
                continue;
            }
            icon(held ? pressed : b.enabled ? normal : dead, b.rect, 21);
            // A label wider than its button is drawn smaller first (down to 70%), as the menu's are, and
            // cut only beyond that ("Отправить сообщение", "Envoyer un message").
            const float room = b.rect.x1 - b.rect.x0 - 8.0f * s;
            const float width = TextMetrics::Width(b.label) * s;
            const float size = width * b.size > room ? std::max(b.size * 0.7f, room / width) : b.size;
            centred(text, fit(b.label, size, room), (b.rect.x0 + b.rect.x1) * 0.5f,
                    (b.rect.y0 + b.rect.y1) * 0.5f - 15.0f * size * s, size);
        }
        for (const auto& line : lines)
        {
            auto& bucket = line.faint ? faint : labels;
            if (line.centred) centred(bucket, fit(line.text, line.size, 600.0f * s), line.x, line.y, line.size);
            else AppendGamerLabel(bucket, fit(line.text, line.size, w * 0.5f + 310.0f * s - line.x), line.x, line.y,
                                  line.size, s);
        }
        if (keyboard != nullptr)
        {
            for (std::size_t i = 0; i < keyboard->Keys().size(); ++i)
            {
                const auto& key = keyboard->Keys()[i];
                const bool lit = panelControl_ == 100 + static_cast<int>(i) ||
                                 (key.kind == ScreenKeyboard::Kind::Shift && keyboard->Shifted());
                icon(lit ? pressed : normal, key.rect, 21);
                const std::string face = keyboard->Label(key);
                const float size = face.size() == 1 ? 0.6f : 0.42f;
                centred(labels, face, (key.rect.x0 + key.rect.x1) * 0.5f,
                        (key.rect.y0 + key.rect.y1) * 0.5f - 16.0f * size * s, size);
            }
        }
        if (focus)
        {
            const Rect f = *focus;
            icon(pressed, Rect{f.x0 - 4.0f * s, f.y0 - 4.0f * s, f.x1 + 4.0f * s, f.y1 + 4.0f * s}, 21);
        }
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        FlushQuads(device, *padEffect_, padRenderer_, panels, viewportW, viewportH, 0.75f);
        FlushQuads(device, *padEffect_, padRenderer_, rows, viewportW, viewportH, 0.6f);
        FlushQuads(device, *padEffect_, padRenderer_, normal, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padRenderer_, dead, viewportW, viewportH, 0.45f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressed, viewportW, viewportH, kPausePressedAlpha);
        FlushQuads(device, *textEffect_, textRenderer_, labels, viewportW, viewportH, 1.0f);
        FlushQuads(device, *textEffect_, textRenderer_, faint, viewportW, viewportH, 0.55f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    namespace
    {
        constexpr int kPickerBack = 100;
        constexpr int kPickerOutside = 101;
        constexpr int kPickerColumns = 5;

        struct PickerLayout
        {
            QuadBatch::Rect panel;
            std::array<QuadBatch::Rect, kLanguageCount> cells;
            QuadBatch::Rect back;
            float s;
        };

        // The panel in the 640x480 reference height, centred: the title, three rows of five flags, Back.
        PickerLayout ComputePickerLayout(int viewportW, int viewportH) noexcept
        {
            PickerLayout l{};
            const float s = static_cast<float>(viewportH) / 480.0f;
            const float cx = static_cast<float>(viewportW) * 0.5f;
            l.s = s;
            l.panel = {cx - 312.0f * s, 22.0f * s, cx + 312.0f * s, 468.0f * s};
            for (std::size_t i = 0; i < kLanguageCount; ++i)
            {
                const float col = static_cast<float>(i % kPickerColumns), row = static_cast<float>(i / kPickerColumns);
                const float x0 = cx - 296.0f * s + col * 120.0f * s, y0 = 78.0f * s + row * 113.0f * s;
                l.cells[i] = {x0, y0, x0 + 112.0f * s, y0 + 103.0f * s};
            }
            l.back = {cx - 80.0f * s, 418.0f * s, cx + 80.0f * s, 460.0f * s};
            return l;
        }
    }

    std::vector<QuadBatch::Rect> InputPad::LanguagePickerTargets(int viewportW, int viewportH)
    {
        const auto l = ComputePickerLayout(viewportW, viewportH);
        std::vector<QuadBatch::Rect> targets(l.cells.begin(), l.cells.end());
        targets.push_back(l.back);
        return targets;
    }

    InputPad::LanguagePickerInput InputPad::UpdateLanguagePicker(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                                                 int viewportW, int viewportH) noexcept
    {
        using Microsoft::Xna::Framework::Input::ButtonState;
        LanguagePickerInput result;
        const auto l = ComputePickerLayout(viewportW, viewportH);
        const float x = static_cast<float>(mouse.getXProperty()), y = static_cast<float>(mouse.getYProperty());
        const bool down = mouse.getLeftButtonProperty() == ButtonState::Pressed;
        const auto over = [&]() {
            for (std::size_t i = 0; i < kLanguageCount; ++i)
                if (InRect(x, y, l.cells[i])) return static_cast<int>(i);
            if (InRect(x, y, l.back)) return kPickerBack;
            return InRect(x, y, l.panel) ? -1 : kPickerOutside;
        };
        if (down && !pickerMouseWasDown_) pickerActive_ = over();
        if (!down && pickerMouseWasDown_ && pickerActive_ != -1)
        {
            // Released over what it was pressed on: a flag chooses its language; Back or off the panel closes.
            if (over() == pickerActive_)
            {
                if (pickerActive_ == kPickerBack || pickerActive_ == kPickerOutside) result.backPressed = true;
                else result.chosen = pickerActive_;
            }
            pickerActive_ = -1;
        }
        pickerMouseWasDown_ = down;
        return result;
    }

    void InputPad::DrawLanguagePicker(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW,
                                      int viewportH, std::optional<QuadBatch::Rect> focus)
    {
        if (!loaded_)
        {
            return;
        }
        const auto l = ComputePickerLayout(viewportW, viewportH);
        const float s = l.s;
        const float padSheetW = LogicalWidth(padTexture_);
        const float padSheetH = LogicalHeight(padTexture_);
        std::vector<Quad> panels, normal, pressed, flags, labels;
        const auto icon = [&](std::vector<Quad>& bucket, const Rect& r, int index) {
            Quad q{r.x0, r.y0, r.x1, r.y1, 0.0f, 0.0f, 0.0f, 0.0f};
            PadIconUv(index, padSheetW, padSheetH, q.u0, q.v0, q.u1, q.v1);
            bucket.push_back(q);
        };
        const auto centred = [&](std::string_view text, float cx, float top, float size, float room) {
            const float width = TextMetrics::Width(text) * size * s;
            const float fitted = width > room ? size * room / width : size;
            AppendGamerLabel(labels, text, cx - TextMetrics::Width(text) * fitted * s * 0.5f,
                             top + (size - fitted) * 16.0f * s, fitted, s);
        };
        // Twice: a panel the screen under it shows through only faintly (the Setup's own labels).
        icon(panels, l.panel, kInitPanelIcon);
        icon(panels, l.panel, kInitPanelIcon);
        const float cx = (l.panel.x0 + l.panel.x1) * 0.5f;
        centred(Tr("Language"), cx, 32.0f * s, 0.75f, 560.0f * s);
        const float cells = static_cast<float>(kLanguageCount);
        for (std::size_t i = 0; i < kLanguageCount; ++i)
        {
            const Rect& r = l.cells[i];
            // The language the game speaks shows lit; so does the one being pressed.
            const bool current = kLanguages[i] == CurrentLanguage();
            const bool lit = current || pickerActive_ == static_cast<int>(i);
            // The current language a little larger as well as lit, so it stands out among fifteen.
            const float grow = current ? 4.0f * s : 0.0f;
            icon(lit ? pressed : normal, Rect{r.x0 - grow, r.y0 - grow, r.x1 + grow, r.y1 + grow}, 21);
            const float mid = (r.x0 + r.x1) * 0.5f;
            if (flagsLoaded_)
            {
                const float fw = 69.0f * s, fh = 46.0f * s;
                Quad flag{mid - fw * 0.5f, r.y0 + 12.0f * s, mid + fw * 0.5f, r.y0 + 12.0f * s + fh,
                          static_cast<float>(i) / cells, 0.0f, static_cast<float>(i + 1) / cells, 1.0f};
                flags.push_back(flag);
            }
            centred(LanguageName(kLanguages[i]), mid, r.y0 + 64.0f * s, 0.6f, 102.0f * s);
        }
        icon(pickerActive_ == kPickerBack ? pressed : normal, l.back, 21);
        centred(Tr("Back"), cx, l.back.y0 + 6.0f * s, 0.62f, 150.0f * s);
        if (focus)
        {
            const Rect f = *focus;
            icon(pressed, Rect{f.x0 - 4.0f * s, f.y0 - 4.0f * s, f.x1 + 4.0f * s, f.y1 + 4.0f * s}, 21);
        }
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        FlushQuads(device, *padEffect_, padRenderer_, panels, viewportW, viewportH, 0.95f);
        FlushQuads(device, *padEffect_, padRenderer_, normal, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressed, viewportW, viewportH, kPausePressedAlpha);
        if (flagsLoaded_ && !flags.empty())
            FlushQuads(device, *flagsEffect_, flagsRenderer_, flags, viewportW, viewportH, 1.0f);
        FlushQuads(device, *textEffect_, textRenderer_, labels, viewportW, viewportH, 1.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    InputPad::PauseInput InputPad::UpdatePause(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                                    int viewportW, int viewportH,
                                                    bool showBack, bool showRestart, bool showRecord,
                                                    bool showOriginal) noexcept
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
        const bool overRecord = showRecord && InRect(mouseX, mouseY, PauseRecordRect(viewportW, viewportH));
        const bool overOriginal = showOriginal && InRect(mouseX, mouseY, PauseOriginalRect(viewportW, viewportH));

        if (mouseDown && !mouseWasDown_)
        {
            if (overRecord) activeControl_ = kPauseControlRecord;
            else if (overOriginal) activeControl_ = kPauseControlOriginal;
            else if (overMenu) activeControl_ = kPauseControlMenu;
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
            else if (activeControl_ == kPauseControlRecord) result.recordPressed = overRecord;
            else if (activeControl_ == kPauseControlOriginal) result.originalPressed = overOriginal;
            activeControl_ = -1;
        }

        mouseWasDown_ = mouseDown;
        return result;
    }

    void InputPad::DrawPause(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                              int viewportW, int viewportH, bool showBack, bool showRestart,
                              float phaseTimeSeconds, GalaxyEggbert::Def::GamePhase fadeOutPhase,
                              bool showRecord, bool showButtons, bool showOriginal,
                              const std::string& originalLabel)
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

        const float padSheetW = LogicalWidth(padTexture_);
        const float padSheetH = LogicalHeight(padTexture_);
        std::vector<Quad> normalQuads;
        std::vector<Quad> pressedQuads;
        std::vector<Quad> labelQuads;

        // Real: buttons/labels are hidden entirely while an exit fade is
        // active (`fadeOutPhase != None`), confirmed via research -- NOT
        // during the entrance flourish (`fadeOutPhase == None` covers
        // both a fresh entry and the settled idle state).
        // showButtons: false under the demo positions (BD-0116), whose controls take the screen.
        if (fadeOutPhase == GalaxyEggbert::Def::GamePhase::None && showButtons)
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
                AppendCenteredLabel(labelQuads, Tr(text), centerX, topY, scale);
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
            appendLabel(0, true, TrMark("Home"));
            appendLabel(1, showBack, TrMark("Back"));
            appendLabel(2, true, TrMark("Setup"));
            appendLabel(3, showRestart, TrMark("Restart"));
            appendLabel(4, true, TrMark("Continue"));
            if (showRecord)
            {
                // Galaxy's own (BD-0116), as the menu's Exit button.
                const Rect r = PauseRecordRect(viewportW, viewportH);
                Quad q{r.x0, r.y0, r.x1, r.y1, 0.0f, 0.0f, 0.0f, 0.0f};
                PadIconUv(21, padSheetW, padSheetH, q.u0, q.v0, q.u1, q.v1);
                (activeControl_ == kPauseControlRecord ? pressedQuads : normalQuads).push_back(q);
                const std::string_view record = Tr("Record");
                const float fitted = FaceLabelScale(record, r, scale);
                AppendCenteredLabel(labelQuads, record, (r.x0 + r.x1) * 0.5f, (r.y0 + r.y1) * 0.5f - 12.0f * fitted,
                                    fitted);
            }
            if (showOriginal)
            {
                // The shipped world again (the web's xnjttjpo edits), left of Record.
                const Rect r = PauseOriginalRect(viewportW, viewportH);
                Quad q{r.x0, r.y0, r.x1, r.y1, 0.0f, 0.0f, 0.0f, 0.0f};
                PadIconUv(21, padSheetW, padSheetH, q.u0, q.v0, q.u1, q.v1);
                (activeControl_ == kPauseControlOriginal ? pressedQuads : normalQuads).push_back(q);
                const std::string_view original = Tr(originalLabel);
                const float fitted = FaceLabelScale(original, r, scale);
                AppendCenteredLabel(labelQuads, original, (r.x0 + r.x1) * 0.5f,
                                    (r.y0 + r.y1) * 0.5f - 12.0f * fitted, fitted);
            }
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
        const float charW = LogicalWidth(blupiyoupieTexture_);
        const float charH = LogicalHeight(blupiyoupieTexture_);
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

        const float padSheetW = LogicalWidth(padTexture_);
        const float padSheetH = LogicalHeight(padTexture_);
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
                                                    int viewportW, int viewportH, bool showReset,
                                                    bool accelOn) noexcept
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
        const bool overTerrain = InRect(mouseX, mouseY, SetupTerrainRect(m));
        const bool overEffectsToggle = InRect(mouseX, mouseY, SetupEffectsRect(m));
        const bool overFrameRate = InRect(mouseX, mouseY, SetupFrameRateRect(m));
        const bool overTextures = InRect(mouseX, mouseY, SetupTexturesRect(m));
        const bool overReset = showReset && InRect(mouseX, mouseY, SetupResetRect(m));
        const bool overNickname = showReset && InRect(mouseX, mouseY, SetupNicknameRect(m));
        const bool overLanguage = InRect(mouseX, mouseY, SetupLanguageRect(m));
        const bool overReturn = InRect(mouseX, mouseY, SetupReturnRect(m));
        const bool overEffects = InRect(mouseX, mouseY, SetupSliderRect(0, m));
        const bool overMusic = InRect(mouseX, mouseY, SetupSliderRect(1, m));
        const bool overSensitivity = accelOn && InRect(mouseX, mouseY, SetupSliderRect(2, m));

        if (mouseDown && !mouseWasDown_)
        {
            if (overSensitivity) activeControl_ = kSetupControlAccelSensitivity;
            else if (overEffects) activeControl_ = kSetupControlEffectsVolume;
            else if (overMusic) activeControl_ = kSetupControlMusicVolume;
            else if (overSounds) activeControl_ = kSetupControlSounds;
            else if (overJump) activeControl_ = kSetupControlJump;
            else if (overZoom) activeControl_ = kSetupControlZoom;
            else if (overAccel) activeControl_ = kSetupControlAccel;
            else if (overTerrain) activeControl_ = kSetupControlTerrain;
            else if (overEffectsToggle) activeControl_ = kSetupControlEffects;
            else if (overFrameRate) activeControl_ = kSetupControlFrameRate;
            else if (overTextures) activeControl_ = kSetupControlTextures;
            else if (overReset) activeControl_ = kSetupControlReset;
            else if (overNickname) activeControl_ = kSetupControlNickname;
            else if (overLanguage) activeControl_ = kSetupControlLanguage;
            else if (overReturn) activeControl_ = kSetupControlReturn;
            else activeControl_ = -1;
        }

        // A slider follows the pointer while it is held (pressed on the slider, as Slider::Move
        // takes the touch while it is near); the caller saves the value once it is let go.
        if (mouseDown && (activeControl_ == kSetupControlEffectsVolume || activeControl_ == kSetupControlMusicVolume))
        {
            const int which = activeControl_ == kSetupControlEffectsVolume ? 0 : 1;
            const int volume = static_cast<int>(std::lround(SetupSliderValueAt(which, mouseX, m) * 100.0f));
            (which == 0 ? result.effectsVolume : result.musicVolume) = volume;
        }
        if (mouseDown && activeControl_ == kSetupControlAccelSensitivity)
        {
            result.accelSensitivity = static_cast<int>(std::lround(SetupSliderValueAt(2, mouseX, m) * 100.0f));
        }
        if (!mouseDown && mouseWasDown_ &&
            (activeControl_ == kSetupControlEffectsVolume || activeControl_ == kSetupControlMusicVolume ||
             activeControl_ == kSetupControlAccelSensitivity))
        {
            result.volumeReleased = true;
        }

        if (!mouseDown && mouseWasDown_)
        {
            // Every toggle is wired (OR-0010, 2026-11-02): the caller flips and saves them.
            if (activeControl_ == kSetupControlSounds) result.soundsToggled = true;
            else if (activeControl_ == kSetupControlJump && overJump) result.jumpToggled = true;
            else if (activeControl_ == kSetupControlZoom && overZoom) result.zoomToggled = true;
            else if (activeControl_ == kSetupControlAccel && overAccel) result.accelToggled = true;
            else if (activeControl_ == kSetupControlTerrain && overTerrain) result.terrainToggled = true;
            else if (activeControl_ == kSetupControlEffects && overEffectsToggle) result.effectsToggled = true;
            else if (activeControl_ == kSetupControlFrameRate && overFrameRate) result.frameRateToggled = true;
            else if (activeControl_ == kSetupControlTextures && overTextures) result.texturesToggled = true;
            else if (activeControl_ == kSetupControlReset) result.resetPressed = true;
            else if (activeControl_ == kSetupControlNickname && overNickname) result.nicknamePressed = true;
            else if (activeControl_ == kSetupControlLanguage && overLanguage) result.languagePressed = true;
            else if (activeControl_ == kSetupControlReturn) result.returnPressed = true;
            activeControl_ = -1;
        }

        mouseWasDown_ = mouseDown;
        return result;
    }

    void InputPad::DrawSetup(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                              int viewportW, int viewportH, bool soundsOn, bool jumpRight, bool autoZoom,
                              bool accel, bool showReset, int selectedGamer, int effectsVolume,
                              int musicVolume, int accelSensitivity, float phaseTimeSeconds,
                              GalaxyEggbert::Def::GamePhase fadeOutPhase, bool terrainRelief, int effectsLevel,
                              int frameRate, int textureScale, const std::string& playerName)
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

        const float padSheetW = LogicalWidth(padTexture_);
        const float padSheetH = LogicalHeight(padTexture_);
        std::vector<Quad> normalQuads;
        std::vector<Quad> pressedQuads;
        std::vector<Quad> labelQuads;
        std::vector<Quad> sliderBarQuads;

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
            const auto appendLabel = [&](const Rect& r, std::string_view text)
            {
                const float leftX = r.x1 + kSetupLabelXOffset * scale;
                const float centerY = (r.y0 + r.y1) * 0.5f;
                // Real DrawTextRightButton(): a two-line label starts 20
                // above the button's centre (12 above the one-line
                // position) and its second line is 24 lower.
                const auto newline = text.find('\n');
                if (newline == std::string_view::npos)
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
            appendButton(SetupJumpRect(m), jumpRight ? kIconSetupToggleOn : kIconSetupToggleOff, kSetupControlJump);
            appendButton(SetupZoomRect(m), autoZoom ? kIconSetupToggleOn : kIconSetupToggleOff, kSetupControlZoom);
            appendButton(SetupAccelRect(m), accel ? kIconSetupToggleOn : kIconSetupToggleOff, kSetupControlAccel);
            appendButton(SetupTerrainRect(m), terrainRelief ? kIconSetupToggleOn : kIconSetupToggleOff,
                         kSetupControlTerrain);
            appendButton(SetupEffectsRect(m), effectsLevel > 0 ? kIconSetupToggleOn : kIconSetupToggleOff,
                         kSetupControlEffects);
            appendButton(SetupFrameRateRect(m), frameRate > 60 ? kIconSetupToggleOn : kIconSetupToggleOff,
                         kSetupControlFrameRate);
            appendButton(SetupTexturesRect(m), textureScale == 1 ? kIconSetupToggleOff : kIconSetupToggleOn,
                         kSetupControlTextures);
            if (showReset)
            {
                appendButton(SetupResetRect(m), kIconSetupReset, kSetupControlReset);
                // The blank face (the menu's Fullscreen, Help) with its label on it.
                const Rect nickname = SetupNicknameRect(m);
                appendButton(nickname, 21, kSetupControlNickname);
                AppendCenteredLabel(labelQuads, Tr("Nickname"), (nickname.x0 + nickname.x1) * 0.5f,
                                    (nickname.y0 + nickname.y1) * 0.5f - 12.0f * scale, scale);
            }
            {
                const Rect language = SetupLanguageRect(m);
                appendButton(language, 21, kSetupControlLanguage);
                AppendCenteredLabel(labelQuads, LanguageName(CurrentLanguage()), (language.x0 + language.x1) * 0.5f,
                                    (language.y0 + language.y1) * 0.5f - 12.0f * scale, scale);
            }
            appendButton(SetupReturnRect(m), kIconSetupReturn, kSetupControlReturn);

            appendLabel(SetupSoundsRect(m), Tr("Sound effects"));
            // The two volume sliders right of it: label over the track, the knob at the value.
            {
                const float jaugeSheetW = LogicalWidth(jaugeTexture_);
                const float jaugeSheetH = LogicalHeight(jaugeTexture_);
                const int volumes[3] = {effectsVolume, musicVolume, accelSensitivity};
                const char* const names[3] = {TrMark("Sound effects"), TrMark("Music"), TrMark("Sensitivity")};
                for (int which = 0; which < (accel ? 3 : 2); ++which)
                {
                    const Rect block = SetupSliderRect(which, m);
                    const Rect track = SetupSliderTrackRect(which, m);
                    AppendCenteredLabel(labelQuads, Tr(names[which]), (block.x0 + block.x1) * 0.5f,
                                        block.y0 + 2.0f * scale, scale);
                    Quad bar;
                    bar.x0 = track.x0;
                    bar.y0 = track.y0;
                    bar.x1 = track.x1;
                    bar.y1 = track.y1;
                    bar.u0 = 0.0f;
                    bar.v0 = 0.0f;
                    bar.u1 = kJaugeCellW / jaugeSheetW;
                    bar.v1 = kJaugeCellH / jaugeSheetH;
                    sliderBarQuads.push_back(bar);
                    const int volume = volumes[which] < 0 ? 0 : volumes[which] > 100 ? 100 : volumes[which];
                    const float knobX = track.x0 + (track.x1 - track.x0) * static_cast<float>(volume) / 100.0f;
                    const float knobY = (track.y0 + track.y1) * 0.5f;
                    const float half = 24.0f * scale;
                    appendButton(Rect{knobX - half, knobY - half, knobX + half, knobY + half}, kSetupSliderKnobIcon,
                                 which == 0   ? kSetupControlEffectsVolume
                                 : which == 1 ? kSetupControlMusicVolume
                                              : kSetupControlAccelSensitivity);
                }
            }
            appendLabel(SetupJumpRect(m), Tr("Jump button on the right"));
            appendLabel(SetupZoomRect(m), Tr("Automatic zoom on action"));
            appendLabel(SetupAccelRect(m), Tr("Accelerometer"));
            appendLabel(SetupTerrainRect(m), Tr("3D terrain"));
            {
                // Its level under it: a press steps to the next (off, low, medium, high, ultra).
                static const char* const kLevels[5] = {TrMark("Off"), TrMark("Low"), TrMark("Medium"), TrMark("High"),
                                                       TrMark("Ultra")};
                appendLabel(SetupEffectsRect(m),
                            TrFormat("3D effects\n{0}", {Tr(kLevels[effectsLevel < 0 ? 0 : effectsLevel > 4 ? 4 : effectsLevel])}));
                appendLabel(SetupFrameRateRect(m), TrFormat("Frame rate: {0} fps", {std::to_string(frameRate)}));
                static const char* const kTextures[3] = {TrMark("Auto"), "1x", "2x"};
                appendLabel(SetupTexturesRect(m),
                            TrFormat("Textures: {0}\n(next start)",
                                     {Tr(kTextures[textureScale == 1 || textureScale == 2 ? textureScale : 0])}));
            }
            if (showReset)
            {
                // Real TX_BUTTON_SETUP_RESET, two lines ("Player {0} :\nErase
                // progress"); it was collapsed to one line, which ran off the
                // screen (2026-10-25).
                // The nickname in place of "Player A" when the player has one (2026-11-11).
                const std::string resetLabel =
                    TrFormat("{0} :\nErase progress",
                             {playerName.empty() ? TrFormat("Player {0}", {std::string(1, static_cast<char>('A' + selectedGamer))})
                                                 : playerName});
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
        FlushQuads(device, *jaugeEffect_, jaugeRenderer_, sliderBarQuads, viewportW, viewportH, 1.0f);
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

        const float padSheetW = LogicalWidth(padTexture_);
        const float padSheetH = LogicalHeight(padTexture_);
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
            const auto appendLabel = [&](const Rect& r, std::string_view text)
            {
                const float centerX = (r.x0 + r.x1) * 0.5f;
                const float topY = r.y1 + kPauseLabelYOffset * scale;
                AppendCenteredLabel(labelQuads, text, centerX, topY, scale);
            };

            appendButton(ResumeButtonRect(0, m), kIconPauseMenu, kResumeControlMenu);
            appendButton(ResumeButtonRect(1, m), kIconPauseContinue, kResumeControlContinue);
            appendLabel(ResumeButtonRect(0, m), Tr("Home"));
            appendLabel(ResumeButtonRect(1, m), Tr("Continue"));
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
        if (!kCheatsEnabled || cheatEntryLocked_) return false;
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
        if (!kCheatsEnabled || cheatEntryLocked_) return 0;
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

    const InputPad::TypedCheatWord* InputPad::TypedCheatWords(int& count) noexcept
    {
        if (!kCheatsEnabled) { count = 0; return nullptr; }
        static constexpr TypedCheatWord kWords[] = {
            {"opendoors", TypedCheat::OpenDoors, "Toggle hub doors; give every key"},
            {"cleanall", TypedCheat::CleanAll, "Clear enemies and hazards"},
            {"megablupi", TypedCheat::MegaBlupi, "Toggle invincibility"},
            {"layegg", TypedCheat::LayEgg, "Set lives to nine"},
            {"killegg", TypedCheat::KillEgg, "No effect, as in the original"},
            {"funskate", TypedCheat::FunSkate, "Ride a skateboard"},
            {"givecopter", TypedCheat::GiveCopter, "Fly a helicopter"},
            {"jeepdrive", TypedCheat::JeepDrive, "Drive a jeep"},
            {"alltreasure", TypedCheat::AllTreasure, "Collect all remaining chests"},
            {"endgoal", TypedCheat::EndGoal, "Teleport to the exit (needs chests)"},
            {"showsecret", TypedCheat::ShowSecret, "Toggle visible flight barriers"},
            {"roundshield", TypedCheat::RoundShield, "Grant a temporary shield"},
            {"quicklollypop", TypedCheat::QuickLollipop, "Grant the lollipop power"},
            {"tenbombs", TypedCheat::TenBombs, "Give ten decoys"},
            {"birdlime", TypedCheat::BirdLime, "Give ten glue shots"},
            {"drivetank", TypedCheat::DriveTank, "Drive a tank"},
            {"powercharge", TypedCheat::PowerCharge, "Grant the cloud power"},
            {"hidedrink", TypedCheat::HideDrink, "Grant temporary invisibility"},
            {"iovercraft", TypedCheat::IOvercraft, "Ride an overcraft"},
            {"udynamite", TypedCheat::UDynamite, "Give one dynamite stick"},
            {"weelkeys", TypedCheat::WeelKeys, "Give all three keys"},
            {"quick", TypedCheat::Quick, "Toggle F7/F8 speed shortcuts"},
            {"ghost", TypedCheat::Ghost, "Toggle ghost mode (pass walls)"},
            {"zoom", TypedCheat::Zoom, "Cycle chase-camera distance"},
            {"help", TypedCheat::Help, "List cheats and explanations"},
            {"cheats", TypedCheat::Help, "Same as help (Mobile Eggbert)"},
            {"compass", TypedCheat::Compass, "Toggle distance + turn/height arrows"},
            {"fairplay", TypedCheat::FairPlay, "Toggle cheat lock; clear active toggles"},
            {"debug", TypedCheat::Debug, "Toggle runtime diagnostics panel"},
            {"fps", TypedCheat::Fps, "Show or hide the frames per second"},
            {"position", TypedCheat::Position, "Show or hide Blupi's position (x y z)"},
            {"honest", TypedCheat::Honest, "Lock every cheat until the game quits"},
            {"xnjttjpo", TypedCheat::XMission, "Edit this world in the 3D editor"},
        };
        count = static_cast<int>(sizeof(kWords) / sizeof(kWords[0]));
        return kWords;
    }

    void InputPad::RestoreCheatEntryLock(bool locked, bool lockedForRun) noexcept
    {
        cheatEntryLockedForRun_ = lockedForRun;
        SetCheatEntryLocked(locked);
    }

    void InputPad::LockCheatEntryForRun() noexcept
    {
        cheatEntryLockedForRun_ = true;
        SetCheatEntryLocked(true);
    }

    void InputPad::SetCheatEntryLocked(bool locked) noexcept
    {
        cheatEntryLocked_ = locked || cheatEntryLockedForRun_;
        cheatGestureIndex_ = 0;
        cheatActiveControl_ = -1;
        cheatMouseWasDown_ = false;
    }

    InputPad::TypedCheatResult InputPad::UpdateTypedGhostCheat(
        const Microsoft::Xna::Framework::Input::KeyboardState& keyboard, bool isPlayPhase,
        int orderedLetter) noexcept
    {
        using Microsoft::Xna::Framework::Input::Keys;

        TypedCheatResult result;
        if (!isPlayPhase || !kCheatsEnabled)
        {
            return result;
        }

        static constexpr Keys kLetterKeys[26] = {
            Keys::A, Keys::B, Keys::C, Keys::D, Keys::E, Keys::F, Keys::G, Keys::H,
            Keys::I, Keys::J, Keys::K, Keys::L, Keys::M, Keys::N, Keys::O, Keys::P,
            Keys::Q, Keys::R, Keys::S, Keys::T, Keys::U, Keys::V, Keys::W, Keys::X,
            Keys::Y, Keys::Z};

        typedIdleSeconds_ += 1.0f / 60.0f;
        // Preserve the reference's rolling cheat buffer even when the player types slowly.
        if (typedIdleSeconds_ > 1.2f) typingCheat_ = false;
        bool completed = false;

        for (int li = 0; li < 26; ++li)
        {
            const bool down = keyboard.IsKeyDown(kLetterKeys[li]);
            if (!down) suppressedGameplayLetters_ &= ~(1u << li);
            // Text events preserve short presses and their order between rendered frames.
            // -1 keeps the snapshot path for autoplay and devices without text input;
            // 0 means text input is active but no new letter arrived this tick.
            if (orderedLetter >= 0 ? orderedLetter == 'a' + li : down && !letterKeyWasDown_[li])
            {
                typedCheatBuffer_ += static_cast<char>('a' + li);
                typedIdleSeconds_ = 0;
                if (typedCheatBuffer_.size() > 32)
                {
                    typedCheatBuffer_ = typedCheatBuffer_.substr(typedCheatBuffer_.size() - 32);
                }

                int count = 0;
                const TypedCheatWord* words = TypedCheatWords(count);
                // The longest word the buffer ends with, should one word end in another.
                int matched = -1;
                for (int ci = 0; ci < count; ++ci)
                {
                    if (cheatEntryLocked_ && (cheatEntryLockedForRun_ || words[ci].cheat != TypedCheat::FairPlay)) continue;
                    const std::string_view name = words[ci].word;
                    if (typedCheatBuffer_.size() >= name.size() &&
                        typedCheatBuffer_.compare(typedCheatBuffer_.size() - name.size(), name.size(), name) == 0 &&
                        (matched < 0 || name.size() > std::string_view(words[matched].word).size()))
                        matched = ci;
                }
                if (matched >= 0)
                {
                    result.cheat = words[matched].cheat;
                    completed = true;
                    result.ghostTyped = result.cheat == TypedCheat::Ghost;
                    result.quickTyped = result.cheat == TypedCheat::Quick;
                    // The original clears the buffer after every word; after "quick" that made
                    // "quicklollypop" impossible to type in the port's MODERN build (its first
                    // five letters fire "quick"). Kept here so both words work.
                    if (result.cheat != TypedCheat::Quick)
                    {
                        typedCheatBuffer_.clear();
                    }
                }
                typingCheat_ = false;
                for (int ci = 0; ci < count; ++ci)
                {
                    const std::string_view word(words[ci].word);
                    const auto max = std::min(word.size() - 1, typedCheatBuffer_.size());
                    for (std::size_t length = 2; length <= max; ++length)
                        if (typedCheatBuffer_.compare(typedCheatBuffer_.size() - length, length, word.data(), length) == 0)
                            typingCheat_ = true;
                }
            }
            letterKeyWasDown_[li] = down;
        }
        if (typingCheat_ || completed)
            for (int i = 0; i < 26; ++i)
                if (keyboard.IsKeyDown(kLetterKeys[i])) suppressedGameplayLetters_ |= 1u << i;
        return result;
    }

    Microsoft::Xna::Framework::Input::KeyboardState InputPad::FilterGameplayKeyboard(
        const Microsoft::Xna::Framework::Input::KeyboardState& keyboard) const
    {
        using namespace Microsoft::Xna::Framework::Input;
        if (!kCheatsEnabled || suppressedGameplayLetters_ == 0) return keyboard;
        const auto pressed = keyboard.GetPressedKeys();
        std::unordered_set<Keys> keys(pressed.begin(), pressed.end());
        for (int i = 0; i < 26; ++i)
            if (suppressedGameplayLetters_ & (1u << i)) keys.erase(static_cast<Keys>(int(Keys::A) + i));
        return KeyboardState(keys);
    }

    void InputPad::DrawCheatMenu(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                  int viewportW, int viewportH)
    {
        if (!loaded_ || !kCheatsEnabled)
        {
            return;
        }

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const auto refToScreenX = [&](float x) { return offsetX + x * scale; };
        const auto refToScreenY = [&](float y) { return y * scale; };

        const float padSheetW = LogicalWidth(padTexture_);
        const float padSheetH = LogicalHeight(padTexture_);
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

    void InputPad::DrawGoodbye(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW,
                               int viewportH, float seconds, bool overTitleScene)
    {
        if (!loaded_)
        {
            return;
        }
        const float scale = static_cast<float>(viewportH) / kRefH;
        const float w = static_cast<float>(viewportW);
        const float h = static_cast<float>(viewportH);
        // As Speedy Blupi's goodbye (CEvent::DrawButtons, WM_PHASE_BYE: two lines at its top, two at
        // its bottom), in Galaxy Eggbert's own words.
        static const char* const kTop[2] = {TrMark("Thank you for playing Galaxy Eggbert!"),
                                            TrMark("We hope Blupi's galaxy was as much fun to play as to build.")};
        static const char* const kBottom[2] = {TrMark("Speedy Blupi and Speedy Eggbert: original games by EPSITEC SA"),
                                               "Galaxy Eggbert: galaxyeggbert.openeggbert.com"};
        std::vector<Quad> panelQuads, labelQuads, faintQuads;
        const auto panel = [&](float y0, float y1) {
            Quad q{w * 0.5f - 330.0f * scale, y0, w * 0.5f + 330.0f * scale, y1, 0.0f, 0.0f, 0.0f, 0.0f};
            PadIconUv(kInitPanelIcon, LogicalWidth(padTexture_), LogicalHeight(padTexture_), q.u0, q.v0, q.u1, q.v1);
            for (int layer = 0; layer < 2; ++layer) panelQuads.push_back(q);
        };
        // The top lines in the band of sky under the title's letters (where Loading stands).
        const float top = h * 0.37f;
        panel(top, top + 96.0f * scale); // the third line: how to leave
        panel(h - 78.0f * scale, h - 10.0f * scale);
        // A line no wider than the panel (a translation can be longer than the English).
        const auto centred = [&](std::vector<Quad>& quads, std::string_view text, float lineTop) {
            const float room = 640.0f * scale;
            const float width = TextMetrics::Width(text) * kPauseLabelScale * scale;
            AppendCenteredLabel(quads, text, w * 0.5f, lineTop, width > room ? scale * room / width : scale);
        };
        for (int i = 0; i < 2; ++i)
        {
            centred(labelQuads, Tr(kTop[i]), top + (8.0f + 28.0f * static_cast<float>(i)) * scale);
            centred(labelQuads, Tr(kBottom[i]), h - (70.0f - 28.0f * static_cast<float>(i)) * scale);
        }
        // After a moment, how to leave (the original waited for the same), the text being dark on the panel.
        if (seconds > 1.5f)
            centred(faintQuads, Tr("A key, a click or a button closes the game."), top + 64.0f * scale);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        if (!overTitleScene)
        {
            const float logoW = 640.0f * scale, logoH = 160.0f * scale;
            const std::vector<Quad> logo{Quad{(w - logoW) * 0.5f, h * 0.06f, (w + logoW) * 0.5f, h * 0.06f + logoH,
                                              0.0f, 0.0f, 1.0f, 1.0f}};
            FlushQuads(device, *speedyblupiEffect_, speedyblupiRenderer_, logo, viewportW, viewportH, 1.0f);
        }
        FlushQuads(device, *padEffect_, padRenderer_, panelQuads, viewportW, viewportH, kInitPanelOpacity);
        FlushQuads(device, *textEffect_, textRenderer_, labelQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *textEffect_, textRenderer_, faintQuads, viewportW, viewportH, 0.7f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    void InputPad::DrawGalaxyLoading(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW,
                                      int viewportH, float phaseTimeSeconds, bool overTitleScene)
    {
        if (!loaded_)
        {
            return;
        }
        const float scale = static_cast<float>(viewportH) / kRefH;
        const float w = static_cast<float>(viewportW);
        const float h = static_cast<float>(viewportH);

        // The scene's own tile letters are the title; "Loading" and the gauge sit on one of the
        // menu's translucent panels in the band of sky between them and Blupi.
        std::vector<Quad> labelQuads;
        const int dots = static_cast<int>(phaseTimeSeconds * 3.0f) % 4;
        AppendCenteredLabel(labelQuads, std::string(Tr("Loading")) + std::string(static_cast<std::size_t>(dots), '.'),
                            w * 0.5f, h * 0.43f, scale);

        // The gauge, centred under it, filled as Wait's (the original's waitTable curve).
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
        const float jaugeSheetW = LogicalWidth(jaugeTexture_);
        const float jaugeSheetH = LogicalHeight(jaugeTexture_);
        const float gaugeW = kJaugeCellW * kJaugeZoom * scale;
        const float gaugeH = kJaugeCellH * kJaugeZoom * scale;
        const float x0 = (w - gaugeW) * 0.5f;
        const float y0 = h * 0.51f;
        std::vector<Quad> panelQuads;
        {
            Quad q{x0 - 24.0f * scale, h * 0.40f, x0 + gaugeW + 24.0f * scale, y0 + gaugeH + 14.0f * scale,
                   0.0f, 0.0f, 0.0f, 0.0f};
            PadIconUv(kInitPanelIcon, LogicalWidth(padTexture_),
                      LogicalHeight(padTexture_), q.u0, q.v0, q.u1, q.v1);
            panelQuads.push_back(q);
        }
        std::vector<Quad> jaugeQuads;
        {
            Quad q;
            q.x0 = x0;
            q.y0 = y0;
            q.x1 = x0 + gaugeW;
            q.y1 = y0 + gaugeH;
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
            q.x0 = x0;
            q.y0 = y0;
            q.x1 = x0 + fillW * kJaugeZoom * scale;
            q.y1 = y0 + gaugeH;
            q.u0 = 0.0f;
            q.v0 = (kJaugeCellH * static_cast<float>(kJaugeModeYellow)) / jaugeSheetH;
            q.u1 = fillW / jaugeSheetW;
            q.v1 = (kJaugeCellH * static_cast<float>(kJaugeModeYellow + 1)) / jaugeSheetH;
            jaugeQuads.push_back(q);
        }

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);
        if (!overTitleScene)
        {
            // No scene behind yet: the Galaxy Eggbert logo (640 x 160) on the dark of the cleared screen.
            const float logoW = 640.0f * scale, logoH = 160.0f * scale;
            const std::vector<Quad> logo{Quad{(w - logoW) * 0.5f, h * 0.06f, (w + logoW) * 0.5f, h * 0.06f + logoH,
                                              0.0f, 0.0f, 1.0f, 1.0f}};
            FlushQuads(device, *speedyblupiEffect_, speedyblupiRenderer_, logo, viewportW, viewportH, 1.0f);
        }
        FlushQuads(device, *padEffect_, padRenderer_, panelQuads, viewportW, viewportH, kInitPanelOpacity);
        FlushQuads(device, *textEffect_, textRenderer_, labelQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *jaugeEffect_, jaugeRenderer_, jaugeQuads, viewportW, viewportH, 1.0f);
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

        const float jaugeSheetW = LogicalWidth(jaugeTexture_);
        const float jaugeSheetH = LogicalHeight(jaugeTexture_);
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
        const bool overExit = InRect(mouseX, mouseY, layout.exit);
        const bool overFullscreen = InRect(mouseX, mouseY, layout.fullscreen);
        const bool overHelp = InRect(mouseX, mouseY, layout.help);
        const bool overDemo = InRect(mouseX, mouseY, layout.demo);
        const bool overGalaxy = InRect(mouseX, mouseY, layout.galaxy);
        const bool overDemos = InRect(mouseX, mouseY, layout.demos);
        const bool overMultiplayer = multiplayerShown_ && InRect(mouseX, mouseY, layout.multiplayer);
        const bool overLanguage = InRect(mouseX, mouseY, layout.language);

        if (mouseDown && !mouseWasDown_)
        {
            if (overGamerA) activeControl_ = kInitControlGamerA;
            else if (overGamerB) activeControl_ = kInitControlGamerB;
            else if (overGamerC) activeControl_ = kInitControlGamerC;
            else if (overSetup) activeControl_ = kInitControlSetup;
            else if (overPlay) activeControl_ = kInitControlPlay;
            else if (overEditor) activeControl_ = kInitControlEditor;
            else if (overExit) activeControl_ = kInitControlExit;
            else if (overFullscreen) activeControl_ = kInitControlFullscreen;
            else if (overHelp) activeControl_ = kInitControlHelp;
            else if (overDemo) activeControl_ = kInitControlDemo;
            else if (overGalaxy) activeControl_ = kInitControlGalaxy;
            else if (overDemos) activeControl_ = kInitControlDemos;
            else if (overMultiplayer) activeControl_ = kInitControlMultiplayer;
            else if (overLanguage) activeControl_ = kInitControlLanguage;
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
                case kInitControlExit: result.exitPressed = true; break;
                case kInitControlFullscreen: result.fullscreenPressed = overFullscreen; break;
                case kInitControlHelp: result.helpPressed = overHelp; break;
                case kInitControlDemo: result.demoPressed = overDemo; break;
                case kInitControlGalaxy: result.galaxyPressed = overGalaxy; break;
                case kInitControlDemos: result.demosPressed = overDemos; break;
                case kInitControlMultiplayer: result.multiplayerPressed = overMultiplayer; break;
                case kInitControlLanguage: result.languagePressed = overLanguage; break;
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
                             bool overTitleScene, bool fullscreen, const std::string& versionText,
                             const std::string& buildText)
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
        const float charW = LogicalWidth(blupiyoupieTexture_);
        const float charH = LogicalHeight(blupiyoupieTexture_);

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

        const float padSheetW = LogicalWidth(padTexture_);
        const float padSheetH = LogicalHeight(padTexture_);
        std::vector<Quad> normalQuads;
        std::vector<Quad> pressedQuads;
        std::vector<Quad> labelQuads;
        std::vector<Quad> panelQuads;

        std::vector<Quad> editorIconQuads;
        std::vector<Quad> flagQuads;

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
                // The player's nickname in place of "Player A" (a Galaxy addition, 2026-11-11).
                std::string title = info.nickname.empty() ? TrFormat("Player {0}", {std::string(1, letter)}) : info.nickname;
                AppendGamerLabel(labelQuads, title, textLeft, r.y0 + kGamerTitleYOffset * s, kGamerTitleScale, s);
                AppendGamerLabel(labelQuads, TrFormat("Main gates : {0}/12", {std::to_string(info.mainGates)}), textLeft,
                                r.y0 + kGamerMDoorsYOffset * s, kGamerBodyScale, s);
                AppendGamerLabel(labelQuads, TrFormat("Secondary gates : {0}/52", {std::to_string(info.secondaryGates)}),
                                textLeft, r.y0 + kGamerSDoorsYOffset * s, kGamerBodyScale, s);
                const std::string blupi = TrFormat("Blupi : {0}", {std::to_string(info.lives)});
                AppendGamerLabel(labelQuads, blupi, textLeft, r.y0 + kGamerLivesYOffset * s, kGamerBodyScale, s);
                if (info.galaxyGates >= 0)
                {
                    // Right-aligned in the panel, smaller when it would reach the Blupi count.
                    const std::string gates = TrCount(" : {0} gate", " : {0} gates", info.galaxyGates,
                                                       {std::to_string(info.galaxyGates)});
                    std::string named = info.galaxyName.empty() ? std::string(Tr("Galaxy")) : info.galaxyName;
                    const float right = layout.leftPanel.x1 - 8.0f * s;
                    const float room = right - (textLeft + (TextMetrics::Width(blupi) * kGamerBodyScale + 10.0f) * s);
                    // A long name is cut ("...") where even the smallest letters would reach Blupi's count.
                    bool cut = false;
                    while (named.size() > 1 && TextMetrics::Width(named + (cut ? "..." : "") + gates) * 0.3f * s > room)
                    {
                        TextMetrics::PopBack(named);
                        cut = true;
                    }
                    const std::string galaxy = named + (cut ? "..." : "") + gates;
                    const float scale = std::max(0.3f, std::min(kGamerBodyScale, room / (TextMetrics::Width(galaxy) * s)));
                    AppendGamerLabel(labelQuads, galaxy, right - TextMetrics::Width(galaxy) * scale * s,
                                     r.y0 + (kGamerLivesYOffset + (kGamerBodyScale - scale) * 16.0f) * s, scale, s);
                }
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
            // A blank face's label, smaller when a translation is wider than the face.
            const auto faceLabel = [&](const Rect& r, std::string_view text, float labelScale, float lift) {
                const float room = r.x1 - r.x0 - 10.0f * s;
                const float width = TextMetrics::Width(text) * kPauseLabelScale * labelScale;
                const float fitted = width > room ? labelScale * room / width : labelScale;
                AppendCenteredLabel(labelQuads, text, (r.x0 + r.x1) * 0.5f,
                                    (r.y0 + r.y1) * 0.5f - lift * s * fitted / labelScale, fitted);
            };
            appendIconQuad(activeControl_ == kInitControlHelp ? pressedQuads : normalQuads,layout.help,21);
            faceLabel(layout.help, Tr("Help"), s, 12.0f);
            appendIconQuad(activeControl_ == kInitControlFullscreen ? pressedQuads : normalQuads, layout.fullscreen, 21);
            faceLabel(layout.fullscreen, fullscreen ? Tr("Window") : Tr("Fullscreen"), s, 12.0f);
            // The language's flag on the blank face (its code when the flags are missing).
            appendIconQuad(activeControl_ == kInitControlLanguage ? pressedQuads : normalQuads, layout.language, 21);
            if (flagsLoaded_)
            {
                std::size_t index = 0;
                while (index < std::size(kLanguages) && kLanguages[index] != CurrentLanguage()) ++index;
                const float cells = static_cast<float>(std::size(kLanguages));
                const Rect& r = layout.language;
                const float fw = 34.0f * s, fh = fw * 2.0f / 3.0f;
                Quad flag;
                flag.x0 = (r.x0 + r.x1) * 0.5f - fw * 0.5f;
                flag.y0 = (r.y0 + r.y1) * 0.5f - fh * 0.5f - 2.0f * s;
                flag.x1 = flag.x0 + fw;
                flag.y1 = flag.y0 + fh;
                flag.u0 = static_cast<float>(index) / cells;
                flag.u1 = static_cast<float>(index + 1) / cells;
                flag.v0 = 0.0f;
                flag.v1 = 1.0f;
                flagQuads.push_back(flag);
            }
            else
            {
                std::string code(LanguageCode(CurrentLanguage()));
                for (auto& c : code) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                faceLabel(layout.language, code, s, 12.0f);
            }
            appendIconQuad(activeControl_ == kInitControlExit ? pressedQuads : normalQuads, layout.exit, 21);
            faceLabel(layout.exit, Tr("Exit"), s, 12.0f);
            appendIconQuad(activeControl_ == kInitControlDemo ? pressedQuads : normalQuads, layout.demo, 21);
            faceLabel(layout.demo, Tr("Demo"), s, 12.0f);
            appendIconQuad(activeControl_ == kInitControlGalaxy ? pressedQuads : normalQuads, layout.galaxy, 21);
            faceLabel(layout.galaxy, Tr("Galaxy"), s, 12.0f);
            appendIconQuad(activeControl_ == kInitControlDemos ? pressedQuads : normalQuads, layout.demos, 21);
            faceLabel(layout.demos, Tr("Demos"), s, 12.0f);
            if (multiplayerShown_)
            {
                appendIconQuad(activeControl_ == kInitControlMultiplayer ? pressedQuads : normalQuads,
                               layout.multiplayer, 21);
                faceLabel(layout.multiplayer, Tr("Multiplayer"), 0.68f * s, 8.0f);
            }

            // Real labels: TX_BUTTON_PLAY under Play, TX_BUTTON_SETUP right of
            // Setup (Game1::DrawButtonsText()); the editor's label under its
            // button, on two lines like the original's two-line labels.
            AppendCenteredLabel(labelQuads, Tr("Play"), (layout.play.x0 + layout.play.x1) * 0.5f,
                                layout.play.y1 + kPauseLabelYOffset * s, s);
            // Galaxy's own (the owner, 2026-11-11): the version and under it the build's date and
            // time, left of Play, right-aligned a little left of the right panel.
            const float versionRight = layout.rightPanel.x0 - kInitVersionGap * s;
            const float playMiddle = (layout.play.y0 + layout.play.y1) * 0.5f;
            const float textWidth = std::max(TextMetrics::Width(versionText) * kInitVersionScale,
                                             TextMetrics::Width(buildText) * kInitBuildScale) * s;
            if (textWidth > 0.0f)
            {
                // On the menu's own translucent panel, as the buttons' groups: the 3D title scene
                // behind it moves and would make the small letters hard to read.
                appendIconQuad(panelQuads,
                               Rect{versionRight - textWidth - 8.0f * s, playMiddle - (kInitVersionLineGap + 4.0f) * s,
                                    layout.rightPanel.x0 - 4.0f * s, playMiddle + 22.0f * s},
                               kInitPanelIcon);
            }
            AppendGamerLabel(labelQuads, versionText,
                             versionRight - TextMetrics::Width(versionText) * kInitVersionScale * s,
                             playMiddle - kInitVersionLineGap * s, kInitVersionScale, s);
            AppendGamerLabel(labelQuads, buildText,
                             versionRight - TextMetrics::Width(buildText) * kInitBuildScale * s, playMiddle,
                             kInitBuildScale, s);
            {
                // Smaller when a translation would reach the Multiplayer button (or the panel's edge).
                const std::string_view setup = Tr("Setup");
                const float left = layout.setup.x1 + kSetupLabelXOffset * s;
                const float room = (multiplayerShown_ ? layout.multiplayer.x0 : layout.leftPanel.x1) - 4.0f * s - left;
                const float width = TextMetrics::Width(setup) * kSetupLabelScale * s;
                AppendLeftAlignedLabel(labelQuads, setup, left, (layout.setup.y0 + layout.setup.y1) * 0.5f,
                                       width > room && room > 0.0f ? s * room / width : s);
            }
            const float editorCenterX = (layout.editor.x0 + layout.editor.x1) * 0.5f;
            AppendCenteredLabel(labelQuads, Tr("3D World"), editorCenterX, layout.editor.y1 + kPauseLabelYOffset * s, s);
            AppendCenteredLabel(labelQuads, Tr("Editor"), editorCenterX, layout.editor.y1 + (kPauseLabelYOffset + 22.0f) * s,
                                s);

            const float iconHalf = (layout.editor.x1 - layout.editor.x0) * kInitEditorIconFraction * 0.5f;
            const float iconCenterY = (layout.editor.y0 + layout.editor.y1) * 0.5f;
            const float buttonSheetW = LogicalWidth(buttonTexture_);
            const float buttonSheetH = LogicalHeight(buttonTexture_);
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
        if (flagsLoaded_ && !flagQuads.empty())
            FlushQuads(device, *flagsEffect_, flagsRenderer_, flagQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *textEffect_, textRenderer_, labelQuads, viewportW, viewportH, 1.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }
}
