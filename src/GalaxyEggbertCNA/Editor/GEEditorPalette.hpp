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
        enum class PlacementKind
        {
            Block,
            Object,
            SpawnPoint,
            BigDecor,
        };

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
            DeleteAtTarget,
            SelectSkyRegion,
        };

        struct UpdateResult
        {
            Action action = Action::None;
            bool clickConsumed = false;
            int skyRegion = -1;
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
            return placementKind_ == PlacementKind::Object;
        }

        [[nodiscard]] bool IsSpawnPointMode() const noexcept
        {
            return placementKind_ == PlacementKind::SpawnPoint;
        }

        [[nodiscard]] bool IsBigDecorMode() const noexcept
        {
            return placementKind_ == PlacementKind::BigDecor;
        }

        [[nodiscard]] PlacementKind SelectedPlacementKind() const noexcept
        {
            return placementKind_;
        }

        [[nodiscard]] ObjectType SelectedObjectType() const noexcept
        {
            return ToObjectType(selectedObjectType_);
        }

        [[nodiscard]] std::uint16_t SelectedObjectVisualIcon() const noexcept
        {
            return static_cast<std::uint16_t>(selectedObjectVisualIcon_);
        }

        [[nodiscard]] std::uint16_t SelectedBigDecorIcon() const noexcept
        {
            return static_cast<std::uint16_t>(selectedBigDecorIcon_);
        }

        void SetSelectedSkyRegion(std::uint32_t skyRegion) noexcept
        {
            selectedSkyRegion_ = static_cast<int>(skyRegion);
        }

        void Draw(
            Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
            int viewportWidth, int viewportHeight, bool stopConfirmArmed = false,
            bool hasPlacementPreview = false,
            int placementX = 0, int placementY = 0, int placementZ = 0);

    private:
        [[nodiscard]] const PaletteCategory* OpenCategory() const noexcept;
        [[nodiscard]] const std::vector<int>& ContentBlockIds() const noexcept;
        [[nodiscard]] const std::vector<int>& ContentObjectTypeIds() const noexcept;
        [[nodiscard]] const std::vector<int>& ContentButtonIconIds() const noexcept;
        [[nodiscard]] const std::vector<int>& ContentSkyRegionIds() const noexcept;
        [[nodiscard]] const std::vector<int>& ContentObjectVisualIconIds() const noexcept;
        [[nodiscard]] const std::vector<int>& ContentSpawnPointIds() const noexcept;
        [[nodiscard]] const std::vector<int>& ContentBigDecorIconIds() const noexcept;

        std::vector<PaletteCategory> categories_;
        int openCategory_ = -1;
        int selectedBlockType_;
        int selectedObjectType_;
        int selectedObjectVisualIcon_ = 0;
        int selectedBigDecorIcon_ = 0;
        PlacementKind placementKind_ = PlacementKind::Block;
        int selectedSkyRegion_ = 0;
        float notYetImplementedSeconds_ = 0.0f;

        GEEditorPaletteLayout layout_;
        GEEditorPaletteInput input_;
        GEEditorPaletteRenderer renderer_;
    };
}
