#include "EditorPaletteInput.hpp"

#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>

namespace GalaxyEggbert::Editor
{
    EditorPaletteInput::Result EditorPaletteInput::HitTest(
        float x, float y, const EditorPaletteLayout& layout, const State& state,
        int viewportWidth, int viewportHeight) const noexcept
    {
        if (GalaxyEggbert::CNA::GEQuadBatch::InRect(x, y, layout.DeleteToolRect()))
        {
            return {HitKind::DeleteTool, -1, true};
        }
        if (GalaxyEggbert::CNA::GEQuadBatch::InRect(x, y, layout.PlayTestRect(viewportWidth, viewportHeight)))
        {
            return {HitKind::PlayTest, -1, true};
        }
        if (GalaxyEggbert::CNA::GEQuadBatch::InRect(x, y, layout.StopRect(viewportWidth, viewportHeight)))
        {
            return {HitKind::Stop, -1, true};
        }
        if (state.openCategory < 0)
        {
            for (int i = 0; i < EditorPaletteLayout::PlacementButtonCount; ++i)
            {
                if (GalaxyEggbert::CNA::GEQuadBatch::InRect(
                        x, y, layout.PlacementButtonRect(i, viewportWidth, viewportHeight)))
                {
                    return {HitKind::PlacementButton, i, true};
                }
            }
        }
        for (int i = 0; i < state.categoryCount; ++i)
        {
            if (GalaxyEggbert::CNA::GEQuadBatch::InRect(x, y, layout.CategoryButtonRect(i)))
            {
                return {HitKind::Category, i, true};
            }
        }
        if (state.openCategory >= 0)
        {
            for (int i = 0; i < state.contentItemCount; ++i)
            {
                if (GalaxyEggbert::CNA::GEQuadBatch::InRect(
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

    EditorPaletteInput::Result EditorPaletteInput::Update(
        const Microsoft::Xna::Framework::Input::MouseState& mouse,
        const EditorPaletteLayout& layout, const State& state,
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
