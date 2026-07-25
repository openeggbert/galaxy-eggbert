#include "Hud.hpp"

#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Vector4.hpp>

#include <algorithm>
#include <cstdio>
#include <filesystem>

namespace GalaxyEggbert::Game
{
    namespace
    {
        // Real mobile-eggbert HUD layout constants (Decor::DrawInfo,
        // Decor.cpp:1185-1249), all in the original 640x480 screen space.
        constexpr float kRefW = 640.0f;
        constexpr float kRefH = 480.0f;
        constexpr float kLivesX = 210.0f, kLivesY = 417.0f, kLivesStep = 16.0f;
        constexpr float kKey1X = 520.0f, kKey2X = 530.0f, kKey3X = 540.0f, kKeyY = 418.0f;
        constexpr float kPanelX0 = 410.0f, kPanelY0 = 445.0f, kPanelX1 = 510.0f, kPanelY1 = 480.0f;
        constexpr float kTreasureTextCenterX = 460.0f, kTreasureTextY = 450.0f;
        // Training-hint banner (plan.md HUD-024, Decor.cpp:1294-1306):
        // full-width panel at the very top of screen, real opacity 1.0
        // (unlike the treasure panel's 0.6).
        constexpr float kHintPanelX0 = 0.0f, kHintPanelY0 = 0.0f, kHintPanelX1 = 640.0f, kHintPanelY1 = 40.0f;
        constexpr float kHintTextCenterX = 320.0f, kHintTextBaseY = 5.0f;
        // Real `(1.0 - scale) * 35.0 * 0.6` vertical nudge as text shrinks
        // to fit the panel width.
        constexpr float kHintTextYNudge = 35.0f * 0.6f;
        constexpr float kBulletX = 570.0f, kBulletY = 442.0f, kBulletStep = 4.0f;
        constexpr float kDynamiteX = 505.0f, kDynamiteY = 414.0f;
        constexpr int kKeyIcon1 = 215, kKeyIcon2 = 222, kKeyIcon3 = 229; // element.png, same as DrawInfo
        constexpr int kLifeIcon = 48;  // blupi.png, same as DrawInfo
        constexpr int kPanelIcon = 15; // pad.png, same as DrawInfo
        constexpr int kBulletIcon = 176;   // element.png, same as DrawInfo
        constexpr int kDynamiteIcon = 252; // element.png, same as DrawInfo

        // button.png: 40px cells, 6 columns (sheet 240x1040, Pixmap.cpp:
        // 592-600). Perso icon 108 at (0,438); its "= N" text at (32,452)
        // renders at real scale 0.7 (smaller than the treasure counter's
        // scale-1.0 text).
        constexpr float kButtonTilePx = 40.0f;
        constexpr int kButtonCols = 6;
        constexpr float kPersoX = 0.0f, kPersoY = 438.0f;
        constexpr float kPersoTextX = 32.0f, kPersoTextY = 452.0f;
        constexpr float kPersoTextScale = 0.7f;
        constexpr int kPersoIcon = 108; // button.png, same as DrawInfo

        // jauge.png: 124x88px, 4 stacked 22px-tall rows (JaugeMode Empty=0/
        // Red=1/Blue=2/Yellow=3 -- Jauge.hpp). Real Jauge::Draw() is a
        // two-layer sprite: the full 124x22 Empty row always drawn as a
        // background, then (if level>0) a colored strip from the mode's
        // row, cropped to [6, 6+level*114/100] pixels -- fill starts after
        // a fixed 6px left border, matching real Jauge.hpp exactly.
        constexpr float kJaugeW = 124.0f, kJaugeH = 22.0f;
        constexpr float kJaugeFillX0 = 6.0f, kJaugeFillMaxW = 114.0f;
        constexpr int kJaugeRowEmpty = 0, kJaugeRowRed = 1, kJaugeRowBlue = 2, kJaugeRowYellow = 3;
        // Real Decor.cpp positions: m_jauges[0] (water/Nage breath, Blue/Red)
        // at (90,450); m_jauges[1] (Shield/Power/Cloud/Hide shared timer,
        // always Yellow) at (90,428).
        constexpr float kWaterGaugeX = 90.0f, kWaterGaugeY = 450.0f;
        constexpr float kPowerGaugeX = 90.0f, kPowerGaugeY = 428.0f;
        // Real low-air warning: Decor.cpp:4621-4623 switches m_jauges[0]
        // from Blue to Red exactly when the water level drops to 25.
        constexpr int kWaterGaugeWarnLevel = 25;
        // The real DrawInfo panel opacity is 0.6 -- restored 2026-07-18 now
        // that EasyGL (not Vulkan) is the default graphics backend
        // (CMakeLists.txt), which renders a BasicEffect Alpha<1 draw
        // correctly. Forced to 1.0 from 2026-07-10 through today: on CNA's
        // Vulkan backend a BasicEffect draw with Alpha < 1 doesn't render at
        // all (verified empirically 2026-07-10: the identical panel quad
        // appears at 1.0 and vanishes at 0.6, while EasyGL shows it at
        // both). This is a genuine CNA/Vulkan-only bug, still unfixed --
        // building GalaxyEggbertCNA with `-DCNA_GRAPHICS_BACKEND=VULKAN`
        // will make every semi-transparent panel vanish again.
        constexpr float kPanelOpacity = 0.6f;

