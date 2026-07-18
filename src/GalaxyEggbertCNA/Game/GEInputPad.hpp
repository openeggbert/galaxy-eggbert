#pragma once

#include "GEQuadBatch.hpp"

#include <Easy3D/BillboardMesh.hpp>
#include <Easy3D/BillboardMeshRenderer.hpp>
#include <GalaxyEggbert/def/GamePhase.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Input/KeyboardState.hpp>
#include <Microsoft/Xna/Framework/Input/MouseState.hpp>

#include <memory>
#include <string>
#include <vector>

namespace GalaxyEggbert::CNA
{
    // Real mobile-eggbert on-screen touch controls for GalaxyEggbertCNA
    // (2026-07-13, plan.md MENU-021..027/028..039), a mouse-driven port of
    // the real `InputPad` class (`InputPad.cpp`/`.hpp`). Verified directly
    // against that source's real button rects, icons, and press semantics
    // (per-button research, cross-checked against `Pixmap.cpp` for the
    // real `pad.png` 140x140/8-col tile convention -- a DIFFERENT sheet
    // convention from `button.png`'s 40x40/6-col one `GEHud` uses for the
    // Perso icon).
    //
    // Real coordinate space mismatch: `InputPad`'s own real rects are
    // expressed directly in actual `drawBounds` pixel space (the real
    // device's own screen size), NOT the fixed 640x480 reference space
    // `Decor::DrawInfo`/`GEHud` use -- confirmed by their own real formulas
    // (`bsf1 = drawBoundsHeight/5`, `bsf2 = drawBoundsHeight*140/480`) and,
    // decisively, by the real Pause row (5 buttons at native 140-unit
    // spacing spanning >700 real units) not fitting inside a 640-wide
    // space at all. This class instead lays out proportionally-adapted
    // rects THAT DO fit the existing 640x480 reference space (same order,
    // same relative row/corner placement, scaled-down sizing) -- an
    // engine-appropriate adaptation of the real layout, not a literal
    // pixel-for-pixel port (which the coordinate mismatch makes
    // impossible anyway).
    //
    // Real press semantics (`InputPad::ButtonDetect`), ported faithfully:
    // PlayJump is LEVEL-triggered (fires every frame the pointer is
    // inside its rect, no release needed -- matches how this engine's own
    // keyboard Jump already works, `Keys::IsKeyDown`); every other button
    // here (PlayAction, PlayPause, and all 5 Pause-row buttons) is
    // EDGE-triggered on release (single-fire the frame the mouse releases
    // after having been pressed down inside that button's rect -- matches
    // the real "glyph goes from non-None back to None" description).
    //
    // Real D-pad (`InputPad`'s own touch tracking): discrete, not analog
    // -- the touch/click point's offset from the pad's center is
    // thresholded per axis into {-1,0,+1}, not a proportional joystick
    // magnitude. Ported the same way here, using this class's own
    // proportionally-scaled hit-radius instead of the real 140px one.
    class GEInputPad
    {
    public:
        // Loads pad.png (button icons, own instance -- same one-instance-
        // per-draw-path convention GEHud's own pad.png load follows) plus
        // Content/backgrounds/pause.png (real full-screen Pause
        // background, confirmed exactly 640x480 -- a direct pixel match
        // for the existing reference space), Content/backgrounds/
        // blupiyoupie.png (real Pause/Win/Lost character art, confirmed
        // 410x380), win.png/lost.png (real full-screen Win/Lost
        // backgrounds, confirmed exactly 640x480 too), and text.png (own
        // instance, for the real Pause button labels -- see DrawPause()).
        void LoadContent(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);

        // Clears in-flight touch/click tracking. Called from
        // GalaxyEggbertCnaGame::SetPhase() alongside its own debounce
        // resets (matching the real Game1::SetPhase()'s own "clears input
        // debounce state on every transition" behavior) so a drag or
        // button-press in progress at the moment of a KEYBOARD-driven
        // phase change (e.g. Escape while mid-drag) can't leak a stale
        // latched control index into the next phase's own Update* call --
        // Play and Pause reuse the same activeControl_ encoding for
        // different logical buttons, so a stale value could otherwise
        // misfire the wrong one.
        void ResetTouchState() noexcept;

        // Additional input derived from the on-screen Play controls, meant
        // to be OR'd/combined with keyboard input by the caller (mouse and
        // keyboard both work simultaneously, same as the real source
        // supporting touch+physical-button input side by side).
        struct PlayInput
        {
            float turnInput = 0.0f;   // -1/0/+1, D-pad X axis
            float moveInput = 0.0f;   // -1/0/+1, D-pad Y axis
            bool jumpHeld = false;    // PlayJump, level-triggered
            bool actionPressed = false; // PlayAction, edge-triggered (single-fire)
            bool pausePressed = false;  // PlayPause, edge-triggered (single-fire)
        };

