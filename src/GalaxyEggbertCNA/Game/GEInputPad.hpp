#pragma once

#include <Easy3D/BillboardMesh.hpp>
#include <Easy3D/BillboardMeshRenderer.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Input/MouseState.hpp>

#include <memory>
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
        // for the existing reference space) and Content/backgrounds/
        // blupiyoupie.png (real Pause character art, confirmed 410x380,
        // drawn centered at real position (418,190), static/un-animated --
        // the real scale/rotate-in intro animation is a documented
        // simplification).
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
        // the real source exactly. Only Continue/Restart are wired to
        // real, distinct behavior here (return to Play, and return to Play
        // + reset to spawn); Menu/Back/Setup are rendered at their real
        // positions/icons but are NOT wired to any action yet, since the
        // real destinations they'd lead to (main menu, hub-world
        // navigation, settings screen) don't exist in this engine yet --
        // a documented gap, not a silent omission.
        struct PauseInput
        {
            bool continuePressed = false;
            bool restartPressed = false;
        };
        [[nodiscard]] PauseInput UpdatePause(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                             int viewportW, int viewportH,
                                             bool showBack, bool showRestart) noexcept;
        void DrawPause(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                       int viewportW, int viewportH, bool showBack, bool showRestart);

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

        Microsoft::Xna::Framework::Graphics::Texture2D padTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D pauseBgTexture_;
        Microsoft::Xna::Framework::Graphics::Texture2D blupiyoupieTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> padEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> pauseBgEffect_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> blupiyoupieEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> padRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> padPressedRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> pauseBgRenderer_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> blupiyoupieRenderer_;
        bool loaded_ = false;

        // Edge-trigger press tracking: which logical button (if any) the
        // current mouse-down started inside, per the real "must have
        // pressed down inside, fires on release" model. Encoded as a
        // small int rather than a real ButtonGlyph-style enum since only
        // one of UpdatePlay/UpdatePause ever runs in a given frame (Play
        // and Pause are mutually exclusive phases) -- see
        // ResetTouchState()'s comment for why it still needs clearing on
        // a keyboard-driven phase change. Values defined locally in the
        // .cpp (kPlayControlDPad/Jump/Action/Pause, kPauseControlMenu/
        // Back/Setup/Restart/Continue) since only this class needs them.
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