        // blupi.png / element.png share the 60px/10-column tile convention
        // (ObjectIcons::GetElementIconUv, BlupiController's kTilePx/kCols).
        constexpr float kIconTilePx = 60.0f;
        constexpr int kIconCols = 10;

        // text.png: 32px glyph cells, 16 columns; the glyph index IS the
        // ASCII code for the printable range (see Hud.hpp's class comment
        // -- read off the asset, not mobile-eggbert's table_char).
        constexpr float kGlyphCellPx = 32.0f;
        constexpr int kGlyphCols = 16;
        // Fixed advance approximation (the real font is proportional via
        // table_char_width, deliberately not transcribed).
        constexpr float kGlyphAdvance = 17.0f;

        // pad.png: 140px cells, 8 columns (sheet 1120x420).
        constexpr float kPadCellPx = 140.0f;
        constexpr int kPadCols = 8;

        // Interim animation-state indicator (bottom-right) -- not part of
        // the real mobile-eggbert HUD, a debug stand-in until a real Blupi
        // model exists; same 96px on-screen box as the old SpriteBatch
        // version, expressed in the 640x480 reference space.
        constexpr float kAnimIndicatorSize = 77.0f; // ~96px at the old 800x480 window's scale
        constexpr float kAnimIndicatorMargin = 6.0f;

        void AppendQuadUv(std::vector<Easy3D::BillboardVertex>& vertices,
                          std::vector<std::uint32_t>& indices,
                          float x0, float y0, float x1, float y1,
                          float u0, float v0, float u1, float v1)
        {
            // Pixel coordinates with y down (matching the orthographic
            // projection in FlushQuads): corner order top-left, top-right,
            // bottom-right, bottom-left is visually CLOCKWISE on screen --
            // the winding that survives XNA's default CullCounterClockwise
            // (same convention as easy-3d's CubeMesh/BillboardMesh after
            // the 2026-07-10 winding root-cause fix).
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

        // UVs for a fixed-cell icon sheet whose real pixel size is known
        // only at runtime (blupi.png's height differs from element.png's).
        // Defaults match the 60px/10-col blupi.png/element.png convention;
        // button.png (40px/6-col) passes its own tileSize/cols explicitly.
        void IconUv(int icon, float sheetW, float sheetH,
                    float& u0, float& v0, float& u1, float& v1,
                    float tileSize = kIconTilePx, int cols = kIconCols)
        {
            const int col = icon % cols;
            const int row = icon / cols;
            u0 = (static_cast<float>(col) * tileSize) / sheetW;
            v0 = (static_cast<float>(row) * tileSize) / sheetH;
            u1 = (static_cast<float>(col + 1) * tileSize) / sheetW;
            v1 = (static_cast<float>(row + 1) * tileSize) / sheetH;
        }
    }