        // Hit-tests the Play-phase on-screen controls against the current
        // mouse state. Returns whether the mouse press (if any) landed on
        // one of these controls this frame -- the caller should skip its
        // own unrelated mouse handling (e.g. camera drag-look) whenever
        // this is true, matching the real source's own single-owner touch
        // model (a touch point drives exactly one control at a time).
        [[nodiscard]] bool UpdatePlay(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                      int viewportW, int viewportH, PlayInput& outInput) noexcept;
        void DrawPlay(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                      int viewportW, int viewportH);

        // Pause-row buttons. showBack/showRestart mirror the real
        // conditional visibility (`mission!=1`, `mission!=1 &&
        // mission%10!=0`) -- Menu/Setup/Continue are always shown, matching
        // the real source exactly. Continue/Restart/Setup/Menu are all now
        // wired to real, distinct destinations (Menu -> Init, 2026-07-13,
        // now that Init actually exists -- the earlier "no destination
        // screen exists yet" blocker no longer applies); Back's real
        // destination (hub-world navigation) still doesn't exist here, a
        // documented gap, not a silent omission.
        // DrawPause() also draws each visible button's real text label
        // underneath it (confirmed 2026-07-13 against `Game1::
        // DrawButtonsText()`'s real `DrawTextUnderButton()` calls for
        // `Phase::Pause` -- "Home"/"Back"/"Setup"/"Restart"/"Continue",
        // the real English strings; note the real `Menu`/`PauseMenu`
        // button's real EN text is "Home", not "Menu").
        //
        // Real fade-out transitions (plan.md MENU-088/089, 2026-07-13, a
        // dedicated research pass into the real `fadeOutPhase` deferred-
        // transition mechanic): `phaseTimeSeconds`/`fadeOutPhase` together
        // drive TWO distinct real animations sharing this same
        // blupiyoupie.png character at (418,190):
        // - `fadeOutPhase==None` (i.e. NOT currently exiting -- covers
        //   both a fresh entry into Pause/Resume and the settled idle
        //   state once it's finished): a real 0.75s grow-from-a-point +
        //   decelerating 360° spin (`num=min(phaseTimeSeconds/0.75,1)`,
        //   `rotation=(1-num)²*360`) -- confirmed via research this is
        //   NOT part of the generic phase-commit fade at all, it is
        //   Pause/Resume's OWN entrance flourish (same category as Win's
        //   pulse / Lost's grow-in, which this engine already ports).
        // - `fadeOutPhase==Play`: the real exit-to-Play fade -- blupiyoupie
        //   grows from 1x to 11x native size while linearly fading from
        //   opaque to transparent over the real 1.0s commit window (same
        //   "blow up and vanish" idiom already confirmed for Init->Play,
        //   just centered at Pause's own (418,190)).
        // - `fadeOutPhase==PlaySetup` (Pause only -- Resume never reaches
        //   PlaySetup): the real exit-to-PlaySetup fade -- blupiyoupie
        //   slides horizontally off to the right at FIXED native size/
        //   opacity, quadratic ease-in, over 1.0s (no scale/fade/rotation
        //   change at all).
        // - `fadeOutPhase==Init` (or anything else): the real exit-to-Init
        //   fade -- the SAME grow+spin formula as the entry flourish
        //   above, but run in reverse (shrinks to nothing while spinning
        //   UP from 0° to 360°) over the first 0.75s of the 1.0s commit
        //   window -- confirmed via research this leaves a real ~0.25s
        //   "dead" window where the icon has already vanished but the
        //   phase hasn't committed yet; reproduced faithfully, not
        //   "fixed," since it's confirmed real behavior.
        // Real buttons/labels are hidden entirely while `fadeOutPhase !=
        // None` (confirmed: `DrawButtonsBackground()`/`inputPad.Draw()`/
        // `DrawButtonsText()` are all gated on `fadeOutPhase==None` in the
        // real source) -- NOT hidden during the entry flourish itself,
        // only during an active exit.
        struct PauseInput
        {
            bool continuePressed = false;
            bool restartPressed = false;
            bool setupPressed = false;
            bool menuPressed = false;
            // Real `PauseBack` (plan.md MENU-035, wired 2026-07-17 once the
            // hub/mission-progression system existed to give it a real
            // destination) -- the button rect/press-tracking already
            // existed (`kPauseControlBack`), it just wasn't surfaced here.
            bool backPressed = false;
        };
        [[nodiscard]] PauseInput UpdatePause(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                             int viewportW, int viewportH,
                                             bool showBack, bool showRestart) noexcept;
        void DrawPause(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                       int viewportW, int viewportH, bool showBack, bool showRestart,
                       float phaseTimeSeconds,
                       GalaxyEggbert::GamePhase fadeOutPhase = GalaxyEggbert::GamePhase::None);

