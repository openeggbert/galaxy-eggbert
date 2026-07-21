#include "GEWorldEditor.hpp"

#include "GEBoxRegion.hpp"
#include "GEVoxelRaycast.hpp"
#include "Game/GEHud.hpp"
#include "Game/GEWorldRuntime.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        constexpr float kPitchLimit = 1.55334f; // ~89 degrees -- avoids the gimbal flip straight up/down
        constexpr float kMouseLookRadiansPerPixel = 0.0025f;
        constexpr float kMinFlySpeed = 1.0f;
        constexpr float kMaxFlySpeed = 200.0f;
        constexpr float kScrollSpeedStepPerNotch = 1.15f; // multiplicative -- stays useful close-up and world-spanning
        constexpr float kMaxRaycastDistance = 200.0f; // > the 100^3 world's ~173-unit diagonal

        // MoveObject select tool (plan.md EDITOR-110). kHudCenterX/Y is
        // screen center in GEHud::ProjectWorldToHudSpace()'s own private
        // 640x480 reference space (GEHud.cpp's kRefW/kRefH) -- G selects
        // whichever placed object projects nearest this point, within
        // kObjectPickRadius reference-space pixels.
        constexpr float kHudCenterX = 320.0f;
        constexpr float kHudCenterY = 240.0f;
        constexpr float kObjectPickRadius = 80.0f;
        // Editor-UX step sizes (not transcribed real constants -- same
        // category as MoveObjectRecord's own placeholder timing defaults).
        constexpr float kMinObjectSpeed = 0.25f;
        constexpr float kObjectSpeedStep = 0.25f;
        constexpr float kObjectTicksStep = 5.0f; // 0.25s at the real 20Hz reference rate

        // Whichever MoveObjectRecord is anchored at raw-grid cell (x,y,z),
        // if any -- captured as a MoveObjectEdit's "before" state so undo
        // can restore an object that a placement overwrote (PlaceMoveObject
        // replaces silently when two records share an anchor cell, see its
        // own header comment). CollectMoveObjects() reports records without
        // their anchor, so this re-derives it the same way PlaceMoveObject
        // does: floor(posStart).
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
        const Microsoft::Xna::Framework::Input::MouseState& mouse, int viewportWidth, int viewportHeight)
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

    void GEWorldEditor::DrawBrowsing(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                     int viewportWidth, int viewportHeight)
    {
        browserScreen_.Draw(device, viewportWidth, viewportHeight);
    }

    void GEWorldEditor::EnterEditing(float startX, float startY, float startZ) noexcept
    {
        browsing_ = false; // defensive: correct even if a caller skips the explicit ExitBrowser() call
        camX_ = startX;
        camY_ = startY;
        camZ_ = startZ;
        yaw_ = 0.0f;
        pitch_ = -0.35f;
        mouseLookHeldLastFrame_ = false;
        hasLastScrollWheelValue_ = false;
        hasHighlight_ = false;
        highlightRenderer_.Hide();
        commandStack_ = GEEditCommandStack();
        boxFirstCornerPlaced_ = false;
        showingBox_ = false;
        playTestRequested_ = false;
        hasSelectedObject_ = false;
        selectionHighlightRenderer_.Hide();
    }

    void GEWorldEditor::Update(const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
                               const Microsoft::Xna::Framework::Input::MouseState& mouse,
                               float dt, int viewportWidth, int viewportHeight,
                               Easy3D::Camera3D& camera,
                               Worlds::World& world)
    {
        using Microsoft::Xna::Framework::Input::ButtonState;
        using Microsoft::Xna::Framework::Input::Keys;
        using Microsoft::Xna::Framework::Input::Mouse;

        const bool rightHeld = mouse.getRightButtonProperty() == ButtonState::Pressed;
        if (rightHeld && !mouseLookHeldLastFrame_)
        {
            // Just started holding -- capture the cursor and switch to
            // relative-delta mouse reporting. The X/Y already read into
            // `mouse` above were captured before relative mode took
            // effect, so deliberately skip applying a look delta THIS
            // frame; every frame after this one reports true per-frame
            // deltas instead of absolute position.
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
            pitch_ -= static_cast<float>(mouse.getYProperty()) * kMouseLookRadiansPerPixel;
            pitch_ = std::clamp(pitch_, -kPitchLimit, kPitchLimit);
        }
        mouseLookHeldLastFrame_ = rightHeld;

        // Scroll wheel adjusts fly speed (multiplicative per notch, so it
        // stays useful at both close-up and whole-world-spanning
        // distances). getScrollWheelValueProperty() is CUMULATIVE since
        // startup, not per-frame -- the first reading only establishes a
        // baseline, it never adjusts speed itself.
        const int scrollWheelValue = mouse.getScrollWheelValueProperty();
        if (hasLastScrollWheelValue_)
        {
            const int deltaNotches = (scrollWheelValue - lastScrollWheelValue_) / 120;
            if (deltaNotches != 0)
            {
                flySpeed_ *= std::pow(kScrollSpeedStepPerNotch, static_cast<float>(deltaNotches));
                flySpeed_ = std::clamp(flySpeed_, kMinFlySpeed, kMaxFlySpeed);
            }
        }
        lastScrollWheelValue_ = scrollWheelValue;
        hasLastScrollWheelValue_ = true;

        // Forward/right derived from yaw/pitch, matching GEBlupiController's
        // own horizontal-plane convention exactly (forward = (sin(yaw),
        // -cos(yaw)) in XZ, yaw 0 = facing -Z) so this feels consistent
        // with the rest of the game's camera/movement code.
        const float cosPitch = std::cos(pitch_);
        const Easy3D::Camera3D::Vector3 forward(
            std::sin(yaw_) * cosPitch, std::sin(pitch_), -std::cos(yaw_) * cosPitch);
        const Easy3D::Camera3D::Vector3 right(std::cos(yaw_), 0.0f, std::sin(yaw_));
        const Easy3D::Camera3D::Vector3 worldUp(0.0f, 1.0f, 0.0f);

        float moveX = 0.0f;
        float moveY = 0.0f;
        float moveZ = 0.0f;
        const auto move = [&moveX, &moveY, &moveZ](const Easy3D::Camera3D::Vector3& axis, float amount)
        {
            moveX += axis.X * amount;
            moveY += axis.Y * amount;
            moveZ += axis.Z * amount;
        };
        if (keyboard.IsKeyDown(Keys::W)) move(forward, 1.0f);
        if (keyboard.IsKeyDown(Keys::S)) move(forward, -1.0f);
        if (keyboard.IsKeyDown(Keys::D)) move(right, 1.0f);
        if (keyboard.IsKeyDown(Keys::A)) move(right, -1.0f);
        if (keyboard.IsKeyDown(Keys::Space)) move(worldUp, 1.0f);
        if (keyboard.IsKeyDown(Keys::LeftControl)) move(worldUp, -1.0f);

        camX_ += moveX * flySpeed_ * dt;
        camY_ += moveY * flySpeed_ * dt;
        camZ_ += moveZ * flySpeed_ * dt;

        camera.SetPosition(Easy3D::Camera3D::Vector3(camX_, camY_, camZ_));
        camera.SetTarget(Easy3D::Camera3D::Vector3(
            camX_ + forward.X, camY_ + forward.Y, camZ_ + forward.Z));
        camera.SetUp(worldUp);

        // Voxel raycast (plan.md EDITOR-102) -- camera position/direction
        // are in RENDER space (shifted by -kWorldCenterX/Z from @p world's
        // own raw grid space, see GEVoxelRaycast.hpp); only X/Z need the
        // shift reversed, Y is unshifted in both spaces.
        const RaycastHit hit = Raycast(world,
                                        camX_ + static_cast<float>(GEWorldRuntime::kWorldCenterX), camY_,
                                        camZ_ + static_cast<float>(GEWorldRuntime::kWorldCenterZ),
                                        forward.X, forward.Y, forward.Z,
                                        kMaxRaycastDistance);
        hasHighlight_ = hit.hit;
        if (hasHighlight_)
        {
            highlightX_ = static_cast<float>(hit.x) - static_cast<float>(GEWorldRuntime::kWorldCenterX);
            highlightY_ = static_cast<float>(hit.y);
            highlightZ_ = static_cast<float>(hit.z) - static_cast<float>(GEWorldRuntime::kWorldCenterZ);
            hitCellX_ = hit.x;
            hitCellY_ = hit.y;
            hitCellZ_ = hit.z;
            hitNormalX_ = hit.normalX;
            hitNormalY_ = hit.normalY;
            hitNormalZ_ = hit.normalZ;
        }

        // Editing tools (plan.md EDITOR-103) -- edge-triggered on the
        // press, matching every existing click-handler in this codebase
        // (e.g. GEInputPad's "!mouseDown && mouseWasDown_" idiom, just
        // inverted here to trigger on press rather than release since
        // there's no on-screen button geometry to still be hovering over).
        // Palette/toolbar click handling (plan.md EDITOR-106) -- checked
        // BEFORE the 3D-world left-click place logic below, same
        // "inputPadClaimedMouse" idiom GalaxyEggbertCnaGame.cpp's own Play
        // on-screen D-pad already uses, so a palette icon/toolbar click
        // doesn't ALSO place a block at the crosshair this same frame.
        const GEEditorPalette::UpdateResult paletteResult = palette_.Update(mouse, viewportWidth, viewportHeight);

        const bool leftHeld = mouse.getLeftButtonProperty() == ButtonState::Pressed;
        const bool middleHeld = mouse.getMiddleButtonProperty() == ButtonState::Pressed;
        const bool enterHeld = keyboard.IsKeyDown(Keys::Enter);
        const bool undoKeyHeld = keyboard.IsKeyDown(Keys::U);
        const bool redoKeyHeld = keyboard.IsKeyDown(Keys::R);
        const bool boxKeyHeld = keyboard.IsKeyDown(Keys::F);
        const bool escapeKeyHeld = keyboard.IsKeyDown(Keys::Escape);
        const bool selectKeyHeld = keyboard.IsKeyDown(Keys::G);
        const bool targetKeyHeld = keyboard.IsKeyDown(Keys::T);
        const bool cycleFieldKeyHeld = keyboard.IsKeyDown(Keys::Tab);
        const bool increaseFieldKeyHeld = keyboard.IsKeyDown(Keys::OemPlus);
        const bool decreaseFieldKeyHeld = keyboard.IsKeyDown(Keys::OemMinus);
        const bool deleteObjectKeyHeld = keyboard.IsKeyDown(Keys::Delete);
        const bool skyRegionPrevKeyHeld = keyboard.IsKeyDown(Keys::Left);
        const bool skyRegionNextKeyHeld = keyboard.IsKeyDown(Keys::Right);

        // Box-fill tool live tracking (plan.md EDITOR-105) -- while a first
        // corner is placed, the highlight follows a box between it and
        // wherever the raycast currently aims, updated every frame
        // regardless of which (if any) trigger key is pressed this frame.
        showingBox_ = boxFirstCornerPlaced_ && hasHighlight_;
        if (showingBox_)
        {
            const BoxRegion liveRegion = NormalizeAndClamp(
                boxCorner0X_, boxCorner0Y_, boxCorner0Z_,
                hitCellX_, hitCellY_, hitCellZ_,
                static_cast<int>(world.blocksPerAxis()));
            boxMinRenderX_ = static_cast<float>(liveRegion.minX) - static_cast<float>(GEWorldRuntime::kWorldCenterX);
            boxMinRenderY_ = static_cast<float>(liveRegion.minY);
            boxMinRenderZ_ = static_cast<float>(liveRegion.minZ) - static_cast<float>(GEWorldRuntime::kWorldCenterZ);
            boxMaxRenderX_ = static_cast<float>(liveRegion.maxX) - static_cast<float>(GEWorldRuntime::kWorldCenterX);
            boxMaxRenderY_ = static_cast<float>(liveRegion.maxY);
            boxMaxRenderZ_ = static_cast<float>(liveRegion.maxZ) - static_cast<float>(GEWorldRuntime::kWorldCenterZ);
        }

        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        if (leftHeld && !leftHeldLastFrame_ && !paletteResult.clickConsumed && hasHighlight_ &&
            !boxFirstCornerPlaced_)
        {
            const int placeX = static_cast<int>(hitCellX_) + hitNormalX_;
            const int placeY = static_cast<int>(hitCellY_) + hitNormalY_;
            const int placeZ = static_cast<int>(hitCellZ_) + hitNormalZ_;
            if (placeX >= 0 && placeX < blocksPerAxis &&
                placeY >= 0 && placeY < blocksPerAxis &&
                placeZ >= 0 && placeZ < blocksPerAxis)
            {
                const auto px = static_cast<std::uint16_t>(placeX);
                const auto py = static_cast<std::uint16_t>(placeY);
                const auto pz = static_cast<std::uint16_t>(placeZ);
                GEEditCommand command;
                if (palette_.IsObjectMode())
                {
                    // Stationary placement (plan.md EDITOR-109): posEnd ==
                    // posStart is the real "this object doesn't move"
                    // guard (MoveObjectRecord.hpp). Giving it a real patrol
                    // path is EDITOR-110's job.
                    MoveObjectRecord record;
                    record.type = palette_.SelectedObjectType();
                    record.posStartX = static_cast<float>(px);
                    record.posStartY = static_cast<float>(py);
                    record.posStartZ = static_cast<float>(pz);
                    record.posEndX = record.posStartX;
                    record.posEndY = record.posStartY;
                    record.posEndZ = record.posStartZ;

                    command.kind = GEEditCommand::Kind::MoveObjectEdit;
                    command.objectAnchorX = px;
                    command.objectAnchorY = py;
                    command.objectAnchorZ = pz;
                    command.objectBefore = FindMoveObjectAnchoredAt(world, px, py, pz);
                    command.objectAfter = record;
                    PlaceMoveObject(world, record);
                }
                else
                {
                    const Worlds::Block before = world.getBlock(px, py, pz);
                    const Worlds::Block after = Worlds::Block::make(palette_.SelectedBlockType());
                    world.setBlock(px, py, pz, after);
                    command.kind = GEEditCommand::Kind::BlockEdit;
                    command.blockChanges.push_back({px, py, pz, before, after});
                }
                commandStack_.Push(std::move(command));
                needsPresentationRebuild_ = true;
            }
        }
        else if (middleHeld && !middleHeldLastFrame_ && !paletteResult.clickConsumed && hasHighlight_ &&
                 !boxFirstCornerPlaced_)
        {
            const Worlds::Block before = world.getBlock(hitCellX_, hitCellY_, hitCellZ_);
            const Worlds::Block after = Worlds::Block::air();
            world.setBlock(hitCellX_, hitCellY_, hitCellZ_, after);
            GEEditCommand command;
            command.kind = GEEditCommand::Kind::BlockEdit;
            command.blockChanges.push_back({hitCellX_, hitCellY_, hitCellZ_, before, after});
            commandStack_.Push(std::move(command));
            needsPresentationRebuild_ = true;
        }
        else if ((enterHeld && !enterHeldLastFrame_ && !worldPath_.empty()) ||
                 paletteResult.action == GEEditorPalette::ToolbarAction::Save)
        {
            if (!worldPath_.empty())
            {
                world.saveToFile(worldPath_);
            }
        }
        else if ((undoKeyHeld && !undoKeyHeldLastFrame_) ||
                 paletteResult.action == GEEditorPalette::ToolbarAction::Undo)
        {
            if (commandStack_.Undo(world))
            {
                needsPresentationRebuild_ = true;
                RefreshSelectedObjectAfterHistoryChange(world);
            }
        }
        else if ((redoKeyHeld && !redoKeyHeldLastFrame_) ||
                 paletteResult.action == GEEditorPalette::ToolbarAction::Redo)
        {
            if (commandStack_.Redo(world))
            {
                needsPresentationRebuild_ = true;
                RefreshSelectedObjectAfterHistoryChange(world);
            }
        }
        else if (paletteResult.action == GEEditorPalette::ToolbarAction::Back)
        {
            // Re-enters the browser for the SAME gamer slot (plan.md
            // EDITOR-107) -- IsBrowsing() reports true starting next
            // frame's dispatch. Deliberately does NOT auto-save; a player
            // who wants to keep changes presses Save/Enter first (an
            // "unsaved changes?" guard is a later hardening-pass item, not
            // a functional gap in this milestone).
            EnterBrowser(gamerSlot_);
        }
        else if (paletteResult.action == GEEditorPalette::ToolbarAction::PlayTest && !worldPath_.empty())
        {
            // Save first (plan.md EDITOR-108) so the play-tested session
            // matches exactly what's on screen, then request a real
            // gameplay session -- GalaxyEggbertCnaGame still owns actually
            // switching phase/reloading (this class has no GEWorldRuntime
            // access).
            world.saveToFile(worldPath_);
            playTestRequested_ = true;
        }
        else if (((boxKeyHeld && !boxKeyHeldLastFrame_) ||
                  paletteResult.action == GEEditorPalette::ToolbarAction::BoxFill) &&
                 hasHighlight_)
        {
            if (!boxFirstCornerPlaced_)
            {
                boxCorner0X_ = hitCellX_;
                boxCorner0Y_ = hitCellY_;
                boxCorner0Z_ = hitCellZ_;
                boxFirstCornerPlaced_ = true;
            }
            else
            {
                const BoxRegion region = NormalizeAndClamp(
                    boxCorner0X_, boxCorner0Y_, boxCorner0Z_,
                    hitCellX_, hitCellY_, hitCellZ_, blocksPerAxis);
                GEEditCommand command;
                command.kind = GEEditCommand::Kind::BlockEdit;
                const Worlds::Block fillBlock = Worlds::Block::make(palette_.SelectedBlockType());
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
                            if (before == fillBlock)
                            {
                                continue; // no real change -- don't record a no-op undo entry
                            }
                            world.setBlock(ux, uy, uz, fillBlock);
                            command.blockChanges.push_back({ux, uy, uz, before, fillBlock});
                        }
                    }
                }
                if (!command.blockChanges.empty())
                {
                    commandStack_.Push(std::move(command));
                    needsPresentationRebuild_ = true;
                }
                boxFirstCornerPlaced_ = false;
                showingBox_ = false;
            }
        }
        else if (escapeKeyHeld && !escapeKeyHeldLastFrame_ && boxFirstCornerPlaced_)
        {
            boxFirstCornerPlaced_ = false;
            showingBox_ = false;
        }
        else if ((skyRegionPrevKeyHeld && !skyRegionPrevKeyHeldLastFrame_) ||
                 paletteResult.action == GEEditorPalette::ToolbarAction::SkyRegionPrev)
        {
            constexpr std::uint32_t kSkyRegionCount = 32; // world.hpp's own documented valid range, 0-31
            const std::uint32_t before = world.skyRegion();
            const std::uint32_t after = (before + kSkyRegionCount - 1) % kSkyRegionCount;
            world.setSkyRegion(after);
            GEEditCommand command;
            command.kind = GEEditCommand::Kind::SkyRegionEdit;
            command.skyRegionBefore = before;
            command.skyRegionAfter = after;
            commandStack_.Push(std::move(command));
            needsPresentationRebuild_ = true;
        }
        else if ((skyRegionNextKeyHeld && !skyRegionNextKeyHeldLastFrame_) ||
                 paletteResult.action == GEEditorPalette::ToolbarAction::SkyRegionNext)
        {
            constexpr std::uint32_t kSkyRegionCount = 32;
            const std::uint32_t before = world.skyRegion();
            const std::uint32_t after = (before + 1) % kSkyRegionCount;
            world.setSkyRegion(after);
            GEEditCommand command;
            command.kind = GEEditCommand::Kind::SkyRegionEdit;
            command.skyRegionBefore = before;
            command.skyRegionAfter = after;
            commandStack_.Push(std::move(command));
            needsPresentationRebuild_ = true;
        }
        else if (selectKeyHeld && !selectKeyHeldLastFrame_)
        {
            // G: pick whichever placed MoveObject's billboard projects
            // closest to screen center (plan.md EDITOR-110) -- see this
            // class's own Update() doc comment. Reselecting with nothing
            // within the pick radius clears the current selection.
            hasSelectedObject_ = false;
            float bestDistSq = kObjectPickRadius * kObjectPickRadius;
            for (const auto& record : CollectMoveObjects(world))
            {
                const Easy3D::Camera3D::Vector3 renderPos(
                    record.posStartX - static_cast<float>(GEWorldRuntime::kWorldCenterX),
                    record.posStartY,
                    record.posStartZ - static_cast<float>(GEWorldRuntime::kWorldCenterZ));
                float projX = 0.0f;
                float projY = 0.0f;
                if (!GEHud::ProjectWorldToHudSpace(renderPos, camera.GetViewMatrix(), camera.GetProjectionMatrix(),
                                                    viewportWidth, viewportHeight, projX, projY))
                {
                    continue; // behind the camera -- not a real pick candidate
                }
                const float dx = projX - kHudCenterX;
                const float dy = projY - kHudCenterY;
                const float distSq = dx * dx + dy * dy;
                if (distSq < bestDistSq)
                {
                    bestDistSq = distSq;
                    hasSelectedObject_ = true;
                    selectedObject_ = record;
                    selectedAnchorX_ = static_cast<std::uint16_t>(std::floor(record.posStartX));
                    selectedAnchorY_ = static_cast<std::uint16_t>(std::floor(record.posStartY));
                    selectedAnchorZ_ = static_cast<std::uint16_t>(std::floor(record.posStartZ));
                    selectedRenderX_ = renderPos.X;
                    selectedRenderY_ = renderPos.Y;
                    selectedRenderZ_ = renderPos.Z;
                }
            }
        }
        else if (targetKeyHeld && !targetKeyHeldLastFrame_ && hasSelectedObject_ && hasHighlight_)
        {
            // T: give the selected object a real patrol path by setting its
            // posEnd to wherever the crosshair currently aims. posStart (and
            // so the anchor cell) is untouched, so this always overwrites
            // the same anchor PlaceMoveObject already replaced it at.
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
            needsPresentationRebuild_ = true;
        }
        else if (cycleFieldKeyHeld && !cycleFieldKeyHeldLastFrame_ && hasSelectedObject_)
        {
            activeField_ = static_cast<EditableField>(
                (static_cast<int>(activeField_) + 1) % kEditableFieldCount);
        }
        else if (((increaseFieldKeyHeld && !increaseFieldKeyHeldLastFrame_) ||
                  (decreaseFieldKeyHeld && !decreaseFieldKeyHeldLastFrame_)) &&
                 hasSelectedObject_)
        {
            const float sign = (increaseFieldKeyHeld && !increaseFieldKeyHeldLastFrame_) ? 1.0f : -1.0f;
            const std::optional<MoveObjectRecord> after = ApplyActiveFieldDelta(selectedObject_, sign);
            if (after.has_value())
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
                needsPresentationRebuild_ = true;
            }
        }
        else if (deleteObjectKeyHeld && !deleteObjectKeyHeldLastFrame_ && hasSelectedObject_)
        {
            GEEditCommand command;
            command.kind = GEEditCommand::Kind::MoveObjectEdit;
            command.objectAnchorX = selectedAnchorX_;
            command.objectAnchorY = selectedAnchorY_;
            command.objectAnchorZ = selectedAnchorZ_;
            command.objectBefore = selectedObject_;
            command.objectAfter = std::nullopt;
            RemoveMoveObject(world, selectedAnchorX_, selectedAnchorY_, selectedAnchorZ_);
            commandStack_.Push(std::move(command));
            hasSelectedObject_ = false;
            needsPresentationRebuild_ = true;
        }
        leftHeldLastFrame_ = leftHeld;
        middleHeldLastFrame_ = middleHeld;
        enterHeldLastFrame_ = enterHeld;
        undoKeyHeldLastFrame_ = undoKeyHeld;
        redoKeyHeldLastFrame_ = redoKeyHeld;
        boxKeyHeldLastFrame_ = boxKeyHeld;
        escapeKeyHeldLastFrame_ = escapeKeyHeld;
        selectKeyHeldLastFrame_ = selectKeyHeld;
        targetKeyHeldLastFrame_ = targetKeyHeld;
        cycleFieldKeyHeldLastFrame_ = cycleFieldKeyHeld;
        increaseFieldKeyHeldLastFrame_ = increaseFieldKeyHeld;
        decreaseFieldKeyHeldLastFrame_ = decreaseFieldKeyHeld;
        deleteObjectKeyHeldLastFrame_ = deleteObjectKeyHeld;
        skyRegionPrevKeyHeldLastFrame_ = skyRegionPrevKeyHeld;
        skyRegionNextKeyHeldLastFrame_ = skyRegionNextKeyHeld;
    }

    std::optional<MoveObjectRecord> GEWorldEditor::ApplyActiveFieldDelta(
        const MoveObjectRecord& record, float sign) const noexcept
    {
        MoveObjectRecord updated = record;
        switch (activeField_)
        {
            case EditableField::Speed:
            {
                const float value = std::max(kMinObjectSpeed, record.speed + sign * kObjectSpeedStep);
                if (value == record.speed) return std::nullopt;
                updated.speed = value;
                break;
            }
            case EditableField::StepAdvanceTicks:
            {
                const float value = std::max(0.0f, record.stepAdvanceTicks + sign * kObjectTicksStep);
                if (value == record.stepAdvanceTicks) return std::nullopt;
                updated.stepAdvanceTicks = value;
                break;
            }
            case EditableField::StepRecedeTicks:
            {
                const float value = std::max(0.0f, record.stepRecedeTicks + sign * kObjectTicksStep);
                if (value == record.stepRecedeTicks) return std::nullopt;
                updated.stepRecedeTicks = value;
                break;
            }
            case EditableField::TimeStopStartTicks:
            {
                const float value = std::max(0.0f, record.timeStopStartTicks + sign * kObjectTicksStep);
                if (value == record.timeStopStartTicks) return std::nullopt;
                updated.timeStopStartTicks = value;
                break;
            }
            case EditableField::TimeStopEndTicks:
            {
                const float value = std::max(0.0f, record.timeStopEndTicks + sign * kObjectTicksStep);
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
        const auto refreshed = FindMoveObjectAnchoredAt(world, selectedAnchorX_, selectedAnchorY_, selectedAnchorZ_);
        if (refreshed.has_value())
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

    void GEWorldEditor::Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                             const Easy3D::Camera3D& camera,
                             Microsoft::Xna::Framework::Graphics::Texture2D& terrainTexture,
                             Microsoft::Xna::Framework::Graphics::Texture2D& elementTexture,
                             Microsoft::Xna::Framework::Graphics::Texture2D& exploTexture,
                             Microsoft::Xna::Framework::Graphics::Texture2D& blupiTexture,
                             Microsoft::Xna::Framework::Graphics::Texture2D& blupi1Texture,
                             int viewportWidth, int viewportHeight)
    {
        if (showingBox_)
        {
            highlightRenderer_.ShowBox(device, boxMinRenderX_, boxMinRenderY_, boxMinRenderZ_,
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
            selectionHighlightRenderer_.ShowSelectedObject(device, selectedRenderX_, selectedRenderY_,
                                                            selectedRenderZ_);
            selectionHighlightRenderer_.Draw(device, camera);
        }
        else
        {
            selectionHighlightRenderer_.Hide();
        }

        palette_.Draw(device, terrainTexture, elementTexture, exploTexture, blupiTexture, blupi1Texture,
                      viewportWidth, viewportHeight);
    }
}
