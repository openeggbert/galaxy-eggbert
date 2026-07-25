#pragma once

#include "Game/GEQuadBatch.hpp"

namespace GalaxyEggbert::Editor
{
    class EditorPaletteLayout
    {
    public:
        static constexpr int PlacementButtonCount = 7;
        static constexpr int PlacementPlaceIndex = 6;
        static constexpr int GalaxyBackgroundCategoryIndex = 10;

        [[nodiscard]] GalaxyEggbert::CNA::GEQuadBatch::Rect DeleteToolRect() const noexcept;
        [[nodiscard]] GalaxyEggbert::CNA::GEQuadBatch::Rect PlayTestRect(int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GalaxyEggbert::CNA::GEQuadBatch::Rect StopRect(int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GalaxyEggbert::CNA::GEQuadBatch::Rect CategoryButtonRect(int categoryIndex) const noexcept;
        [[nodiscard]] GalaxyEggbert::CNA::GEQuadBatch::Rect PlacementButtonRect(
            int index, int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GalaxyEggbert::CNA::GEQuadBatch::Rect PlacementCoordinatesRect(
            float textWidth, float textHeight,
            int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GalaxyEggbert::CNA::GEQuadBatch::Rect PlacementCoordinatesBackgroundRect(
            const GalaxyEggbert::CNA::GEQuadBatch::Rect& textRect,
            int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GalaxyEggbert::CNA::GEQuadBatch::Rect PaletteCellRect(
            int itemIndex, int itemCount, int openCategory,
            int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] int PopupColumnCount(
            int viewportWidth, int openCategory = -1) const noexcept;
        [[nodiscard]] float PopupOriginY(
            int itemCount, int openCategory, int viewportWidth, int viewportHeight) const noexcept;
    };
}
