#include "WorldEditor.hpp"

#include <Microsoft/Xna/Framework/Vector4.hpp>

#include "CustomWorldStorage.hpp"
#include "VoxelRaycast.hpp"
#include <GalaxyEggbert/Game/WorldRuntime.hpp>

#include <algorithm>
#include <cmath>
#include <optional>

namespace GalaxyEggbert::Editor
{
    namespace
    {
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

    void WorldEditor::EnterBrowser(int gamerSlot)
    {
        gamerSlot_ = gamerSlot;
        browsing_ = true;
        // The showcase world fills an empty slot 1 once per gamer (owner,
        // 2026-10-25; CustomWorldStorage::SeedShowcaseSlot()).
        (void)SeedShowcaseSlot(gamerSlot, "worlds3d/world999.vwr");
        // The gallery of everything drawn (OR-0044) in the next free slot, kept current.
        (void)SeedGallerySlot(gamerSlot, "worlds3d/world998.vwr");
        browserScreen_.ResetMode();
        browserScreen_.Refresh(gamerSlot);
        galaxyScreen_.Refresh(gamerSlot);
        backedUpPath_.clear(); // the next save of a world opened from here backs it up first
    }

    void WorldEditor::ShowGalaxy()
    {
        galaxyView_ = true;
        galaxyScreen_.Refresh(gamerSlot_);
    }

    WorldEditor::BrowserRequest WorldEditor::UpdateBrowsing(
        const Microsoft::Xna::Framework::Input::MouseState& mouse,
        int viewportWidth, int viewportHeight, bool backPressed)
    {
        if (galaxyView_)
        {
            const auto galaxy = galaxyScreen_.Update(mouse, backPressed);
            BrowserRequest request;
            request.galaxyImported = galaxy.imported || galaxy.deleted;
            if (galaxy.action == GalaxyBrowserScreen::Action::Back)
            {
                galaxyView_ = false; // back to the slots
                browserScreen_.Refresh(gamerSlot_);
                browserScreen_.SyncBackKey(backPressed);
            }
            else if (galaxy.action == GalaxyBrowserScreen::Action::Open)
            {
                request.shouldOpen = true;
                request.openPath = galaxy.path;
                request.galaxyMission = galaxy.mission;
            }
            else if (galaxy.action == GalaxyBrowserScreen::Action::New)
            {
                request.shouldCreateNew = true;
                request.createPath = galaxy.path;
                request.galaxyMission = galaxy.mission;
            }
            return request;
        }
        const auto result =
            browserScreen_.Update(mouse, viewportWidth, viewportHeight, backPressed);
        BrowserRequest request;
        if (result.action == EditorBrowserScreen::Action::Open)
        {
            request.shouldOpen = true;
            request.openPath = result.path;
        }
        else if (result.action == EditorBrowserScreen::Action::New)
        {
            request.shouldCreateNew = true;
            request.createPath = result.path;
        }
        else if (result.action == EditorBrowserScreen::Action::Back)
        {
            request.shouldReturnToMenu = true;
        }
        else if (result.action == EditorBrowserScreen::Action::Galaxy)
        {
            ShowGalaxy();
        }
        return request;
    }

    void WorldEditor::DrawBrowsing(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
        int viewportWidth, int viewportHeight)
    {
        if (galaxyView_)
            galaxyScreen_.Draw(device, viewportWidth, viewportHeight);
        else
            browserScreen_.Draw(device, viewportWidth, viewportHeight);
    }

    void WorldEditor::EnterEditing(float startX, float startY, float startZ) noexcept
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
        aimKind_ = AimKind::None;
        placementOffsetX_ = placementOffsetY_ = placementOffsetZ_ = 0;
        placementOffsetKeyHeldLastFrame_ = false;
        manualPlacement_ = false;
        highlightRenderer_.Hide();
        commandStack_ = EditCommandStack();
        boxFirstCornerPlaced_ = false;
        boxForCopy_ = false;
        pasting_ = false;
        showingBox_ = false;
        playTestRequested_ = false;
        hasSelectedObject_ = false;
        selectedPlayerStart_.reset();
        selectionHighlightRenderer_.Hide();
        dirty_ = false;
        stopConfirmArmed_ = false;
        placeKeyHeldLastFrame_ = false;
        stopKeyHeldLastFrame_ = false;
        extendedSelection_ = false;
    }