    void Hud::LoadContent(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        using Microsoft::Xna::Framework::Graphics::BasicEffect;
        using Microsoft::Xna::Framework::Graphics::Texture2D;

        const char* kPaths[6] = {
            "Content/icons/blupi.png",
            "Content/icons/element.png",
            "Content/icons/text.png",
            "Content/icons/pad.png",
            "Content/icons/jauge.png",
            "Content/icons/button.png",
        };
        for (const char* path : kPaths)
        {
            if (!std::filesystem::exists(path))
            {
                std::printf("Hud: %s not found -- HUD disabled.\n", path);
                return;
            }
        }

        blupiTexture_ = Texture2D(kPaths[0], device);
        elementTexture_ = Texture2D(kPaths[1], device);
        textTexture_ = Texture2D(kPaths[2], device);
        padTexture_ = Texture2D(kPaths[3], device);
        jaugeTexture_ = Texture2D(kPaths[4], device);
        buttonTexture_ = Texture2D(kPaths[5], device);

        const auto makeEffect = [&device](Texture2D& texture)
        {
            auto effect = std::make_unique<BasicEffect>(device);
            effect->VertexColorEnabled = false;
            effect->setTextureEnabledProperty(true);
            effect->setTextureProperty(&texture);
            return effect;
        };
        blupiEffect_ = makeEffect(blupiTexture_);
        elementEffect_ = makeEffect(elementTexture_);
        textEffect_ = makeEffect(textTexture_);
        padEffect_ = makeEffect(padTexture_);
        jaugeEffect_ = makeEffect(jaugeTexture_);
        buttonEffect_ = makeEffect(buttonTexture_);
        loaded_ = true;
    }

    void Hud::FlushQuads(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
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

        // Orthographic pixel space, y down from the top-left -- the
        // CreateOrthographicOffCenter(left, right, bottom, top) argument
        // order below maps (0,0) to the top-left corner.
        effect.World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
        effect.View = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
        effect.Projection = Microsoft::Xna::Framework::Matrix::CreateOrthographicOffCenter(
            0.0f, static_cast<float>(viewportW), static_cast<float>(viewportH), 0.0f, 0.0f, 1.0f);
        effect.setAlphaProperty(alpha);

        renderer = std::make_unique<Easy3D::BillboardMeshRenderer>(device, vertices, indices);
        renderer->Draw(device, effect);
        effect.setAlphaProperty(1.0f);
    }

    bool Hud::ProjectWorldToHudSpace(Microsoft::Xna::Framework::Vector3 worldPos,
                                       const Microsoft::Xna::Framework::Matrix& view,
                                       const Microsoft::Xna::Framework::Matrix& projection,
                                       int viewportW, int viewportH, float& outX, float& outY)
    {
        using Microsoft::Xna::Framework::Matrix;
        using Microsoft::Xna::Framework::Vector4;

        const Matrix viewProj = Matrix::Multiply(view, projection);
        // Vector4::Transform (NOT Vector3::Transform, which silently drops
        // W) computes the real clip-space W needed for the perspective
        // divide below.
        const Vector4 clip = Vector4::Transform(worldPos, viewProj);
        if (clip.W <= 0.0001f)
        {
            return false; // behind the camera
        }
        const float ndcX = clip.X / clip.W;
        const float ndcY = clip.Y / clip.W;
        const float viewportX = (ndcX * 0.5f + 0.5f) * static_cast<float>(viewportW);
        const float viewportY = (1.0f - (ndcY * 0.5f + 0.5f)) * static_cast<float>(viewportH);

        // Invert this class's own 640x480 <-> viewport mapping (kRefW/
        // kRefH, the refToScreenX/Y lambdas in Draw() below).
        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        outX = (viewportX - offsetX) / scale;
        outY = viewportY / scale;
        return true;
    }

