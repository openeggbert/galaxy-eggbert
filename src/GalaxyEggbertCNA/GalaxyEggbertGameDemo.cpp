// GalaxyEggbertGame: the resume log, the main-menu demos and the player's recordings. Split out of GalaxyEggbertGame.cpp.
#include "GalaxyEggbertGame.hpp"
#include "GameInternal.hpp"
#include <GalaxyEggbert/Version.hpp>
#include <ctime>

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
#include <thread>
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

    void GalaxyEggbertGame::ReadResumeLog()
    {
        std::error_code error;
        const auto size = std::filesystem::file_size(ResumeLog::Path(), error);
        if (error) return;
        pendingResume_.reset();
        if (size <= 128 * 1024 * 1024)
        {
            std::ifstream in(ResumeLog::Path(), std::ios::binary);
            const std::string text((std::istreambuf_iterator<char>(in)), {});
            ResumeLog log;
            if (in && ResumeLog::Parse(text, ResumeBuild(), log))
            {
                pendingResume_ = std::move(log);
                return;
            }
        }
        DeleteResumeFile();
    }

    void GalaxyEggbertGame::ResetMissionInput()
    {
        positionTrail_.Clear();
        snapPending_ = false;
        pendingCheatLetters_.clear();
        debugVelocity_ = {};
        inputPad_.ResetMissionInput();
        inputPad_.SetJumpRight(saveData_.GetJumpRight());
        controlsHelp_ = {};
        menuConfirmWasDown_ = false;
        jumpKeyWasDown_ = actionKeyWasDown_ = pauseKeyWasDown_ = phaseReturnKeyWasDown_ = false;
        f4KeyWasDown_ = f5KeyWasDown_ = f6KeyWasDown_ = f7KeyWasDown_ = f8KeyWasDown_ = f12KeyWasDown_ = tabKeyWasDown_ = false;
        cameraModeKeyWasDown_ = fullscreenKeyWasDown_ = false;
        turnAroundDownWasHeld_ = false;
        turnAroundRemaining_ = turnAroundStartYaw_ = 0.0f;
        turnAroundDuration_ = 0.3f;
        touchQuietFrames_ = 0;
        mouseLookActive_ = false;
        lastMouseX_ = lastMouseY_ = 0;
        lookYawOffset_ = lookPitchOffset_ = lookVertical_ = 0.0f;
        cheatMenuShown_ = false;
        holdingCrate_ = graceWasInVehicle_ = false;
        wasPushingCrate_ = helicopterDeposited_ = false;
        activeMotorLoop_ = GalaxyEggbert::Def::SoundChannel::SoundChannel0;
        mockeryVariant_ = BlupiController::AnimState::Stop;
        mockeryElapsed_ = 0.0f;
        mockerySounded_ = false;
        reusablePickup_ = {};
        cameraShake_ = CameraShake{};
        chaseZoom_ = 1.0f;
        chaseZoomLingerSeconds_ = 0.0f;
        cheatZoomStep_ = 0;
        bodyPitchRadians_ = 0.0f;
        bodyHasLast_ = false;
        activeCheats_.erase(std::remove_if(activeCheats_.begin(), activeCheats_.end(),
                                           [](const std::string& name) { return name.rfind("zoom", 0) == 0; }),
                            activeCheats_.end());
        accelLastState_ = false;
        accelWaitZero_ = true;
        blupiActiveClipName_.clear();
        blupiClipTimeSeconds_ = 0.0;
        phaseTimeSeconds_ = 0.0f;
        chaseCamera_.Reset(blupi_.GetYaw());
        cameraSmoothedInitialized_ = false;
        cameraTarget_ = Microsoft::Xna::Framework::Vector3(blupi_.GetX(), blupi_.GetY() + 1.0f, blupi_.GetZ());
        camera_.SetLookAt(cameraTarget_ + Microsoft::Xna::Framework::Vector3(0.0f, 1.0f, -5.0f), cameraTarget_);
        sound_.StopAll();
    }

    void GalaxyEggbertGame::StartResumeLog()
    {
        ResetMissionInput();
        resumeLocalCheats_ = {cheatFps_, blupi_.GetCheatSuperBlupi(), cheatPhoto_, cheatFreeze_, cheatXray_, cheatHitbox_, cheatFreecam_, cheatTrail_, cheatIds_, cheatSounds_, cheatChunks_, cheatZoomStep_, activeCheats_};
        backCheckpoints_.clear();
        backPending_ = backReplaying_ = markPending_ = recallPending_ = restartPending_ = false;
        rewindPending_.reset();
        rewindLabelDrawn_ = false;
        markedTick_.reset();
        resumeLog_ = {};
        auto& start = resumeLog_.start;
        start.build = ResumeBuild();
        start.worldPath = loadedMissionPath_;
        start.worldHash = loadedMissionHash_;
        start.mission = worldRuntime_.GetMissionNumber();
        start.lives = interaction_.Lives();
        start.speed = static_cast<int>(gameSpeed_);
        start.seed = missionSeed_;
        start.x = blupi_.GetX(); start.y = blupi_.GetY(); start.z = blupi_.GetZ(); start.yaw = blupi_.GetYaw();
        start.quickCheat = quickCheatEnabled_;
        start.openDoors = cheatDoors_;
        start.showSecret = cheatShowSecret_;
        start.compass = cheatCompass_;
        start.debug = cheatDebug_;
        start.photo = cheatPhoto_;
        start.freeze = cheatFreeze_;
        start.xray = cheatXray_;
        start.hitbox = cheatHitbox_;
        start.trail = cheatTrail_;
        start.ids = cheatIds_;
        start.sounds = cheatSounds_;
        start.chunks = cheatChunks_;
        start.cheatsLocked = inputPad_.IsCheatEntryLockedForRun() ? 2 : inputPad_.IsCheatEntryLocked() ? 1 : 0;
        start.cameraMode = static_cast<int>(cameraMode_);
        std::ostringstream settings;
        saveData_.Write(settings);
        start.saveData = settings.str();
        resumeTicks_ = 0;
        // A galaxy's mission too (BD-0119): its log names the galaxy's file, BeginResume() goes back
        // into the galaxy.
        recordingResume_ = !editorPlayTestActive_ && !loadedMissionPath_.empty();
        missionStartPending_ = false;
        CaptureBackCheckpoint();
    }

    bool GalaxyEggbertGame::BeginResume(ResumeLog log, bool inProcess)
    {
        auto path = MissionWorldPath(log.start.mission);
        int galaxy = -1; // a mission of a player's galaxy (BD-0119) continues there, from its file
        if (log.start.worldPath != path)
        {
            const auto parsed = GalaxyEggbert::Editor::ParseGalaxyWorldPath(log.start.worldPath);
            if (parsed && parsed->second == log.start.mission)
            {
                galaxy = parsed->first;
                path = log.start.worldPath;
            }
        }
        const auto bytes = ReadWorldBytes(path);
        // Validate before replacing the current mission or settings.
        if (!bytes || !log.MatchesWorld(path, ResumeLog::HashBytes(*bytes)))
        {
            std::cout << "GalaxyEggbertCNA: resume world differs or is unreadable; falling back to checkpoint."
                      << std::endl;
            DeleteResumeFile();
            return false;
        }
        WorldRuntime probe;
        std::istringstream world(*bytes, std::ios::in | std::ios::binary);
        if (!probe.LoadFromVwrStream(world))
        {
            DeleteResumeFile();
            return false;
        }
        resumeReplayBegan_ = std::chrono::steady_clock::now();
        resumeReplayTotal_ = log.TickCount();
        resumeLog_ = std::move(log);
        const auto& start = resumeLog_.start;
        replayingResume_ = true;
        recordingResume_ = true;
        sound_.SetReplayMuted(true);
        // Leave the current music playing. Update() does not call UpdateMusic during replay;
        // the same world's track therefore carries on without stopping/restarting at tick zero.
        saveData_ = SaveData{};
        std::istringstream settings(start.saveData);
        saveData_.Read(settings);
        // Back into the galaxy the log was written in, or the campaign (LoadMission() reads its file
        // and its progress by these).
        galaxyGamer_ = galaxy;
        saveData_.SetGalaxyMode(galaxy >= 0);
        sound_.SetEnabled(saveData_.GetSoundEnabled());
        sound_.SetEffectsVolume(static_cast<float>(saveData_.GetEffectsVolume()) / 100.0f);
        quickCheatEnabled_ = start.quickCheat;
        cheatDoors_ = start.openDoors;
        cheatShowSecret_ = start.showSecret;
        cheatCompass_ = start.compass && GalaxyEggbert::Game::kCheatsEnabled;
        cheatDebug_ = start.debug && GalaxyEggbert::Game::kCheatsEnabled;
        cheatPhoto_ = start.photo && GalaxyEggbert::Game::kCheatsEnabled;
        cheatFreeze_ = start.freeze && GalaxyEggbert::Game::kCheatsEnabled;
        cheatXray_ = start.xray && GalaxyEggbert::Game::kCheatsEnabled;
        cheatHitbox_ = start.hitbox && GalaxyEggbert::Game::kCheatsEnabled;
        cheatTrail_ = start.trail && kCheatsEnabled;
        cheatIds_ = start.ids && kCheatsEnabled;
        cheatSounds_ = start.sounds && kCheatsEnabled;
        cheatChunks_ = start.chunks && kCheatsEnabled;
        inputPad_.RestoreCheatEntryLock(start.cheatsLocked != 0 && GalaxyEggbert::Game::kCheatsEnabled,
                                        start.cheatsLocked == 2 && GalaxyEggbert::Game::kCheatsEnabled);
        cameraMode_ = static_cast<CameraMode>(start.cameraMode);
        gameSpeed_ = GalaxyEggbert::Def::ToGameSpeed(start.speed);
        editorPlayTestActive_ = false;
        LoadMission(start.mission, &*bytes);
        interaction_.SetLives(start.lives);
        PlaceBlupi(start.x, start.y, start.z);
        blupi_.SetYaw(start.yaw);
        SetPhase(GalaxyEggbert::Def::GamePhase::Play, true);
        ResetMissionInput();
        activeCheats_.erase(std::remove(activeCheats_.begin(), activeCheats_.end(), "compass"), activeCheats_.end());
        if (cheatCompass_) activeCheats_.push_back("compass");
        activeCheats_.erase(std::remove(activeCheats_.begin(), activeCheats_.end(), "debug"), activeCheats_.end());
        if (cheatDebug_) activeCheats_.push_back("debug");
        activeCheats_.erase(std::remove(activeCheats_.begin(), activeCheats_.end(), "fairplay"), activeCheats_.end());
        activeCheats_.erase(std::remove(activeCheats_.begin(), activeCheats_.end(), "honest"), activeCheats_.end());
        if (inputPad_.IsCheatEntryLocked())
            activeCheats_.push_back(inputPad_.IsCheatEntryLockedForRun() ? "honest" : "fairplay");
        if (inProcess)
        {
            cheatFps_ = resumeLocalCheats_.fps;
            cheatPhoto_ = resumeLocalCheats_.photo;
            cheatFreeze_ = resumeLocalCheats_.freeze;
            cheatXray_ = resumeLocalCheats_.xray;
            cheatHitbox_ = resumeLocalCheats_.hitbox;
            cheatFreecam_ = resumeLocalCheats_.freecam;
            cheatTrail_ = resumeLocalCheats_.trail;
            cheatIds_ = resumeLocalCheats_.ids;
            cheatSounds_ = resumeLocalCheats_.sounds;
            cheatChunks_ = resumeLocalCheats_.chunks;
            cheatZoomStep_ = resumeLocalCheats_.zoomStep;
            activeCheats_ = resumeLocalCheats_.active;
            blupi_.SetCheatSuperBlupi(resumeLocalCheats_.superBlupi);
        }
        else
        {
            markedTick_.reset();
            std::erase(activeCheats_, "photo");
            std::erase(activeCheats_, "freeze");
            std::erase(activeCheats_, "xray");
            std::erase(activeCheats_, "hitbox");
            if (cheatPhoto_) activeCheats_.emplace_back("photo");
            if (cheatFreeze_) activeCheats_.emplace_back("freeze");
            if (cheatXray_) activeCheats_.emplace_back("xray");
            if (cheatHitbox_) activeCheats_.emplace_back("hitbox");
            for (const auto* name : {"trail", "ids", "sounds", "chunks"}) std::erase(activeCheats_, name);
            if (cheatTrail_) activeCheats_.emplace_back("trail");
            if (cheatIds_) activeCheats_.emplace_back("ids");
            if (cheatSounds_) activeCheats_.emplace_back("sounds");
            if (cheatChunks_) activeCheats_.emplace_back("chunks");
            // A Continue becomes the start of a new in-process rewind history, with exactly
            // today's fresh-process defaults for state absent from the file format.
            resumeLocalCheats_ = {cheatFps_, blupi_.GetCheatSuperBlupi(), cheatPhoto_, cheatFreeze_, cheatXray_, cheatHitbox_, cheatFreecam_, cheatTrail_, cheatIds_, cheatSounds_, cheatChunks_, cheatZoomStep_, activeCheats_};
            backCheckpoints_.clear();
        }
        replayRun_ = 0;
        replayRepeat_ = 0;
        resumeTicks_ = 0;
        // This frame's Draw() draws nothing (the world is mid-replay): keep it from being presented,
        // as Update() does for the replay's first 0.25 s.
        SuppressDraw();
        return true;
    }

    void GalaxyEggbertGame::ReplayBatch()
    {
        const auto batchStart = std::chrono::steady_clock::now();
        const char* slow = std::getenv("GALAXY_REPLAY_DELAY_MS"); // capture-only QA knob, per batch
        const int delay = slow ? std::clamp(std::atoi(slow), 0, 100) : 0;
        if (delay) std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        for (int i = 0; i < 600 && replayRun_ < resumeLog_.runs.size(); ++i)
        {
            const auto& run = resumeLog_.runs[replayRun_];
            tickInput_ = run.input;
            UpdateTick();
            ++resumeTicks_;
            if (++replayRepeat_ == run.count)
            {
                replayRepeat_ = 0;
                ++replayRun_;
            }
            // At least one tick, at most 600 or about 12 ms. CNA's fixed-step catch-up loop
            // consumes finite clock debt and draws if any Update does not suppress it.
            if (std::chrono::steady_clock::now() - batchStart >= std::chrono::milliseconds(12)) break;
        }
        if (replayRun_ != resumeLog_.runs.size()) return;
        replayingResume_ = false;
        // "honest" typed in this run after the log was written: its cheats off again, locked.
        if (noCheatsForRun_ && !inputPad_.IsCheatEntryLockedForRun()) TurnCheatsOff(/*forRun=*/true);
        sound_.SetReplayMuted(false);
        resumeAccelWarmStart_ = true;
        activeMotorLoop_ = GalaxyEggbert::Def::SoundChannel::SoundChannel0;
        wasPushingCrate_ = false;
        // The first visible frame must not interpolate from the abandoned future (at >60 Hz).
        tickClock_ = 0.0;
        drawAlpha_ = 1.0f;
        previousEye_ = camera_.Position();
        previousTarget_ = cameraTarget_;
        previousBlupi_ = Microsoft::Xna::Framework::Vector3(blupi_.GetX(), blupi_.GetY(), blupi_.GetZ());
        previousObjects_.clear();
        RefreshTerrainMeshIfChanged();
        if (backReplaying_)
        {
            backReplaying_ = false;
            ShowDemoNotice(rewindNotice_);
            if (std::getenv("GALAXY_BACK_CHECKPOINTS"))
            {
                const auto expected = backCheckpoints_.find(resumeTicks_);
                const bool equal = expected != backCheckpoints_.end() && expected->second == ResumeCheckSignature();
                const std::string line = "BACK SELFTEST " + std::string(equal ? "equal" : "different") +
                    " at tick " + std::to_string(resumeTicks_) + "\n";
                std::cout << line << std::flush;
                if (autoplayTrace_) { std::fputs(line.c_str(), autoplayTrace_); std::fflush(autoplayTrace_); }
                if (!equal)
                {
                    if (expected != backCheckpoints_.end())
                        for (const auto& [name, value] : ResumeCheckSignature())
                            if (expected->second.at(name) != value)
                                std::cout << "BACK difference " << name << ": " << expected->second.at(name)
                                          << " -> " << value << std::endl;
                    autoplayExitCode_ = 1;
                    Exit();
                }
            }
            backCheckpoints_.erase(backCheckpoints_.upper_bound(resumeTicks_), backCheckpoints_.end());
        }
        if (!resumeSelftestSignature_.empty())
        {
            const auto actual = ResumeCheckSignature();
            std::string different;
            for (const auto& [name, value] : resumeSelftestSignature_)
            {
                if (actual.at(name) != value)
                {
                    if (!different.empty()) different += ", ";
                    different += name;
                }
            }
            const std::string line = different.empty() ? "RESUME SELFTEST equal\n"
                                                       : "RESUME SELFTEST different: " + different + "\n";
            std::cout << line << std::flush;
            if (autoplayTrace_) { std::fputs(line.c_str(), autoplayTrace_); std::fflush(autoplayTrace_); }
            resumeSelftestSignature_.clear();
            if (!different.empty()) { autoplayExitCode_ = 1; Exit(); }
        }
    }

    std::map<std::string, std::string> GalaxyEggbertGame::ResumeSignature() const
    {
        std::map<std::string, std::string> result;
        result["cheat input"] = std::string(inputPad_.IsCheatEntryLocked() ? "locked" : "allowed") +
                                (inputPad_.ChosenTypedCheat() >= 0
                                     ? " chosen " + std::to_string(inputPad_.ChosenTypedCheat())
                                     : "");
        std::ostringstream out;
        out << std::setprecision(std::numeric_limits<float>::max_digits10);
        out << cheatDebug_;
        for (const float speed : debugVelocity_) out << ' ' << speed;
        result["debug overlay"] = out.str(); out.str("");
        out << blupi_.GetX() << ' ' << blupi_.GetY() << ' ' << blupi_.GetZ() << ' ' << blupi_.GetYaw()
            << ' ' << blupi_.GetVelocityY() << ' ' << blupi_.GetVehicleSpeed() << ' '
            << static_cast<int>(blupi_.GetAnimState()) << ' ' << blupi_.GetRawAnimIcon() << ' '
            << static_cast<int>(blupi_.GetVehicleMode()) << ' ' << static_cast<int>(blupi_.GetSecretPower())
            << ' ' << blupi_.GetTimeShield() << ' ' << blupi_.IsGhost() << ' ' << blupi_.IsInverted()
            << ' ' << blupi_.IsDeathLocked() << ' ' << blupi_.IsOnGround() << ' ' << blupi_.GetWaterGaugeLevel();
        for (float speed : blupi_.GetMovementSignature()) out << ' ' << speed;
        out << ' ' << blupi_.IsBallooned() << ' ' << blupi_.IsEcrased() << ' ' << blupi_.IsSuspended();
        result["Blupi"] = out.str(); out.str("");
        out << interaction_.TreasuresCollected() << ' ' << interaction_.LifeEggCount() << ' ' << interaction_.Lives()
            << ' ' << interaction_.Key1Count() << ' ' << interaction_.Key2Count() << ' ' << interaction_.Key3Count()
            << ' ' << interaction_.DynamiteCount() << ' ' << interaction_.ExitReached();
        result["counters"] = out.str(); out.str("");
        std::size_t index = 0;
        for (const auto& object : worldRuntime_.GetMobileObjects())
        {
            if (object.active)
                out << index << ':' << static_cast<int>(object.type) << ' ' << object.currentX << ' '
                    << object.currentY << ' ' << object.currentZ << ' ' << object.phase << ' '
                    << object.patrolStep << ' ' << object.patrolTime << '\n';
            ++index;
        }
        result["objects"] = out.str(); out.str("");
        const auto& world = worldRuntime_.GetWorld();
        std::uint64_t hash = 14695981039346656037ull;
        for (int z = 0; z < world.blocksPerAxis(); ++z)
            for (int y = 0; y < world.blocksPerAxis(); ++y)
                for (int x = 0; x < world.blocksPerAxis(); ++x)
                    hash = (hash ^ world.getBlock(x, y, z).rawValue()) * 1099511628211ull;
        result["blocks"] = std::to_string(hash);
        out << static_cast<int>(phase_) << ' ' << static_cast<int>(fadeOutPhase_) << ' ' << phaseTimeSeconds_
            << ' ' << worldRuntime_.GetAnimPhase() << ' ' << static_cast<int>(gameSpeed_);
        result["clocks"] = out.str(); out.str("");
        const auto eye = camera_.Position();
        out << eye.X << ' ' << eye.Y << ' ' << eye.Z << ' ' << cameraTarget_.X << ' ' << cameraTarget_.Y
            << ' ' << cameraTarget_.Z << ' ' << lookYawOffset_ << ' ' << lookPitchOffset_ << ' ' << chaseZoom_;
        result["camera"] = out.str(); out.str("");
        for (const auto& ghost : hideGhosts_)
            out << ghost.clip << ' ' << ghost.clipSeconds << ' ' << ghost.ageTicks << '\n';
        result["Hide trail"] = out.str();
        return result;
    }

    void GalaxyEggbertGame::CheckResumeSelftest()
    {
        const char* at = std::getenv("GALAXY_RESUME_SELFTEST");
        if (resumeSelftestDone_ || !at || resumeTicks_ != std::strtoull(at, nullptr, 10)) return;
        resumeSelftestDone_ = true;
        resumeSelftestSignature_ = ResumeCheckSignature();
        ResumeLog parsed;
        if (!ResumeLog::Parse(resumeLog_.Serialize(), ResumeBuild(), parsed) || !BeginResume(std::move(parsed), true))
        {
            const char* line = "RESUME SELFTEST different: log could not be restored\n";
            std::cout << line << std::flush;
            if (autoplayTrace_) { std::fputs(line, autoplayTrace_); std::fflush(autoplayTrace_); }
            autoplayExitCode_ = 1;
            Exit();
        }
    }

    float GalaxyEggbertGame::ResumeReplaySeconds() const
    {
        return std::chrono::duration<float>(std::chrono::steady_clock::now() - resumeReplayBegan_).count();
    }

    void GalaxyEggbertGame::CaptureBackCheckpoint()
    {
        const char* checkpoints = std::getenv("GALAXY_BACK_CHECKPOINTS");
        if (!checkpoints) return;
        std::istringstream list(checkpoints);
        std::uint64_t tick;
        while (list >> tick)
        {
            if (tick == resumeTicks_) backCheckpoints_[tick] = ResumeCheckSignature();
            if (list.peek() == ',') list.get();
        }
    }

    std::map<std::string, std::string> GalaxyEggbertGame::ResumeCheckSignature() const
    {
        auto result = ResumeSignature(); // keep shipped demo signatures unchanged
        std::ostringstream out;
        out << cheatTrail_ << ' ' << cheatIds_ << ' ' << cheatSounds_ << ' ' << cheatChunks_ << ' ' << cheatFps_ << ' ' << cheatZoomStep_ << ' ' << blupi_.GetCheatSuperBlupi() << ' '
            << quickCheatEnabled_ << ' ' << cheatDoors_ << ' ' << cheatShowSecret_ << ' '
            << cheatCompass_ << ' ' << cheatPhoto_ << ' ' << cheatFreeze_ << ' ' << cheatFreecam_ << ' ' << cheatXray_ << ' ' << cheatHitbox_ << ' ' << inputPad_.IsCheatEntryLockedForRun() << ' ' << static_cast<int>(cameraMode_);
        for (const auto& name : activeCheats_) out << ' ' << std::quoted(name);
        result["cheat toggles"] = out.str();
        out.str("");
        out << std::setprecision(std::numeric_limits<float>::max_digits10)
            << freecamYaw_ << ' ' << freecamPitch_ << ' '
            << freecamReturnEye_.X << ' ' << freecamReturnEye_.Y << ' ' << freecamReturnEye_.Z << ' '
            << freecamReturnTarget_.X << ' ' << freecamReturnTarget_.Y << ' ' << freecamReturnTarget_.Z;
        result["free camera"] = out.str();
        out.str("");
        for (const auto& p : positionTrail_.Points()) out << p[0] << ' ' << p[1] << ' ' << p[2] << '\n';
        result["position trail"] = out.str();
        result["cheat keyboard"] = inputPad_.CheatKeys().StateSignature();
        result["controls help"] = std::to_string(controlsHelp_.shown) + " " +
            std::to_string(controlsHelp_.cheats) + " " + std::to_string(controlsHelp_.page);
        return result;
    }

    const char* GalaxyEggbertGame::TimelineCheatRefusal() const
    {
        if (replayingResume_) return "replay";
        if (demoActive_ || demoRecording_ || demoRecordArmed_) return "demo";
        if (editorPlayTestActive_) return "editor play-test";
        if (NetMatchActive()) return "network match";
        if (phase_ != GalaxyEggbert::Def::GamePhase::Play ||
            fadeOutPhase_ != GalaxyEggbert::Def::GamePhase::None ||
            interaction_.ExitReached() || winCheerActive_) return "mission ending or not in Play";
        if (!recordingResume_ || missionStartPending_) return "no mission log";
        return nullptr;
    }

    void GalaxyEggbertGame::ArmRewind(std::uint64_t target, const char* notice)
    {
        if (autoplay_)
        {
            RewindToTick(target, notice);
            return;
        }
        rewindPending_ = PendingRewind{target, notice};
        rewindLabelDrawn_ = false;
        rewindWaitUpdates_ = 0;
    }

    void GalaxyEggbertGame::RewindToTick(std::uint64_t target, const char* notice)
    {
        auto log = resumeLog_;
        target = std::min(target, log.TickCount());
        const auto removed = log.DropLastTicks(log.TickCount() - target);
        if (BeginResume(std::move(log), true))
        {
            backReplaying_ = true;
            rewindNotice_ = notice;
            if (markedTick_ && target < *markedTick_) markedTick_.reset();
            std::cout << "GalaxyEggbertCNA: " << notice << " removed " << removed << " ticks, replay to " << target << "." << std::endl;
        }
        else ShowDemoNotice(std::string(notice) + ": world changed; cannot rewind");
    }

    // ---- Main-menu demos (DemoReplay.hpp) ----

    void GalaxyEggbertGame::EnableDemoRecording(const std::string& path, int maxTicks, const std::string& title)
    {
        demoRecordPath_ = path;
        demoRecordMaxTicks_ = std::max(0, maxTicks);
        demoRecordTitle_ = title;
        demoRecordArmed_ = !path.empty();
    }

    void GalaxyEggbertGame::EnableDemoVerify(const std::string& path)
    {
        demoVerifyPath_ = path;
        demoVerifyPending_ = demoVerifying_ = true;
        // One fixed step per tick as fast as the machine goes, undrawn (UpdateDemo()).
        setIsFixedTimeStepProperty(false);
    }

    void GalaxyEggbertGame::SavePlayerData()
    {
        // A resume reconstruction replays settings already saved; a demo is not the player's game.
        if (replayingResume_ || demoActive_) return;
        saveData_.Save();
    }

    int GalaxyEggbertGame::CountDemos() const
    {
        // The ten positions (the original's demo200 ... while files exist); an empty one is skipped.
        return GalaxyEggbert::Game::kDemoSlotCount;
    }

    std::string GalaxyEggbertGame::DemoSlotPath(int slot) const
    {
        const auto own = GalaxyEggbert::Game::UserDemoPath(slot);
        std::error_code error;
        return std::filesystem::exists(own, error) ? own : GalaxyEggbert::Game::DemoPath(slot);
    }

    GalaxyEggbert::Game::DemoLiveInput GalaxyEggbertGame::ReadDemoLiveInput(bool fromTickInput)
    {
        using namespace Microsoft::Xna::Framework::Input;
        using Touch::TouchLocationState;
        GalaxyEggbert::Game::DemoLiveInput live;
        live.keyEvents.swap(demoKeyEvents_);
        const auto addTouch = [&live](int state) {
            if (state == static_cast<int>(TouchLocationState::Pressed))
            {
                ++live.touches;
                ++live.touchPresses;
            }
            else if (state == static_cast<int>(TouchLocationState::Moved))
            {
                ++live.touches;
            }
        };
        if (fromTickInput)
        {
            // The menu's own live capture (CaptureInput()).
            live.keys = tickInput_.keys;
            live.mouseX = tickInput_.mouseX;
            live.mouseY = tickInput_.mouseY;
            live.wheel = tickInput_.wheel;
            live.mouseButtons = tickInput_.buttons;
            for (const auto& touch : tickInput_.touches) addTouch(touch.state);
        }
        else
        {
            // During a demo the game reads the recording; the player is read here only.
            for (const auto key : Keyboard::GetState().GetPressedKeys()) live.keys.push_back(static_cast<int>(key));
            std::sort(live.keys.begin(), live.keys.end());
            const auto mouse = Mouse::GetState();
            live.mouseX = mouse.getXProperty();
            live.mouseY = mouse.getYProperty();
            live.wheel = mouse.getScrollWheelValueProperty();
            const ButtonState buttons[] = {mouse.getLeftButtonProperty(), mouse.getMiddleButtonProperty(),
                mouse.getRightButtonProperty(), mouse.getXButton1Property(), mouse.getXButton2Property()};
            for (unsigned i = 0; i < 5; ++i)
                if (buttons[i] == ButtonState::Pressed) live.mouseButtons |= 1u << i;
            for (const auto& touch : Touch::TouchPanel::GetState()) addTouch(static_cast<int>(touch.getStateProperty()));
        }
        live.padButtons = GalaxyEggbert::Game::GamepadKeys(ReadGamepadFrame());
        return live;
    }

    bool GalaxyEggbertGame::StartDemo(int number)
    {
        // The player's recording of this position (BD-0116), else -- or when it cannot be played --
        // the shipped demo.
        const auto own = GalaxyEggbert::Game::UserDemoPath(number);
        std::error_code error;
        if (std::filesystem::exists(own, error) && StartDemoFile(own, number)) return true;
        return StartDemoFile(GalaxyEggbert::Game::DemoPath(number), number);
    }

    std::string GalaxyEggbertGame::ResolveDemoWorld(GalaxyEggbert::Game::DemoRecording& demo, int& galaxy) const
    {
        using GalaxyEggbert::Game::ResumeLog;
        auto& start = demo.log.start;
        galaxy = -1;
        const auto campaign = MissionWorldPath(start.mission);
        if (start.worldPath == campaign || !GalaxyEggbert::Editor::IsGalaxyMission(start.mission)) return campaign;
        auto named = GalaxyEggbert::Editor::ParseGalaxyWorldPath(start.worldPath);
        // An embedded galaxy can outlive its original user folder (or come from another PC).
        // Recover its galaxy context from the portable suffix, without loading that old path.
        if (!named && demo.worldBytes)
        {
            for (int g = 0; g < 3; ++g)
            {
                const auto local = GalaxyEggbert::Editor::GalaxyWorldPath(g, start.mission).generic_string();
                const auto suffix = local.substr(local.rfind("/gamer"));
                if (start.worldPath.ends_with(suffix)) named = std::make_pair(g, start.mission);
            }
        }
        std::vector<int> players;
        if (named && named->second == start.mission) players.push_back(named->first);
        for (int g = 0; g < 3; ++g)
            if (std::find(players.begin(), players.end(), g) == players.end()) players.push_back(g);
        for (const int g : players)
        {
            const auto path = GalaxyEggbert::Editor::GalaxyWorldPath(g, start.mission).generic_string();
            const auto bytes = ReadWorldBytes(path);
            if (bytes && ResumeLog::HashBytes(*bytes) == start.worldHash)
            {
                galaxy = g;
                start.worldPath = path;
                return path;
            }
        }
        if (named && named->second == start.mission)
        {
            galaxy = named->first; // its world changed or is gone: reported as such
            return start.worldPath;
        }
        return campaign;
    }

    bool GalaxyEggbertGame::StartDemoFile(const std::string& path, int number, bool collection)
    {
        using namespace GalaxyEggbert::Game;
        // Everything is checked before anything of the player's is touched.
        DemoRecording demo;
        const auto text = ReadWorldBytes(path);
        auto status = text ? ParseDemo(*text, kDemoSimulationRevision, demo, collection) : DemoLoadStatus::Missing;
        std::optional<std::string> world;
        std::string worldPath;
        bool ownWorld = false;
        int demoGalaxy = -1; // a recording made in a player's galaxy (BD-0119) plays from its file
        if (status == DemoLoadStatus::Ok)
        {
            worldPath = ResolveDemoWorld(demo, demoGalaxy);
            world = ReadWorldBytes(worldPath);
            status = DemoWorldStatus(demo, worldPath, world);
            if (status == DemoLoadStatus::OwnWorld)
            {
                const auto cached = CacheDemoWorld(demo);
                if (cached)
                {
                    worldPath = *cached;
                    world = demo.worldBytes;
                    ownWorld = true;
                    status = DemoLoadStatus::Ok;
                }
                else status = DemoLoadStatus::WorldMissing;
            }
            if (status == DemoLoadStatus::Ok)
            {
                WorldRuntime probe;
                std::istringstream stream(*world, std::ios::in | std::ios::binary);
                if (!probe.LoadFromVwrStream(stream)) status = DemoLoadStatus::WorldMissing;
            }
        }
        if (status != DemoLoadStatus::Ok)
        {
            if (collection) demoSlotsMessage_ = Tr(DemoLoadStatusText(status));
            std::cout << "GalaxyEggbertCNA: demo " << path << " skipped: " << DemoLoadStatusText(status) << "."
                      << std::endl;
            return false;
        }

        if (!demoActive_)
        {
            // The player's own state, put back by StopDemo().
            demoSaved_.saveData = saveData_;
            demoSaved_.cameraMode = static_cast<int>(cameraMode_);
            demoSaved_.gameSpeed = gameSpeed_;
            demoSaved_.quickCheat = quickCheatEnabled_;
            demoSaved_.openDoors = cheatDoors_;
            demoSaved_.showSecret = cheatShowSecret_;
            demoSaved_.compass = cheatCompass_;
            demoSaved_.debug = cheatDebug_;
            demoSaved_.photo = cheatPhoto_;
            demoSaved_.freeze = cheatFreeze_;
            demoSaved_.xray = cheatXray_;
            demoSaved_.hitbox = cheatHitbox_;
            demoSaved_.trail = cheatTrail_;
            demoSaved_.ids = cheatIds_;
            demoSaved_.sounds = cheatSounds_;
            demoSaved_.chunks = cheatChunks_;
            demoSaved_.cheatsLocked = inputPad_.IsCheatEntryLocked();
            demoSaved_.cheatsLockedForRun = inputPad_.IsCheatEntryLockedForRun();
            demoSaved_.lives = interaction_.Lives();
            demoSaved_.activeCheats = activeCheats_;
            demoSaved_.galaxyGamer = galaxyGamer_;
        }
        demoCollectionPlayback_ = collection;
        demoActive_ = true;
        demoSessionLive_ = demoEndRequested_ = demoStopRequested_ = menuInputLatched_ = false;
        demo_ = std::move(demo);
        demoNumber_ = number;
        demoRun_ = 0;
        demoRepeat_ = 0;
        demoTicks_ = 0;
        const auto& start = demo_.log.start;

        // The recording's profile (its hub doors, last world, controls), heard and drawn with the
        // player's own sound, volumes and display settings. Never saved.
        SaveData profile;
        std::istringstream settings(start.saveData);
        profile.Read(settings);
        const auto& own = demoSaved_.saveData;
        profile.SetSoundEnabled(own.GetSoundEnabled());
        profile.SetEffectsVolume(own.GetEffectsVolume());
        profile.SetMusicVolume(own.GetMusicVolume());
        profile.SetTerrainRelief(own.GetTerrainRelief());
        profile.SetEffectsLevel(own.GetEffectsLevel());
        profile.SetFullscreen(own.GetFullscreen());
        profile.SetGalaxyMode(demoGalaxy >= 0);
        saveData_ = profile;
        galaxyGamer_ = demoGalaxy;
        sound_.SetReplayMuted(demoVerifying_);
        sound_.SetEnabled(saveData_.GetSoundEnabled());
        sound_.SetEffectsVolume(static_cast<float>(saveData_.GetEffectsVolume()) / 100.0f);
        ApplyMusicVolume();

        // The recorded start, as BeginResume() sets up a resume's.
        quickCheatEnabled_ = start.quickCheat;
        cheatDoors_ = start.openDoors;
        cheatShowSecret_ = start.showSecret;
        cheatCompass_ = start.compass && GalaxyEggbert::Game::kCheatsEnabled;
        cheatDebug_ = start.debug && GalaxyEggbert::Game::kCheatsEnabled;
        cheatPhoto_ = start.photo && GalaxyEggbert::Game::kCheatsEnabled;
        cheatFreeze_ = start.freeze && GalaxyEggbert::Game::kCheatsEnabled;
        cheatXray_ = start.xray && GalaxyEggbert::Game::kCheatsEnabled;
        cheatHitbox_ = start.hitbox && GalaxyEggbert::Game::kCheatsEnabled;
        cheatTrail_ = start.trail && kCheatsEnabled;
        cheatIds_ = start.ids && kCheatsEnabled;
        cheatSounds_ = start.sounds && kCheatsEnabled;
        cheatChunks_ = start.chunks && kCheatsEnabled;
        inputPad_.RestoreCheatEntryLock(start.cheatsLocked != 0 && GalaxyEggbert::Game::kCheatsEnabled,
                                        start.cheatsLocked == 2 && GalaxyEggbert::Game::kCheatsEnabled);
        cameraMode_ = static_cast<CameraMode>(start.cameraMode);
        gameSpeed_ = GalaxyEggbert::Def::GameSpeed::Normal;
        editorPlayTestActive_ = false;
        controlsHelp_ = {};
        cheatMenuShown_ = false;
        LoadMission(start.mission, &*world, &worldPath); // the demo's seed, nothing saved, no resume log
        interaction_.SetLives(start.lives);
        PlaceBlupi(start.x, start.y, start.z);
        blupi_.SetYaw(start.yaw);
        SetPhase(GalaxyEggbert::Def::GamePhase::Play, /*bypassFade=*/true);
        ResetMissionInput();
        activeCheats_.clear();
        if (cheatPhoto_) activeCheats_.emplace_back("photo");
        if (cheatFreeze_) activeCheats_.emplace_back("freeze");
        if (cheatXray_) activeCheats_.emplace_back("xray");
        if (cheatHitbox_) activeCheats_.emplace_back("hitbox");
        if (cheatTrail_) activeCheats_.emplace_back("trail");
        if (cheatIds_) activeCheats_.emplace_back("ids");
        if (cheatSounds_) activeCheats_.emplace_back("sounds");
        if (cheatChunks_) activeCheats_.emplace_back("chunks");
        if (cheatCompass_) activeCheats_.push_back("compass");
        if (cheatDebug_) activeCheats_.push_back("debug");
        if (inputPad_.IsCheatEntryLocked()) activeCheats_.push_back("fairplay");
        // What is held as it starts (the click on Demo, say) cannot stop it.
        if (!demoVerifying_) demoLastLive_ = ReadDemoLiveInput(false);
        demoSessionLive_ = true;
        if (ownWorld) ShowDemoNotice(std::string(Tr(DemoLoadStatusText(DemoLoadStatus::OwnWorld))));
        std::cout << "GalaxyEggbertCNA: demo " << path << " \"" << demo_.title << "\": mission " << start.mission
                  << ", " << demo_.log.TickCount() << " ticks." << std::endl;
        return true;
    }

    bool GalaxyEggbertGame::StartNextDemo()
    {
        for (const int number : GalaxyEggbert::Game::DemoOrderAfter(demoNumber_, CountDemos()))
        {
            if (StartDemo(number)) return true;
        }
        return false;
    }

    bool GalaxyEggbertGame::DemoTick()
    {
        if (demoRun_ >= demo_.log.runs.size())
        {
            EndOfDemo();
            return false;
        }
        const auto& run = demo_.log.runs[demoRun_];
        const std::uint32_t count = run.count;
        tickInput_ = run.input;
        try
        {
            UpdateTick();
        }
        catch (const std::exception& error)
        {
            // A failure inside the replayed game takes the player back to the menu, never further.
            std::cout << "GalaxyEggbertCNA: demo " << demoNumber_ << " failed: " << error.what() << std::endl;
            if (demoVerifying_)
            {
                std::cout << "DEMO VERIFY " << demoVerifyPath_ << ": failed" << std::endl;
                autoplayExitCode_ = 1;
                Exit();
                return false;
            }
            StopDemo();
            return false;
        }
        ++demoTicks_;
        if (++demoRepeat_ == count)
        {
            demoRepeat_ = 0;
            ++demoRun_;
        }
        if (demoEndRequested_ || demoRun_ >= demo_.log.runs.size())
        {
            EndOfDemo();
            return false;
        }
        return true;
    }

    void GalaxyEggbertGame::UpdateDemo()
    {
        if (demoVerifying_)
        {
            for (int i = 0; i < 600 && DemoTick(); ++i) {}
            SuppressDraw();
            return;
        }
        // TreatEvent(): a new key or button stops the demo, a mouse movement does not.
        const auto live = ReadDemoLiveInput(false);
        const bool pressed = GalaxyEggbert::Game::DemoNewPress(demoLastLive_, live);
        demoLastLive_ = live;
        if (pressed || demoStopRequested_ || !getIsActiveProperty())
        {
            StopDemo();
            return;
        }
        DemoTick();
    }

    void GalaxyEggbertGame::EndOfDemo()
    {
        using namespace GalaxyEggbert::Game;
        const auto different = DemoSignatureDifferences(demo_.endSignature, HashDemoSignature(ResumeSignature()));
        const bool complete = demoRun_ >= demo_.log.runs.size();
        std::string differences;
        if (!complete) differences = "stopped after tick " + std::to_string(demoTicks_);
        for (const auto& name : different) differences += (differences.empty() ? "" : ", ") + name;
        if (demoVerifying_)
        {
            std::cout << "DEMO VERIFY " << demoVerifyPath_ << ": "
                      << (differences.empty() ? "equal" : "different: " + differences) << " (" << demoTicks_
                      << " ticks)" << std::endl;
            autoplayExitCode_ = differences.empty() ? 0 : 1;
            Exit();
            return;
        }
        if (differences.empty())
            std::cout << "GalaxyEggbertCNA: demo " << demoNumber_ << " finished." << std::endl;
        else
            std::cout << "GalaxyEggbertCNA: demo " << demoNumber_ << " ended unlike its recording (" << differences
                      << "); tools/verify_demos.sh checks the demos." << std::endl;
        if (demoCollectionPlayback_)
        {
            const bool changed = !differences.empty();
            StopDemo();
            demoSlotsMessage_ = changed ? std::string(Tr("It played differently than it was recorded.")) : "";
            return;
        }
        // DemoStep(): the next demo, the first again after the last.
        if (!StartNextDemo()) StopDemo();
    }

    void GalaxyEggbertGame::StopDemo()
    {
        if (!demoActive_) return;
        demoSessionLive_ = demoEndRequested_ = demoStopRequested_ = false;
        StopVehicleMotorSound();
        sound_.StopAll();
        saveData_ = demoSaved_.saveData;
        cameraMode_ = static_cast<CameraMode>(demoSaved_.cameraMode);
        gameSpeed_ = demoSaved_.gameSpeed;
        quickCheatEnabled_ = demoSaved_.quickCheat;
        cheatDoors_ = demoSaved_.openDoors;
        cheatShowSecret_ = demoSaved_.showSecret;
        cheatCompass_ = demoSaved_.compass;
        cheatDebug_ = demoSaved_.debug;
        cheatPhoto_ = demoSaved_.photo;
        cheatFreeze_ = demoSaved_.freeze;
        cheatXray_ = demoSaved_.xray;
        cheatHitbox_ = demoSaved_.hitbox;
        cheatTrail_ = demoSaved_.trail;
        cheatIds_ = demoSaved_.ids;
        cheatSounds_ = demoSaved_.sounds;
        cheatChunks_ = demoSaved_.chunks;
        inputPad_.RestoreCheatEntryLock(demoSaved_.cheatsLocked, demoSaved_.cheatsLockedForRun);
        interaction_.SetLives(demoSaved_.lives);
        activeCheats_ = demoSaved_.activeCheats;
        galaxyGamer_ = demoSaved_.galaxyGamer;
        sound_.SetReplayMuted(false);
        sound_.SetEnabled(saveData_.GetSoundEnabled());
        sound_.SetEffectsVolume(static_cast<float>(saveData_.GetEffectsVolume()) / 100.0f);
        ApplyMusicVolume();
        controlsHelp_ = {};
        cheatMenuShown_ = false;
        // Still demoActive_: SetPhase() leaves the resume file alone.
        SetPhase(GalaxyEggbert::Def::GamePhase::Init, /*bypassFade=*/true);
        demoActive_ = false;
        demo_ = {};
        ResetMissionInput();
        // The title scene at once, so no frame shows the plain menu over the demo's world.
        if (titleSceneRequested_) LoadTitleScene();
        // DemoPlayStop() -> Init; the press that stopped it does not reach the menu.
        menuInputLatched_ = true;
        demoLastLive_ = ReadDemoLiveInput(false);
        demoIdleTimer_.Reset();
        if (demoCollectionPlayback_)
        {
            demoCollectionPlayback_ = false;
            demoSlotsShown_ = true;
            demoSlotsMode_ = 3;
            demoSlotsMessage_.clear();
            focusNavigator_.Reset();
            std::cout << "GalaxyEggbertCNA: recording stopped, back to My recordings." << std::endl;
        }
        else std::cout << "GalaxyEggbertCNA: demo stopped, back to the menu." << std::endl;
    }

    bool GalaxyEggbertGame::UpdateDemoMenu()
    {
        using GalaxyEggbert::Def::GamePhase;
        if (SuppressLiveInput() || !demoRecordPath_.empty() || phase_ != GamePhase::Init)
        {
            demoIdleTimer_.Reset();
            menuInputLatched_ = false;
            return false;
        }
        const auto live = ReadDemoLiveInput(true);
        const bool activity = GalaxyEggbert::Game::DemoActivity(demoLastLive_, live);
        demoLastLive_ = live;
        if (menuInputLatched_)
        {
            if (GalaxyEggbert::Game::DemoInputHeld(live) || !live.keyEvents.empty())
            {
                tickInput_.keys.clear();
                tickInput_.buttons = 0;
                tickInput_.touches.clear();
            }
            else
            {
                menuInputLatched_ = false;
            }
        }
        // Only the settled main menu of the active window counts: not Play, Pause, Setup, the
        // editor, the help or a menu transition.
        const bool eligible = fadeOutPhase_ == GamePhase::None && getIsActiveProperty() && !controlsHelp_.shown &&
                              !menuInputLatched_ && !demoSlotsShown_ && !goodbyeShown_ && !NetScreenShown();
        if (!demoIdleTimer_.Update(demoClockSeconds_, eligible, activity)) return false;
        std::cout << "GalaxyEggbertCNA: " << GalaxyEggbert::Game::kDemoIdleSeconds
                  << " s without input in the main menu: the demos play." << std::endl;
        demoNumber_ = 0; // DemoStep(): an automatic demo starts with the first
        if (StartNextDemo()) return true;
        std::cout << "GalaxyEggbertCNA: no demo can be played." << std::endl;
        return false;
    }

    void GalaxyEggbertGame::BeginDemoRecording()
    {
        demoRecordArmed_ = false;
        demoRecordLog_ = {};
        demoRecordLog_.start = resumeLog_.start; // StartResumeLog() just took it
        demoRecording_ = true;
        demoEndRequested_ = false;
        std::cout << "GalaxyEggbertCNA: recording demo " << demoRecordPath_ << " from mission "
                  << demoRecordLog_.start.mission
                  << (demoRecordByPlayer_ ? " (leaving the mission or Play ends it)." : " (Escape ends it).")
                  << std::endl;
    }

    bool GalaxyEggbertGame::FinishDemoRecording()
    {
        if (!demoRecording_) return false;
        demoRecording_ = false;
        using namespace GalaxyEggbert::Game;
        DemoRecording demo;
        demo.title = demoRecordTitle_;
        demo.gameVersion = GalaxyEggbert::VersionString();
        const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm utc{};
#if defined(_WIN32)
        gmtime_s(&utc, &now);
#else
        gmtime_r(&now, &utc);
#endif
        char date[32]{};
        std::strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%SZ", &utc);
        demo.recordedAt = date;
        if (demoRecordByPlayer_)
        {
            demo.worldBytes = std::move(demoRecordWorldBytes_);
            if (!demo.worldBytes) return false;
        }
        demo.log = std::move(demoRecordLog_);
        demo.endSignature = HashDemoSignature(ResumeSignature());
        const std::string text = demo.Serialize();
        DemoRecording check;
        const auto status = ParseDemo(text, kDemoSimulationRevision, check);
        if (status != DemoLoadStatus::Ok)
        {
            std::cout << "GalaxyEggbertCNA: demo " << demoRecordPath_ << " not written: " << DemoLoadStatusText(status)
                      << "." << std::endl;
            if (!demoRecordByPlayer_) autoplayExitCode_ = 1;
            return false;
        }
        if (demoRecordByPlayer_)
        {
            // Kept for the save panel (EndPlayerRecording()), written there under its name.
            std::cout << "GalaxyEggbertCNA: recording ended (mission " << demo.log.start.mission << ", "
                      << demo.log.TickCount() << " ticks)." << std::endl;
            pendingRecording_ = std::move(demo);
            return true;
        }
        const std::string temporary = demoRecordPath_ + ".tmp";
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        out << text;
        out.close();
        std::error_code error;
        if (out) std::filesystem::rename(temporary, demoRecordPath_, error);
        if (!out || error)
        {
            std::filesystem::remove(temporary, error);
            std::cout << "GalaxyEggbertCNA: could not write demo " << demoRecordPath_ << "." << std::endl;
            autoplayExitCode_ = 1;
            return false;
        }
        std::cout << "GalaxyEggbertCNA: demo recorded: " << demoRecordPath_ << " (mission " << demo.log.start.mission
                  << ", " << demo.log.TickCount() << " ticks in " << demo.log.runs.size() << " runs)." << std::endl;
        return true;
    }

    // ---- The player's own demo recordings (BD-0116) ----

    std::vector<GalaxyEggbert::Game::InputPad::DemoSlotInfo> GalaxyEggbertGame::DemoSlotInfos() const
    {
        using namespace GalaxyEggbert::Game;
        std::vector<InputPad::DemoSlotInfo> slots(static_cast<std::size_t>(kDemoSlotCount));
        for (int slot = 1; slot <= kDemoSlotCount; ++slot)
        {
            auto& info = slots[static_cast<std::size_t>(slot - 1)];
            const auto own = UserDemoPath(slot);
            std::error_code error;
            info.recorded = std::filesystem::exists(own, error);
            DemoRecording demo;
            const auto text = ReadWorldBytes(info.recorded ? own : DemoPath(slot));
            const bool ok = text && ParseDemo(*text, kDemoSimulationRevision, demo) == DemoLoadStatus::Ok;
            // A recording named by the player shows its name (2026-11-10).
            info.line1 = info.recorded ? (ok && !IsAutomaticDemoTitle(demo.title) ? demo.title : std::string(Tr("Recorded")))
                                       : std::string(ok ? Tr("Shipped") : Tr("Empty"));
            if (ok)
            {
                const int mission = demo.log.start.mission;
                const bool galaxy = demo.log.start.worldPath.find("/galaxy/world") != std::string::npos;
                info.line2 = TrFormat(galaxy ? "Galaxy {0}" : "Mission {0}", {std::to_string(mission)});
            }
            else if (info.recorded)
            {
                info.line2 = Tr("(unplayable)");
            }
        }
        return slots;
    }

    void GalaxyEggbertGame::ShowDemoNotice(std::string text)
    {
        std::cout << "GalaxyEggbertCNA: " << text << std::endl;
        demoNotice_ = std::move(text);
        demoNoticeSeconds_ = 4.0f;
    }

    void GalaxyEggbertGame::UpdateDemoSlotsScreen(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                                  int viewportW, int viewportH)
    {
        if (demoNaming_)
        {
            // The name typed on the real keyboard, then the keyboard on the screen.
            for (const auto& [typed, shift] : browserTypedKeys_)
                if (demoNaming_ && GalaxyEggbert::Game::ScreenKeyboard::TypeKey(typed, shift, demoNameDraft_,
                                                                             GalaxyEggbert::Game::kDemoNameMax) ==
                                       GalaxyEggbert::Game::ScreenKeyboard::Result::Done)
                    FinishDemoNaming(true);
            browserTypedKeys_.clear();
            if (!demoNaming_) return;
            const auto keys = inputPad_.UpdateDemoSlots(mouse, viewportW, viewportH, demoSlotInfos_, -1, false,
                                                        &demoKeyboard_);
            if (keys.keyboardKey >= 0)
            {
                const auto typed = demoKeyboard_.Press(keys.keyboardKey, demoNameDraft_, GalaxyEggbert::Game::kDemoNameMax);
                if (typed == GalaxyEggbert::Game::ScreenKeyboard::Result::Done) FinishDemoNaming(true);
                else if (typed == GalaxyEggbert::Game::ScreenKeyboard::Result::Cancel) FinishDemoNaming(false);
            }
            else if (keys.outsideClick)
            {
                FinishDemoNaming(true);
            }
            return;
        }
        browserTypedKeys_.clear();
        if (demoSlotsMode_ == 3 && phase_ == GalaxyEggbert::Def::GamePhase::Pause)
        {
            // Pause keeps its import picker: starting a collection replay here would replace
            // the still-live paused mission. My recordings is entered from the main menu.
            using namespace GalaxyEggbert::Game;
            const int first = demoImportPage_ * InputPad::kDemoListRows;
            const int count = std::clamp(static_cast<int>(demoImports_.size()) - first, 0, InputPad::kDemoListRows);
            const auto picked = inputPad_.UpdateDemoSlots(mouse, viewportW, viewportH, demoSlotInfos_, count,
                                                          DemoImportPages() > 1);
            if (GalaxyEggbert::Editor::ConsumeUploadDone()) ListDemoImports();
            if (picked.cancelPressed) CancelDemoSlotsStep();
            else if (picked.importRow > 0)
            {
                demoImportIndex_ = static_cast<std::size_t>(first + picked.importRow - 1);
                demoSlotsMode_ = 2;
                demoSlotsMessage_.clear();
                demoDeleteArmed_ = -1;
            }
            else if (picked.deleteRow > 0)
            {
                const int index = first + picked.deleteRow - 1;
                const auto entry = demoCollection_[static_cast<std::size_t>(index)];
                if (demoDeleteArmed_ == index)
                {
                    const bool removed = DeleteDemoFile(entry.path);
                    ListDemoImports();
                    demoSlotsMessage_ = TrFormat(removed ? "{0} deleted." : "{0} could not be deleted.", {entry.name});
                }
                else
                {
                    demoDeleteArmed_ = index;
                    demoSlotsMessage_ = TrFormat("Delete {0}? Click its bin again.", {entry.name});
                }
            }
            else if (picked.prevPage || picked.nextPage)
            {
                demoDeleteArmed_ = -1;
                demoImportPage_ = std::clamp(demoImportPage_ + (picked.nextPage ? 1 : -1), 0, DemoImportPages() - 1);
            }
            else if (picked.exportPressed || picked.namePressed || picked.importPressed)
            {
                demoDeleteArmed_ = -1;
                demoSlotsMode_ = picked.exportPressed ? 1 : picked.namePressed ? 4 : 0;
                demoSlotsMessage_.clear();
            }
            return;
        }
        if (demoSlotsMode_ == 3)
        {
            UpdateRecordings(mouse, viewportW, viewportH);
            return;
        }
        const auto slots = inputPad_.UpdateDemoSlots(mouse, viewportW, viewportH, demoSlotInfos_);
        if (demoSlotsMode_ == 2)
        {
            if (slots.cancelPressed) CancelDemoSlotsStep();
            else if (slots.recordSlot > 0) HandleDemoSlotClick(slots.recordSlot);
            return;
        }
        if (slots.recordSlot > 0)
        {
            HandleDemoSlotClick(slots.recordSlot);
        }
        else if (slots.exportPressed)
        {
            demoSlotsMode_ = demoSlotsMode_ == 1 ? 0 : 1;
            demoSlotsMessage_ = demoSlotsMode_ == 1 ? Tr("Click a position to export its demo.") : "";
        }
        else if (slots.importPressed)
        {
            ToggleDemoImport();
        }
        else if (slots.namePressed)
        {
            demoSlotsMode_ = demoSlotsMode_ == 4 ? 0 : 4;
            demoSlotsMessage_ = demoSlotsMode_ == 4 ? Tr("Click a recorded position to name its demo.") : "";
        }
        else if (slots.restoreSlot > 0)
        {
            std::error_code error;
            std::filesystem::remove(GalaxyEggbert::Game::UserDemoPath(slots.restoreSlot), error);
            demoSlotInfos_ = DemoSlotInfos();
            demoSlotsMessage_ = TrFormat("Position {0} plays its shipped demo again.", {std::to_string(slots.restoreSlot)});
            std::cout << "GalaxyEggbertCNA: " << demoSlotsMessage_ << std::endl;
        }
        else if (slots.cancelPressed)
        {
            // Cancel leaves Export, Import or Name first, then the positions.
            CancelDemoSlotsStep();
        }
    }

    void GalaxyEggbertGame::StartGoodbye()
    {
        goodbyeShown_ = true;
        goodbyeSeconds_ = 0.0f;
        demoSlotsShown_ = false;
        goodbyeInputWas_ = ReadDemoLiveInput(true); // what closed the menu is not what closes the game
        std::cout << "GalaxyEggbertCNA: goodbye (a key, a click, a button or a touch closes the game)." << std::endl;
    }

    void GalaxyEggbertGame::CancelDemoSlotsStep()
    {
        demoCollectionStartDelay_ = 0;
        demoDeleteArmed_ = -1;
        if (demoNaming_)
        {
            FinishDemoNaming(false);
        }
        else if (demoSlotsMode_ == 2)
        {
            demoSlotsMode_ = 3;
            demoSlotsMessage_.clear();
        }
        else if (demoSlotsMode_ != 0)
        {
            demoSlotsMode_ = 0;
            demoSlotsMessage_.clear();
        }
        else
        {
            demoSlotsShown_ = false;
        }
    }

    void GalaxyEggbertGame::StartDemoNaming(int slot)
    {
        using namespace GalaxyEggbert::Game;
        const std::string title = slot == 0 && SelectedRecording() ? SelectedRecording()->name : DemoTitle(UserDemoPath(slot));
        demoNaming_ = true;
        demoNameSlot_ = slot;
        demoNameDraft_ = slot == 0 ? SanitizeDemoName(title) : IsAutomaticDemoTitle(title) ? std::string() : SanitizeDemoName(title);
        browserTypedKeys_.clear();
        typedShiftPending_ = false;
        demoKeyboard_.Layout(InputPad::DemoKeyboardRect(tickInput_.viewportWidth, tickInput_.viewportHeight));
        demoKeyboard_.Begin(demoNameDraft_);
        demoSlotsMessage_ = Tr("Type its name or use the keys: OK keeps it, Cancel leaves it.");
    }

    void GalaxyEggbertGame::FinishDemoNaming(bool keep)
    {
        using namespace GalaxyEggbert::Game;
        demoNaming_ = false;
        if (demoNameSlot_ == 0)
        {
            demoSlotsMode_ = 3;
            demoSlotsMessage_.clear();
            if (!keep || !SelectedRecording()) return;
            const auto path = RenameDemoFile(SelectedRecording()->path, demoNameDraft_);
            if (path.empty())
                demoSlotsMessage_ = Tr("The recording could not be renamed. Use a name with a letter or digit.");
            else
            {
                demoImports_[demoImportIndex_] = path;
                ListDemoImports();
            }
            return;
        }
        demoSlotsMode_ = 0;
        const std::string position = std::to_string(demoNameSlot_);
        const std::string name = SanitizeDemoName(demoNameDraft_);
        if (!keep)
        {
            demoSlotsMessage_.clear();
            return;
        }
        if (name.empty())
        {
            demoSlotsMessage_ = TrFormat("Position {0}: a name needs a letter or a digit; it was left as it was.", {position});
            return;
        }
        demoSlotsMessage_ = RenameDemo(UserDemoPath(demoNameSlot_), name)
                                ? TrFormat("Position {0} is named {1}.", {position, name})
                                : TrFormat("Position {0} could not be named.", {position});
        demoSlotInfos_ = DemoSlotInfos();
        std::cout << "GalaxyEggbertCNA: " << demoSlotsMessage_ << std::endl;
    }

    std::vector<GalaxyEggbert::Game::QuadBatch::Rect> GalaxyEggbertGame::DemoSlotsFocusTargets(int w, int h) const
    {
        using GalaxyEggbert::Game::InputPad;
        if (demoNaming_) return demoKeyboard_.Rects();
        std::vector<GalaxyEggbert::Game::QuadBatch::Rect> targets;
        if (demoSlotsMode_ == 3 && phase_ == GalaxyEggbert::Def::GamePhase::Pause)
        {
            const int first = demoImportPage_ * InputPad::kDemoListRows;
            for (int i = 0; i < InputPad::kDemoListRows && first + i < static_cast<int>(demoImports_.size()); ++i)
            {
                targets.push_back(InputPad::DemoListRowRect(i, w, h));
                targets.push_back(InputPad::DemoListDeleteRect(i, w, h));
            }
            if (DemoImportPages() > 1)
            {
                targets.push_back(InputPad::DemoListPrevRect(w, h));
                targets.push_back(InputPad::DemoListNextRect(w, h));
            }
            for (const auto r : {InputPad::DemoExportRect(w, h), InputPad::DemoImportRect(w, h),
                                 InputPad::DemoNameRect(w, h), InputPad::DemoCancelRect(w, h)}) targets.push_back(r);
            return targets;
        }
        if (demoSlotsMode_ == 3)
        {
            for (const auto& button : RecordingButtons(w, h))
                if (button.enabled) targets.push_back(button.rect);
            return targets;
        }
        else
        {
            for (int i = 0; i < static_cast<int>(demoSlotInfos_.size()) && i < 10; ++i)
            {
                targets.push_back(InputPad::DemoSlotRect(i, w, h));
                if (demoSlotsMode_ != 2 && demoSlotInfos_[static_cast<std::size_t>(i)].recorded)
                    targets.push_back(InputPad::DemoRestoreRect(i, w, h));
            }
        }
        if (demoSlotsMode_ == 2)
        {
            targets.push_back(InputPad::DemoCancelRect(w, h));
            return targets;
        }
        for (const auto& r : {InputPad::DemoExportRect(w, h), InputPad::DemoImportRect(w, h), InputPad::DemoNameRect(w, h),
                              InputPad::DemoCancelRect(w, h)})
            targets.push_back(r);
        return targets;
    }

    void GalaxyEggbertGame::DrawDemoSlotsScreen(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                                int viewportW, int viewportH)
    {
        using namespace GalaxyEggbert::Game;
        if (demoSlotsMode_ == 3 && !demoNaming_ && phase_ == GalaxyEggbert::Def::GamePhase::Init)
        {
            inputPad_.DrawRecordings(device, viewportW, viewportH, demoCollection_, demoImportPage_,
                                    SelectedRecording() ? static_cast<int>(demoImportIndex_) : -1,
                                    demoSlotsMessage_, GalaxyEggbert::Editor::FileUploadAvailable(), focusHighlight_);
            return;
        }
        DemoSlotsExtras extras;
        extras.focus = focusHighlight_;
        extras.collectionLink = phase_ == GalaxyEggbert::Def::GamePhase::Init;
        if (demoSlotsMode_ == 3 && !demoNaming_)
        {
            std::vector<std::string> rows;
            const int first = demoImportPage_ * InputPad::kDemoListRows;
            for (int i = first; i < static_cast<int>(demoCollection_.size()) && i < first + InputPad::kDemoListRows; ++i)
            {
                const auto& entry = demoCollection_[static_cast<std::size_t>(i)];
                rows.push_back(entry.name + " - " + entry.metadata + " " + InputPad::RecordingState(entry));
            }
            inputPad_.DrawDemoSlots(device, viewportW, viewportH, demoSlotInfos_, demoSlotsMessage_,
                                   std::string(Tr("Import a demo: choose the file")), 2, &rows,
                                   demoDeleteArmed_ - first,
                                   DemoImportPages() > 1 ? std::to_string(demoImportPage_ + 1) + " / " +
                                       std::to_string(DemoImportPages()) : "", extras);
            return;
        }
        extras.positionChoice = demoSlotsMode_ == 2;
        if (demoNaming_)
        {
            extras.keyboard = &demoKeyboard_;
            extras.draft = demoNameDraft_;
        }
        const std::string title = demoNaming_ ? (demoNameSlot_ == 0 ? std::string(Tr("Rename")) :
                                      TrFormat("Name the demo in position {0}", {std::to_string(demoNameSlot_)}))
                                : demoSlotsMode_ == 2 && SelectedRecording()
                                    ? TrFormat("Menu demo: choose the position for {0}", {SelectedRecording()->name})
                                : std::string(demoSlotsMode_ == 4 ? Tr("Name a demo: choose its position")
                                    : demoSlotsMode_ == 1 ? Tr("Export a demo: choose its position")
                                    : phase_ == GalaxyEggbert::Def::GamePhase::Init ? Tr("The demo positions")
                                    : Tr("Record a demo: choose its position"));
        inputPad_.DrawDemoSlots(device, viewportW, viewportH, demoSlotInfos_, demoSlotsMessage_, title,
                               demoSlotsMode_ == 4 ? 3 : demoSlotsMode_, nullptr, -1, {}, extras);
    }

    std::string GalaxyEggbertGame::DemoImportPrompt() const
    {
        if (demoImports_.empty())
            return GalaxyEggbert::Editor::FileUploadAvailable() ? std::string(Tr("Choose a .gdemo file: it is listed here."))
                : TrFormat("Put .gdemo files in {0}/ to import them.", {GalaxyEggbert::Game::SharedDemoDir()});
        return std::string(Tr("Choose a recording first."));
    }

    int GalaxyEggbertGame::DemoImportPages() const noexcept
    {
        constexpr int kRows = GalaxyEggbert::Game::InputPad::kDemoListRows;
        return std::max(1, (static_cast<int>(demoImports_.size()) + kRows - 1) / kRows);
    }

    void GalaxyEggbertGame::ListDemoImports()
    {
        using namespace GalaxyEggbert::Game;
        const std::string selected = demoImportIndex_ < demoImports_.size() ? demoImports_[demoImportIndex_] : "";
        demoCollection_ = ListDemoCollection([this](DemoRecording& demo) {
            int galaxy = -1;
            return ResolveDemoWorld(demo, galaxy);
        });
        demoImports_.clear();
        for (const auto& entry : demoCollection_) demoImports_.push_back(entry.path);
        const auto found = std::find(demoImports_.begin(), demoImports_.end(), selected);
        demoImportIndex_ = static_cast<std::size_t>(found - demoImports_.begin());
        demoDeleteArmed_ = -1;
        demoImportPage_ = std::min(demoImportPage_, DemoImportPages() - 1);
        if (SelectedRecording()) demoImportPage_ = static_cast<int>(demoImportIndex_) / InputPad::kDemoListRows;
    }

    const GalaxyEggbert::Game::DemoCollectionEntry* GalaxyEggbertGame::SelectedRecording() const
    {
        return demoImportIndex_ < demoCollection_.size() ? &demoCollection_[demoImportIndex_] : nullptr;
    }

    std::vector<GalaxyEggbert::Game::PanelButton> GalaxyEggbertGame::RecordingButtons(int w, int h) const
    {
        using GalaxyEggbert::Game::InputPad;
        const int first = demoImportPage_ * InputPad::kDemoListRows;
        const int count = std::clamp(static_cast<int>(demoCollection_.size()) - first, 0, InputPad::kDemoListRows);
        return InputPad::RecordingButtons(w, h, count, SelectedRecording() ? static_cast<int>(demoImportIndex_) - first : -1,
                                         DemoImportPages() > 1, GalaxyEggbert::Editor::FileUploadAvailable());
    }

    void GalaxyEggbertGame::ToggleDemoImport()
    {
        demoSlotsMode_ = 3;
        demoImportPage_ = 0;
        demoImportIndex_ = demoImports_.size(); // opening the collection selects nothing
        ListDemoImports();
        demoSlotsMessage_ = DemoImportPrompt();
        if (phase_ == GalaxyEggbert::Def::GamePhase::Pause &&
            GalaxyEggbert::Editor::RequestUpload(GalaxyEggbert::Game::SharedDemoDir(), ".gdemo"))
            demoSlotsMessage_ = Tr("Choose a .gdemo file: it is listed here.");
        focusNavigator_.Reset();
    }

    void GalaxyEggbertGame::PlaySelectedRecording()
    {
        if (!SelectedRecording()) return;
        const bool older = SelectedRecording()->olderRevision;
        const auto path = SelectedRecording()->path;
        if (StartDemoFile(path, 0, true) && older)
            ShowDemoNotice(std::string(Tr("Recorded with an older version: it may play differently.")));
    }

    void GalaxyEggbertGame::UpdateRecordings(const Microsoft::Xna::Framework::Input::MouseState& mouse, int w, int h)
    {
        using namespace GalaxyEggbert::Game;
        if (GalaxyEggbert::Editor::ConsumeUploadDone())
        {
            ListDemoImports();
            demoImportPage_ = 0;
            demoSlotsMessage_ = DemoImportPrompt();
        }
        const int action = inputPad_.UpdatePanel(mouse, RecordingButtons(w, h)).button;
        if (action < 0)
        {
            if (demoCollectionStartDelay_ > 0 && --demoCollectionStartDelay_ == 0) PlaySelectedRecording();
            return;
        }
        demoCollectionStartDelay_ = 0;
        const int armed = demoDeleteArmed_;
        demoDeleteArmed_ = -1; // every other action disarms deletion
        if (action < InputPad::kDemoListRows)
        {
            demoImportIndex_ = static_cast<std::size_t>(demoImportPage_ * InputPad::kDemoListRows + action);
            demoSlotsMessage_ = InputPad::RecordingState(*SelectedRecording());
        }
        else if (action == InputPad::RecordingCancel) CancelDemoSlotsStep();
        else if (action == InputPad::RecordingPrev || action == InputPad::RecordingNext)
            demoImportPage_ = std::clamp(demoImportPage_ + (action == InputPad::RecordingNext ? 1 : -1), 0, DemoImportPages() - 1);
        else if (action == InputPad::RecordingAdd)
        {
            if (GalaxyEggbert::Editor::RequestUpload(SharedDemoDir(), ".gdemo"))
                demoSlotsMessage_ = Tr("Choose a .gdemo file: it is listed here.");
        }
        else if (!SelectedRecording()) demoSlotsMessage_ = Tr("Choose a recording first.");
        else
        {
            const auto entry = *SelectedRecording();
            if (action == InputPad::RecordingRename) StartDemoNaming(0);
            else if (action == InputPad::RecordingDelete)
            {
                if (armed != static_cast<int>(demoImportIndex_))
                {
                    demoDeleteArmed_ = static_cast<int>(demoImportIndex_);
                    demoSlotsMessage_ = TrFormat("Delete {0}? Press Delete again.", {entry.name});
                }
                else
                {
                    const bool removed = DeleteDemoFile(entry.path);
                    ListDemoImports();
                    demoSlotsMessage_ = TrFormat(removed ? "{0} deleted." : "{0} could not be deleted.", {entry.name});
                }
            }
            else if (action == InputPad::RecordingExport)
            {
                const auto name = std::filesystem::path(entry.path).filename().string();
                if (GalaxyEggbert::Editor::OfferDownload(entry.path, name)) demoSlotsMessage_ = name;
                else demoSlotsMessage_ = TrFormat("It is the file {0}.", {std::filesystem::absolute(entry.path).string()});
            }
            else if (action == InputPad::RecordingMenu && entry.olderRevision)
                demoSlotsMessage_ = Tr("A recording of an older version cannot be a menu demo.");
            else if (!entry.Playable()) demoSlotsMessage_ = InputPad::RecordingState(entry);
            else if (action == InputPad::RecordingMenu)
            {
                demoSlotsMode_ = 2;
                demoSlotsMessage_.clear();
                focusNavigator_.Reset();
            }
            else if (action == InputPad::RecordingPlay)
            {
                if (entry.olderRevision)
                {
                    demoSlotsMessage_ = Tr("Recorded with an older version: it may play differently.");
                    demoCollectionStartDelay_ = 60; // show the warning before starting, with Cancel still available
                }
                else PlaySelectedRecording();
            }
        }
        if (!demoSlotsMessage_.empty()) std::cout << "GalaxyEggbertCNA: " << demoSlotsMessage_ << std::endl;
    }

    void GalaxyEggbertGame::HandleDemoSlotClick(int slot)
    {
        using namespace GalaxyEggbert::Game;
        const std::string position = std::to_string(slot);
        if (demoSlotsMode_ == 4)
        {
            std::error_code error;
            if (!std::filesystem::exists(UserDemoPath(slot), error))
                demoSlotsMessage_ = TrFormat("Position {0} has no recording of yours to name.", {position});
            else if (DemoTitle(UserDemoPath(slot)).empty())
                demoSlotsMessage_ = TrFormat("The recording in position {0} cannot be read.", {position});
            else
                StartDemoNaming(slot);
            return;
        }
        if (demoSlotsMode_ == 0)
        {
            // Replays 3/3: a recording goes into My recordings, a position gets it from there.
            demoSlotsMessage_ = Tr("Recordings are made in a mission with Pause > Record and kept in My recordings.");
            return;
        }
        if (demoSlotsMode_ == 1)
        {
            demoSlotsMode_ = 0;
            const auto exported = ExportDemo(DemoSlotPath(slot), saveData_.GetSelectedGamer(), slot);
            if (exported.empty())
            {
                demoSlotsMessage_ = TrFormat("Position {0} has no demo to export.", {position});
                return;
            }
            const std::string name = std::filesystem::path(exported).filename().string();
            demoSlotsMessage_ = GalaxyEggbert::Editor::OfferDownload(exported, name)
                                    ? TrFormat("Demo {0} exported: downloaded as {1}.", {std::to_string(slot), name})
                                    : TrFormat("Demo {0} exported to {1}.", {std::to_string(slot), exported});
            std::cout << "GalaxyEggbertCNA: " << demoSlotsMessage_ << std::endl;
            return;
        }
        if (!SelectedRecording()) return;
        const auto entry = *SelectedRecording();
        // Re-read at action time: a file may have been replaced while the picker was open.
        ListDemoImports();
        demoSlotsMode_ = 3;
        if (!SelectedRecording()) { demoSlotsMessage_ = Tr(DemoLoadStatusText(DemoLoadStatus::Missing)); return; }
        if (SelectedRecording()->olderRevision)
        { demoSlotsMessage_ = Tr("A recording of an older version cannot be a menu demo."); return; }
        if (!SelectedRecording()->Playable()) { demoSlotsMessage_ = InputPad::RecordingState(*SelectedRecording()); return; }
        const auto status = ImportDemo(entry.path, slot);
        demoSlotsMessage_ = status == DemoLoadStatus::Ok
            ? TrFormat("Position {0} plays {1} now.", {position, entry.name})
            : TrFormat("{0} cannot be imported: {1}.", {entry.name, Tr(DemoLoadStatusText(status))});
        demoSlotInfos_ = DemoSlotInfos();
        std::cout << "GalaxyEggbertCNA: " << demoSlotsMessage_ << std::endl;
    }

    void GalaxyEggbertGame::StartPlayerRecording()
    {
        using namespace GalaxyEggbert::Game;
        demoSlotsShown_ = false;
        const int mission = worldRuntime_.GetMissionNumber();
        // Speedy Blupi's DemoRecStart(): the mission from its start, at the normal speed.
        demoRecordByPlayer_ = true;
        demoRecordWorldBytes_.reset();
        demoRecordPath_.clear(); // kept in memory at its end, then named (EndPlayerRecording())
        demoRecordMaxTicks_ = 0; // kDemoMaxTicks, ten minutes
        demoRecordTitle_ = std::string("Recorded by player ") + static_cast<char>('A' + saveData_.GetSelectedGamer()) +
                           (galaxyGamer_ >= 0 ? ": " + GalaxyEggbert::Editor::GalaxyMissionName(mission) + " of a galaxy"
                                              : ": mission " + std::to_string(mission));
        // The name offered when it is saved (letters, digits, spaces: SanitizeDemoName()).
        recordingSaveDefault_ = galaxyGamer_ >= 0 ? SanitizeDemoName(GalaxyEggbert::Editor::GalaxyMissionName(mission))
                                                  : "Mission " + std::to_string(mission);
        if (recordingSaveDefault_.empty()) recordingSaveDefault_ = "Mission " + std::to_string(mission);
        demoRecordArmed_ = true;
        deferredMission_ = 0;
        gameSpeed_ = GalaxyEggbert::Def::GameSpeed::Normal;
        LoadMission(mission);
        if (!missionStartPending_)
        {
            // Its world could not be loaded again: nothing is armed, the game goes on as it was.
            demoRecordByPlayer_ = false;
            demoRecordTitle_.clear();
            demoRecordArmed_ = false;
            ShowDemoNotice(std::string(Tr("The demo could not be recorded.")));
            return;
        }
        SetPhase(GalaxyEggbert::Def::GamePhase::Play, /*bypassFade=*/true);
        std::cout << "GalaxyEggbertCNA: recording mission " << mission << " again from its start." << std::endl;
    }

    void GalaxyEggbertGame::EndPlayerRecording(bool written)
    {
        demoRecordWorldBytes_.reset();
        demoRecordByPlayer_ = false;
        demoRecordPath_.clear();
        demoRecordTitle_.clear();
        demoRecordArmed_ = false;
        demoEndRequested_ = false;
        if (written && pendingRecording_)
        {
            OpenRecordingSave(); // the held-back mission once it is saved or discarded
            return;
        }
        pendingRecording_.reset();
        ShowDemoNotice(std::string(Tr("The demo could not be recorded.")));
        LoadDeferredMission();
    }

    void GalaxyEggbertGame::LoadDeferredMission()
    {
        if (const int mission = std::exchange(deferredMission_, 0); mission != 0)
        {
            LoadMission(mission); // where the recorded mission was going
        }
    }

    // ---- The "Save the recording" panel (replays 3/3) ----

    void GalaxyEggbertGame::OpenRecordingSave()
    {
        using namespace GalaxyEggbert::Game;
        recordingSaveOpen_ = true;
        recordingDiscardArmed_ = false;
        recordingSaveKeysWas_ = true;
        recordingSavePointerHeld_ = true;
        recordingAutosavedPath_.clear();
        demoNameDraft_ = recordingSaveDefault_;
        browserTypedKeys_.clear();
        typedShiftPending_ = false;
        demoKeyboard_.Layout(InputPad::DemoKeyboardRect(tickInput_.viewportWidth, tickInput_.viewportHeight));
        demoKeyboard_.Begin(demoNameDraft_);
        demoKeyboard_.SetActionLabels(std::string(Tr("Save")), std::string(Tr("Discard")));
        focusNavigator_.Reset();
        focusHighlight_.reset();
        recordingSaveMessage_ = Tr("Name it: Save keeps it in My recordings, Discard drops it.");
        mouseLookActive_ = false;
        inputPad_.ResetTouchState();
    }

    std::string GalaxyEggbertGame::WritePendingRecording()
    {
        if (!pendingRecording_) return {};
        return GalaxyEggbert::Game::SaveDemoToCollection(*pendingRecording_, demoNameDraft_, recordingSaveDefault_);
    }

    void GalaxyEggbertGame::FinishRecordingSave(bool keep)
    {
        using namespace GalaxyEggbert::Game;
        std::error_code error;
        if (keep)
        {
            // Saved already when the game was deactivated: that file gives way to the chosen name.
            if (!recordingAutosavedPath_.empty()) std::filesystem::remove(recordingAutosavedPath_, error);
            const auto path = WritePendingRecording();
            if (path.empty())
            {
                recordingSaveMessage_ = Tr("The recording could not be saved.");
                return; // the panel stays: Save again, or Discard
            }
            const std::string name = SanitizeDemoName(demoNameDraft_).empty() ? recordingSaveDefault_
                                                                             : SanitizeDemoName(demoNameDraft_);
            ShowDemoNotice(TrFormat("Saved in My recordings: {0}.", {name}));
            std::cout << "GalaxyEggbertCNA: recording saved: " << path << "." << std::endl;
        }
        else
        {
            if (!recordingAutosavedPath_.empty()) std::filesystem::remove(recordingAutosavedPath_, error);
            ShowDemoNotice(std::string(Tr("The recording was discarded.")));
            std::cout << "GalaxyEggbertCNA: recording discarded." << std::endl;
        }
        recordingSaveOpen_ = false;
        pendingRecording_.reset();
        recordingAutosavedPath_.clear();
        demoKeyboard_.SetActionLabels({}, {});
        focusNavigator_.Reset();
        focusHighlight_.reset();
        browserTypedKeys_.clear();
        // What ended it is still held (Enter, Escape, A, B, a touch): the screen under it waits for
        // its release (the menu's latch; Pause's own edges).
        menuInputLatched_ = true;
        menuConfirmWasDown_ = pauseKeyWasDown_ = focusPadBWas_ = true;
        LoadDeferredMission();
    }

    void GalaxyEggbertGame::UpdateRecordingSave()
    {
        using namespace GalaxyEggbert::Game;
        using Microsoft::Xna::Framework::Input::Keys;
        const int w = tickInput_.viewportWidth, h = tickInput_.viewportHeight;
        demoKeyboard_.Layout(InputPad::DemoKeyboardRect(w, h)); // the window may have changed
        const auto keys = CurrentKeyboardState();
        const GamepadFrame pad = ReadGamepadFrame();
        // Escape and the gamepad's B discard (twice); Enter (the typed keys) or A on a key press it.
        const bool escape = keys.IsKeyDown(Keys::Escape) || pad.b;
        const bool escapePressed = escape && !recordingSaveKeysWas_;
        const auto discard = [this]() {
            if (recordingDiscardArmed_)
            {
                FinishRecordingSave(false);
                return;
            }
            recordingDiscardArmed_ = true;
            recordingSaveMessage_ = Tr("Discard the recording? Press Discard again.");
        };
        // The name typed on the real keyboard (Enter saves).
        for (const auto& [typed, shift] : browserTypedKeys_)
        {
            if (!recordingSaveOpen_) break;
            if (recordingSaveKeysWas_ && typed == Keys::Enter) continue; // the Enter that was held
            const auto result = ScreenKeyboard::TypeKey(typed, shift, demoNameDraft_, kDemoNameMax);
            if (result == ScreenKeyboard::Result::Done) FinishRecordingSave(true);
            else if (result == ScreenKeyboard::Result::Edited) recordingDiscardArmed_ = false;
        }
        browserTypedKeys_.clear();
        if (!recordingSaveOpen_) return;
        if (escapePressed)
        {
            discard();
            recordingSaveKeysWas_ = escape || keys.IsKeyDown(Keys::Enter) || pad.a;
            return;
        }
        recordingSaveKeysWas_ = escape || keys.IsKeyDown(Keys::Enter) || pad.a;
        // The arrows (and the d-pad) choose a key, A presses it (FocusNavigator, as the naming panel).
        FocusNavigator::Input in;
        in.up = keys.IsKeyDown(Keys::Up) || pad.dpadUp;
        in.down = keys.IsKeyDown(Keys::Down) || pad.dpadDown;
        in.left = keys.IsKeyDown(Keys::Left) || pad.dpadLeft;
        in.right = keys.IsKeyDown(Keys::Right) || pad.dpadRight;
        in.confirm = pad.a;
        unsigned now = (in.up ? 1u : 0u) | (in.down ? 2u : 0u) | (in.left ? 4u : 0u) | (in.right ? 8u : 0u) |
                       (in.confirm ? 16u : 0u);
        const unsigned pressed = now & ~focusKeysWas_;
        focusKeysWas_ = now;
        in.up = (pressed & 1u) != 0;
        in.down = (pressed & 2u) != 0;
        in.left = (pressed & 4u) != 0;
        in.right = (pressed & 8u) != 0;
        in.confirm = (pressed & 16u) != 0;
        const bool pointerDown = (tickInput_.buttons & 1u) != 0 || !tickInput_.touches.empty();
        in.pointer = pointerDown;
        const auto targets = demoKeyboard_.Rects();
        const auto focus = focusNavigator_.Update(targets, in);
        focusHighlight_.reset();
        if (focus.active && focus.focused >= 0 && static_cast<std::size_t>(focus.focused) < targets.size())
            focusHighlight_ = targets[static_cast<std::size_t>(focus.focused)];
        // The mouse (or a finger) on the keys; a press already down when the panel opened is not one.
        auto mouse = CurrentMouseState();
        for (const auto& touch : tickInput_.touches)
        {
            using Microsoft::Xna::Framework::Input::Touch::TouchLocationState;
            using Microsoft::Xna::Framework::Input::ButtonState;
            if (touch.state == int(TouchLocationState::Pressed) || touch.state == int(TouchLocationState::Moved))
            {
                mouse = Microsoft::Xna::Framework::Input::MouseState(int(touch.x), int(touch.y), 0, ButtonState::Pressed,
                                                                     ButtonState::Released, ButtonState::Released,
                                                                     ButtonState::Released, ButtonState::Released);
                break;
            }
        }
        if (recordingSavePointerHeld_)
        {
            if (pointerDown && !focus.active) return;
            recordingSavePointerHeld_ = false;
            // Its press tracking from the screen before (a stale press) is let go here, unused.
            (void)inputPad_.UpdateDemoSlots(CurrentMouseState(), w, h, {}, -1, false, &demoKeyboard_);
            return;
        }
        if (focus.active)
        {
            using Microsoft::Xna::Framework::Input::ButtonState;
            mouse = Microsoft::Xna::Framework::Input::MouseState(
                static_cast<int>(focus.x), static_cast<int>(focus.y), 0,
                focus.pressed ? ButtonState::Pressed : ButtonState::Released, ButtonState::Released,
                ButtonState::Released, ButtonState::Released, ButtonState::Released);
        }
        const auto input = inputPad_.UpdateDemoSlots(mouse, w, h, {}, -1, false, &demoKeyboard_);
        if (input.keyboardKey < 0) return;
        const auto result = demoKeyboard_.Press(input.keyboardKey, demoNameDraft_, kDemoNameMax);
        if (result == ScreenKeyboard::Result::Done) FinishRecordingSave(true);
        else if (result == ScreenKeyboard::Result::Cancel) discard();
        else if (result == ScreenKeyboard::Result::Edited) recordingDiscardArmed_ = false;
    }

    void GalaxyEggbertGame::DrawRecordingSave(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        GalaxyEggbert::Game::DemoSlotsExtras extras;
        extras.keyboard = &demoKeyboard_;
        extras.draft = demoNameDraft_;
        extras.focus = focusHighlight_;
        const auto& viewport = device.getViewportProperty();
        inputPad_.DrawDemoSlots(device, viewport.getWidthProperty(), viewport.getHeightProperty(), {},
                                recordingSaveMessage_, std::string(Tr("Save the recording")), 0, nullptr, -1, {},
                                extras);
    }

    Microsoft::Xna::Framework::Input::KeyboardState GalaxyEggbertGame::CurrentKeyboardState() const
    {
        using namespace Microsoft::Xna::Framework::Input;
        std::unordered_set<Keys> keys;
        for (int key : tickInput_.keys) keys.insert(static_cast<Keys>(key));
        return KeyboardState(keys);
    }

}
