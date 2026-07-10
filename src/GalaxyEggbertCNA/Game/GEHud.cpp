#include "GEHud.hpp"

#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>

#include <cstdio>
#include <filesystem>

namespace GalaxyEggbert::CNA
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
        constexpr int kKeyIcon1 = 215, kKeyIcon2 = 222, kKeyIcon3 = 229; // element.png, same as DrawInfo
        constexpr int kLifeIcon = 48;  // blupi.png, same as DrawInfo
        constexpr int kPanelIcon = 15; // pad.png, same as DrawInfo
        // The real DrawInfo panel opacity is 0.6 -- deliberately 1.0 here
        // for now: on CNA's Vulkan backend a BasicEffect draw with
        // Alpha < 1 doesn't render at all (verified empirically 2026-07-10:
        // the identical panel quad appears at 1.0 and vanishes at 0.6,
        // while EasyGL shows it at both), and Vulkan is the default
        // backend. Restore 0.6 once that CNA quirk is fixed (tracked in
        // NEXT.md section 5).
        constexpr float kPanelOpacity = 1.0f;

        // blupi.png / element.png share the 60px/10-column tile convention
        // (GEObjectIcons::GetElementIconUv, GEBlupiController's kTilePx/kCols).
        constexpr float kIconTilePx = 60.0f;
        constexpr int kIconCols = 10;

        // text.png: 32px glyph cells, 16 columns; the glyph index IS the
        // ASCII code for the printable range (see GEHud.hpp's class comment
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

        // UVs for a 60px/10-col icon sheet whose real pixel size is known
        // only at runtime (blupi.png's height differs from element.png's).
        void IconUv(int icon, float sheetW, float sheetH,
                    float& u0, float& v0, float& u1, float& v1)
        {
            const int col = icon % kIconCols;
            const int row = icon / kIconCols;
            u0 = (static_cast<float>(col) * kIconTilePx) / sheetW;
            v0 = (static_cast<float>(row) * kIconTilePx) / sheetH;
            u1 = (static_cast<float>(col + 1) * kIconTilePx) / sheetW;
            v1 = (static_cast<float>(row + 1) * kIconTilePx) / sheetH;
        }
    }

    void GEHud::LoadContent(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        using Microsoft::Xna::Framework::Graphics::BasicEffect;
        using Microsoft::Xna::Framework::Graphics::Texture2D;

        const char* kPaths[4] = {
            "Content/icons/blupi.png",
            "Content/icons/element.png",
            "Content/icons/text.png",
            "Content/icons/pad.png",
        };
        for (const char* path : kPaths)
        {
            if (!std::filesystem::exists(path))
            {
                std::printf("GEHud: %s not found -- HUD disabled.\n", path);
                return;
            }
        }

        blupiTexture_ = Texture2D(kPaths[0], device);
        elementTexture_ = Texture2D(kPaths[1], device);
        textTexture_ = Texture2D(kPaths[2], device);
        padTexture_ = Texture2D(kPaths[3], device);

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
        loaded_ = true;
    }

    void GEHud::FlushQuads(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
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

    void GEHud::Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                     int viewportW, int viewportH,
                     int lives, bool key1, bool key2, bool key3,
                     int treasures, int totalTreasures,
                     int animIcon)
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

        const float blupiSheetW = static_cast<float>(blupiTexture_.getWidthProperty());
        const float blupiSheetH = static_cast<float>(blupiTexture_.getHeightProperty());
        const float elementSheetW = static_cast<float>(elementTexture_.getWidthProperty());
        const float elementSheetH = static_cast<float>(elementTexture_.getHeightProperty());

        std::vector<Quad> blupiQuads;
        std::vector<Quad> elementQuads;
        std::vector<Quad> textQuads;
        std::vector<Quad> padQuads;

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

        // Interim animation-state indicator, bottom-right (see GEHud.hpp).
        {
            Quad q;
            q.x1 = static_cast<float>(viewportW) - kAnimIndicatorMargin * scale;
            q.y1 = static_cast<float>(viewportH) - kAnimIndicatorMargin * scale;
            q.x0 = q.x1 - kAnimIndicatorSize * scale;
            q.y0 = q.y1 - kAnimIndicatorSize * scale;
            IconUv(animIcon, blupiSheetW, blupiSheetH, q.u0, q.v0, q.u1, q.v1);
            blupiQuads.push_back(q);
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

        // Treasure counter: pad.png icon-15 panel + centered "N/M" text.
        // Real DrawInfo gates this on being in a real level (mission
        // checks); the CNA equivalent is simply "the world has treasures".
        if (totalTreasures > 0)
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
            const float textSheetW = static_cast<float>(textTexture_.getWidthProperty());
            const float textSheetH = static_cast<float>(textTexture_.getHeightProperty());
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

        // Alpha blending for the icon sheets' real alpha channels; the
        // panel additionally gets the real 0.6 opacity via BasicEffect's
        // Alpha. Panel first so the text draws on top of it.
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
        FlushQuads(device, *padEffect_, padRenderer_, padQuads, viewportW, viewportH, kPanelOpacity);
        FlushQuads(device, *textEffect_, textRenderer_, textQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *blupiEffect_, blupiRenderer_, blupiQuads, viewportW, viewportH, 1.0f);
        FlushQuads(device, *elementEffect_, elementRenderer_, elementQuads, viewportW, viewportH, 1.0f);
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
    }
}
