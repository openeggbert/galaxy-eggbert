#include "GEEditorPaletteRenderer.hpp"

#include "Game/GEQuadBatch.hpp"

#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>

#include <Easy3D/BillboardMeshRenderer.hpp>

#include <array>
#include <cstdio>
#include <filesystem>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        constexpr int kDeleteIcon = 6;
        constexpr int kPlayTestIcon = 48;
        constexpr int kStopIcon = 40;
        constexpr float kButtonTileSize = 40.0f;
        constexpr int kButtonColumns = 6;
        constexpr int kButtonSheetIconOffset = 6;
        constexpr float kGlyphCellPx = 32.0f;
        constexpr int kGlyphColumns = 16;
        constexpr float kGlyphAdvance = 17.0f;
        constexpr float kTextScale = 0.5f;
        constexpr float kSelectionPadding = 3.0f;
        constexpr int kSkyRegionCount = 32;

        Microsoft::Xna::Framework::Vector3 ButtonGreen()
        {
            return {0.20f, 0.62f, 0.22f};
        }

        Microsoft::Xna::Framework::Vector3 ButtonGreenActive()
        {
            return {0.45f, 0.90f, 0.35f};
        }

        Microsoft::Xna::Framework::Vector3 SelectionGold()
        {
            return {0.95f, 0.80f, 0.20f};
        }

        float LabelWidth(const char* message)
        {
            int length = 0;
            while (message[length] != '\0')
            {
                ++length;
            }
            return static_cast<float>(length) * kGlyphAdvance * kTextScale;
        }

        void AppendLabel(
            std::vector<GEQuadBatch::Quad>& quads, const char* message,
            float left, float top, float sheetWidth, float sheetHeight)
        {
            float penX = left;
            for (const char* c = message; *c != '\0'; ++c)
            {
                const int rank = static_cast<int>(static_cast<unsigned char>(*c));
                const int column = rank % kGlyphColumns;
                const int row = rank / kGlyphColumns;
                const float glyphSize = kGlyphCellPx * kTextScale;
                quads.push_back({
                    penX, top, penX + glyphSize, top + glyphSize,
                    static_cast<float>(column) * kGlyphCellPx / sheetWidth,
                    static_cast<float>(row) * kGlyphCellPx / sheetHeight,
                    static_cast<float>(column + 1) * kGlyphCellPx / sheetWidth,
                    static_cast<float>(row + 1) * kGlyphCellPx / sheetHeight,
                });
                penX += kGlyphAdvance * kTextScale;
            }
        }
    }

    struct GEEditorPaletteRenderer::Impl
    {
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> flatEffect;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> flatRenderer;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> buttonTexture;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> buttonEffect;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> buttonRenderer;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> textTexture;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> textEffect;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> textRenderer;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> skyEffect;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> skyRenderer;
        std::array<std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D>,
                   kSkyRegionCount> skyTextures;
        std::array<bool, kSkyRegionCount> skyTextureAttempted{};
    };

    GEEditorPaletteRenderer::GEEditorPaletteRenderer()
        : impl_(std::make_unique<Impl>())
    {
    }

    GEEditorPaletteRenderer::~GEEditorPaletteRenderer() = default;

    void GEEditorPaletteRenderer::Draw(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
        const GEEditorPaletteLayout& layout, const State& state,
        int viewportWidth, int viewportHeight)
    {
        using Microsoft::Xna::Framework::Graphics::BasicEffect;
        using Microsoft::Xna::Framework::Graphics::BlendState;
        using Microsoft::Xna::Framework::Graphics::Texture2D;

        if (!impl_->flatEffect)
        {
            impl_->flatEffect = std::make_unique<BasicEffect>(device);
            impl_->flatEffect->VertexColorEnabled = false;
            impl_->flatEffect->setTextureEnabledProperty(false);
        }
        if (!impl_->buttonTexture)
        {
            impl_->buttonTexture = std::make_unique<Texture2D>("Content/icons/button.png", device);
            impl_->buttonEffect = std::make_unique<BasicEffect>(device);
            impl_->buttonEffect->VertexColorEnabled = false;
            impl_->buttonEffect->setTextureEnabledProperty(true);
        }
        impl_->buttonEffect->setTextureProperty(impl_->buttonTexture.get());
        if (!impl_->textTexture && std::filesystem::exists("Content/icons/text.png"))
        {
            impl_->textTexture = std::make_unique<Texture2D>("Content/icons/text.png", device);
            impl_->textEffect = std::make_unique<BasicEffect>(device);
            impl_->textEffect->VertexColorEnabled = false;
            impl_->textEffect->setTextureEnabledProperty(true);
        }
        if (impl_->textEffect)
        {
            impl_->textEffect->setTextureProperty(impl_->textTexture.get());
        }

        const bool overview = state.openCategory < 0;
        const int itemCount = static_cast<int>(state.contentButtonIconIds.size());
        device.setBlendStateProperty(BlendState::NonPremultiplied);

        std::vector<GEQuadBatch::Quad> green;
        std::vector<GEQuadBatch::Quad> active;
        std::vector<GEQuadBatch::Quad> gold;
        const auto appendSolid = [](std::vector<GEQuadBatch::Quad>& target, const GEQuadBatch::Rect& rect)
        {
            target.push_back({rect.x0, rect.y0, rect.x1, rect.y1, 0, 0, 1, 1});
        };

        appendSolid(green, layout.DeleteToolRect());
        appendSolid(state.stopConfirmArmed ? active : green, layout.StopRect(viewportWidth, viewportHeight));
        appendSolid(green, layout.PlayTestRect(viewportWidth, viewportHeight));
        for (int i = 0; i < static_cast<int>(state.categories.size()); ++i)
        {
            appendSolid(i == state.openCategory ? active : green, layout.CategoryButtonRect(i));
        }
        if (overview)
        {
            for (int i = 0; i < GEEditorPaletteLayout::PlacementPlaceIndex; ++i)
            {
                appendSolid(green, layout.PlacementButtonRect(i, viewportWidth, viewportHeight));
            }
            appendSolid(gold, layout.PlacementButtonRect(
                GEEditorPaletteLayout::PlacementPlaceIndex, viewportWidth, viewportHeight));
        }
        else
        {
            for (int i = 0; i < itemCount; ++i)
            {
                const GEQuadBatch::Rect cell = layout.PaletteCellRect(
                    i, itemCount, state.openCategory, viewportWidth, viewportHeight);
                appendSolid(green, cell);
                const int objectType = i < static_cast<int>(state.contentObjectTypeIds.size()) ?
                    state.contentObjectTypeIds[static_cast<std::size_t>(i)] : 0;
                const int blockType = i < static_cast<int>(state.contentBlockIds.size()) ?
                    state.contentBlockIds[static_cast<std::size_t>(i)] : 0;
                const int skyRegion = i < static_cast<int>(state.contentSkyRegionIds.size()) ?
                    state.contentSkyRegionIds[static_cast<std::size_t>(i)] : -1;
                const int spawnPoint = i < static_cast<int>(state.contentSpawnPointIds.size()) ?
                    state.contentSpawnPointIds[static_cast<std::size_t>(i)] : 0;
                const int bigDecorIcon = i < static_cast<int>(state.contentBigDecorIconIds.size()) ?
                    state.contentBigDecorIconIds[static_cast<std::size_t>(i)] : 0;
                const bool selected =
                    (skyRegion >= 0 && skyRegion == state.selectedSkyRegion) ||
                    (skyRegion < 0 && spawnPoint > 0 && state.spawnPointMode) ||
                    (skyRegion < 0 && bigDecorIcon > 0 && state.bigDecorMode &&
                     bigDecorIcon == state.selectedBigDecorIcon) ||
                    (skyRegion < 0 && spawnPoint == 0 && bigDecorIcon == 0 &&
                     objectType > 0 && state.objectMode &&
                     objectType == state.selectedObjectType) ||
                    (skyRegion < 0 && spawnPoint == 0 && bigDecorIcon == 0 &&
                     objectType == 0 && blockType > 0 && !state.objectMode &&
                     !state.spawnPointMode && !state.bigDecorMode &&
                     blockType == state.selectedBlockType);
                if (selected)
                {
                    gold.push_back({
                        cell.x0 - kSelectionPadding, cell.y0 - kSelectionPadding,
                        cell.x1 + kSelectionPadding, cell.y1 + kSelectionPadding,
                        0, 0, 1, 1,
                    });
                }
            }
        }

        impl_->flatEffect->setDiffuseColorProperty(ButtonGreen());
        GEQuadBatch::FlushQuads(
            device, *impl_->flatEffect, impl_->flatRenderer, green,
            viewportWidth, viewportHeight, 1.0f);
        impl_->flatEffect->setDiffuseColorProperty(ButtonGreenActive());
        GEQuadBatch::FlushQuads(
            device, *impl_->flatEffect, impl_->flatRenderer, active,
            viewportWidth, viewportHeight, 1.0f);
        impl_->flatEffect->setDiffuseColorProperty(SelectionGold());
        GEQuadBatch::FlushQuads(
            device, *impl_->flatEffect, impl_->flatRenderer, gold,
            viewportWidth, viewportHeight, 1.0f);

        const auto appendButtonIcon = [&](std::vector<GEQuadBatch::Quad>& quads, int icon,
                                          const GEQuadBatch::Rect& rect)
        {
            const float sheetWidth = static_cast<float>(impl_->buttonTexture->getWidthProperty());
            const float sheetHeight = static_cast<float>(impl_->buttonTexture->getHeightProperty());
            const int sheetIcon = icon + kButtonSheetIconOffset;
            const int column = sheetIcon % kButtonColumns;
            const int row = sheetIcon / kButtonColumns;
            quads.push_back({
                rect.x0, rect.y0, rect.x1, rect.y1,
                static_cast<float>(column) * kButtonTileSize / sheetWidth,
                static_cast<float>(row) * kButtonTileSize / sheetHeight,
                static_cast<float>(column + 1) * kButtonTileSize / sheetWidth,
                static_cast<float>(row + 1) * kButtonTileSize / sheetHeight,
            });
        };

        std::vector<GEQuadBatch::Quad> buttonQuads;
        appendButtonIcon(buttonQuads, kDeleteIcon, layout.DeleteToolRect());
        appendButtonIcon(buttonQuads, kPlayTestIcon, layout.PlayTestRect(viewportWidth, viewportHeight));
        appendButtonIcon(buttonQuads, kStopIcon, layout.StopRect(viewportWidth, viewportHeight));
        for (int i = 0; i < static_cast<int>(state.categories.size()); ++i)
        {
            appendButtonIcon(
                buttonQuads, state.categories[static_cast<std::size_t>(i)].buttonIconId,
                layout.CategoryButtonRect(i));
        }
        if (!overview)
        {
            for (int i = 0; i < itemCount; ++i)
            {
                if (i < static_cast<int>(state.contentSkyRegionIds.size()))
                {
                    continue;
                }
                appendButtonIcon(
                    buttonQuads, state.contentButtonIconIds[static_cast<std::size_t>(i)],
                    layout.PaletteCellRect(
                        i, itemCount, state.openCategory, viewportWidth, viewportHeight));
            }
        }
        GEQuadBatch::FlushQuads(
            device, *impl_->buttonEffect, impl_->buttonRenderer, buttonQuads,
            viewportWidth, viewportHeight, 1.0f);

        if (!state.contentSkyRegionIds.empty())
        {
            if (!impl_->skyEffect)
            {
                impl_->skyEffect = std::make_unique<BasicEffect>(device);
                impl_->skyEffect->VertexColorEnabled = false;
                impl_->skyEffect->setTextureEnabledProperty(true);
            }

            std::vector<GEQuadBatch::Quad> missingSkyRegions;
            for (int i = 0; i < static_cast<int>(state.contentSkyRegionIds.size()); ++i)
            {
                const int region = state.contentSkyRegionIds[static_cast<std::size_t>(i)];
                if (region < 0 || region >= kSkyRegionCount)
                {
                    continue;
                }
                const std::size_t regionIndex = static_cast<std::size_t>(region);
                if (!impl_->skyTextureAttempted[regionIndex])
                {
                    impl_->skyTextureAttempted[regionIndex] = true;
                    char backgroundPath[64];
                    std::snprintf(
                        backgroundPath, sizeof(backgroundPath),
                        "Content/backgrounds/decor%03d.png", region);
                    if (std::filesystem::exists(backgroundPath))
                    {
                        impl_->skyTextures[regionIndex] =
                            std::make_unique<Texture2D>(backgroundPath, device);
                    }
                }

                const GEQuadBatch::Rect cell = layout.PaletteCellRect(
                    i, itemCount, state.openCategory, viewportWidth, viewportHeight);
                if (impl_->skyTextures[regionIndex])
                {
                    // The source image is 4:3. Crop its horizontal edges
                    // instead of squeezing it into the square menu cell.
                    const std::vector<GEQuadBatch::Quad> thumbnail = {
                        {cell.x0, cell.y0, cell.x1, cell.y1,
                         0.125f, 0.0f, 0.875f, 1.0f},
                    };
                    impl_->skyEffect->setTextureProperty(
                        impl_->skyTextures[regionIndex].get());
                    GEQuadBatch::FlushQuads(
                        device, *impl_->skyEffect, impl_->skyRenderer, thumbnail,
                        viewportWidth, viewportHeight, 1.0f);
                }
                else
                {
                    appendSolid(missingSkyRegions, cell);
                }
            }
            impl_->flatEffect->setDiffuseColorProperty(
                {100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f});
            GEQuadBatch::FlushQuads(
                device, *impl_->flatEffect, impl_->flatRenderer, missingSkyRegions,
                viewportWidth, viewportHeight, 1.0f);

            if (impl_->textTexture && impl_->textEffect)
            {
                const float sheetWidth =
                    static_cast<float>(impl_->textTexture->getWidthProperty());
                const float sheetHeight =
                    static_cast<float>(impl_->textTexture->getHeightProperty());
                std::vector<GEQuadBatch::Quad> numberBackgrounds;
                std::vector<GEQuadBatch::Quad> numberLabels;
                for (int i = 0; i < static_cast<int>(state.contentSkyRegionIds.size()); ++i)
                {
                    const int region = state.contentSkyRegionIds[static_cast<std::size_t>(i)];
                    char label[4];
                    std::snprintf(label, sizeof(label), "%d", region);
                    const GEQuadBatch::Rect cell = layout.PaletteCellRect(
                        i, itemCount, state.openCategory, viewportWidth, viewportHeight);
                    const float labelWidth = LabelWidth(label);
                    numberBackgrounds.push_back({
                        cell.x0 + 2.0f, cell.y0 + 2.0f,
                        cell.x0 + labelWidth + 8.0f,
                        cell.y0 + kGlyphCellPx * kTextScale + 6.0f,
                        0, 0, 1, 1,
                    });
                    AppendLabel(
                        numberLabels, label, cell.x0 + 5.0f, cell.y0 + 5.0f,
                        sheetWidth, sheetHeight);
                }
                impl_->flatEffect->setDiffuseColorProperty({1.0f, 1.0f, 1.0f});
                GEQuadBatch::FlushQuads(
                    device, *impl_->flatEffect, impl_->flatRenderer, numberBackgrounds,
                    viewportWidth, viewportHeight, 0.78f);
                impl_->textEffect->setDiffuseColorProperty({0.0f, 0.0f, 0.0f});
                GEQuadBatch::FlushQuads(
                    device, *impl_->textEffect, impl_->textRenderer, numberLabels,
                    viewportWidth, viewportHeight, 1.0f);
            }
        }

        if (state.hasPlacementPreview)
        {
            const float cx = static_cast<float>(viewportWidth) * 0.5f;
            const float cy = static_cast<float>(viewportHeight) * 0.5f;
            const std::vector<GEQuadBatch::Quad> reticle = {
                {cx - 10, cy - 1, cx - 3, cy + 1, 0, 0, 1, 1},
                {cx + 3, cy - 1, cx + 10, cy + 1, 0, 0, 1, 1},
                {cx - 1, cy - 10, cx + 1, cy - 3, 0, 0, 1, 1},
                {cx - 1, cy + 3, cx + 1, cy + 10, 0, 0, 1, 1},
            };
            impl_->flatEffect->setDiffuseColorProperty({1.0f, 0.05f, 0.05f});
            GEQuadBatch::FlushQuads(
                device, *impl_->flatEffect, impl_->flatRenderer, reticle,
                viewportWidth, viewportHeight, 1.0f);
        }

        if (overview && impl_->textTexture && impl_->textEffect)
        {
            constexpr const char* kLabels[GEEditorPaletteLayout::PlacementButtonCount] = {
                "X-", "X+", "Y-", "Y+", "Z-", "Z+", "PLACE",
            };
            const float sheetWidth = static_cast<float>(impl_->textTexture->getWidthProperty());
            const float sheetHeight = static_cast<float>(impl_->textTexture->getHeightProperty());
            std::vector<GEQuadBatch::Quad> axisLabels;
            for (int i = 0; i < GEEditorPaletteLayout::PlacementPlaceIndex; ++i)
            {
                const GEQuadBatch::Rect button =
                    layout.PlacementButtonRect(i, viewportWidth, viewportHeight);
                AppendLabel(
                    axisLabels, kLabels[i],
                    button.x0 + ((button.x1 - button.x0) - LabelWidth(kLabels[i])) * 0.5f,
                    button.y0 + ((button.y1 - button.y0) - kGlyphCellPx * kTextScale) * 0.5f,
                    sheetWidth, sheetHeight);
            }
            impl_->textEffect->setDiffuseColorProperty({1.0f, 1.0f, 1.0f});
            GEQuadBatch::FlushQuads(
                device, *impl_->textEffect, impl_->textRenderer, axisLabels,
                viewportWidth, viewportHeight, 1.0f);

            const GEQuadBatch::Rect place = layout.PlacementButtonRect(
                GEEditorPaletteLayout::PlacementPlaceIndex, viewportWidth, viewportHeight);
            std::vector<GEQuadBatch::Quad> placeLabel;
            AppendLabel(
                placeLabel, kLabels[GEEditorPaletteLayout::PlacementPlaceIndex],
                place.x0 + ((place.x1 - place.x0) -
                            LabelWidth(kLabels[GEEditorPaletteLayout::PlacementPlaceIndex])) * 0.5f,
                place.y0 + ((place.y1 - place.y0) - kGlyphCellPx * kTextScale) * 0.5f,
                sheetWidth, sheetHeight);
            impl_->textEffect->setDiffuseColorProperty({0.12f, 0.08f, 0.02f});
            GEQuadBatch::FlushQuads(
                device, *impl_->textEffect, impl_->textRenderer, placeLabel,
                viewportWidth, viewportHeight, 1.0f);

            if (state.hasPlacementPreview)
            {
                char coordinates[48];
                std::snprintf(
                    coordinates, sizeof(coordinates), "X:%d Y:%d Z:%d",
                    state.placementX, state.placementY, state.placementZ);
                const GEQuadBatch::Rect coordinateRect =
                    layout.PlacementCoordinatesRect(
                        LabelWidth(coordinates), kGlyphCellPx * kTextScale,
                        viewportWidth, viewportHeight);
                const GEQuadBatch::Rect coordinateBackgroundRect =
                    layout.PlacementCoordinatesBackgroundRect(
                        coordinateRect, viewportWidth, viewportHeight);
                const std::vector<GEQuadBatch::Quad> coordinateBackground = {
                    {
                        coordinateBackgroundRect.x0, coordinateBackgroundRect.y0,
                        coordinateBackgroundRect.x1, coordinateBackgroundRect.y1,
                        0, 0, 1, 1,
                    },
                };
                impl_->flatEffect->setDiffuseColorProperty({1.0f, 1.0f, 1.0f});
                GEQuadBatch::FlushQuads(
                    device, *impl_->flatEffect, impl_->flatRenderer,
                    coordinateBackground, viewportWidth, viewportHeight, 0.72f);
                std::vector<GEQuadBatch::Quad> coordinateLabel;
                AppendLabel(
                    coordinateLabel, coordinates, coordinateRect.x0, coordinateRect.y0,
                    sheetWidth, sheetHeight);
                impl_->textEffect->setDiffuseColorProperty({0.0f, 0.0f, 0.0f});
                GEQuadBatch::FlushQuads(
                    device, *impl_->textEffect, impl_->textRenderer, coordinateLabel,
                    viewportWidth, viewportHeight, 1.0f);
            }
        }

        if (state.noticeVisible && impl_->textTexture && impl_->textEffect)
        {
            constexpr const char* kMessage = "Not yet implemented.";
            const float messageWidth = LabelWidth(kMessage);
            const float left = (static_cast<float>(viewportWidth) - messageWidth) * 0.5f;
            const float top = static_cast<float>(viewportHeight) * 0.5f - 8.0f;
            const std::vector<GEQuadBatch::Quad> background = {
                {left - 8, top - 6, left + messageWidth + 8,
                 top + kGlyphCellPx * kTextScale + 6, 0, 0, 1, 1},
            };
            impl_->flatEffect->setDiffuseColorProperty({1.0f, 0.90f, 0.05f});
            GEQuadBatch::FlushQuads(
                device, *impl_->flatEffect, impl_->flatRenderer, background,
                viewportWidth, viewportHeight, 1.0f);
            std::vector<GEQuadBatch::Quad> notice;
            AppendLabel(
                notice, kMessage, left, top,
                static_cast<float>(impl_->textTexture->getWidthProperty()),
                static_cast<float>(impl_->textTexture->getHeightProperty()));
            impl_->textEffect->setDiffuseColorProperty({1.0f, 0.05f, 0.05f});
            GEQuadBatch::FlushQuads(
                device, *impl_->textEffect, impl_->textRenderer, notice,
                viewportWidth, viewportHeight, 1.0f);
        }

        device.setBlendStateProperty(BlendState::Opaque);
    }
}
