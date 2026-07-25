#include "GEEditorPalette.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>

#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>

#include <algorithm>
#include <filesystem>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        constexpr float kButtonSize = 32.0f;
        constexpr float kButtonGap = 4.0f;
        constexpr float kColumnX = 10.0f;
        constexpr float kColumnY0 = 10.0f;
        // Eggbert 2's button.png is a 40px-tile sheet.  Its editor rail
        // uses the tiles at native size with a two-pixel gap: y=11, 53,
        // 95, ... 431 in the original 640x480 editor.
        constexpr float kFreeMenuButtonSize = 40.0f;
        constexpr float kFreeMenuButtonGap = 2.0f;
        // The original rail occupies the left edge. Galaxy-only controls
        // deliberately live away from it so they do not replace Free
        // Eggbert's exit button or its menu order.
        constexpr float kGalaxyControlsX = 450.0f;

        // Fixed actions form a compact row above the permanent category
        // rail. This leaves the rail visible while a category's real icons
        // open beside it, as in the Free Eggbert editor reference.
        constexpr int kIndexUndo = 0;
        constexpr int kIndexRedo = 1;
        constexpr int kIndexSave = 2;
        constexpr int kIndexBack = 3;
        constexpr int kIndexPlayTest = 4;
        constexpr int kIndexModeToggle = 5;
        constexpr int kIndexBoxFill = 6;
        constexpr int kIndexTabToggle = 7;
        constexpr int kIndexSkyRegionPrev = 8;
        constexpr int kIndexSkyRegionNext = 9;
        constexpr int kIndexPagePrev = 10;
        constexpr int kIndexPageNext = 11;
        constexpr int kToolbarButtonCount = 12;
        // Galaxy-only utility controls are intentionally kept apart from
        // Eggbert 2's left rail, but they still use its real button.png
        // glyph language rather than anonymous coloured squares.
        constexpr int kGalaxyToolbarButtonIcons[kToolbarButtonCount] = {
            111, 112, 59, 6, 124, 100, 22, 125, 75, 74, 1, 2};
        constexpr float kCategoryRailY0 = kColumnY0 + kFreeMenuButtonSize + kFreeMenuButtonGap;
        constexpr float kPopupX0 = kColumnX + kFreeMenuButtonSize + kFreeMenuButtonGap;
        constexpr int kFreeDeleteToolButtonIcon = 6; // red X, Event.cpp WM_DECOR1
        // Event.cpp stores the visible icon in the second array (48/40).
        // Its final array (124/119) contains tooltip ids, not button.png
        // tiles -- confusing those two produced the region/platform icons
        // visible in the 2026-07-25 10:11:45 regression screenshot.
        constexpr int kFreePlayTestButtonIcon = 48; // dice, Event.cpp WM_PHASE_PLAYTEST
        constexpr int kFreeStopButtonIcon = 40; // STOP, Event.cpp WM_PHASE_INFO
        constexpr float kFreeButtonTileSize = 40.0f;
        constexpr int kFreeButtonColumns = 6;
        // CButton draws the first six button.png tiles as backgrounds, then
        // draws every table icon at `iconMenu + 6` (Free Eggbert's
        // CButton::Draw()).  All menu vectors below intentionally retain
        // the original table values, so apply this offset in one place.
        constexpr int kButtonSheetIconOffset = 6;

        constexpr float kGlyphCellPx = 32.0f;
        constexpr int kGlyphColumns = 16;
        constexpr float kGlyphAdvance = 17.0f;
        constexpr float kNoticeScale = 0.5f;
        constexpr int kPlacementTouchButtonCount = 7;
        constexpr int kPlacementTouchPlaceIndex = 6;
        constexpr float kPlacementTouchGap = 2.0f;
        constexpr float kPlacementTouchPlaceWidth = 64.0f;
        constexpr float kPlacementTouchCompactPlaceWidth =
            kFreeMenuButtonSize * 2.0f + kPlacementTouchGap;
        constexpr float kPlacementTouchBottom = 49.0f;
        constexpr float kPlacementTouchLeftClearance = 52.0f;
        constexpr float kPlacementTouchRightClearance = 106.0f;

        void AppendNoticeLabel(std::vector<GEQuadBatch::Quad>& quads, const char* message,
                               float leftX, float topY, float textSheetW, float textSheetH)
        {
            float penX = leftX;
            for (const char* c = message; *c != '\0'; ++c)
            {
                const int rank = static_cast<int>(static_cast<unsigned char>(*c));
                const int column = rank % kGlyphColumns;
                const int row = rank / kGlyphColumns;
                const float glyphSize = kGlyphCellPx * kNoticeScale;
                quads.push_back({penX, topY, penX + glyphSize, topY + glyphSize,
                                 static_cast<float>(column) * kGlyphCellPx / textSheetW,
                                 static_cast<float>(row) * kGlyphCellPx / textSheetH,
                                 static_cast<float>(column + 1) * kGlyphCellPx / textSheetW,
                                 static_cast<float>(row + 1) * kGlyphCellPx / textSheetH});
                penX += kGlyphAdvance * kNoticeScale;
            }
        }

        float LabelWidth(const char* message)
        {
            int length = 0;
            while (message[length] != '\0')
            {
                ++length;
            }
            return static_cast<float>(length) * kGlyphAdvance * kNoticeScale;
        }

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
        : blockCategories_(ConfirmedBlockCategories()), allIcons_(AllBlockIconIdsInOrder()),
          objectCategories_(ConfirmedObjectCategories()), allObjects_(AllObjectTypeIdsInOrder()),
          selectedBlockType_(static_cast<int>(GalaxyEggbert::BlockTypes::RockPile)),
          selectedObjectType_(static_cast<int>(ObjectType::ObjectType6))
    {
    }

    const std::vector<int>& GEEditorPalette::CurrentIconList() const noexcept
    {
        static const std::vector<int> empty;
        if (showAllTab_)
        {
            return mode_ == PaletteMode::Objects ? allObjects_ : allIcons_;
        }
        const auto& categories = CurrentCategories();
        if (openCategory_ >= 0 && openCategory_ < static_cast<int>(categories.size()))
        {
            return categories[static_cast<std::size_t>(openCategory_)].iconIds;
        }
        return empty;
    }

    int GEEditorPalette::ContentItemCount() const noexcept
    {
        return static_cast<int>(CurrentIconList().size());
    }

    int GEEditorPalette::ContentItemId(int index) const noexcept
    {
        return CurrentIconList()[static_cast<std::size_t>(index)];
    }

    int GEEditorPalette::ContentObjectTypeId(int index) const noexcept
    {
        if (showAllTab_)
        {
            return mode_ == PaletteMode::Objects ? ContentItemId(index) : 0;
        }
        const auto& categories = CurrentCategories();
        if (openCategory_ < 0 || openCategory_ >= static_cast<int>(categories.size()))
        {
            return 0;
        }
        const auto& objectIds = categories[static_cast<std::size_t>(openCategory_)].objectTypeIds;
        return index >= 0 && index < static_cast<int>(objectIds.size()) ?
            objectIds[static_cast<std::size_t>(index)] : 0;
    }

    bool GEEditorPalette::IsContentItemImplemented(int index) const noexcept
    {
        return ContentItemId(index) > 0 || ContentObjectTypeId(index) > 0;
    }

    int GEEditorPalette::ItemsPerPage(int viewportWidth, int viewportHeight) const noexcept
    {
        const float available = static_cast<float>(viewportHeight) - PopupOriginY();
        const int rows = std::max(1, static_cast<int>(available / (kFreeMenuButtonSize + kFreeMenuButtonGap)));
        return PopupColumnCount(viewportWidth) * rows;
    }

    int GEEditorPalette::PageCount(int viewportWidth, int viewportHeight) const noexcept
    {
        const int perPage = ItemsPerPage(viewportWidth, viewportHeight);
        const int count = ContentItemCount();
        return std::max(1, (count + perPage - 1) / perPage);
    }

    GEQuadBatch::Rect GEEditorPalette::ToolbarButtonRect(int index) const noexcept
    {
        const float x0 = kGalaxyControlsX + static_cast<float>(index) * (kButtonSize + kButtonGap);
        return {x0, kColumnY0, x0 + kButtonSize, kColumnY0 + kButtonSize};
    }

    GEQuadBatch::Rect GEEditorPalette::FreeDeleteToolRect() const noexcept
    {
        return {kColumnX, kColumnY0, kColumnX + kFreeMenuButtonSize, kColumnY0 + kFreeMenuButtonSize};
    }

    GEQuadBatch::Rect GEEditorPalette::PlayTestRect(int viewportWidth, int viewportHeight) const noexcept
    {
        const float x0 = static_cast<float>(viewportWidth) - 96.0f;
        const float y0 = static_cast<float>(viewportHeight) - 49.0f;
        return {x0, y0, x0 + kFreeMenuButtonSize, y0 + kFreeMenuButtonSize};
    }

    GEQuadBatch::Rect GEEditorPalette::StopRect(int viewportWidth, int viewportHeight) const noexcept
    {
        const float x0 = static_cast<float>(viewportWidth) - 54.0f;
        const float y0 = static_cast<float>(viewportHeight) - 49.0f;
        return {x0, y0, x0 + kFreeMenuButtonSize, y0 + kFreeMenuButtonSize};
    }

    GEQuadBatch::Rect GEEditorPalette::TabToggleRect(int, int) const noexcept
    {
        return ToolbarButtonRect(kIndexTabToggle);
    }

    GEQuadBatch::Rect GEEditorPalette::PagePrevRect(int, int) const noexcept
    {
        return ToolbarButtonRect(kIndexPagePrev);
    }

    GEQuadBatch::Rect GEEditorPalette::PageNextRect(int, int) const noexcept
    {
        return ToolbarButtonRect(kIndexPageNext);
    }

    GEQuadBatch::Rect GEEditorPalette::CategoryButtonRect(int categoryIndex) const noexcept
    {
        const float y0 = kCategoryRailY0 + static_cast<float>(categoryIndex) *
            (kFreeMenuButtonSize + kFreeMenuButtonGap);
        return {kColumnX, y0, kColumnX + kFreeMenuButtonSize, y0 + kFreeMenuButtonSize};
    }

    GEQuadBatch::Rect GEEditorPalette::PlacementTouchButtonRect(
        int index, int viewportWidth, int viewportHeight) const noexcept
    {
        const float yBottom = static_cast<float>(viewportHeight) - kPlacementTouchBottom;
        const float availableLeft = kPlacementTouchLeftClearance;
        const float availableRight = static_cast<float>(viewportWidth) - kPlacementTouchRightClearance;
        const float horizontalWidth =
            6.0f * (kFreeMenuButtonSize + kPlacementTouchGap) + kPlacementTouchPlaceWidth;

        if (availableRight - availableLeft >= horizontalWidth)
        {
            const float x0 = availableLeft +
                (availableRight - availableLeft - horizontalWidth) * 0.5f;
            if (index == kPlacementTouchPlaceIndex)
            {
                const float placeX = x0 + 6.0f * (kFreeMenuButtonSize + kPlacementTouchGap);
                return {placeX, yBottom, placeX + kPlacementTouchPlaceWidth,
                        yBottom + kFreeMenuButtonSize};
            }
            const float buttonX = x0 + static_cast<float>(index) *
                (kFreeMenuButtonSize + kPlacementTouchGap);
            return {buttonX, yBottom, buttonX + kFreeMenuButtonSize,
                    yBottom + kFreeMenuButtonSize};
        }

        // Narrow/portrait layout: four axis buttons on the upper row, then
        // the remaining Z pair and one double-width PLACE button below.
        const float compactWidth = 4.0f * kFreeMenuButtonSize + 3.0f * kPlacementTouchGap;
        const float x0 = std::max(2.0f, availableRight - compactWidth);
        if (index < 4)
        {
            const float buttonX = x0 + static_cast<float>(index) *
                (kFreeMenuButtonSize + kPlacementTouchGap);
            const float buttonY = yBottom - kFreeMenuButtonSize - kPlacementTouchGap;
            return {buttonX, buttonY, buttonX + kFreeMenuButtonSize,
                    buttonY + kFreeMenuButtonSize};
        }
        if (index < kPlacementTouchPlaceIndex)
        {
            const float buttonX = x0 + static_cast<float>(index - 4) *
                (kFreeMenuButtonSize + kPlacementTouchGap);
            return {buttonX, yBottom, buttonX + kFreeMenuButtonSize,
                    yBottom + kFreeMenuButtonSize};
        }
        const float placeX = x0 + 2.0f * (kFreeMenuButtonSize + kPlacementTouchGap);
        return {placeX, yBottom, placeX + kPlacementTouchCompactPlaceWidth,
                yBottom + kFreeMenuButtonSize};
    }

    int GEEditorPalette::PopupColumnCount(int viewportWidth) const noexcept
    {
        const float available = static_cast<float>(viewportWidth) - kPopupX0 - kColumnX;
        return std::max(1, static_cast<int>(available / (kFreeMenuButtonSize + kFreeMenuButtonGap)));
    }

    float GEEditorPalette::PopupOriginY() const noexcept
    {
        return openCategory_ >= 0 ? CategoryButtonRect(openCategory_).y0 : kCategoryRailY0;
    }

    GEQuadBatch::Rect GEEditorPalette::PaletteCellRect(int indexOnPage, int viewportWidth, int) const noexcept
    {
        const int columns = PopupColumnCount(viewportWidth);
        const int column = indexOnPage % columns;
        const int row = indexOnPage / columns;
        const float x0 = kPopupX0 + static_cast<float>(column) * (kFreeMenuButtonSize + kFreeMenuButtonGap);
        const float y0 = PopupOriginY() + static_cast<float>(row) * (kFreeMenuButtonSize + kFreeMenuButtonGap);
        return {x0, y0, x0 + kFreeMenuButtonSize, y0 + kFreeMenuButtonSize};
    }

    bool GEEditorPalette::HitsAnyControl(float x, float y, int viewportWidth,
                                          int viewportHeight) const noexcept
    {
        if (GEQuadBatch::InRect(x, y, FreeDeleteToolRect()))
        {
            return true;
        }
        // Kept as a keyboard/test compatibility hit area while the visual
        // Galaxy-only toolbar is intentionally removed.  It is not drawn,
        // so Eggbert 2's menu remains the only visible menu.
        for (int i = 0; i < kToolbarButtonCount; ++i)
        {
            if (i == kIndexBack || i == kIndexPlayTest)
            {
                continue;
            }
            if (i >= kIndexPagePrev && PageCount(viewportWidth, viewportHeight) <= 1)
            {
                continue;
            }
            if (GEQuadBatch::InRect(x, y, ToolbarButtonRect(i)))
            {
                return true;
            }
        }
        if (GEQuadBatch::InRect(x, y, PlayTestRect(viewportWidth, viewportHeight)) ||
            GEQuadBatch::InRect(x, y, StopRect(viewportWidth, viewportHeight)))
        {
            return true;
        }
        if (IsCategoryOverview())
        {
            for (int i = 0; i < kPlacementTouchButtonCount; ++i)
            {
                if (GEQuadBatch::InRect(
                        x, y, PlacementTouchButtonRect(i, viewportWidth, viewportHeight)))
                {
                    return true;
                }
            }
        }
        const auto& categories = CurrentCategories();
        for (int i = 0; i < static_cast<int>(categories.size()); ++i)
        {
            if (GEQuadBatch::InRect(x, y, CategoryButtonRect(i)))
            {
                return true;
            }
        }
        if (IsCategoryOverview())
        {
            return false;
        }
        const int pageCount = PageCount(viewportWidth, viewportHeight);
        const int perPage = ItemsPerPage(viewportWidth, viewportHeight);
        const int startIdx = std::clamp(page_, 0, pageCount - 1) * perPage;
        for (int i = 0; i < perPage; ++i)
        {
            if (startIdx + i >= ContentItemCount())
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
        const Microsoft::Xna::Framework::Input::MouseState& mouse, int viewportWidth, int viewportHeight,
        float elapsedSeconds)
    {
        using ButtonState = Microsoft::Xna::Framework::Input::ButtonState;
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;
        const float mx = static_cast<float>(mouse.getXProperty());
        const float my = static_cast<float>(mouse.getYProperty());

        UpdateResult result;
        notYetImplementedSeconds_ = std::max(0.0f, notYetImplementedSeconds_ - std::max(0.0f, elapsedSeconds));
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

            const int pageCount = PageCount(viewportWidth, viewportHeight);
            page_ = std::clamp(page_, 0, pageCount - 1);

            if (GEQuadBatch::InRect(mx, my, FreeDeleteToolRect()))
            {
                // Eggbert 2's top red X is retained in its original first
                // menu position. Galaxy's middle-click delete is already
                // reliable; do not pretend this icon has a second verified
                // behavior until it does.
                notYetImplementedSeconds_ = 2.0f;
                result.clickConsumed = true;
            }
            else if (IsCategoryOverview())
            {
                constexpr ToolbarAction kPlacementActions[kPlacementTouchButtonCount] = {
                    ToolbarAction::PlacementXMinus,
                    ToolbarAction::PlacementXPlus,
                    ToolbarAction::PlacementYMinus,
                    ToolbarAction::PlacementYPlus,
                    ToolbarAction::PlacementZMinus,
                    ToolbarAction::PlacementZPlus,
                    ToolbarAction::PlaceSelection,
                };
                for (int i = 0; i < kPlacementTouchButtonCount; ++i)
                {
                    if (GEQuadBatch::InRect(
                            mx, my, PlacementTouchButtonRect(i, viewportWidth, viewportHeight)))
                    {
                        result.action = kPlacementActions[i];
                        result.clickConsumed = true;
                        break;
                    }
                }
                if (result.clickConsumed)
                {
                    mouseWasDown_ = mouseDown;
                    return result;
                }
            }
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
            else if (GEQuadBatch::InRect(mx, my, StopRect(viewportWidth, viewportHeight)))
            {
                result.action = ToolbarAction::Back;
                result.clickConsumed = true;
            }
            else if (GEQuadBatch::InRect(mx, my, PlayTestRect(viewportWidth, viewportHeight)))
            {
                result.action = ToolbarAction::PlayTest;
                result.clickConsumed = true;
            }
            else if (GEQuadBatch::InRect(mx, my, ToolbarButtonRect(kIndexModeToggle)))
            {
                mode_ = mode_ == PaletteMode::Blocks ? PaletteMode::Objects : PaletteMode::Blocks;
                showAllTab_ = false;
                openCategory_ = -1;
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
                // A click while a group is expanded closes that group first;
                // otherwise this opens/closes the full numeric fallback
                // list without ever hiding the category rail.
                if (openCategory_ >= 0)
                {
                    openCategory_ = -1;
                }
                else
                {
                    showAllTab_ = !showAllTab_;
                }
                page_ = 0;
                result.clickConsumed = true;
            }
            else if (GEQuadBatch::InRect(mx, my, ToolbarButtonRect(kIndexSkyRegionPrev)))
            {
                result.action = ToolbarAction::SkyRegionPrev;
                result.clickConsumed = true;
            }
            else if (GEQuadBatch::InRect(mx, my, ToolbarButtonRect(kIndexSkyRegionNext)))
            {
                result.action = ToolbarAction::SkyRegionNext;
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
                const auto& categories = CurrentCategories();
                for (int i = 0; i < static_cast<int>(categories.size()); ++i)
                {
                    if (GEQuadBatch::InRect(mx, my, CategoryButtonRect(i)))
                    {
                        showAllTab_ = false;
                        openCategory_ = openCategory_ == i ? -1 : i;
                        page_ = 0;
                        result.clickConsumed = true;
                        break;
                    }
                }
                if (result.clickConsumed || IsCategoryOverview())
                {
                    mouseWasDown_ = mouseDown;
                    return result;
                }

                const int perPage = ItemsPerPage(viewportWidth, viewportHeight);
                const int startIdx = page_ * perPage;
                for (int i = 0; i < perPage; ++i)
                {
                    const int idx = startIdx + i;
                    if (idx >= ContentItemCount())
                    {
                        break;
                    }
                    if (GEQuadBatch::InRect(mx, my, PaletteCellRect(i, viewportWidth, viewportHeight)))
                    {
                        if (IsContentItemImplemented(idx))
                        {
                            const int objectType = ContentObjectTypeId(idx);
                            if (objectType > 0)
                            {
                                selectedObjectType_ = objectType;
                                mode_ = PaletteMode::Objects;
                            }
                            else
                            {
                                selectedBlockType_ = ContentItemId(idx);
                                mode_ = PaletteMode::Blocks;
                            }
                            // The source strip has completed its job. Close
                            // it so the placement controls reappear without
                            // requiring a second tap on the category.
                            if (!showAllTab_)
                            {
                                openCategory_ = -1;
                                page_ = 0;
                            }
                        }
                        else
                        {
                            notYetImplementedSeconds_ = 2.0f;
                        }
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
                               int viewportWidth, int viewportHeight, bool backConfirmArmed,
                               bool hasPlacementPreview)
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
        if (!buttonTexture_)
        {
            // Same 240x1040, 40px/6-column sheet that Free Eggbert's
            // WM_PHASE_BUILD menu uses. It contains the category glyphs,
            // the dice test button and the red STOP exit button.
            buttonTexture_ = std::make_unique<Texture2D>("Content/icons/button.png", device);
        }
        ensureTexturedEffect(buttonEffect_, *buttonTexture_);
        if (!textTexture_ && std::filesystem::exists("Content/icons/text.png"))
        {
            textTexture_ = std::make_unique<Texture2D>("Content/icons/text.png", device);
        }
        if (textTexture_)
        {
            ensureTexturedEffect(textEffect_, *textTexture_);
        }
        if (!flatEffect_)
        {
            flatEffect_ = std::make_unique<BasicEffect>(device);
            flatEffect_->VertexColorEnabled = false;
            flatEffect_->setTextureEnabledProperty(false);
        }

        const int pageCount = PageCount(viewportWidth, viewportHeight);
        const int clampedPage = std::clamp(page_, 0, pageCount - 1);
        const int perPage = ItemsPerPage(viewportWidth, viewportHeight);
        const int startIdx = clampedPage * perPage;
        const bool categoryOverview = IsCategoryOverview();

        device.setBlendStateProperty(BlendState::NonPremultiplied);

        // Solid green backing FIRST: the always-green action buttons, the
        // mode-toggle/tab-toggle "on"-state buttons (brighter green), and
        // every category and expanded-menu cell -- the real icon (or, for
        // the fixed action buttons, nothing further) draws on top of this.
        std::vector<GEQuadBatch::Quad> greenQuads;
        std::vector<GEQuadBatch::Quad> greenActiveQuads;
        std::vector<GEQuadBatch::Quad> highlightQuads;
        std::vector<GEQuadBatch::Quad> placementActionQuads;
        const auto addGreen = [&greenQuads](const GEQuadBatch::Rect& r)
        {
            greenQuads.push_back({r.x0, r.y0, r.x1, r.y1, 0.0f, 0.0f, 1.0f, 1.0f});
        };
        const auto addGreenActive = [&greenActiveQuads](const GEQuadBatch::Rect& r)
        {
            greenActiveQuads.push_back({r.x0, r.y0, r.x1, r.y1, 0.0f, 0.0f, 1.0f, 1.0f});
        };

        addGreen(FreeDeleteToolRect());
        if (backConfirmArmed)
        {
            addGreenActive(StopRect(viewportWidth, viewportHeight));
        }
        else
        {
            addGreen(StopRect(viewportWidth, viewportHeight));
        }
        addGreen(PlayTestRect(viewportWidth, viewportHeight));
        if (categoryOverview)
        {
            for (int i = 0; i < kPlacementTouchPlaceIndex; ++i)
            {
                addGreen(PlacementTouchButtonRect(i, viewportWidth, viewportHeight));
            }
            const GEQuadBatch::Rect place =
                PlacementTouchButtonRect(kPlacementTouchPlaceIndex, viewportWidth, viewportHeight);
            placementActionQuads.push_back(
                {place.x0, place.y0, place.x1, place.y1, 0.0f, 0.0f, 1.0f, 1.0f});
        }

        const auto& categories = CurrentCategories();
        for (int i = 0; i < static_cast<int>(categories.size()); ++i)
        {
            const GEQuadBatch::Rect button = CategoryButtonRect(i);
            if (!showAllTab_ && openCategory_ == i)
            {
                addGreenActive(button);
            }
            else
            {
                addGreen(button);
            }
        }

        if (!categoryOverview)
        {
            for (int i = 0; i < perPage; ++i)
            {
                const int idx = startIdx + i;
                if (idx >= ContentItemCount())
                {
                    break;
                }
                const GEQuadBatch::Rect cell = PaletteCellRect(i, viewportWidth, viewportHeight);
                addGreen(cell);
                const int objectType = ContentObjectTypeId(idx);
                const bool selected = objectType > 0 ?
                    (mode_ == PaletteMode::Objects && objectType == selectedObjectType_) :
                    (mode_ == PaletteMode::Blocks && ContentItemId(idx) == selectedBlockType_);
                if (selected)
                {
                    highlightQuads.push_back(
                        {cell.x0 - kSelectionHighlightPadding, cell.y0 - kSelectionHighlightPadding,
                         cell.x1 + kSelectionHighlightPadding, cell.y1 + kSelectionHighlightPadding,
                         0.0f, 0.0f, 1.0f, 1.0f});
                }
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
        if (!placementActionQuads.empty())
        {
            flatEffect_->setDiffuseColorProperty(SelectionGold());
            GEQuadBatch::FlushQuads(device, *flatEffect_, flatRenderer_, placementActionQuads,
                                    viewportWidth, viewportHeight, 1.0f);
        }

        // Free Eggbert's own button-sheet glyphs, drawn over the green
        // squares. The first pass establishes the permanent left rail; the
        // second adds the currently expanded row in the same source order.
        const auto appendButtonIcon = [&](std::vector<GEQuadBatch::Quad>& quads, int icon,
                                          const GEQuadBatch::Rect& rect)
        {
            const float sheetW = static_cast<float>(buttonTexture_->getWidthProperty());
            const float sheetH = static_cast<float>(buttonTexture_->getHeightProperty());
            const int sheetIcon = icon + kButtonSheetIconOffset;
            const int column = sheetIcon % kFreeButtonColumns;
            const int row = sheetIcon / kFreeButtonColumns;
            const float u0 = static_cast<float>(column) * kFreeButtonTileSize / sheetW;
            const float v0 = static_cast<float>(row) * kFreeButtonTileSize / sheetH;
            const float u1 = static_cast<float>(column + 1) * kFreeButtonTileSize / sheetW;
            const float v1 = static_cast<float>(row + 1) * kFreeButtonTileSize / sheetH;
            quads.push_back({rect.x0, rect.y0, rect.x1, rect.y1, u0, v0, u1, v1});
        };
        std::vector<GEQuadBatch::Quad> buttonQuads;
        appendButtonIcon(buttonQuads, kFreeDeleteToolButtonIcon, FreeDeleteToolRect());
        appendButtonIcon(buttonQuads, kFreePlayTestButtonIcon, PlayTestRect(viewportWidth, viewportHeight));
        appendButtonIcon(buttonQuads, kFreeStopButtonIcon, StopRect(viewportWidth, viewportHeight));
        for (int i = 0; i < static_cast<int>(categories.size()); ++i)
        {
            appendButtonIcon(buttonQuads, categories[static_cast<std::size_t>(i)].buttonIconId,
                             CategoryButtonRect(i));
        }
        const std::vector<int>* freeButtonRow =
            (!showAllTab_ && openCategory_ >= 0) ?
                &categories[static_cast<std::size_t>(openCategory_)].buttonIconIds : nullptr;
        if (!categoryOverview)
        {
            for (int i = 0; i < perPage; ++i)
            {
                const int idx = startIdx + i;
                if (idx >= ContentItemCount())
                {
                    break;
                }
                if (freeButtonRow == nullptr || idx >= static_cast<int>(freeButtonRow->size()))
                {
                    continue;
                }
                appendButtonIcon(buttonQuads, (*freeButtonRow)[static_cast<std::size_t>(idx)],
                                 PaletteCellRect(i, viewportWidth, viewportHeight));
            }
        }
        GEQuadBatch::FlushQuads(device, *buttonEffect_, buttonRenderer_, buttonQuads, viewportWidth,
                                viewportHeight, 1.0f);

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
            const auto appendObjectIcon = [&](int typeId, const GEQuadBatch::Rect& cell)
            {
                const ObjectType type = ToObjectType(typeId);
                const int icon = GetObjIcon(type, /*phase=*/0);
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
            };
            if (!categoryOverview)
            {
                for (int i = 0; i < perPage; ++i)
                {
                    const int idx = startIdx + i;
                    if (idx >= ContentItemCount())
                    {
                        break;
                    }
                    if (freeButtonRow != nullptr && idx < static_cast<int>(freeButtonRow->size()))
                    {
                        continue; // source-accurate Free Eggbert button glyph is already drawn above
                    }
                    appendObjectIcon(ContentItemId(idx), PaletteCellRect(i, viewportWidth, viewportHeight));
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
            const auto appendBlockIcon = [&](int iconId, const GEQuadBatch::Rect& cell)
            {
                const Easy3D::UvRect uv = tileAtlas_.GetTileUv(iconId);
                paletteQuads.push_back({cell.x0, cell.y0, cell.x1, cell.y1, uv.U0, uv.V0, uv.U1, uv.V1});
            };
            if (!categoryOverview)
            {
                for (int i = 0; i < perPage; ++i)
                {
                    const int idx = startIdx + i;
                    if (idx >= ContentItemCount())
                    {
                        break;
                    }
                    if (freeButtonRow != nullptr && idx < static_cast<int>(freeButtonRow->size()))
                    {
                        continue; // source-accurate Free Eggbert button glyph is already drawn above
                    }
                    appendBlockIcon(ContentItemId(idx), PaletteCellRect(i, viewportWidth, viewportHeight));
                }
            }
            GEQuadBatch::FlushQuads(device, *paletteEffect_, paletteRenderer_, paletteQuads, viewportWidth,
                                   viewportHeight, 1.0f);
        }

        if (hasPlacementPreview)
        {
            // The target cube says WHERE the next block will be created;
            // this small fixed reticle says exactly WHAT the ray is aiming
            // at.  Together they make a face/adjacent-cell choice legible
            // even at distance or against a busy texture.
            const float cx = static_cast<float>(viewportWidth) * 0.5f;
            const float cy = static_cast<float>(viewportHeight) * 0.5f;
            const std::vector<GEQuadBatch::Quad> reticle = {
                {cx - 10.0f, cy - 1.0f, cx - 3.0f, cy + 1.0f, 0, 0, 1, 1},
                {cx + 3.0f, cy - 1.0f, cx + 10.0f, cy + 1.0f, 0, 0, 1, 1},
                {cx - 1.0f, cy - 10.0f, cx + 1.0f, cy - 3.0f, 0, 0, 1, 1},
                {cx - 1.0f, cy + 3.0f, cx + 1.0f, cy + 10.0f, 0, 0, 1, 1},
            };
            flatEffect_->setDiffuseColorProperty(Microsoft::Xna::Framework::Vector3(1.0f, 0.05f, 0.05f));
            GEQuadBatch::FlushQuads(device, *flatEffect_, flatRenderer_, reticle, viewportWidth, viewportHeight,
                                    1.0f);
        }

        if (categoryOverview && textTexture_ && textEffect_)
        {
            constexpr const char* kLabels[kPlacementTouchButtonCount] = {
                "X-", "X+", "Y-", "Y+", "Z-", "Z+", "PLACE",
            };
            const float textSheetW = static_cast<float>(textTexture_->getWidthProperty());
            const float textSheetH = static_cast<float>(textTexture_->getHeightProperty());
            std::vector<GEQuadBatch::Quad> axisLabels;
            for (int i = 0; i < kPlacementTouchPlaceIndex; ++i)
            {
                const GEQuadBatch::Rect button =
                    PlacementTouchButtonRect(i, viewportWidth, viewportHeight);
                AppendNoticeLabel(
                    axisLabels, kLabels[i],
                    button.x0 + ((button.x1 - button.x0) - LabelWidth(kLabels[i])) * 0.5f,
                    button.y0 + ((button.y1 - button.y0) - kGlyphCellPx * kNoticeScale) * 0.5f,
                    textSheetW, textSheetH);
            }
            textEffect_->setDiffuseColorProperty(Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f));
            GEQuadBatch::FlushQuads(device, *textEffect_, textRenderer_, axisLabels,
                                    viewportWidth, viewportHeight, 1.0f);

            const GEQuadBatch::Rect place =
                PlacementTouchButtonRect(kPlacementTouchPlaceIndex, viewportWidth, viewportHeight);
            std::vector<GEQuadBatch::Quad> placeLabel;
            AppendNoticeLabel(
                placeLabel, kLabels[kPlacementTouchPlaceIndex],
                place.x0 + ((place.x1 - place.x0) -
                            LabelWidth(kLabels[kPlacementTouchPlaceIndex])) * 0.5f,
                place.y0 + ((place.y1 - place.y0) - kGlyphCellPx * kNoticeScale) * 0.5f,
                textSheetW, textSheetH);
            textEffect_->setDiffuseColorProperty(Microsoft::Xna::Framework::Vector3(0.12f, 0.08f, 0.02f));
            GEQuadBatch::FlushQuads(device, *textEffect_, textRenderer_, placeLabel,
                                    viewportWidth, viewportHeight, 1.0f);
        }

        if (notYetImplementedSeconds_ > 0.0f && textTexture_ && textEffect_)
        {
            // Red, centered, two-second feedback makes every source menu
            // entry safely discoverable while signalling which Galaxy
            // editor actions are still awaiting an implementation.
            constexpr const char* kMessage = "Not yet implemented.";
            const float messageWidth = 20.0f * kGlyphAdvance * kNoticeScale;
            const float messageLeft = (static_cast<float>(viewportWidth) - messageWidth) * 0.5f;
            const float messageTop = static_cast<float>(viewportHeight) * 0.5f - 8.0f;
            const std::vector<GEQuadBatch::Quad> noticeBackground = {
                {messageLeft - 8.0f, messageTop - 6.0f,
                 messageLeft + messageWidth + 8.0f, messageTop + kGlyphCellPx * kNoticeScale + 6.0f,
                 0, 0, 1, 1},
            };
            flatEffect_->setDiffuseColorProperty(Microsoft::Xna::Framework::Vector3(1.0f, 0.90f, 0.05f));
            GEQuadBatch::FlushQuads(device, *flatEffect_, flatRenderer_, noticeBackground,
                                    viewportWidth, viewportHeight, 1.0f);
            std::vector<GEQuadBatch::Quad> notice;
            AppendNoticeLabel(notice, kMessage, messageLeft, messageTop,
                              static_cast<float>(textTexture_->getWidthProperty()),
                              static_cast<float>(textTexture_->getHeightProperty()));
            textEffect_->setDiffuseColorProperty(Microsoft::Xna::Framework::Vector3(1.0f, 0.05f, 0.05f));
            GEQuadBatch::FlushQuads(device, *textEffect_, textRenderer_, notice, viewportWidth, viewportHeight,
                                    1.0f);
        }

        device.setBlendStateProperty(BlendState::Opaque);
    }
}
