#include "GEEditorPalette.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>

#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>

#include <algorithm>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        // Smaller than the old 48px toolbar buttons -- 9 full-width rows
        // (the old design's action-button count) would need 514px of
        // vertical space alone, more than this engine's actual default
        // 800x480 window has room for. Pairing 2 actions per row (like the
        // existing Prev/Next paging split) keeps 8 actions + paging in 5
        // rows instead of 9, leaving real room for content below.
        constexpr float kButtonSize = 32.0f;
        constexpr float kButtonGap = 4.0f;
        constexpr float kColumnX = 10.0f;
        constexpr float kColumnY0 = 10.0f;

        // Fixed action-button indices, top to bottom, 2 per row (left/right
        // half) -- see ToolbarButtonRect().
        constexpr int kIndexUndo = 0;
        constexpr int kIndexRedo = 1;
        constexpr int kIndexSave = 2;
        constexpr int kIndexBack = 3;
        constexpr int kIndexPlayTest = 4;
        constexpr int kIndexModeToggle = 5;
        constexpr int kIndexBoxFill = 6;
        constexpr int kIndexTabToggle = 7;
        constexpr int kHeaderRowCount = 5; // (Undo/Redo),(Save/Back),(PlayTest/ModeToggle),(BoxFill/TabToggle),(Paging)

        constexpr float kSelectionHighlightPadding = 3.0f;

        // Free-eggbert's own solid-green editor button color (2026-07-19
        // redesign, user-supplied reference screenshot) -- opaque, not
        // translucent like the old grey backing: every button reads as a
        // real, solid UI element, not a faint overlay on the 3D scene.
        Microsoft::Xna::Framework::Vector3 ButtonGreen()
        {
            return {0.20f, 0.62f, 0.22f};
        }
        // Brighter green for a button's "on" state (Objects mode, All tab)
        // -- position alone can't convey this, same reasoning the old
        // mode-toggle color-swap already used.
        Microsoft::Xna::Framework::Vector3 ButtonGreenActive()
        {
            return {0.45f, 0.90f, 0.35f};
        }
        // Gold highlight behind the selected palette icon -- distinct from
        // both greens so the selection reads unambiguously.
        Microsoft::Xna::Framework::Vector3 SelectionGold()
        {
            return {0.95f, 0.80f, 0.20f};
        }
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

    int GEEditorPalette::ItemsPerPage(int viewportHeight) const noexcept
    {
        const float used = kColumnY0 + static_cast<float>(kHeaderRowCount) * (kButtonSize + kButtonGap);
        const float available = static_cast<float>(viewportHeight) - used;
        const int count = static_cast<int>(available / (kButtonSize + kButtonGap));
        return std::max(1, count);
    }

    int GEEditorPalette::PageCount(int viewportHeight) const noexcept
    {
        const int perPage = ItemsPerPage(viewportHeight);
        const int count = static_cast<int>(CurrentIconList().size());
        return std::max(1, (count + perPage - 1) / perPage);
    }

    namespace
    {
        GEQuadBatch::Rect RowRect(int row) noexcept
        {
            const float y0 = kColumnY0 + static_cast<float>(row) * (kButtonSize + kButtonGap);
            return {kColumnX, y0, kColumnX + kButtonSize, y0 + kButtonSize};
        }
        GEQuadBatch::Rect LeftHalf(const GEQuadBatch::Rect& row) noexcept
        {
            const float mid = (row.x0 + row.x1) * 0.5f - kButtonGap * 0.25f;
            return {row.x0, row.y0, mid, row.y1};
        }
        GEQuadBatch::Rect RightHalf(const GEQuadBatch::Rect& row) noexcept
        {
            const float mid = (row.x0 + row.x1) * 0.5f + kButtonGap * 0.25f;
            return {mid, row.y0, row.x1, row.y1};
        }
    }

    GEQuadBatch::Rect GEEditorPalette::ToolbarButtonRect(int index) const noexcept
    {
        const GEQuadBatch::Rect row = RowRect(index / 2);
        return (index % 2 == 1) ? RightHalf(row) : LeftHalf(row);
    }

    GEQuadBatch::Rect GEEditorPalette::TabToggleRect(int, int) const noexcept
    {
        return ToolbarButtonRect(kIndexTabToggle);
    }

    GEQuadBatch::Rect GEEditorPalette::PagePrevRect(int, int) const noexcept
    {
        return LeftHalf(RowRect(kHeaderRowCount - 1));
    }

    GEQuadBatch::Rect GEEditorPalette::PageNextRect(int, int) const noexcept
    {
        return RightHalf(RowRect(kHeaderRowCount - 1));
    }

    GEQuadBatch::Rect GEEditorPalette::PaletteCellRect(int indexOnPage, int, int) const noexcept
    {
        return RowRect(kHeaderRowCount + indexOnPage);
    }

    bool GEEditorPalette::HitsAnyControl(float x, float y, int viewportWidth,
                                          int viewportHeight) const noexcept
    {
        for (int i = 0; i <= kIndexTabToggle; ++i)
        {
            if (GEQuadBatch::InRect(x, y, ToolbarButtonRect(i)))
            {
                return true;
            }
        }
        const int pageCount = PageCount(viewportHeight);
        if (pageCount > 1 &&
            (GEQuadBatch::InRect(x, y, PagePrevRect(viewportWidth, viewportHeight)) ||
             GEQuadBatch::InRect(x, y, PageNextRect(viewportWidth, viewportHeight))))
        {
            return true;
        }
        const auto& icons = CurrentIconList();
        const int perPage = ItemsPerPage(viewportHeight);
        const int startIdx = std::clamp(page_, 0, pageCount - 1) * perPage;
        for (int i = 0; i < perPage; ++i)
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

            const int pageCount = PageCount(viewportHeight);
            page_ = std::clamp(page_, 0, pageCount - 1);

            if (GEQuadBatch::InRect(mx, my, ToolbarButtonRect(kIndexUndo)))
            {
                result.action = ToolbarAction::Undo;
                result.clickConsumed = true;
            }
            else if (GEQuadBatch::InRect(mx, my, ToolbarButtonRect(kIndexRedo)))
            {
                result.action = ToolbarAction::Redo;
                result.clickConsumed = true;
            }
            else if (GEQuadBatch::InRect(mx, my, ToolbarButtonRect(kIndexSave)))
            {
                result.action = ToolbarAction::Save;
                result.clickConsumed = true;
            }
            else if (GEQuadBatch::InRect(mx, my, ToolbarButtonRect(kIndexBack)))
            {
                result.action = ToolbarAction::Back;
                result.clickConsumed = true;
            }
            else if (GEQuadBatch::InRect(mx, my, ToolbarButtonRect(kIndexPlayTest)))
            {
                result.action = ToolbarAction::PlayTest;
                result.clickConsumed = true;
            }
            else if (GEQuadBatch::InRect(mx, my, ToolbarButtonRect(kIndexModeToggle)))
            {
                mode_ = mode_ == PaletteMode::Blocks ? PaletteMode::Objects : PaletteMode::Blocks;
                page_ = 0;
                result.clickConsumed = true;
            }
            else if (GEQuadBatch::InRect(mx, my, ToolbarButtonRect(kIndexBoxFill)))
            {
                result.action = ToolbarAction::BoxFill;
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
                const int perPage = ItemsPerPage(viewportHeight);
                const int startIdx = page_ * perPage;
                for (int i = 0; i < perPage; ++i)
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
        }

        const int pageCount = PageCount(viewportHeight);
        const int clampedPage = std::clamp(page_, 0, pageCount - 1);
        const auto& icons = CurrentIconList();
        const int perPage = ItemsPerPage(viewportHeight);
        const int startIdx = clampedPage * perPage;

        device.setBlendStateProperty(BlendState::NonPremultiplied);

        // Solid green backing FIRST: the always-green action buttons, the
        // mode-toggle/tab-toggle "on"-state buttons (brighter green), and
        // every content cell (2026-07-19 redesign, matching free-eggbert's
        // own solid-green button style) -- the real icon (or, for the
        // fixed action buttons, nothing further) draws on top of this.
        std::vector<GEQuadBatch::Quad> greenQuads;
        std::vector<GEQuadBatch::Quad> greenActiveQuads;
        std::vector<GEQuadBatch::Quad> highlightQuads;
        const auto addGreen = [&greenQuads](const GEQuadBatch::Rect& r)
        {
            greenQuads.push_back({r.x0, r.y0, r.x1, r.y1, 0.0f, 0.0f, 1.0f, 1.0f});
        };
        const auto addGreenActive = [&greenActiveQuads](const GEQuadBatch::Rect& r)
        {
            greenActiveQuads.push_back({r.x0, r.y0, r.x1, r.y1, 0.0f, 0.0f, 1.0f, 1.0f});
        };

        addGreen(ToolbarButtonRect(kIndexUndo));
        addGreen(ToolbarButtonRect(kIndexRedo));
        addGreen(ToolbarButtonRect(kIndexSave));
        addGreen(ToolbarButtonRect(kIndexBack));
        addGreen(ToolbarButtonRect(kIndexPlayTest));
        if (mode_ == PaletteMode::Objects)
        {
            addGreenActive(ToolbarButtonRect(kIndexModeToggle));
        }
        else
        {
            addGreen(ToolbarButtonRect(kIndexModeToggle));
        }
        addGreen(ToolbarButtonRect(kIndexBoxFill));
        if (showAllTab_)
        {
            addGreenActive(ToolbarButtonRect(kIndexTabToggle));
        }
        else
        {
            addGreen(ToolbarButtonRect(kIndexTabToggle));
        }
        if (pageCount > 1)
        {
            addGreen(PagePrevRect(viewportWidth, viewportHeight));
            addGreen(PageNextRect(viewportWidth, viewportHeight));
        }

        const int selectedId = mode_ == PaletteMode::Objects ? selectedObjectType_ : selectedBlockType_;
        for (int i = 0; i < perPage; ++i)
        {
            const int idx = startIdx + i;
            if (idx >= static_cast<int>(icons.size()))
            {
                break;
            }
            const GEQuadBatch::Rect cell = PaletteCellRect(i, viewportWidth, viewportHeight);
            addGreen(cell);
            if (icons[static_cast<std::size_t>(idx)] == selectedId)
            {
                highlightQuads.push_back(
                    {cell.x0 - kSelectionHighlightPadding, cell.y0 - kSelectionHighlightPadding,
                     cell.x1 + kSelectionHighlightPadding, cell.y1 + kSelectionHighlightPadding,
                     0.0f, 0.0f, 1.0f, 1.0f});
            }
        }

        flatEffect_->setDiffuseColorProperty(ButtonGreen());
        GEQuadBatch::FlushQuads(device, *flatEffect_, flatRenderer_, greenQuads, viewportWidth, viewportHeight, 1.0f);
        if (!greenActiveQuads.empty())
        {
            flatEffect_->setDiffuseColorProperty(ButtonGreenActive());
            GEQuadBatch::FlushQuads(device, *flatEffect_, flatRenderer_, greenActiveQuads, viewportWidth,
                                    viewportHeight, 1.0f);
        }
        if (!highlightQuads.empty())
        {
            flatEffect_->setDiffuseColorProperty(SelectionGold());
            GEQuadBatch::FlushQuads(device, *flatEffect_, flatRenderer_, highlightQuads, viewportWidth,
                                    viewportHeight, 1.0f);
        }

        if (mode_ == PaletteMode::Objects)
        {
            // Real per-type icons: GEObjectIcons::GetObjIcon() plus its
            // atlas-selection predicates say which of the 5 real sprite
            // sheets each type's icon actually lives on -- the SAME lookup
            // already used to render real MoveObject billboards during
            // gameplay. Batched per atlas so each needs only one draw call
            // per page. Always phase 0 -- a static preview, no per-cell
            // animation.
            std::vector<GEQuadBatch::Quad> terrainQuads, elementQuads, exploQuads, blupiQuads, blupi1Quads;
            for (int i = 0; i < perPage; ++i)
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
            // Textured palette icons, drawn on top of the green pass above.
            std::vector<GEQuadBatch::Quad> paletteQuads;
            for (int i = 0; i < perPage; ++i)
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