        // Win/Lost screens (plan.md MENU-046..057). Real backgrounds
        // (win.png/lost.png, confirmed 640x480) plus the real
        // blupiyoupie.png animation -- verified directly against
        // `Game1.cpp`'s real `Draw()` phase branches (2026-07-13):
        // Win pulses forever (`num = sin(phaseTime/0.15s)/2+1`, centered at
        // real position (418,238), no rotation); Lost grows in once from
        // nothing over a real 5s (`num = min(phaseTime/5s, 1)`) with a
        // decaying spin (`rotation = (1-num)^2 * 2160 degrees`, 6 full
        // turns unwinding to 0 as it settles). Both share the real
        // WinLostReturn button (icon 3, same as PlayPause -- real rect
        // confirmed via `InputPad.cpp`'s own `bsf1=drawBoundsHeight/5`
        // formula, NOT reused verbatim from PlayPause's rect, which is a
        // different, smaller/more corner-flush box). The real destination
        // (`Init`, confirmed via `Game1.cpp`'s `WinLostReturn -> SetPhase
        // (Init)`) -- Init now EXISTS
        // (2026-07-13) but this is left as the established return-to-
        // Play-at-spawn simplification (already established for the
        // keyboard path, HUD-023) rather than changed to Init, a
        // deliberate gameplay-flow choice kept separate from the
        // fade-transitions work (plan.md MENU-088/089) -- Win/Lost are not
        // among the 5 real deferring source phases anyway (`Play`/`Lost`/
        // `Win` never defer), so this transition stays instant either way.
        // Real score/mission-time/lives-remaining text overlays described
        // in plan.md MENU-049/050/051/056/057 were searched for directly
        // in `Game1.cpp`'s `Draw()`/`DrawButtonsText()`/
        // `DrawButtonsBackground()` and NOT found anywhere -- like the
        // already-flagged MENU-050/051 "score", these are treated as
        // unconfirmed/likely-not-backed-by-found-source rather than
        // invented, and are deliberately NOT drawn here.
        //
        // Returns true exactly on the frame the WinLostReturn button is
        // released (edge-triggered, same semantics as every other
        // non-Jump button in this class) -- the caller should reset Blupi
        // to spawn and return to Play (see this method's own class-comment
        // paragraph above for why that, not a real Init transition).
        [[nodiscard]] bool UpdateWinLost(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                         int viewportW, int viewportH) noexcept;
        void DrawWinLost(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                         int viewportW, int viewportH, bool won, float phaseTimeSeconds);

