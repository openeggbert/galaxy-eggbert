#include "GEInputPad.hpp"

#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>

#include <algorithm>
#include <cstdio>
#include <filesystem>

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

        struct Rect { float x0, y0, x1, y1; };

        constexpr Rect kJumpRect{kJumpX0, kJumpY0, kJumpX1, kJumpY1};
        constexpr Rect kActionRect{kActionX0, kActionY0, kActionX1, kActionY1};
        constexpr Rect kPlayPauseRect{kPlayPauseX0, kPlayPauseY0, kPlayPauseX1, kPlayPauseY1};
        constexpr Rect kDPadHitRect{kDPadCenterX - kDPadHitHalf, kDPadCenterY - kDPadHitHalf,
                                     kDPadCenterX + kDPadHitHalf, kDPadCenterY + kDPadHitHalf};

        Rect PauseButtonRect(int index)
        {
            const float x0 = kPauseRowX0 + static_cast<float>(index) * (kPauseButtonSize + kPauseButtonGap);
            return Rect{x0, kPauseRowY0, x0 + kPauseButtonSize, kPauseRowY1};
        }

        bool InRect(float x, float y, const Rect& r)
        {
            return x >= r.x0 && x < r.x1 && y >= r.y0 && y < r.y1;
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

        void AppendQuadUv(std::vector<Easy3D::BillboardVertex>& vertices,
                          std::vector<std::uint32_t>& indices,
                          float x0, float y0, float x1, float y1,
                          float u0, float v0, float u1, float v1)
        {
            const auto base = static_cast<std::uint32_t>(vertices.size());
            vertices.push_back({{x0, y0, 0.0f}, {u0, v0}});
            vertices.push_back({{x1, y0, 0.0f}, {u1, v0}});
            vertices.push_back({{x1, y1, 0.0f}, {u1, v1}});
            vertices.push_back({{x0, y1, 0.0f}, {u0, v1}});
            indices.push_back(base + 0);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
            indices.push_back(base + 0);
            indices.push_back(base + 2);
            indices.push_back(base + 3);
        }

        void PadIconUv(int icon, float sheetW, float sheetH, float& u0, float& v0, float& u1, float& v1)
        {
            const int col = icon % kPadCols;
            const int row = icon / kPadCols;
            u0 = (static_cast<float>(col) * kPadCellPx) / sheetW;
            v0 = (static_cast<float>(row) * kPadCellPx) / sheetH;
            u1 = (static_cast<float>(col + 1) * kPadCellPx) / sheetW;
            v1 = (static_cast<float>(row + 1) * kPadCellPx) / sheetH;
        }
    }

    void GEInputPad::LoadContent(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        using Microsoft::Xna::Framework::Graphics::BasicEffect;
        using Microsoft::Xna::Framework::Graphics::Texture2D;

        const char* kPaths[3] = {
            "Content/icons/pad.png",
            "Content/backgrounds/pause.png",
            "Content/backgrounds/blupiyoupie.png",
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
        if (quads.empty())
        {
            return;
        }
        std::vector<Easy3D::BillboardVertex> vertices;
        std::vector<std::uint32_t> indices;
        vertices.reserve(quads.size() * 4);
        indices.reserve(quads.size() * 6);
        for (const Quad& q : quads)
        {
            AppendQuadUv(vertices, indices, q.x0, q.y0, q.x1, q.y1, q.u0, q.v0, q.u1, q.v1);
        }

        effect.World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
        effect.View = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
        effect.Projection = Microsoft::Xna::Framework::Matrix::CreateOrthographicOffCenter(
            0.0f, static_cast<float>(viewportW), static_cast<float>(viewportH), 0.0f, 0.0f, 1.0f);
        effect.setAlphaProperty(alpha);

        renderer = std::make_unique<Easy3D::BillboardMeshRenderer>(device, vertices, indices);
        renderer->Draw(device, effect);
        effect.setAlphaProperty(1.0f);
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
            // Only Continue/Restart are wired to real behavior (see
            // GEInputPad.hpp's UpdatePause() comment) -- Menu/Back/Setup
            // render at their real position/icon but are intentionally
            // inert, no destination screen exists yet.
            if (activeControl_ == kPauseControlContinue) result.continuePressed = true;
            else if (activeControl_ == kPauseControlRestart) result.restartPressed = true;
            activeControl_ = -1;
        }

        mouseWasDown_ = mouseDown;
        return result;
    }

    void GEInputPad::DrawPause(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                              int viewportW, int viewportH, bool showBack, bool showRestart)
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

        const float charW = static_cast<float>(blupiyoupieTexture_.getWidthProperty());
        const float charH = static_cast<float>(blupiyoupieTexture_.getHeightProperty());
        Quad character;
        character.x0 = refToScreenX(kCharacterCenterX - charW * 0.5f);
        character.y0 = refToScreenY(kCharacterCenterY - charH * 0.5f);
        character.x1 = refToScreenX(kCharacterCenterX + charW * 0.5f);
        character.y1 = refToScreenY(kCharacterCenterY + charH * 0.5f);
        character.u0 = 0.0f;
        character.v0 = 0.0f;
        character.u1 = 1.0f;
        character.v1 = 1.0f;
        std::vector<Quad> characterQuads{character};

        const float padSheetW = static_cast<float>(padTexture_.getWidthProperty());
        const float padSheetH = static_cast<float>(padTexture_.getHeightProperty());
        std::vector<Quad> normalQuads;
        std::vector<Quad> pressedQuads;

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

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
        FlushQuads(device, *pauseBgEffect_, pauseBgRenderer_, backgroundQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *blupiyoupieEffect_, blupiyoupieRenderer_, characterQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padRenderer_, normalQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressedQuads, viewportW, viewportH, kPausePressedAlpha);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }
}
