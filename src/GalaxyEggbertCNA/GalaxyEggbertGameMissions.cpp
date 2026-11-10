// GalaxyEggbertGame: the autoplay steps, the missions and worlds loaded (campaign, editor, galaxy, title scene) and the phases. Split out of GalaxyEggbertGame.cpp.
#include "GalaxyEggbertGame.hpp"
#include "GameInternal.hpp"

#include <GalaxyEggbert/Game/WaterSurface.hpp>
#include <GalaxyEggbert/Game/CheatConfig.hpp>
#include <GalaxyEggbert/Game/DebugOverlay.hpp>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif
#include <GalaxyEggbert/Game/GamepadInput.hpp>
#include <GalaxyEggbert/Game/BarRows.hpp>
#include <GalaxyEggbert/Game/BlockModels.hpp>
#include <GalaxyEggbert/Game/BoltMesh.hpp>
#include <GalaxyEggbert/Game/EnemyBlupi.hpp>
#include <GalaxyEggbert/Game/PlateRotationMetadata.hpp>
#include <GalaxyEggbert/Game/GlueStrands.hpp>

#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <GalaxyEggbert/Editor/CustomWorldStorage.hpp>
#include <GalaxyEggbert/Game/BrickEdges.hpp>
#include <GalaxyEggbert/Editor/PlatformFiles.hpp>
#include <GalaxyEggbert/Editor/PlayerStartColor.hpp>
#include <GalaxyEggbert/Game/ObjectDefinitionRegistry.hpp>
#include "GalaxyEggbert/BlockTypes.hpp"
#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
#include "GalaxyEggbert/Worlds/Block.hpp"

#include <GalaxyEggbert/HubDoors.hpp>
#include <GalaxyEggbert/Game/LiftVisual.hpp>
#include <GalaxyEggbert/Game/ObjectVerticalPlacement.hpp>
#include <GalaxyEggbert/Game/TerrainLighting.hpp>
#include <GalaxyEggbert/Game/TexturedMesh.hpp>
#include <Microsoft/Xna/Framework/Graphics/RasterizerState.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/Input/GamePad.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>
#include <Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp>
#include <Microsoft/Xna/Framework/Rectangle.hpp>
#include <Microsoft/Xna/Framework/Graphics/ModelMeshPart.hpp>
#include <Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp>

#include <Microsoft/Xna/Framework/Graphics/SamplerState.hpp>

#include <algorithm>
#include <Microsoft/Xna/Framework/BoundingFrustum.hpp>
#include <Microsoft/Xna/Framework/BoundingSphere.hpp>
#include <Microsoft/Xna/Framework/ContainmentType.hpp>
#include <chrono>
#include <unordered_set>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <optional>
#include <set>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#include "GameHelpers.hpp"

namespace GalaxyEggbert::CNA
{
    using namespace GalaxyEggbert::Game;
    using namespace GalaxyEggbert::CNA::detail;

