#include "GEEditorPaletteInput.hpp"

#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>

namespace GalaxyEggbert::CNA
{
    GEEditorPaletteInput::Result GEEditorPaletteInput::HitTest(
        float x, float y, const GEEditorPaletteLayout& layout, const State& state,
        int viewportWidth, int viewportHeight) const noexcept
    {
        if (GEQuadBatch::InRect(x, y, layout.DeleteToolRect()))
        {
            return {HitKind::DeleteTool, -1, true};
        }
        if (GEQuadBatch::InRect(x, y, layout.PlayTestRect(viewportWidth, viewportHeight)))
        {
            return {HitKind::PlayTest, -1, true};
        }
        if (GEQuadBatch::InRect(x, y, layout.StopRect(viewportWidth, viewportHeight)))
        {
            return {HitKind::Stop, -1, true};
        }
        if (state.openCategory < 0)
        {
            for (int i = 0; i < GEEditorPaletteLayout::PlacementButtonCount; ++i)
            {
                if (GEQuadBatch::InRect(
                        x, y, layout.PlacementButtonRect(i, viewportWidth, viewportHeight)))
                {
                    return {HitKind::PlacementButton, i, true};
                }
            }
        }
        for (int i = 0; i < state.categoryCount; ++i)
        {
            if (GEQuadBatch::InRect(x, y, layout.CategoryButtonRect(i)))
            {
                return {HitKind::Category, i, true};
            }
        }
        if (state.openCategory >= 0)
        {
            for (int i = 0; i < state.contentItemCount; ++i)
            {
                if (GEQuadBatch::InRect(
                        x, y, layout.PaletteCellRect(
                            i, state.contentItemCount, state.openCategory,
                            viewportWidth, viewportHeight)))
                {
                    return {HitKind::ContentItem, i, true};
                }
            }
        }
        return {};
    }

    GEEditorPaletteInput::Result GEEditorPaletteInput::Update(
        const Microsoft::Xna::Framework::Input::MouseState& mouse,
        const GEEditorPaletteLayout& layout, const State& state,
        int viewportWidth, int viewportHeight)
    {
        using Microsoft::Xna::Framework::Input::ButtonState;
        const bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;
        const float x = static_cast<float>(mouse.getXProperty());
        const float y = static_cast<float>(mouse.getYProperty());

        if (mouseDown && !mouseWasDown_)
        {
            pressStartedOnControl_ =
                HitTest(x, y, layout, state, viewportWidth, viewportHeight).hit != HitKind::None;
        }

        Result result;
        if (mouseDown && pressStartedOnControl_)
        {
            result.clickConsumed = true;
        }
        if (!mouseDown && mouseWasDown_ && pressStartedOnControl_)
        {
            result = HitTest(x, y, layout, state, viewportWidth, viewportHeight);
            result.clickConsumed = true;
            pressStartedOnControl_ = false;
        }
        else if (!mouseDown && mouseWasDown_)
        {
            pressStartedOnControl_ = false;
        }

        mouseWasDown_ = mouseDown;
        return result;
    }
}