    void WorldEditor::Update(
        const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
        const Microsoft::Xna::Framework::Input::MouseState& mouse,
        float dt, int viewportWidth, int viewportHeight,
        CnaEngine::Graphics::Camera& camera, Worlds::World& world, const Navigation& navigation)
    {
        const Microsoft::Xna::Framework::Vector3 forward = UpdateCamera(keyboard, mouse, dt, camera, navigation);
        palette_.SetSelectedSkyRegion(world.skyRegion());
        palette_.SetSelectedMusicTrack(world.musicTrack());
        const EditorPalette::UpdateResult paletteResult =
            palette_.Update(mouse, viewportWidth, viewportHeight, dt);
        if (paletteResult.picked) selectedPlayerStart_.reset();
        const bool placementOffsetKeyHeld =
            UpdatePlacementOffset(keyboard, paletteResult.action);
        // Aim through the cursor; over the palette or the PLACE/offset buttons keep the last aim
        // (the centre aims when there is none yet, e.g. a touch that began on a button).
        const bool looking =
            mouse.getRightButtonProperty() == Microsoft::Xna::Framework::Input::ButtonState::Pressed;
        if (!navigation.keepAim && !manualPlacement_ &&
            (looking || !palette_.IsPointerOverControl(mouse, viewportWidth, viewportHeight)))
        {
            AimAt(world, navigation.centeredAim ? forward : AimDirection(forward, mouse, viewportWidth, viewportHeight, camera));
        }
        else if (aimKind_ == AimKind::None)
        {
            AimAt(world, forward);
        }
        UpdatePlacementPreview(world);

        const FrameInput input = ReadFrameInput(keyboard, mouse, paletteResult);
        UpdateBoxPreview(world);
        if (!HandleExtendedSelection(input, world) &&
            !HandleClipboard(input, world) &&
            !HandlePlacement(input, world) &&
            !HandleRemoval(input, world) &&
            !HandleSessionAndHistory(input, world) &&
            !HandleBoxFill(input, world) &&
            !HandleSkyRegion(input, world) &&
            !HandleMusic(input, world) &&
            !HandleTurn(input, world) &&
            !HandleVariant(input, world))
        {
            (void)HandleObjectEditing(input, camera, viewportWidth, viewportHeight, world);
        }
        palette_.SetSelectedSkyRegion(world.skyRegion());
        StoreInputEdges(input, placementOffsetKeyHeld);
    }