        // PlaySetup/MainSetup screen (plan.md MENU-058..069). PlaySetup is
        // reachable via Pause's real Setup button (see UpdatePause()'s own
        // comment); MainSetup is reachable via the real Init menu's own
        // InitSetup button (plan.md MENU-006..020, added 2026-07-13 once
        // Init actually existed here). Real setup.png background
        // (confirmed exact 640x480). Real button rects verified directly
        // against `InputPad.cpp`'s own bsf2=drawBoundsHeight*140/480
        // formula -- AT drawBoundsHeight=480 (this engine's own reference
        // height) bsf2 is EXACTLY 140, meaning these rects need no
        // proportional adaptation at all (unlike the Pause row's
        // bsf1-based layout) -- a rare literal 1:1 port. SetupSounds is
        // functionally wired to the pre-existing GESound::SetEnabled()/
        // IsEnabled() (a real, meaningful desktop equivalent of the real
        // sound on/off toggle); its icon really swaps between 13 (on) and
        // 21 (off), not just an opacity change (confirmed via
        // `Pixmap.cpp`'s own `selected ? 13 : 21` -- a DIFFERENT real
        // "pressed" convention from every other button in this class).
        // SetupJump/SetupZoom/SetupAccel render at their real positions/
        // icons/labels but stay intentionally inert: Jump-button-side and
        // accelerometer-tilt controls have no meaningful desktop
        // equivalent, auto-zoom has no camera-zoom concept in this engine
        // yet -- documented gaps, not silent omissions.
        //
        // SetupReset (`showReset` -- real: shown ONLY on `MainSetup`,
        // confirmed via `Game1::DrawButtonsText()`'s own `if (phase ==
        // MainSetup)` gate around its label draw, hence `showReset` being
        // the CALLER's job, mirroring the pattern already used for
        // Pause's showBack/showRestart) is now fully wired: real
        // `SetupReset -> gameData.Reset(); gameData.Write();` (confirmed
        // 2026-07-13 via `Game1.cpp` -- the SAME full reset as Cheat5, not
        // a per-gamer-only reset, despite the button's own real label
        // implying otherwise) maps directly onto `GESaveData::Reset()`/
        // `Save()`, now that `GESaveData` actually exists. Real label
        // ("Player {0} :\nErase progress", 2 lines) is collapsed to one
        // line here ("Player {0}: Erase progress") -- no multi-line text
        // renderer exists in this class, a documented simplification of
        // the label's formatting only, not its real gamer-letter content
        // (now meaningful via `GESaveData::GetSelectedGamer()`).
        //
        // SetupReturn is real-position/icon/label AND fully functional
        // (confirmed via `Game1.cpp`'s real `SetupReturn` handler: `if
        // (playSetup) SetPhase(Play,-1); else SetPhase(Init);` -- now
        // BOTH branches are reachable here, so the caller picks Play vs.
        // Init based on which of PlaySetup/MainSetup is current).
        //
        // Real fade-out transitions (plan.md MENU-088/089, 2026-07-13):
        // speedyblupi.png slides in from off-screen-right while fading in
        // (`num=1-(1-t)²` eased over 1.0s, `opacity=num²`, `Left=720-640*
        // num, Right=1360-640*num`) whenever entering (`fadeOutPhase==
        // None`); the SAME formula with `num`/`num2` both inverted plays
        // during an active exit (`fadeOutPhase!=None`) -- confirmed via
        // research this is shared verbatim by MainSetup and PlaySetup, NOT
        // gated by which one it is. Two `gear.png` decorations (native
        // 226x226) at real FIXED rects -- (487,148)-(713,374) at native
        // size, and (118,268)-(570,720) at literally 2x native size, a
        // real intentional asymmetry, not a mistake -- perpetually rotate
        // (`num2` keeps growing slowly even once idle/settled,
        // `rotation1=-num2*250°`, `rotation2=+num2*125°` counter-rotating
        // at half rate) with opacity ramping `0.5→0.1` while entering
        // (real: the exit side's opacity ramp is the mirror, `0.1→0.5`,
        // confirmed as genuinely real even though it reads as visually odd
        // -- see this class's own `.cpp` comment). None of this pauses
        // interactivity: real buttons are hidden ONLY during an active
        // EXIT fade (`fadeOutPhase!=None`), not during the entry
        // animation, which plays purely as a decorative overlay on top of
        // an already-interactive screen.
        struct SetupInput
        {
            bool soundsToggled = false;
            bool resetPressed = false;
            bool returnPressed = false;
        };
        [[nodiscard]] SetupInput UpdateSetup(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                             int viewportW, int viewportH, bool showReset) noexcept;
        void DrawSetup(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                       int viewportW, int viewportH, bool soundsOn, bool showReset, int selectedGamer,
                       float phaseTimeSeconds,
                       GalaxyEggbert::GamePhase fadeOutPhase = GalaxyEggbert::GamePhase::None);

