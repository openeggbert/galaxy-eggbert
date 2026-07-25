#include "GEEditorPalette.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>

#include <algorithm>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        const std::vector<int>& EmptyIds()
        {
            static const std::vector<int> empty;
            return empty;
        }
    }

    GEEditorPalette::GEEditorPalette()
        : categories_(ConfirmedBlockCategories()),
          selectedBlockType_(static_cast<int>(GalaxyEggbert::BlockTypes::RockPile)),
          selectedObjectType_(static_cast<int>(ObjectType::ObjectType6))
    {
    }

    const PaletteCategory* GEEditorPalette::OpenCategory() const noexcept
    {
        return openCategory_ >= 0 && openCategory_ < static_cast<int>(categories_.size()) ?
            &categories_[static_cast<std::size_t>(openCategory_)] : nullptr;
    }

    const std::vector<int>& GEEditorPalette::ContentBlockIds() const noexcept
    {
        const PaletteCategory* category = OpenCategory();
        return category ? category->iconIds : EmptyIds();
    }

    const std::vector<int>& GEEditorPalette::ContentObjectTypeIds() const noexcept
    {
        const PaletteCategory* category = OpenCategory();
        return category ? category->objectTypeIds : EmptyIds();
    }

    const std::vector<int>& GEEditorPalette::ContentButtonIconIds() const noexcept
    {
        const PaletteCategory* category = OpenCategory();
        return category ? category->buttonIconIds : EmptyIds();
    }

    GEEditorPalette::UpdateResult GEEditorPalette::Update(
        const Microsoft::Xna::Framework::Input::MouseState& mouse,
        int viewportWidth, int viewportHeight, float elapsedSeconds)
    {
        notYetImplementedSeconds_ = std::max(
            0.0f, notYetImplementedSeconds_ - std::max(0.0f, elapsedSeconds));

        const GEEditorPaletteInput::State inputState{
            static_cast<int>(categories_.size()),
            static_cast<int>(ContentButtonIconIds().size()),
            openCategory_,
        };
        const GEEditorPaletteInput::Result pointer =
            input_.Update(mouse, layout_, inputState, viewportWidth, viewportHeight);

        UpdateResult result;
        result.clickConsumed = pointer.clickConsumed;
        if (pointer.hit == GEEditorPaletteInput::HitKind::None)
        {
            return result;
        }

        switch (pointer.hit)
        {
            case GEEditorPaletteInput::HitKind::DeleteTool:
                notYetImplementedSeconds_ = 2.0f;
                break;
            case GEEditorPaletteInput::HitKind::PlayTest:
                result.action = Action::PlayTest;
                break;
            case GEEditorPaletteInput::HitKind::Stop:
                result.action = Action::Stop;
                break;
            case GEEditorPaletteInput::HitKind::PlacementButton:
            {
                constexpr Action actions[GEEditorPaletteLayout::PlacementButtonCount] = {
                    Action::PlacementXMinus,
                    Action::PlacementXPlus,
                    Action::PlacementYMinus,
                    Action::PlacementYPlus,
                    Action::PlacementZMinus,
                    Action::PlacementZPlus,
                    Action::PlaceSelection,
                };
                if (pointer.index >= 0 &&
                    pointer.index < GEEditorPaletteLayout::PlacementButtonCount)
                {
                    result.action = actions[pointer.index];
                }
                break;
            }
            case GEEditorPaletteInput::HitKind::Category:
                openCategory_ = openCategory_ == pointer.index ? -1 : pointer.index;
                break;
            case GEEditorPaletteInput::HitKind::ContentItem:
            {
                const auto& blockIds = ContentBlockIds();
                const auto& objectIds = ContentObjectTypeIds();
                const int index = pointer.index;
                const int blockType = index >= 0 && index < static_cast<int>(blockIds.size()) ?
                    blockIds[static_cast<std::size_t>(index)] : 0;
                const int objectType = index >= 0 && index < static_cast<int>(objectIds.size()) ?
                    objectIds[static_cast<std::size_t>(index)] : 0;
                if (objectType > 0)
                {
                    selectedObjectType_ = objectType;
                    objectMode_ = true;
                    openCategory_ = -1;
                }
                else if (blockType > 0)
                {
                    selectedBlockType_ = blockType;
                    objectMode_ = false;
                    openCategory_ = -1;
                }
                else
                {
                    notYetImplementedSeconds_ = 2.0f;
                }
                break;
            }
            case GEEditorPaletteInput::HitKind::None:
                break;
        }
        return result;
    }

    void GEEditorPalette::Draw(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
        int viewportWidth, int viewportHeight, bool stopConfirmArmed,
        bool hasPlacementPreview)
    {
        const GEEditorPaletteRenderer::State state{
            categories_,
            ContentBlockIds(),
            ContentObjectTypeIds(),
            ContentButtonIconIds(),
            openCategory_,
            selectedBlockType_,
            selectedObjectType_,
            objectMode_,
            notYetImplementedSeconds_ > 0.0f,
            stopConfirmArmed,
            hasPlacementPreview,
        };
        renderer_.Draw(device, layout_, state, viewportWidth, viewportHeight);
    }
}
