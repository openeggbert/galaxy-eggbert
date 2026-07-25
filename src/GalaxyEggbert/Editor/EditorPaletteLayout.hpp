#pragma once

#include <GalaxyEggbert/Game/QuadBatch.hpp>

namespace GalaxyEggbert::Editor
{
    class EditorPaletteLayout
    {
    public:
        static constexpr int PlacementButtonCount = 7;
        static constexpr int PlacementPlaceIndex = 6;
        static constexpr int GalaxyBackgroundCategoryIndex = 10;

        [[nodiscard]] GalaxyEggbert::Game::QuadBatch::Rect DeleteToolRect() const noexcept;
        [[nodiscard]] GalaxyEggbert::Game::QuadBatch::Rect PlayTestRect(int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GalaxyEggbert::Game::QuadBatch::Rect StopRect(int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GalaxyEggbert::Game::QuadBatch::Rect CategoryButtonRect(int categoryIndex) const noexcept;
        [[nodiscard]] GalaxyEggbert::Game::QuadBatch::Rect PlacementButtonRect(
            int index, int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GalaxyEggbert::Game::QuadBatch::Rect PlacementCoordinatesRect(
            float textWidth, float textHeight,
            int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GalaxyEggbert::Game::QuadBatch::Rect PlacementCoordinatesBackgroundRect(
            const GalaxyEggbert::Game::QuadBatch::Rect& textRect,
            int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] GalaxyEggbert::Game::QuadBatch::Rect PaletteCellRect(
            int itemIndex, int itemCount, int openCategory,
            int viewportWidth, int viewportHeight) const noexcept;
        [[nodiscard]] int PopupColumnCount(
            int viewportWidth, int openCategory = -1) const noexcept;
        [[nodiscard]] float PopupOriginY(
            int itemCount, int openCategory, int viewportWidth, int viewportHeight) const noexcept;
    };
}