        // Resume screen (plan.md MENU-040..045). Real background is
        // pause.png, the SAME image as Pause (confirmed via `Game1.cpp`'s
        // real `SetPhase()` background dispatch: `case Phase::Pause: case
        // Phase::Resume: BackgroundCache("pause");` -- one shared case),
        // reusing `DrawPause()`'s own background+character draw, INCLUDING
        // its real 0.75s entry grow+spin flourish and exit-to-Init
        // shrink+reverse-spin fade (2026-07-13, plan.md MENU-088/089,
        // confirmed via research: "Resume's entry animation is not
        // separately coded; it is bit-for-bit Pause's"). Only 2 real
        // buttons, a DIFFERENT set from Pause's 5: ResumeMenu (icon 11,
        // same as PauseMenu -- real rect independently re-derived from
        // `InputPad.cpp`'s own bsf2=140 formula, NOT reused from
        // PauseMenu's rect, which is a different position) and
        // ResumeContinue (icon 10, same as PauseContinue, its own distinct
        // rect). Both get real text labels ("Home"/"Continue", confirmed
        // via `Game1::DrawButtonsText()`'s real `Phase::Resume` branch --
        // same real strings as Pause's own Menu/Continue labels).
        // ResumeMenu is now wired to real `SetPhase(Init)` (2026-07-13,
        // now that Init exists -- was previously inert for the same
        // reason PauseMenu was).
        //
        // Real ResumeContinue -> `ContinueMission()` -> `SetPhase(Play,
        // -2)` (confirmed via research): the `-2` mission sentinel
        // BYPASSES the generic fade-defer mechanism entirely -- Resume->
        // Play is the ONE transition among this engine's 5 deferring
        // source phases that is genuinely, always instant in the real
        // game, not merely fast. The caller must pass `bypassFade=true`
        // to `GalaxyEggbertCnaGame::SetPhase()` for this specific call
        // site (not this class's concern -- `UpdateResume()` only reports
        // which button fired).
        //
        // Real trigger for entering this phase (`Game1::OnActivated()`, a
        // WP7 app-reactivation OS lifecycle event gated on a real
        // serialized mid-level `Decor::Current*()` snapshot -- a
        // SEPARATE, heavier save mechanism than `GameData`/`GESaveData`)
        // has no desktop equivalent and is far beyond this engine's
        // single-`.vwr`-world scope to replicate faithfully. Adapted
        // trigger (the caller's responsibility, not this method's):
        // offered at startup whenever `GESaveData::GetHasProgress()` is
        // true -- a documented simplification of WHEN Resume appears, not
        // of the screen/buttons themselves.
        //
        // ResumeContinue is the caller's responsibility to wire (restore
        // saved lives, reset Blupi to spawn, return to Play) -- NOT a true
        // real mid-level resume (no serialized position/treasure/key
        // state exists to restore), a documented simplification matching
        // the one already established for PauseRestart/WinLostReturn.
        struct ResumeInput
        {
            bool menuPressed = false;
            bool continuePressed = false;
        };
        [[nodiscard]] ResumeInput UpdateResume(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                               int viewportW, int viewportH) noexcept;
        void DrawResume(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                        int viewportW, int viewportH, float phaseTimeSeconds,
                        GalaxyEggbert::GamePhase fadeOutPhase = GalaxyEggbert::GamePhase::None);

        // Hidden cheat menu (plan.md CHEAT-001..009, 2026-07-13), verified
        // directly against the real gesture-recognition/cheat-overlay code
        // (a dedicated research pass; several draft plan.md descriptions
        // this pass corrected, see GEInteractionSystem.hpp's own cheat
        // method comments for the per-cheat corrections). Two independent
        // pieces:
        //
        // (1) Gesture unlock: real is a 10-tap sequence
        // (`12,22,32,12,11,21,22,21,31,32`, `Game1.hpp`'s own
        // `cheatGesteLength=10` -- an earlier plan.md draft said 6, which
        // was wrong) over 6 real INVISIBLE zones in a 3-column x 2-row
        // grid spanning the top-left ~2/3 width x ~57% height of the
        // screen, active only during real `Phase::Play` (confirmed via
        // `InputPad.cpp`, pushed alongside PlayPause/Action/Jump). Any
        // wrong tap resets progress to 0; no timeout. UpdateCheatGesture()
        // should only be called while phase_==Play (the caller's
        // responsibility, matching the real gate) and returns true the
        // one frame the full sequence completes. Simplification: only
        // presses landing INSIDE one of the 6 zones are tracked at all
        // (advance or reset) -- the real source additionally resets
        // progress on ANY other button press too (D-pad/Jump/etc.); not
        // modeled, a minor forgiving deviation for this genuinely obscure
        // Easter-egg feature, not a functional loss (the exact real
        // sequence still unlocks it either way).
        //
        // (2) The cheat overlay itself: 9 real buttons (icon 0 -- the
        // D-pad ring icon, reused generically -- + a single-letter real
        // text label per button: D/B/S/E/R/T/C/T/G for cheats 1-9,
        // `Decor::GetCheatTinyText()` -- note cheats 6 and 8 share the
        // same real "T" label, Trial vs Treasure, a real ambiguity in the
        // original game, not a transcription error here). Real rects are
        // a row of nine 80x80 boxes in ABSOLUTE pixels at the literal
        // top-left, NOT scaled by drawBoundsHeight at all (`InputPad.cpp`
        // special-cases this range before the normal per-button switch --
        // a genuine real inconsistency vs. every other button in the
        // game) -- 9*80=720 exceeds even this engine's 640-wide
        // reference space, so (same "literal port doesn't fit" situation
        // as the Pause row) this class instead spans the full reference
        // width in 9 equal columns. No background swap in the real
        // source (renders as a transparent overlay atop whatever's
        // already on screen) and no confirmation -- pressing any of the 9
        // immediately closes the overlay and applies the effect (edge/
        // release-triggered here, matching every other non-Jump button in
        // this class, though that specific real detail -- press vs.
        // release -- was not independently re-confirmed for these 9).
        // Returns 1-9 for the cheat just released, 0 for none; the caller
        // owns the actual cheat effects (this class has no access to
        // GEInteractionSystem/GEBlupiController) and the "is the overlay
        // currently shown" bool (set true when UpdateCheatGesture()
        // returns true, false after DrawCheatMenu() stops being called).
        [[nodiscard]] bool UpdateCheatGesture(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                              int viewportW, int viewportH) noexcept;
        [[nodiscard]] int UpdateCheatMenu(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                          int viewportW, int viewportH) noexcept;
        void DrawCheatMenu(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                           int viewportW, int viewportH);

