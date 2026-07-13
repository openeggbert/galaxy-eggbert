#pragma once

#include <Easy3D/BillboardMesh.hpp>
#include <Easy3D/BillboardMeshRenderer.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
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
        // the real source exactly. Continue/Restart/Setup are wired to
        // real, distinct behavior here (return to Play; return to Play +
        // reset to spawn; and enter PlaySetup, confirmed 2026-07-13 via
        // `Game1.cpp`'s real `PauseSetup -> SetPhase(PlaySetup)`); Menu/
        // Back are rendered at their real positions/icons but are NOT
        // wired to any action yet, since the real destinations they'd
        // lead to (main menu, hub-world navigation) don't exist in this
        // engine yet -- a documented gap, not a silent omission.
        // DrawPause() also draws each visible button's real text label
        // underneath it (confirmed 2026-07-13 against `Game1::
        // DrawButtonsText()`'s real `DrawTextUnderButton()` calls for
        // `Phase::Pause` -- "Home"/"Back"/"Setup"/"Restart"/"Continue",
        // the real English strings; note the real `Menu`/`PauseMenu`
        // button's real EN text is "Home", not "Menu").
        struct PauseInput
        {
            bool continuePressed = false;
            bool restartPressed = false;
            bool setupPressed = false;
        };
        [[nodiscard]] PauseInput UpdatePause(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                             int viewportW, int viewportH,
                                             bool showBack, bool showRestart) noexcept;
        void DrawPause(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                       int viewportW, int viewportH, bool showBack, bool showRestart);

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
        // (Init)`) doesn't exist in this engine -- the caller's own
        // return-to-Play-at-spawn simplification (already established for
        // the keyboard path, HUD-023) is reused here, not a new one.
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

        // PlaySetup screen (plan.md MENU-058..069), reachable only via
        // Pause's real Setup button (see UpdatePause()'s own comment) --
        // MainSetup (reachable from a real Init/main-menu screen this
        // engine doesn't have) is real-but-unreachable here, same pattern
        // as Trial/MainSetup/Resume/Ranking in GamePhase's own comment.
        // Real setup.png background (confirmed exact 640x480). Real button
        // rects verified directly against `InputPad.cpp`'s own
        // bsf2=drawBoundsHeight*140/480 formula -- AT drawBoundsHeight=480
        // (this engine's own reference height) bsf2 is EXACTLY 140,
        // meaning these rects need no proportional adaptation at all
        // (unlike the Pause row's bsf1-based layout) -- a rare literal
        // 1:1 port. SetupSounds is functionally wired to the pre-existing
        // GESound::SetEnabled()/IsEnabled() (a real, meaningful desktop
        // equivalent of the real sound on/off toggle); its icon really
        // swaps between 13 (on) and 21 (off), not just an opacity change
        // (confirmed via `Pixmap.cpp`'s own `selected ? 13 : 21` -- a
        // DIFFERENT real "pressed" convention from every other button in
        // this class). SetupJump/SetupZoom/SetupAccel/SetupReset render at
        // their real positions/icons/labels but are intentionally inert:
        // Jump-button-side and accelerometer-tilt controls have no
        // meaningful desktop equivalent, auto-zoom has no camera-zoom
        // concept in this engine yet, and Reset needs GameData (doesn't
        // exist) -- documented gaps, not silent omissions (SetupReset's
        // own real label additionally needs a real gamer letter/number
        // this engine has no concept of, so its label is deliberately not
        // rendered at all rather than inventing one). SetupReturn is
        // real-position/icon/label AND fully functional (confirmed via
        // `Game1.cpp`'s real `SetupReturn` handler: `if (playSetup)
        // SetPhase(Play,-1); else SetPhase(Init);` -- MainSetup's Init
        // branch is unreachable here, so this always resumes Play in
        // place, no origin-respawn simplification needed since PlaySetup
        // never actually stops gameplay progress).
        //
        // Explicitly NOT ported (documented simplifications, same
        // precedent as Pause/Win/Lost skipping some real animations): the
        // 2 rotating gear.png background decorations and the
        // speedyblupi.png slide-in -- both pure cosmetic flourish with no
        // functional value, requiring an indefinitely-continuing rotation
        // formula and a second texture used nowhere else in this class.
        struct SetupInput
        {
            bool soundsToggled = false;
            bool returnPressed = false;
        };
        [[nodiscard]] SetupInput UpdateSetup(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                             int viewportW, int viewportH) noexcept;
        void DrawSetup(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                       int viewportW, int viewportH, bool soundsOn);

        // Resume screen (plan.md MENU-040..045). Real background is
        // pause.png, the SAME image as Pause (confirmed via `Game1.cpp`'s
        // real `SetPhase()` background dispatch: `case Phase::Pause: case
        // Phase::Resume: BackgroundCache("pause");` -- one shared case),
        // reusing `DrawPause()`'s own background+character draw (static,
        // same documented non-animation simplification already
        // established for Pause). Only 2 real buttons, a DIFFERENT set
        // from Pause's 5: ResumeMenu (icon 11, same as PauseMenu -- real
        // rect independently re-derived from `InputPad.cpp`'s own
        // bsf2=140 formula, NOT reused from PauseMenu's rect, which is a
        // different position) and ResumeContinue (icon 10, same as
        // PauseContinue, its own distinct rect). Both get real text
        // labels ("Home"/"Continue", confirmed via `Game1::
        // DrawButtonsText()`'s real `Phase::Resume` branch -- same real
        // strings as Pause's own Menu/Continue labels).
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
        // ResumeMenu is real-position/icon/label but intentionally inert
        // (same reasoning as Pause's own Menu button -- no Init/main-menu
        // screen exists). ResumeContinue is the caller's responsibility
        // to wire (restore saved lives, reset Blupi to spawn, return to
        // Play) -- NOT a true real mid-level resume (no serialized
        // position/treasure/key state exists to restore), a documented
        // simplification matching the one already established for
        // PauseRestart/WinLostReturn.
        [[nodiscard]] bool UpdateResume(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                        int viewportW, int viewportH) noexcept;
        void DrawResume(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                        int viewportW, int viewportH);

    private:
        struct Quad
        {
            float x0, y0, x1, y1;
            float u0, v0, u1, v1;
        };

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

        Microsoft::Xna::Framework::Graphics::Texture2D padTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D pauseBgTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D blupiyoupieTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D winBgTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D lostBgTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D setupBgTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D textTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> padEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> pauseBgEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> blupiyoupieEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> winBgEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> lostBgEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> setupBgEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> textEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> padRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> padPressedRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> pauseBgRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> blupiyoupieRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> winBgRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> lostBgRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> setupBgRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> textRenderer_;
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
    };
}
