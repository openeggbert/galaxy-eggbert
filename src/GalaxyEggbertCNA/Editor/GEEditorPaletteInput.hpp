#pragma once

#include "GEEditorPaletteLayout.hpp"

#include <Microsoft/Xna/Framework/Input/Mouse.hpp>

namespace GalaxyEggbert::CNA
{
    class GEEditorPaletteInput
    {
    public:
        enum class HitKind
        {
            None,
            DeleteTool,
            PlayTest,
            Stop,
            PlacementButton,
            Category,
            ContentItem,
        };

        struct State
        {
            int categoryCount = 0;
            int contentItemCount = 0;
            int openCategory = -1;
        };

        struct Result
        {
            HitKind hit = HitKind::None;
            int index = -1;
            bool clickConsumed = false;
        };

        [[nodiscard]] Result Update(
            const Microsoft::Xna::Framework::Input::MouseState& mouse,
            const GEEditorPaletteLayout& layout, const State& state,
            int viewportWidth, int viewportHeight);

    private:
        [[nodiscard]] Result HitTest(
            float x, float y, const GEEditorPaletteLayout& layout, const State& state,
            int viewportWidth, int viewportHeight) const noexcept;

        bool mouseWasDown_ = false;
        bool pressStartedOnControl_ = false;
    };
}
