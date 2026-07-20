#include "GEInputPad.hpp"

#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        constexpr float kRefW = 640.0f;
        constexpr float kRefH = 480.0f;

        // pad.png: 140px cells, 8 columns (sheet 1120x420) -- same
        // convention GEHud.cpp's own kPadCellPx/kPadCols already documents
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

        // Real DrawInfo/Init panel opacity is 0.6 (same value as GEHud.cpp's
        // own kPanelOpacity, same real panel icon -- kInitPanelIcon's own
        // comment). Restored 2026-07-18 now that EasyGL (not Vulkan) is the
        // default graphics backend (CMakeLists.txt); a CNA/Vulkan-only bug
        // (BasicEffect Alpha<1 doesn't render at all under Vulkan, still
        // unfixed) means building with `-DCNA_GRAPHICS_BACKEND=VULKAN` will
        // make these panels vanish again -- see GEHud.cpp's kPanelOpacity
        // for the full empirical citation.
        constexpr float kInitPanelOpacity = 0.6f;

        // Play on-screen control layout (proportionally adapted into the
        // existing 640x480 reference space -- see GEInputPad.hpp's class
        // comment for why a literal drawBounds-relative port isn't
        // possible). D-pad hit-square half-extent 70 happens to equal the
        // real source's own 140px hit-radius/2 coincidentally (both are
        // "70" in their respective unit systems, not a derived value).
        constexpr float kDPadCenterX = 80.0f, kDPadCenterY = 400.0f;
        constexpr float kDPadHitHalf = 70.0f;
        constexpr float kDPadRingSize = 100.0f;
        constexpr float kDPadThumbSize = 50.0f;
        constexpr float kDPadThreshold = 20.0f; // reference-space px per axis for the discrete -1/0/+1 read
        constexpr float kDPadThumbMaxOffset = kDPadHitHalf - kDPadThumbSize * 0.5f;

        constexpr float kJumpX0 = 550.0f, kJumpY0 = 390.0f, kJumpX1 = 620.0f, kJumpY1 = 460.0f;
        constexpr float kActionX0 = 550.0f, kActionY0 = 310.0f, kActionX1 = 620.0f, kActionY1 = 380.0f;
        constexpr float kPlayPauseX0 = 580.0f, kPlayPauseY0 = 10.0f, kPlayPauseX1 = 630.0f, kPlayPauseY1 = 60.0f;

        // Pause-row layout: 5 buttons, 90x90, 20px gap, left-aligned at
        // X=55, spanning Y [310,400] -- proportionally adapted the same
        // way (see GEInputPad.hpp), preserving the real left-to-right
        // order (Menu, Back, Setup, Restart, Continue).
        constexpr float kPauseButtonSize = 90.0f;
        constexpr float kPauseButtonGap = 20.0f;
        constexpr float kPauseRowX0 = 55.0f;
        constexpr float kPauseRowY0 = 310.0f;
        constexpr float kPauseRowY1 = kPauseRowY0 + kPauseButtonSize;

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

        // Real WinLostReturn button (icon 3, shared with PlayPause but a
        // DIFFERENT, bigger, less corner-flush rect -- confirmed via
        // `InputPad.cpp`'s own real formula: Left/Right = drawBoundsWidth
        // - bsf1*2.2/1.2, Top/Bottom = bsf1*0.2/1.2, where
        // bsf1=drawBoundsHeight/5. In this engine's 640x480 reference
        // space, bsf1=96, giving (428.8,19.2)-(524.8,115.2).
        constexpr float kWinLostReturnX0 = 640.0f - 96.0f * 2.2f;
        constexpr float kWinLostReturnY0 = 96.0f * 0.2f;
        constexpr float kWinLostReturnX1 = 640.0f - 96.0f * 1.2f;
        constexpr float kWinLostReturnY1 = 96.0f * 1.2f;

        // text.png: 32px glyph cells, 16 columns, glyph index == ASCII
        // code for the printable range (same convention GEHud.cpp already
        // documents -- read off the asset, not mobile-eggbert's own
        // table_char). Fixed advance approximation (the real font is
        // proportional via table_char_width, deliberately not
        // transcribed).
        constexpr float kGlyphCellPx = 32.0f;
        constexpr int kGlyphCols = 16;
        constexpr float kGlyphAdvance = 17.0f;
        constexpr float kPauseLabelScale = 0.7f; // real DrawTextUnderButton() scale
        constexpr float kPauseLabelYOffset = 2.0f; // real "buttonRect.Bottom + 2"
        constexpr float kSetupLabelScale = 0.7f; // real DrawTextRightButton() scale
        constexpr float kSetupLabelXOffset = 10.0f; // real "buttonRect.Right + 10"
        constexpr float kSetupLabelYNudge = 8.0f; // real "(Top+Bottom)/2 - 8" single-line case

        // PlaySetup screen (plan.md MENU-058..069), verified directly
        // against `InputPad.cpp`'s own real formula: bsf2 =
        // drawBoundsHeight*140/480, which is EXACTLY 140 at
        // drawBoundsHeight=480 (this engine's own reference height) --
        // these rects are used here completely unadapted (see
        // GEInputPad.hpp's UpdateSetup()/DrawSetup() class comment).
        // Left column (leftXForButtonsInLeftColumn=20,
        // rightXForButtonsInLeftColumn=20+70=90): Sounds/Jump/Zoom/Accel
        // stacked bottom-up. Reset sits at the same row as Sounds, further
        // right. Return is a big bottom-right corner button.
        constexpr float kSetupLeftColX0 = 20.0f, kSetupLeftColX1 = 90.0f;
        constexpr float kSetupSoundsY0 = 180.0f, kSetupSoundsY1 = 250.0f;
        constexpr float kSetupJumpY0 = 250.0f, kSetupJumpY1 = 320.0f;
        constexpr float kSetupZoomY0 = 320.0f, kSetupZoomY1 = 390.0f;
        constexpr float kSetupAccelY0 = 390.0f, kSetupAccelY1 = 460.0f;
        constexpr float kSetupResetX0 = 450.0f, kSetupResetX1 = 520.0f;
        constexpr float kSetupResetY0 = 180.0f, kSetupResetY1 = 250.0f;
        constexpr float kSetupReturnX0 = 508.0f, kSetupReturnX1 = 620.0f;
        constexpr float kSetupReturnY0 = 348.0f, kSetupReturnY1 = 460.0f;

        // Resume screen (plan.md MENU-040..045): 2 real buttons, verified
        // against `InputPad.cpp`'s own bsf2=140-at-this-reference-height
        // formula (same "no adaptation needed" situation as Setup above),
        // NOT reused from Pause's own Menu/Continue rects (a different
        // real position, since Resume's row sits lower-center rather than
        // Pause's own bottom row).
        constexpr float kResumeMenuX0 = 180.6f, kResumeMenuX1 = 320.6f;
        constexpr float kResumeContinueX0 = 320.6f, kResumeContinueX1 = 460.6f;
        constexpr float kResumeRowY0 = 308.0f, kResumeRowY1 = 448.0f;

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

        // Rect/InRect (plan.md EDITOR-106): moved to the shared GEQuadBatch
        // so the editor UI can reuse the exact same hit-testing primitive.
        // Every existing kXxxRect/InRect(...) use below is unaffected --
        // GEQuadBatch::Rect is structurally/behaviorally identical.
        using Rect = GEQuadBatch::Rect;
        using GEQuadBatch::InRect;

        constexpr Rect kJumpRect{kJumpX0, kJumpY0, kJumpX1, kJumpY1};
        constexpr Rect kActionRect{kActionX0, kActionY0, kActionX1, kActionY1};
        constexpr Rect kPlayPauseRect{kPlayPauseX0, kPlayPauseY0, kPlayPauseX1, kPlayPauseY1};
        constexpr Rect kDPadHitRect{kDPadCenterX - kDPadHitHalf, kDPadCenterY - kDPadHitHalf,
                                     kDPadCenterX + kDPadHitHalf, kDPadCenterY + kDPadHitHalf};
        constexpr Rect kWinLostReturnRect{kWinLostReturnX0, kWinLostReturnY0, kWinLostReturnX1, kWinLostReturnY1};
        constexpr Rect kSetupSoundsRect{kSetupLeftColX0, kSetupSoundsY0, kSetupLeftColX1, kSetupSoundsY1};
        constexpr Rect kSetupJumpRect{kSetupLeftColX0, kSetupJumpY0, kSetupLeftColX1, kSetupJumpY1};
        constexpr Rect kSetupZoomRect{kSetupLeftColX0, kSetupZoomY0, kSetupLeftColX1, kSetupZoomY1};
        constexpr Rect kSetupAccelRect{kSetupLeftColX0, kSetupAccelY0, kSetupLeftColX1, kSetupAccelY1};
        constexpr Rect kSetupResetRect{kSetupResetX0, kSetupResetY0, kSetupResetX1, kSetupResetY1};
        constexpr Rect kSetupReturnRect{kSetupReturnX0, kSetupReturnY0, kSetupReturnX1, kSetupReturnY1};
        constexpr Rect kResumeMenuRect{kResumeMenuX0, kResumeRowY0, kResumeMenuX1, kResumeRowY1};
        constexpr Rect kResumeContinueRect{kResumeContinueX0, kResumeRowY0, kResumeContinueX1, kResumeRowY1};

        Rect PauseButtonRect(int index)
        {
            const float x0 = kPauseRowX0 + static_cast<float>(index) * (kPauseButtonSize + kPauseButtonGap);
            return Rect{x0, kPauseRowY0, x0 + kPauseButtonSize, kPauseRowY1};
        }

        // Logical control indices, local to this file -- Play and Pause
        // reuse the same activeControl_ member but are never both live in
        // the same frame (see GEInputPad.hpp's ResetTouchState() comment).
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

        // Real 3 gamer-slot buttons (A/B/C) + InitSetup, stacked in a
        // column at X=[20,90], plus InitPlay -- verified against
        // `InputPad.cpp`'s own real `buttonSizeFactor2=
        // drawBoundsHeight*140/480` formula, EXACTLY 140 at this engine's
        // own 480 reference height (same "no adaptation needed"
        // situation as PlaySetup's own row, confirmed via research);
        // InitPlay's own real Left/Right/Top/Bottom (480/620/300/440)
        // likewise need no adaptation at this reference size.
        constexpr float kInitGamerColX0 = 20.0f, kInitGamerColX1 = 90.0f;
        constexpr float kInitGamerAY0 = 166.0f, kInitGamerAY1 = 236.0f;
        constexpr float kInitGamerBY0 = 236.0f, kInitGamerBY1 = 306.0f;
        constexpr float kInitGamerCY0 = 306.0f, kInitGamerCY1 = 376.0f;
        constexpr float kInitSetupY0 = 390.0f, kInitSetupY1 = 460.0f;
        constexpr float kInitPlayX0 = 480.0f, kInitPlayY0 = 300.0f;
        constexpr float kInitPlayX1 = 620.0f, kInitPlayY1 = 440.0f;

        // Not a real mobile-eggbert button (plan.md EDITOR-107): opens the
        // in-game 3D world editor's browser. Placed in the same left
        // column as the gamer slots/Setup, in the otherwise-empty space
        // above GamerA's row -- same 70px row height as Setup below it.
        constexpr float kInitEditorY0 = 90.0f, kInitEditorY1 = 160.0f;

        // Semi-transparent background panels behind the gamer-slot rows /
        // action buttons (plan.md MENU-014/015) -- pure cosmetic decoration,
        // no functional value (real source has these purely for visual
        // legibility of the text/icons drawn on top). Reuses the same
        // pad.png icon-15 panel convention already established by
        // GEHud.cpp's DrawInfo panels (kPanelIcon there). Panel width
        // extends to cover the gamer row's own text labels (title/gates/
        // lives), not just the icon.
        constexpr int kInitPanelIcon = 15; // pad.png, same convention as GEHud's DrawInfo panel
        constexpr float kInitGamerPanelX0 = 12.0f, kInitGamerPanelX1 = 400.0f;
        constexpr float kInitButtonPanelMargin = 8.0f;

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
        // Placeholder icon (plan.md EDITOR-107) -- pad.png icon 14 has no
        // real mobile-eggbert meaning at all (unused by every other real
        // screen in this class); picked only because it's free, not
        // because its actual appearance was confirmed to suit "3D world
        // editor" -- a cosmetic detail refinable once the user can look at
        // a screenshot, same category as other deliberately-deferred
        // visual-judgment items in this project.
        constexpr int kIconInitEditor = 14;

        // Real per-slot text (`Game1::DrawButtonGamerText()`/
        // `MyResource`): "Player {letter}" (scale 0.7) + "Main gates :
        // {n}/12" + "Secondary gates : {n}/52" + "Blupi : {lives}" (scale
        // 0.45), offset from the button's own top-right corner. Door
        // counts are static "0/12"/"0/52" text (see DrawInit()'s own
        // header comment for why -- this engine has no per-gamer
        // door-flags array).
        constexpr float kGamerTitleScale = 0.7f;
        constexpr float kGamerBodyScale = 0.45f;
        constexpr float kGamerTextXOffset = 5.0f;
        constexpr float kGamerTitleYOffset = 3.0f;
        constexpr float kGamerMDoorsYOffset = 25.0f;
        constexpr float kGamerSDoorsYOffset = 39.0f;
        constexpr float kGamerLivesYOffset = 53.0f;

        constexpr Rect kInitGamerARect{kInitGamerColX0, kInitGamerAY0, kInitGamerColX1, kInitGamerAY1};
        constexpr Rect kInitGamerBRect{kInitGamerColX0, kInitGamerBY0, kInitGamerColX1, kInitGamerBY1};
        constexpr Rect kInitGamerCRect{kInitGamerColX0, kInitGamerCY0, kInitGamerColX1, kInitGamerCY1};
        constexpr Rect kInitSetupRect{kInitGamerColX0, kInitSetupY0, kInitGamerColX1, kInitSetupY1};
        constexpr Rect kInitPlayRect{kInitPlayX0, kInitPlayY0, kInitPlayX1, kInitPlayY1};
        constexpr Rect kInitEditorRect{kInitGamerColX0, kInitEditorY0, kInitGamerColX1, kInitEditorY1};

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
        // GalaxyEggbertCnaGame.cpp's own copy drives the actual phase
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

        // AppendQuadUv (plan.md EDITOR-106): moved to the shared GEQuadBatch.
        using GEQuadBatch::AppendQuadUv;

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
        // AppendRotatedQuadUv (plan.md EDITOR-106): moved to the shared GEQuadBatch.
        using GEQuadBatch::AppendRotatedQuadUv;
    }

    void GEInputPad::LoadContent(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        using Microsoft::Xna::Framework::Graphics::BasicEffect;
        using Microsoft::Xna::Framework::Graphics::Texture2D;

        const char* kPaths[12] = {
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
        };
        for (const char* path : kPaths)
        {
            if (!std::filesystem::exists(path))
            {
                std::printf("GEInputPad: %s not found -- on-screen controls disabled.\n", path);
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
        loaded_ = true;
    }

    void GEInputPad::ResetTouchState() noexcept
    {
        activeControl_ = -1;
        mouseWasDown_ = false;
        dpadDragOffsetX_ = 0.0f;
        dpadDragOffsetY_ = 0.0f;
    }

    void GEInputPad::FlushQuads(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                Microsoft::Xna::Framework::Graphics::BasicEffect& effect,
                                std::unique_ptr<Easy3D::BillboardMeshRenderer>& renderer,
                                const std::vector<Quad>& quads, int viewportW, int viewportH,
                                float alpha)
    {
        GEQuadBatch::FlushQuads(device, effect, renderer, quads, viewportW, viewportH, alpha);
    }

    void GEInputPad::AppendCenteredLabel(std::vector<Quad>& quads, const std::string& text,
                                        float centerX, float topY, float viewportScale) const
    {
        if (text.empty())
        {
            return;
        }
        const float textSheetW = static_cast<float>(textTexture_.getWidthProperty());
        const float textSheetH = static_cast<float>(textTexture_.getHeightProperty());
        const float cellPx = kGlyphCellPx * kPauseLabelScale * viewportScale;
        const float advance = kGlyphAdvance * kPauseLabelScale * viewportScale;
        const float totalAdvance = static_cast<float>(text.size()) * advance;
        float penX = centerX - totalAdvance * 0.5f;
        for (const char c : text)
        {
            const int rank = static_cast<int>(static_cast<unsigned char>(c));
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
            penX += advance;
        }
    }

    void GEInputPad::AppendLeftAlignedLabel(std::vector<Quad>& quads, const std::string& text,
                                            float leftX, float centerY, float viewportScale) const
    {
        if (text.empty())
        {
            return;
        }
        const float textSheetW = static_cast<float>(textTexture_.getWidthProperty());
        const float textSheetH = static_cast<float>(textTexture_.getHeightProperty());
        const float cellPx = kGlyphCellPx * kSetupLabelScale * viewportScale;
        const float advance = kGlyphAdvance * kSetupLabelScale * viewportScale;
        const float topY = centerY - kSetupLabelYNudge * viewportScale;
        float penX = leftX;
        for (const char c : text)
        {
            const int rank = static_cast<int>(static_cast<unsigned char>(c));
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
            penX += advance;
        }
    }

    void GEInputPad::AppendGamerLabel(std::vector<Quad>& quads, const std::string& text, float leftX, float topY,
                                      float labelScale, float viewportScale) const
    {
        if (text.empty())
        {
            return;
        }
        const float textSheetW = static_cast<float>(textTexture_.getWidthProperty());
        const float textSheetH = static_cast<float>(textTexture_.getHeightProperty());
        const float cellPx = kGlyphCellPx * labelScale * viewportScale;
        const float advance = kGlyphAdvance * labelScale * viewportScale;
        float penX = leftX;
        for (const char c : text)
        {
            const int rank = static_cast<int>(static_cast<unsigned char>(c));
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
            penX += advance;
        }
    }

    GEInputPad::CharacterAnim GEInputPad::ComputePauseResumeCharacterAnim(
        float phaseTimeSeconds, GalaxyEggbert::GamePhase fadeOutPhase) const
    {
        using GalaxyEggbert::GamePhase;

        CharacterAnim anim{};
        anim.centerX = kCharacterCenterX;
        anim.centerY = kCharacterCenterY;
        anim.opacity = 1.0f;

        const float charW = static_cast<float>(blupiyoupieTexture_.getWidthProperty());
        const float charH = static_cast<float>(blupiyoupieTexture_.getHeightProperty());

        if (fadeOutPhase == GamePhase::None)
        {
            // Real entrance flourish (NOT part of the generic commit
            // fade): grow from a point + decelerating 360 degree spin
            // over a real 0.75s.
            const float t = std::min(phaseTimeSeconds / kPauseEntryDurationSeconds, 1.0f);
            anim.halfW = (charW * 0.5f) * t;
            anim.halfH = (charH * 0.5f) * t;
            anim.rotationDegrees = (1.0f - t) * (1.0f - t) * 360.0f;
        }
        else if (fadeOutPhase == GamePhase::Play)
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
        else if (fadeOutPhase == GamePhase::PlaySetup)
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

    GEInputPad::SetupFadeAnim GEInputPad::ComputeSetupFadeAnim(float phaseTimeSeconds, bool exiting) const
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

    bool GEInputPad::UpdatePlay(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                int viewportW, int viewportH, PlayInput& outInput) noexcept
    {
        outInput = PlayInput{};

        // Hit-testing is pure layout math -- deliberately NOT gated on
        // loaded_ (unlike DrawPlay()/DrawPause() below), so controls still
        // respond even if pad.png somehow failed to load, and so this
        // logic is testable headless without a GraphicsDevice (see
        // tools/VerifyGEInputPad.cpp).
        using Microsoft::Xna::Framework::Input::ButtonState;

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const float mouseRefX = (static_cast<float>(mouse.getXProperty()) - offsetX) / scale;
        const float mouseRefY = static_cast<float>(mouse.getYProperty()) / scale;
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;

        const bool overDPad = InRect(mouseRefX, mouseRefY, kDPadHitRect);
        const bool overJump = InRect(mouseRefX, mouseRefY, kJumpRect);
        const bool overAction = InRect(mouseRefX, mouseRefY, kActionRect);
        const bool overPause = InRect(mouseRefX, mouseRefY, kPlayPauseRect);

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
        // of where the press started (see GEInputPad.hpp's class comment).
        outInput.jumpHeld = mouseDown && overJump;

        // Real discrete D-pad: only active while the drag that STARTED on
        // the pad continues, using the CURRENT drag point each frame (not
        // a fixed initial grab offset).
        if (mouseDown && activeControl_ == kPlayControlDPad)
        {
            const float dx = mouseRefX - kDPadCenterX;
            const float dy = mouseRefY - kDPadCenterY;
            outInput.turnInput = dx > kDPadThreshold ? 1.0f : (dx < -kDPadThreshold ? -1.0f : 0.0f);
            // Screen Y increases downward; dragging the thumb UP (negative
            // dy) should read as "forward" (+1), matching a physical D-pad.
            outInput.moveInput = dy < -kDPadThreshold ? 1.0f : (dy > kDPadThreshold ? -1.0f : 0.0f);
            dpadDragOffsetX_ = std::clamp(dx, -kDPadThumbMaxOffset, kDPadThumbMaxOffset);
            dpadDragOffsetY_ = std::clamp(dy, -kDPadThumbMaxOffset, kDPadThumbMaxOffset);
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

    void GEInputPad::DrawPlay(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
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

        const auto appendIconQuad = [&](std::vector<Quad>& bucket, float x0, float y0, float x1, float y1, int icon)
        {
            Quad q;
            q.x0 = refToScreenX(x0);
            q.y0 = refToScreenY(y0);
            q.x1 = refToScreenX(x1);
            q.y1 = refToScreenY(y1);
            PadIconUv(icon, padSheetW, padSheetH, q.u0, q.v0, q.u1, q.v1);
            bucket.push_back(q);
        };

        // D-pad: ring always centered; thumb offset by the live drag (zero
        // when not actively dragging, snapping back to center).
        appendIconQuad(normalQuads,
                        kDPadCenterX - kDPadRingSize * 0.5f, kDPadCenterY - kDPadRingSize * 0.5f,
                        kDPadCenterX + kDPadRingSize * 0.5f, kDPadCenterY + kDPadRingSize * 0.5f,
                        kIconDPadRing);
        const float thumbCx = kDPadCenterX + dpadDragOffsetX_;
        const float thumbCy = kDPadCenterY + dpadDragOffsetY_;
        appendIconQuad(activeControl_ == kPlayControlDPad ? pressedQuads : normalQuads,
                        thumbCx - kDPadThumbSize * 0.5f, thumbCy - kDPadThumbSize * 0.5f,
                        thumbCx + kDPadThumbSize * 0.5f, thumbCy + kDPadThumbSize * 0.5f,
                        kIconDPadThumb);

        appendIconQuad(activeControl_ == kPlayControlJump ? pressedQuads : normalQuads,
                        kJumpRect.x0, kJumpRect.y0, kJumpRect.x1, kJumpRect.y1, kIconPlayJump);
        appendIconQuad(activeControl_ == kPlayControlAction ? pressedQuads : normalQuads,
                        kActionRect.x0, kActionRect.y0, kActionRect.x1, kActionRect.y1, kIconPlayAction);
        appendIconQuad(activeControl_ == kPlayControlPause ? pressedQuads : normalQuads,
                        kPlayPauseRect.x0, kPlayPauseRect.y0, kPlayPauseRect.x1, kPlayPauseRect.y1, kIconPlayPause);

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
        FlushQuads(device, *padEffect_, padRenderer_, normalQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressedQuads, viewportW, viewportH, kPlayPressedAlpha);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    GEInputPad::PauseInput GEInputPad::UpdatePause(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                                    int viewportW, int viewportH,
                                                    bool showBack, bool showRestart) noexcept
    {
        PauseInput result;

        // Same reasoning as UpdatePlay() above -- not gated on loaded_.
        using Microsoft::Xna::Framework::Input::ButtonState;

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const float mouseRefX = (static_cast<float>(mouse.getXProperty()) - offsetX) / scale;
        const float mouseRefY = static_cast<float>(mouse.getYProperty()) / scale;
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;

        // Unconditional: Menu/Setup/Continue. Conditional: Back/Restart --
        // real visibility rules (mission!=1; mission!=1 && mission%10!=0).
        const bool overMenu = InRect(mouseRefX, mouseRefY, PauseButtonRect(0));
        const bool overBack = showBack && InRect(mouseRefX, mouseRefY, PauseButtonRect(1));
        const bool overSetup = InRect(mouseRefX, mouseRefY, PauseButtonRect(2));
        const bool overRestart = showRestart && InRect(mouseRefX, mouseRefY, PauseButtonRect(3));
        const bool overContinue = InRect(mouseRefX, mouseRefY, PauseButtonRect(4));

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
            // behavior (see GEInputPad.hpp's UpdatePause() comment -- Menu
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

    void GEInputPad::DrawPause(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                              int viewportW, int viewportH, bool showBack, bool showRestart,
                              float phaseTimeSeconds, GalaxyEggbert::GamePhase fadeOutPhase)
    {
        if (!loaded_)
        {
            return;
        }

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const auto refToScreenX = [&](float x) { return offsetX + x * scale; };
        const auto refToScreenY = [&](float y) { return y * scale; };

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
        std::vector<Quad> backgroundQuads{background};

        // Real entrance flourish / exit fades (plan.md MENU-088/089) --
        // see ComputePauseResumeCharacterAnim()'s own comment for the
        // per-`fadeOutPhase` formulas.
        const auto anim = ComputePauseResumeCharacterAnim(phaseTimeSeconds, fadeOutPhase);
        std::vector<Easy3D::BillboardVertex> charVertices;
        std::vector<std::uint32_t> charIndices;
        if (anim.visible)
        {
            AppendRotatedQuadUv(charVertices, charIndices, refToScreenX(anim.centerX), refToScreenY(anim.centerY),
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
        if (fadeOutPhase == GalaxyEggbert::GamePhase::None)
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
                const Rect r = PauseButtonRect(index);
                const float centerX = refToScreenX((r.x0 + r.x1) * 0.5f);
                const float topY = refToScreenY(r.y1 + kPauseLabelYOffset);
                AppendCenteredLabel(labelQuads, text, centerX, topY, scale);
            };

            const auto appendButton = [&](int index, bool visible, int icon, int controlId)
            {
                if (!visible)
                {
                    return;
                }
                const Rect r = PauseButtonRect(index);
                Quad q;
                q.x0 = refToScreenX(r.x0);
                q.y0 = refToScreenY(r.y0);
                q.x1 = refToScreenX(r.x1);
                q.y1 = refToScreenY(r.y1);
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

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
        FlushQuads(device, *pauseBgEffect_, pauseBgRenderer_, backgroundQuads, viewportW, viewportH, 1.0f);
        if (!charIndices.empty())
        {
            blupiyoupieEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            blupiyoupieEffect_->View = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            blupiyoupieEffect_->Projection = Microsoft::Xna::Framework::Matrix::CreateOrthographicOffCenter(
                0.0f, static_cast<float>(viewportW), static_cast<float>(viewportH), 0.0f, 0.0f, 1.0f);
            blupiyoupieEffect_->setAlphaProperty(anim.opacity);
            blupiyoupieRenderer_ = std::make_unique<Easy3D::BillboardMeshRenderer>(device, charVertices, charIndices);
            blupiyoupieRenderer_->Draw(device, *blupiyoupieEffect_);
            blupiyoupieEffect_->setAlphaProperty(1.0f);
        }
        FlushQuads(device, *padEffect_, padRenderer_, normalQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressedQuads, viewportW, viewportH, kPausePressedAlpha);
        FlushQuads(device, *textEffect_, textRenderer_, labelQuads, viewportW, viewportH, 1.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    bool GEInputPad::UpdateWinLost(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                   int viewportW, int viewportH) noexcept
    {
        using Microsoft::Xna::Framework::Input::ButtonState;

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const float mouseRefX = (static_cast<float>(mouse.getXProperty()) - offsetX) / scale;
        const float mouseRefY = static_cast<float>(mouse.getYProperty()) / scale;
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;

        const bool overReturn = InRect(mouseRefX, mouseRefY, kWinLostReturnRect);

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

    void GEInputPad::DrawWinLost(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
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
        std::vector<Quad> backgroundQuads{background};

        // Real per-phase animation (see GEInputPad.hpp's class comment for
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

        std::vector<Easy3D::BillboardVertex> charVertices;
        std::vector<std::uint32_t> charIndices;
        if (halfWRef > 0.0f && halfHRef > 0.0f)
        {
            AppendRotatedQuadUv(charVertices, charIndices,
                                refToScreenX(kCharacterCenterX), refToScreenY(kWinLostCharacterCenterY),
                                halfWRef * scale, halfHRef * scale, rotationDegrees,
                                0.0f, 0.0f, 1.0f, 1.0f);
        }

        const float padSheetW = static_cast<float>(padTexture_.getWidthProperty());
        const float padSheetH = static_cast<float>(padTexture_.getHeightProperty());
        std::vector<Quad> normalQuads;
        std::vector<Quad> pressedQuads;
        {
            Quad q;
            q.x0 = refToScreenX(kWinLostReturnRect.x0);
            q.y0 = refToScreenY(kWinLostReturnRect.y0);
            q.x1 = refToScreenX(kWinLostReturnRect.x1);
            q.y1 = refToScreenY(kWinLostReturnRect.y1);
            PadIconUv(kIconPlayPause, padSheetW, padSheetH, q.u0, q.v0, q.u1, q.v1);
            (activeControl_ == kWinLostControlReturn ? pressedQuads : normalQuads).push_back(q);
        }

        auto& bgEffect = won ? *winBgEffect_ : *lostBgEffect_;
        auto& bgRenderer = won ? winBgRenderer_ : lostBgRenderer_;

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
        FlushQuads(device, bgEffect, bgRenderer, backgroundQuads, viewportW, viewportH, 1.0f);
        if (!charIndices.empty())
        {
            blupiyoupieEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            blupiyoupieEffect_->View = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            blupiyoupieEffect_->Projection = Microsoft::Xna::Framework::Matrix::CreateOrthographicOffCenter(
                0.0f, static_cast<float>(viewportW), static_cast<float>(viewportH), 0.0f, 0.0f, 1.0f);
            blupiyoupieRenderer_ = std::make_unique<Easy3D::BillboardMeshRenderer>(device, charVertices, charIndices);
            blupiyoupieRenderer_->Draw(device, *blupiyoupieEffect_);
        }
        FlushQuads(device, *padEffect_, padRenderer_, normalQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressedQuads, viewportW, viewportH, kPlayPressedAlpha);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    GEInputPad::SetupInput GEInputPad::UpdateSetup(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                                    int viewportW, int viewportH, bool showReset) noexcept
    {
        SetupInput result;

        using Microsoft::Xna::Framework::Input::ButtonState;

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const float mouseRefX = (static_cast<float>(mouse.getXProperty()) - offsetX) / scale;
        const float mouseRefY = static_cast<float>(mouse.getYProperty()) / scale;
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;

        const bool overSounds = InRect(mouseRefX, mouseRefY, kSetupSoundsRect);
        const bool overJump = InRect(mouseRefX, mouseRefY, kSetupJumpRect);
        const bool overZoom = InRect(mouseRefX, mouseRefY, kSetupZoomRect);
        const bool overAccel = InRect(mouseRefX, mouseRefY, kSetupAccelRect);
        const bool overReset = showReset && InRect(mouseRefX, mouseRefY, kSetupResetRect);
        const bool overReturn = InRect(mouseRefX, mouseRefY, kSetupReturnRect);

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
            // GEInputPad.hpp's UpdateSetup() class comment).
            if (activeControl_ == kSetupControlSounds) result.soundsToggled = true;
            else if (activeControl_ == kSetupControlReset) result.resetPressed = true;
            else if (activeControl_ == kSetupControlReturn) result.returnPressed = true;
            activeControl_ = -1;
        }

        mouseWasDown_ = mouseDown;
        return result;
    }

    void GEInputPad::DrawSetup(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                              int viewportW, int viewportH, bool soundsOn, bool showReset, int selectedGamer,
                              float phaseTimeSeconds, GalaxyEggbert::GamePhase fadeOutPhase)
    {
        if (!loaded_)
        {
            return;
        }

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const auto refToScreenX = [&](float x) { return offsetX + x * scale; };
        const auto refToScreenY = [&](float y) { return y * scale; };

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
        std::vector<Quad> backgroundQuads{background};

        // Real speedyblupi.png slide-in + 2 rotating gear.png decorations
        // (plan.md MENU-088/089, MENU-059/060) -- see
        // ComputeSetupFadeAnim()'s own comment for the exact real formula,
        // shared verbatim by entry and exit (num/num2 inverted on exit).
        const bool exiting = fadeOutPhase != GalaxyEggbert::GamePhase::None;
        const auto anim = ComputeSetupFadeAnim(phaseTimeSeconds, exiting);

        Quad speedyQuad;
        speedyQuad.x0 = refToScreenX(anim.speedyLeft);
        speedyQuad.y0 = refToScreenY(0.0f);
        speedyQuad.x1 = refToScreenX(anim.speedyRight);
        speedyQuad.y1 = refToScreenY(160.0f);
        speedyQuad.u0 = 0.0f;
        speedyQuad.v0 = 0.0f;
        speedyQuad.u1 = 1.0f;
        speedyQuad.v1 = 1.0f;
        std::vector<Quad> speedyQuads{speedyQuad};

        std::vector<Easy3D::BillboardVertex> gearVertices;
        std::vector<std::uint32_t> gearIndices;
        AppendRotatedQuadUv(gearVertices, gearIndices, refToScreenX(kGear1CenterX), refToScreenY(kGear1CenterY),
                            kGear1Half * scale, kGear1Half * scale, anim.gearRotation1, 0.0f, 0.0f, 1.0f, 1.0f);
        AppendRotatedQuadUv(gearVertices, gearIndices, refToScreenX(kGear2CenterX), refToScreenY(kGear2CenterY),
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
                q.x0 = refToScreenX(r.x0);
                q.y0 = refToScreenY(r.y0);
                q.x1 = refToScreenX(r.x1);
                q.y1 = refToScreenY(r.y1);
                PadIconUv(icon, padSheetW, padSheetH, q.u0, q.v0, q.u1, q.v1);
                (activeControl_ == controlId ? pressedQuads : normalQuads).push_back(q);
            };
            const auto appendLabel = [&](const Rect& r, const char* text)
            {
                const float rightX = refToScreenX(r.x1);
                const float centerY = refToScreenY((r.y0 + r.y1) * 0.5f);
                AppendLeftAlignedLabel(labelQuads, text, rightX + kSetupLabelXOffset, centerY, scale);
            };

            // Real icon SWAP (not just opacity) for the 3 toggle-style
            // buttons -- Jump/Zoom/Accel have no real state tracked in
            // this engine (no meaningful desktop equivalent, see class
            // comment), so they always render the "off" icon.
            appendButton(kSetupSoundsRect, soundsOn ? kIconSetupToggleOn : kIconSetupToggleOff, kSetupControlSounds);
            appendButton(kSetupJumpRect, kIconSetupToggleOff, kSetupControlJump);
            appendButton(kSetupZoomRect, kIconSetupToggleOff, kSetupControlZoom);
            appendButton(kSetupAccelRect, kIconSetupToggleOff, kSetupControlAccel);
            if (showReset)
            {
                appendButton(kSetupResetRect, kIconSetupReset, kSetupControlReset);
            }
            appendButton(kSetupReturnRect, kIconSetupReturn, kSetupControlReturn);

            appendLabel(kSetupSoundsRect, "Sound effects");
            appendLabel(kSetupJumpRect, "Jump button on the right");
            appendLabel(kSetupZoomRect, "Automatic zoom on action");
            appendLabel(kSetupAccelRect, "Accelerometer");
            if (showReset)
            {
                // Real 2-line label ("Player {0} :\nErase progress")
                // collapsed to one line -- see class comment.
                const std::string resetLabel = "Player " + std::string(1, static_cast<char>('A' + selectedGamer)) +
                                               ": Erase progress";
                appendLabel(kSetupResetRect, resetLabel.c_str());
            }
            // SetupReturn has no real label at all in the source (same as WinLostReturn).
        }

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
        FlushQuads(device, *setupBgEffect_, setupBgRenderer_, backgroundQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *speedyblupiEffect_, speedyblupiRenderer_, speedyQuads, viewportW, viewportH,
                   anim.speedyOpacity);
        gearEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
        gearEffect_->View = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
        gearEffect_->Projection = Microsoft::Xna::Framework::Matrix::CreateOrthographicOffCenter(
            0.0f, static_cast<float>(viewportW), static_cast<float>(viewportH), 0.0f, 0.0f, 1.0f);
        gearEffect_->setAlphaProperty(anim.gearOpacity);
        gearRenderer_ = std::make_unique<Easy3D::BillboardMeshRenderer>(device, gearVertices, gearIndices);
        gearRenderer_->Draw(device, *gearEffect_);
        gearEffect_->setAlphaProperty(1.0f);
        FlushQuads(device, *padEffect_, padRenderer_, normalQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressedQuads, viewportW, viewportH, kPausePressedAlpha);
        FlushQuads(device, *textEffect_, textRenderer_, labelQuads, viewportW, viewportH, 1.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    GEInputPad::ResumeInput GEInputPad::UpdateResume(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                                      int viewportW, int viewportH) noexcept
    {
        ResumeInput result;

        using Microsoft::Xna::Framework::Input::ButtonState;

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const float mouseRefX = (static_cast<float>(mouse.getXProperty()) - offsetX) / scale;
        const float mouseRefY = static_cast<float>(mouse.getYProperty()) / scale;
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;

        const bool overMenu = InRect(mouseRefX, mouseRefY, kResumeMenuRect);
        const bool overContinue = InRect(mouseRefX, mouseRefY, kResumeContinueRect);

        if (mouseDown && !mouseWasDown_)
        {
            if (overMenu) activeControl_ = kResumeControlMenu;
            else if (overContinue) activeControl_ = kResumeControlContinue;
            else activeControl_ = -1;
        }

        if (!mouseDown && mouseWasDown_)
        {
            // Both buttons are now wired to real behavior (see
            // GEInputPad.hpp's UpdateResume() class comment -- Menu now
            // goes to Init, 2026-07-13, now that Init exists).
            if (activeControl_ == kResumeControlContinue) result.continuePressed = true;
            else if (activeControl_ == kResumeControlMenu) result.menuPressed = true;
            activeControl_ = -1;
        }

        mouseWasDown_ = mouseDown;
        return result;
    }

    void GEInputPad::DrawResume(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                               int viewportW, int viewportH, float phaseTimeSeconds,
                               GalaxyEggbert::GamePhase fadeOutPhase)
    {
        if (!loaded_)
        {
            return;
        }

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const auto refToScreenX = [&](float x) { return offsetX + x * scale; };
        const auto refToScreenY = [&](float y) { return y * scale; };

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
        std::vector<Quad> backgroundQuads{background};

        // Real: Resume's entry flourish/exit fade is bit-for-bit Pause's
        // own (confirmed via research) -- Resume->Play (Continue) never
        // actually reaches this as an exit fade at all (the real `-2`
        // mission sentinel bypasses the defer mechanism entirely, see
        // this method's own class comment), so in practice fadeOutPhase
        // here is only ever None or Init.
        const auto anim = ComputePauseResumeCharacterAnim(phaseTimeSeconds, fadeOutPhase);
        std::vector<Easy3D::BillboardVertex> charVertices;
        std::vector<std::uint32_t> charIndices;
        if (anim.visible)
        {
            AppendRotatedQuadUv(charVertices, charIndices, refToScreenX(anim.centerX), refToScreenY(anim.centerY),
                                anim.halfW * scale, anim.halfH * scale, anim.rotationDegrees, 0.0f, 0.0f, 1.0f, 1.0f);
        }

        const float padSheetW = static_cast<float>(padTexture_.getWidthProperty());
        const float padSheetH = static_cast<float>(padTexture_.getHeightProperty());
        std::vector<Quad> normalQuads;
        std::vector<Quad> pressedQuads;
        std::vector<Quad> labelQuads;

        if (fadeOutPhase == GalaxyEggbert::GamePhase::None)
        {
            const auto appendButton = [&](const Rect& r, int icon, int controlId)
            {
                Quad q;
                q.x0 = refToScreenX(r.x0);
                q.y0 = refToScreenY(r.y0);
                q.x1 = refToScreenX(r.x1);
                q.y1 = refToScreenY(r.y1);
                PadIconUv(icon, padSheetW, padSheetH, q.u0, q.v0, q.u1, q.v1);
                (activeControl_ == controlId ? pressedQuads : normalQuads).push_back(q);
            };
            const auto appendLabel = [&](const Rect& r, const char* text)
            {
                const float centerX = refToScreenX((r.x0 + r.x1) * 0.5f);
                const float topY = refToScreenY(r.y1 + kPauseLabelYOffset);
                AppendCenteredLabel(labelQuads, text, centerX, topY, scale);
            };

            appendButton(kResumeMenuRect, kIconPauseMenu, kResumeControlMenu);
            appendButton(kResumeContinueRect, kIconPauseContinue, kResumeControlContinue);
            appendLabel(kResumeMenuRect, "Home");
            appendLabel(kResumeContinueRect, "Continue");
        }

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
        FlushQuads(device, *pauseBgEffect_, pauseBgRenderer_, backgroundQuads, viewportW, viewportH, 1.0f);
        if (!charIndices.empty())
        {
            blupiyoupieEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            blupiyoupieEffect_->View = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            blupiyoupieEffect_->Projection = Microsoft::Xna::Framework::Matrix::CreateOrthographicOffCenter(
                0.0f, static_cast<float>(viewportW), static_cast<float>(viewportH), 0.0f, 0.0f, 1.0f);
            blupiyoupieEffect_->setAlphaProperty(anim.opacity);
            blupiyoupieRenderer_ = std::make_unique<Easy3D::BillboardMeshRenderer>(device, charVertices, charIndices);
            blupiyoupieRenderer_->Draw(device, *blupiyoupieEffect_);
            blupiyoupieEffect_->setAlphaProperty(1.0f);
        }
        FlushQuads(device, *padEffect_, padRenderer_, normalQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressedQuads, viewportW, viewportH, kPausePressedAlpha);
        FlushQuads(device, *textEffect_, textRenderer_, labelQuads, viewportW, viewportH, 1.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    bool GEInputPad::UpdateCheatGesture(const Microsoft::Xna::Framework::Input::MouseState& mouse,
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

    int GEInputPad::UpdateCheatMenu(const Microsoft::Xna::Framework::Input::MouseState& mouse,
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

    GEInputPad::TypedCheatResult GEInputPad::UpdateTypedGhostCheat(
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

    void GEInputPad::DrawCheatMenu(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
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

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
        FlushQuads(device, *padEffect_, padRenderer_, normalQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressedQuads, viewportW, viewportH, kPausePressedAlpha);
        FlushQuads(device, *textEffect_, textRenderer_, labelQuads, viewportW, viewportH, 1.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    void GEInputPad::DrawWait(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW,
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
        std::vector<Quad> backgroundQuads{background};

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

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
        FlushQuads(device, *waitBgEffect_, waitBgRenderer_, backgroundQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *jaugeEffect_, jaugeRenderer_, jaugeQuads, viewportW, viewportH, 1.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }

    GEInputPad::InitInput GEInputPad::UpdateInit(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                                  int viewportW, int viewportH) noexcept
    {
        InitInput result;

        using Microsoft::Xna::Framework::Input::ButtonState;

        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const float mouseRefX = (static_cast<float>(mouse.getXProperty()) - offsetX) / scale;
        const float mouseRefY = static_cast<float>(mouse.getYProperty()) / scale;
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;

        const bool overGamerA = InRect(mouseRefX, mouseRefY, kInitGamerARect);
        const bool overGamerB = InRect(mouseRefX, mouseRefY, kInitGamerBRect);
        const bool overGamerC = InRect(mouseRefX, mouseRefY, kInitGamerCRect);
        const bool overSetup = InRect(mouseRefX, mouseRefY, kInitSetupRect);
        const bool overPlay = InRect(mouseRefX, mouseRefY, kInitPlayRect);
        const bool overEditor = InRect(mouseRefX, mouseRefY, kInitEditorRect);

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

    void GEInputPad::DrawInit(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW,
                             int viewportH, float phaseTimeSeconds, int selectedGamer, int livesA, int livesB,
                             int livesC, GalaxyEggbert::GamePhase fadeOutPhase)
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
        std::vector<Quad> backgroundQuads{background};

        const float t = std::min(phaseTimeSeconds / kInitEntryDurationSeconds, 1.0f);
        const float charW = static_cast<float>(blupiyoupieTexture_.getWidthProperty());
        const float charH = static_cast<float>(blupiyoupieTexture_.getHeightProperty());

        float titleLeft = kInitTitleLeft, titleRight = kInitTitleRight, titleTop, titleBottom;
        float titleOpacity = 1.0f;
        float charHalfW, charHalfH, charOpacity, charRotation = 0.0f;

        using GalaxyEggbert::GamePhase;
        if (fadeOutPhase == GamePhase::None)
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
        else if (fadeOutPhase == GamePhase::Play)
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

        Quad titleQuad;
        titleQuad.x0 = refToScreenX(titleLeft);
        titleQuad.y0 = refToScreenY(titleTop);
        titleQuad.x1 = refToScreenX(titleRight);
        titleQuad.y1 = refToScreenY(titleBottom);
        titleQuad.u0 = 0.0f;
        titleQuad.v0 = 0.0f;
        titleQuad.u1 = 1.0f;
        titleQuad.v1 = 1.0f;
        std::vector<Quad> titleQuads{titleQuad};

        std::vector<Easy3D::BillboardVertex> charVertices;
        std::vector<std::uint32_t> charIndices;
        if (charHalfW > 0.0f && charHalfH > 0.0f)
        {
            AppendRotatedQuadUv(charVertices, charIndices,
                                refToScreenX(kInitCharacterCenterX), refToScreenY(kInitCharacterCenterY),
                                charHalfW * scale, charHalfH * scale, charRotation, 0.0f, 0.0f, 1.0f, 1.0f);
        }

        const float padSheetW = static_cast<float>(padTexture_.getWidthProperty());
        const float padSheetH = static_cast<float>(padTexture_.getHeightProperty());
        std::vector<Quad> normalQuads;
        std::vector<Quad> pressedQuads;
        std::vector<Quad> labelQuads;
        std::vector<Quad> panelQuads;

        // Real: buttons are hidden entirely while an exit fade is active.
        if (fadeOutPhase == GamePhase::None)
        {
            const auto appendIconQuad = [&](std::vector<Quad>& bucket, const Rect& r, int icon)
            {
                Quad q;
                q.x0 = refToScreenX(r.x0);
                q.y0 = refToScreenY(r.y0);
                q.x1 = refToScreenX(r.x1);
                q.y1 = refToScreenY(r.y1);
                PadIconUv(icon, padSheetW, padSheetH, q.u0, q.v0, q.u1, q.v1);
                bucket.push_back(q);
            };

            const auto appendPanel = [&](float x0, float y0, float x1, float y1)
            {
                Quad q;
                q.x0 = refToScreenX(x0);
                q.y0 = refToScreenY(y0);
                q.x1 = refToScreenX(x1);
                q.y1 = refToScreenY(y1);
                PadIconUv(kInitPanelIcon, padSheetW, padSheetH, q.u0, q.v0, q.u1, q.v1);
                panelQuads.push_back(q);
            };
            appendPanel(kInitGamerPanelX0, kInitGamerAY0, kInitGamerPanelX1, kInitGamerAY1);
            appendPanel(kInitGamerPanelX0, kInitGamerBY0, kInitGamerPanelX1, kInitGamerBY1);
            appendPanel(kInitGamerPanelX0, kInitGamerCY0, kInitGamerPanelX1, kInitGamerCY1);
            appendPanel(kInitGamerColX0 - kInitButtonPanelMargin, kInitSetupY0 - kInitButtonPanelMargin,
                        kInitGamerColX1 + kInitButtonPanelMargin, kInitSetupY1 + kInitButtonPanelMargin);
            appendPanel(kInitPlayX0 - kInitButtonPanelMargin, kInitPlayY0 - kInitButtonPanelMargin,
                        kInitPlayX1 + kInitButtonPanelMargin, kInitPlayY1 + kInitButtonPanelMargin);
            appendPanel(kInitGamerColX0 - kInitButtonPanelMargin, kInitEditorY0 - kInitButtonPanelMargin,
                        kInitGamerColX1 + kInitButtonPanelMargin, kInitEditorY1 + kInitButtonPanelMargin);

            const auto appendGamerRow = [&](const Rect& r, int control, int iconOff, int iconSel, bool selected,
                                            char letter, int lives)
            {
                appendIconQuad(activeControl_ == control ? pressedQuads : normalQuads, r,
                              selected ? iconSel : iconOff);

                const float textLeft = refToScreenX(r.x1 + kGamerTextXOffset);
                std::string title = "Player ";
                title += letter;
                AppendGamerLabel(labelQuads, title, textLeft, refToScreenY(r.y0 + kGamerTitleYOffset),
                                kGamerTitleScale, scale);
                AppendGamerLabel(labelQuads, "Main gates : 0/12", textLeft, refToScreenY(r.y0 + kGamerMDoorsYOffset),
                                kGamerBodyScale, scale);
                AppendGamerLabel(labelQuads, "Secondary gates : 0/52", textLeft,
                                refToScreenY(r.y0 + kGamerSDoorsYOffset), kGamerBodyScale, scale);
                AppendGamerLabel(labelQuads, "Blupi : " + std::to_string(lives), textLeft,
                                refToScreenY(r.y0 + kGamerLivesYOffset), kGamerBodyScale, scale);
            };

            appendGamerRow(kInitGamerARect, kInitControlGamerA, kIconInitGamerAOff, kIconInitGamerASel,
                          selectedGamer == 0, 'A', livesA);
            appendGamerRow(kInitGamerBRect, kInitControlGamerB, kIconInitGamerBOff, kIconInitGamerBSel,
                          selectedGamer == 1, 'B', livesB);
            appendGamerRow(kInitGamerCRect, kInitControlGamerC, kIconInitGamerCOff, kIconInitGamerCSel,
                          selectedGamer == 2, 'C', livesC);

            appendIconQuad(activeControl_ == kInitControlSetup ? pressedQuads : normalQuads, kInitSetupRect,
                          kIconInitSetup);
            appendIconQuad(activeControl_ == kInitControlPlay ? pressedQuads : normalQuads, kInitPlayRect,
                          kIconInitPlay);
            appendIconQuad(activeControl_ == kInitControlEditor ? pressedQuads : normalQuads, kInitEditorRect,
                          kIconInitEditor);
        }

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
        FlushQuads(device, *initBgEffect_, initBgRenderer_, backgroundQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *speedyblupiEffect_, speedyblupiRenderer_, titleQuads, viewportW, viewportH,
                   titleOpacity);
        if (!charIndices.empty())
        {
            blupiyoupieEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            blupiyoupieEffect_->View = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            blupiyoupieEffect_->Projection = Microsoft::Xna::Framework::Matrix::CreateOrthographicOffCenter(
                0.0f, static_cast<float>(viewportW), static_cast<float>(viewportH), 0.0f, 0.0f, 1.0f);
            blupiyoupieEffect_->setAlphaProperty(charOpacity);
            blupiyoupieRenderer_ = std::make_unique<Easy3D::BillboardMeshRenderer>(device, charVertices, charIndices);
            blupiyoupieRenderer_->Draw(device, *blupiyoupieEffect_);
            blupiyoupieEffect_->setAlphaProperty(1.0f);
        }
        FlushQuads(device, *padEffect_, padRenderer_, panelQuads, viewportW, viewportH, kInitPanelOpacity);
        FlushQuads(device, *padEffect_, padRenderer_, normalQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressedQuads, viewportW, viewportH, kPausePressedAlpha);
        FlushQuads(device, *textEffect_, textRenderer_, labelQuads, viewportW, viewportH, 1.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }
}
