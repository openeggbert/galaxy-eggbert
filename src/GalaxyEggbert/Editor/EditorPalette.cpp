#include "EditorPalette.hpp"

#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Game/ObjectDefinitionRegistry.hpp>

#include <algorithm>

namespace GalaxyEggbert::Editor
{
    namespace
    {
        const std::vector<int>& EmptyIds()
        {
            static const std::vector<int> empty;
            return empty;
        }
    }

    EditorPalette::EditorPalette()
        : categories_(ConfirmedBlockCategories()),
          selectedBlockType_(static_cast<int>(GalaxyEggbert::BlockTypes::RockPile)),
          selectedObjectType_(static_cast<int>(GalaxyEggbert::Def::ObjectType::ObjectType6))
    {
        categories_.push_back(GalaxyBackgroundCategory());
    }

    const PaletteCategory* EditorPalette::OpenCategory() const noexcept
    {
        return openCategory_ >= 0 && openCategory_ < static_cast<int>(categories_.size()) ?
            &categories_[static_cast<std::size_t>(openCategory_)] : nullptr;
    }

    const std::vector<int>& EditorPalette::ContentBlockIds() const noexcept
    {
        const PaletteCategory* category = OpenCategory();
        return category ? category->iconIds : EmptyIds();
    }

    const std::vector<int>& EditorPalette::ContentObjectTypeIds() const noexcept
    {
        const PaletteCategory* category = OpenCategory();
        return category ? category->objectTypeIds : EmptyIds();
    }

    const std::vector<int>& EditorPalette::ContentButtonIconIds() const noexcept
    {
        const PaletteCategory* category = OpenCategory();
        return category ? category->buttonIconIds : EmptyIds();
    }

    const std::vector<int>& EditorPalette::ContentSkyRegionIds() const noexcept
    {
        const PaletteCategory* category = OpenCategory();
        return category ? category->skyRegionIds : EmptyIds();
    }

    const std::vector<int>& EditorPalette::ContentObjectVisualIconIds() const noexcept
    {
        const PaletteCategory* category = OpenCategory();
        return category ? category->objectVisualIconIds : EmptyIds();
    }

    const std::vector<int>& EditorPalette::ContentSpawnPointIds() const noexcept
    {
        const PaletteCategory* category = OpenCategory();
        return category ? category->spawnPointIds : EmptyIds();
    }

    const std::vector<int>& EditorPalette::ContentBigDecorIconIds() const noexcept
    {
        const PaletteCategory* category = OpenCategory();
        return category ? category->bigDecorIconIds : EmptyIds();
    }