        // Real SECOND, independent cheat-entry method (confirmed via a
        // direct read of `InputPad.cpp:686-753`, plan.md BLUPI-111) --
        // distinct from the on-screen button-glyph cheat menu above
        // (UpdateCheatGesture/UpdateCheatMenu), which only reaches a
        // handful of cheats. Real mechanism: a rolling lowercase-letter
        // buffer (A-Z keys, capped at 32 chars, oldest dropped), built
        // ONLY during real `Phase::Play`, appending on each key's
        // down-edge (not held-repeat). Each real cheat name is matched
        // against the buffer's own SUFFIX (not a full-buffer-reset
        // match), so typing "...ghost" anywhere fires it without needing
        // to clear the buffer first. Only "ghost" is wired here for now
        // (plan.md BLUPI-111) -- the real table has ~26 entries total
        // (most of the already-implemented CHEAT-001..009 plus several
        // requiring features this engine doesn't have yet, e.g. Debug
        // overlay/Zoom levels/game-speed Quick toggle) -- extending this
        // to more names is a real, faithful, but separate task each time
        // (needs its own verification pass per name, same as every other
        // secret power/cheat this session), not a blind mass-port.
        // Returns true for exactly the one Update() call where "ghost"
        // was just completed (one-shot signal, same idiom as this
        // class's edge-triggered button presses) -- a no-op (always
        // false) outside Play.
        [[nodiscard]] bool UpdateTypedGhostCheat(
            const Microsoft::Xna::Framework::Input::KeyboardState& keyboard, bool isPlayPhase) noexcept;

        // Wait phase (plan.md MENU-001..005), verified directly against
        // `Game1.cpp`'s real `Phase::First -> Wait` transition (a
        // dedicated research pass): real wait.png background (confirmed
        // 640x480) + a real jauge.png progress gauge at real position
        // (196,426), zoom 2.0, mode Yellow -- the fill level (0-100) comes
        // from the real NON-LINEAR `waitTable` lookup curve (`Game1.hpp`),
        // not a linear ramp, ported verbatim (see this class's .cpp
        // constants). Real minimum duration is a FIXED 5.0s wall-clock
        // cosmetic timer, completely decoupled from actual asset loading
        // (confirmed via research) -- this engine already loads
        // everything synchronously in `LoadContent()`, matching the real
        // source's own synchronous `First`->`Wait` transition, so the
        // gauge fill here is purely cosmetic too. The caller owns the
        // actual phase-transition timing (checking phaseTimeSeconds >=
        // 5.0f and moving to Init/Resume) -- this just draws.
        void DrawWait(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                     int viewportW, int viewportH, float phaseTimeSeconds);