    void GalaxyEggbertGame::StepAutoplay()
    {
        using Microsoft::Xna::Framework::Input::Keys;
        constexpr float kDegreesPerRadian = 57.2957795f;
        GalaxyEggbert::Game::AutoplayState state;
        state.tick = autoplayTick_++;
        state.x = blupi_.GetX() + WorldRuntime::kWorldCenterX;
        state.y = blupi_.GetY();
        state.z = blupi_.GetZ() + WorldRuntime::kWorldCenterZ;
        state.yawDegrees = blupi_.GetYaw() * kDegreesPerRadian;
        state.onGround = blupi_.IsOnGround();
        state.dead = blupi_.IsDeathLocked();
        state.ghost = blupi_.IsGhost();
        state.anim = static_cast<int>(blupi_.GetAnimState());
        state.exitReached = interaction_.ExitReached();
        state.blitz = ((worldRuntime_.GetAnimPhase() % 100) + 100) % 100;
        state.temp = (((worldRuntime_.GetAnimPhase() / 4) % 20) + 20) % 20;
        state.crush = (((worldRuntime_.GetAnimPhase() / 3) % 10) + 10) % 10;
        if (autoplaySwitchMission_ != worldRuntime_.GetMissionNumber())
        {
            autoplaySwitchMission_ = worldRuntime_.GetMissionNumber();
            autoplaySwitchPresses_ = 0;
            autoplaySawsTurning_ = -1;
        }
        if (autoplaySawsTurning_ < 0)
        {
            const auto& world = worldRuntime_.GetWorld();
            const int n = world.blocksPerAxis();
            autoplaySawsTurning_ = 0;
            for (int z = 0; z < n; ++z)
                for (int y = 0; y < n; ++y)
                    for (int x = 0; x < n; ++x)
                        autoplaySawsTurning_ += world.getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                                               static_cast<std::uint16_t>(z)).type() ==
                                                GalaxyEggbert::BlockTypes::Saw;
        }
        state.switchPresses = autoplaySwitchPresses_;
        state.sawsTurning = autoplaySawsTurning_;
        state.phase = phase_;
        state.mission = worldRuntime_.GetMissionNumber();
        state.treasures = interaction_.TreasuresCollected();
        state.eggs = interaction_.LifeEggCount();
        state.lives = interaction_.Lives();
        state.keys = interaction_.Key1Count() + interaction_.Key2Count() + interaction_.Key3Count();
        state.dynamite = interaction_.DynamiteCount();
        state.vehicle = static_cast<int>(blupi_.GetVehicleMode());
        for (const auto& object : worldRuntime_.GetMobileObjects())
        {
            if (object.active && GetObjectDefinition(object.type).semanticKind == ObjectSemanticKind::Lift)
            {
                state.lifts.push_back({object.currentX + WorldRuntime::kWorldCenterX,
                                       LiftRiderCenterY(object.currentY),
                                       object.currentZ + WorldRuntime::kWorldCenterZ});
            }
            const auto kind = GetObjectDefinition(object.type).semanticKind;
            if (object.active && (kind == ObjectSemanticKind::Enemy || kind == ObjectSemanticKind::Projectile))
            {
                state.enemies.push_back({object.currentX + WorldRuntime::kWorldCenterX, object.currentY,
                                         object.currentZ + WorldRuntime::kWorldCenterZ});
            }
            if (object.active && object.type == GalaxyEggbert::Def::ObjectType::ObjectType12)
            {
                state.crates.push_back({object.currentX + WorldRuntime::kWorldCenterX, object.currentY,
                                        object.currentZ + WorldRuntime::kWorldCenterZ});
            }
        }
        const GalaxyEggbert::Game::AutoplayKeys keys = autoplay_->Step(state);
        std::unordered_set<Keys> down;
        if (keys.up) down.insert(Keys::Up);
        if (keys.down) down.insert(Keys::Down);
        if (keys.left) down.insert(Keys::Left);
        if (keys.right) down.insert(Keys::Right);
        if (keys.jump) down.insert(Keys::LeftControl);
        if (keys.action) down.insert(Keys::Space);
        if (keys.crouch) down.insert(Keys::LeftShift);
        if (keys.lookUp) down.insert(Keys::RightShift);
        if (keys.letter >= 'a' && keys.letter <= 'z')
            down.insert(static_cast<Keys>(static_cast<int>(Keys::A) + keys.letter - 'a'));
        autoplayKeys_ = Microsoft::Xna::Framework::Input::KeyboardState(down);
        const std::string lines = autoplay_->TakeLog();
        std::fputs(lines.c_str(), autoplayTrace_);
        std::fflush(autoplayTrace_);
        if (autoplay_->Finished())
        {
            std::fclose(autoplayTrace_);
            autoplayTrace_ = nullptr;
            autoplayExitCode_ = autoplay_->Passed() ? 0 : 1;
            std::cout << "GalaxyEggbertCNA: autoplay " << (autoplay_->Passed() ? "passed" : "FAILED") << " after "
                      << state.tick << " ticks (autoplay_trace.txt)." << std::endl;
            Exit();
        }
    }

    std::optional<GalaxyEggbert::HubArrival> GalaxyEggbertGame::AdaptHubDoors(int missionNumber)
    {
        // Decor::AdaptDoors(): m_doors[] from the save (IsMissionDoorUnlocked), or every door with
        // the opendoors cheat (Decor::CheatAction(OpenDoors) flips m_bCheatDoors and adapts again).
        auto& terrain = worldRuntime_.GetWorldMutable();
        const auto adapted = GalaxyEggbert::AdaptHubDoors(
            terrain, missionNumber, [this](int index) { return saveData_.IsMissionDoorUnlocked(index); },
            cheatDoors_);
        for (const auto& gold : adapted.goldDoors)
        {
            InteractionSystem::OpenDoor(worldRuntime_, gold.x, gold.y, gold.z, sound_);
        }
        for (const auto& sign : adapted.completedSigns)
        {
            const auto x = static_cast<std::uint16_t>(sign.x);
            const auto y = static_cast<std::uint16_t>(sign.y);
            const auto z = static_cast<std::uint16_t>(sign.z);
            terrain.setBlock(x, y, z, GalaxyEggbert::Worlds::Block::make(
                                          GalaxyEggbert::CompletedWorldSignIcon(terrain.getBlock(x, y, z).type())));
        }
        std::optional<GalaxyEggbert::HubArrival> arrival;
        for (const auto& door : adapted.missionDoors)
        {
            InteractionSystem::OpenDoor(worldRuntime_, door.door.x, door.door.y, door.door.z, sound_);
            if (const auto past = GalaxyEggbert::ArrivalPastDoor(worldRuntime_.GetWorld(), door.door, door.sign))
            {
                arrival = past;
            }
        }
        return arrival;
    }

    void GalaxyEggbertGame::PlaceBlupi(float x, float y, float z)
    {
        blupi_.SetPosition(x, y, z);
        // As before Step(), crate support must reflect the objects' current positions.
        std::vector<std::array<float, 3>> crateCells;
        for (const auto& obj : worldRuntime_.GetMobileObjects())
        {
            if (obj.active && GetObjectDefinition(obj.type).semanticKind == ObjectSemanticKind::StaticObstacle)
            {
                crateCells.push_back({obj.currentX + WorldRuntime::kWorldCenterX, obj.currentY,
                                      obj.currentZ + WorldRuntime::kWorldCenterZ});
            }
        }
        blupi_.SetCrateCells(std::move(crateCells));
        blupi_.SettleOnGround(worldRuntime_.GetWorld(),
                             WorldRuntime::IsTempPassableAtPhase(worldRuntime_.GetAnimPhase()));
    }

    void GalaxyEggbertGame::LoadMission(int missionNumber, const std::string* validatedBytes)
    {
        // A demo shows one mission: where play would go on to another, the demo (or its recording)
        // ends instead, with nothing loaded or saved (DemoReplay.hpp).
        if ((demoActive_ && demoSessionLive_) || demoRecording_)
        {
            demoEndRequested_ = true;
            // The player's recording ends as its replay will (here); the mission follows once it
            // is written (EndPlayerRecording()).
            if (demoRecording_ && demoRecordSlot_ > 0) deferredMission_ = missionNumber;
            return;
        }
        if (!replayingResume_)
        {
            DeleteResumeFile();
            pendingResume_.reset();
            recordingResume_ = false;
        }
        titleSceneActive_ = false;
        titleSceneRequested_ = false;
        winCheerActive_ = false;
        pendingSkate_ = {};
        teleportArrivalGuard_ = false;
        accelWaitZero_ = true; // StartMission(): the first tilt waits for a level device
        const auto path = MissionPath(missionNumber);
        // Hash and decode one snapshot. During resume this is the already validated snapshot,
        // so an editor save/file replacement between validation and load cannot change it.
        const auto fileBytes = validatedBytes ? std::optional<std::string>{} : ReadWorldBytes(path);
        const auto* bytes = validatedBytes ? validatedBytes : (fileBytes ? &*fileBytes : nullptr);
        std::istringstream world(bytes ? *bytes : std::string{}, std::ios::in | std::ios::binary);
        if (!bytes || !worldRuntime_.LoadFromVwrStream(world))
        {
            std::cout << "GalaxyEggbertCNA: LoadMission(" << missionNumber << "): " << path
                       << " not found or failed to load — staying on the current mission." << std::endl;
            return;
        }
        loadedMissionPath_ = path;
        loadedMissionHash_ = ResumeLog::HashBytes(*bytes);
        if (galaxyGamer_ >= 0) worldRuntime_.OverrideMissionNumber(missionNumber); // its place in the galaxy

        RebuildWorldPresentation();

        // The hubs (Decor::AdaptDoors(), MainSwitchInitialize(), HubDoors.hpp): the doors that
        // are open slide up as the hub appears (sound 33, as in the original and the video), the
        // signs of completed worlds change, and Blupi starts past the last door opened in a world
        // hub, or beside the sign of the world he last played in the main hub.
        if (missionNumber != 1)
        {
            saveData_.SetLastWorld(missionNumber / 10);
        }
        std::optional<GalaxyEggbert::HubArrival> hubArrival = AdaptHubDoors(missionNumber);
        if (missionNumber == 1)
        {
            const auto lastWorldSigns = GalaxyEggbert::FindWorldSigns(worldRuntime_.GetWorld(), saveData_.GetLastWorld());
            if (!lastWorldSigns.empty())
            {
                hubArrival = GalaxyEggbert::ArrivalBesideSign(worldRuntime_.GetWorld(), lastWorldSigns.front());
            }
        }
        RefreshTerrainMeshIfChanged();

        // Real PlayPrepare() (Decor.cpp:381-429): every mission load resets
        // vehicle mode, all 4 secret powers, Invert, Ghost, keys, dynamite
        // count, and recounts this world's own treasures from scratch --
        // ONLY lives (m_nbVies) survive. `interaction_`/`blupi_` are plain
        // value members (not pointers), so reassigning fresh defaults is a
        // real reset, not a partial one -- same idiom already used
        // elsewhere in this class for "start over" semantics.
        const int preservedLives = interaction_.Lives();
        interaction_ = InteractionSystem();
        interaction_.SetLives(preservedLives);
        missionSeed_ = replayingResume_ ? resumeLog_.start.seed
                     : demoActive_     ? demo_.log.start.seed
                                       : std::random_device{}();
        interaction_.SeedRandom(missionSeed_);
        blupi_ = BlupiController();
        // A fresh Blupi knows no BigDecor: give him the solid ones again (M3D-0006). They were set
        // only by RebuildWorldPresentation(), before this reset, so in play every house, tree and
        // statue let him through (BD-0080, the owner's report of 2026-11-06).
        blupi_.SetSolidBoxes(worldRuntime_.SolidBigDecorBoxes());
        wasInHelicopter_ = false;
        hideGhosts_.clear();
        hideGhostTracking_ = false;

        if (worldRuntime_.HasExplicitSpawnPoint())
        {
            PlaceBlupi(
                worldRuntime_.GetSpawnRenderX(), worldRuntime_.GetSpawnRenderY(),
                worldRuntime_.GetSpawnRenderZ());
            blupi_.SetYaw(worldRuntime_.GetSpawnYaw());
        }
        else if (galaxyGamer_ >= 0)
        {
            // A galaxy world made before its start was set: where the editor's play-test puts him.
            PlaceBlupi(GalaxyEggbert::Editor::kEditorStarterSpawnRenderX, GalaxyEggbert::Editor::kEditorStarterSpawnY,
                       GalaxyEggbert::Editor::kEditorStarterSpawnRenderZ);
        }
        else
        {
            PlaceBlupi(0.0f, 1.0f, 0.0f);
        }
        if (hubArrival)
        {
            PlaceBlupi(static_cast<float>(hubArrival->cell.x - WorldRuntime::kWorldCenterX),
                       static_cast<float>(hubArrival->cell.y),
                       static_cast<float>(hubArrival->cell.z - WorldRuntime::kWorldCenterZ));
            blupi_.SetYaw(hubArrival->yaw);
        }
        chaseCamera_.Reset(blupi_.GetYaw()); // a new world starts with the camera behind him
        cameraSmoothedInitialized_ = false;  // ... there at once, not easing over from the last world

        missionStartPending_ = !replayingResume_ && !demoActive_;
        saveData_.SetMissionNumber(missionNumber);
        SavePlayerData();

        std::cout << "GalaxyEggbertCNA: LoadMission(" << missionNumber << "): loaded " << path << "."
                   << std::endl;
    }

    void GalaxyEggbertGame::LoadCustomWorldForEditing(const std::filesystem::path& path)
    {
        titleSceneActive_ = false;
        if (!worldRuntime_.LoadFromVwrFile(path.string()))
        {
            std::cout << "GalaxyEggbertCNA: LoadCustomWorldForEditing(" << path
                       << "): not found or failed to load." << std::endl;
            return;
        }
        // A galaxy world is saved as the mission of its place (BD-0119).
        if (const auto galaxy = GalaxyEggbert::Editor::ParseGalaxyWorldPath(path.generic_string()))
            worldRuntime_.OverrideMissionNumber(galaxy->second);

        RebuildWorldPresentation();

        // Start the free-fly camera above the world's own block centroid
        // (render space) -- falls back to a fixed point at the world's
        // own center for a brand-new, all-air world where
        // CentroidX/Y/Z() would otherwise be a meaningless (0,0,0).
        // Render-space (0,15,0) is raw grid (50,15,50), the actual center
        // of the 100^3 world, NOT (50,15,50) in render space (which would
        // be the far corner, just outside bounds).
        if (terrainRenderer_ && terrainRenderer_->BlockCount() > 0)
        {
            worldEditor_.EnterEditing(terrainRenderer_->CentroidX(), terrainRenderer_->CentroidY() + 8.0f,
                                      terrainRenderer_->CentroidZ());
        }
        else
        {
            worldEditor_.EnterEditing(0.0f, 15.0f, 0.0f);
        }
        worldEditor_.SetWorldPath(path);
        worldEditor_.ExitBrowser();

        std::cout << "GalaxyEggbertCNA: LoadCustomWorldForEditing(" << path << "): loaded." << std::endl;
    }

    void GalaxyEggbertGame::LoadCustomWorldForPlayTest(const std::filesystem::path& path)
    {
        titleSceneActive_ = false;
        if (!worldRuntime_.LoadFromVwrFile(path.string()))
        {
            std::cout << "GalaxyEggbertCNA: LoadCustomWorldForPlayTest(" << path
                       << "): not found or failed to load." << std::endl;
            return;
        }
        if (const auto galaxy = GalaxyEggbert::Editor::ParseGalaxyWorldPath(path.generic_string()))
            worldRuntime_.OverrideMissionNumber(galaxy->second);

        StopVehicleMotorSound();
        RebuildWorldPresentation();

        // Fresh defaults, no "preserve lives" concept (unlike LoadMission()) --
        // there's no real progression state to carry between play-test runs.
        interaction_ = InteractionSystem();
        blupi_ = BlupiController();
        // A fresh Blupi knows no BigDecor: give him the solid ones again (M3D-0006). They were set
        // only by RebuildWorldPresentation(), before this reset, so in play every house, tree and
        // statue let him through (BD-0080, the owner's report of 2026-11-06).
        blupi_.SetSolidBoxes(worldRuntime_.SolidBigDecorBoxes());
        wasInHelicopter_ = false;
        hideGhosts_.clear();
        hideGhostTracking_ = false;
        if (worldRuntime_.HasExplicitSpawnPoint())
        {
            PlaceBlupi(
                worldRuntime_.GetSpawnRenderX(), worldRuntime_.GetSpawnRenderY(),
                worldRuntime_.GetSpawnRenderZ());
            blupi_.SetYaw(worldRuntime_.GetSpawnYaw());
        }
        else
        {
            // Preserve the established 3x3 starter-board default for old
            // custom worlds whose formerly-reserved header slots are zero.
            PlaceBlupi(
                GalaxyEggbert::Editor::kEditorStarterSpawnRenderX,
                GalaxyEggbert::Editor::kEditorStarterSpawnY,
                GalaxyEggbert::Editor::kEditorStarterSpawnRenderZ);
        }

        editorPlayTestActive_ = true;
        editorPlayTestWorldPath_ = path;
        SetPhase(GalaxyEggbert::Def::GamePhase::Play, /*bypassFade=*/true);

        std::cout << "GalaxyEggbertCNA: LoadCustomWorldForPlayTest(" << path << "): loaded." << std::endl;
    }

    // ---- xnjttjpo: the world being played in the 3D editor (BD-0118) ----

    void GalaxyEggbertGame::OpenMissionInEditor()
    {
        if (phase_ != GalaxyEggbert::Def::GamePhase::Play) return;
        const int mission = worldRuntime_.GetMissionNumber();
        const std::filesystem::path path = MissionPath(mission);
        std::error_code error;
        if (mission <= 0 || !std::filesystem::exists(path, error))
        {
            std::cout << "GalaxyEggbertCNA: xnjttjpo: mission " << mission << " has no world file to edit." << std::endl;
            return;
        }
        StopVehicleMotorSound();
        editorMission_ = mission;
        editorMissionLives_ = interaction_.Lives();
        // As XMISSION wrote data\worldNNN.blp: the shipped file itself, its backups in the player's
        // folder (a galaxy world's beside it, as the editor's).
        // On the web the shipped worlds are the page's data, in memory: a save is kept in the browser's
        // storage too and laid over them at the next start (main.cpp, ApplyCampaignEdits()).
        // A shipped world's saves are also kept as the edit (on the web laid over the packed worlds
        // at the next start), and the world as shipped is kept first, for Pause's "Original" (on the
        // web ApplyCampaignEdits() kept it already if an edit was laid over it at the start).
        std::filesystem::path keepCopy;
        if (galaxyGamer_ < 0)
        {
            keepCopy = GalaxyEggbert::Editor::CampaignEditsDir();
            GalaxyEggbert::Editor::KeepCampaignOriginal(path, GalaxyEggbert::Editor::CampaignOriginalsDir());
        }
        worldEditor_.BeginOutsideSession(galaxyGamer_ >= 0 ? std::filesystem::path{}
                                                           : GalaxyEggbert::Editor::CampaignBackupsDir(),
                                         keepCopy);
        LoadCustomWorldForEditing(path);
        SetPhase(GalaxyEggbert::Def::GamePhase::Editor, /*bypassFade=*/true);
        std::cout << "GalaxyEggbertCNA: xnjttjpo: editing mission " << mission << " (" << path.generic_string()
                  << ") in the 3D editor; its Stop returns to the mission." << std::endl;
    }

    bool GalaxyEggbertGame::CanRevertWorld() const
    {
        return galaxyGamer_ < 0 && !demoActive_ && !editorPlayTestActive_ && !replayingResume_ &&
               !loadedMissionPath_.empty() && GalaxyEggbert::Editor::HasCampaignEdit(loadedMissionPath_);
    }

    void GalaxyEggbertGame::RevertWorld()
    {
        revertArmed_ = false;
        const int mission = worldRuntime_.GetMissionNumber();
        const bool restored =
            GalaxyEggbert::Editor::RevertCampaignEdit(loadedMissionPath_, GalaxyEggbert::Editor::CampaignOriginalsDir());
        const int lives = interaction_.Lives();
        LoadMission(mission); // from its start, as shipped
        interaction_.SetLives(lives);
        SetPhase(GalaxyEggbert::Def::GamePhase::Play, /*bypassFade=*/true);
        ShowDemoNotice(restored ? "Mission " + std::to_string(mission) + ": the world as shipped again."
                                : "Mission " + std::to_string(mission) + ": the world as shipped from the next start.");
    }

    void GalaxyEggbertGame::LeaveMissionEditor()
    {
        const int mission = std::exchange(editorMission_, 0);
        worldEditor_.EndOutsideSession();
        editorPlayTestActive_ = false;
        SetPhase(GalaxyEggbert::Def::GamePhase::Play, /*bypassFade=*/true);
        interaction_.SetLives(editorMissionLives_);
        LoadMission(mission); // from its start, with what was saved
        std::cout << "GalaxyEggbertCNA: xnjttjpo: back to mission " << mission << "." << std::endl;
    }

    // ---- The player's own galaxy (BD-0119) ----

    std::string GalaxyEggbertGame::MissionPath(int mission) const
    {
        if (galaxyGamer_ >= 0) return GalaxyEggbert::Editor::GalaxyWorldPath(galaxyGamer_, mission).generic_string();
        return MissionWorldPath(mission);
    }

    void GalaxyEggbertGame::StartGalaxy()
    {
        const int gamer = saveData_.GetSelectedGamer();
        std::error_code error;
        if (!std::filesystem::exists(GalaxyEggbert::Editor::GalaxyWorldPath(gamer, 1), error))
        {
            // Nothing to play yet: the editor, on the galaxy, to build its main hub.
            std::cout << "GalaxyEggbertCNA: player " << static_cast<char>('A' + gamer)
                      << "'s galaxy has no main hub yet: the editor's galaxy screen." << std::endl;
            worldEditor_.EnterBrowser(gamer);
            worldEditor_.ShowGalaxy();
            SetPhase(GalaxyEggbert::Def::GamePhase::Editor, /*bypassFade=*/true);
            return;
        }
        galaxyGamer_ = gamer;
        saveData_.SetGalaxyMode(true);
        int mission = saveData_.GetMissionNumber();
        if (!GalaxyEggbert::Editor::IsGalaxyMission(mission) ||
            !std::filesystem::exists(GalaxyEggbert::Editor::GalaxyWorldPath(gamer, mission), error))
            mission = 1;
        const int lives = saveData_.GetHasProgress() ? saveData_.GetLives() : 3;
        saveData_.SetHasProgress(true);
        interaction_.SetLives(lives);
        LoadMission(mission);
        interaction_.SetLives(lives);
        SetPhase(GalaxyEggbert::Def::GamePhase::Play);
        std::cout << "GalaxyEggbertCNA: player " << static_cast<char>('A' + gamer) << "'s galaxy: "
                  << GalaxyEggbert::Editor::GalaxyMissionName(mission) << "." << std::endl;
    }

    void GalaxyEggbertGame::LeaveGalaxy()
    {
        if (galaxyGamer_ < 0) return;
        galaxyGamer_ = -1;
        saveData_.SetGalaxyMode(false);
        std::cout << "GalaxyEggbertCNA: back from the galaxy to the campaign's menu." << std::endl;
    }

    void GalaxyEggbertGame::LoadTitleScene()
    {
        titleSceneRequested_ = false;
        if (!std::filesystem::exists("worlds3d/title.vwr") || !worldRuntime_.LoadFromVwrFile("worlds3d/title.vwr"))
        {
            titleSceneActive_ = false;
            titleSceneUnavailable_ = true; // Wait and the editor's list fall back to their plain screens
            return;
        }
        RebuildWorldPresentation();
        blupi_ = BlupiController();
        // A fresh Blupi knows no BigDecor: give him the solid ones again (M3D-0006). They were set
        // only by RebuildWorldPresentation(), before this reset, so in play every house, tree and
        // statue let him through (BD-0080, the owner's report of 2026-11-06).
        blupi_.SetSolidBoxes(worldRuntime_.SolidBigDecorBoxes());
        if (worldRuntime_.HasExplicitSpawnPoint())
        {
            blupi_.SetPosition(worldRuntime_.GetSpawnRenderX(), worldRuntime_.GetSpawnRenderY(),
                               worldRuntime_.GetSpawnRenderZ());
        }
        // Facing the camera, which stands on the -Z side of the scene
        // (BlupiController moves along (sin yaw, -cos yaw): yaw 0 is -Z).
        blupi_.SetYaw(0.0f);
        titleSceneActive_ = true;
        titleSceneTime_ = 0.0f;
    }

    void GalaxyEggbertGame::UpdateTitleScene(float dt)
    {
        titleSceneTime_ += dt;
        worldRuntime_.Update(dt);
        if (terrainRenderer_)
        {
            if (!replayingResume_)
                terrainRenderer_->Update(getGraphicsDeviceProperty(), worldRuntime_.GetAnimPhase(), BlupiFocusRaw());
        }
        // Blupi idles (Stop's real fidget cycle) where he stands.
        blupi_.Step(worldRuntime_.GetWorld(), 0.0f, 0.0f, false, false, false, dt);
        if (blupiModelLoaded_)
        {
            std::string clipName = blupiModelIsReal_ ? BlupiClipName(blupi_)
                                                     : BlupiAnimStateToPlaceholderClipName(blupi_.GetAnimState());
            if (blupiModel_->Clips.find(clipName) == blupiModel_->Clips.end())
            {
                clipName = MissingClipFallback(blupi_);
            }
            if (clipName != blupiActiveClipName_)
            {
                blupiActiveClipName_ = clipName;
                blupiClipTimeSeconds_ = 0.0;
            }
            blupiClipTimeSeconds_ += static_cast<double>(dt);
        }
        // The camera stands 5 blocks in front of Blupi at his eye height and
        // looks 3.5 degrees up, so he fills the lower middle of the screen
        // and the tile-built title (worlds3d/title.vwr, 56 blocks behind him)
        // the band above the menu panels; it sways 7 degrees either way over
        // 14 seconds (tools/worlds/title.py frames the scene for this).
        constexpr float kSwayRadians = 0.12f;
        constexpr float kSwayPeriodSeconds = 14.0f;
        constexpr float kDistance = 5.0f;
        constexpr float kHeight = 1.0f;
        constexpr float kLookUp = 0.061f; // tan(3.5 degrees)
        const float sway = kSwayRadians * std::sin(titleSceneTime_ * 6.2831853f / kSwayPeriodSeconds);
        const Microsoft::Xna::Framework::Vector3 target(blupi_.GetX(), blupi_.GetY() + kHeight + 40.0f * kLookUp,
                                                        blupi_.GetZ() + 40.0f - kDistance);
        const Microsoft::Xna::Framework::Vector3 eye(blupi_.GetX() + kDistance * std::sin(sway), blupi_.GetY() + kHeight,
                                                     blupi_.GetZ() - kDistance * std::cos(sway));
        camera_.SetLookAt(eye, target);
        cameraTarget_ = target;
    }

    void GalaxyEggbertGame::SetPhase(GalaxyEggbert::Def::GamePhase next, bool bypassFade) noexcept
    {
        using GalaxyEggbert::Def::GamePhase;
        if (next != GamePhase::Pause) demoSlotsShown_ = false;
        if (next != GamePhase::Pause) revertArmed_ = false;
        if (next == GamePhase::Init) galaxyNamesStale_ = true; // a name typed in the editor shows
        // Pause, Win, Lost or a menu end a demo (and its recording) rather than show over it.
        if (next != GamePhase::Play && ((demoActive_ && demoSessionLive_) || demoRecording_))
        {
            demoEndRequested_ = true;
        }
        // The menu is the campaign's: a galaxy being played ends there (BD-0119).
        if (next == GamePhase::Init && !demoActive_) LeaveGalaxy();
        if (next == GamePhase::Init || next == GamePhase::Win || next == GamePhase::Lost || next == GamePhase::Editor)
        {
            if (!replayingResume_) DeleteResumeFile();
            recordingResume_ = false;
            missionStartPending_ = false;
            pendingResume_.reset();
        }
        // Real Game1::SetPhase() (2026-07-13, plan.md MENU-088/089, see
        // this method's own declaration comment for the full real
        // deferred-transition mechanic): only defers when the CURRENT
        // phase is one of the 5 real deferring phases, no fade is already
        // pending, and bypassFade wasn't requested (the real `mission==
        // -2` sentinel). Every other phase commits instantly, matching
        // the real source exactly (confirmed via research: Play->Pause,
        // Play->Win/Lost, and Resume->Play-via-Continue are ALL genuinely
        // instant in the real game, not merely fast).
        const bool sourceDefers = phase_ == GalaxyEggbert::Def::GamePhase::Init ||
                                   phase_ == GalaxyEggbert::Def::GamePhase::MainSetup ||
                                   phase_ == GalaxyEggbert::Def::GamePhase::PlaySetup ||
                                   phase_ == GalaxyEggbert::Def::GamePhase::Pause ||
                                   phase_ == GalaxyEggbert::Def::GamePhase::Resume;
        if (sourceDefers && fadeOutPhase_ == GalaxyEggbert::Def::GamePhase::None && !bypassFade)
        {
            fadeOutPhase_ = next;
            phaseTimeSeconds_ = 0.0f;
            return;
        }

        // Real Game1::SetPhase() resets a per-phase timer (real `phaseTime`,
        // now ported as phaseTimeSeconds_ -- see its own member comment,
        // 2026-07-13) and clears input debounce state on every transition
        // (Game1.cpp:979-1058) -- this engine's own equivalent is just the
        // 2 key-debounce trackers below.
        // Game1.cpp:1023 stops world sound on a phase change. Motor loops belong
        // to the live vehicle, not the menu or the next world.
        // Resume will start the appropriate loop again on its first Play update.
        if (next != GamePhase::Play)
        {
            StopVehicleMotorSound();
            pendingCheatLetters_.clear();
        }
        const GamePhase previous = phase_;
        phase_ = next;
        NetPhaseChanged(previous, next); // the network game's pause and abandon
        editorDevices_.Reset();
        editorCommandKeys_.Reset();
        if (next == GalaxyEggbert::Def::GamePhase::Init && !titleSceneActive_)
        {
            titleSceneRequested_ = true;
        }
        fadeOutPhase_ = GalaxyEggbert::Def::GamePhase::None;
        pauseKeyWasDown_ = false;
        phaseReturnKeyWasDown_ = false;
        phaseTimeSeconds_ = 0.0f;
        inputPad_.ResetTouchState();
    }

    const char* GalaxyEggbertGame::PhaseOverlayMessage() const noexcept
    {
        switch (phase_)
        {
            case GalaxyEggbert::Def::GamePhase::Pause:
                // Handled by inputPad_.DrawPause() instead (2026-07-13,
                // plan.md MENU-028..039) -- the real background/character/
                // buttons, not a generic text overlay.
                return nullptr;
            case GalaxyEggbert::Def::GamePhase::Win:
            case GalaxyEggbert::Def::GamePhase::Lost:
                // Handled by inputPad_.DrawWinLost() instead (2026-07-13,
                // plan.md MENU-046..057) -- the real win.png/lost.png
                // background + blupiyoupie.png animation, not a generic
                // text overlay.
                return nullptr;
            default:
                return nullptr;
        }
    }

}
