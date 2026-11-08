#pragma once
#include <string>

#include "EditorPaletteInput.hpp"
#include "EditorPaletteLayout.hpp"
#include "EditorPaletteRenderer.hpp"
#include "PaletteCategories.hpp"

#include <GalaxyEggbert/Def/ObjectType.hpp>
#include <GalaxyEggbert/PlayerStart.hpp>

#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>

#include <cstdint>
#include <vector>

namespace GalaxyEggbert::Editor
{
    class EditorPalette
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
            SelectMusicTrack,
        };

        struct UpdateResult
        {
            Action action = Action::None;
            bool clickConsumed = false;
            int skyRegion = -1;
            int musicTrack = -1;
            bool picked = false; // an item to place was picked (pasting stops, ED-0010)
        };

        EditorPalette();

        UpdateResult Update(
            const Microsoft::Xna::Framework::Input::MouseState& mouse,
            int viewportWidth, int viewportHeight, float elapsedSeconds = 0.0f);

        // True while the pointer is over a palette control (button, category, item).
        [[nodiscard]] bool IsPointerOverControl(
            const Microsoft::Xna::Framework::Input::MouseState& mouse,
            int viewportWidth, int viewportHeight) const;

        [[nodiscard]] bool IsNotYetImplementedNoticeVisible() const noexcept
        {
            return notYetImplementedSeconds_ > 0.0f;
        }
        [[nodiscard]] bool IsPopupOpen() const noexcept { return openCategory_ >= 0; }

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
        [[nodiscard]] PlayerStartColor SelectedPlayerStartColor() const noexcept { return selectedPlayerStartColor_; }

        [[nodiscard]] bool IsBigDecorMode() const noexcept
        {
            return placementKind_ == PlacementKind::BigDecor;
        }

        [[nodiscard]] PlacementKind SelectedPlacementKind() const noexcept
        {
            return placementKind_;
        }

        [[nodiscard]] GalaxyEggbert::Def::ObjectType SelectedObjectType() const noexcept
        {
            return GalaxyEggbert::Def::ToObjectType(selectedObjectType_);
        }

        [[nodiscard]] std::uint16_t SelectedObjectVisualIcon() const noexcept
        {
            return static_cast<std::uint16_t>(selectedObjectVisualIcon_);
        }

        // The button.png glyph of the entry last chosen (the menu's identity of an item: the lifts'
        // four speeds share one object type).
        [[nodiscard]] int SelectedButtonIcon() const noexcept
        {
            return selectedButtonIcon_;
        }

        [[nodiscard]] std::uint16_t SelectedBigDecorIcon() const noexcept
        {
            return static_cast<std::uint16_t>(selectedBigDecorIcon_);
        }

        // Selects a block icon as a click on its palette entry does (scripted checks).
        void SelectBlock(std::uint16_t blockType) noexcept
        {
            selectedBlockType_ = blockType;
            blockPicked_ = true;
            placementKind_ = PlacementKind::Block;
        }

        // Selects an object type as a click on its palette entry does (scripted checks).
        void SelectObject(GalaxyEggbert::Def::ObjectType type) noexcept
        {
            selectedObjectType_ = static_cast<int>(type);
            selectedObjectVisualIcon_ = 0;
            placementKind_ = PlacementKind::Object;
        }

        // Selects a BigDecor icon as a click on its palette entry does (scripted checks).
        void SelectBigDecor(std::uint16_t icon) noexcept
        {
            selectedBigDecorIcon_ = icon;
            placementKind_ = PlacementKind::BigDecor;
        }

        void SetSelectedSkyRegion(std::uint32_t skyRegion) noexcept
        {
            selectedSkyRegion_ = static_cast<int>(skyRegion);
        }

        void SetSelectedMusicTrack(std::uint32_t musicTrack) noexcept
        {
            selectedMusicTrack_ = static_cast<int>(musicTrack);
        }

        void Draw(
            Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
            int viewportWidth, int viewportHeight, bool stopConfirmArmed = false,
            bool hasPlacementPreview = false,
            int placementX = 0, int placementY = 0, int placementZ = 0,
            const std::vector<std::string>& statusLines = {});

    private:
        [[nodiscard]] const PaletteCategory* OpenCategory() const noexcept;
        [[nodiscard]] const std::vector<int>& ContentBlockIds() const noexcept;
        [[nodiscard]] const std::vector<int>& ContentObjectTypeIds() const noexcept;
        [[nodiscard]] const std::vector<int>& ContentButtonIconIds() const noexcept;
        [[nodiscard]] const std::vector<int>& ContentSkyRegionIds() const noexcept;
        [[nodiscard]] const std::vector<int>& ContentMusicTrackIds() const noexcept;
        [[nodiscard]] const std::vector<int>& ContentObjectVisualIconIds() const noexcept;
        [[nodiscard]] const std::vector<int>& ContentSpawnPointIds() const noexcept;
        [[nodiscard]] const std::vector<int>& ContentBigDecorIconIds() const noexcept;

        std::vector<PaletteCategory> categories_;
        int openCategory_ = -1;
        int selectedBlockType_;
        // A block was picked from the palette (a lift then wears it, BD-0082; before, the default look).
        bool blockPicked_ = false;
        int selectedObjectType_;
        int selectedObjectVisualIcon_ = 0;
        int selectedButtonIcon_ = 0;
        int selectedBigDecorIcon_ = 0;
        PlayerStartColor selectedPlayerStartColor_ = PlayerStartColor::Yellow;
        PlacementKind placementKind_ = PlacementKind::Block;
        int selectedSkyRegion_ = 0;
        int selectedMusicTrack_ = 0;
        float notYetImplementedSeconds_ = 0.0f;

        EditorPaletteLayout layout_;
        EditorPaletteInput input_;
        EditorPaletteRenderer renderer_;
    };
}