        // Init phase / gamer-select menu (plan.md MENU-006..020),
        // verified directly against `Game1.cpp`'s real Init rendering/
        // input dispatch (a dedicated research pass). Real init.png
        // background (confirmed 640x480); real speedyblupi.png title logo
        // (640x160) sliding DOWN from above the screen over a real 1.0s
        // entry (`num=1-(1-t)^2`, Left/Right FIXED at 80/720 -- confirmed
        // via research this is a VERTICAL slide, not the horizontal
        // "slides in from the right" an earlier doc-comment in the real
        // source itself incorrectly claims); real blupiyoupie.png
        // (shared texture with Pause/Win/Lost) scaling in 50%->100% while
        // fading in 0.25->1.0 opacity over the same 1.0s, centered at
        // real (468,280) -- a DIFFERENT position from Pause's (418,190)/
        // WinLost's (418,238). Real exit-fade animations are now ALSO
        // ported (2026-07-13, plan.md MENU-088/089, a dedicated research
        // pass into the real `fadeOutPhase` mechanic): `fadeOutPhase==
        // Play` reverses the title slide at 2x speed (`num=1-2t`, so it's
        // already off-screen again by 0.5s and keeps going negative) while
        // blupiyoupie grows from 1x to 11x native size while linearly
        // fading out (same "blow up and vanish" idiom as Pause->Play,
        // just at Init's own (468,280)); `fadeOutPhase==MainSetup` slides
        // the title back off to the right while fading (`num=(1-t)²`,
        // `opacity=num²`) while blupiyoupie stays FIXED size/position and
        // just fades out (`opacity=(1-t)²`, confirmed via research NO
        // zoom happens here despite an earlier doc-comment in the real
        // source itself claiming one). Real buttons are hidden entirely
        // while an exit fade is active (`fadeOutPhase!=None`), same rule
        // as every other screen in this class.
        //
        // Real 3 independent gamer slots (A/B/C), stacked in a column,
        // plus InitSetup/InitPlay -- all 5 rects need NO proportional
        // adaptation (verified: real `buttonSizeFactor2`/InitPlay formula
        // evaluate to the exact same values at this engine's own 480
        // reference height, same "rare literal port" situation as
        // PlaySetup's own row). A single tap on a gamer slot SELECTS it
        // (returned as gamerSelected, 0-2) but does NOT enter Play by
        // itself -- confirmed via research this matches the real source's
        // own select-then-separate-confirm behavior (`Game1::SetGamer()`
        // vs. the separate real InitPlay button). InitRanking/InitBuy are
        // NOT modeled: their real visibility gate
        // (`getIsTrialModeProperty()`/`getIsRankingModeProperty()`)
        // resolves to "never shown by default" in this port (hardcoded
        // false / QA-cheat-only, confirmed via research) -- same
        // "unreachable in this port" precedent already established for
        // the Trial phase itself.
        //
        // Real per-slot text (`Game1::DrawButtonGamerText()`/
        // `MyResource`): "Player {letter}" + "Main gates : {n}/12" +
        // "Secondary gates : {n}/52" + "Blupi : {lives}". This engine has
        // no per-gamer door-flags array (a single hand-authored .vwr
        // world, not the real 100+-level/200-door-flag structure) -- per
        // explicit user direction, the door-count lines are rendered with
        // the real STRING verbatim (static "0/12"/"0/52") rather than
        // omitted, even though those two numbers are not real tracked
        // data (only the title/lives lines reflect real per-slot state).
        struct InitInput
        {
            int gamerSelected = -1; // 0/1/2 if a gamer slot was just tapped (released) this frame, else -1
            bool playPressed = false;
            bool setupPressed = false;
            // Not a real mobile-eggbert button (plan.md EDITOR-107): opens
            // the in-game 3D world editor's browser for the CURRENTLY
            // selected gamer slot -- content-creation tooling, exempt from
            // the faithful-remake rule (see plan.md section 6).
            bool editorPressed = false;
        };
        [[nodiscard]] InitInput UpdateInit(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                           int viewportW, int viewportH) noexcept;
        void DrawInit(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                     int viewportW, int viewportH, float phaseTimeSeconds,
                     int selectedGamer, int livesA, int livesB, int livesC,
                     GalaxyEggbert::GamePhase fadeOutPhase = GalaxyEggbert::GamePhase::None);

    private:
        // Alias, not a redeclaration (plan.md EDITOR-106): the real
        // definition moved to the shared GEQuadBatch so the editor UI can
        // reuse it too. Every existing FlushQuads()/AppendXxxLabel() call
        // site below is unaffected -- Quad is structurally/behaviorally
        // identical, just no longer privately owned by this class.
        using Quad = GEQuadBatch::Quad;

        void FlushQuads(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                        Microsoft::Xna::Framework::Graphics::BasicEffect& effect,
                        std::unique_ptr<Easy3D::BillboardMeshRenderer>& renderer,
                        const std::vector<Quad>& quads, int viewportW, int viewportH,
                        float alpha);

        // Appends one horizontally-centered line of text.png glyph quads
        // (real `Text::DrawTextCenter` semantics: centered at centerX,
        // top edge at topY, unscaled Y -- see DrawPause()'s real button-
        // label use). A member (not a free function) since text.png's
        // real pixel dimensions are only known once loaded.
        void AppendCenteredLabel(std::vector<Quad>& quads, const std::string& text,
                                 float centerX, float topY, float scale) const;

        // Real `Text::DrawTextRightButton()` semantics: LEFT-aligned
        // starting at leftX, vertically centered around centerY (see
        // DrawSetup()'s real button-label use -- a different alignment
        // from AppendCenteredLabel()'s Pause-row use above).
        void AppendLeftAlignedLabel(std::vector<Quad>& quads, const std::string& text,
                                    float leftX, float centerY, float scale) const;

