#pragma once

#include "GEEditorPaletteInput.hpp"
#include "GEEditorPaletteLayout.hpp"
#include "GEEditorPaletteRenderer.hpp"
#include "GEPaletteCategories.hpp"

#include <GalaxyEggbert/def/ObjectType.hpp>

#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>

#include <cstdint>
#include <vector>

namespace GalaxyEggbert::CNA
{
    class GEEditorPalette
    {
    public:
        enum class Action
        {
            None,
            Stop,
            PlayTest,
            PlacementXMinus,
            PlacementXPlus,
            PlacementYMinus,
            PlacementYPlus,
            PlacementZMinus,
            PlacementZPlus,
            PlaceSelection,
        };

        struct UpdateResult
        {
            Action action = Action::None;
            bool clickConsumed = false;
        };

        GEEditorPalette();

        UpdateResult Update(
            const Microsoft::Xna::Framework::Input::MouseState& mouse,
            int viewportWidth, int viewportHeight, float elapsedSeconds = 0.0f);

        [[nodiscard]] bool IsNotYetImplementedNoticeVisible() const noexcept
        {
            return notYetImplementedSeconds_ > 0.0f;
        }

        [[nodiscard]] std::uint16_t SelectedBlockType() const noexcept
        {
            return static_cast<std::uint16_t>(selectedBlockType_);
        }

        [[nodiscard]] bool IsObjectMode() const noexcept
        {
            return objectMode_;
        }

        [[nodiscard]] ObjectType SelectedObjectType() const noexcept
        {
            return ToObjectType(selectedObjectType_);
        }

        void Draw(
            Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
            int viewportWidth, int viewportHeight, bool stopConfirmArmed = false,
            bool hasPlacementPreview = false);

    private:
        [[nodiscard]] const PaletteCategory* OpenCategory() const noexcept;
        [[nodiscard]] const std::vector<int>& ContentBlockIds() const noexcept;
        [[nodiscard]] const std::vector<int>& ContentObjectTypeIds() const noexcept;
        [[nodiscard]] const std::vector<int>& ContentButtonIconIds() const noexcept;

        std::vector<PaletteCategory> categories_;
        int openCategory_ = -1;
        int selectedBlockType_;
        int selectedObjectType_;
        bool objectMode_ = false;
        float notYetImplementedSeconds_ = 0.0f;

        GEEditorPaletteLayout layout_;
        GEEditorPaletteInput input_;
        GEEditorPaletteRenderer renderer_;
    };
}
