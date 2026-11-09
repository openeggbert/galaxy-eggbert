// GalaxyEggbertGame: the resume log, the main-menu demos and the player's recordings. Split out of GalaxyEggbertGame.cpp.
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

    void GalaxyEggbertGame::ReadResumeLog()
    {
        std::error_code error;
        const auto size = std::filesystem::file_size(ResumeLog::kPath, error);
        if (error) return;
        pendingResume_.reset();
        if (size <= 128 * 1024 * 1024)
        {
            std::ifstream in(ResumeLog::kPath, std::ios::binary);
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
        pendingCheatLetters_.clear();
        debugVelocity_ = {};
        inputPad_.ResetMissionInput();
        inputPad_.SetJumpRight(saveData_.GetJumpRight());
        controlsHelp_ = {};
        menuConfirmWasDown_ = false;
        jumpKeyWasDown_ = actionKeyWasDown_ = pauseKeyWasDown_ = phaseReturnKeyWasDown_ = false;
        f5KeyWasDown_ = f6KeyWasDown_ = f7KeyWasDown_ = f8KeyWasDown_ = f12KeyWasDown_ = tabKeyWasDown_ = false;
        cameraModeKeyWasDown_ = fullscreenKeyWasDown_ = false;
        turnAroundDownWasHeld_ = false;
        turnAroundRemaining_ = turnAroundStartYaw_ = 0.0f;
        turnAroundDuration_ = 0.3f;
        touchQuietFrames_ = 0;
        mouseLookActive_ = false;
        lastMouseX_ = lastMouseY_ = 0;
        lookYawOffset_ = lookPitchOffset_ = 0.0f;
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
        start.cheatsLocked = inputPad_.IsCheatEntryLocked();
        start.cameraMode = static_cast<int>(cameraMode_);
        std::ostringstream settings;
        saveData_.Write(settings);
        start.saveData = settings.str();
        resumeTicks_ = 0;
        // A galaxy's mission too (BD-0119): its log names the galaxy's file, BeginResume() goes back
        // into the galaxy.
        recordingResume_ = !editorPlayTestActive_ && !loadedMissionPath_.empty();
        missionStartPending_ = false;
    }

    bool GalaxyEggbertGame::BeginResume(ResumeLog log)
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
        resumeLog_ = std::move(log);
        const auto& start = resumeLog_.start;
        replayingResume_ = true;
        recordingResume_ = true;
        sound_.SetReplayMuted(true);
        UpdateMusic();
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
        inputPad_.SetCheatEntryLocked(start.cheatsLocked && GalaxyEggbert::Game::kCheatsEnabled);
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
        if (inputPad_.IsCheatEntryLocked()) activeCheats_.push_back("fairplay");
        replayRun_ = 0;
        replayRepeat_ = 0;
        resumeTicks_ = 0;
        return true;
    }

    void GalaxyEggbertGame::ReplayBatch()
    {
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
        }
        if (replayRun_ != resumeLog_.runs.size()) return;
        replayingResume_ = false;
        sound_.SetReplayMuted(false);
        resumeAccelWarmStart_ = true;
        activeMotorLoop_ = GalaxyEggbert::Def::SoundChannel::SoundChannel0;
        wasPushingCrate_ = false;
        RefreshTerrainMeshIfChanged();
        if (!resumeSelftestSignature_.empty())
        {
            const auto actual = ResumeSignature();
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
        result["cheat input"] = inputPad_.IsCheatEntryLocked() ? "locked" : "allowed";
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
        resumeSelftestSignature_ = ResumeSignature();
        ResumeLog parsed;
        if (!ResumeLog::Parse(resumeLog_.Serialize(), ResumeBuild(), parsed) || !BeginResume(std::move(parsed)))
        {
            const char* line = "RESUME SELFTEST different: log could not be restored\n";
            std::cout << line << std::flush;
            if (autoplayTrace_) { std::fputs(line, autoplayTrace_); std::fflush(autoplayTrace_); }
            autoplayExitCode_ = 1;
            Exit();
        }
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
        const auto named = GalaxyEggbert::Editor::ParseGalaxyWorldPath(start.worldPath);
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

    bool GalaxyEggbertGame::StartDemoFile(const std::string& path, int number)
    {
        using namespace GalaxyEggbert::Game;
        // Everything is checked before anything of the player's is touched.
        DemoRecording demo;
        const auto text = ReadWorldBytes(path);
        auto status = text ? ParseDemo(*text, kDemoSimulationRevision, demo) : DemoLoadStatus::Missing;
        std::optional<std::string> world;
        int demoGalaxy = -1; // a recording made in a player's galaxy (BD-0119) plays from its file
        if (status == DemoLoadStatus::Ok)
        {
            const auto worldPath = ResolveDemoWorld(demo, demoGalaxy);
            world = ReadWorldBytes(worldPath);
            status = DemoWorldStatus(demo, worldPath, world);
            if (status == DemoLoadStatus::Ok)
            {
                WorldRuntime probe;
                std::istringstream stream(*world, std::ios::in | std::ios::binary);
                if (!probe.LoadFromVwrStream(stream)) status = DemoLoadStatus::WorldMissing;
            }
        }
        if (status != DemoLoadStatus::Ok)
        {
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
            demoSaved_.cheatsLocked = inputPad_.IsCheatEntryLocked();
            demoSaved_.lives = interaction_.Lives();
            demoSaved_.activeCheats = activeCheats_;
            demoSaved_.galaxyGamer = galaxyGamer_;
        }
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
        inputPad_.SetCheatEntryLocked(start.cheatsLocked && GalaxyEggbert::Game::kCheatsEnabled);
        cameraMode_ = static_cast<CameraMode>(start.cameraMode);
        gameSpeed_ = GalaxyEggbert::Def::GameSpeed::Normal;
        editorPlayTestActive_ = false;
        controlsHelp_ = {};
        cheatMenuShown_ = false;
        LoadMission(start.mission, &*world); // the demo's seed, nothing saved, no resume log
        interaction_.SetLives(start.lives);
        PlaceBlupi(start.x, start.y, start.z);
        blupi_.SetYaw(start.yaw);
        SetPhase(GalaxyEggbert::Def::GamePhase::Play, /*bypassFade=*/true);
        ResetMissionInput();
        activeCheats_.clear();
        if (cheatCompass_) activeCheats_.push_back("compass");
        if (cheatDebug_) activeCheats_.push_back("debug");
        if (inputPad_.IsCheatEntryLocked()) activeCheats_.push_back("fairplay");
        // What is held as it starts (the click on Demo, say) cannot stop it.
        if (!demoVerifying_) demoLastLive_ = ReadDemoLiveInput(false);
        demoSessionLive_ = true;
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
        inputPad_.SetCheatEntryLocked(demoSaved_.cheatsLocked);
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
        std::cout << "GalaxyEggbertCNA: demo stopped, back to the menu." << std::endl;
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
                              !menuInputLatched_ && !demoSlotsShown_;
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
                  << (demoRecordSlot_ > 0 ? " (leaving the mission or Play ends it)." : " (Escape ends it).")
                  << std::endl;
    }

    bool GalaxyEggbertGame::FinishDemoRecording()
    {
        if (!demoRecording_) return false;
        demoRecording_ = false;
        using namespace GalaxyEggbert::Game;
        DemoRecording demo;
        demo.title = demoRecordTitle_;
        demo.log = std::move(demoRecordLog_);
        demo.endSignature = HashDemoSignature(ResumeSignature());
        const std::string text = demo.Serialize();
        DemoRecording check;
        const auto status = ParseDemo(text, kDemoSimulationRevision, check);
        if (status != DemoLoadStatus::Ok)
        {
            std::cout << "GalaxyEggbertCNA: demo " << demoRecordPath_ << " not written: " << DemoLoadStatusText(status)
                      << "." << std::endl;
            if (demoRecordSlot_ == 0) autoplayExitCode_ = 1;
            return false;
        }
        if (demoRecordSlot_ > 0)
        {
            std::error_code made;
            std::filesystem::create_directories(std::filesystem::path(demoRecordPath_).parent_path(), made);
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
            if (demoRecordSlot_ == 0) autoplayExitCode_ = 1;
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
            info.line1 = info.recorded ? "Recorded" : ok ? "Shipped" : "Empty";
            if (ok)
            {
                const int mission = demo.log.start.mission;
                const bool galaxy = demo.log.start.worldPath.find("/galaxy/world") != std::string::npos;
                info.line2 = (galaxy ? "Galaxy " : "Mission ") + std::to_string(mission);
            }
            else if (info.recorded)
            {
                info.line2 = "(unplayable)";
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
        // The demo positions (BD-0116): over the pause screen, or from the menu's Demos.
        constexpr int kRows = GalaxyEggbert::Game::InputPad::kDemoListRows;
        const int pageFirst = demoImportPage_ * kRows;
        const int pageRows = std::clamp(static_cast<int>(demoImports_.size()) - pageFirst, 0, kRows);
        const auto slots = inputPad_.UpdateDemoSlots(
            mouse, viewportW, viewportH, demoSlotInfos_,
            demoSlotsMode_ == 3 ? pageRows : -1, demoSlotsMode_ == 3 && DemoImportPages() > 1);
        if (demoSlotsMode_ == 3 && GalaxyEggbert::Editor::ConsumeUploadDone())
        {
            demoImportPage_ = 0;
            ListDemoImports(); // the file just chosen is the newest, the first row
            demoSlotsMessage_ = "The file is listed: click it to import it.";
        }
        const int armedBefore = demoDeleteArmed_;
        if (slots.importRow > 0 || slots.prevPage || slots.nextPage || slots.recordSlot > 0 ||
            slots.restoreSlot > 0 || slots.exportPressed || slots.importPressed || slots.cancelPressed)
            demoDeleteArmed_ = -1; // any other click: the bin asks again
        if (slots.deleteRow > 0 && demoSlotsMode_ == 3 && pageFirst + slots.deleteRow - 1 < static_cast<int>(demoImports_.size()))
        {
            // A shared demo deleted (two clicks): on the web the only way to reach it.
            const int index = pageFirst + slots.deleteRow - 1;
            const auto file = demoImports_[static_cast<std::size_t>(index)];
            const std::string name = std::filesystem::path(file).filename().string();
            if (armedBefore != index)
            {
                demoDeleteArmed_ = index;
                demoSlotsMessage_ = "Delete " + name + "? Click its bin again.";
            }
            else
            {
                std::error_code error;
                std::filesystem::remove(file, error);
                ListDemoImports();
                demoSlotsMessage_ = error ? name + " could not be deleted." : name + " deleted.";
            }
        }
        else if (slots.prevPage || slots.nextPage)
        {
            demoImportPage_ = std::clamp(demoImportPage_ + (slots.nextPage ? 1 : -1), 0, DemoImportPages() - 1);
        }
        else if (slots.importRow > 0 && demoSlotsMode_ == 3 &&
                 pageFirst + slots.importRow - 1 < static_cast<int>(demoImports_.size()))
        {
            demoImportIndex_ = static_cast<std::size_t>(pageFirst + slots.importRow - 1);
            demoSlotsMode_ = 2;
            demoSlotsMessage_ = DemoImportPrompt();
        }
        else if (slots.recordSlot > 0)
        {
            HandleDemoSlotClick(slots.recordSlot);
        }
        else if (slots.exportPressed)
        {
            demoSlotsMode_ = demoSlotsMode_ == 1 ? 0 : 1;
            demoSlotsMessage_ = demoSlotsMode_ == 1 ? "Click a position to export its demo." : "";
        }
        else if (slots.importPressed)
        {
            ToggleDemoImport();
        }
        else if (slots.restoreSlot > 0)
        {
            std::error_code error;
            std::filesystem::remove(GalaxyEggbert::Game::UserDemoPath(slots.restoreSlot), error);
            demoSlotInfos_ = DemoSlotInfos();
            demoSlotsMessage_ = "Position " + std::to_string(slots.restoreSlot) + " plays its shipped demo again.";
            std::cout << "GalaxyEggbertCNA: " << demoSlotsMessage_ << std::endl;
        }
        else if (slots.cancelPressed)
        {
            // Cancel leaves Export or Import first, then the positions.
            if (demoSlotsMode_ != 0)
            {
                demoSlotsMode_ = 0;
                demoSlotsMessage_.clear();
            }
            else
            {
                demoSlotsShown_ = false;
            }
        }
    }

    void GalaxyEggbertGame::DrawDemoSlotsScreen(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                                int viewportW, int viewportH)
    {
        // The list's page (Import, mode 3).
        const int demoPageFirst = demoImportPage_ * GalaxyEggbert::Game::InputPad::kDemoListRows;
        std::vector<std::string> demoPageRows;
        for (int i = demoPageFirst; i < static_cast<int>(demoImportRows_.size()) &&
                                    i < demoPageFirst + GalaxyEggbert::Game::InputPad::kDemoListRows; ++i)
            demoPageRows.push_back(demoImportRows_[static_cast<std::size_t>(i)]);
        inputPad_.DrawDemoSlots(device, viewportW, viewportH,
                                    demoSlotInfos_, demoSlotsMessage_,
                                    demoSlotsMode_ == 1   ? "Export a demo: choose its position"
                                    : demoSlotsMode_ == 2 ? "Import a demo: choose its position"
                                    : demoSlotsMode_ == 3 ? "Import a demo: choose the file"
                                    : phase_ == GalaxyEggbert::Def::GamePhase::Init
                                        ? "The demo positions"
                                        : "Record a demo: choose its position",
                                    demoSlotsMode_ == 3 ? 2 : demoSlotsMode_,
                                    demoSlotsMode_ == 3 ? &demoPageRows : nullptr,
                                    demoDeleteArmed_ >= demoPageFirst &&
                                            demoDeleteArmed_ < demoPageFirst + GalaxyEggbert::Game::InputPad::kDemoListRows
                                        ? demoDeleteArmed_ - demoPageFirst
                                        : -1,
                                    DemoImportPages() > 1 ? std::to_string(demoImportPage_ + 1) + " / " +
                                                                std::to_string(DemoImportPages())
                                                          : std::string());
    }

    std::string GalaxyEggbertGame::DemoImportPrompt() const
    {
        if (demoImports_.empty())
            return "Put .gdemo files in " + GalaxyEggbert::Game::SharedDemoDir() + "/ to import them.";
        if (demoSlotsMode_ == 3) return "Click the file to import.";
        const std::string name = std::filesystem::path(demoImports_[demoImportIndex_]).filename().string();
        return "Import " + name + ": click a position (Import: the list again).";
    }

    int GalaxyEggbertGame::DemoImportPages() const noexcept
    {
        constexpr int kRows = GalaxyEggbert::Game::InputPad::kDemoListRows;
        return std::max(1, (static_cast<int>(demoImports_.size()) + kRows - 1) / kRows);
    }

    void GalaxyEggbertGame::ListDemoImports()
    {
        using namespace GalaxyEggbert::Game;
        demoImports_ = ListSharedDemos();
        demoImportIndex_ = 0;
        demoDeleteArmed_ = -1;
        demoImportPage_ = std::min(demoImportPage_, DemoImportPages() - 1);
        demoImportRows_.clear();
        for (const auto& path : demoImports_)
        {
            // The file and what it holds: its mission, a galaxy's, or why it cannot be imported.
            std::string row = std::filesystem::path(path).filename().string() + "  -  ";
            DemoRecording demo;
            const auto text = ReadWorldBytes(path);
            const auto status = text ? ParseDemo(*text, kDemoSimulationRevision, demo) : DemoLoadStatus::Missing;
            if (status == DemoLoadStatus::Ok)
                row += (demo.log.start.worldPath.find("/galaxy/world") != std::string::npos ? "galaxy " : "mission ") +
                       std::to_string(demo.log.start.mission) + ", " + std::to_string([&demo] {
                           std::uint64_t ticks = 0;
                           for (const auto& run : demo.log.runs) ticks += run.count;
                           return ticks / 60;
                       }()) + " s";
            else
                row += DemoLoadStatusText(status);
            demoImportRows_.push_back(std::move(row));
        }
    }

    void GalaxyEggbertGame::ToggleDemoImport()
    {
        // Import shows the list of the shared files; from a file chosen, the list again; from the
        // list, off.
        if (demoSlotsMode_ == 3)
        {
            demoSlotsMode_ = 0;
            demoSlotsMessage_.clear();
            return;
        }
        const bool fromChosen = demoSlotsMode_ == 2;
        demoSlotsMode_ = 3;
        demoImportPage_ = 0;
        ListDemoImports();
        demoSlotsMessage_ = !fromChosen && GalaxyEggbert::Editor::RequestUpload(GalaxyEggbert::Game::SharedDemoDir(), ".gdemo")
                                ? "Choose a .gdemo file: it is listed here."
                                : DemoImportPrompt();
    }

    void GalaxyEggbertGame::HandleDemoSlotClick(int slot)
    {
        using namespace GalaxyEggbert::Game;
        const std::string position = "Position " + std::to_string(slot);
        if (demoSlotsMode_ == 0)
        {
            if (phase_ == GalaxyEggbert::Def::GamePhase::Init)
                demoSlotsMessage_ = "A recording is made in a mission: Pause > Record.";
            else
                StartPlayerRecording(slot);
            return;
        }
        if (demoSlotsMode_ == 1)
        {
            demoSlotsMode_ = 0;
            const auto exported = ExportDemo(DemoSlotPath(slot), saveData_.GetSelectedGamer(), slot);
            if (exported.empty())
            {
                demoSlotsMessage_ = position + " has no demo to export.";
                return;
            }
            const std::string name = std::filesystem::path(exported).filename().string();
            demoSlotsMessage_ = GalaxyEggbert::Editor::OfferDownload(exported, name)
                                    ? "Demo " + std::to_string(slot) + " exported: downloaded as " + name + "."
                                    : "Demo " + std::to_string(slot) + " exported to " + exported + ".";
            std::cout << "GalaxyEggbertCNA: " << demoSlotsMessage_ << std::endl;
            return;
        }
        if (demoImports_.empty())
        {
            demoSlotsMessage_ = DemoImportPrompt();
            return;
        }
        demoSlotsMode_ = 0;
        const std::string source = demoImports_[demoImportIndex_];
        const std::string name = std::filesystem::path(source).filename().string();
        const auto status = ImportDemo(source, slot);
        if (status != DemoLoadStatus::Ok)
        {
            demoSlotsMessage_ = name + " cannot be imported: " + DemoLoadStatusText(status) + ".";
            std::cout << "GalaxyEggbertCNA: " << demoSlotsMessage_ << std::endl;
            return;
        }
        demoSlotInfos_ = DemoSlotInfos();
        // Whether it can play here: its world, the campaign's or a galaxy's.
        DemoRecording demo;
        const auto text = ReadWorldBytes(UserDemoPath(slot));
        int galaxy = -1;
        bool playable = false;
        if (text && ParseDemo(*text, kDemoSimulationRevision, demo) == DemoLoadStatus::Ok)
        {
            const auto world = ResolveDemoWorld(demo, galaxy);
            playable = DemoWorldStatus(demo, world, ReadWorldBytes(world)) == DemoLoadStatus::Ok;
        }
        demoSlotsMessage_ = position + " plays " + name + " now." +
                            (playable ? "" : " Its world is not here: it is skipped until it is.");
        std::cout << "GalaxyEggbertCNA: " << demoSlotsMessage_ << std::endl;
    }

    void GalaxyEggbertGame::StartPlayerRecording(int slot)
    {
        using namespace GalaxyEggbert::Game;
        demoSlotsShown_ = false;
        const int mission = worldRuntime_.GetMissionNumber();
        // Speedy Blupi's DemoRecStart(): the mission from its start, at the normal speed.
        demoRecordSlot_ = slot;
        demoRecordPath_ = UserDemoPath(slot);
        demoRecordMaxTicks_ = 0; // kDemoMaxTicks, ten minutes
        demoRecordTitle_ = std::string("Recorded by player ") + static_cast<char>('A' + saveData_.GetSelectedGamer()) +
                           (galaxyGamer_ >= 0 ? ": " + GalaxyEggbert::Editor::GalaxyMissionName(mission) + " of a galaxy"
                                              : ": mission " + std::to_string(mission));
        demoRecordArmed_ = true;
        deferredMission_ = 0;
        gameSpeed_ = GalaxyEggbert::Def::GameSpeed::Normal;
        LoadMission(mission);
        if (!missionStartPending_)
        {
            // Its world could not be loaded again: nothing is armed, the game goes on as it was.
            demoRecordSlot_ = 0;
            demoRecordPath_.clear();
            demoRecordTitle_.clear();
            demoRecordArmed_ = false;
            ShowDemoNotice("The demo could not be recorded.");
            return;
        }
        SetPhase(GalaxyEggbert::Def::GamePhase::Play, /*bypassFade=*/true);
        std::cout << "GalaxyEggbertCNA: demo position " << slot << ": mission " << mission
                  << " again from its start, recorded." << std::endl;
    }

    void GalaxyEggbertGame::EndPlayerRecording(bool written)
    {
        const int slot = demoRecordSlot_;
        demoRecordSlot_ = 0;
        demoRecordPath_.clear();
        demoRecordTitle_.clear();
        demoRecordArmed_ = false;
        demoEndRequested_ = false;
        ShowDemoNotice(written ? "Demo recorded in position " + std::to_string(slot) + "."
                               : "The demo could not be recorded.");
        if (const int mission = std::exchange(deferredMission_, 0); mission != 0)
        {
            LoadMission(mission); // where the recorded mission was going
        }
    }

    Microsoft::Xna::Framework::Input::KeyboardState GalaxyEggbertGame::CurrentKeyboardState() const
    {
        using namespace Microsoft::Xna::Framework::Input;
        std::unordered_set<Keys> keys;
        for (int key : tickInput_.keys) keys.insert(static_cast<Keys>(key));
        return KeyboardState(keys);
    }

}
