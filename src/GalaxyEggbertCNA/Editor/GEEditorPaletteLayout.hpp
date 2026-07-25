#pragma once

#include "Game/GEQuadBatch.hpp"

namespace GalaxyEggbert::CNA
{
    class GEEditorPaletteLayout
    {
    public:
        static constexpr int PlacementButtonCount = 7;
        static constexpr int PlacementPlaceIndex = 6;

        [[nodiscard]] GEQuadBatch::Rect DeleteToolRect() const noexcept;
        [[nodiscard]] GEQuadBatch::Rect PlayTestRect(int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect StopRect(int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect CategoryButtonRect(int categoryIndex) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect PlacementButtonRect(
            int index, int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect PlacementCoordinatesRect(
            float textWidth, float textHeight,
            int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect PaletteCellRect(
            int itemIndex, int itemCount, int openCategory,
            int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] int PopupColumnCount(int viewportWidth) const noexcept;
        [[nodiscard]] float PopupOriginY(
            int itemCount, int openCategory, int viewportWidth, int viewportHeight) const noexcept;
    };
}