    std::optional<MoveObjectRecord> WorldEditor::ApplyActiveFieldDelta(
        const MoveObjectRecord& record, float sign) const noexcept
    {
        MoveObjectRecord updated = record;
        switch (activeField_)
        {
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

    void WorldEditor::RefreshSelectedObjectAfterHistoryChange(const Worlds::World& world)
    {
        if (selectedPlayerStart_ && !FindPlayerStart(world,*selectedPlayerStart_)) selectedPlayerStart_.reset();
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

    bool WorldEditor::ConsumeNeedsPresentationRebuild() noexcept
    {
        const bool result = needsPresentationRebuild_;
        needsPresentationRebuild_ = false;
        return result;
    }

    void WorldEditor::BeginOutsideSession(std::filesystem::path backupDir, std::filesystem::path keepCopyIn)
    {
        outsideSession_ = true;
        leaveRequested_ = false;
        outsideBackupDir_ = std::move(backupDir);
        outsideCopyDir_ = std::move(keepCopyIn);
        backedUpPath_.clear(); // its first save keeps the world as it was
    }

    void WorldEditor::EndOutsideSession() noexcept
    {
        outsideSession_ = false;
        leaveRequested_ = false;
        outsideBackupDir_.clear();
        outsideCopyDir_.clear();
        backedUpPath_.clear();
    }

    bool WorldEditor::ConsumePlayTestRequested() noexcept
    {
        if (!thumbnailRequest_.empty())
        {
            return false; // the save's thumbnail first
        }
        const bool result = playTestRequested_;
        playTestRequested_ = false;
        return result;
    }

    void WorldEditor::DrawWorldOverlays(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
        const CnaEngine::Graphics::Camera& camera, const Worlds::World& world)
    {
        // The world's axes on its bounds; the path ends depth-tested; the cursor's cell and the
        // selection's box over everything, as when they were drawn after the HUD had turned the depth
        // test off.
        DrawWorldAxes(device, camera, world);
        pathEndpoints_.Draw(device,camera,world);
        device.SetDepthTestEnabled(false);
        const auto target = CurrentEditTarget(world);
        const bool sameCell = target && target->x==placementCellX_ &&
            target->y==placementCellY_ && target->z==placementCellZ_;
        if (showingBox_)
        {
            highlightRenderer_.ShowBox(
                device, boxMinRenderX_, boxMinRenderY_, boxMinRenderZ_,
                boxMaxRenderX_, boxMaxRenderY_, boxMaxRenderZ_);
            highlightRenderer_.Draw(device, camera);
        }
        else if (hasHighlight_ && !sameCell)
        {
            highlightRenderer_.ShowCell(device, highlightX_, highlightY_, highlightZ_);
            highlightRenderer_.Draw(device, camera);
        }
        else
        {
            highlightRenderer_.Hide();
        }
        if (target)
        {
            selectionHighlightRenderer_.ShowSelectedObject(device,
                float(target->x)-Game::WorldRuntime::kWorldCenterX, float(target->y),
                float(target->z)-Game::WorldRuntime::kWorldCenterZ);
            selectionHighlightRenderer_.Draw(device,camera);
        }
        else selectionHighlightRenderer_.Hide();
    }

    namespace
    {
        // The world's bounds in render space: cell (x, y, z) is centred on (x - 50, y, z - 50).
        struct WorldBounds
        {
            float x0, x1, y0, y1, z0, z1;
        };

        WorldBounds BoundsOf(const Worlds::World& world)
        {
            const float n = static_cast<float>(world.blocksPerAxis());
            const float cx = static_cast<float>(Game::WorldRuntime::kWorldCenterX);
            const float cz = static_cast<float>(Game::WorldRuntime::kWorldCenterZ);
            return {-0.5f - cx, n - 0.5f - cx, -0.5f, n - 0.5f, -0.5f - cz, n - 0.5f - cz};
        }

        const Microsoft::Xna::Framework::Color kAxisX(235, 70, 60);
        const Microsoft::Xna::Framework::Color kAxisY(70, 215, 95);
        const Microsoft::Xna::Framework::Color kAxisZ(70, 135, 250);
    }

    void WorldEditor::DrawWorldAxes(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                    const CnaEngine::Graphics::Camera& camera, const Worlds::World& world)
    {
        // The world's twelve edges, each in its axis's colour (X red, Y green, Z blue), and along the three
        // that leave its origin corner (cell 0, 0, 0) a tick every ten cells, a longer one every fifty.
        using Microsoft::Xna::Framework::Vector3;
        const auto b = BoundsOf(world);
        worldAxes_.Clear();
        for (const float y : {b.y0, b.y1})
            for (const float z : {b.z0, b.z1})
                worldAxes_.Line(Vector3(b.x0, y, z), Vector3(b.x1, y, z), kAxisX);
        for (const float x : {b.x0, b.x1})
            for (const float z : {b.z0, b.z1})
                worldAxes_.Line(Vector3(x, b.y0, z), Vector3(x, b.y1, z), kAxisY);
        for (const float x : {b.x0, b.x1})
            for (const float y : {b.y0, b.y1})
                worldAxes_.Line(Vector3(x, y, b.z0), Vector3(x, y, b.z1), kAxisZ);
        const int n = static_cast<int>(world.blocksPerAxis());
        for (int i = 10; i < n; i += 10)
        {
            const float length = i % 50 == 0 ? 1.6f : 0.8f;
            const float at = static_cast<float>(i);
            worldAxes_.Line(Vector3(b.x0 + at, b.y0, b.z0), Vector3(b.x0 + at, b.y0 + length, b.z0), kAxisX);
            worldAxes_.Line(Vector3(b.x0 + at, b.y0, b.z0), Vector3(b.x0 + at, b.y0, b.z0 + length), kAxisX);
            worldAxes_.Line(Vector3(b.x0, b.y0 + at, b.z0), Vector3(b.x0 + length, b.y0 + at, b.z0), kAxisY);
            worldAxes_.Line(Vector3(b.x0, b.y0 + at, b.z0), Vector3(b.x0, b.y0 + at, b.z0 + length), kAxisY);
            worldAxes_.Line(Vector3(b.x0, b.y0, b.z0 + at), Vector3(b.x0 + length, b.y0, b.z0 + at), kAxisZ);
            worldAxes_.Line(Vector3(b.x0, b.y0, b.z0 + at), Vector3(b.x0, b.y0 + length, b.z0 + at), kAxisZ);
        }
        device.SetDepthTestEnabled(true);
        worldAxes_.Draw(device, camera, true);
    }

    std::vector<EditorPaletteRenderer::State::ViewLabel> WorldEditor::AxisLabels(
        const CnaEngine::Graphics::Camera& camera, int viewportWidth, int viewportHeight,
        const Worlds::World& world) const
    {
        // The axes' names past their ends (and 0 at the origin corner), where the camera sees them.
        using Microsoft::Xna::Framework::Vector3;
        using Microsoft::Xna::Framework::Vector4;
        const auto b = BoundsOf(world);
        const auto viewProjection = camera.View() * camera.Projection();
        std::vector<EditorPaletteRenderer::State::ViewLabel> labels;
        const auto add = [&](const Vector3& at, const char* text, const Microsoft::Xna::Framework::Color& colour)
        {
            const Vector4 clip = Vector4::Transform(Vector4(at.X, at.Y, at.Z, 1.0f), viewProjection);
            if (clip.W <= 0.05f)
            {
                return; // behind the camera
            }
            // Hidden behind the world's blocks, as the axes' lines are (2026-11-10, the owner: the
            // letters showed through the blocks in front of them): a block on the way from the camera.
            // The last stretch is left out: the label lies just past the world's edge, whose own
            // blocks (a floor to the bounds) would hide it from every view at a slant.
            constexpr float kNearLabel = 2.5f;
            const Vector3 to(at.X - camX_, at.Y - camY_, at.Z - camZ_);
            const float distance = to.Length();
            if (distance > kNearLabel &&
                Raycast(world, camX_ + static_cast<float>(Game::WorldRuntime::kWorldCenterX), camY_,
                        camZ_ + static_cast<float>(Game::WorldRuntime::kWorldCenterZ), to.X / distance,
                        to.Y / distance, to.Z / distance, distance - kNearLabel)
                    .hit)
            {
                return;
            }
            const float x = (clip.X / clip.W * 0.5f + 0.5f) * static_cast<float>(viewportWidth);
            const float y = (0.5f - clip.Y / clip.W * 0.5f) * static_cast<float>(viewportHeight);
            if (x < -20.0f || y < -20.0f || x > viewportWidth + 20.0f || y > viewportHeight + 20.0f)
            {
                return;
            }
            labels.push_back({x, y, text,
                              Vector3(colour.getRProperty() / 255.0f, colour.getGProperty() / 255.0f,
                                      colour.getBProperty() / 255.0f)});
        };
        add(Vector3(b.x1 + 1.5f, b.y0, b.z0), "X", kAxisX);
        add(Vector3(b.x0, b.y1 + 1.5f, b.z0), "Y", kAxisY);
        add(Vector3(b.x0, b.y0, b.z1 + 1.5f), "Z", kAxisZ);
        add(Vector3(b.x0 - 1.0f, b.y0 - 1.0f, b.z0 - 1.0f), "0", Microsoft::Xna::Framework::Color(230, 230, 230));
        return labels;
    }

    void WorldEditor::Draw(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
        const CnaEngine::Graphics::Camera& camera,
        int viewportWidth, int viewportHeight, const Worlds::World& world)
    {
        const auto target = CurrentEditTarget(world);
        auto status = StatusLines();
        for (const auto& start : CollectPlayerStarts(world))
            status.push_back(std::string(PlayerStartNames[static_cast<int>(start.color)])+" start X:"+
                std::to_string(start.x)+" Y:"+std::to_string(start.y)+" Z:"+std::to_string(start.z));
        if (target)
        {
            status.insert(status.begin(), "Red selection X:"+std::to_string(target->x)+" Y:"+
                std::to_string(target->y)+" Z:"+std::to_string(target->z));
            if (target->kind==EditTarget::Kind::PlayerStart)
                status.insert(status.begin(),std::string(PlayerStartNames[static_cast<int>(target->color)])+" Blupi: O turns, DEL removes");
        }

        palette_.Draw(
            device, viewportWidth, viewportHeight, stopConfirmArmed_, hasHighlight_,
            placementCellX_, placementCellY_, placementCellZ_, status,
            AxisLabels(camera, viewportWidth, viewportHeight, world));
    }
}
