#include "GEEditorPalette.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>

#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>

#include <algorithm>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        constexpr float kToolbarButtonSize = 48.0f;
        constexpr float kToolbarGap = 8.0f;
        constexpr float kToolbarX = 10.0f;
        constexpr float kToolbarY0 = 10.0f;

        constexpr float kPaletteIconSize = 40.0f;
        constexpr float kPaletteIconGap = 4.0f;
        constexpr int kPaletteCols = 8;
        constexpr int kPaletteRows = 4;
        constexpr int kIconsPerPage = kPaletteCols * kPaletteRows;
        constexpr float kGridMargin = 10.0f;

        constexpr float kTabToggleWidth = 80.0f;
        constexpr float kTabToggleHeight = 24.0f;
        constexpr float kPageButtonSize = 24.0f;
        constexpr float kControlGap = 4.0f;

        constexpr float kSelectionHighlightPadding = 4.0f;

        float GridWidth() { return kPaletteCols * (kPaletteIconSize + kPaletteIconGap) - kPaletteIconGap; }
        float GridHeight() { return kPaletteRows * (kPaletteIconSize + kPaletteIconGap) - kPaletteIconGap; }
    }

    GEEditorPalette::GEEditorPalette()
        : confirmedFlat_(), allIcons_(AllBlockIconIdsInOrder()),
          confirmedObjectsFlat_(), allObjects_(AllObjectTypeIdsInOrder()),
          selectedBlockType_(static_cast<int>(GalaxyEggbert::BlockTypes::RockPile)),
          selectedObjectType_(static_cast<int>(ObjectType::ObjectType6))
    {
        for (const auto& category : ConfirmedBlockCategories())
        {
            confirmedFlat_.insert(confirmedFlat_.end(), category.iconIds.begin(), category.iconIds.end());
        }

        for (const auto& category : ConfirmedObjectCategories())
        {
            confirmedObjectsFlat_.insert(confirmedObjectsFlat_.end(), category.iconIds.begin(),
                                         category.iconIds.end());
        }
    }

    int GEEditorPalette::PageCount() const noexcept
    {
        const int count = static_cast<int>(CurrentIconList().size());
        return std::max(1, (count + kIconsPerPage - 1) / kIconsPerPage);
    }

    GEQuadBatch::Rect GEEditorPalette::ToolbarButtonRect(int index) const noexcept
    {
        const float y0 = kToolbarY0 + static_cast<float>(index) * (kToolbarButtonSize + kToolbarGap);
        return {kToolbarX, y0, kToolbarX + kToolbarButtonSize, y0 + kToolbarButtonSize};
    }

    GEQuadBatch::Rect GEEditorPalette::TabToggleRect(int viewportWidth, int viewportHeight) const noexcept
    {
        const float gridX0 = static_cast<float>(viewportWidth) - GridWidth() - kGridMargin;
        const float gridY0 = static_cast<float>(viewportHeight) - GridHeight() - kGridMargin;
        const float y0 = gridY0 - kTabToggleHeight - kControlGap;
        return {gridX0, y0, gridX0 + kTabToggleWidth, y0 + kTabToggleHeight};
    }

    GEQuadBatch::Rect GEEditorPalette::PagePrevRect(int viewportWidth, int viewportHeight) const noexcept
    {
        const GEQuadBatch::Rect tab = TabToggleRect(viewportWidth, viewportHeight);
        return {tab.x1 + kControlGap, tab.y0, tab.x1 + kControlGap + kPageButtonSize, tab.y0 + kPageButtonSize};
    }

    GEQuadBatch::Rect GEEditorPalette::PageNextRect(int viewportWidth, int viewportHeight) const noexcept
    {
        const GEQuadBatch::Rect prev = PagePrevRect(viewportWidth, viewportHeight);
        return {prev.x1 + kControlGap, prev.y0, prev.x1 + kControlGap + kPageButtonSize, prev.y0 + kPageButtonSize};
    }

    GEQuadBatch::Rect GEEditorPalette::PaletteCellRect(int indexOnPage, int viewportWidth,
                                                       int viewportHeight) const noexcept
    {
        const int col = indexOnPage % kPaletteCols;
        const int row = indexOnPage / kPaletteCols;
        const float gridX0 = static_cast<float>(viewportWidth) - GridWidth() - kGridMargin;
        const float gridY0 = static_cast<float>(viewportHeight) - GridHeight() - kGridMargin;
        const float x0 = gridX0 + static_cast<float>(col) * (kPaletteIconSize + kPaletteIconGap);
        const float y0 = gridY0 + static_cast<float>(row) * (kPaletteIconSize + kPaletteIconGap);
        return {x0, y0, x0 + kPaletteIconSize, y0 + kPaletteIconSize};
    }

    bool GEEditorPalette::HitsAnyControl(float x, float y, int viewportWidth,
                                          int viewportHeight) const noexcept
    {
        for (int i = 0; i <= 5; ++i)
        {
            if (GEQuadBatch::InRect(x, y, ToolbarButtonRect(i)))
            {
                return true;
            }
        }
        if (GEQuadBatch::InRect(x, y, TabToggleRect(viewportWidth, viewportHeight)))
        {
            return true;
        }
        if (PageCount() > 1 &&
            (GEQuadBatch::InRect(x, y, PagePrevRect(viewportWidth, viewportHeight)) ||
             GEQuadBatch::InRect(x, y, PageNextRect(viewportWidth, viewportHeight))))
        {
            return true;
        }
        const auto& icons = CurrentIconList();
        const int startIdx = std::clamp(page_, 0, PageCount() - 1) * kIconsPerPage;
        for (int i = 0; i < kIconsPerPage; ++i)
        {
            if (startIdx + i >= static_cast<int>(icons.size()))
            {
                break;
            }
            if (GEQuadBatch::InRect(x, y, PaletteCellRect(i, viewportWidth, viewportHeight)))
            {
                return true;
            }
        }
        return false;
    }

    GEEditorPalette::UpdateResult GEEditorPalette::Update(
        const Microsoft::Xna::Framework::Input::MouseState& mouse, int viewportWidth, int viewportHeight)
    {
        using ButtonState = Microsoft::Xna::Framework::Input::ButtonState;
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;
        const float mx = static_cast<float>(mouse.getXProperty());
        const float my = static_cast<float>(mouse.getYProperty());

        UpdateResult result;
        // Claim the mouse from the PRESS frame onward, not just on the
        // release that fires the action: GEWorldEditor's own world-editing
        // clicks are edge-triggered on press, so a palette click that only
        // reported clickConsumed on release would still place/remove a
        // block at the crosshair behind the palette on the way down.
        if (mouseDown && !mouseWasDown_)
        {
            pressStartedOnPalette_ = HitsAnyControl(mx, my, viewportWidth, viewportHeight);
        }
        if (mouseDown && pressStartedOnPalette_)
        {
            result.clickConsumed = true;
        }

        if (!mouseDown && mouseWasDown_)
        {
            const bool releaseIsOnPalette = pressStartedOnPalette_;
            pressStartedOnPalette_ = false;
            if (!releaseIsOnPalette)
            {
                // The press that this release completes began out in the 3D
                // view -- not this class's click to interpret, even if the
                // cursor has since wandered over the palette.
                mouseWasDown_ = mouseDown;
                return result;
            }

            const int pageCount = PageCount();
            page_ = std::clamp(page_, 0, pageCount - 1);

            if (GEQuadBatch::InRect(mx, my, ToolbarButtonRect(0)))
            {
                result.action = ToolbarAction::Undo;
                result.clickConsumed = true;
            }
            else if (GEQuadBatch::InRect(mx, my, ToolbarButtonRect(1)))
            {
                result.action = ToolbarAction::Redo;
                result.clickConsumed = true;
            }
            else if (GEQuadBatch::InRect(mx, my, ToolbarButtonRect(2)))
            {
                result.action = ToolbarAction::Save;
                result.clickConsumed = true;
            }
            else if (GEQuadBatch::InRect(mx, my, ToolbarButtonRect(3)))
            {
                result.action = ToolbarAction::Back;
                result.clickConsumed = true;
            }
            else if (GEQuadBatch::InRect(mx, my, ToolbarButtonRect(4)))
            {
                result.action = ToolbarAction::PlayTest;
                result.clickConsumed = true;
            }
            else if (GEQuadBatch::InRect(mx, my, ToolbarButtonRect(5)))
            {
                mode_ = mode_ == PaletteMode::Blocks ? PaletteMode::Objects : PaletteMode::Blocks;
                page_ = 0;
                result.clickConsumed = true;
            }
            else if (GEQuadBatch::InRect(mx, my, TabToggleRect(viewportWidth, viewportHeight)))
            {
                showAllTab_ = !showAllTab_;
                page_ = 0;
                result.clickConsumed = true;
            }
            else if (pageCount > 1 && GEQuadBatch::InRect(mx, my, PagePrevRect(viewportWidth, viewportHeight)))
            {
                page_ = (page_ - 1 + pageCount) % pageCount;
                result.clickConsumed = true;
            }
            else if (pageCount > 1 && GEQuadBatch::InRect(mx, my, PageNextRect(viewportWidth, viewportHeight)))
            {
                page_ = (page_ + 1) % pageCount;
                result.clickConsumed = true;
            }
            else
            {
                const auto& icons = CurrentIconList();
                const int startIdx = page_ * kIconsPerPage;
                for (int i = 0; i < kIconsPerPage; ++i)
                {
                    const int idx = startIdx + i;
                    if (idx >= static_cast<int>(icons.size()))
                    {
                        break;
                    }
                    if (GEQuadBatch::InRect(mx, my, PaletteCellRect(i, viewportWidth, viewportHeight)))
                    {
                        (mode_ == PaletteMode::Objects ? selectedObjectType_ : selectedBlockType_) =
                            icons[static_cast<std::size_t>(idx)];
                        result.clickConsumed = true;
                        break;
                    }
                }
            }
        }
        mouseWasDown_ = mouseDown;
        return result;
    }

    void GEEditorPalette::Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                               Microsoft::Xna::Framework::Graphics::Texture2D& terrainTexture,
                               Microsoft::Xna::Framework::Graphics::Texture2D& elementTexture,
                               Microsoft::Xna::Framework::Graphics::Texture2D& exploTexture,
                               Microsoft::Xna::Framework::Graphics::Texture2D& blupiTexture,
                               Microsoft::Xna::Framework::Graphics::Texture2D& blupi1Texture,
                               int viewportWidth, int viewportHeight)
    {
        using Microsoft::Xna::Framework::Graphics::BasicEffect;
        using Microsoft::Xna::Framework::Graphics::BlendState;
        using Microsoft::Xna::Framework::Graphics::Texture2D;

        const auto ensureTexturedEffect = [&device](std::unique_ptr<BasicEffect>& effect, Texture2D& texture)
        {
            if (!effect)
            {
                effect = std::make_unique<BasicEffect>(device);
                effect->VertexColorEnabled = false;
                effect->setTextureEnabledProperty(true);
            }
            effect->setTextureProperty(&texture);
        };
        ensureTexturedEffect(paletteEffect_, terrainTexture);
        ensureTexturedEffect(elementEffect_, elementTexture);
        ensureTexturedEffect(exploEffect_, exploTexture);
        ensureTexturedEffect(blupiEffect_, blupiTexture);
        ensureTexturedEffect(blupi1Effect_, blupi1Texture);
        if (!flatEffect_)
        {
            flatEffect_ = std::make_unique<BasicEffect>(device);
            flatEffect_->VertexColorEnabled = false;
            flatEffect_->setTextureEnabledProperty(false);
            flatEffect_->setDiffuseColorProperty(Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f));
        }

        const int pageCount = PageCount();
        const int clampedPage = std::clamp(page_, 0, pageCount - 1);
        const auto& icons = CurrentIconList();
        const int startIdx = clampedPage * kIconsPerPage;

        device.setBlendStateProperty(BlendState::NonPremultiplied);

        // Flat-colored UI drawn FIRST: toolbar buttons, tab toggle, paging
        // buttons, a translucent backing behind every Objects-mode cell
        // (real object sprites can have transparent margins, so a plain
        // backing keeps them legible against the 3D scene behind -- same
        // technique/color as the toolbar buttons above), and a
        // slightly-larger backing square behind the selected icon (added
        // after that cell's own backing, so it reads as a highlighted
        // border once the icon renders on top).
        std::vector<GEQuadBatch::Quad> flatQuads;
        const auto addFlat = [&flatQuads](const GEQuadBatch::Rect& r)
        {
            flatQuads.push_back({r.x0, r.y0, r.x1, r.y1, 0.0f, 0.0f, 1.0f, 1.0f});
        };
        addFlat(ToolbarButtonRect(0));
        addFlat(ToolbarButtonRect(1));
        addFlat(ToolbarButtonRect(2));
        addFlat(ToolbarButtonRect(3));
        addFlat(ToolbarButtonRect(4));
        addFlat(TabToggleRect(viewportWidth, viewportHeight));
        const int selectedId = mode_ == PaletteMode::Objects ? selectedObjectType_ : selectedBlockType_;
        if (pageCount > 1)
        {
            addFlat(PagePrevRect(viewportWidth, viewportHeight));
            addFlat(PageNextRect(viewportWidth, viewportHeight));
        }
        for (int i = 0; i < kIconsPerPage; ++i)
        {
            const int idx = startIdx + i;
            if (idx >= static_cast<int>(icons.size()))
            {
                break;
            }
            const GEQuadBatch::Rect cell = PaletteCellRect(i, viewportWidth, viewportHeight);
            if (mode_ == PaletteMode::Objects)
            {
                addFlat(cell);
            }
            if (icons[static_cast<std::size_t>(idx)] == selectedId)
            {
                addFlat({cell.x0 - kSelectionHighlightPadding, cell.y0 - kSelectionHighlightPadding,
                        cell.x1 + kSelectionHighlightPadding, cell.y1 + kSelectionHighlightPadding});
            }
        }
        flatEffect_->setDiffuseColorProperty(Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f));
        GEQuadBatch::FlushQuads(device, *flatEffect_, flatRenderer_, flatQuads, viewportWidth, viewportHeight, 0.5f);

        // Mode-toggle button, drawn in its own flush so it can carry the
        // current mode's own color -- the only cue distinguishing the two
        // modes at a glance, since no toolbar button has a text label.
        flatEffect_->setDiffuseColorProperty(mode_ == PaletteMode::Objects
                                                 ? Microsoft::Xna::Framework::Vector3(1.0f, 0.6f, 0.2f)
                                                 : Microsoft::Xna::Framework::Vector3(0.4f, 0.6f, 0.9f));
        const GEQuadBatch::Rect modeRect = ToolbarButtonRect(5);
        const std::vector<GEQuadBatch::Quad> modeQuad{
            {modeRect.x0, modeRect.y0, modeRect.x1, modeRect.y1, 0.0f, 0.0f, 1.0f, 1.0f}};
        GEQuadBatch::FlushQuads(device, *flatEffect_, flatRenderer_, modeQuad, viewportWidth, viewportHeight, 0.85f);

        if (mode_ == PaletteMode::Objects)
        {
            // Real per-type icons (2026-07-19): GEObjectIcons::GetObjIcon()
            // plus its atlas-selection predicates say which of the 5 real
            // sprite sheets each type's icon actually lives on -- the SAME
            // lookup already used to render real MoveObject billboards
            // during gameplay. Batched per atlas so each needs only one
            // draw call per page, same shape as the single-atlas
            // Blocks-mode batch below. Always phase 0 -- a static preview,
            // no per-cell animation.
            std::vector<GEQuadBatch::Quad> terrainQuads, elementQuads, exploQuads, blupiQuads, blupi1Quads;
            for (int i = 0; i < kIconsPerPage; ++i)
            {
                const int idx = startIdx + i;
                if (idx >= static_cast<int>(icons.size()))
                {
                    break;
                }
                const ObjectType type = ToObjectType(icons[static_cast<std::size_t>(idx)]);
                const int icon = GetObjIcon(type, /*phase=*/0);
                const GEQuadBatch::Rect cell = PaletteCellRect(i, viewportWidth, viewportHeight);

                if (IsUniformCubeObject(type) || IsObjectMPngSourced(type))
                {
                    const Easy3D::UvRect uv = tileAtlas_.GetTileUv(icon);
                    terrainQuads.push_back({cell.x0, cell.y0, cell.x1, cell.y1, uv.U0, uv.V0, uv.U1, uv.V1});
                }
                else if (IsExploPngSourced(type))
                {
                    const ObjectIconUv uv = GetExploIconUv(icon);
                    exploQuads.push_back({cell.x0, cell.y0, cell.x1, cell.y1, uv.U0, uv.V0, uv.U1, uv.V1});
                }
                else if (IsBlupiPngSourcedAtPhase(type, 0))
                {
                    const ObjectIconUv uv = GetBlupiIconUv(icon);
                    std::vector<GEQuadBatch::Quad>& target = UsesBlupi1Texture(type) ? blupi1Quads : blupiQuads;
                    target.push_back({cell.x0, cell.y0, cell.x1, cell.y1, uv.U0, uv.V0, uv.U1, uv.V1});
                }
                else
                {
                    const ObjectIconUv uv = GetElementIconUv(icon);
                    elementQuads.push_back({cell.x0, cell.y0, cell.x1, cell.y1, uv.U0, uv.V0, uv.U1, uv.V1});
                }
            }
            if (!terrainQuads.empty())
            {
                GEQuadBatch::FlushQuads(device, *paletteEffect_, paletteRenderer_, terrainQuads, viewportWidth,
                                        viewportHeight, 1.0f);
            }
            if (!elementQuads.empty())
            {
                GEQuadBatch::FlushQuads(device, *elementEffect_, elementRenderer_, elementQuads, viewportWidth,
                                        viewportHeight, 1.0f);
            }
            if (!exploQuads.empty())
            {
                GEQuadBatch::FlushQuads(device, *exploEffect_, exploRenderer_, exploQuads, viewportWidth,
                                        viewportHeight, 1.0f);
            }
            if (!blupiQuads.empty())
            {
                GEQuadBatch::FlushQuads(device, *blupiEffect_, blupiRenderer_, blupiQuads, viewportWidth,
                                        viewportHeight, 1.0f);
            }
            if (!blupi1Quads.empty())
            {
                GEQuadBatch::FlushQuads(device, *blupi1Effect_, blupi1Renderer_, blupi1Quads, viewportWidth,
                                        viewportHeight, 1.0f);
            }
        }
        else
        {
            // Textured palette icons, drawn on top of the flat pass above.
            std::vector<GEQuadBatch::Quad> paletteQuads;
            for (int i = 0; i < kIconsPerPage; ++i)
            {
                const int idx = startIdx + i;
                if (idx >= static_cast<int>(icons.size()))
                {
                    break;
                }
                const GEQuadBatch::Rect cell = PaletteCellRect(i, viewportWidth, viewportHeight);
                const Easy3D::UvRect uv = tileAtlas_.GetTileUv(icons[static_cast<std::size_t>(idx)]);
                paletteQuads.push_back({cell.x0, cell.y0, cell.x1, cell.y1, uv.U0, uv.V0, uv.U1, uv.V1});
            }
            GEQuadBatch::FlushQuads(device, *paletteEffect_, paletteRenderer_, paletteQuads, viewportWidth,
                                   viewportHeight, 1.0f);
        }

        device.setBlendStateProperty(BlendState::Opaque);
    }
}
