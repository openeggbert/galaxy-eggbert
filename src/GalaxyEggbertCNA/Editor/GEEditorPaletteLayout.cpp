#include "GEEditorPaletteLayout.hpp"

#include <algorithm>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        constexpr float kMenuX = 10.0f;
        constexpr float kMenuY = 10.0f;
        constexpr float kButtonSize = 40.0f;
        constexpr float kGap = 2.0f;
        constexpr float kCategoryY = kMenuY + kButtonSize + kGap;
        constexpr float kPopupX = kMenuX + kButtonSize + kGap;
        constexpr float kPlacementPlaceWidth = 64.0f;
        constexpr float kPlacementCompactPlaceWidth = kButtonSize * 2.0f + kGap;
        constexpr float kPlacementBottom = 49.0f;
        constexpr float kPlacementLeftClearance = 52.0f;
        constexpr float kPlacementRightClearance = 106.0f;
    }

    GEQuadBatch::Rect GEEditorPaletteLayout::DeleteToolRect() const noexcept
    {
        return {kMenuX, kMenuY, kMenuX + kButtonSize, kMenuY + kButtonSize};
    }

    GEQuadBatch::Rect GEEditorPaletteLayout::PlayTestRect(
        int viewportWidth, int viewportHeight) const noexcept
    {
        const float x = static_cast<float>(viewportWidth) - 96.0f;
        const float y = static_cast<float>(viewportHeight) - 49.0f;
        return {x, y, x + kButtonSize, y + kButtonSize};
    }

    GEQuadBatch::Rect GEEditorPaletteLayout::StopRect(
        int viewportWidth, int viewportHeight) const noexcept
    {
        const float x = static_cast<float>(viewportWidth) - 54.0f;
        const float y = static_cast<float>(viewportHeight) - 49.0f;
        return {x, y, x + kButtonSize, y + kButtonSize};
    }

    GEQuadBatch::Rect GEEditorPaletteLayout::CategoryButtonRect(int categoryIndex) const noexcept
    {
        const float y = kCategoryY + static_cast<float>(categoryIndex) * (kButtonSize + kGap);
        return {kMenuX, y, kMenuX + kButtonSize, y + kButtonSize};
    }

    GEQuadBatch::Rect GEEditorPaletteLayout::PlacementButtonRect(
        int index, int viewportWidth, int viewportHeight) const noexcept
    {
        const float yBottom = static_cast<float>(viewportHeight) - kPlacementBottom;
        const float availableLeft = kPlacementLeftClearance;
        const float availableRight = static_cast<float>(viewportWidth) - kPlacementRightClearance;
        const float horizontalWidth =
            6.0f * (kButtonSize + kGap) + kPlacementPlaceWidth;

        if (availableRight - availableLeft >= horizontalWidth)
        {
            const float x = availableLeft +
                (availableRight - availableLeft - horizontalWidth) * 0.5f;
            if (index == PlacementPlaceIndex)
            {
                const float placeX = x + 6.0f * (kButtonSize + kGap);
                return {placeX, yBottom, placeX + kPlacementPlaceWidth, yBottom + kButtonSize};
            }
            const float buttonX = x + static_cast<float>(index) * (kButtonSize + kGap);
            return {buttonX, yBottom, buttonX + kButtonSize, yBottom + kButtonSize};
        }

        const float compactWidth = 4.0f * kButtonSize + 3.0f * kGap;
        const float x = std::max(2.0f, availableRight - compactWidth);
        if (index < 4)
        {
            const float buttonX = x + static_cast<float>(index) * (kButtonSize + kGap);
            const float buttonY = yBottom - kButtonSize - kGap;
            return {buttonX, buttonY, buttonX + kButtonSize, buttonY + kButtonSize};
        }
        if (index < PlacementPlaceIndex)
        {
            const float buttonX = x + static_cast<float>(index - 4) * (kButtonSize + kGap);
            return {buttonX, yBottom, buttonX + kButtonSize, yBottom + kButtonSize};
        }
        const float placeX = x + 2.0f * (kButtonSize + kGap);
        return {placeX, yBottom, placeX + kPlacementCompactPlaceWidth, yBottom + kButtonSize};
    }

    GEQuadBatch::Rect GEEditorPaletteLayout::PlacementCoordinatesRect(
        float textWidth, float textHeight,
        int viewportWidth, int viewportHeight) const noexcept
    {
        constexpr float kTextGap = 8.0f;
        const GEQuadBatch::Rect first =
            PlacementButtonRect(0, viewportWidth, viewportHeight);
        const GEQuadBatch::Rect place =
            PlacementButtonRect(PlacementPlaceIndex, viewportWidth, viewportHeight);
        const GEQuadBatch::Rect playTest = PlayTestRect(viewportWidth, viewportHeight);
        const float top = place.y0 + ((place.y1 - place.y0) - textHeight) * 0.5f;

        const float right = place.x1 + kTextGap;
        if (right + textWidth <= playTest.x0 - kTextGap)
        {
            return {right, top, right + textWidth, top + textHeight};
        }

        const float left = first.x0 - kTextGap - textWidth;
        if (left >= kMenuX + kButtonSize + kTextGap)
        {
            return {left, top, left + textWidth, top + textHeight};
        }

        const float centered = std::clamp(
            (first.x0 + place.x1 - textWidth) * 0.5f,
            2.0f, std::max(2.0f, static_cast<float>(viewportWidth) - textWidth - 2.0f));
        const float above = std::max(
            2.0f, std::min(first.y0, place.y0) - textHeight - 4.0f);
        return {centered, above, centered + textWidth, above + textHeight};
    }

    int GEEditorPaletteLayout::PopupColumnCount(int viewportWidth) const noexcept
    {
        const float available = static_cast<float>(viewportWidth) - kPopupX - kMenuX;
        return std::max(1, static_cast<int>(available / (kButtonSize + kGap)));
    }

    float GEEditorPaletteLayout::PopupOriginY(
        int itemCount, int openCategory, int viewportWidth, int viewportHeight) const noexcept
    {
        const int columns = PopupColumnCount(viewportWidth);
        const int rows = std::max(1, (itemCount + columns - 1) / columns);
        const float popupHeight = static_cast<float>(rows) * kButtonSize +
            static_cast<float>(rows - 1) * kGap;
        const float lowestOrigin = std::max(kMenuY, static_cast<float>(viewportHeight) - popupHeight - 8.0f);
        return std::min(CategoryButtonRect(openCategory).y0, lowestOrigin);
    }

    GEQuadBatch::Rect GEEditorPaletteLayout::PaletteCellRect(
        int itemIndex, int itemCount, int openCategory,
        int viewportWidth, int viewportHeight) const noexcept
    {
        const int columns = PopupColumnCount(viewportWidth);
        const int column = itemIndex % columns;
        const int row = itemIndex / columns;
        const float x = kPopupX + static_cast<float>(column) * (kButtonSize + kGap);
        const float y = PopupOriginY(
            itemCount, openCategory, viewportWidth, viewportHeight) +
            static_cast<float>(row) * (kButtonSize + kGap);
        return {x, y, x + kButtonSize, y + kButtonSize};
    }
}
