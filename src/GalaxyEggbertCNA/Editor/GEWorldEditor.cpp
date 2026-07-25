#include "GEWorldEditor.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        constexpr float kMinObjectSpeed = 0.25f;
        constexpr float kObjectSpeedStep = 0.25f;
        constexpr float kObjectTicksStep = 5.0f;

        std::optional<MoveObjectRecord> FindMoveObjectAnchoredAt(
            const Worlds::World& world, std::uint16_t x, std::uint16_t y, std::uint16_t z)
        {
            for (const auto& record : CollectMoveObjects(world))
            {
                if (static_cast<std::uint16_t>(std::floor(record.posStartX)) == x &&
                    static_cast<std::uint16_t>(std::floor(record.posStartY)) == y &&
                    static_cast<std::uint16_t>(std::floor(record.posStartZ)) == z)
                {
                    return record;
                }
            }
            return std::nullopt;
        }
    }

    void GEWorldEditor::EnterBrowser(int gamerSlot)
    {
        gamerSlot_ = gamerSlot;
        browsing_ = true;
        browserScreen_.Refresh(gamerSlot);
    }

    GEWorldEditor::BrowserRequest GEWorldEditor::UpdateBrowsing(
        const Microsoft::Xna::Framework::Input::MouseState& mouse,
        int viewportWidth, int viewportHeight)
    {
        const auto result = browserScreen_.Update(mouse, viewportWidth, viewportHeight);
        BrowserRequest request;
        if (result.action == GEEditorBrowserScreen::Action::Open)
        {
            request.shouldOpen = true;
            request.openPath = result.path;
        }
        else if (result.action == GEEditorBrowserScreen::Action::New)
        {
            request.shouldCreateNew = true;
        }
        return request;
    }

    void GEWorldEditor::DrawBrowsing(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
        int viewportWidth, int viewportHeight)
    {
        browserScreen_.Draw(device, viewportWidth, viewportHeight);
    }

    void GEWorldEditor::EnterEditing(float startX, float startY, float startZ) noexcept
    {
        browsing_ = false;
        camX_ = startX;
        camY_ = startY;
        camZ_ = startZ;
        yaw_ = 0.0f;
        pitch_ = -0.35f;
        mouseLookHeldLastFrame_ = false;
        hasLastScrollWheelValue_ = false;
        hasHighlight_ = false;
        hasRaycastHit_ = false;
        placementOffsetX_ = placementOffsetY_ = placementOffsetZ_ = 0;
        placementOffsetKeyHeldLastFrame_ = false;
        highlightRenderer_.Hide();
        commandStack_ = GEEditCommandStack();
        boxFirstCornerPlaced_ = false;
        showingBox_ = false;
        playTestRequested_ = false;
        hasSelectedObject_ = false;
        selectionHighlightRenderer_.Hide();
        dirty_ = false;
        stopConfirmArmed_ = false;
    }

    void GEWorldEditor::Update(
        const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
        const Microsoft::Xna::Framework::Input::MouseState& mouse,
        float dt, int viewportWidth, int viewportHeight,
        Easy3D::Camera3D& camera, Worlds::World& world)
    {
        const Easy3D::Camera3D::Vector3 forward = UpdateCamera(keyboard, mouse, dt, camera);
        const GEEditorPalette::UpdateResult paletteResult =
            palette_.Update(mouse, viewportWidth, viewportHeight, dt);
        const bool placementOffsetKeyHeld =
            UpdatePlacementOffset(keyboard, paletteResult.action);
        UpdatePlacementPreview(world, forward);

        const FrameInput input = ReadFrameInput(keyboard, mouse, paletteResult);
        UpdateBoxPreview(world);
        if (!HandlePlacement(input, world) &&
            !HandleBlockRemoval(input, world) &&
            !HandleSessionAndHistory(input, world) &&
            !HandleBoxFill(input, world) &&
            !HandleSkyRegion(input, world))
        {
            (void)HandleObjectEditing(input, camera, viewportWidth, viewportHeight, world);
        }
        StoreInputEdges(input, placementOffsetKeyHeld);
    }

    std::optional<MoveObjectRecord> GEWorldEditor::ApplyActiveFieldDelta(
        const MoveObjectRecord& record, float sign) const noexcept
    {
        MoveObjectRecord updated = record;
        switch (activeField_)
        {
            case EditableField::Speed:
            {
                const float value = std::max(
                    kMinObjectSpeed, record.speed + sign * kObjectSpeedStep);
                if (value == record.speed) return std::nullopt;
                updated.speed = value;
                break;
            }
            case EditableField::StepAdvanceTicks:
            {
                const float value = std::max(
                    0.0f, record.stepAdvanceTicks + sign * kObjectTicksStep);
                if (value == record.stepAdvanceTicks) return std::nullopt;
                updated.stepAdvanceTicks = value;
                break;
            }
            case EditableField::StepRecedeTicks:
            {
                const float value = std::max(
                    0.0f, record.stepRecedeTicks + sign * kObjectTicksStep);
                if (value == record.stepRecedeTicks) return std::nullopt;
                updated.stepRecedeTicks = value;
                break;
            }
            case EditableField::TimeStopStartTicks:
            {
                const float value = std::max(
                    0.0f, record.timeStopStartTicks + sign * kObjectTicksStep);
                if (value == record.timeStopStartTicks) return std::nullopt;
                updated.timeStopStartTicks = value;
                break;
            }
            case EditableField::TimeStopEndTicks:
            {
                const float value = std::max(
                    0.0f, record.timeStopEndTicks + sign * kObjectTicksStep);
                if (value == record.timeStopEndTicks) return std::nullopt;
                updated.timeStopEndTicks = value;
                break;
            }
        }
        return updated;
    }

    void GEWorldEditor::RefreshSelectedObjectAfterHistoryChange(const Worlds::World& world)
    {
        if (!hasSelectedObject_)
        {
            return;
        }
        const auto refreshed = FindMoveObjectAnchoredAt(
            world, selectedAnchorX_, selectedAnchorY_, selectedAnchorZ_);
        if (refreshed)
        {
            selectedObject_ = *refreshed;
        }
        else
        {
            hasSelectedObject_ = false;
        }
    }

    bool GEWorldEditor::ConsumeNeedsPresentationRebuild() noexcept
    {
        const bool result = needsPresentationRebuild_;
        needsPresentationRebuild_ = false;
        return result;
    }

    bool GEWorldEditor::ConsumePlayTestRequested() noexcept
    {
        const bool result = playTestRequested_;
        playTestRequested_ = false;
        return result;
    }

    void GEWorldEditor::Draw(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
        const Easy3D::Camera3D& camera,
        int viewportWidth, int viewportHeight)
    {
        if (showingBox_)
        {
            highlightRenderer_.ShowBox(
                device, boxMinRenderX_, boxMinRenderY_, boxMinRenderZ_,
                boxMaxRenderX_, boxMaxRenderY_, boxMaxRenderZ_);
            highlightRenderer_.Draw(device, camera);
        }
        else if (hasHighlight_)
        {
            highlightRenderer_.ShowCell(device, highlightX_, highlightY_, highlightZ_);
            highlightRenderer_.Draw(device, camera);
        }
        else
        {
            highlightRenderer_.Hide();
        }

        if (hasSelectedObject_)
        {
            selectionHighlightRenderer_.ShowSelectedObject(
                device, selectedRenderX_, selectedRenderY_, selectedRenderZ_);
            selectionHighlightRenderer_.Draw(device, camera);
        }
        else
        {
            selectionHighlightRenderer_.Hide();
        }

        palette_.Draw(
            device, viewportWidth, viewportHeight, stopConfirmArmed_, hasHighlight_);
    }
}
