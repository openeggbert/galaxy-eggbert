#include <GalaxyEggbert/Game/PlateRotationMetadata.hpp>
#include "WorldEditor.hpp"
#include "AutoTile.hpp"

#include "BoxRegion.hpp"
#include "VoxelRaycast.hpp"
#include <GalaxyEggbert/Game/BlockModels.hpp>
#include <GalaxyEggbert/Game/Hud.hpp>
#include <GalaxyEggbert/Game/ObjectDefinitionRegistry.hpp>
#include <GalaxyEggbert/Game/WorldRuntime.hpp>

#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace GalaxyEggbert::Editor
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

        std::optional<BigDecorRecord> FindBigDecorAt(
            const Worlds::World& world, std::uint16_t x, std::uint16_t y, std::uint16_t z)
        {
            for (const auto& record : CollectBigDecor(world))
            {
                if (record.x == x && record.y == y && record.z == z)
                {
                    return record;
                }
            }
            return std::nullopt;
        }
    }

    Microsoft::Xna::Framework::Vector3 WorldEditor::UpdateCamera(
        const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
        const Microsoft::Xna::Framework::Input::MouseState& mouse,
        float dt, CnaEngine::Graphics::Camera& camera)
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
            camera.SetPerspective(kEditorDefaultFov, camera.AspectRatio(), camera.NearPlane(), camera.FarPlane());
        }
        else
        {
            const int notches = (scroll - lastScrollWheelValue_) / 120;
            if (notches != 0)
            {
                camera.SetPerspective(std::clamp(
                    camera.FieldOfView() *
                        std::pow(kScrollZoomStepPerNotch, static_cast<float>(notches)),
                    kEditorMinFov, kEditorMaxFov), camera.AspectRatio(), camera.NearPlane(), camera.FarPlane());
            }
        }
        lastScrollWheelValue_ = scroll;
        hasLastScrollWheelValue_ = true;

        const float cosPitch = std::cos(pitch_);
        const Microsoft::Xna::Framework::Vector3 forward(
            std::sin(yaw_) * cosPitch, std::sin(pitch_), -std::cos(yaw_) * cosPitch);
        const Microsoft::Xna::Framework::Vector3 right(std::cos(yaw_), 0.0f, std::sin(yaw_));
        const Microsoft::Xna::Framework::Vector3 up(0.0f, 1.0f, 0.0f);

        float moveX = 0.0f;
        float moveY = 0.0f;
        float moveZ = 0.0f;
        const auto move = [&moveX, &moveY, &moveZ](
            const Microsoft::Xna::Framework::Vector3& axis, float amount)
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
        cameraTarget_ = {camX_ + forward.X, camY_ + forward.Y, camZ_ + forward.Z};
        camera.SetLookAt({camX_, camY_, camZ_}, cameraTarget_, up);
        return forward;
    }

    bool WorldEditor::UpdatePlacementOffset(
        const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
        EditorPalette::Action paletteAction)
    {
        using Microsoft::Xna::Framework::Input::Keys;
        switch (paletteAction)
        {
            case EditorPalette::Action::PlacementXMinus: --placementOffsetX_; break;
            case EditorPalette::Action::PlacementXPlus:  ++placementOffsetX_; break;
            case EditorPalette::Action::PlacementYMinus: --placementOffsetY_; break;
            case EditorPalette::Action::PlacementYPlus:  ++placementOffsetY_; break;
            case EditorPalette::Action::PlacementZMinus: --placementOffsetZ_; break;
            case EditorPalette::Action::PlacementZPlus:  ++placementOffsetZ_; break;
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

    void WorldEditor::UpdatePlacementPreview(
        const Worlds::World& world, const Microsoft::Xna::Framework::Vector3& forward)
    {
        const RaycastHit hit = Raycast(
            world,
            camX_ + static_cast<float>(GalaxyEggbert::Game::WorldRuntime::kWorldCenterX), camY_,
            camZ_ + static_cast<float>(GalaxyEggbert::Game::WorldRuntime::kWorldCenterZ),
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
                highlightX_ = static_cast<float>(previewX - GalaxyEggbert::Game::WorldRuntime::kWorldCenterX);
                highlightY_ = static_cast<float>(previewY);
                highlightZ_ = static_cast<float>(previewZ - GalaxyEggbert::Game::WorldRuntime::kWorldCenterZ);
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
            camX_ + static_cast<float>(GalaxyEggbert::Game::WorldRuntime::kWorldCenterX) +
            forward.X * distance + 0.5f));
        const int targetZ = static_cast<int>(std::floor(
            camZ_ + static_cast<float>(GalaxyEggbert::Game::WorldRuntime::kWorldCenterZ) +
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
            highlightX_ = static_cast<float>(previewX - GalaxyEggbert::Game::WorldRuntime::kWorldCenterX);
            highlightY_ = static_cast<float>(previewY);
            highlightZ_ = static_cast<float>(previewZ - GalaxyEggbert::Game::WorldRuntime::kWorldCenterZ);
            placementCellX_ = static_cast<std::uint16_t>(previewX);
            placementCellY_ = static_cast<std::uint16_t>(previewY);
            placementCellZ_ = static_cast<std::uint16_t>(previewZ);
        }
    }

    WorldEditor::FrameInput WorldEditor::ReadFrameInput(
        const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
        const Microsoft::Xna::Framework::Input::MouseState& mouse,
        EditorPalette::UpdateResult paletteResult) const
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
            keyboard.IsKeyDown(Keys::LeftControl) || keyboard.IsKeyDown(Keys::RightControl),
            keyboard.IsKeyDown(Keys::M),
            keyboard.IsKeyDown(Keys::O),
            keyboard.IsKeyDown(Keys::V),
            keyboard.IsKeyDown(Keys::H),
            keyboard.IsKeyDown(Keys::C),
            keyboard.IsKeyDown(Keys::P),
            keyboard.IsKeyDown(Keys::Q),
            keyboard.IsKeyDown(Keys::X),
        };
    }

    void WorldEditor::UpdateBoxPreview(const Worlds::World& world)
    {
        if (pasting_ && clipboard_ && hasHighlight_ && !boxFirstCornerPlaced_)
        {
            // Where a click stamps the copy: its min corner at the red preview.
            const BoxRegion region = NormalizeAndClamp(
                placementCellX_, placementCellY_, placementCellZ_,
                placementCellX_ + clipboard_->sizeX - 1, placementCellY_ + clipboard_->sizeY - 1,
                placementCellZ_ + clipboard_->sizeZ - 1, static_cast<int>(world.blocksPerAxis()));
            showingBox_ = true;
            boxMinRenderX_ = static_cast<float>(region.minX - GalaxyEggbert::Game::WorldRuntime::kWorldCenterX);
            boxMinRenderY_ = static_cast<float>(region.minY);
            boxMinRenderZ_ = static_cast<float>(region.minZ - GalaxyEggbert::Game::WorldRuntime::kWorldCenterZ);
            boxMaxRenderX_ = static_cast<float>(region.maxX - GalaxyEggbert::Game::WorldRuntime::kWorldCenterX);
            boxMaxRenderY_ = static_cast<float>(region.maxY);
            boxMaxRenderZ_ = static_cast<float>(region.maxZ - GalaxyEggbert::Game::WorldRuntime::kWorldCenterZ);
            return;
        }
        showingBox_ = boxFirstCornerPlaced_ && hasHighlight_;
        if (!showingBox_)
        {
            return;
        }
        const BoxRegion region = NormalizeAndClamp(
            boxCorner0X_, boxCorner0Y_, boxCorner0Z_,
            hitCellX_, hitCellY_, hitCellZ_, static_cast<int>(world.blocksPerAxis()));
        boxMinRenderX_ = static_cast<float>(region.minX - GalaxyEggbert::Game::WorldRuntime::kWorldCenterX);
        boxMinRenderY_ = static_cast<float>(region.minY);
        boxMinRenderZ_ = static_cast<float>(region.minZ - GalaxyEggbert::Game::WorldRuntime::kWorldCenterZ);
        boxMaxRenderX_ = static_cast<float>(region.maxX - GalaxyEggbert::Game::WorldRuntime::kWorldCenterX);
        boxMaxRenderY_ = static_cast<float>(region.maxY);
        boxMaxRenderZ_ = static_cast<float>(region.maxZ - GalaxyEggbert::Game::WorldRuntime::kWorldCenterZ);
    }

    bool WorldEditor::HandlePlacement(const FrameInput& input, Worlds::World& world)
    {
        const bool requested =
            (input.leftHeld && !leftHeldLastFrame_ && !input.palette.clickConsumed) ||
            input.palette.action == EditorPalette::Action::PlaceSelection;
        if (!requested || !hasHighlight_ || boxFirstCornerPlaced_)
        {
            return false;
        }

        const auto x = placementCellX_;
        const auto y = placementCellY_;
        const auto z = placementCellZ_;
        EditCommand command;
        if (!palette_.IsObjectMode())
        {
            pendingEnd_.reset(); // another menu: the moving object stays where it is
        }
        if (palette_.IsSpawnPointMode())
        {
            command.kind = EditCommand::Kind::SpawnPointEdit;
            command.spawnBefore = {
                world.hasSpawnPoint(), world.spawnX(), world.spawnY(), world.spawnZ()};
            command.spawnAfter = {true, x, y, z};
            world.setSpawnPoint(x, y, z);
        }
        else if (palette_.IsBigDecorMode())
        {
            // A click again where the previous click put a BigDecor cycles it (ED-0006; the ray
            // meets the same cell, BigDecor not being voxels); Ctrl places without cycling.
            const std::uint16_t selected = palette_.SelectedBigDecorIcon();
            const std::optional<BigDecorRecord> existing = FindBigDecorAt(world, x, y, z);
            std::optional<std::uint16_t> cycled;
            if (!input.ctrlHeld && existing)
            {
                cycled = bigDecorVariants_.Cycle(x, y, z, existing->icon, selected);
            }
            const BigDecorRecord record{
                cycled ? *cycled : bigDecorVariants_.IconToPlace(selected, input.ctrlHeld), x, y, z};
            command.kind = EditCommand::Kind::BigDecorEdit;
            command.bigDecorAnchorX = x;
            command.bigDecorAnchorY = y;
            command.bigDecorAnchorZ = z;
            command.bigDecorBefore = existing;
            command.bigDecorAfter = record;
            PlaceBigDecor(world, record);
            bigDecorVariants_.NotePlaced(x, y, z);
            commandStack_.Push(std::move(command));
            MarkMutated();
            return true;
        }
        else if (palette_.IsObjectMode() && pendingEnd_)
        {
            // The moving object placed by the previous click takes this cell as its end (the
            // original's m_liftEnd); a click on it again leaves it still.
            const auto anchor = *pendingEnd_;
            pendingEnd_.reset();
            const auto placed = FindMoveObjectAt(world, anchor[0], anchor[1], anchor[2]);
            if (!placed || (x == anchor[0] && y == anchor[1] && z == anchor[2]))
            {
                return true;
            }
            const MoveObjectRecord after = WithEndAt(*placed, x, y, z);
            command.kind = EditCommand::Kind::MoveObjectEdit;
            command.objectAnchorX = anchor[0];
            command.objectAnchorY = anchor[1];
            command.objectAnchorZ = anchor[2];
            command.objectBefore = placed;
            command.objectAfter = after;
            PlaceMoveObject(world, after);
            commandStack_.Push(std::move(command));
            MarkMutated();
            return true;
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
            // A lift takes its menu entry's speed per cell and its rests (the PC editor's
            // PlaceDecor4: slow, normal, fast, fast/slow; the conveyor lift as the normal one);
            // setting its end scales the step by the cells it covers.
            if (const auto preset = LiftPreset(record.type, palette_.SelectedButtonIcon()))
            {
                record.stepAdvanceTicks = preset->advance;
                record.stepRecedeTicks = preset->recede;
                record.timeStopStartTicks = record.timeStopEndTicks = preset->rest;
            }
            command.kind = EditCommand::Kind::MoveObjectEdit;
            command.objectAnchorX = x;
            command.objectAnchorY = y;
            command.objectAnchorZ = z;
            command.objectBefore = FindMoveObjectAt(world, x, y, z);
            command.objectAfter = record;
            PlaceMoveObject(world, record);
            // A lift or a creature on a patrol: the next click sets where it goes (OR-0072).
            if (IsLiftType(record.type) || Game::GetObjectDefinition(record.type).patrolTicksPerCell > 0)
            {
                variants_.Forget();
                commandStack_.Push(std::move(command));
                MarkMutated();
                pendingEnd_ = std::array<std::uint16_t, 3>{x, y, z};
                return true;
            }
        }
        else
        {
            const std::uint16_t selected = palette_.SelectedBlockType();
            // A click on the block the previous click placed cycles its variant (ED-0001, the
            // original's PlaceDecor with bMix; MR-0003); Ctrl places without cycling.
            std::optional<std::uint16_t> cycled;
            if (!input.ctrlHeld)
            {
                cycled = variants_.Cycle(hitCellX_, hitCellY_, hitCellZ_,
                                         world.getBlock(hitCellX_, hitCellY_, hitCellZ_).type(), selected);
            }
            const std::uint16_t cx = cycled ? hitCellX_ : x;
            const std::uint16_t cy = cycled ? hitCellY_ : y;
            const std::uint16_t cz = cycled ? hitCellZ_ : z;
            const Worlds::Block before = world.getBlock(cx, cy, cz);
            const Worlds::Block after =
                Worlds::Block::make(cycled ? *cycled : variants_.IconToPlace(selected, input.ctrlHeld));
            world.setBlock(cx, cy, cz, after);
            variants_.NotePlaced(cx, cy, cz);
            command.kind = EditCommand::Kind::BlockEdit;
            command.blockChanges.push_back({cx, cy, cz, before, after});
            // The PC editor re-tiles after every placement, a variant cycled too (decdesign.cpp,
            // PlaceItemFromMenu*: AdaptBorder(cel); ED-0007).
            AutoTile::AdaptBorder(world, cx, cy, cz, [this]() { return static_cast<int>(autoTileRandom_() & 0x7fffffff); },
                                  command.blockChanges);
            commandStack_.Push(std::move(command));
            MarkMutated();
            return true;
        }
        variants_.Forget();
        commandStack_.Push(std::move(command));
        MarkMutated();
        return true;
    }

    void WorldEditor::RemoveObjectWithHistory(
        Worlds::World& world, const MoveObjectRecord& record,
        std::uint16_t anchorX, std::uint16_t anchorY, std::uint16_t anchorZ)
    {
        EditCommand command;
        command.kind = EditCommand::Kind::MoveObjectEdit;
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

    void WorldEditor::RemoveBigDecorWithHistory(
        Worlds::World& world, const BigDecorRecord& record)
    {
        EditCommand command;
        command.kind = EditCommand::Kind::BigDecorEdit;
        command.bigDecorAnchorX = record.x;
        command.bigDecorAnchorY = record.y;
        command.bigDecorAnchorZ = record.z;
        command.bigDecorBefore = record;
        RemoveBigDecor(world, record.x, record.y, record.z);
        commandStack_.Push(std::move(command));
        MarkMutated();
    }

    bool WorldEditor::HandleRemoval(const FrameInput& input, Worlds::World& world)
    {
        const bool toolbarRequested =
            input.palette.action == EditorPalette::Action::DeleteAtTarget;
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

        if (toolbarRequested && palette_.IsBigDecorMode() && hasHighlight_)
        {
            const auto bigDecor = FindBigDecorAt(
                world, placementCellX_, placementCellY_, placementCellZ_);
            if (bigDecor)
            {
                RemoveBigDecorWithHistory(world, *bigDecor);
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
        EditCommand command;
        command.kind = EditCommand::Kind::BlockEdit;
        command.blockChanges.push_back({hitCellX_, hitCellY_, hitCellZ_, before, after});
        // The PC editor's DeleteCel re-tiles round the emptied cell (ED-0007).
        AutoTile::AdaptBorder(world, hitCellX_, hitCellY_, hitCellZ_,
                              [this]() { return static_cast<int>(autoTileRandom_() & 0x7fffffff); }, command.blockChanges);
        commandStack_.Push(std::move(command));
        MarkMutated();
        return true;
    }

    bool WorldEditor::HandleSessionAndHistory(const FrameInput& input, Worlds::World& world)
    {
        if (input.enterHeld && !enterHeldLastFrame_ && !worldPath_.empty())
        {
            world.saveToFile(worldPath_);
            dirty_ = false;
            stopConfirmArmed_ = false;
            return true;
        }
        if (input.escapeHeld && !escapeKeyHeldLastFrame_ && pendingEnd_ && !boxFirstCornerPlaced_)
        {
            pendingEnd_.reset(); // the moving object stays still
            return true;
        }
        if (input.undoHeld && !undoKeyHeldLastFrame_)
        {
            pendingEnd_.reset();
            if (commandStack_.Undo(world))
            {
                MarkMutated();
                RefreshSelectedObjectAfterHistoryChange(world);
            }
            return true;
        }
        if (input.redoHeld && !redoKeyHeldLastFrame_)
        {
            pendingEnd_.reset();
            if (commandStack_.Redo(world))
            {
                MarkMutated();
                RefreshSelectedObjectAfterHistoryChange(world);
            }
            return true;
        }
        if (input.palette.action == EditorPalette::Action::Stop)
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
        if (input.palette.action == EditorPalette::Action::PlayTest && !worldPath_.empty())
        {
            world.saveToFile(worldPath_);
            dirty_ = false;
            stopConfirmArmed_ = false;
            playTestRequested_ = true;
            return true;
        }
        return false;
    }

    bool WorldEditor::HandleClipboard(const FrameInput& input, Worlds::World& world)
    {
        if (input.pasteHeld && !pasteKeyHeldLastFrame_ && !boxFirstCornerPlaced_)
        {
            pasting_ = !pasting_ && clipboard_.has_value();
            pendingEnd_.reset();
            return true;
        }
        if (!pasting_ || !clipboard_)
        {
            return false;
        }
        if (input.escapeHeld && !escapeKeyHeldLastFrame_)
        {
            pasting_ = false;
            return true;
        }
        if (input.clipTurnHeld && !clipTurnKeyHeldLastFrame_)
        {
            clipboard_ = TurnedQuarter(*clipboard_);
            return true;
        }
        if (input.clipMirrorHeld && !clipMirrorKeyHeldLastFrame_)
        {
            clipboard_ = MirroredX(*clipboard_);
            return true;
        }
        // A pick in the palette (a block, an object) is for placing it: the pasting stops.
        if (input.palette.picked)
        {
            pasting_ = false;
            return false;
        }
        const bool requested =
            (input.leftHeld && !leftHeldLastFrame_ && !input.palette.clickConsumed) ||
            input.palette.action == EditorPalette::Action::PlaceSelection;
        if (!requested)
        {
            return false;
        }
        if (!hasHighlight_)
        {
            return true;
        }
        EditCommand command = PasteBox(world, *clipboard_, placementCellX_, placementCellY_, placementCellZ_,
                                       input.ctrlHeld,
                                       [this]() { return static_cast<int>(autoTileRandom_() & 0x7fffffff); });
        if (!command.children.empty())
        {
            commandStack_.Push(std::move(command));
            MarkMutated();
        }
        return true;
    }

    bool WorldEditor::HandleBoxFill(const FrameInput& input, Worlds::World& world)
    {
        const bool fillPressed = input.boxHeld && !boxKeyHeldLastFrame_;
        const bool copyPressed = input.copyHeld && !copyKeyHeldLastFrame_;
        if ((fillPressed || copyPressed) && hasHighlight_)
        {
            if (!boxFirstCornerPlaced_)
            {
                boxCorner0X_ = hitCellX_;
                boxCorner0Y_ = hitCellY_;
                boxCorner0Z_ = hitCellZ_;
                boxFirstCornerPlaced_ = true;
                boxForCopy_ = copyPressed && !fillPressed;
                pasting_ = false;
                pendingEnd_.reset();
                return true;
            }
            if (boxForCopy_ ? !copyPressed : !fillPressed)
            {
                return true; // the box ends with the key that began it
            }

            const BoxRegion region = NormalizeAndClamp(
                boxCorner0X_, boxCorner0Y_, boxCorner0Z_,
                hitCellX_, hitCellY_, hitCellZ_, static_cast<int>(world.blocksPerAxis()));
            if (boxForCopy_)
            {
                clipboard_ = CopyBox(world, region);
                boxFirstCornerPlaced_ = false;
                showingBox_ = false;
                return true;
            }
            EditCommand command;
            command.kind = EditCommand::Kind::BlockEdit;
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
            // Each cell filled re-tiled as if placed one by one (ED-0007).
            const std::vector<BlockChange> filled = command.blockChanges;
            for (const auto& change : filled)
            {
                AutoTile::AdaptBorder(world, change.x, change.y, change.z,
                                      [this]() { return static_cast<int>(autoTileRandom_() & 0x7fffffff); },
                                      command.blockChanges);
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

    bool WorldEditor::HandleMusic(const FrameInput& input, Worlds::World& world)
    {
        // The palette's Music group picks the world's background music (none, or the original
        // tracks 1-10; OR-0054), and M steps through them (OR-0014); the game plays the chosen
        // one while editing, so it is heard at once.
        const std::uint32_t before = world.musicTrack();
        std::uint32_t after = before;
        if (input.palette.action == EditorPalette::Action::SelectMusicTrack && input.palette.musicTrack >= 0)
        {
            after = static_cast<std::uint32_t>(input.palette.musicTrack);
        }
        else if (input.musicNextHeld && !musicKeyHeldLastFrame_)
        {
            after = (before + 1) % (Worlds::World::kMusicTrackCount + 1);
        }
        else
        {
            return false;
        }
        if (after == before)
        {
            return true;
        }
        world.setMusicTrack(after);
        EditCommand command;
        command.kind = EditCommand::Kind::MusicEdit;
        command.musicBefore = before;
        command.musicAfter = after;
        commandStack_.Push(std::move(command));
        MarkMutated();
        return true;
    }

    MoveObjectRecord WorldEditor::WithEndAt(const MoveObjectRecord& record, std::uint16_t x, std::uint16_t y,
                                            std::uint16_t z) const
    {
        MoveObjectRecord after = record;
        after.posEndX = static_cast<float>(x);
        after.posEndY = static_cast<float>(y);
        after.posEndZ = static_cast<float>(z);
        // A creature's patrol takes its type's own speed and rests, as the original editor
        // gave them (BD-0016); the fields stay editable afterwards.
        const auto& definition = Game::GetObjectDefinition(after.type);
        if (IsLiftType(after.type))
        {
            // The original multiplies the lift's per-cell step by the cells to its end
            // (PlaceDecor4: stepAdvance *= dist); here the step per cell is the record's over
            // the cells it covered before, so the end can be set again.
            const auto cellsOf = [](const MoveObjectRecord& r) {
                return std::max({1.0f, std::fabs(r.posEndX - r.posStartX), std::fabs(r.posEndY - r.posStartY),
                                 std::fabs(r.posEndZ - r.posStartZ)});
            };
            const float before = cellsOf(record);
            const float now = cellsOf(after);
            after.stepAdvanceTicks = record.stepAdvanceTicks / before * now;
            after.stepRecedeTicks = record.stepRecedeTicks / before * now;
        }
        else if (definition.patrolTicksPerCell > 0)
        {
            const float cells = std::max({std::fabs(after.posEndX - after.posStartX),
                                          std::fabs(after.posEndY - after.posStartY),
                                          std::fabs(after.posEndZ - after.posStartZ)});
            after.stepAdvanceTicks = after.stepRecedeTicks = cells * static_cast<float>(definition.patrolTicksPerCell);
            after.timeStopStartTicks = after.timeStopEndTicks = static_cast<float>(definition.patrolRestTicks);
        }
        return after;
    }

    namespace
    {
        // The icon after `icon` in its variant family, else in its auto-tile family (the distinct
        // icons of its kAdaptDecor row, in their order), else none.
        std::optional<std::uint16_t> NextVariant(std::uint16_t icon)
        {
            const int family = FindVariantFamily(icon);
            if (family >= 0)
            {
                const auto& icons = VariantFamilies()[static_cast<std::size_t>(family)].icons;
                for (std::size_t i = 0; i < icons.size(); ++i)
                {
                    if (icons[i] == icon)
                    {
                        return icons[(i + 1) % icons.size()];
                    }
                }
            }
            for (std::size_t row = 0; row < AutoTile::kAdaptDecor.size() / 16; ++row)
            {
                std::vector<std::uint16_t> distinct;
                for (std::size_t i = 0; i < 16; ++i)
                {
                    const auto v = static_cast<std::uint16_t>(AutoTile::kAdaptDecor[row * 16 + i]);
                    if (std::find(distinct.begin(), distinct.end(), v) == distinct.end())
                    {
                        distinct.push_back(v);
                    }
                }
                for (std::size_t i = 0; i < distinct.size(); ++i)
                {
                    if (distinct[i] == icon && distinct.size() > 1)
                    {
                        return distinct[(i + 1) % distinct.size()];
                    }
                }
            }
            return std::nullopt;
        }
    }

    bool WorldEditor::HandleVariant(const FrameInput& input, Worlds::World& world)
    {
        if (!input.variantHeld || variantKeyHeldLastFrame_ || !hasRaycastHit_)
        {
            return false;
        }
        const Worlds::Block before = world.getBlock(hitCellX_, hitCellY_, hitCellZ_);
        const auto next = before.isAir() ? std::nullopt : NextVariant(before.type());
        if (!next)
        {
            return true;
        }
        const Worlds::Block after = Worlds::Block::make(*next);
        world.setBlock(hitCellX_, hitCellY_, hitCellZ_, after);
        EditCommand command;
        command.kind = EditCommand::Kind::BlockEdit;
        command.blockChanges.push_back({hitCellX_, hitCellY_, hitCellZ_, before, after});
        commandStack_.Push(std::move(command));
        variants_.Forget();
        MarkMutated();
        return true;
    }

    std::vector<std::string> WorldEditor::StatusLines() const
    {
        std::vector<std::string> lines;
        if (pendingEnd_)
        {
            lines.emplace_back("Click where it goes (its end). ESC or a click on it: it stays.");
        }
        if (boxFirstCornerPlaced_)
        {
            lines.emplace_back(boxForCopy_ ? "C again on the other corner: copied. ESC: cancel"
                                           : "F again on the other corner: filled. ESC: cancel");
        }
        if (clipboard_)
        {
            std::string size = std::to_string(clipboard_->sizeX) + "x" + std::to_string(clipboard_->sizeY) + "x" +
                               std::to_string(clipboard_->sizeZ);
            if (clipboard_->quarterTurns != 0)
            {
                size += ", turned " + std::to_string(90 * clipboard_->quarterTurns);
            }
            if (clipboard_->mirrored)
            {
                size += ", mirrored";
            }
            if (pasting_)
            {
                lines.emplace_back("PASTING " + size + ": a click stamps it at the red preview");
                lines.emplace_back("CTRL+click: keeps what is under its air  Q: turn  X: mirror  P/ESC: stop");
            }
            else
            {
                lines.emplace_back("Copied " + size + ". P: paste it");
            }
        }
        if (helpVisible_)
        {
            lines.emplace_back("LEFT CLICK/PLACE: place   MIDDLE CLICK: remove");
            lines.emplace_back("Click the block just placed again: its next variant");
            lines.emplace_back("V: next variant of the aimed block   CTRL+click: no cycling");
            lines.emplace_back("O: turn the aimed tile, model, object or decor");
            lines.emplace_back("Lift or creature: place it, then click where it goes");
            lines.emplace_back("G: pick object  T: its end  TAB +/-: its values  DEL: remove");
            lines.emplace_back("F: box fill  ESC: cancel  U/R: undo/redo  ENTER: save");
            lines.emplace_back("C C: copy a box (two corners)  P: paste it  Q: turn  X: mirror");
            lines.emplace_back("M: music  ARROWS: background  RMB: look  WASD SPACE CTRL: fly");
            lines.emplace_back("NUMPAD: move the red preview   H: hide the keys");
        }
        else if (!pendingEnd_)
        {
            lines.emplace_back("H: keys");
        }
        return lines;
    }

    bool WorldEditor::HandleTurn(const FrameInput& input, Worlds::World& world)
    {
        if (!input.turnHeld || turnKeyHeldLastFrame_ || !hasRaycastHit_)
        {
            return false;
        }
        // An object or a BigDecor in the cell the preview shows (where a click would put one)
        // turns a quarter at a time (OR-0071): its cell's quarter turns.
        if (hasHighlight_ && (FindMoveObjectAt(world, placementCellX_, placementCellY_, placementCellZ_) ||
                              FindBigDecorAt(world, placementCellX_, placementCellY_, placementCellZ_)))
        {
            const int before = Game::ObjectQuarterTurns(world, placementCellX_, placementCellY_, placementCellZ_);
            const int after = (before + 1) % 4;
            Game::SetObjectQuarterTurns(world, placementCellX_, placementCellY_, placementCellZ_, after);
            EditCommand command;
            command.kind = EditCommand::Kind::TurnEdit;
            command.turnX = placementCellX_;
            command.turnY = placementCellY_;
            command.turnZ = placementCellZ_;
            command.turnBefore = before;
            command.turnAfter = after;
            command.turnObject = true;
            commandStack_.Push(std::move(command));
            MarkMutated();
            return true;
        }
        if (world.getBlock(hitCellX_, hitCellY_, hitCellZ_).isAir())
        {
            return true;
        }
        // A model with a front turns a quarter at a time (OR-0071); a plate, fan or wind tile
        // between its two axes (M3D-0005).
        const std::uint16_t icon = world.getBlock(hitCellX_, hitCellY_, hitCellZ_).type();
        const bool front = std::find(Game::kFrontModelIcons.begin(), Game::kFrontModelIcons.end(), icon) !=
                               Game::kFrontModelIcons.end() ||
                           Game::IsSignModelIcon(icon);
        // A front model's turns are kept as an object's (type 5): the plate bit (type 2) of signs
        // facing X, which the hub generators set, is no turn of the model.
        const int before = front ? Game::ObjectQuarterTurns(world, hitCellX_, hitCellY_, hitCellZ_)
                                 : Game::QuarterTurns(world, hitCellX_, hitCellY_, hitCellZ_);
        const int after = front ? (before + 1) % 4 : (before & 1) ^ 1;
        if (front)
        {
            Game::SetObjectQuarterTurns(world, hitCellX_, hitCellY_, hitCellZ_, after);
        }
        else
        {
            Game::SetQuarterTurns(world, hitCellX_, hitCellY_, hitCellZ_, after);
        }
        EditCommand command;
        command.kind = EditCommand::Kind::TurnEdit;
        command.turnX = hitCellX_;
        command.turnY = hitCellY_;
        command.turnZ = hitCellZ_;
        command.turnBefore = before;
        command.turnAfter = after;
        command.turnObject = front;
        commandStack_.Push(std::move(command));
        MarkMutated();
        return true;
    }

    bool WorldEditor::HandleSkyRegion(const FrameInput& input, Worlds::World& world)
    {
        constexpr std::uint32_t kRegionCount = 32;
        const std::uint32_t before = world.skyRegion();
        if (input.palette.action == EditorPalette::Action::SelectSkyRegion)
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
            EditCommand command;
            command.kind = EditCommand::Kind::SkyRegionEdit;
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
        EditCommand command;
        command.kind = EditCommand::Kind::SkyRegionEdit;
        command.skyRegionBefore = before;
        command.skyRegionAfter = after;
        commandStack_.Push(std::move(command));
        MarkMutated();
        return true;
    }

    bool WorldEditor::HandleObjectEditing(
        const FrameInput& input, CnaEngine::Graphics::Camera& camera,
        int viewportWidth, int viewportHeight, Worlds::World& world)
    {
        if (input.selectHeld && !selectKeyHeldLastFrame_)
        {
            hasSelectedObject_ = false;
            float bestDistance = kObjectPickRadius * kObjectPickRadius;
            for (const auto& record : CollectMoveObjects(world))
            {
                const Microsoft::Xna::Framework::Vector3 renderPosition(
                    record.posStartX - static_cast<float>(GalaxyEggbert::Game::WorldRuntime::kWorldCenterX),
                    record.posStartY,
                    record.posStartZ - static_cast<float>(GalaxyEggbert::Game::WorldRuntime::kWorldCenterZ));
                float projectedX = 0.0f;
                float projectedY = 0.0f;
                if (!GalaxyEggbert::Game::Hud::ProjectWorldToHudSpace(
                        renderPosition, camera.View(), camera.Projection(),
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
            const MoveObjectRecord after = WithEndAt(selectedObject_, hitCellX_, hitCellY_, hitCellZ_);
            EditCommand command;
            command.kind = EditCommand::Kind::MoveObjectEdit;
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
                EditCommand command;
                command.kind = EditCommand::Kind::MoveObjectEdit;
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

    void WorldEditor::StoreInputEdges(
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
        musicKeyHeldLastFrame_ = input.musicNextHeld;
        turnKeyHeldLastFrame_ = input.turnHeld;
        variantKeyHeldLastFrame_ = input.variantHeld;
        copyKeyHeldLastFrame_ = input.copyHeld;
        pasteKeyHeldLastFrame_ = input.pasteHeld;
        clipTurnKeyHeldLastFrame_ = input.clipTurnHeld;
        clipMirrorKeyHeldLastFrame_ = input.clipMirrorHeld;
        if (input.helpHeld && !helpKeyHeldLastFrame_)
        {
            helpVisible_ = !helpVisible_;
        }
        helpKeyHeldLastFrame_ = input.helpHeld;
        placementOffsetKeyHeldLastFrame_ = placementOffsetKeyHeld;
    }
}