    void Hud::Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                     int viewportW, int viewportH,
                     int lives, bool key1, bool key2, bool key3,
                     int treasures, int totalTreasures, bool showTreasureCounter,
                     int bullets, int dynamite, int perso,
                     bool waterGaugeVisible, int waterGaugeLevel,
                     bool powerGaugeVisible, int powerGaugeLevel,
                     const char* trainingHint,
                     const char* overlayMessage,
                     int animIcon, bool animIconUsesElementSheet,
                     bool voyageActive, int voyageIconId, bool voyageIsButtonChannel,
                     float voyageX, float voyageY,
                     GalaxyEggbert::Def::GameSpeed gameSpeed)
    {
        if (!loaded_)
        {
            return;
        }

        // Map the 640x480 reference layout to the real viewport the same
        // way mobile-eggbert's own origin handling does: uniform scale by
        // height, horizontally centered.
        const float scale = static_cast<float>(viewportH) / kRefH;
        const float offsetX = (static_cast<float>(viewportW) - kRefW * scale) * 0.5f;
        const auto refToScreenX = [&](float x) { return offsetX + x * scale; };
        const auto refToScreenY = [&](float y) { return y * scale; };

        const float textSheetW = static_cast<float>(textTexture_.getWidthProperty());
        const float textSheetH = static_cast<float>(textTexture_.getHeightProperty());

        // Non-Play phase overlay (plan.md HUD-023): the real HUD is fully
        // hidden, replaced by just this one big centered message.
        if (overlayMessage != nullptr && overlayMessage[0] != '\0')
        {
            std::vector<Quad> messageQuads;
            const std::string str(overlayMessage);
            constexpr float kMessageScale = 1.5f;
            const float cellPx = kGlyphCellPx * kMessageScale;
            const float advance = kGlyphAdvance * kMessageScale;
            const float totalAdvance = static_cast<float>(str.size()) * advance;
            float penX = kRefW * 0.5f - totalAdvance * 0.5f;
            const float textY = kRefH * 0.5f - cellPx * 0.5f;
            for (const char c : str)
            {
                const int rank = static_cast<int>(static_cast<unsigned char>(c));
                const int gcol = rank % kGlyphCols;
                const int grow = rank / kGlyphCols;
                Quad q;
                q.x0 = refToScreenX(penX - (cellPx - advance) * 0.5f);
                q.y0 = refToScreenY(textY);
                q.x1 = q.x0 + cellPx * scale;
                q.y1 = q.y0 + cellPx * scale;
                q.u0 = (static_cast<float>(gcol) * kGlyphCellPx) / textSheetW;
                q.v0 = (static_cast<float>(grow) * kGlyphCellPx) / textSheetH;
                q.u1 = (static_cast<float>(gcol + 1) * kGlyphCellPx) / textSheetW;
                q.v1 = (static_cast<float>(grow + 1) * kGlyphCellPx) / textSheetH;
                messageQuads.push_back(q);
                penX += advance;
            }
            device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
            FlushQuads(device, *textEffect_, textRenderer_, messageQuads, viewportW, viewportH, 1.0f);
            device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
            return;
        }

        const float blupiSheetW = static_cast<float>(blupiTexture_.getWidthProperty());
        const float blupiSheetH = static_cast<float>(blupiTexture_.getHeightProperty());
        const float elementSheetW = static_cast<float>(elementTexture_.getWidthProperty());
        const float elementSheetH = static_cast<float>(elementTexture_.getHeightProperty());
        const float jaugeSheetW = static_cast<float>(jaugeTexture_.getWidthProperty());
        const float jaugeSheetH = static_cast<float>(jaugeTexture_.getHeightProperty());
        const float buttonSheetW = static_cast<float>(buttonTexture_.getWidthProperty());
        const float buttonSheetH = static_cast<float>(buttonTexture_.getHeightProperty());

        std::vector<Quad> blupiQuads;
        std::vector<Quad> elementQuads;
        std::vector<Quad> textQuads;
        std::vector<Quad> padQuads;
        std::vector<Quad> jaugeQuads;
        std::vector<Quad> buttonQuads;
        std::vector<Quad> hintPanelQuads;

        // Real Jauge::Draw(): the full Empty-row background always drawn
        // first, then (if level > 0) a colored strip from the mode's row,
        // cropped to [6, 6+level*114/100] pixels (Jauge.hpp).
        const auto appendJauge = [&](float refX, float refY, int level, int modeRow)
        {
            Quad bg;
            bg.x0 = refToScreenX(refX);
            bg.y0 = refToScreenY(refY);
            bg.x1 = bg.x0 + kJaugeW * scale;
            bg.y1 = bg.y0 + kJaugeH * scale;
            bg.u0 = 0.0f;
            bg.v0 = (static_cast<float>(kJaugeRowEmpty) * kJaugeH) / jaugeSheetH;
            bg.u1 = kJaugeW / jaugeSheetW;
            bg.v1 = (static_cast<float>(kJaugeRowEmpty + 1) * kJaugeH) / jaugeSheetH;
            jaugeQuads.push_back(bg);

            if (level > 0)
            {
                const float fillW = kJaugeFillMaxW * static_cast<float>(level) / 100.0f;
                Quad fill;
                fill.x0 = refToScreenX(refX + kJaugeFillX0);
                fill.y0 = refToScreenY(refY);
                fill.x1 = fill.x0 + fillW * scale;
                fill.y1 = fill.y0 + kJaugeH * scale;
                fill.u0 = kJaugeFillX0 / jaugeSheetW;
                fill.v0 = (static_cast<float>(modeRow) * kJaugeH) / jaugeSheetH;
                fill.u1 = (kJaugeFillX0 + fillW) / jaugeSheetW;
                fill.v1 = (static_cast<float>(modeRow + 1) * kJaugeH) / jaugeSheetH;
                jaugeQuads.push_back(fill);
            }
        };

        // Lives: one blupi.png icon 48 per life, X += 16 (a fanned,
        // overlapping row -- the real DrawInfo look).
        for (int i = 0; i < lives; ++i)
        {
            Quad q;
            q.x0 = refToScreenX(kLivesX + static_cast<float>(i) * kLivesStep);
            q.y0 = refToScreenY(kLivesY);
            q.x1 = q.x0 + kIconTilePx * scale;
            q.y1 = q.y0 + kIconTilePx * scale;
            IconUv(kLifeIcon, blupiSheetW, blupiSheetH, q.u0, q.v0, q.u1, q.v1);
            blupiQuads.push_back(q);
        }

        // Interim animation-state indicator, bottom-right (see Hud.hpp).
        // Real BlupiSearchIcon() picks the sheet per-action, not a single
        // constant one (mobile-eggbert-reference/08-animations.md §2's own
        // "Channel selection" note) -- animIconUsesElementSheet tells us
        // which of the two this frame's icon actually indexes into (see
        // BlupiController::AnimIconUsesElementSheet()'s own comment).
        {
            Quad q;
            q.x1 = static_cast<float>(viewportW) - kAnimIndicatorMargin * scale;
            q.y1 = static_cast<float>(viewportH) - kAnimIndicatorMargin * scale;
            q.x0 = q.x1 - kAnimIndicatorSize * scale;
            q.y0 = q.y1 - kAnimIndicatorSize * scale;
            if (animIconUsesElementSheet)
            {
                IconUv(animIcon, elementSheetW, elementSheetH, q.u0, q.v0, q.u1, q.v1);
                elementQuads.push_back(q);
            }
            else
            {
                IconUv(animIcon, blupiSheetW, blupiSheetH, q.u0, q.v0, q.u1, q.v1);
                blupiQuads.push_back(q);
            }
        }

        // Held keys, element.png 215/222/229 at their real positions.
        const struct { bool held; float x; int icon; } keys[3] = {
            {key1, kKey1X, kKeyIcon1},
            {key2, kKey2X, kKeyIcon2},
            {key3, kKey3X, kKeyIcon3},
        };
        for (const auto& key : keys)
        {
            if (!key.held)
            {
                continue;
            }
            Quad q;
            q.x0 = refToScreenX(key.x);
            q.y0 = refToScreenY(kKeyY);
            q.x1 = q.x0 + kIconTilePx * scale;
            q.y1 = q.y0 + kIconTilePx * scale;
            IconUv(key.icon, elementSheetW, elementSheetH, q.u0, q.v0, q.u1, q.v1);
            elementQuads.push_back(q);
        }

        // Bullets: element.png icon 176 x bullets held, X += 4 (real
        // DrawInfo) -- a heavily-overlapping "fanned" row, same real look
        // as the life icons above.
        for (int i = 0; i < bullets; ++i)
        {
            Quad q;
            q.x0 = refToScreenX(kBulletX + static_cast<float>(i) * kBulletStep);
            q.y0 = refToScreenY(kBulletY);
            q.x1 = q.x0 + kIconTilePx * scale;
            q.y1 = q.y0 + kIconTilePx * scale;
            IconUv(kBulletIcon, elementSheetW, elementSheetH, q.u0, q.v0, q.u1, q.v1);
            elementQuads.push_back(q);
        }

        // Dynamite: element.png icon 252, single icon while carrying one
        // (real `if (m_blupiDynamite > 0)`, no counter loop -- there's only
        // ever at most 1 in the real game too).
        if (dynamite > 0)
        {
            Quad q;
            q.x0 = refToScreenX(kDynamiteX);
            q.y0 = refToScreenY(kDynamiteY);
            q.x1 = q.x0 + kIconTilePx * scale;
            q.y1 = q.y0 + kIconTilePx * scale;
            IconUv(kDynamiteIcon, elementSheetW, elementSheetH, q.u0, q.v0, q.u1, q.v1);
            elementQuads.push_back(q);
        }

        // Perso: button.png icon 108 + "= N" text at real scale 0.7, shown
        // only while carrying at least one decoy (real `if (m_blupiPerso >
        // 0)`).
        if (perso > 0)
        {
            Quad icon;
            icon.x0 = refToScreenX(kPersoX);
            icon.y0 = refToScreenY(kPersoY);
            icon.x1 = icon.x0 + kButtonTilePx * scale;
            icon.y1 = icon.y0 + kButtonTilePx * scale;
            IconUv(kPersoIcon, buttonSheetW, buttonSheetH, icon.u0, icon.v0, icon.u1, icon.v1,
                   kButtonTilePx, kButtonCols);
            buttonQuads.push_back(icon);

            char text[16];
            std::snprintf(text, sizeof(text), "= %d", perso);
            const std::string str(text);
            const float cellPx = kGlyphCellPx * kPersoTextScale;
            const float advance = kGlyphAdvance * kPersoTextScale;
            float penX = kPersoTextX;
            for (const char c : str)
            {
                const int rank = static_cast<int>(static_cast<unsigned char>(c));
                const int gcol = rank % kGlyphCols;
                const int grow = rank / kGlyphCols;
                Quad q;
                q.x0 = refToScreenX(penX - (cellPx - advance) * 0.5f);
                q.y0 = refToScreenY(kPersoTextY);
                q.x1 = q.x0 + cellPx * scale;
                q.y1 = q.y0 + cellPx * scale;
                q.u0 = (static_cast<float>(gcol) * kGlyphCellPx) / textSheetW;
                q.v0 = (static_cast<float>(grow) * kGlyphCellPx) / textSheetH;
                q.u1 = (static_cast<float>(gcol + 1) * kGlyphCellPx) / textSheetW;
                q.v1 = (static_cast<float>(grow + 1) * kGlyphCellPx) / textSheetH;
                textQuads.push_back(q);
                penX += advance;
            }
        }

        // Voyage (plan.md `158`, real `Decor::VoyageDraw`) -- the flying
        // pickup-reward icon, drawn at its current interpolated position
        // (already in this class's own 640x480 reference space). Same
        // quad-building pattern as every fixed-position icon above, just
        // with a dynamic position instead of a fixed constant.
        if (voyageActive)
        {
            Quad q;
            const float tileSize = voyageIsButtonChannel ? kButtonTilePx : kIconTilePx;
            q.x0 = refToScreenX(voyageX);
            q.y0 = refToScreenY(voyageY);
            q.x1 = q.x0 + tileSize * scale;
            q.y1 = q.y0 + tileSize * scale;
            if (voyageIsButtonChannel)
            {
                IconUv(voyageIconId, buttonSheetW, buttonSheetH, q.u0, q.v0, q.u1, q.v1, kButtonTilePx, kButtonCols);
                buttonQuads.push_back(q);
            }
            else
            {
                IconUv(voyageIconId, elementSheetW, elementSheetH, q.u0, q.v0, q.u1, q.v1);
                elementQuads.push_back(q);
            }
        }

        // Treasure counter: pad.png icon-15 panel + centered "N/M" text.
        // Real gate (SCORE-017, wired 2026-07-20): `Decor.cpp:1236`,
        // `(m_mission != 1 && m_mission % 10 != 0) || m_bPrivate` -- hidden
        // on the global hub and every world hub (m_bPrivate never applies
        // here, no custom-level-load path exists). showTreasureCounter is
        // the caller's own version of that same check (mirrors the
        // Pause-menu `showRestart` gate already computed the identical
        // way). This used to be approximated as just "the world has
        // treasures" -- true in practice since hub worlds have none, but
        // not the real gate.
        if (totalTreasures > 0 && showTreasureCounter)
        {
            Quad panel;
            panel.x0 = refToScreenX(kPanelX0);
            panel.y0 = refToScreenY(kPanelY0);
            panel.x1 = refToScreenX(kPanelX1);
            panel.y1 = refToScreenY(kPanelY1);
            const int col = kPanelIcon % kPadCols;
            const int row = kPanelIcon / kPadCols;
            const float padSheetW = static_cast<float>(padTexture_.getWidthProperty());
            const float padSheetH = static_cast<float>(padTexture_.getHeightProperty());
            panel.u0 = (static_cast<float>(col) * kPadCellPx) / padSheetW;
            panel.v0 = (static_cast<float>(row) * kPadCellPx) / padSheetH;
            panel.u1 = (static_cast<float>(col + 1) * kPadCellPx) / padSheetW;
            panel.v1 = (static_cast<float>(row + 1) * kPadCellPx) / padSheetH;
            padQuads.push_back(panel);

            char text[16];
            std::snprintf(text, sizeof(text), "%d/%d", treasures, totalTreasures);
            const std::string str(text);
            const float totalAdvance = static_cast<float>(str.size()) * kGlyphAdvance;
            float penX = kTreasureTextCenterX - totalAdvance * 0.5f;
            for (const char c : str)
            {
                const int rank = static_cast<int>(static_cast<unsigned char>(c));
                const int gcol = rank % kGlyphCols;
                const int grow = rank / kGlyphCols;
                Quad q;
                q.x0 = refToScreenX(penX - (kGlyphCellPx - kGlyphAdvance) * 0.5f);
                q.y0 = refToScreenY(kTreasureTextY);
                q.x1 = q.x0 + kGlyphCellPx * scale;
                q.y1 = q.y0 + kGlyphCellPx * scale;
                q.u0 = (static_cast<float>(gcol) * kGlyphCellPx) / textSheetW;
                q.v0 = (static_cast<float>(grow) * kGlyphCellPx) / textSheetH;
                q.u1 = (static_cast<float>(gcol + 1) * kGlyphCellPx) / textSheetW;
                q.v1 = (static_cast<float>(grow + 1) * kGlyphCellPx) / textSheetH;
                textQuads.push_back(q);
                penX += kGlyphAdvance;
            }
        }

        // Real GalaxyEggbert::Def::GameSpeed indicator (SCORE-009/010/011, added 2026-07-20,
        // `InputPad.cpp:1383-1404`): shown only while speed != Normal, a
        // small pad.png icon-15 panel + "0.5x"/"Nx" text at the real
        // (5, drawBoundsHeight-22) position -- same panel/text idiom as
        // the treasure counter above, just a different real position/scale.
        if (gameSpeed != GalaxyEggbert::Def::GameSpeed::Normal)
        {
            const bool isSlow = gameSpeed == GalaxyEggbert::Def::GameSpeed::Slow;
            const std::string speedText = isSlow ? "0.5x" : std::to_string(GalaxyEggbert::Def::ToRaw(gameSpeed)) + "x";
            constexpr float kSpeedTextScale = 0.55f;
            constexpr float kSpeedPadding = 3.0f;
            constexpr float kSpeedTextH = 14.0f;
            constexpr float kSpeedBaseY = kRefH - 22.0f;
            const float speedTextW = static_cast<float>(speedText.size()) * kGlyphAdvance * kSpeedTextScale;

            Quad speedPanel;
            speedPanel.x0 = refToScreenX(5.0f - kSpeedPadding);
            speedPanel.y0 = refToScreenY(kSpeedBaseY - kSpeedPadding);
            speedPanel.x1 = refToScreenX(5.0f + speedTextW + kSpeedPadding);
            speedPanel.y1 = refToScreenY(kSpeedBaseY + kSpeedTextH + kSpeedPadding);
            const int speedCol = kPanelIcon % kPadCols;
            const int speedRow = kPanelIcon / kPadCols;
            const float padSheetW = static_cast<float>(padTexture_.getWidthProperty());
            const float padSheetH = static_cast<float>(padTexture_.getHeightProperty());
            speedPanel.u0 = (static_cast<float>(speedCol) * kPadCellPx) / padSheetW;
            speedPanel.v0 = (static_cast<float>(speedRow) * kPadCellPx) / padSheetH;
            speedPanel.u1 = (static_cast<float>(speedCol + 1) * kPadCellPx) / padSheetW;
            speedPanel.v1 = (static_cast<float>(speedRow + 1) * kPadCellPx) / padSheetH;
            padQuads.push_back(speedPanel);

            const float speedCellPx = kGlyphCellPx * kSpeedTextScale;
            const float speedAdvance = kGlyphAdvance * kSpeedTextScale;
            float speedPenX = 5.0f;
            for (const char c : speedText)
            {
                const int rank = static_cast<int>(static_cast<unsigned char>(c));
                const int gcol = rank % kGlyphCols;
                const int grow = rank / kGlyphCols;
                Quad q;
                q.x0 = refToScreenX(speedPenX);
                q.y0 = refToScreenY(kSpeedBaseY);
                q.x1 = q.x0 + speedCellPx * scale;
                q.y1 = q.y0 + speedCellPx * scale;
                q.u0 = (static_cast<float>(gcol) * kGlyphCellPx) / textSheetW;
                q.v0 = (static_cast<float>(grow) * kGlyphCellPx) / textSheetH;
                q.u1 = (static_cast<float>(gcol + 1) * kGlyphCellPx) / textSheetW;
                q.v1 = (static_cast<float>(grow + 1) * kGlyphCellPx) / textSheetH;
                textQuads.push_back(q);
                speedPenX += speedAdvance;
            }
        }

        // Water/Nage breath gauge (real m_jauges[0]): Blue normally, Red
        // once the level drops to the real low-air warning threshold.
        if (waterGaugeVisible)
        {
            appendJauge(kWaterGaugeX, kWaterGaugeY, waterGaugeLevel,
                        waterGaugeLevel <= kWaterGaugeWarnLevel ? kJaugeRowRed : kJaugeRowBlue);
        }

        // Shield/Power/Cloud/Hide shared countdown gauge (real
        // m_jauges[1]), always Yellow.
        if (powerGaugeVisible)
        {
            appendJauge(kPowerGaugeX, kPowerGaugeY, powerGaugeLevel, kJaugeRowYellow);
        }

        // Training-hint banner (plan.md HUD-024): full-width panel at real
        // opacity 1.0 (own quad batch, NOT sharing padQuads/kPanelOpacity --
        // that panel is 0.6 in the real source, even though both currently
        // render at 1.0 here for the same CNA-Vulkan reason, see
        // kPanelOpacity's own comment), with the hint text centered and
        // shrunk to fit if needed.
        if (trainingHint != nullptr && trainingHint[0] != '\0')
        {
            Quad panel;
            panel.x0 = refToScreenX(kHintPanelX0);
            panel.y0 = refToScreenY(kHintPanelY0);
            panel.x1 = refToScreenX(kHintPanelX1);
            panel.y1 = refToScreenY(kHintPanelY1);
            const int col = kPanelIcon % kPadCols;
            const int row = kPanelIcon / kPadCols;
            const float padSheetW = static_cast<float>(padTexture_.getWidthProperty());
            const float padSheetH = static_cast<float>(padTexture_.getHeightProperty());
            panel.u0 = (static_cast<float>(col) * kPadCellPx) / padSheetW;
            panel.v0 = (static_cast<float>(row) * kPadCellPx) / padSheetH;
            panel.u1 = (static_cast<float>(col + 1) * kPadCellPx) / padSheetW;
            panel.v1 = (static_cast<float>(row + 1) * kPadCellPx) / padSheetH;
            hintPanelQuads.push_back(panel);

            const std::string str(trainingHint);
            const float rawWidth = static_cast<float>(str.size()) * kGlyphAdvance;
            const float textScale = rawWidth > 0.0f ? std::min(kHintPanelX1 / rawWidth, 1.0f) : 1.0f;
            const float cellPx = kGlyphCellPx * textScale;
            const float advance = kGlyphAdvance * textScale;
            const float totalAdvance = static_cast<float>(str.size()) * advance;
            const float textY = kHintTextBaseY + (1.0f - textScale) * kHintTextYNudge;
            float penX = kHintTextCenterX - totalAdvance * 0.5f;
            for (const char c : str)
            {
                const int rank = static_cast<int>(static_cast<unsigned char>(c));
                const int gcol = rank % kGlyphCols;
                const int grow = rank / kGlyphCols;
                Quad q;
                q.x0 = refToScreenX(penX - (cellPx - advance) * 0.5f);
                q.y0 = refToScreenY(textY);
                q.x1 = q.x0 + cellPx * scale;
                q.y1 = q.y0 + cellPx * scale;
                q.u0 = (static_cast<float>(gcol) * kGlyphCellPx) / textSheetW;
                q.v0 = (static_cast<float>(grow) * kGlyphCellPx) / textSheetH;
                q.u1 = (static_cast<float>(gcol + 1) * kGlyphCellPx) / textSheetW;
                q.v1 = (static_cast<float>(grow + 1) * kGlyphCellPx) / textSheetH;
                textQuads.push_back(q);
                penX += advance;
            }
        }

        // Alpha blending for the icon sheets' real alpha channels; the
        // panel additionally gets the real 0.6 opacity via BasicEffect's
        // Alpha. Panel first so the text draws on top of it.
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
        FlushQuads(device, *padEffect_, padRenderer_, padQuads, viewportW, viewportH, kPanelOpacity);
        // Reuses padEffect_ (same pad.png texture/shader) but its own
        // dedicated renderer/mesh buffer, at the real, distinct 1.0 opacity
        // for the training-hint banner.
        FlushQuads(device, *padEffect_, hintPanelRenderer_, hintPanelQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *textEffect_, textRenderer_, textQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *blupiEffect_, blupiRenderer_, blupiQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *elementEffect_, elementRenderer_, elementQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *jaugeEffect_, jaugeRenderer_, jaugeQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *buttonEffect_, buttonRenderer_, buttonQuads, viewportW, viewportH, 1.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }
}
