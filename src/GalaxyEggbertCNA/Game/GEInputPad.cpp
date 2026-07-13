#include "GEInputPad.hpp"

#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>
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

        struct Rect { float x0, y0, x1, y1; };

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

        constexpr int kWinLostControlReturn = 0;

        constexpr int kSetupControlSounds = 0;
        constexpr int kSetupControlJump = 1;
        constexpr int kSetupControlZoom = 2;
        constexpr int kSetupControlAccel = 3;
        constexpr int kSetupControlReset = 4;
        constexpr int kSetupControlReturn = 5;

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

        // Real Lost-screen spin: the sprite rotates about its own rect
        // CENTER (confirmed via mobile-eggbert's `Misc::RotateAdjust` --
        // it compensates SpriteBatch's real top-left rotation origin by
        // shifting the rect so the visual pivot lands on the center
        // instead). Builds one standalone quad directly rather than going
        // through the axis-aligned Quad/FlushQuads path above, since a
        // rotated quad's 4 corners aren't expressible as a single (x0,y0)-
        // (x1,y1) rect. Positive rotationDegrees is clockwise on screen
        // (standard XNA SpriteBatch convention in this Y-down space).
        void AppendRotatedQuadUv(std::vector<Easy3D::BillboardVertex>& vertices,
                                 std::vector<std::uint32_t>& indices,
                                 float centerX, float centerY, float halfW, float halfH,
                                 float rotationDegrees,
                                 float u0, float v0, float u1, float v1)
        {
            const float rad = rotationDegrees * (3.14159265f / 180.0f);
            const float c = std::cos(rad);
            const float s = std::sin(rad);
            const auto rotate = [&](float dx, float dy, float& outX, float& outY)
            {
                outX = centerX + dx * c - dy * s;
                outY = centerY + dx * s + dy * c;
            };
            float x0, y0, x1, y1, x2, y2, x3, y3;
            rotate(-halfW, -halfH, x0, y0);
            rotate(halfW, -halfH, x1, y1);
            rotate(halfW, halfH, x2, y2);
            rotate(-halfW, halfH, x3, y3);

            const auto base = static_cast<std::uint32_t>(vertices.size());
            vertices.push_back({{x0, y0, 0.0f}, {u0, v0}});
            vertices.push_back({{x1, y1, 0.0f}, {u1, v0}});
            vertices.push_back({{x2, y2, 0.0f}, {u1, v1}});
            vertices.push_back({{x3, y3, 0.0f}, {u0, v1}});
            indices.push_back(base + 0);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
            indices.push_back(base + 0);
            indices.push_back(base + 2);
            indices.push_back(base + 3);
        }
    }

    void GEInputPad::LoadContent(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        using Microsoft::Xna::Framework::Graphics::BasicEffect;
        using Microsoft::Xna::Framework::Graphics::Texture2D;

        const char* kPaths[7] = {
            "Content/icons/pad.png",
            "Content/backgrounds/pause.png",
            "Content/backgrounds/blupiyoupie.png",
            "Content/backgrounds/win.png",
            "Content/backgrounds/lost.png",
            "Content/backgrounds/setup.png",
            "Content/icons/text.png",
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
            // Continue/Restart/Setup are wired to real behavior (see
            // GEInputPad.hpp's UpdatePause() comment) -- Menu/Back render
            // at their real position/icon but are intentionally inert, no
            // destination screen exists yet.
            if (activeControl_ == kPauseControlContinue) result.continuePressed = true;
            else if (activeControl_ == kPauseControlRestart) result.restartPressed = true;
            else if (activeControl_ == kPauseControlSetup) result.setupPressed = true;
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

        // Real button labels (confirmed 2026-07-13 against `Game1::
        // DrawButtonsText()`'s real `DrawTextUnderButton()` calls for
        // `Phase::Pause`): centered under each VISIBLE button, at real
        // scale 0.7, Y = button's real bottom edge + 2. Real EN strings --
        // note PauseMenu's real text is "Home", not "Menu".
        std::vector<Quad> labelQuads;
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

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
        FlushQuads(device, *pauseBgEffect_, pauseBgRenderer_, backgroundQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *blupiyoupieEffect_, blupiyoupieRenderer_, characterQuads, viewportW, viewportH, 1.0f);
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
                                                    int viewportW, int viewportH) noexcept
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
        const bool overReset = InRect(mouseRefX, mouseRefY, kSetupResetRect);
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
            // Only Sounds (a real, meaningful desktop toggle) and Return
            // are wired to real behavior -- Jump/Zoom/Accel/Reset render
            // at their real position/icon/label but are intentionally
            // inert (see GEInputPad.hpp's UpdateSetup() class comment).
            if (activeControl_ == kSetupControlSounds) result.soundsToggled = true;
            else if (activeControl_ == kSetupControlReturn) result.returnPressed = true;
            activeControl_ = -1;
        }

        mouseWasDown_ = mouseDown;
        return result;
    }

    void GEInputPad::DrawSetup(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                              int viewportW, int viewportH, bool soundsOn)
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
        // space, same as pause.png/win.png/lost.png.
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

        const float padSheetW = static_cast<float>(padTexture_.getWidthProperty());
        const float padSheetH = static_cast<float>(padTexture_.getHeightProperty());
        std::vector<Quad> normalQuads;
        std::vector<Quad> pressedQuads;
        std::vector<Quad> labelQuads;

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
        // buttons -- Jump/Zoom/Accel have no real state tracked in this
        // engine (no meaningful desktop equivalent, see class comment), so
        // they always render the "off" icon.
        appendButton(kSetupSoundsRect, soundsOn ? kIconSetupToggleOn : kIconSetupToggleOff, kSetupControlSounds);
        appendButton(kSetupJumpRect, kIconSetupToggleOff, kSetupControlJump);
        appendButton(kSetupZoomRect, kIconSetupToggleOff, kSetupControlZoom);
        appendButton(kSetupAccelRect, kIconSetupToggleOff, kSetupControlAccel);
        appendButton(kSetupResetRect, kIconSetupReset, kSetupControlReset);
        appendButton(kSetupReturnRect, kIconSetupReturn, kSetupControlReturn);

        appendLabel(kSetupSoundsRect, "Sound effects");
        appendLabel(kSetupJumpRect, "Jump button on the right");
        appendLabel(kSetupZoomRect, "Automatic zoom on action");
        appendLabel(kSetupAccelRect, "Accelerometer");
        // SetupReset's real label needs a real gamer letter/number this
        // engine has no concept of -- deliberately not rendered rather
        // than inventing one (see class comment). SetupReturn has no
        // real label at all in the source (same as WinLostReturn).

        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
        FlushQuads(device, *setupBgEffect_, setupBgRenderer_, backgroundQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padRenderer_, normalQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *padEffect_, padPressedRenderer_, pressedQuads, viewportW, viewportH, kPausePressedAlpha);
        FlushQuads(device, *textEffect_, textRenderer_, labelQuads, viewportW, viewportH, 1.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }
}