    EditorPalette::UpdateResult EditorPalette::Update(
        const Microsoft::Xna::Framework::Input::MouseState& mouse,
        int viewportWidth, int viewportHeight, float elapsedSeconds)
    {
        notYetImplementedSeconds_ = std::max(
            0.0f, notYetImplementedSeconds_ - std::max(0.0f, elapsedSeconds));

        const EditorPaletteInput::State inputState{
            static_cast<int>(categories_.size()),
            static_cast<int>(ContentButtonIconIds().size()),
            openCategory_,
        };
        const EditorPaletteInput::Result pointer =
            input_.Update(mouse, layout_, inputState, viewportWidth, viewportHeight);

        UpdateResult result;
        result.clickConsumed = pointer.clickConsumed;
        if (pointer.hit == EditorPaletteInput::HitKind::None)
        {
            return result;
        }

        switch (pointer.hit)
        {
            case EditorPaletteInput::HitKind::DeleteTool:
                result.action = Action::DeleteAtTarget;
                break;
            case EditorPaletteInput::HitKind::PlayTest:
                result.action = Action::PlayTest;
                break;
            case EditorPaletteInput::HitKind::Stop:
                result.action = Action::Stop;
                break;
            case EditorPaletteInput::HitKind::PlacementButton:
            {
                constexpr Action actions[EditorPaletteLayout::PlacementButtonCount] = {
                    Action::PlacementXMinus,
                    Action::PlacementXPlus,
                    Action::PlacementYMinus,
                    Action::PlacementYPlus,
                    Action::PlacementZMinus,
                    Action::PlacementZPlus,
                    Action::PlaceSelection,
                };
                if (pointer.index >= 0 &&
                    pointer.index < EditorPaletteLayout::PlacementButtonCount)
                {
                    result.action = actions[pointer.index];
                }
                break;
            }
            case EditorPaletteInput::HitKind::Category:
                openCategory_ = openCategory_ == pointer.index ? -1 : pointer.index;
                break;
            case EditorPaletteInput::HitKind::ContentItem:
            {
                const auto& blockIds = ContentBlockIds();
                const auto& objectIds = ContentObjectTypeIds();
                const auto& skyRegionIds = ContentSkyRegionIds();
                const auto& objectVisualIconIds = ContentObjectVisualIconIds();
                const auto& spawnPointIds = ContentSpawnPointIds();
                const auto& bigDecorIconIds = ContentBigDecorIconIds();
                const int index = pointer.index;
                if (index >= 0 && index < static_cast<int>(skyRegionIds.size()))
                {
                    result.action = Action::SelectSkyRegion;
                    result.skyRegion = skyRegionIds[static_cast<std::size_t>(index)];
                    selectedSkyRegion_ = result.skyRegion;
                    break;
                }
                const int blockType = index >= 0 && index < static_cast<int>(blockIds.size()) ?
                    blockIds[static_cast<std::size_t>(index)] : 0;
                const int objectType = index >= 0 && index < static_cast<int>(objectIds.size()) ?
                    objectIds[static_cast<std::size_t>(index)] : 0;
                const int spawnPoint =
                    index >= 0 && index < static_cast<int>(spawnPointIds.size()) ?
                        spawnPointIds[static_cast<std::size_t>(index)] : 0;
                const int bigDecorIcon =
                    index >= 0 && index < static_cast<int>(bigDecorIconIds.size()) ?
                        bigDecorIconIds[static_cast<std::size_t>(index)] : 0;
                if (spawnPoint > 0)
                {
                    placementKind_ = PlacementKind::SpawnPoint;
                    openCategory_ = -1;
                }
                else if (bigDecorIcon > 0)
                {
                    selectedBigDecorIcon_ = bigDecorIcon;
                    placementKind_ = PlacementKind::BigDecor;
                    openCategory_ = -1;
                }
                else if (objectType > 0 &&
                         GalaxyEggbert::Game::GetObjectDefinition(
                             static_cast<GalaxyEggbert::Def::ObjectType>(objectType))
                                 .placementKind ==
                             GalaxyEggbert::Game::ObjectPlacementKind::Direct)
                {
                    selectedObjectType_ = objectType;
                    const int visualIcon =
                        index >= 0 && index < static_cast<int>(objectVisualIconIds.size()) ?
                            objectVisualIconIds[static_cast<std::size_t>(index)] : 0;
                    selectedObjectVisualIcon_ =
                        visualIcon < 0 ? selectedBlockType_ : visualIcon;
                    placementKind_ = PlacementKind::Object;
                    openCategory_ = -1;
                }
                else if (blockType > 0 &&
                         GalaxyEggbert::IsSupportedBlockType(
                             static_cast<std::uint16_t>(blockType)))
                {
                    selectedBlockType_ = blockType;
                    placementKind_ = PlacementKind::Block;
                    openCategory_ = -1;
                }
                else
                {
                    notYetImplementedSeconds_ = 2.0f;
                }
                break;
            }
            case EditorPaletteInput::HitKind::None:
                break;
        }
        return result;
    }

    void EditorPalette::Draw(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
        int viewportWidth, int viewportHeight, bool stopConfirmArmed,
        bool hasPlacementPreview,
        int placementX, int placementY, int placementZ)
    {
        const EditorPaletteRenderer::State state{
            categories_,
            ContentBlockIds(),
            ContentObjectTypeIds(),
            ContentButtonIconIds(),
            ContentSkyRegionIds(),
            ContentSpawnPointIds(),
            ContentBigDecorIconIds(),
            openCategory_,
            selectedBlockType_,
            selectedObjectType_,
            selectedSkyRegion_,
            selectedBigDecorIcon_,
            IsObjectMode(),
            IsSpawnPointMode(),
            IsBigDecorMode(),
            notYetImplementedSeconds_ > 0.0f,
            stopConfirmArmed,
            hasPlacementPreview,
            placementX,
            placementY,
            placementZ,
        };
        renderer_.Draw(device, layout_, state, viewportWidth, viewportHeight);
    }
}