        // Real `Game1::DrawButtonGamerText()` semantics (Init's per-slot
        // 4-line text block): LEFT-aligned starting at leftX, EXPLICIT
        // topY (not vertically centered/nudged like AppendLeftAlignedLabel
        // above, since each line has its own real fixed Top+N offset) and
        // an explicit label scale (0.7 title / 0.45 body -- this is the
        // first real use of the 0.45x scale in this engine, plan.md
        // MENU-086).
        void AppendGamerLabel(std::vector<Quad>& quads, const std::string& text,
                              float leftX, float topY, float labelScale, float viewportScale) const;

        // Real Pause/Resume character animation (plan.md MENU-088/089):
        // shared by DrawPause()/DrawResume() since both use the identical
        // formula (confirmed via research). Returned rect/rotation/opacity
        // are in REFERENCE space (caller applies refToScreenX/Y + scale),
        // matching the convention DrawWinLost() already uses.
        struct CharacterAnim
        {
            float centerX, centerY, halfW, halfH, rotationDegrees, opacity;
            bool visible;
        };
        [[nodiscard]] CharacterAnim ComputePauseResumeCharacterAnim(
            float phaseTimeSeconds, GalaxyEggbert::GamePhase fadeOutPhase) const;

        // Real MainSetup/PlaySetup speedyblupi+gear decoration (plan.md
        // MENU-088/089): shared by entry (fadeOutPhase==None) and exit
        // (fadeOutPhase!=None, formula run with num/num2 both inverted)
        // per the real source (confirmed identical for both PlaySetup and
        // MainSetup, and for either real exit destination).
        struct SetupFadeAnim
        {
            float speedyLeft, speedyRight, speedyOpacity;
            float gearOpacity, gearRotation1, gearRotation2;
        };
        [[nodiscard]] SetupFadeAnim ComputeSetupFadeAnim(float phaseTimeSeconds, bool exiting) const;

        Microsoft::Xna::Framework::Graphics::Texture2D padTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D pauseBgTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D blupiyoupieTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D winBgTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D lostBgTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D setupBgTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D textTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D waitBgTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D jaugeTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D initBgTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D speedyblupiTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D gearTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> padEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> pauseBgEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> blupiyoupieEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> winBgEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> lostBgEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> setupBgEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> textEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> waitBgEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> jaugeEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> initBgEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> speedyblupiEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> gearEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> padRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> padPressedRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> pauseBgRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> blupiyoupieRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> winBgRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> lostBgRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> setupBgRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> textRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> waitBgRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> jaugeRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> initBgRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> speedyblupiRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> gearRenderer_;
        bool loaded_ = false;

        // Edge-trigger press tracking: which logical button (if any) the
        // current mouse-down started inside, per the real "must have
        // pressed down inside, fires on release" model. Encoded as a
        // small int rather than a real ButtonGlyph-style enum since only
        // one of UpdatePlay/UpdatePause/UpdateSetup/UpdateWinLost ever
        // runs in a given frame (Play/Pause/PlaySetup/Win/Lost are
        // mutually exclusive phases) -- see ResetTouchState()'s comment
        // for why it still needs clearing on a keyboard-driven phase
        // change. Values defined locally in the .cpp
        // (kPlayControlDPad/Jump/Action/Pause, kPauseControlMenu/Back/
        // Setup/Restart/Continue, kSetupControlSounds/Jump/Zoom/Accel/
        // Reset/Return, kWinLostControlReturn) since only this class
        // needs them.
        int activeControl_ = -1;
        bool mouseWasDown_ = false;

        // Live D-pad drag offset (reference-space pixels from the pad's
        // center), used only to position the thumb icon in DrawPlay --
        // the discrete {-1,0,+1} UpdatePlay() output is a separate,
        // thresholded read of the same drag.
        float dpadDragOffsetX_ = 0.0f;
        float dpadDragOffsetY_ = 0.0f;

        // Cheat gesture/menu press tracking -- deliberately SEPARATE from
        // activeControl_/mouseWasDown_ above: those are safely shared
        // across Play/Pause/Setup/Resume/WinLost because exactly one of
        // those phases is ever active at a time, but the cheat gesture
        // zones (and, once unlocked, the cheat menu overlay) are checked
        // DURING Play, simultaneously with the normal D-pad/Jump/Action/
        // Pause controls -- reusing the same tracker would corrupt
        // whichever one runs second in a given frame. cheatGestureIndex_
        // is how far through the real 10-tap sequence progress is so far.
        int cheatActiveControl_ = -1;
        bool cheatMouseWasDown_ = false;
        int cheatGestureIndex_ = 0;

        // Typed-cheat-code buffer (plan.md BLUPI-111, see
        // UpdateTypedGhostCheat()'s own comment) -- real rolling
        // lowercase-letter buffer plus per-key down-edge tracking.
        std::string typedCheatBuffer_;
        bool letterKeyWasDown_[26] = {};
    };
}
