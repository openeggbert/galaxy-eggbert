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
        constexpr float kEditorDefaultFov = 0.785398163f; // 45 degrees
        constexpr float kEditorMinFov = 0.261799388f; // 15 degrees, zoomed in
        constexpr float kEditorMaxFov = 1.3962634f; // 80 degrees, zoomed out
        constexpr float kScrollZoomStepPerNotch = 0.90f;
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
        dirty_ = false; // EDITOR-112: a freshly loaded/created world starts clean
        backConfirmArmed_ = false;
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

        // Free Eggbert uses the wheel as a view zoom. The 3D editor has no
        // 2D camera scale, so the equivalent is field of view: wheel-up
        // narrows it (zoom in), wheel-down widens it (zoom out). The value
        // is cumulative, hence the first reading only establishes a
        // baseline.
        const int scrollWheelValue = mouse.getScrollWheelValueProperty();
        if (!hasLastScrollWheelValue_)
        {
            camera.SetFieldOfView(kEditorDefaultFov);
        }
        else
        {
            const int deltaNotches = (scrollWheelValue - lastScrollWheelValue_) / 120;
            if (deltaNotches != 0)
            {
                camera.SetFieldOfView(std::clamp(
                    camera.GetFieldOfView() * std::pow(kScrollZoomStepPerNotch, static_cast<float>(deltaNotches)),
                    kEditorMinFov, kEditorMaxFov));
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

        // The on-screen placement pad is handled before raycasting so a
        // tap on any X/Y/Z button moves the red preview in this very frame.
        // Its action button is consumed later by the same placement path as
        // a normal left click.
        const GEEditorPalette::UpdateResult paletteResult =
            palette_.Update(mouse, viewportWidth, viewportHeight, dt);
        switch (paletteResult.action)
        {
            case GEEditorPalette::ToolbarAction::PlacementXMinus: --placementOffsetX_; break;
            case GEEditorPalette::ToolbarAction::PlacementXPlus:  ++placementOffsetX_; break;
            case GEEditorPalette::ToolbarAction::PlacementYMinus: --placementOffsetY_; break;
            case GEEditorPalette::ToolbarAction::PlacementYPlus:  ++placementOffsetY_; break;
            case GEEditorPalette::ToolbarAction::PlacementZMinus: --placementOffsetZ_; break;
            case GEEditorPalette::ToolbarAction::PlacementZPlus:  ++placementOffsetZ_; break;
            default: break;
        }

        const bool offsetXMinusHeld = keyboard.IsKeyDown(Keys::NumPad4);
        const bool offsetXPlusHeld = keyboard.IsKeyDown(Keys::NumPad6);
        const bool offsetZMinusHeld = keyboard.IsKeyDown(Keys::NumPad8);
        const bool offsetZPlusHeld = keyboard.IsKeyDown(Keys::NumPad2);
        const bool offsetYMinusHeld = keyboard.IsKeyDown(Keys::NumPad7);
        const bool offsetYPlusHeld = keyboard.IsKeyDown(Keys::NumPad9);
        const bool offsetResetHeld = keyboard.IsKeyDown(Keys::NumPad5);
        const bool placementOffsetKeyHeld = offsetXMinusHeld || offsetXPlusHeld || offsetZMinusHeld ||
                                            offsetZPlusHeld || offsetYMinusHeld || offsetYPlusHeld || offsetResetHeld;
        if (placementOffsetKeyHeld && !placementOffsetKeyHeldLastFrame_)
        {
            if (offsetResetHeld)
            {
                placementOffsetX_ = placementOffsetY_ = placementOffsetZ_ = 0;
            }
            else
            {
                placementOffsetX_ += offsetXPlusHeld ? 1 : (offsetXMinusHeld ? -1 : 0);
                placementOffsetZ_ += offsetZPlusHeld ? 1 : (offsetZMinusHeld ? -1 : 0);
                placementOffsetY_ += offsetYPlusHeld ? 1 : (offsetYMinusHeld ? -1 : 0);
            }
        }

        // Voxel raycast (plan.md EDITOR-102) -- camera position/direction
        // are in RENDER space (shifted by -kWorldCenterX/Z from @p world's
        // own raw grid space, see GEVoxelRaycast.hpp); only X/Z need the
        // shift reversed, Y is unshifted in both spaces.
        const RaycastHit hit = Raycast(world,
                                        camX_ + static_cast<float>(GEWorldRuntime::kWorldCenterX), camY_,
                                        camZ_ + static_cast<float>(GEWorldRuntime::kWorldCenterZ),
                                        forward.X, forward.Y, forward.Z,
                                        kMaxRaycastDistance);
        hasRaycastHit_ = hit.hit;
        hasHighlight_ = hit.hit;
        if (hasHighlight_)
        {
            hitCellX_ = hit.x;
            hitCellY_ = hit.y;
            hitCellZ_ = hit.z;
            hitNormalX_ = hit.normalX;
            hitNormalY_ = hit.normalY;
            hitNormalZ_ = hit.normalZ;
            // The translucent cube is a placement preview, not merely the
            // surface currently under the cursor: it marks the exact
            // adjacent cell that the next left click will create.
            const int previewX = static_cast<int>(hit.x) + hit.normalX + placementOffsetX_;
            const int previewY = static_cast<int>(hit.y) + hit.normalY + placementOffsetY_;
            const int previewZ = static_cast<int>(hit.z) + hit.normalZ + placementOffsetZ_;
            const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
            if (previewX >= 0 && previewX < blocksPerAxis &&
                previewY >= 0 && previewY < blocksPerAxis &&
                previewZ >= 0 && previewZ < blocksPerAxis)
            {
                highlightX_ = static_cast<float>(previewX) - static_cast<float>(GEWorldRuntime::kWorldCenterX);
                highlightY_ = static_cast<float>(previewY);
                highlightZ_ = static_cast<float>(previewZ) - static_cast<float>(GEWorldRuntime::kWorldCenterZ);
                placementCellX_ = static_cast<std::uint16_t>(previewX);
                placementCellY_ = static_cast<std::uint16_t>(previewY);
                placementCellZ_ = static_cast<std::uint16_t>(previewZ);
            }
            else
            {
                hasHighlight_ = false; // there is no valid cell to place into beyond this boundary
            }
        }
        else if (forward.Y < -0.0001f)
        {
            // Brand-new custom worlds are deliberately all air. Before
            // this fallback existed, their raycast could never hit, so the
            // editor offered no first cell to place and a new world was
            // permanently unbuildable. Treat the y=0 build plane as an
            // empty first-placement target; after that first block exists,
            // normal voxel-face placement resumes immediately.
            const float targetDistance = -camY_ / forward.Y;
            const float rawX = camX_ + static_cast<float>(GEWorldRuntime::kWorldCenterX) + forward.X * targetDistance;
            const float rawZ = camZ_ + static_cast<float>(GEWorldRuntime::kWorldCenterZ) + forward.Z * targetDistance;
            const int targetX = static_cast<int>(std::floor(rawX + 0.5f));
            const int targetZ = static_cast<int>(std::floor(rawZ + 0.5f));
            const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
            if (targetDistance >= 0.0f && targetDistance <= kMaxRaycastDistance &&
                targetX >= 0 && targetX < blocksPerAxis && targetZ >= 0 && targetZ < blocksPerAxis)
            {
                hasHighlight_ = true;
                const int previewX = targetX + placementOffsetX_;
                const int previewY = placementOffsetY_;
                const int previewZ = targetZ + placementOffsetZ_;
                if (previewX < 0 || previewX >= blocksPerAxis || previewY < 0 || previewY >= blocksPerAxis ||
                    previewZ < 0 || previewZ >= blocksPerAxis)
                {
                    hasHighlight_ = false;
                }
                else
                {
                    highlightX_ = static_cast<float>(previewX) - static_cast<float>(GEWorldRuntime::kWorldCenterX);
                    highlightY_ = static_cast<float>(previewY);
                    highlightZ_ = static_cast<float>(previewZ) - static_cast<float>(GEWorldRuntime::kWorldCenterZ);
                    placementCellX_ = static_cast<std::uint16_t>(previewX);
                    placementCellY_ = static_cast<std::uint16_t>(previewY);
                    placementCellZ_ = static_cast<std::uint16_t>(previewZ);
                }
                hitCellX_ = static_cast<std::uint16_t>(targetX);
                hitCellY_ = 0;
                hitCellZ_ = static_cast<std::uint16_t>(targetZ);
                hitNormalX_ = 0;
                hitNormalY_ = 0;
                hitNormalZ_ = 0;
            }
        }

        // Editing tools (plan.md EDITOR-103) -- edge-triggered on the
        // press, matching every existing click-handler in this codebase
        // (e.g. GEInputPad's "!mouseDown && mouseWasDown_" idiom, just
        // inverted here to trigger on press rather than release since
        // there's no on-screen button geometry to still be hovering over).
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
        const bool placeSelectionRequested =
            (leftHeld && !leftHeldLastFrame_ && !paletteResult.clickConsumed) ||
            paletteResult.action == GEEditorPalette::ToolbarAction::PlaceSelection;
        if (placeSelectionRequested && hasHighlight_ &&
            !boxFirstCornerPlaced_)
        {
            const int placeX = placementCellX_;
            const int placeY = placementCellY_;
            const int placeZ = placementCellZ_;
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
                MarkMutated();
            }
        }
        else if (middleHeld && !middleHeldLastFrame_ && !paletteResult.clickConsumed && hasRaycastHit_ &&
                 !boxFirstCornerPlaced_)
        {
            const Worlds::Block before = world.getBlock(hitCellX_, hitCellY_, hitCellZ_);
            const Worlds::Block after = Worlds::Block::air();
            world.setBlock(hitCellX_, hitCellY_, hitCellZ_, after);
            GEEditCommand command;
            command.kind = GEEditCommand::Kind::BlockEdit;
            command.blockChanges.push_back({hitCellX_, hitCellY_, hitCellZ_, before, after});
            commandStack_.Push(std::move(command));
            MarkMutated();
        }
        else if ((enterHeld && !enterHeldLastFrame_ && !worldPath_.empty()) ||
                 paletteResult.action == GEEditorPalette::ToolbarAction::Save)
        {
            if (!worldPath_.empty())
            {
                world.saveToFile(worldPath_);
                dirty_ = false; // EDITOR-112: matches disk again, Back no longer needs to confirm
                backConfirmArmed_ = false;
            }
        }
        else if ((undoKeyHeld && !undoKeyHeldLastFrame_) ||
                 paletteResult.action == GEEditorPalette::ToolbarAction::Undo)
        {
            if (commandStack_.Undo(world))
            {
                MarkMutated();
                RefreshSelectedObjectAfterHistoryChange(world);
            }
        }
        else if ((redoKeyHeld && !redoKeyHeldLastFrame_) ||
                 paletteResult.action == GEEditorPalette::ToolbarAction::Redo)
        {
            if (commandStack_.Redo(world))
            {
                MarkMutated();
                RefreshSelectedObjectAfterHistoryChange(world);
            }
        }
        else if (paletteResult.action == GEEditorPalette::ToolbarAction::Back)
        {
            // Re-enters the browser for the SAME gamer slot (plan.md
            // EDITOR-107) -- IsBrowsing() reports true starting next
            // frame's dispatch. Deliberately does NOT auto-save; a player
            // who wants to keep changes presses Save/Enter first.
            //
            // Unsaved-changes guard (EDITOR-112): if dirty_, the FIRST Back
            // press only arms a confirm (backConfirmArmed_) instead of
            // leaving -- same real "two-tap" idiom this editor's own
            // GEEditorBrowserScreen already uses for its delete button
            // (armedDeleteIndex_). The palette's Back button brightens
            // while armed (ButtonGreenActive(), see GEEditorPalette::Draw())
            // so the "click again to discard" state is visible without any
            // text. A SECOND Back press while armed actually leaves,
            // discarding whatever wasn't saved. PlayTest/Save below both
            // clear backConfirmArmed_ too (dirty_ became false, so there's
            // nothing left to confirm).
            if (dirty_ && !backConfirmArmed_)
            {
                backConfirmArmed_ = true;
            }
            else
            {
                backConfirmArmed_ = false;
                EnterBrowser(gamerSlot_);
            }
        }
        else if (paletteResult.action == GEEditorPalette::ToolbarAction::PlayTest && !worldPath_.empty())
        {
            // Save first (plan.md EDITOR-108) so the play-tested session
            // matches exactly what's on screen, then request a real
            // gameplay session -- GalaxyEggbertCnaGame still owns actually
            // switching phase/reloading (this class has no GEWorldRuntime
            // access).
            world.saveToFile(worldPath_);
            dirty_ = false;
            backConfirmArmed_ = false;
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
                    MarkMutated();
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
            MarkMutated();
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
            MarkMutated();
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
            MarkMutated();
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
                MarkMutated();
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
            MarkMutated();
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
        placementOffsetKeyHeldLastFrame_ = placementOffsetKeyHeld;
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
                      viewportWidth, viewportHeight, backConfirmArmed_, hasHighlight_);
    }
}
