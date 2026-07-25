#include "GEWorldEditor.hpp"

#include "GEBoxRegion.hpp"
#include "GEVoxelRaycast.hpp"
#include "Game/GEHud.hpp"
#include "Game/GEWorldRuntime.hpp"

#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        constexpr float kPitchLimit = 1.55334f;
        constexpr float kMouseLookRadiansPerPixel = 0.0025f;
        constexpr float kEditorDefaultFov = 0.785398163f;
        constexpr float kEditorMinFov = 0.261799388f;
        constexpr float kEditorMaxFov = 1.3962634f;
        constexpr float kScrollZoomStepPerNotch = 0.90f;
        constexpr float kMaxRaycastDistance = 200.0f;
        constexpr float kHudCenterX = 320.0f;
        constexpr float kHudCenterY = 240.0f;
        constexpr float kObjectPickRadius = 80.0f;

        std::optional<MoveObjectRecord> FindMoveObjectAt(
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

    Easy3D::Camera3D::Vector3 GEWorldEditor::UpdateCamera(
        const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
        const Microsoft::Xna::Framework::Input::MouseState& mouse,
        float dt, Easy3D::Camera3D& camera)
    {
        using Microsoft::Xna::Framework::Input::ButtonState;
        using Microsoft::Xna::Framework::Input::Keys;
        using Microsoft::Xna::Framework::Input::Mouse;

        const bool rightHeld = mouse.getRightButtonProperty() == ButtonState::Pressed;
        if (rightHeld && !mouseLookHeldLastFrame_)
        {
            Mouse::SetCaptureEXT(true);
            Mouse::setIsRelativeMouseModeEXTProperty(true);
        }
        else if (!rightHeld && mouseLookHeldLastFrame_)
        {
            Mouse::setIsRelativeMouseModeEXTProperty(false);
            Mouse::SetCaptureEXT(false);
        }
        else if (rightHeld)
        {
            yaw_ += static_cast<float>(mouse.getXProperty()) * kMouseLookRadiansPerPixel;
            pitch_ = std::clamp(
                pitch_ - static_cast<float>(mouse.getYProperty()) * kMouseLookRadiansPerPixel,
                -kPitchLimit, kPitchLimit);
        }
        mouseLookHeldLastFrame_ = rightHeld;

        const int scroll = mouse.getScrollWheelValueProperty();
        if (!hasLastScrollWheelValue_)
        {
            camera.SetFieldOfView(kEditorDefaultFov);
        }
        else
        {
            const int notches = (scroll - lastScrollWheelValue_) / 120;
            if (notches != 0)
            {
                camera.SetFieldOfView(std::clamp(
                    camera.GetFieldOfView() *
                        std::pow(kScrollZoomStepPerNotch, static_cast<float>(notches)),
                    kEditorMinFov, kEditorMaxFov));
            }
        }
        lastScrollWheelValue_ = scroll;
        hasLastScrollWheelValue_ = true;

        const float cosPitch = std::cos(pitch_);
        const Easy3D::Camera3D::Vector3 forward(
            std::sin(yaw_) * cosPitch, std::sin(pitch_), -std::cos(yaw_) * cosPitch);
        const Easy3D::Camera3D::Vector3 right(std::cos(yaw_), 0.0f, std::sin(yaw_));
        const Easy3D::Camera3D::Vector3 up(0.0f, 1.0f, 0.0f);

        float moveX = 0.0f;
        float moveY = 0.0f;
        float moveZ = 0.0f;
        const auto move = [&moveX, &moveY, &moveZ](
            const Easy3D::Camera3D::Vector3& axis, float amount)
        {
            moveX += axis.X * amount;
            moveY += axis.Y * amount;
            moveZ += axis.Z * amount;
        };
        if (keyboard.IsKeyDown(Keys::W)) move(forward, 1.0f);
        if (keyboard.IsKeyDown(Keys::S)) move(forward, -1.0f);
        if (keyboard.IsKeyDown(Keys::D)) move(right, 1.0f);
        if (keyboard.IsKeyDown(Keys::A)) move(right, -1.0f);
        if (keyboard.IsKeyDown(Keys::Space)) move(up, 1.0f);
        if (keyboard.IsKeyDown(Keys::LeftControl)) move(up, -1.0f);

        camX_ += moveX * flySpeed_ * dt;
        camY_ += moveY * flySpeed_ * dt;
        camZ_ += moveZ * flySpeed_ * dt;
        camera.SetPosition({camX_, camY_, camZ_});
        camera.SetTarget({camX_ + forward.X, camY_ + forward.Y, camZ_ + forward.Z});
        camera.SetUp(up);
        return forward;
    }

    bool GEWorldEditor::UpdatePlacementOffset(
        const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
        GEEditorPalette::Action paletteAction)
    {
        using Microsoft::Xna::Framework::Input::Keys;
        switch (paletteAction)
        {
            case GEEditorPalette::Action::PlacementXMinus: --placementOffsetX_; break;
            case GEEditorPalette::Action::PlacementXPlus:  ++placementOffsetX_; break;
            case GEEditorPalette::Action::PlacementYMinus: --placementOffsetY_; break;
            case GEEditorPalette::Action::PlacementYPlus:  ++placementOffsetY_; break;
            case GEEditorPalette::Action::PlacementZMinus: --placementOffsetZ_; break;
            case GEEditorPalette::Action::PlacementZPlus:  ++placementOffsetZ_; break;
            default: break;
        }

        const bool xMinus = keyboard.IsKeyDown(Keys::NumPad4);
        const bool xPlus = keyboard.IsKeyDown(Keys::NumPad6);
        const bool zMinus = keyboard.IsKeyDown(Keys::NumPad8);
        const bool zPlus = keyboard.IsKeyDown(Keys::NumPad2);
        const bool yMinus = keyboard.IsKeyDown(Keys::NumPad7);
        const bool yPlus = keyboard.IsKeyDown(Keys::NumPad9);
        const bool reset = keyboard.IsKeyDown(Keys::NumPad5);
        const bool held = xMinus || xPlus || zMinus || zPlus || yMinus || yPlus || reset;
        if (held && !placementOffsetKeyHeldLastFrame_)
        {
            if (reset)
            {
                placementOffsetX_ = placementOffsetY_ = placementOffsetZ_ = 0;
            }
            else
            {
                placementOffsetX_ += xPlus ? 1 : (xMinus ? -1 : 0);
                placementOffsetZ_ += zPlus ? 1 : (zMinus ? -1 : 0);
                placementOffsetY_ += yPlus ? 1 : (yMinus ? -1 : 0);
            }
        }
        return held;
    }

    void GEWorldEditor::UpdatePlacementPreview(
        const Worlds::World& world, const Easy3D::Camera3D::Vector3& forward)
    {
        const RaycastHit hit = Raycast(
            world,
            camX_ + static_cast<float>(GEWorldRuntime::kWorldCenterX), camY_,
            camZ_ + static_cast<float>(GEWorldRuntime::kWorldCenterZ),
            forward.X, forward.Y, forward.Z, kMaxRaycastDistance);
        hasRaycastHit_ = hit.hit;
        hasHighlight_ = hit.hit;
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        if (hit.hit)
        {
            hitCellX_ = hit.x;
            hitCellY_ = hit.y;
            hitCellZ_ = hit.z;
            const int previewX = static_cast<int>(hit.x) + hit.normalX + placementOffsetX_;
            const int previewY = static_cast<int>(hit.y) + hit.normalY + placementOffsetY_;
            const int previewZ = static_cast<int>(hit.z) + hit.normalZ + placementOffsetZ_;
            if (previewX >= 0 && previewX < blocksPerAxis &&
                previewY >= 0 && previewY < blocksPerAxis &&
                previewZ >= 0 && previewZ < blocksPerAxis)
            {
                highlightX_ = static_cast<float>(previewX - GEWorldRuntime::kWorldCenterX);
                highlightY_ = static_cast<float>(previewY);
                highlightZ_ = static_cast<float>(previewZ - GEWorldRuntime::kWorldCenterZ);
                placementCellX_ = static_cast<std::uint16_t>(previewX);
                placementCellY_ = static_cast<std::uint16_t>(previewY);
                placementCellZ_ = static_cast<std::uint16_t>(previewZ);
            }
            else
            {
                hasHighlight_ = false;
            }
            return;
        }

        if (forward.Y >= -0.0001f)
        {
            return;
        }
        const float distance = -camY_ / forward.Y;
        const int targetX = static_cast<int>(std::floor(
            camX_ + static_cast<float>(GEWorldRuntime::kWorldCenterX) +
            forward.X * distance + 0.5f));
        const int targetZ = static_cast<int>(std::floor(
            camZ_ + static_cast<float>(GEWorldRuntime::kWorldCenterZ) +
            forward.Z * distance + 0.5f));
        if (distance < 0.0f || distance > kMaxRaycastDistance ||
            targetX < 0 || targetX >= blocksPerAxis ||
            targetZ < 0 || targetZ >= blocksPerAxis)
        {
            return;
        }

        hitCellX_ = static_cast<std::uint16_t>(targetX);
        hitCellY_ = 0;
        hitCellZ_ = static_cast<std::uint16_t>(targetZ);
        const int previewX = targetX + placementOffsetX_;
        const int previewY = placementOffsetY_;
        const int previewZ = targetZ + placementOffsetZ_;
        hasHighlight_ =
            previewX >= 0 && previewX < blocksPerAxis &&
            previewY >= 0 && previewY < blocksPerAxis &&
            previewZ >= 0 && previewZ < blocksPerAxis;
        if (hasHighlight_)
        {
            highlightX_ = static_cast<float>(previewX - GEWorldRuntime::kWorldCenterX);
            highlightY_ = static_cast<float>(previewY);
            highlightZ_ = static_cast<float>(previewZ - GEWorldRuntime::kWorldCenterZ);
            placementCellX_ = static_cast<std::uint16_t>(previewX);
            placementCellY_ = static_cast<std::uint16_t>(previewY);
            placementCellZ_ = static_cast<std::uint16_t>(previewZ);
        }
    }

    GEWorldEditor::FrameInput GEWorldEditor::ReadFrameInput(
        const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
        const Microsoft::Xna::Framework::Input::MouseState& mouse,
        GEEditorPalette::UpdateResult paletteResult) const
    {
        using Microsoft::Xna::Framework::Input::ButtonState;
        using Microsoft::Xna::Framework::Input::Keys;
        return {
            paletteResult,
            mouse.getLeftButtonProperty() == ButtonState::Pressed,
            mouse.getMiddleButtonProperty() == ButtonState::Pressed,
            keyboard.IsKeyDown(Keys::Enter),
            keyboard.IsKeyDown(Keys::U),
            keyboard.IsKeyDown(Keys::R),
            keyboard.IsKeyDown(Keys::F),
            keyboard.IsKeyDown(Keys::Escape),
            keyboard.IsKeyDown(Keys::G),
            keyboard.IsKeyDown(Keys::T),
            keyboard.IsKeyDown(Keys::Tab),
            keyboard.IsKeyDown(Keys::OemPlus),
            keyboard.IsKeyDown(Keys::OemMinus),
            keyboard.IsKeyDown(Keys::Delete),
            keyboard.IsKeyDown(Keys::Left),
            keyboard.IsKeyDown(Keys::Right),
        };
    }

    void GEWorldEditor::UpdateBoxPreview(const Worlds::World& world)
    {
        showingBox_ = boxFirstCornerPlaced_ && hasHighlight_;
        if (!showingBox_)
        {
            return;
        }
        const BoxRegion region = NormalizeAndClamp(
            boxCorner0X_, boxCorner0Y_, boxCorner0Z_,
            hitCellX_, hitCellY_, hitCellZ_, static_cast<int>(world.blocksPerAxis()));
        boxMinRenderX_ = static_cast<float>(region.minX - GEWorldRuntime::kWorldCenterX);
        boxMinRenderY_ = static_cast<float>(region.minY);
        boxMinRenderZ_ = static_cast<float>(region.minZ - GEWorldRuntime::kWorldCenterZ);
        boxMaxRenderX_ = static_cast<float>(region.maxX - GEWorldRuntime::kWorldCenterX);
        boxMaxRenderY_ = static_cast<float>(region.maxY);
        boxMaxRenderZ_ = static_cast<float>(region.maxZ - GEWorldRuntime::kWorldCenterZ);
    }

    bool GEWorldEditor::HandlePlacement(const FrameInput& input, Worlds::World& world)
    {
        const bool requested =
            (input.leftHeld && !leftHeldLastFrame_ && !input.palette.clickConsumed) ||
            input.palette.action == GEEditorPalette::Action::PlaceSelection;
        if (!requested || !hasHighlight_ || boxFirstCornerPlaced_)
        {
            return false;
        }

        const auto x = placementCellX_;
        const auto y = placementCellY_;
        const auto z = placementCellZ_;
        GEEditCommand command;
        if (palette_.IsSpawnPointMode())
        {
            command.kind = GEEditCommand::Kind::SpawnPointEdit;
            command.spawnBefore = {
                world.hasSpawnPoint(), world.spawnX(), world.spawnY(), world.spawnZ()};
            command.spawnAfter = {true, x, y, z};
            world.setSpawnPoint(x, y, z);
        }
        else if (palette_.IsObjectMode())
        {
            MoveObjectRecord record;
            record.type = palette_.SelectedObjectType();
            record.visualIcon = palette_.SelectedObjectVisualIcon();
            record.posStartX = static_cast<float>(x);
            record.posStartY = static_cast<float>(y);
            record.posStartZ = static_cast<float>(z);
            record.posEndX = record.posStartX;
            record.posEndY = record.posStartY;
            record.posEndZ = record.posStartZ;
            command.kind = GEEditCommand::Kind::MoveObjectEdit;
            command.objectAnchorX = x;
            command.objectAnchorY = y;
            command.objectAnchorZ = z;
            command.objectBefore = FindMoveObjectAt(world, x, y, z);
            command.objectAfter = record;
            PlaceMoveObject(world, record);
        }
        else
        {
            const Worlds::Block before = world.getBlock(x, y, z);
            const Worlds::Block after = Worlds::Block::make(palette_.SelectedBlockType());
            world.setBlock(x, y, z, after);
            command.kind = GEEditCommand::Kind::BlockEdit;
            command.blockChanges.push_back({x, y, z, before, after});
        }
        commandStack_.Push(std::move(command));
        MarkMutated();
        return true;
    }

    void GEWorldEditor::RemoveObjectWithHistory(
        Worlds::World& world, const MoveObjectRecord& record,
        std::uint16_t anchorX, std::uint16_t anchorY, std::uint16_t anchorZ)
    {
        GEEditCommand command;
        command.kind = GEEditCommand::Kind::MoveObjectEdit;
        command.objectAnchorX = anchorX;
        command.objectAnchorY = anchorY;
        command.objectAnchorZ = anchorZ;
        command.objectBefore = record;
        command.objectAfter = std::nullopt;
        RemoveMoveObject(world, anchorX, anchorY, anchorZ);
        commandStack_.Push(std::move(command));
        if (hasSelectedObject_ &&
            selectedAnchorX_ == anchorX &&
            selectedAnchorY_ == anchorY &&
            selectedAnchorZ_ == anchorZ)
        {
            hasSelectedObject_ = false;
        }
        MarkMutated();
    }

    bool GEWorldEditor::HandleRemoval(const FrameInput& input, Worlds::World& world)
    {
        const bool toolbarRequested =
            input.palette.action == GEEditorPalette::Action::DeleteAtTarget;
        const bool middleRequested =
            input.middleHeld && !middleHeldLastFrame_ && !input.palette.clickConsumed;
        if ((!toolbarRequested && !middleRequested) || boxFirstCornerPlaced_)
        {
            return false;
        }

        if (toolbarRequested && hasSelectedObject_)
        {
            RemoveObjectWithHistory(
                world, selectedObject_,
                selectedAnchorX_, selectedAnchorY_, selectedAnchorZ_);
            return true;
        }

        if (toolbarRequested && palette_.IsObjectMode() && hasHighlight_)
        {
            const std::optional<MoveObjectRecord> object = FindMoveObjectAt(
                world, placementCellX_, placementCellY_, placementCellZ_);
            if (object)
            {
                RemoveObjectWithHistory(
                    world, *object,
                    placementCellX_, placementCellY_, placementCellZ_);
                return true;
            }
        }

        if (!hasRaycastHit_)
        {
            return toolbarRequested;
        }
        const Worlds::Block before = world.getBlock(hitCellX_, hitCellY_, hitCellZ_);
        if (before.isAir())
        {
            return toolbarRequested;
        }
        const Worlds::Block after = Worlds::Block::air();
        world.setBlock(hitCellX_, hitCellY_, hitCellZ_, after);
        GEEditCommand command;
        command.kind = GEEditCommand::Kind::BlockEdit;
        command.blockChanges.push_back({hitCellX_, hitCellY_, hitCellZ_, before, after});
        commandStack_.Push(std::move(command));
        MarkMutated();
        return true;
    }

    bool GEWorldEditor::HandleSessionAndHistory(const FrameInput& input, Worlds::World& world)
    {
        if (input.enterHeld && !enterHeldLastFrame_ && !worldPath_.empty())
        {
            world.saveToFile(worldPath_);
            dirty_ = false;
            stopConfirmArmed_ = false;
            return true;
        }
        if (input.undoHeld && !undoKeyHeldLastFrame_)
        {
            if (commandStack_.Undo(world))
            {
                MarkMutated();
                RefreshSelectedObjectAfterHistoryChange(world);
            }
            return true;
        }
        if (input.redoHeld && !redoKeyHeldLastFrame_)
        {
            if (commandStack_.Redo(world))
            {
                MarkMutated();
                RefreshSelectedObjectAfterHistoryChange(world);
            }
            return true;
        }
        if (input.palette.action == GEEditorPalette::Action::Stop)
        {
            if (dirty_ && !stopConfirmArmed_)
            {
                stopConfirmArmed_ = true;
            }
            else
            {
                stopConfirmArmed_ = false;
                EnterBrowser(gamerSlot_);
            }
            return true;
        }
        if (input.palette.action == GEEditorPalette::Action::PlayTest && !worldPath_.empty())
        {
            world.saveToFile(worldPath_);
            dirty_ = false;
            stopConfirmArmed_ = false;
            playTestRequested_ = true;
            return true;
        }
        return false;
    }

    bool GEWorldEditor::HandleBoxFill(const FrameInput& input, Worlds::World& world)
    {
        if (input.boxHeld && !boxKeyHeldLastFrame_ && hasHighlight_)
        {
            if (!boxFirstCornerPlaced_)
            {
                boxCorner0X_ = hitCellX_;
                boxCorner0Y_ = hitCellY_;
                boxCorner0Z_ = hitCellZ_;
                boxFirstCornerPlaced_ = true;
                return true;
            }

            const BoxRegion region = NormalizeAndClamp(
                boxCorner0X_, boxCorner0Y_, boxCorner0Z_,
                hitCellX_, hitCellY_, hitCellZ_, static_cast<int>(world.blocksPerAxis()));
            GEEditCommand command;
            command.kind = GEEditCommand::Kind::BlockEdit;
            const Worlds::Block fill = Worlds::Block::make(palette_.SelectedBlockType());
            for (int x = region.minX; x <= region.maxX; ++x)
            {
                for (int y = region.minY; y <= region.maxY; ++y)
                {
                    for (int z = region.minZ; z <= region.maxZ; ++z)
                    {
                        const auto ux = static_cast<std::uint16_t>(x);
                        const auto uy = static_cast<std::uint16_t>(y);
                        const auto uz = static_cast<std::uint16_t>(z);
                        const Worlds::Block before = world.getBlock(ux, uy, uz);
                        if (before == fill)
                        {
                            continue;
                        }
                        world.setBlock(ux, uy, uz, fill);
                        command.blockChanges.push_back({ux, uy, uz, before, fill});
                    }
                }
            }
            if (!command.blockChanges.empty())
            {
                commandStack_.Push(std::move(command));
                MarkMutated();
            }
            boxFirstCornerPlaced_ = false;
            showingBox_ = false;
            return true;
        }
        if (input.escapeHeld && !escapeKeyHeldLastFrame_ && boxFirstCornerPlaced_)
        {
            boxFirstCornerPlaced_ = false;
            showingBox_ = false;
            return true;
        }
        return false;
    }

    bool GEWorldEditor::HandleSkyRegion(const FrameInput& input, Worlds::World& world)
    {
        constexpr std::uint32_t kRegionCount = 32;
        const std::uint32_t before = world.skyRegion();
        if (input.palette.action == GEEditorPalette::Action::SelectSkyRegion)
        {
            if (input.palette.skyRegion < 0 ||
                input.palette.skyRegion >= static_cast<int>(kRegionCount))
            {
                return true;
            }
            const auto after = static_cast<std::uint32_t>(input.palette.skyRegion);
            if (after == before)
            {
                return true;
            }
            world.setSkyRegion(after);
            GEEditCommand command;
            command.kind = GEEditCommand::Kind::SkyRegionEdit;
            command.skyRegionBefore = before;
            command.skyRegionAfter = after;
            commandStack_.Push(std::move(command));
            MarkMutated();
            return true;
        }

        int direction = 0;
        if (input.skyPreviousHeld && !skyRegionPrevKeyHeldLastFrame_)
        {
            direction = -1;
        }
        else if (input.skyNextHeld && !skyRegionNextKeyHeldLastFrame_)
        {
            direction = 1;
        }
        if (direction == 0)
        {
            return false;
        }

        const std::uint32_t after = direction < 0 ?
            (before + kRegionCount - 1) % kRegionCount :
            (before + 1) % kRegionCount;
        world.setSkyRegion(after);
        GEEditCommand command;
        command.kind = GEEditCommand::Kind::SkyRegionEdit;
        command.skyRegionBefore = before;
        command.skyRegionAfter = after;
        commandStack_.Push(std::move(command));
        MarkMutated();
        return true;
    }

    bool GEWorldEditor::HandleObjectEditing(
        const FrameInput& input, Easy3D::Camera3D& camera,
        int viewportWidth, int viewportHeight, Worlds::World& world)
    {
        if (input.selectHeld && !selectKeyHeldLastFrame_)
        {
            hasSelectedObject_ = false;
            float bestDistance = kObjectPickRadius * kObjectPickRadius;
            for (const auto& record : CollectMoveObjects(world))
            {
                const Easy3D::Camera3D::Vector3 renderPosition(
                    record.posStartX - static_cast<float>(GEWorldRuntime::kWorldCenterX),
                    record.posStartY,
                    record.posStartZ - static_cast<float>(GEWorldRuntime::kWorldCenterZ));
                float projectedX = 0.0f;
                float projectedY = 0.0f;
                if (!GEHud::ProjectWorldToHudSpace(
                        renderPosition, camera.GetViewMatrix(), camera.GetProjectionMatrix(),
                        viewportWidth, viewportHeight, projectedX, projectedY))
                {
                    continue;
                }
                const float dx = projectedX - kHudCenterX;
                const float dy = projectedY - kHudCenterY;
                const float distance = dx * dx + dy * dy;
                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    hasSelectedObject_ = true;
                    selectedObject_ = record;
                    selectedAnchorX_ = static_cast<std::uint16_t>(std::floor(record.posStartX));
                    selectedAnchorY_ = static_cast<std::uint16_t>(std::floor(record.posStartY));
                    selectedAnchorZ_ = static_cast<std::uint16_t>(std::floor(record.posStartZ));
                    selectedRenderX_ = renderPosition.X;
                    selectedRenderY_ = renderPosition.Y;
                    selectedRenderZ_ = renderPosition.Z;
                }
            }
            return true;
        }
        if (input.targetHeld && !targetKeyHeldLastFrame_ && hasSelectedObject_ && hasHighlight_)
        {
            MoveObjectRecord after = selectedObject_;
            after.posEndX = static_cast<float>(hitCellX_);
            after.posEndY = static_cast<float>(hitCellY_);
            after.posEndZ = static_cast<float>(hitCellZ_);
            GEEditCommand command;
            command.kind = GEEditCommand::Kind::MoveObjectEdit;
            command.objectAnchorX = selectedAnchorX_;
            command.objectAnchorY = selectedAnchorY_;
            command.objectAnchorZ = selectedAnchorZ_;
            command.objectBefore = selectedObject_;
            command.objectAfter = after;
            PlaceMoveObject(world, after);
            commandStack_.Push(std::move(command));
            selectedObject_ = after;
            MarkMutated();
            return true;
        }
        if (input.cycleFieldHeld && !cycleFieldKeyHeldLastFrame_ && hasSelectedObject_)
        {
            activeField_ = static_cast<EditableField>(
                (static_cast<int>(activeField_) + 1) % kEditableFieldCount);
            return true;
        }
        if (((input.increaseFieldHeld && !increaseFieldKeyHeldLastFrame_) ||
             (input.decreaseFieldHeld && !decreaseFieldKeyHeldLastFrame_)) &&
            hasSelectedObject_)
        {
            const float sign =
                input.increaseFieldHeld && !increaseFieldKeyHeldLastFrame_ ? 1.0f : -1.0f;
            const std::optional<MoveObjectRecord> after =
                ApplyActiveFieldDelta(selectedObject_, sign);
            if (after)
            {
                GEEditCommand command;
                command.kind = GEEditCommand::Kind::MoveObjectEdit;
                command.objectAnchorX = selectedAnchorX_;
                command.objectAnchorY = selectedAnchorY_;
                command.objectAnchorZ = selectedAnchorZ_;
                command.objectBefore = selectedObject_;
                command.objectAfter = *after;
                PlaceMoveObject(world, *after);
                commandStack_.Push(std::move(command));
                selectedObject_ = *after;
                MarkMutated();
            }
            return true;
        }
        if (input.deleteObjectHeld && !deleteObjectKeyHeldLastFrame_ && hasSelectedObject_)
        {
            RemoveObjectWithHistory(
                world, selectedObject_,
                selectedAnchorX_, selectedAnchorY_, selectedAnchorZ_);
            return true;
        }
        return false;
    }

    void GEWorldEditor::StoreInputEdges(
        const FrameInput& input, bool placementOffsetKeyHeld) noexcept
    {
        leftHeldLastFrame_ = input.leftHeld;
        middleHeldLastFrame_ = input.middleHeld;
        enterHeldLastFrame_ = input.enterHeld;
        undoKeyHeldLastFrame_ = input.undoHeld;
        redoKeyHeldLastFrame_ = input.redoHeld;
        boxKeyHeldLastFrame_ = input.boxHeld;
        escapeKeyHeldLastFrame_ = input.escapeHeld;
        selectKeyHeldLastFrame_ = input.selectHeld;
        targetKeyHeldLastFrame_ = input.targetHeld;
        cycleFieldKeyHeldLastFrame_ = input.cycleFieldHeld;
        increaseFieldKeyHeldLastFrame_ = input.increaseFieldHeld;
        decreaseFieldKeyHeldLastFrame_ = input.decreaseFieldHeld;
        deleteObjectKeyHeldLastFrame_ = input.deleteObjectHeld;
        skyRegionPrevKeyHeldLastFrame_ = input.skyPreviousHeld;
        skyRegionNextKeyHeldLastFrame_ = input.skyNextHeld;
        placementOffsetKeyHeldLastFrame_ = placementOffsetKeyHeld;
    }
}
