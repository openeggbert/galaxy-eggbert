#include "WorldEditor.hpp"

#include "CustomWorldStorage.hpp"
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
            request.galaxyImported = galaxy.imported;
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

    void WorldEditor::Draw(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
        const CnaEngine::Graphics::Camera& camera,
        int viewportWidth, int viewportHeight, const Worlds::World& world)
    {
        pathEndpoints_.Draw(device,camera,world);
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

        auto status = StatusLines();
        for (const auto& start : CollectPlayerStarts(world))
            status.push_back(std::string(PlayerStartNames[static_cast<int>(start.color)])+" start X:"+
                std::to_string(start.x)+" Y:"+std::to_string(start.y)+" Z:"+std::to_string(start.z));
        if (target)
        {
            selectionHighlightRenderer_.ShowSelectedObject(device,
                float(target->x)-Game::WorldRuntime::kWorldCenterX, float(target->y),
                float(target->z)-Game::WorldRuntime::kWorldCenterZ);
            selectionHighlightRenderer_.Draw(device,camera);
            status.insert(status.begin(), "Red selection X:"+std::to_string(target->x)+" Y:"+
                std::to_string(target->y)+" Z:"+std::to_string(target->z));
            if (target->kind==EditTarget::Kind::PlayerStart)
                status.insert(status.begin(),std::string(PlayerStartNames[static_cast<int>(target->color)])+" Blupi: O turns, DEL removes");
        }
        else selectionHighlightRenderer_.Hide();

        palette_.Draw(
            device, viewportWidth, viewportHeight, stopConfirmArmed_, hasHighlight_,
            placementCellX_, placementCellY_, placementCellZ_, status);
    }
}
