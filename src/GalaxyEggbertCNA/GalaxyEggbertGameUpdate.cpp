// GalaxyEggbertGame: Update() and the simulation tick (UpdateTick). Split out of GalaxyEggbertGame.cpp.
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

    void GalaxyEggbertGame::Update(Microsoft::Xna::Framework::GameTime& gameTime)
    {
        Game::Update(gameTime);
        {
            // Real seconds for the menu's idle timer (DemoReplay.hpp), whatever the game's speed;
            // a long stall (a suspended machine) counts as a quarter of a second.
            const auto now = std::chrono::steady_clock::now();
            demoClockSeconds_ = demoClockStarted_
                ? std::min(0.25f, std::chrono::duration<float>(now - demoClock_).count()) : 0.0f;
            demoClock_ = now;
            demoClockStarted_ = true;
        }
        if (replayingResume_)
        {
            ReplayBatch();
            SuppressDraw();
            return;
        }
        if (demoVerifyPending_)
        {
            demoVerifyPending_ = false;
            if (!StartDemoFile(demoVerifyPath_, 0))
            {
                std::cout << "DEMO VERIFY " << demoVerifyPath_ << ": cannot be played" << std::endl;
                demoVerifying_ = false;
                autoplayExitCode_ = 1;
                Exit();
                return;
            }
        }
        UpdateMusic();
        if (demoActive_)
        {
            UpdateDemo();
            return;
        }

        if (goldenCaptureMode_ && goldenCaptureAwaitingDraw_)
        {
            return;
        }

        // INFRA-001 (plan.md §7) -- see EnableGoldenCaptureMode()'s own
        // comment. The one-time setup runs here; the actual screenshot
        // capture (needs a rendered frame) happens in Draw().
        if (goldenCaptureMode_)
        {
            if (!goldenCaptureArmed_)
            {
                goldenCaptureArmed_ = true;
                if (phase_ != GalaxyEggbert::Def::GamePhase::Play)
                {
                    SetPhase(GalaxyEggbert::Def::GamePhase::Play, /*bypassFade=*/true);
                }
                LoadMission(999);
            }
            ++goldenCaptureTick_;
        }

        // INFRA-002's behavioral-trace mode -- see EnableGoldenTraceMode()'s
        // own comment. One-time setup mirrors goldenCaptureMode_'s own
        // shape above; SetYaw() here (not a turnInput tick) is the "fixed
        // camera/orientation" part of the scripted scenario, same category
        // as LoadMission(999)'s own fixed-world setup, not something a real
        // player's turn input would organically reach in exactly 1 tick.
        if (goldenTraceMode_)
        {
            if (!goldenTraceArmed_)
            {
                goldenTraceArmed_ = true;
                if (phase_ != GalaxyEggbert::Def::GamePhase::Play)
                {
                    SetPhase(GalaxyEggbert::Def::GamePhase::Play, /*bypassFade=*/true);
                }
                LoadMission(999);
                blupi_.SetYaw(-1.5707963f); // face west, down the tested corridor
            }
        }

        // `--mission N` (EnableStartMission()): one-time, the same way the
        // capture modes above load their fixed world.
        if (startMission_ != 0)
        {
            SetPhase(GalaxyEggbert::Def::GamePhase::Play, /*bypassFade=*/true);
            if (startGalaxy_)
            {
                galaxyGamer_ = saveData_.GetSelectedGamer();
                saveData_.SetGalaxyMode(true);
                std::cout << "GalaxyEggbertCNA: --galaxy: player " << static_cast<char>('A' + galaxyGamer_)
                          << "'s galaxy." << std::endl;
            }
            LoadMission(startMission_);
            startMission_ = 0;
            if (startSpawnSet_)
            {
                PlaceBlupi(startSpawnX_ - WorldRuntime::kWorldCenterX, startSpawnY_,
                           startSpawnZ_ - WorldRuntime::kWorldCenterZ);
                blupi_.SetYaw(startYawDegrees_ * 3.14159265f / 180.0f);
                chaseCamera_.Reset(blupi_.GetYaw());
                cameraSmoothedInitialized_ = false;
            }
        }

        if (missionStartPending_ && phase_ == GalaxyEggbert::Def::GamePhase::Play &&
            fadeOutPhase_ == GalaxyEggbert::Def::GamePhase::None)
        {
            StartResumeLog();
            if (demoRecordArmed_) BeginDemoRecording();
        }
        if (autoplay_ && !autoplay_->Finished())
        {
            StepAutoplay();
            if ((autoplayShotEvery_ > 0 && autoplayTick_ % autoplayShotEvery_ == 0) ||
                (autoplay_ && autoplay_->WantsShot()))
            {
                autoplayShotPending_ = true;
            }
            // Drawing is most of a tick's time (software rendering under Xvfb) and changes
            // nothing in play: one frame in 30.
            if (autoplayTick_ % 30 != 0 && !autoplayShotPending_)
            {
                SuppressDraw();
            }
        }
        CaptureInput();
        if (demoRecording_)
        {
            using Microsoft::Xna::Framework::Input::Keys;
            // Escape (a gamepad's Start) ends a developer's recording before it would pause (the
            // player's pauses, which ends it); the F-keys (camera, fullscreen, help, game speed) and
            // Tab (slow) are left out of both.
            if (demoRecordSlot_ == 0 &&
                std::binary_search(tickInput_.keys.begin(), tickInput_.keys.end(), static_cast<int>(Keys::Escape)))
            {
                FinishDemoRecording();
                Exit();
                return;
            }
            tickInput_.keys.erase(std::remove_if(tickInput_.keys.begin(), tickInput_.keys.end(), [](int key) {
                return key == static_cast<int>(Keys::Tab) ||
                       (key >= static_cast<int>(Keys::F1) && key <= static_cast<int>(Keys::F24));
            }), tickInput_.keys.end());
        }
        if (UpdateDemoMenu()) return;
        UpdateTick();
        if (recordingResume_ && !missionStartPending_ && !replayingResume_)
        {
            resumeLog_.Append(tickInput_);
            ++resumeTicks_;
            CheckResumeSelftest();
        }
        if (demoRecording_)
        {
            demoRecordLog_.Append(tickInput_);
            const std::uint64_t limit = demoRecordMaxTicks_ > 0
                ? std::min<std::uint64_t>(static_cast<std::uint64_t>(demoRecordMaxTicks_), GalaxyEggbert::Game::kDemoMaxTicks)
                : GalaxyEggbert::Game::kDemoMaxTicks;
            if (demoEndRequested_ || demoRecordLog_.TickCount() >= limit || (autoplay_ && autoplay_->Finished()))
            {
                const bool written = FinishDemoRecording();
                if (demoRecordSlot_ == 0)
                    Exit();
                else
                    EndPlayerRecording(written);
            }
        }
        if (xmissionPending_)
        {
            xmissionPending_ = false;
            OpenMissionInEditor();
        }
        demoNoticeSeconds_ = std::max(0.0f, demoNoticeSeconds_ - demoClockSeconds_);
    }

    void GalaxyEggbertGame::UpdateTick()
    {
        // While he is hidden his own sounds are not played (Decor::PlaySound, BD-0075); in play only.
        sound_.SetBlupiHidden(phase_ == GalaxyEggbert::Def::GamePhase::Play && blupi_.IsHidden());
        // The world's sounds are heard from Blupi, as the original's from its view (BD-0076).
        sound_.SetListener(blupi_.GetX(), blupi_.GetY(), blupi_.GetZ());

        constexpr float dt = 1.0f / 60.0f;
        const auto helpKeys = CurrentKeyboardState();
        auto helpMouse=CurrentMouseState();
        bool helpPointerDown=helpMouse.getLeftButtonProperty()==Microsoft::Xna::Framework::Input::ButtonState::Pressed;
        float helpX=helpMouse.getXProperty(),helpY=helpMouse.getYProperty();
        for (const auto& touch : tickInput_.touches)
        {
            using Microsoft::Xna::Framework::Input::Touch::TouchLocationState;
            if (touch.state==int(TouchLocationState::Pressed) || touch.state==int(TouchLocationState::Moved))
            {
                helpPointerDown=true;helpX=touch.x;helpY=touch.y;break;
            }
        }
        if (controlsHelp_.Update(helpKeys,helpPointerDown,
            GalaxyEggbert::Game::ControlsHelp::OverClose(helpX,helpY,tickInput_.viewportWidth,tickInput_.viewportHeight)))
        {
            inputPad_.ResetTouchState();
            mouseLookActive_ = false;
            pauseKeyWasDown_ = helpKeys.IsKeyDown(Microsoft::Xna::Framework::Input::Keys::Escape);
            menuConfirmWasDown_ = helpKeys.IsKeyDown(Microsoft::Xna::Framework::Input::Keys::Enter);
            return;
        }
        phaseTimeSeconds_ += dt;

        // Real fade-out phase-transition commit (plan.md MENU-088/089) --
        // freezes ALL input/simulation while a fade is pending (matching
        // the real source's own early-return before `inputPad.Update()`/
        // `decor.MoveStep()`), committing the deferred phase once the
        // real 1.0s window elapses. See SetPhase()'s own comment for the
        // full real deferred-transition mechanic this drives.
        if (fadeOutPhase_ != GalaxyEggbert::Def::GamePhase::None)
        {
            if (phaseTimeSeconds_ >= kFadeCommitDurationSeconds)
            {
                // Real "fadeOutPhase != None is REQUIRED to defer" guard
                // (see SetPhase()'s own comment) -- calling SetPhase()
                // with fadeOutPhase_ STILL set to the pending target
                // (not pre-cleared) makes its own `sourceDefers &&
                // fadeOutPhase_==None` condition false, so it correctly
                // falls through to the commit branch instead of
                // re-deferring to the same target forever.
                SetPhase(fadeOutPhase_);
            }
            return;
        }

        // Play on-screen control input (2026-07-13, plan.md MENU-021..027)
        // -- computed here, in Update()'s own top-level scope, so the
        // movement block further below (still inside the Play-gated
        // section) can read it after the early return this block itself
        // may trigger. Left default-zero when phase_ isn't Play (that
        // branch below never runs inputPad_.UpdatePlay()). inputPadClaimedMouse
        // tells the pre-existing mouse drag-look code (further below) that
        // this frame's press landed on an on-screen control, so the two
        // features don't fight over the same left-mouse-button input.
        InputPad::PlayInput padPlayInput;
        bool inputPadClaimedMouse = false;

        // Phase transitions (plan.md HUD-023) -- read regardless of the
        // current phase, since the big Play-gated simulation block below
        // (and its own input reading) doesn't run at all outside Play.
        if (UpdateTickScreens(dt, padPlayInput, inputPadClaimedMouse))
        {
            return;
        }

        // Galaxy's loading screen (OR-0009) and the editor's world list (OR-0012) stand on the
        // title scene as the menu does; a world left in the editor gives way to it.
        const bool editorBrowsing =
            phase_ == GalaxyEggbert::Def::GamePhase::Editor && worldEditor_.IsBrowsing();
        if ((phase_ == GalaxyEggbert::Def::GamePhase::Wait || editorBrowsing) && !titleSceneActive_ &&
            !titleSceneUnavailable_)
        {
            titleSceneRequested_ = true;
        }
        if (titleSceneRequested_)
        {
            LoadTitleScene();
        }
        if ((phase_ == GalaxyEggbert::Def::GamePhase::Init || phase_ == GalaxyEggbert::Def::GamePhase::Wait ||
             editorBrowsing) &&
            titleSceneActive_)
        {
            UpdateTitleScene(dt);
        }

        if (missionStartPending_) return;
        if (phase_ != GalaxyEggbert::Def::GamePhase::Play)
        {
            return;
        }

        // Real GalaxyEggbert::Def::GameSpeed (SCORE-009/010/011) applied as a continuous
        // dt-scale factor on the core simulation only (see gameSpeed_'s
        // own member comment for the full rationale) -- everything else in
        // this Update() (camera, sound one-shot timers, animation-icon
        // selection, cheat/menu input already handled above) keeps using
        // the real, unscaled `dt` from the top of this function.
        const float simDt = dt * GameSpeedFactor(gameSpeed_);

        worldRuntime_.Update(simDt);

        if (terrainRenderer_ && UpdateTickWorld(dt, simDt, padPlayInput, inputPadClaimedMouse))
        {
            return; // the world left (a mission over, the bye) or a phase took over
        }

        if (phase_ == GalaxyEggbert::Def::GamePhase::Play && blupi_.CanInteractWithWorld())
            RecordHideGhost(); // Decor.cpp:5238-5254: no new Hide trail while ghosting

        if (goldenCaptureMode_ && goldenCaptureNextIndex_ < kGoldenCaptureCount &&
            goldenCaptureTick_ >= kGoldenCaptureTicks[goldenCaptureNextIndex_])
        {
            goldenCaptureAwaitingDraw_ = true;
        }
    }


    // UpdateTick's screens: every phase's input (the menus, Setup, Pause, Win/Lost, the editor, the
    // demos' screens, the phase keys). True when the tick ends here.
    bool GalaxyEggbertGame::UpdateTickScreens(float dt, InputPad::PlayInput& padPlayInput, bool& inputPadClaimedMouse)
    {
        using Microsoft::Xna::Framework::Input::ButtonState;
        using Microsoft::Xna::Framework::Input::Keys;
        auto phaseKeys = CurrentKeyboardState();
        const bool fullscreenDown = phaseKeys.IsKeyDown(Keys::F11);
        if (fullscreenDown && !fullscreenKeyWasDown_) ToggleFullscreen();
        fullscreenKeyWasDown_ = fullscreenDown;
        const bool confirmDown = phaseKeys.IsKeyDown(Keys::Enter);
        const bool menuConfirmPressed = confirmDown && !menuConfirmWasDown_;
        menuConfirmWasDown_ = confirmDown;
        auto viewport = getGraphicsDeviceProperty().getViewportProperty();
        viewport.setWidthProperty(tickInput_.viewportWidth);
        viewport.setHeightProperty(tickInput_.viewportHeight);

        auto phaseMouse = CurrentMouseState();
        GalaxyEggbert::Editor::WorldEditor::Navigation editorNavigation;
        if (phase_ == GalaxyEggbert::Def::GamePhase::Editor || phase_ == GalaxyEggbert::Def::GamePhase::Init)
        {
            const bool editing = phase_ == GalaxyEggbert::Def::GamePhase::Editor && !worldEditor_.IsBrowsing();
            std::vector<InputPad::TouchPoint> touches;
            using Microsoft::Xna::Framework::Input::Touch::TouchLocationState;
            for (const auto& t : tickInput_.touches)
            {
                const auto state = static_cast<TouchLocationState>(t.state);
                if (state != TouchLocationState::Invalid)
                    touches.push_back({t.id,t.x,t.y,state == TouchLocationState::Pressed,state == TouchLocationState::Released});
            }
            auto device = editorDevices_.Update(phaseKeys, phaseMouse,
                SuppressLiveInput() || replayingResume_ || menuInputLatched_ ? GalaxyEggbert::Game::GamepadFrame{}
                                                                            : ReadGamepadFrame(),
                touches, tickInput_.touchControlsEnabled, editing,
                editing && worldEditor_.Palette().IsPopupOpen(),
                viewport.getWidthProperty(), viewport.getHeightProperty(), dt,
                [&](int x,int y) {
                    using namespace Microsoft::Xna::Framework::Input;
                    return editing && worldEditor_.Palette().IsPointerOverControl(
                        MouseState(x,y,0,ButtonState::Released,ButtonState::Released,ButtonState::Released,
                            ButtonState::Released,ButtonState::Released),
                        viewport.getWidthProperty(),viewport.getHeightProperty());
                });
            phaseKeys = device.keyboard;
            phaseMouse = device.mouse;
            editorNavigation = device.navigation;
        }

        bool mousePausePressed = false;
        bool mouseContinuePressed = false;
        bool mouseRestartPressed = false;
        bool mouseSetupPressed = false;
        bool mouseMenuPressed = false;
        bool mouseBackPressed = false;
        if (phase_ == GalaxyEggbert::Def::GamePhase::Play)
        {
            if (UpdateScreenPlay(phaseKeys, viewport, padPlayInput, inputPadClaimedMouse, mousePausePressed))
            {
                return true;
            }
        }
        else if (phase_ == GalaxyEggbert::Def::GamePhase::Pause)
        {
            const auto mouse = CurrentMouseState();
            // Real conditional visibility: Back/Restart hidden on
            // mission 1 (can't go "back" from/replay the very first
            // level); Restart additionally hidden on decade-boundary
            // missions (real `mission % 10 != 0` gate).
            const int mission = worldRuntime_.GetMissionNumber();
            const bool showBack = mission != 1;
            const bool showRestart = mission != 1 && mission % 10 != 0;
            if (demoSlotsShown_)
            {
                UpdateDemoSlotsScreen(mouse, viewport.getWidthProperty(), viewport.getHeightProperty());
            }
            else
            {
                const auto pauseInput = inputPad_.UpdatePause(
                    mouse, viewport.getWidthProperty(), viewport.getHeightProperty(), showBack, showRestart,
                    CanRecordDemo(), CanRevertWorld());
                if (pauseInput.originalPressed)
                {
                    if (revertArmed_) RevertWorld();
                    else revertArmed_ = true;
                }
                mouseContinuePressed = pauseInput.continuePressed;
                mouseRestartPressed = pauseInput.restartPressed;
                mouseSetupPressed = pauseInput.setupPressed;
                mouseMenuPressed = pauseInput.menuPressed;
                mouseBackPressed = pauseInput.backPressed;
                if (pauseInput.recordPressed)
                {
                    demoSlotsShown_ = true;
                    demoSlotsMode_ = 0;
                    demoSlotInfos_ = DemoSlotInfos();
                    demoSlotsMessage_.clear();
                }
            }
        }
        else if (phase_ == GalaxyEggbert::Def::GamePhase::PlaySetup ||
                 phase_ == GalaxyEggbert::Def::GamePhase::MainSetup)
        {
            // Real PlaySetup (2026-07-13, plan.md MENU-058..069),
            // reachable via Pause's real Setup button, and MainSetup
            // (2026-07-13, plan.md MENU-006..020), reachable via
            // Init's own InitSetup button -- sharing UpdateSetup()/
            // DrawSetup(), the real "MainSetup additionally shows
            // SetupReset" difference (see InputPad::UpdateSetup()'s
            // own class comment) is the only distinction made here.
            const bool isMainSetup = phase_ == GalaxyEggbert::Def::GamePhase::MainSetup;
            const auto mouse = CurrentMouseState();
            const auto setupInput = inputPad_.UpdateSetup(
                mouse, viewport.getWidthProperty(), viewport.getHeightProperty(), isMainSetup,
                saveData_.GetAccelActive());
            if (setupInput.soundsToggled)
            {
                // Real SetupSounds toggle -- a genuinely meaningful
                // desktop equivalent, wired to the pre-existing
                // Sound::SetEnabled()/IsEnabled(). Persisted
                // immediately (2026-07-13, plan.md MENU-067, see
                // SaveData.hpp), matching the real source's own
                // `gameData.setSoundActiveProperty(...);
                // gameData.Write();` -- write-on-toggle, not a timer
                // or continuous autosave.
                sound_.SetEnabled(!sound_.IsEnabled());
                saveData_.SetSoundEnabled(sound_.IsEnabled());
                SavePlayerData();
            }
            // The volume sliders (a Galaxy addition, the owner's request of 2026-11-02): live
            // while dragged, saved when let go.
            if (setupInput.effectsVolume >= 0)
            {
                saveData_.SetEffectsVolume(setupInput.effectsVolume);
                sound_.SetEffectsVolume(static_cast<float>(saveData_.GetEffectsVolume()) / 100.0f);
            }
            if (setupInput.musicVolume >= 0)
            {
                saveData_.SetMusicVolume(setupInput.musicVolume);
                ApplyMusicVolume();
            }
            if (setupInput.accelSensitivity >= 0)
            {
                saveData_.SetAccelSensitivity(setupInput.accelSensitivity);
            }
            if (setupInput.volumeReleased)
            {
                SavePlayerData();
                // A sample of the effects at the new volume (the Sounds toggle's own click).
                sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel0);
            }
            // The other toggles (OR-0010), saved at once as the original's GameData::Write().
            if (setupInput.jumpToggled)
            {
                saveData_.SetJumpRight(!saveData_.GetJumpRight());
                inputPad_.SetJumpRight(saveData_.GetJumpRight());
                SavePlayerData();
            }
            if (setupInput.zoomToggled)
            {
                saveData_.SetAutoZoom(!saveData_.GetAutoZoom());
                SavePlayerData();
            }
            if (setupInput.accelToggled)
            {
                saveData_.SetAccelActive(!saveData_.GetAccelActive());
                SavePlayerData();
            }
            if (setupInput.terrainToggled)
            {
                // 3D terrain on or off (SaveData::GetTerrainRelief): the terrain is built again
                // with the reliefs or without them, their memory freed when off.
                saveData_.SetTerrainRelief(!saveData_.GetTerrainRelief());
                SavePlayerData();
                ApplyTerrainReliefSetting();
            }
            if (setupInput.effectsToggled)
            {
                // 3D effects on or off (SaveData::GetEffects3D): the next frame is drawn with or
                // without them.
                saveData_.SetEffectsLevel((saveData_.GetEffectsLevel() + 1) % 4);
                SavePlayerData();
                // The terrain built again, with or without its ambient occlusion.
                if (terrainRenderer_ && terrainRenderer_->AmbientOcclusion() != TerrainOcclusionWanted())
                {
                    ApplyTerrainReliefSetting();
                }
            }
            if (setupInput.resetPressed)
            {
                // Real SetupReset: `gameData.Reset(); gameData.Write();`
                // -- the SAME full reset as Cheat5 (confirmed
                // 2026-07-13 via Game1.cpp), only reachable from
                // MainSetup (isMainSetup gates its rect/visibility).
                saveData_.Reset();
                SavePlayerData();
                inputPad_.SetJumpRight(saveData_.GetJumpRight());
                sound_.SetEffectsVolume(static_cast<float>(saveData_.GetEffectsVolume()) / 100.0f);
                ApplyMusicVolume();
            }
            if (setupInput.returnPressed ||
                (phaseKeys.IsKeyDown(Keys::Escape) && !pauseKeyWasDown_))
            {
                // Real SetupReturn: `if (playSetup) SetPhase(Play,-1);
                // else SetPhase(Init);` -- both branches are now
                // reachable here. Escape is this engine's own
                // keyboard pick for the same action (real source has
                // no separate Setup-phase keyboard binding confirmed).
                SetPhase(isMainSetup ? GalaxyEggbert::Def::GamePhase::Init : GalaxyEggbert::Def::GamePhase::Play);
            }
        }
        else if (phase_ == GalaxyEggbert::Def::GamePhase::Wait)
        {
            // Real Wait (plan.md MENU-001..005): a fixed 5.0s
            // wall-clock cosmetic timer (confirmed via research,
            // decoupled from actual asset loading, which already
            // finished synchronously in LoadContent()). See
            // InputPad::DrawWait()'s own class comment for the real
            // non-linear waitTable gauge-fill curve this timer drives.
            if (phaseTimeSeconds_ >= kWaitDurationSeconds)
            {
                // Game1.cpp:255-270: a saved mission takes precedence over the checkpoint.
                ReadResumeLog();
                SetPhase((pendingResume_ || saveData_.GetHasProgress()) ? GalaxyEggbert::Def::GamePhase::Resume
                                                     : GalaxyEggbert::Def::GamePhase::Init);
            }
        }
        else if (phase_ == GalaxyEggbert::Def::GamePhase::Init)
        {
            // Real Init / gamer-select menu (2026-07-13, plan.md
            // MENU-006..020) -- see InputPad::UpdateInit()/
            // DrawInit()'s own class comment for full detail.
            const auto mouse = phaseMouse;
            if (demoSlotsShown_)
            {
                // The demo positions from the menu's Demos button: Escape closes them, not the game.
                UpdateDemoSlotsScreen(mouse, viewport.getWidthProperty(), viewport.getHeightProperty());
                const bool escape = phaseKeys.IsKeyDown(Keys::Escape);
                if (escape && !pauseKeyWasDown_) demoSlotsShown_ = false;
                pauseKeyWasDown_ = escape;
                return true;
            }
            auto initInput = inputPad_.UpdateInit(
                mouse, viewport.getWidthProperty(), viewport.getHeightProperty());
            if (initInput.demosPressed)
            {
                demoSlotsShown_ = true;
                demoSlotsMode_ = 0;
                demoSlotInfos_ = DemoSlotInfos();
                demoSlotsMessage_ = "Export, Import or Original here; record in a mission: Pause > Record.";
                return true;
            }
            if (initInput.fullscreenPressed) ToggleFullscreen();
            if (initInput.helpPressed)
            {
                controlsHelp_.shown=true;
                controlsHelp_.cheats=false;
                return true;
            }
            // The Demo button plays the demos as the idle timer does (UpdateDemoMenu()).
            if (initInput.demoPressed)
            {
                demoNumber_ = 0;
                if (!StartNextDemo())
                    std::cout << "GalaxyEggbertCNA: no demo can be played." << std::endl;
                return true;
            }
            // The selected player's own galaxy (BD-0119).
            if (initInput.galaxyPressed)
            {
                StartGalaxy();
                return true;
            }
            // Enter (a gamepad's A, GamepadInput.hpp) plays as the Play button does: the menu
            // without a pointer (owner 2026-11-02: gamepads).
            {
                const bool enterDown = phaseKeys.IsKeyDown(
                    Microsoft::Xna::Framework::Input::Keys::Enter);
                if (enterDown && !initEnterWasDown_)
                {
                    initInput.playPressed = true;
                }
                initEnterWasDown_ = enterDown;
            }
            if (initInput.gamerSelected >= 0)
            {
                // Real Game1::SetGamer(): a single tap immediately
                // selects AND persists that slot, independent of
                // entering Play (confirmed via research).
                saveData_.SetSelectedGamer(initInput.gamerSelected);
                SavePlayerData();
            }
            if (initInput.exitPressed)
            {
                Exit();
                return true;
            }
            if (initInput.playPressed)
            {
                // Real InitPlay: `SetPhase(Play, 1)` -- always
                // (re-)enters mission 1, the global hub (2026-07-17,
                // now that `LoadMission()` exists to make this a real
                // world load instead of just restaying on whatever was
                // already loaded). Restores the selected gamer's own
                // checkpointed lives if that slot has previous progress
                // (this engine's own adaptation, since the real
                // per-gamer lastWorld/door state has no equivalent here
                // beyond lives) -- read AFTER LoadMission(1), which
                // would otherwise reset lives to the default 3.
                LoadMission(1);
                interaction_.SetLives(saveData_.GetHasProgress() ? saveData_.GetLives() : 3);
                SetPhase(GalaxyEggbert::Def::GamePhase::Play);
            }
            else if (initInput.setupPressed)
            {
                SetPhase(GalaxyEggbert::Def::GamePhase::MainSetup);
            }
            else if (initInput.editorPressed)
            {
                // Not a real mobile-eggbert transition (plan.md
                // EDITOR-107): opens the in-game 3D world editor's
                // browser for the currently selected gamer slot.
                worldEditor_.EnterBrowser(saveData_.GetSelectedGamer());
                worldEditor_.ShowSlots();
                SetPhase(GalaxyEggbert::Def::GamePhase::Editor, /*bypassFade=*/true);
            }
            else if (phaseKeys.IsKeyDown(Keys::Escape) && !pauseKeyWasDown_)
            {
                // Real hardware Back-button behavior from Init is
                // Exit() (`Game1.cpp`'s real gamepad-Back handler).
                // Research separately found that the real source's
                // OWN Escape key unconditionally maps to Pause
                // regardless of phase, including from Init -- flagged
                // by that research as a likely-UNINTENDED quirk of the
                // real source (Escape apparently meant only for
                // in-Play pause) rather than deliberate menu-screen
                // design. NOT replicated here: this engine's own
                // Escape binding reuses the real hardware-Back->Exit()
                // behavior instead, which reads as the clearly
                // intentional one.
                Exit();
            }
        }
        else if (phase_ == GalaxyEggbert::Def::GamePhase::Resume)
        {
            // Real Resume (2026-07-13, plan.md MENU-040..045) -- see
            // InputPad::UpdateResume()'s class comment for the
            // menu layout; this is just the two real
            // buttons' behavior once already in the phase.
            const auto mouse = CurrentMouseState();
            const auto resumeInput = inputPad_.UpdateResume(
                mouse, viewport.getWidthProperty(), viewport.getHeightProperty());
            if (resumeInput.continuePressed || menuConfirmPressed)
            {
                // Game1.cpp:255-270, Decor.cpp:11146-11300: continue the saved moment.
                if (pendingResume_)
                {
                    auto log = std::move(*pendingResume_);
                    pendingResume_.reset();
                    if (BeginResume(std::move(log))) return true;
                }
                LoadMission(saveData_.GetMissionNumber());
                interaction_.SetLives(saveData_.GetLives());
                SetPhase(GalaxyEggbert::Def::GamePhase::Play, /*bypassFade=*/true);
            }
            else if (resumeInput.menuPressed ||
                     (phaseKeys.IsKeyDown(Keys::Escape) && !pauseKeyWasDown_))
            {
                // Real ResumeMenu/Back -> Init -- now wired
                // (2026-07-13, now that Init exists; was previously
                // adapted to an instant restart-without-lives-restore
                // workaround). This DOES defer/animate (the real
                // shrink+reverse-spin fade, plan.md MENU-088/089),
                // since Resume IS one of the 5 real deferring phases
                // and this path doesn't pass bypassFade.
                SetPhase(GalaxyEggbert::Def::GamePhase::Init);
            }
        }
        else if (phase_ == GalaxyEggbert::Def::GamePhase::Editor)
        {
            // In-game 3D world editor (plan.md section 6, EDITOR-107) --
            // see WorldEditor's own class comment. IsBrowsing()
            // distinguishes the per-gamer-slot world browser (no World
            // loaded yet) from actively editing one.
            const auto mouse = phaseMouse;
            if (worldEditor_.IsBrowsing())
            {
                // The galaxy's name being typed takes the keys (Escape stays the screen's back).
                for (const auto& [typed, shift] : browserTypedKeys_)
                    if (worldEditor_.WantsText()) worldEditor_.TypeKey(typed, shift);
                browserTypedKeys_.clear();
                const auto request = worldEditor_.UpdateBrowsing(
                    mouse, viewport.getWidthProperty(), viewport.getHeightProperty(),
                    phaseKeys.IsKeyDown(Keys::Escape));
                if (request.galaxyImported)
                {
                    // Another galaxy in place of the player's: its doors and checkpoint start over.
                    saveData_.ResetGalaxyProgress(saveData_.GetSelectedGamer());
                    SavePlayerData();
                }
                if (request.shouldReturnToMenu)
                {
                    SetPhase(GalaxyEggbert::Def::GamePhase::Init, /*bypassFade=*/true);
                }
                else if (request.shouldOpen)
                {
                    LoadCustomWorldForEditing(request.openPath);
                }
                else if (request.shouldCreateNew && !request.createPath.empty())
                {
                    // A new world in the empty slot the player clicked -- or the galaxy's place.
                    std::error_code ec;
                    std::filesystem::create_directories(request.createPath.parent_path(), ec);
                    (request.galaxyMission > 0 ? GalaxyEggbert::Editor::CreateGalaxyStarterWorld(request.galaxyMission)
                                               : GalaxyEggbert::Editor::CreateEditorStarterWorld())
                        .saveToFile(request.createPath);
                    LoadCustomWorldForEditing(request.createPath);
                }
            }
            else
            {
                worldEditor_.Update(phaseKeys, mouse, dt,
                                     viewport.getWidthProperty(), viewport.getHeightProperty(), camera_,
                                     worldRuntime_.GetWorldMutable(), editorNavigation);
                cameraTarget_ = worldEditor_.CameraTarget();
                // blupi_recon decblock.cpp MoveStep calls MoveObjectStep even in BUILD.
                // Preview lift patrols too; authored endpoints and numbered markers stay fixed.
                worldRuntime_.Update(dt);
                if (terrainRenderer_)
                {
                    if (!replayingResume_)
            terrainRenderer_->Update(getGraphicsDeviceProperty(), worldRuntime_.GetAnimPhase(), BlupiFocusRaw());
                }
                if (worldEditor_.ConsumeNeedsPresentationRebuild())
                {
                    RebuildWorldPresentation();
                }
                InteractionSystem::AdvanceEditorPlatforms(worldRuntime_, dt);
                if (worldEditor_.ConsumePlayTestRequested())
                {
                    // Update()'s own PlayTest handling already saved
                    // the world to GetWorldPath() before requesting
                    // this (plan.md EDITOR-108).
                    LoadCustomWorldForPlayTest(worldEditor_.GetWorldPath());
                }
                else if (worldEditor_.ConsumeLeaveRequested() && editorMission_ != 0)
                {
                    // Stop of an xnjttjpo visit (BD-0118): back to the mission.
                    LeaveMissionEditor();
                }
            }
        }

        // Real Pause trigger is gamepad-Back/a touch PlayPause button
        // (see phase_'s own class-comment) -- Escape is this engine's
        // own keyboard binding, OR'd with the on-screen PlayPause
        // button click (edge-triggered already, see UpdatePlay()).
        const bool pausePressed = phaseKeys.IsKeyDown(Keys::Escape) || mousePausePressed;
        if (pausePressed && !pauseKeyWasDown_)
        {
            if (phase_ == GalaxyEggbert::Def::GamePhase::Play)
            {
                SetPhase(GalaxyEggbert::Def::GamePhase::Pause);
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::Pause)
            {
                if (demoSlotsShown_)
                    demoSlotsShown_ = false; // Escape closes the demo positions first
                else
                    SetPhase(GalaxyEggbert::Def::GamePhase::Play);
            }
        }
        pauseKeyWasDown_ = pausePressed;

        if (mouseContinuePressed ||
            (phase_ == GalaxyEggbert::Def::GamePhase::Pause && menuConfirmPressed && !demoSlotsShown_))
        {
            // Real PauseContinue: resume in place.
            SetPhase(GalaxyEggbert::Def::GamePhase::Play);
        }
        else if (mouseRestartPressed)
        {
            if (editorPlayTestActive_)
            {
                // Play-test session (plan.md EDITOR-108): reload the
                // SAME custom world fresh, same intent as real
                // PauseRestart below -- worldRuntime_.GetMissionNumber()
                // is meaningless here (a sandbox world is never a real
                // mission), so LoadMission() would be the wrong call.
                LoadCustomWorldForPlayTest(editorPlayTestWorldPath_);
            }
            else
            {
                // Real PauseRestart (plan.md MENU-036): reload the CURRENT
                // mission fresh -- upgraded 2026-07-17 from the previous
                // origin-respawn-in-place simplification, now that
                // LoadMission() exists to do a genuine level reload
                // (fresh vehicle/secret-power/key/dynamite/treasure state,
                // same as touching any other mission trigger).
                LoadMission(worldRuntime_.GetMissionNumber());
                SetPhase(GalaxyEggbert::Def::GamePhase::Play);
            }
        }
        else if (mouseSetupPressed)
        {
            // Real PauseSetup: SetPhase(PlaySetup).
            SetPhase(GalaxyEggbert::Def::GamePhase::PlaySetup);
        }
        else if (mouseMenuPressed)
        {
            if (editorPlayTestActive_)
            {
                // Play-test session (plan.md EDITOR-108): PauseMenu
                // returns to the editor on the SAME custom world,
                // instead of the real Init hub.
                LoadCustomWorldForEditing(editorPlayTestWorldPath_);
                editorPlayTestActive_ = false;
                SetPhase(GalaxyEggbert::Def::GamePhase::Editor, /*bypassFade=*/true);
            }
            else
            {
                // Real PauseMenu: SetPhase(Init) -- now wired (2026-07-13,
                // now that Init exists).
                SetPhase(GalaxyEggbert::Def::GamePhase::Init);
            }
        }
        else if (mouseBackPressed)
        {
            if (editorPlayTestActive_)
            {
                // Play-test session (plan.md EDITOR-108): PauseBack
                // returns to the editor too, same as PauseMenu above.
                LoadCustomWorldForEditing(editorPlayTestWorldPath_);
                editorPlayTestActive_ = false;
                SetPhase(GalaxyEggbert::Def::GamePhase::Editor, /*bypassFade=*/true);
            }
            else
            {
                // Real PauseBack/MissionBack (plan.md MENU-035): wired
                // 2026-07-17 now that the hub/mission-progression system
                // gives it a real destination -- the same
                // ComputeMissionBack() formula shared by exit-reached
                // (WinLostReturn below): a hub (mission%10==0) goes to the
                // global hub (1), a sublevel goes to its own world's hub.
                LoadMission(WorldRuntime::ComputeMissionBack(worldRuntime_.GetMissionNumber()));
                SetPhase(GalaxyEggbert::Def::GamePhase::Play);
            }
        }

        if (phase_ == GalaxyEggbert::Def::GamePhase::Win || phase_ == GalaxyEggbert::Def::GamePhase::Lost)
        {
            // Real WinLostReturn button (icon 3), OR'd with this
            // engine's own Space-key pick -- edge-triggered already
            // (see UpdateWinLost()), so no separate debounce needed
            // for the mouse path.
            const auto mouse = CurrentMouseState();
            const bool mouseReturnPressed = inputPad_.UpdateWinLost(
                mouse, viewport.getWidthProperty(), viewport.getHeightProperty());
            const bool returnPressed = phaseKeys.IsKeyDown(Keys::Space) || confirmDown || mouseReturnPressed;
            if (returnPressed && !phaseReturnKeyWasDown_)
            {
                if (editorPlayTestActive_)
                {
                    // Play-test session (plan.md EDITOR-108):
                    // WinLostReturn goes back to the editor on the SAME
                    // custom world either way (Win or Lost), instead of
                    // the real hub/win-exit-formula destination below.
                    LoadCustomWorldForEditing(editorPlayTestWorldPath_);
                    editorPlayTestActive_ = false;
                    SetPhase(GalaxyEggbert::Def::GamePhase::Editor, /*bypassFade=*/true);
                }
                else
                {
                    if (phase_ == GalaxyEggbert::Def::GamePhase::Win)
                    {
                        // Real WinLostReturn from Win (plan.md TILE-006/
                        // SCORE-013..019): upgraded 2026-07-17 from the
                        // previous origin-respawn simplification -- reaching
                        // the exit now actually advances via the real
                        // win-exit formula (`ComputeWinExitTarget()`, NOT
                        // the plain `ComputeMissionBack()` PauseBack above
                        // uses -- this one additionally special-cases
                        // mission 1's own exit -> 199 and 199's own exit,
                        // real destination is actually Init, not straight
                        // back into Play -- this engine's own already-
                        // established "skip Init, return to Play directly"
                        // simplification is kept as-is, only WHICH mission
                        // loads is now real). Also unlocks the NEXT
                        // sublevel's door in this world's own hub (real
                        // `Decor::OpenDoorsWin()`, `m_doors[mission+1]=1`)
                        // -- only for a genuine sublevel (mission%10!=0);
                        // real source's `else` branch (hub/global-hub wins)
                        // uses `OpenGoldsWin()` instead, a different,
                        // cosmetic-only flag this engine doesn't model.
                        AdvanceAfterWin();
                    }
                    else
                    {
                        // Real WinLostReturn from Lost: verified directly
                        // against Decor.cpp:6395-6397/11716 (DoorsLost()) --
                        // Lost does NOT reload the level or reset treasure/
                        // key/dynamite progress, only lives (already reset
                        // to 3 the instant GameOverCount() incremented, see
                        // HUD-023). Repositioning to spawn (unchanged from
                        // before) is the correct, already-accepted behavior
                        // here -- do NOT call LoadMission() on this path,
                        // it would wrongly wipe the current mission's
                        // in-progress treasure/key state.
                        if (worldRuntime_.HasExplicitSpawnPoint())
                        {
                            PlaceBlupi(worldRuntime_.GetSpawnRenderX(), worldRuntime_.GetSpawnRenderY(),
                                       worldRuntime_.GetSpawnRenderZ());
                        }
                        else
                        {
                            PlaceBlupi(0.0f, 1.0f, 0.0f);
                        }
                        cameraSmoothedInitialized_ = false;
                    }
                    SetPhase(GalaxyEggbert::Def::GamePhase::Play);
                }
            }
            phaseReturnKeyWasDown_ = returnPressed;
        }
        return false;
    }

    // UpdateTick's world, in play with a terrain: Blupi's input and step, his actions, the
    // interactions, the vehicles, the camera, the sounds. True when the tick ends here.
    bool GalaxyEggbertGame::UpdateTickWorld(float dt, float simDt, const InputPad::PlayInput& padPlayInput,
                                            bool inputPadClaimedMouse)
    {
        if (!replayingResume_)
            terrainRenderer_->Update(getGraphicsDeviceProperty(), worldRuntime_.GetAnimPhase(), BlupiFocusRaw());

        // Tank controls (2026-07-05, matching mobile-eggbert's control
        // feel): Left/Right turn, Up/Down
        // move forward/back along the current facing — arrows are not
        // a strafe pad. LCtrl jumps and Space is reserved for "Action"
        // (matches mobile-eggbert's own InputPad.cpp key glyphs exactly:
        // LeftControl -> PlayJump, Space -> PlayAction) — Space is read
        // here but not wired to anything yet, since there is no
        // interactive-object system in GalaxyEggbertCNA yet. LShift
        // crouches and RShift looks up; RCtrl backs up (2026-11-02).
        using Microsoft::Xna::Framework::Input::Keys;
        const auto keys = inputPad_.FilterGameplayKeyboard(CurrentKeyboardState());
        const auto directions = GalaxyEggbert::Game::ReadPlayDirections(keys);
        float turnInput = directions.turn;
        float moveInput = directions.move;
        // On-screen D-pad OR'd in (2026-07-13, plan.md MENU-021..024):
        // both axes are already the same discrete {-1,0,+1} shape as
        // the keyboard reads above, so a plain add+clamp combines them
        // without needing a separate "which source wins" rule.
        turnInput = std::clamp(turnInput + padPlayInput.turnInput, -1.0f, 1.0f);
        moveInput = std::clamp(moveInput + padPlayInput.moveInput, -1.0f, 1.0f);
        turnInput = std::clamp(turnInput + AccelTurnInput(), -1.0f, 1.0f);
        bool jumpPressed = keys.IsKeyDown(Keys::LeftControl) || padPlayInput.jumpHeld;
        // INFRA-002's behavioral-trace scripted input (plan.md §7) --
        // walk forward down the tested corridor for the whole capture,
        // with one jump partway through, so real movement/collision/
        // gravity code is actually exercised, not just watched at rest
        // (see EnableGoldenTraceMode()'s own comment for why this is a
        // separate mode from the passive screenshot capture).
        if (goldenTraceMode_)
        {
            moveInput = 1.0f;
            jumpPressed = goldenTraceTick_ >= 40 && goldenTraceTick_ < 46;
        }
        const bool crouchHeld = keys.IsKeyDown(Keys::LeftShift) || padPlayInput.crouchHeld;
        // Holding a crate (crouching, one right in front) is the one time he backs up: Down
        // pulls it (the original's Pop) instead of turning him round.
        const bool interactsWithWorld = blupi_.CanInteractWithWorld();
        const bool pullingStance =
            interactsWithWorld && crouchHeld && (holdingCrate_ || InteractionSystem::CrateInFront(
                                                worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(),
                                                std::sin(blupi_.GetYaw()), -std::cos(blupi_.GetYaw())) != nullptr);
        // Down turns Blupi around instead of backing him up (user decision
        // 2026-10-24): the original has no backing up; reversing direction
        // plays its Turn (ACTION_TURN, 6 ticks at 20 Hz = 0.3 s, or the
        // vehicle's own, TurnAroundSeconds) and then walks the new way. He
        // stands while turning; with Down still held afterwards he walks on
        // in the new direction. One press, one turn.
        {
            constexpr float kPi = 3.14159265358979f;
            const bool downHeld = moveInput < 0.0f;
            const bool canTurn = !blupi_.IsTeleporting() && !blupi_.IsBye() && !blupi_.IsDeathLocked() &&
                                 !blupi_.IsPickupFrozen() && !blupi_.IsOneShotAnimPlaying() && !pullingStance;
            if (downHeld && !turnAroundDownWasHeld_ && turnAroundRemaining_ <= 0.0f && canTurn)
            {
                turnAroundDuration_ = TurnAroundSeconds(blupi_.GetAnimState());
                turnAroundRemaining_ = turnAroundDuration_;
                turnAroundStartYaw_ = blupi_.GetYaw();
            }
            turnAroundDownWasHeld_ = downHeld;
            if (turnAroundRemaining_ > 0.0f)
            {
                turnAroundRemaining_ = std::max(0.0f, turnAroundRemaining_ - simDt);
                blupi_.SetYaw(turnAroundStartYaw_ + kPi * (1.0f - turnAroundRemaining_ / turnAroundDuration_));
                turnInput = 0.0f;
                moveInput = 0.0f;
            }
            else if (downHeld && !pullingStance)
            {
                moveInput = -moveInput;
            }
            // Right Ctrl backs him up (owner 2026-11-02: "pravý ctrl bude couvání"), on foot or
            // in a vehicle, facing as he is: a Galaxy addition beside Down's turn-around (the
            // original has no backing up); a gamepad's RB and the on-screen back-up button too.
            if ((directions.backUp || padPlayInput.backUpHeld) && turnAroundRemaining_ <= 0.0f &&
                !pullingStance)
            {
                moveInput = -1.0f;
            }
        }
        // What the player asks of a crate (UpdateCratePush() below).
        const float crateMoveInput = moveInput;
        const auto& lastCratePush = interaction_.LastCratePush();
        const bool actionPressed = keys.IsKeyDown(Keys::Space) || padPlayInput.actionPressed;
        const bool lookUpHeld = keys.IsKeyDown(Keys::RightShift);
        const bool wasOnGround = blupi_.IsOnGround();
        const bool wasEcrased = blupi_.IsEcrased();
        const bool wasTeleporting = blupi_.IsTeleporting();
        const bool wasBye = blupi_.IsBye();
        // Compared at the very end of Update() (not right after Step(),
        // like wasEcrased above) since the balloon can also end via
        // interaction_.Update() -> PopBalloon() later this same frame,
        // not just Step()'s own natural-timeout countdown.
        const bool wasBallooned = blupi_.IsBallooned();
        const float blupiXBeforeStep = blupi_.GetX();
        // Vanishing/Temp tile (plan.md E3D-MIG-146) -- computed once
        // per frame from the same 20-ticks/sec animPhase_ every other
        // per-tile timing cycle here already uses (Blitz/Crusher), then
        // threaded into Step() itself (not a post-hoc GetGroundBlockType()
        // check like every other hazard) since this changes whether the
        // tile IS the ground at all, not just what Blupi is standing on.
        const bool tempPassable = WorldRuntime::IsTempPassableAtPhase(worldRuntime_.GetAnimPhase());
        // Water Surf/Nage detection (plan.md E3D-MIG-148) -- computed
        // from Blupi's PRE-step position (matching tempPassable's own
        // timing above), fed into Step() itself since Nage changes
        // gravity/jump behavior, not just a post-hoc hazard check.
        // GetBlockTypeAt() (the tile he currently occupies) and
        // GetBlockTypeAbove() (one cell above that) together reproduce
        // the real IsSurfWater ("water here, dry above") / IsDeepWater
        // ("water here AND above") distinction -- see
        // BlupiController::IsSurf()/IsNage()'s own comment.
        // Real gate (Decor.cpp:5284-5285, found 2026-07-16) also excludes every vehicle mode +
        // Balloon/Ecrase entirely -- while riding/ballooned/squashed, water tiles never
        // trigger Surf/Nage at all. This engine had no such exclusion; water detection ran
        // unconditionally. A vehicle that meets water is wrecked first, Blupi going into the water on foot
        // (Decor.cpp:5415-5435, 2026-10-26, reported by the world authoring: the helicopter
        // flew through water, the skateboard rode over it, against 013's "it dislikes
        // water!"): the vehicle is gone, an explosion, a small shake, channel 10. (The
        // helicopter's spinning ByeBye debris is not drawn.)
        if (interactsWithWorld && blupi_.IsInVehicle() && !blupi_.IsOneShotAnimPlaying() && !blupi_.IsTeleporting() &&
            !blupi_.IsDeathLocked() &&
            GalaxyEggbert::BlockTypes::isWater(blupi_.GetBlockTypeAt(worldRuntime_.GetWorld())))
        {
            WreckVehicle();
        }
        const bool canEnterWater = interactsWithWorld && !blupi_.IsInVehicle() && !blupi_.IsBallooned() && !blupi_.IsEcrased();
        const auto waterBlockAt = blupi_.GetBlockTypeAt(worldRuntime_.GetWorld());
        const auto waterBlockAbove = blupi_.GetBlockTypeAbove(worldRuntime_.GetWorld());
        const bool atWaterTile = canEnterWater && GalaxyEggbert::BlockTypes::isWater(waterBlockAt);
        const bool aboveIsWaterTile = GalaxyEggbert::BlockTypes::isWater(waterBlockAbove);
        const bool inSurfWater = atWaterTile && !aboveIsWaterTile;
        const bool inDeepWater = atWaterTile && aboveIsWaterTile;
        const bool wasSurf = blupi_.IsSurf();
        const bool wasNage = blupi_.IsNage();
        // Push animation state (plan.md BLUPI-036, found 2026-07-18):
        // `CrateBeingPushedThisFrame()` is only known AFTER
        // `interaction_.Update()` runs (later this same frame, see
        // `wasPushingCrate_`'s own edge-detection below), so this
        // deliberately reads LAST frame's own result -- a one-frame lag,
        // imperceptible for animation purposes and the same category of
        // simplification as this engine's existing camera-smoothing lag.
        // Crates are solid for Blupi (SetCrateCells()), and he holds one to pull it while he
        // crouches facing it (Decor.cpp's StopPop/Pop): taken when it is right in front of
        // him, kept while he backs away with it, let go when he stops crouching.
        {
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
            const bool canHold = interactsWithWorld && crouchHeld && blupi_.IsOnGround() && !blupi_.IsInVehicle() &&
                                 !blupi_.IsBallooned() && !blupi_.IsEcrased();
            const auto* crate = canHold ? InteractionSystem::CrateInFront(
                                              worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(),
                                              std::sin(blupi_.GetYaw()), -std::cos(blupi_.GetYaw()))
                                        : nullptr;
            const float crateGap = crate ? std::hypot(crate->currentX - blupi_.GetX(), crate->currentZ - blupi_.GetZ())
                                         : 0.0f;
            holdingCrate_ = crate != nullptr && (holdingCrate_ || crateGap < 1.1f);
            // The crate he pushes or holds moves him (FollowCrate() after UpdateCratePush());
            // holding one he stands still (StopPop) until it follows him.
            // A jump takes him off the crate at his own speed (the original's Jump follows Push
            // at the march speed): onto the crate he has pushed, say.
            blupi_.SetCrateDriven((lastCratePush.pushing && !jumpPressed) || holdingCrate_);
            if (holdingCrate_ && !lastCratePush.pulling)
            {
                moveInput = 0.0f;
            }
        }
        blupi_.SetTurning(turnAroundRemaining_ > 0.0f && !blupi_.IsInVehicle());
        blupi_.SetRidingLift(interaction_.IsRidingLift());
        blupi_.SetLiftUnder(interaction_.RideLiftX(), interaction_.RideLiftZ(), interaction_.RideLiftIsConveyor());
        blupi_.SetLiftBelow(blupi_.GetVehicleMode() == BlupiController::VehicleMode::Overcraft &&
                            InteractionSystem::LiftWithinOverHeight(worldRuntime_, blupi_.GetX(), blupi_.GetY(),
                                                                     blupi_.GetZ()));
        // Steering (Left/Right, a Galaxy control: the original's Blupi only faces left or right)
        // turns him at the player's pace whatever the game speed (owner 2026-11-04: at F6-F8 he
        // spun far too fast). Step turns by turnInput * kTurnSpeed * its sped-up dt, so the rate
        // is scaled back. Down's turn-around stays the original's Turn action, in game ticks.
        const float steering = turnInput / GameSpeedFactor(gameSpeed_);
        sound_.SetBlupiHidden(blupi_.IsHidden());
        const float stepStartFeetY = blupi_.GetY();
        const float stepStartX = blupi_.GetX(), stepStartZ = blupi_.GetZ();
        blupi_.SetTerrainAnimationPhase(worldRuntime_.GetAnimPhase());
        blupi_.Step(worldRuntime_.GetWorld(), steering, moveInput, jumpPressed,
                    crouchHeld, lookUpHeld, simDt, tempPassable, inSurfWater, inDeepWater,
                    wasPushingCrate_, holdingCrate_);
        debugVelocity_ = {(blupi_.GetX() - stepStartX) / simDt,
                          (blupi_.GetY() - stepStartFeetY) / simDt,
                          (blupi_.GetZ() - stepStartZ) / simDt};
        UpdateBodyTilt(simDt);
        if (interactsWithWorld) TickPendingSkate(simDt);
        // Automatic zoom on action (Decor::MoveHotSpot, Decor.cpp:538-600; the Setup's
        // SetupZoom, on by default; OR-0010): while he walks with his focus, not flying the
        // helicopter nor under Power, the view eases in to 1.3x (1/30 a tick); it stays 30
        // ticks after he stops unless he loses his focus or crouches or looks up.
        {
            const bool focus = !blupi_.IsOneShotAnimPlaying() && !blupi_.IsDeathLocked() &&
                               !blupi_.IsTeleporting() && !blupi_.IsBye() && !blupi_.IsPickupFrozen();
            bool zoomIn = saveData_.GetAutoZoom() && focus && (moveInput != 0.0f || turnInput != 0.0f) &&
                          blupi_.GetVehicleMode() != BlupiController::VehicleMode::Helicopter &&
                          blupi_.GetSecretPower() != GalaxyEggbert::Def::SecretPower::Power;
            if (zoomIn)
            {
                chaseZoomLingerSeconds_ = 30.0f / 20.0f;
            }
            else if (chaseZoomLingerSeconds_ > 0.0f)
            {
                chaseZoomLingerSeconds_ =
                    (!focus || crouchHeld || lookUpHeld) ? 0.0f : chaseZoomLingerSeconds_ - simDt;
                zoomIn = chaseZoomLingerSeconds_ > 0.0f;
            }
            const float target = zoomIn ? 1.3f : 1.0f;
            const float step = simDt * 20.0f / 30.0f;
            chaseZoom_ = chaseZoom_ < target ? std::min(chaseZoom_ + step, target) : std::max(chaseZoom_ - step, target);
        }
        // The helicopter flown into a ceiling is wrecked (Decor.cpp:5264-5283, OR-0019), unless
        // the shield, the drink's Hide or SuperBlupi protect it.
        if (interactsWithWorld && blupi_.HelicopterHitsCeiling(worldRuntime_.GetWorld(), tempPassable) && !blupi_.IsDeathLocked() &&
            !blupi_.IsTeleporting() && !blupi_.IsOneShotAnimPlaying() /* m_blupiFocus */ && blupi_.GetSecretPower() != GalaxyEggbert::Def::SecretPower::Shield &&
            blupi_.GetSecretPower() != GalaxyEggbert::Def::SecretPower::Hide && !blupi_.GetCheatSuperBlupi())
        {
            WreckVehicle();
        }

        // INFRA-002's behavioral-trace capture (plan.md §7) -- one line
        // per tick, POST-step so it reflects this frame's real result.
        // Fixed tick count (kGoldenTraceTicks) chosen to comfortably
        // cover the whole walk-jump-land-settle script above with a
        // margin, same reasoning as kGoldenCaptureTicks' own comment.
        if (goldenTraceMode_ && !goldenTraceWritten_)
        {
            constexpr int kGoldenTraceTicks = 150;
            char line[160];
            std::snprintf(line, sizeof(line),
                           "tick=%d x=%.6f y=%.6f z=%.6f velY=%.6f onGround=%d animState=%d animIcon=%d\n",
                           goldenTraceTick_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(), blupi_.GetVelocityY(),
                           blupi_.IsOnGround() ? 1 : 0, static_cast<int>(blupi_.GetAnimState()),
                           blupi_.GetAnimIcon());
            goldenTraceLines_ += line;
            ++goldenTraceTick_;
            if (goldenTraceTick_ >= kGoldenTraceTicks)
            {
                goldenTraceWritten_ = true;
                std::FILE* f = std::fopen("golden_trace.txt", "wb");
                if (f)
                {
                    std::fwrite(goldenTraceLines_.data(), 1, goldenTraceLines_.size(), f);
                    std::fclose(f);
                    std::cout << "GalaxyEggbertCNA: wrote golden_trace.txt (" << goldenTraceTick_ << " ticks)."
                              << std::endl;
                }
                else
                {
                    std::cout << "GalaxyEggbertCNA: FAILED to write golden_trace.txt." << std::endl;
                }
                Exit();
            }
        }

        // Real mobile-eggbert jump/land/footstep sounds (2026-07-10).
        // jumpPressed is edge-detected the same way "C" is below, gated
        // on wasOnGround so holding the key while airborne doesn't
        // replay the jump sound.
        if (jumpPressed && !jumpKeyWasDown_ && wasOnGround)
        {
            sound_.PlayJump();
        }
        const auto groundIcon = blupi_.GetGroundBlockType(worldRuntime_.GetWorld());
        // Real mobile-eggbert also suppresses this generic landing-thud
        // sound specifically when the landing spot is a spring
        // (Decor.cpp ~2984, `if (!IsRessort(end))`) -- the bounce sound
        // below covers it instead, since Blupi is about to launch back
        // upward immediately rather than coming to rest.
        if (!wasOnGround && blupi_.IsOnGround() && groundIcon != GalaxyEggbert::BlockTypes::Spring)
        {
            sound_.PlayLand(groundIcon);
        }
        // No footsteps while he walks: the original plays channel 3 (and its material remaps)
        // only as he lands, on the ground, a lift or in a vehicle (Decor.cpp:2986, 3018, 4204,
        // 4220, 4365, 4392); a step sound every 0.3 s was Galaxy's own (BD-0037).

        // Real crouch/look-up transition sounds (SOUND-017/030/031,
        // added 2026-07-20, Decor.cpp:3278-3287/3628-3633) -- ch7/ch21
        // fire once, 0.2s after entering Down/Up; ch20 fires once on
        // the Down->Stop release transition. See
        // BlupiController::UpdateAnim()'s own comment for why the
        // real speedX==0/speedY==0 gate needs no separate modeling
        // here.
        UpdateMockerySound(dt);
        {
            const bool inHelicopter = blupi_.GetVehicleMode() == BlupiController::VehicleMode::Helicopter;
            if (wasInHelicopter_ && !inHelicopter && !helicopterDeposited_)
            {
                interaction_.ByeByeHelicopter(blupi_.GetX(), blupi_.GetY(), blupi_.GetZ());
            }
            wasInHelicopter_ = inHelicopter;
            helicopterDeposited_ = false;
        }
        // Hide's trail ages 20 ticks a second (ObjectType58's 20 ticks, Decor.cpp:8391).
        for (auto& ghost : hideGhosts_)
        {
            ghost.ageTicks += dt * 20.0f;
        }
        hideGhosts_.erase(std::remove_if(hideGhosts_.begin(), hideGhosts_.end(),
                                         [](const HideGhost& ghost) { return ghost.ageTicks >= 20.0f; }),
                          hideGhosts_.end());
        for (const auto& blupiEvent : blupi_.EventsThisFrame())
        {
            switch (blupiEvent.kind)
            {
                case BlupiController::EventKind::DownEntrySoundFired:
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel7);
                    break;
                case BlupiController::EventKind::UpEntrySoundFired:
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel21);
                    break;
                case BlupiController::EventKind::DownReleaseSoundFired:
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel20);
                    break;
                case BlupiController::EventKind::VertigoStarted:
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel6);
                    break;
                case BlupiController::EventKind::HeadBumped:
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel40);
                    break;
                case BlupiController::EventKind::CeilingTapped:
                    sound_.Play(GalaxyEggbert::Game::Sound::CeilingChannelFor(blupi_.GetCeilingIcon()));
                    break;
                case BlupiController::EventKind::TurnStarted:
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel5);
                    break;
                case BlupiController::EventKind::IdleFidget:
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel37);
                    break;
                case BlupiController::EventKind::BarGrabbed:
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel34);
                    break;
                case BlupiController::EventKind::BarJumpedOff:
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel35);
                    break;
                case BlupiController::EventKind::BarIdleFidget:
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel36);
                    break;
                case BlupiController::EventKind::CrateGripped:
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel39);
                    break;
                case BlupiController::EventKind::ReactionStarted:
                    // Decor.cpp:6258/6275 (ch46, Ouf1b's at its 4th tick), 6291 (ch48), 6301 (ch49).
                    switch (blupi_.GetAnimState())
                    {
                        case BlupiController::AnimState::Ouf1a:
                        case BlupiController::AnimState::Ouf1b:
                            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel46);
                            break;
                        case BlupiController::AnimState::Ouf3:
                            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel48);
                            break;
                        case BlupiController::AnimState::Ouf4:
                            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel49);
                            break;
                        case BlupiController::AnimState::Ouf5: // Decor.cpp:3134-3139 (ch47, its 4th tick)
                            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel47);
                            break;
                        default:
                            break;
                    }
                    break;
            }
        }
        jumpKeyWasDown_ = jumpPressed;

        // Crusher squash recovery sound (real channel 41, per
        // Decor.cpp:5180-5197 -- the same "buff expired" channel other
        // timed states reuse on their own recovery, not a dedicated
        // crusher-only sound). Entry sound (channel 70) is played at the
        // trigger site below instead, since only that call site knows
        // whether this is a genuinely new trigger (TriggerCrush()
        // returns false, a no-op, if already squashed).
        if (wasEcrased && !blupi_.IsEcrased())
        {
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel41);
            // He springs up out of it: four sparks and a flash (Decor.cpp:5185-5196; BD-0054).
            interaction_.SpawnInvertBurst(worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(), true);
            interaction_.SpawnEffect(worldRuntime_, GalaxyEggbert::Def::ObjectType::ObjectType90, blupi_.GetX(), blupi_.GetY(),
                                     blupi_.GetZ());
        }

        // Teleporter transit completion (plan.md E3D-MIG-147, real
        // Decor.cpp:6349-6358) -- fires the one frame the kTeleportDuration
        // countdown naturally elapses inside Step() (blupi_ was fully
        // frozen for the whole transit, so this is the only way it ends,
        // unlike balloon's PopBalloon() interrupt path). Looks up the
        // paired destination via the same icon TriggerTeleport() was
        // called with; relocates Blupi there with the original's two
        // sparkle clusters (ObjectType27) and no sound: channel 71 plays
        // only on entering (Decor.cpp:5603, 6350-6361; BD-0017).
        // If no paired cell exists anywhere in the grid, real behavior
        // is "regains control in place" -- a silent no-op, which is
        // already true here since blupi_'s position was never touched.
        if (wasTeleporting && !blupi_.IsTeleporting())
        {
            float destX, destY, destZ;
            if (worldRuntime_.FindTeleportDestination(blupi_.GetTeleportIcon(), blupi_.GetX(), blupi_.GetY(),
                                                        blupi_.GetZ(), destX, destY, destZ))
            {
                blupi_.SetPosition(destX, destY, destZ);
                // Under the partner pillar, facing as he did (the original keeps m_blupiDir;
                // BD-0017). The arrival spot is m_blupiPosHelico (Decor.cpp:6360): no teleporter
                // takes him again, nor does a hazard touch him, until he has left it. The
                // camera goes behind him at once rather than easing over from the departure,
                // maybe far away (2026-10-26, reported by the world authoring).
                interaction_.StartContactGrace(destX, destY, destZ);
                teleportArrivalGuard_ = true;
                teleportArrivalCellX_ = static_cast<int>(std::lround(destX));
                teleportArrivalCellZ_ = static_cast<int>(std::lround(destZ));
                interaction_.SpawnTeleportArrival(worldRuntime_, destX, destY, destZ);
                chaseCamera_.Reset(blupi_.GetYaw());
                cameraSmoothedInitialized_ = false;
            }
        }

        // "Bye" farewell completion (plan.md BLUPI-049) -- fires the one
        // frame the real 1.5s freeze naturally elapses inside Step(),
        // same before/after-Step() detection shape as the teleport
        // completion just above. The actual world-select portal contact
        // below is what calls TriggerBye() and remembers byePendingTarget_;
        // this is just where the deferred LoadMission() finally happens.
        if (wasBye && !blupi_.IsBye())
        {
            // A galaxy world's play-test goes on in that galaxy (BD-0119), not into the campaign.
            if (editorPlayTestActive_)
            {
                if (const auto galaxy =
                        GalaxyEggbert::Editor::ParseGalaxyWorldPath(editorPlayTestWorldPath_.generic_string()))
                {
                    const auto next = GalaxyEggbert::Editor::GalaxyWorldPath(galaxy->first, byePendingTarget_);
                    std::error_code error;
                    if (std::filesystem::exists(next, error))
                        LoadCustomWorldForPlayTest(next);
                    else
                        std::cout << "GalaxyEggbertCNA: play-test: "
                                  << GalaxyEggbert::Editor::GalaxyMissionName(byePendingTarget_)
                                  << " of this galaxy is not made yet." << std::endl;
                    return true;
                }
            }
            LoadMission(byePendingTarget_);
            return true;
        }

        const bool actionPressedEdge = actionPressed && !actionKeyWasDown_;
        bool actionUsed = false;
        // Decor::BlupiStep, Decor.cpp:2737-2741: ghost returns before hazards, springs,
        // water, world signs, teleporters, fans, safe positions, switches and hand actions.
        if (interactsWithWorld && UpdateTickActions(actionPressedEdge, actionUsed, jumpPressed, stepStartFeetY, tempPassable, wasNage, wasSurf))
        {
            return true;
        }
        actionKeyWasDown_ = actionPressed;

        // Interactive objects (2026-07-10, see InteractionSystem.hpp)
        // -- platform lift patrol + riding (plan.md E3D-MIG-152, 2026-
        // 07-12), crate push, pickup collection, generic hazard contact
        // (ObjectType2/3/4/16/17/20/96/97), and (2026-07-11) the wasp's
        // balloon status. Runs after blupi_.Step() so blupi_'s position
        // is this frame's final value; blupiXBeforeStep lets the
        // interaction system infer movement direction for crate push
        // without BlupiController needing a velocity accessor.
        // crouchHeld gates ObjectType3's real duck-immunity;
        // blupi_.IsBallooned() gates whether a 3/16/96/97 hazard pops
        // the balloon instead of killing. blupiFacingDX/DZ (plan.md
        // E3D-MIG-160) is Blupi's facing rounded to the nearest
        // cardinal grid direction (GetYaw()'s own 0=-Z/forward=
        // (sin,0,-cos) convention), used for the door-detection probe
        // one cell ahead of him.
        const int blupiFacingDX = static_cast<int>(std::lround(std::sin(blupi_.GetYaw())));
        const int blupiFacingDZ = static_cast<int>(std::lround(-std::cos(blupi_.GetYaw())));
        // Secret powers (plan.md E3D-MIG-170) -- blupiCanGrantX mirrors
        // each TriggerX()'s own internal gate exactly (see
        // BlupiController::TriggerShield()/Power()/Cloud()/Hide()'s
        // own comments), computed here from GetSecretPower() since
        // InteractionSystem has no access to BlupiController.
        const auto secretPower = blupi_.GetSecretPower();
        const bool canGrantShield = secretPower != GalaxyEggbert::Def::SecretPower::Shield &&
                                    secretPower != GalaxyEggbert::Def::SecretPower::Hide &&
                                    secretPower != GalaxyEggbert::Def::SecretPower::Power;
        // Real Sucette(26)/Drink(30)/Charge(31) gates also exclude every
        // vehicle mount plus Balloon/Ecrase (Decor.cpp:6025-6087:
        // !m_blupiHelico/Over/Balloon/Ecrase/Jeep/Tank/Skate) -- Shield
        // and Invert have no such clause in real source (verified
        // directly, NEXT.md's listing of Shield here was wrong), so
        // canGrantShield/canGrantInvert deliberately don't get this.
        const bool blupiVehicleOrSquashed = blupi_.GetVehicleMode() != BlupiController::VehicleMode::None ||
                                             blupi_.IsBallooned() || blupi_.IsEcrased();
        const bool canGrantPower = secretPower != GalaxyEggbert::Def::SecretPower::Shield &&
                                    !blupiVehicleOrSquashed;
        const bool canGrantCloud = secretPower == GalaxyEggbert::Def::SecretPower::None &&
                                    !blupiVehicleOrSquashed;
        const bool canGrantHide = secretPower != GalaxyEggbert::Def::SecretPower::Shield &&
                                   secretPower != GalaxyEggbert::Def::SecretPower::Cloud &&
                                   !blupiVehicleOrSquashed;
        // Invert/Mirror (plan.md PICKUP-011) -- BlupiController::TriggerInvert()'s gate: not
        // while Hide; taken again while inverted (Decor.cpp:6040, BD-0025).
        const bool canGrantInvert = secretPower != GalaxyEggbert::Def::SecretPower::Hide;
        // Fire: F, tank-only physical Ctrl alias, gamepad RT or touch Fire.
        // Helicopter Ctrl is reserved for climbing (owner's latest control request).
        // Original reference: blupi_recon/src/event.cpp VK_SPACE/VK_SHIFT -> KEY_FIRE,
        // decblupi.cpp:901 helicopter and :1387 tank; Mobile Decor.cpp:3694/4317.
        // Tank shoots ahead; the helicopter drops downward only while airborne, off a lift.
        const bool firePressed = keys.IsKeyDown(Keys::F) || padPlayInput.fireHeld;
        const bool inHelicopter = blupi_.GetVehicleMode() == BlupiController::VehicleMode::Helicopter;
        const bool canFire = blupi_.GetVehicleMode() == BlupiController::VehicleMode::Tank ||
                             (inHelicopter && !blupi_.IsOnGround() && !interaction_.IsRidingLift());
        // Captured before Update() so the Lost transition below can
        // detect the exact frame GameOverCount() increments (plan.md
        // HUD-023's own real trigger, Decor.cpp:6374-6435).
        const int gameOverCountBeforeUpdate = interaction_.GameOverCount();
        // Real BlupiElectro aura (plan.md `068`) -- active whenever
        // Cloud is the current secret power (this engine's own
        // GalaxyEggbert::Def::SecretPower::Cloud, matching the real Power-Charge pickup).
        const bool cloudActive = secretPower == GalaxyEggbert::Def::SecretPower::Cloud;

        // Keep the real position: already-awake followers still home in Ghost mode
        // (Decor.cpp:8026), and dynamite's blast is outside BlupiStep (9167-9173).
        // Contact/detection guards are explicit inside InteractionSystem.
        const float interactionBlupiX = blupi_.GetX();
        const float interactionBlupiY = blupi_.GetY();
        const float interactionBlupiZ = blupi_.GetZ();
        // Crate-push gate (found 2026-07-16, Decor.cpp:6130-6132): real source also excludes
        // every vehicle mode + Ecrase (Balloon is covered separately by the existing
        // blupi_.IsBallooned() argument just below).
        // Off a vehicle, or arrived by a teleporter: the hazards' contact grace (the original's
        // m_blupiPosHelico, InteractionSystem::StartContactGrace).
        if ((graceWasInVehicle_ && !blupi_.IsInVehicle()) || (wasTeleporting && !blupi_.IsTeleporting()))
        {
            interaction_.StartContactGrace(blupi_.GetX(), blupi_.GetY(), blupi_.GetZ());
        }
        graceWasInVehicle_ = blupi_.IsInVehicle();
        const bool canPushCrate = !blupi_.IsInVehicle() && !blupi_.IsEcrased();
        // Pushing and pulling crates (CratePush.hpp): the crate moves at the original's speed,
        // rests wherever it stops, and holds Blupi against it at the original's distance.
        {
            InteractionSystem::CratePushInput crateInput;
            crateInput.blupiX = interactionBlupiX;
            crateInput.blupiY = interactionBlupiY;
            crateInput.blupiZ = interactionBlupiZ;
            crateInput.facingX = std::sin(blupi_.GetYaw());
            crateInput.facingZ = -std::cos(blupi_.GetYaw());
            crateInput.onGround = blupi_.IsOnGround();
            crateInput.canPush = canPushCrate && !blupi_.IsBallooned() && !blupi_.IsNage() && !blupi_.IsSurf() &&
                                 !blupi_.IsDeathLocked() && !blupi_.IsGhost();
            crateInput.walkingForward = crateMoveInput > 0.0f;
            crateInput.holding = holdingCrate_;
            crateInput.pullingBack = holdingCrate_ && crateMoveInput < 0.0f;
            crateInput.power = blupi_.GetSecretPower() == GalaxyEggbert::Def::SecretPower::Power;
            const auto& crateWorld = worldRuntime_.GetWorld();
            // Backing away he may walk off a ledge (and fall, which ends the pull), as the original.
            crateInput.blupiClearAt = [&crateWorld, tempPassable](float x, float y, float z)
            { return BlupiController::IsClearAt(crateWorld, x, y, z, tempPassable); };
            const auto crateOut = interaction_.UpdateCratePush(simDt, worldRuntime_, crateInput);
            if (interactsWithWorld && crateOut.holdBlupi && !blupi_.IsDeathLocked())
            {
                // Faster than any crate (Power's 10 px a tick = 3.1 cells/s): he keeps up exactly.
                constexpr float kFollowCellsPerSecond = 4.0f;
                blupi_.FollowCrate(crateWorld, crateOut.blupiX, crateOut.blupiZ, kFollowCellsPerSecond * simDt,
                                   tempPassable);
                // He squares up to the crate's face unless the player turns him.
                if (turnInput == 0.0f)
                {
                    constexpr float kPi = 3.14159265358979f;
                    const float targetYaw = std::atan2(crateOut.faceX, -crateOut.faceZ);
                    float delta = std::remainder(targetYaw - blupi_.GetYaw(), 2.0f * kPi);
                    const float maxTurn = BlupiController::kTurnSpeed * simDt;
                    delta = std::clamp(delta, -maxTurn, maxTurn);
                    blupi_.SetYaw(blupi_.GetYaw() + delta);
                }
            }
        }
        const float crateIntentDX = 0.0f;
        const float crateIntentDZ = 0.0f;
        // A hanging mine spares him while he is down (MoveObjectDetect skips it in Down), not
        // while the key is held as he walks on (OR-0041: a step stands him up).
        interaction_.Update(simDt, worldRuntime_, interactionBlupiX, interactionBlupiY, interactionBlupiZ,
                             crateIntentDX, sound_, blupi_.GetAnimState() == BlupiController::AnimState::Down,
                             blupi_.IsBallooned(), blupiFacingDX, blupiFacingDZ, blupi_.IsInvincible(),
                             canGrantShield, canGrantPower, canGrantCloud, canGrantHide,
                             firePressed, canFire, cloudActive, canGrantInvert, actionPressedEdge,
                             canPushCrate, crateIntentDZ, holdingCrate_,
                             blupi_.GetVehicleMode() == BlupiController::VehicleMode::Jeep ||
                                 blupi_.GetVehicleMode() == BlupiController::VehicleMode::Tank ||
                                 blupi_.GetVehicleMode() == BlupiController::VehicleMode::Skateboard,
                             inHelicopter, blupi_.IsHidden(),
                             // m_blupiFocus or in the air (Decor.cpp:5788-5790): not while he
                             // takes a pickup, teleports, waves goodbye or cheers on the ground.
                             BlupiFocused(), BlupiOutOfPlay(), interactsWithWorld, blupi_.GetYaw());
        RefreshTerrainMeshIfChanged();

        // The action button with nothing to do, standing still: he shakes his head (Non, 18
        // ticks, channel 27; Decor.cpp:6366-6372, 3188). Moving cuts it short.
        if (interactsWithWorld && actionPressedEdge && !actionUsed && !interaction_.ActionPressSpent() &&
            blupi_.GetAnimState() == BlupiController::AnimState::Stop)
        {
            blupi_.TriggerReaction(BlupiController::AnimState::Non, BlupiController::kNonDuration);
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel27);
        }

        // Voyage (plan.md `158`) -- a pickup touched above may have
        // recorded a this-frame voyage request (InteractionSystem has
        // no camera access, so it can't project the world<->HUD-space
        // endpoint itself). Resolve it here and start the real voyage.
        ResolvePendingVoyage();
        // Death-lock/life-loss Voyage follow-up -- see its own comment.
        ResolveDeathLock();
        // Sucette/Drink/Charge real 2-stage pickup delay -- see its own comment.
        ResolvePickupFreeze();

        // Pollution puff (plan.md VISUAL-013) -- called every non-ghost
        // frame, matching the real function's own internal gate
        // (see TickPollutionPuff()'s own comment). isMoving reuses the
        // same blupiXBeforeStep delta already computed above for the
        // interaction system's crate-push direction inference, instead
        // of adding a new horizontal-speed accessor to BlupiController.
        if (interactsWithWorld) // Decor.cpp:3888/4009/4293/4436, inside BlupiStep
        {
            const auto vehicleMode = blupi_.GetVehicleMode();
            interaction_.TickPollutionPuff(
                worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(),
                vehicleMode == BlupiController::VehicleMode::Helicopter,
                vehicleMode == BlupiController::VehicleMode::Overcraft,
                vehicleMode == BlupiController::VehicleMode::Jeep,
                vehicleMode == BlupiController::VehicleMode::Tank,
                blupi_.GetX() != blupiXBeforeStep, blupi_.GetVelocityY() > 0.0f, blupiFacingDX);
        }

        // Shield/Power magic trail (plan.md VISUAL-011-adjacent) --
        // skipped by Ghost; TickMagicTrail() further requires Shield or Power.
        if (interactsWithWorld) // Decor.cpp:5204-5254, after the ghost return
            interaction_.TickMagicTrail(worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(),
                                        secretPower == GalaxyEggbert::Def::SecretPower::Shield,
                                        secretPower == GalaxyEggbert::Def::SecretPower::Power);

        if (UpdateTickEvents(gameOverCountBeforeUpdate, wasBallooned))
        {
            return true;
        }

        if (UpdateTickCamera(dt, simDt, keys, inputPadClaimedMouse, crouchHeld, lookUpHeld, moveInput, turnInput))
        {
            return true;
        }

        return false;
    }


    // UpdateTickWorld's actions, while Blupi acts in the world: the action key and what it does (pick up,
    // push, enter, fire, the doors, the signs, the vehicles). True when the tick ends here.
    bool GalaxyEggbertGame::UpdateTickActions(bool actionPressedEdge, bool& actionUsed, bool& jumpPressed, float stepStartFeetY, bool tempPassable,
                                              bool wasNage, bool wasSurf)
    {
        // Shared death consequence (2026-07-11, plan.md E3D-MIG-067
        // subset; deferred-timing follow-up 2026-07-14) -- takes the
        // real death sound channel for the specific cause (channel 8:
        // fall-off-world/lava, the real shared "you died" sound per
        // 07-sounds.md; channel 51: spikes/drip's real Glu-death sound,
        // distinct from channel 8's) and starts the real death-lock
        // (`BlupiController::TriggerDeathLock()`, see its own
        // comment) instead of applying life loss/respawn instantly --
        // both now happen once the lock elapses, via
        // ConsumeDeathLockResolved() below. `cause` drives the real
        // per-type lock duration; `shouldRespawn` is the real
        // `m_blupiRestart` flag for this specific site (see each call
        // site's own comment for its citation).
        const auto triggerDeath = [this](GalaxyEggbert::Def::SoundChannel channel,
                                          GalaxyEggbert::Game::BlupiController::DeathCause cause,
                                          bool shouldRespawn, bool playChannel = true)
        {
            if (playChannel)
            {
                sound_.Play(channel);
            }
            blupi_.TriggerDeathLock(cause, shouldRespawn);
        };

        // Real Clear2/Clear3 "soul ascends" death VFX (plan.md `158`
        // death-VFX follow-up, `Decor::BlupiDead`'s own Clear2/Clear3
        // branches, Decor.cpp:6589-6606) -- called directly here (not
        // via the pending-request round-trip pickups use) since these
        // 2 real trigger sites (fall-off-world, Lava) already have
        // `camera_`/viewport in scope, unlike the generic-hazard-
        // contact coinflip site inside InteractionSystem::Update()
        // (see RequestClear2Ascend()'s own comment). kind must be
        // Clear2Ascend (icon 230, offsetY 300) or Clear3Ascend (icon
        // 40, offsetY 2000).
        // Takes an EXPLICIT death position (not read live from blupi_)
        // -- harmless now that triggerDeath() no longer moves Blupi
        // synchronously (2026-07-14, deferred-timing follow-up), but
        // callers still capture blupi_.GetX/Y/Z() BEFORE calling
        // triggerDeath() for clarity/consistency with every call site.
        const auto triggerDeathAscend = [this](GalaxyEggbert::Game::InteractionSystem::VoyageKind kind,
                                                float offsetY, int icon, float deathX, float deathY, float deathZ)
        {
            auto viewport = getGraphicsDeviceProperty().getViewportProperty();
            viewport.setWidthProperty(tickInput_.viewportWidth);
            viewport.setHeightProperty(tickInput_.viewportHeight);
            float hudX = 0.0f, hudY = 0.0f;
            if (Hud::ProjectWorldToHudSpace(Microsoft::Xna::Framework::Vector3(deathX, deathY, deathZ),
                                               camera_.View(), camera_.Projection(),
                                               viewport.getWidthProperty(), viewport.getHeightProperty(), hudX,
                                               hudY))
            {
                interaction_.BeginVoyage(worldRuntime_, kind, icon, false, hudX, hudY, hudX, hudY - offsetY,
                                          sound_, deathX, deathY, deathZ);
            }
        };

        // Fall-off-world death -- mobile-eggbert-reference/
        // 10-blupi-mechanics.md §8: "Walking off the world's bottom
        // row ... -> Clear2", checked before the rest of the frame
        // runs. kFallDeathY is a simplification of the real grid-row
        // check (`(end.Y+30)/64 >= 99`, Decor.cpp:2754) -- this world's
        // real floors/hazards all sit at Y>=0 (the sample world's
        // water/pit hazard is only 1 block deep), so any Y clearly
        // below that means Blupi fell through a hole with nothing
        // under it, not a legitimate low point in the level.
        //
        // **-60.0f, not -5.0f (user-reported, 2026-07-11): the real
        // fall is NOT near-instant.** Directly inspected
        // `../mobile-eggbert/worlds/world001.txt`: its real terrain
        // (`Decor:` grid) occupies only rows 0-21, leaving rows 22-98
        // (77 rows, ~4928 real px) of completely empty grid before the
        // real row-99 death check ever fires -- a deliberate, generous
        // "pit of doom" margin, not a tight threshold. Verified real
        // gravity directly against Decor.cpp:2966-2968 (`end.Y +=
        // (int)(m_blupiVitesseY * 2.0); if (m_blupiVitesseY < 20.0)
        // m_blupiVitesseY += 2.0;`, starting at 1.0): terminal velocity
        // is actually 21, not literally 20 (the uncapped +2 increment
        // overshoots the `<20` gate by one step) -- 840 real px/sec
        // (13.125 tiles/sec) at the real 20Hz reference rate. Falling
        // that observed ~4928px margin at these real constants takes
        // ~6.1s (the real ramp-up to terminal covers the first ~200px
        // in 0.5s, the remaining ~4728px at 840px/s takes ~5.6s) --
        // several seconds, matching the user's recollection of a real,
        // noticeable fall, not the sub-1-second death the old -5.0f
        // threshold gave with this engine's own gravity. -60.0f
        // reproduced a comparable ~6.2s fall (verified live at 6.72s)
        // using this engine's own already-tuned kGravity=25/
        // kFallLimit=-10 (not the real tick-domain values, which
        // aren't cross-checked against this engine's own constants
        // per plan.md `065`) -- an equivalent NUMBER OF SECONDS, not
        // the same literal unit distance, since this world's own
        // terrain (Y 0-13) is far shorter than a real level's, so
        // matching real "feel" (a real fall you notice) makes more
        // sense here than matching the real absolute margin.
        //
        // **-27.0f, not -60.0f (user feedback, 2026-07-13): even the
        // "feel"-matched ~6.2-6.7s read as too long in practice.** User
        // asked for roughly half. Using this engine's own kGravity=25/
        // kFallLimit=-10: reaching terminal velocity takes a fixed
        // 0.4s (10/25) covering 2.0 units regardless of the threshold;
        // the rest is covered at the 10 units/s terminal rate. This is
        // a game-feel adjustment, not a faithfulness correction (the
        // real absolute margin was never matched here anyway, see
        // above) -- verified live at 3.417s (down from the previously
        // live-verified 6.72s), i.e. almost exactly half.
        constexpr float kFallDeathY = -27.0f;
        if (blupi_.GetY() < kFallDeathY)
        {
            const float deathX = blupi_.GetX();
            const float deathY = blupi_.GetY();
            const float deathZ = blupi_.GetZ();
            // Real m_blupiRestart=true at this site (Decor.cpp:2757).
            triggerDeath(GalaxyEggbert::Def::SoundChannel::SoundChannel8,
                         GalaxyEggbert::Game::BlupiController::DeathCause::Clear2, true);
            // Real Clear2 ascend (plan.md `158` death-VFX follow-up,
            // Decor.cpp:2754-2761) -- deterministic, no coinflip.
            triggerDeathAscend(GalaxyEggbert::Game::InteractionSystem::VoyageKind::Clear2Ascend, 300.0f, 230,
                                deathX, deathY, deathZ);
        }

        // Lava hazard (plan.md E3D-MIG-140) -- deterministic death, no
        // vehicle immunity (mobile-eggbert-reference/
        // 12-hazards-and-interactables.md: "Lava(icon 68): deterministic
        // Clear3, no vehicle immunity, no focus requirement" -- Shield/
        // Hide immunity IS modeled now, plan.md E3D-MIG-170, confirmed
        // directly against Decor.cpp:5497) -- the
        // simplest of the 5 real isHazard() tile types to implement
        // faithfully for exactly that reason; Crusher/Saw/Blitz each
        // have real gating/timing conditions BlockTypes::isHazard() does
        // NOT encode, so each gets its own dedicated
        // task (E3D-MIG-142..144) instead of one generic "hazard" check.
        // The lava pool's actual wave surface, not the invisible top of its voxel:
        // the collision and contact query share the height field with the renderer.
        if (!blupi_.IsInvincible() && blupi_.IsTouchingLava(worldRuntime_.GetWorld()))
        {
            const float deathX = blupi_.GetX();
            const float deathY = blupi_.GetY();
            const float deathZ = blupi_.GetZ();
            // Real m_blupiRestart=true at this site (Decor.cpp:5500).
            triggerDeath(GalaxyEggbert::Def::SoundChannel::SoundChannel8,
                         GalaxyEggbert::Game::BlupiController::DeathCause::Clear3, true);
            // Real Clear3 ascend (plan.md `158` death-VFX follow-up,
            // Decor.cpp:5497-5499) -- deterministic, no coinflip.
            triggerDeathAscend(GalaxyEggbert::Game::InteractionSystem::VoyageKind::Clear3Ascend, 2000.0f, 40,
                                deathX, deathY, deathZ);
        }

        // Spikes hazard (plan.md E3D-MIG-141) -- real channel 51 (the
        // Glu-death sound, distinct from lava/fall's channel 8, per
        // 07-sounds.md). Real behavior (12-hazards-and-interactables.md)
        // gates this on vehicle immunity (Over/Jeep/Tank protect,
        // unlike lava) and requires m_blupiFocus (BlupiFocused(), BD-0060),
        // and vehicle immunity exists (`HasVehicleHazard
        // Immunity()`, fixed 2026-07-16 -- this comment previously said
        // "neither vehicles nor a focus concept exist... yet", stale
        // since vehicles were implemented `171`). The real check also
        // restricts to a narrow central x-band within the tile
        // (`pos.X%64` roughly 15-49, touching the tile's edges doesn't
        // count) -- not modeled, since BlupiController's single-point
        // 3D collision has no sub-tile position within a cell to test
        // against; the whole tile is lethal here.
        // m_blupiFocus too (Decor.cpp:5504-5505): busy, he is not caught.
        if (!blupi_.IsInvincible() && !blupi_.HasVehicleHazardImmunity() && BlupiFocused() &&
            blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Spike)
        {
            // Real GalaxyEggbert::Def::BlupiAction::Glu, m_blupiRestart=true (Decor.cpp:5504-5510);
            // the trap's tentacle rises where he stands (ObjectType53; BD-0054).
            interaction_.SpawnEffect(worldRuntime_, GalaxyEggbert::Def::ObjectType::ObjectType53, blupi_.GetX(), blupi_.GetY(),
                                     blupi_.GetZ());
            triggerDeath(GalaxyEggbert::Def::SoundChannel::SoundChannel51,
                         GalaxyEggbert::Game::BlupiController::DeathCause::Glu, true);
        }

        // Water drip hazard (plan.md TILE-032, real Decor::IsGoutte,
        // confirmed 2026-07-14 via direct Decor.cpp read -- despite the
        // "drip"/glue-sounding name, real behavior is a deterministic
        // kill, mechanically identical to Spike above: same
        // GalaxyEggbert::Def::BlupiAction::Glu death, same gate shape, same real channel 51
        // sound (Decor.cpp:5513-5519), same real Over/Jeep/Tank vehicle
        // immunity (fixed 2026-07-16, same as Spike above).
        // His own cell, as IsGoutte(m_blupiPos) (no +60 as IsPiege has; BD-0033): the drip
        // hangs under a ceiling, he dies going into it.
        // m_blupiFocus too (Decor.cpp:5513-5514; BD-0066): busy, he is not caught.
        if (!blupi_.IsInvincible() && !blupi_.HasVehicleHazardImmunity() && BlupiFocused() &&
            blupi_.GetBlockTypeAt(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Drip)
        {
            // Real GalaxyEggbert::Def::BlupiAction::Glu, m_blupiRestart=true (Decor.cpp:5513-5519).
            triggerDeath(GalaxyEggbert::Def::SoundChannel::SoundChannel51,
                         GalaxyEggbert::Game::BlupiController::DeathCause::Glu, true);
        }

        // Blitz hazard (plan.md E3D-MIG-144) -- real channel 8, same
        // lethality profile as lava per mobile-eggbert-reference/
        // 12-hazards-and-interactables.md ("No vehicle immunity --
        // same lethality profile as lava; only Shield/Hide/SuperBlupi
        // protect, and there is no focus requirement") -- unlike
        // spikes/drip/saw, Blitz needed no immunity simplification to
        // implement faithfully. Real `BlitzActif()`: a 100-tick cycle
        // at the same 20-ticks/sec reference rate worldRuntime_'s
        // GetAnimPhase() already advances at (Config::ScaleTime(1),
        // matching the real per-tile animation-divisor rate), lethal
        // only on even ticks within the first half (`num%2==0 &&
        // num<50` -- a rapid flicker for ~2.5s, 25% duty cycle over the
        // full ~5s period, then fully inactive for the second half).
        // The real cosmetic emitter tile (icon 304, sits one cell
        // above 305, times a zap sound cue only) is NOT implemented --
        // audio-only polish, not the hazard itself.
        // Lightning and saws catch Blupi in his own cell (Decor::IsBlitz/IsScie(m_blupiPos)),
        // as well as under his feet where worlds lay them as floor.
        const auto blockHere = blupi_.GetBlockTypeAt(worldRuntime_.GetWorld());
        const auto blockUnder = blupi_.GetGroundBlockType(worldRuntime_.GetWorld());
        if (!blupi_.IsInvincible() &&
            (blockUnder == GalaxyEggbert::BlockTypes::Blitz || blockHere == GalaxyEggbert::BlockTypes::Blitz) &&
            WorldRuntime::IsBlitzActiveAtPhase(worldRuntime_.GetAnimPhase()))
        {
            // Real GalaxyEggbert::Def::BlupiAction::Clear1, m_blupiRestart=true (Decor.cpp:5541-5547).
            triggerDeath(GalaxyEggbert::Def::SoundChannel::SoundChannel8,
                         GalaxyEggbert::Game::BlupiController::DeathCause::Clear1, true);
        }

        // Crusher hazard (plan.md E3D-MIG-143) -- unlike every hazard
        // above, this is NOT lethal: it squashes Blupi (reduced move
        // speed, no jump, ~10s auto-recovery) rather than costing a
        // life. Real `IsEcraseur()`'s 3-out-of-10 danger window
        // (`m_time / 3 % 10 <= 2`) is `IsCrusherActiveAtPhase()`. `TriggerCrush()` itself is idempotent (a
        // no-op while already squashed, matching the real
        // `!m_blupiEcrase` re-trigger guard) -- only play the real
        // entry sound (channel 70) when it actually starts a NEW squash.
        // Blupi's own cell, as IsEcraseur(m_blupiPos) (BD-0032): the crusher hangs in the cell
        // he walks through, not in the floor.
        // m_blupiFocus too (Decor.cpp:5551-5552; BD-0066).
        if (!blupi_.IsInvincible() && BlupiFocused() &&
            blupi_.GetBlockTypeAt(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Crusher &&
            WorldRuntime::IsCrusherActiveAtPhase(worldRuntime_.GetAnimPhase()) &&
            blupi_.TriggerCrush())
        {
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel70);
            // Four sparks, a flash and the big shake (Decor.cpp:5582-5591; BD-0054).
            interaction_.SpawnInvertBurst(worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(), true);
            interaction_.SpawnEffect(worldRuntime_, GalaxyEggbert::Def::ObjectType::ObjectType90, blupi_.GetX(), blupi_.GetY(),
                                     blupi_.GetZ());
            cameraShake_.Trigger(CameraShakeType::Big);
        }

        // Saw hazard (plan.md E3D-MIG-142) -- real channel 75 (the
        // "cut apart" death cue, distinct from every other hazard's
        // sound so far), deterministic, real Over/Jeep/Tank vehicle
        // immunity (`HasVehicleHazardImmunity()`, fixed 2026-07-16,
        // identical to Spike/Drip above -- this comment previously said
        // "no immunity simplification beyond what every hazard here
        // already lacks (vehicles... don't exist yet)", stale since
        // vehicles were implemented `171`). Only the active icon
        // (`Saw`, 378) is lethal -- the stopped variant (`SawStopped`,
        // 379) is excluded by the registry's hazard classification. A
        // saw starts active or stopped per however the world was
        // authored; TryActivateSwitch() below is what flips it between
        // the two.
        // m_blupiFocus too (Decor.cpp:5521-5522; BD-0066).
        if (!blupi_.IsInvincible() && !blupi_.HasVehicleHazardImmunity() && BlupiFocused() &&
            blupi_.IsTouchingSawBlade(worldRuntime_.GetWorld(), stepStartFeetY))
        {
            const float deathX = blupi_.GetX();
            const float deathY = blupi_.GetY();
            const float deathZ = blupi_.GetZ();
            // Real source has NO separate contact-site sound for Saw
            // (unlike Lava/fall's own extra channel 8) -- channel 75
            // comes entirely from BlupiDead's own Clear4 branch, which
            // SpawnSawDeathBurst() plays itself (plan.md `158`
            // death-VFX follow-up, Decor.cpp:5521-5524/6608-6613), so
            // triggerDeath() must NOT also play it here. Real
            // m_blupiRestart=true at this site (Decor.cpp:5526).
            triggerDeath(GalaxyEggbert::Def::SoundChannel::SoundChannel75,
                         GalaxyEggbert::Game::BlupiController::DeathCause::Clear4, true, false);
            interaction_.SpawnSawDeathBurst(worldRuntime_, deathX, deathY, deathZ, sound_);
        }

        // Spring / bounce tile (plan.md E3D-MIG-145, icon 211,
        // verified directly against Decor.cpp:2835-2911/7312-7320) --
        // NOT a hazard, launches Blupi upward instead of costing a
        // life. TriggerSpringBounce() is idempotent (a no-op while
        // already airborne, matching the real `!m_blupiAir` guard), so
        // only play the real bounce sound (channel 41) on an actual new
        // trigger. Real vehicle-dismount-first gate fixed 2026-07-16
        // (Decor.cpp:2837-2893): touching a spring while riding ANY
        // vehicle (Shield/Hide protect against this, matching every
        // other real forced-dismount-style gate) forcibly ejects
        // Blupi first (small shake + channel 10, depositing the
        // vehicle pickup back into the world via the same
        // `DismountAndDepositVehicle()` helper the voluntary
        // action-button dismount uses) -- the bounce itself still
        // applies on the same contact, real source does not treat
        // these as mutually exclusive.
        // The vehicle is wrecked there (an explosion, ObjectType9), not put down: BD-0015.
        if (blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Spring &&
            blupi_.IsInVehicle() && blupi_.GetSecretPower() != GalaxyEggbert::Def::SecretPower::Shield &&
            blupi_.GetSecretPower() != GalaxyEggbert::Def::SecretPower::Hide && !blupi_.GetCheatSuperBlupi())
        {
            WreckVehicle();
        }
        if (blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Spring &&
            blupi_.TriggerSpringBounce(jumpPressed && BlupiFocused())) // held counts with focus (2900)
        {
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel41);
        }

        // Taunts (plan.md BLUPI-067; Decor.cpp:5627-5635, MockeryDetect 9518-9601; BD-0044):
        // every tick he is in control, on foot or swimming, an enemy beside him at his height
        // (below him too while he jumps), or lava or a crusher under his jump, arms a taunt as
        // an Ouf; BlupiController plays it if he stands idle within 20 ticks. Nothing is armed
        // during the cooldown.
        if (blupi_.IsMockeryCooldownElapsed() && !blupi_.IsInVehicle() && !blupi_.IsSuspended() &&
            !blupi_.IsBallooned() && !blupi_.IsEcrased() && !blupi_.IsDeathLocked() && !blupi_.IsTeleporting() &&
            !blupi_.IsBye() && !blupi_.IsPickupFrozen() && !blupi_.IsOneShotAnimPlaying())
        {
            using A = GalaxyEggbert::Def::BlupiAction;
            const bool airborne = !blupi_.IsOnGround() && !blupi_.IsSurf() && !blupi_.IsNage();
            const A taunt = InteractionSystem::MockeryDetect(worldRuntime_, blupi_.GetX(), blupi_.GetY(),
                                                             blupi_.GetZ(), blupi_.GetYaw(), airborne);
            if (taunt != A::None)
            {
                blupi_.ArmOuf(taunt == A::Mockery    ? BlupiController::AnimState::Mockery
                              : taunt == A::Mockeryi ? BlupiController::AnimState::Mockeryi
                                                     : BlupiController::AnimState::Mockeryp);
            }
        }

        // Hub/mission-progression: world-select portal contact (plan.md
        // hub/mission system, found/implemented 2026-07-17, real
        // `Decor::IsWorld()`/the `Bye`-action mission-change handler).
        // Contact-triggered, no debounce needed. The real ~1.5s "turn and
        // wave" `Bye` freeze is now modeled (plan.md BLUPI-049, found/
        // wired 2026-07-17): `TriggerBye()` freezes Blupi and remembers
        // the target mission; `SetYaw()` turns him to face the camera
        // (real `m_blupiFront=true`), matching the real trigger scope
        // exactly (`Decor.cpp:5476-5488`) -- world-select ONLY, not the
        // exit-goal/PauseBack/PauseRestart paths below/elsewhere, which
        // stay instant in real source too. `LoadMission()` itself now
        // happens later, off the `wasBye`/`IsBye()` completion check
        // right after `Step()` above, once the freeze naturally elapses.
        //
        // Uses `GetBlockTypeAt()` (Blupi's own occupied cell), NOT
        // `GetGroundBlockType()` (found/fixed 2026-07-21, live bug
        // report): real `Decor::IsWorld(m_blupiPos)` is a plain direct
        // position lookup, independent of solidity/grounding entirely
        // -- matching that exactly needs the query that already exists
        // for the same reason as water detection, not the one gated on
        // IsOnGround(). These markers' real per-icon quarter-cell mask
        // is genuinely all-zero/thin (same category as Lava/Crusher/
        // Saw), so INFRA-005's sub-tile-precision wall collision
        // correctly stopped treating them as solid enough to
        // (accidentally) step up onto -- `GetGroundBlockType()` alone
        // then never saw them again, since nothing elevates Blupi onto
        // them any more. The real game was never checking "am I
        // standing ON the marker" in the first place, only "is the
        // marker AT my own position" -- mere contact, exactly as the
        // real 2D game and this engine's own pre-INFRA-005 (buggier)
        // collision both allowed by different, unrelated routes.
        // Only while not already saying goodbye (Decor.cpp:5476 `m_blupiAction != Bye`): he still
        // stands on the marker during the farewell, and returning here every frame skipped the
        // rest of the update, the Bye clip's playback with it (OR-0002: the farewell froze).
        if (!blupi_.IsBye())
        {
            const auto groundBlock = blupi_.GetBlockTypeAt(worldRuntime_.GetWorld());
            if (GalaxyEggbert::BlockTypes::isWorldSelect(groundBlock))
            {
                const int target = WorldRuntime::ComputeWorldSelectTarget(
                    worldRuntime_.GetMissionNumber(), GalaxyEggbert::BlockTypes::worldSelectIndex(groundBlock),
                    galaxyGamer_ >= 0 ||
                        (editorPlayTestActive_ && GalaxyEggbert::Editor::ParseGalaxyWorldPath(
                                                      editorPlayTestWorldPath_.generic_string())));
                if (blupi_.TriggerBye())
                {
                    byePendingTarget_ = target;
                    const float dx = camera_.Position().X - blupi_.GetX();
                    const float dz = camera_.Position().Z - blupi_.GetZ();
                    blupi_.SetYaw(std::atan2(dx, -dz));
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel32);
                }
                return true;
            }
        }

        // Teleporter (plan.md E3D-MIG-147, icons 330-333, verified
        // directly against Decor.cpp:7378-7394/5593-5606) -- checked
        // one cell ABOVE Blupi (GetBlockTypeAbove()), matching the real
        // detection geometry exactly. Teleporter icons are always
        // non-solid for collision (GroundHeightAt's own
        // registry-backed teleporter collision skip), so Blupi can genuinely walk into
        // the open space beneath a floating pillar to trigger this,
        // the same way real mobile-eggbert's per-tile-independent 2D
        // collision lets him walk under one (see GetBlockTypeAbove()'s
        // own comment). The real narrow 7px-wide left-edge sub-tile
        // band is NOT modeled -- no sub-tile position exists in this
        // engine's single-point collision, same simplification already
        // applied to spikes' own real sub-tile band. Real gate
        // (`!m_blupiHelico/Over/Balloon/Ecrase/Jeep/Tank/Skate &&
        // !m_blupiAir && m_blupiFocus`) is checked inside
        // TriggerTeleport() itself (grounded, not ballooned/squashed,
        // and -- fixed 2026-07-16 -- not in any vehicle mode either);
        // focus by BlupiFocused() (2026-11-04). Idempotent, same shape as
        // every other Trigger*() here, so channel 71 only plays on an
        // actual new trigger.
        // Not while m_blupiPosHelico is set (Decor.cpp:5594): just off a vehicle, out of a
        // popped balloon, or where a teleporter put him, until he is 30 px from that spot
        // (BD-0017); and not before he has left the cell a teleporter put him in, since here
        // the whole cell under a pillar triggers, not the original's 7 px band (BD-0021).
        const auto aboveIcon = blupi_.GetBlockTypeAbove(worldRuntime_.GetWorld());
        if (teleportArrivalGuard_ && (static_cast<int>(std::lround(blupi_.GetX())) != teleportArrivalCellX_ ||
                                      static_cast<int>(std::lround(blupi_.GetZ())) != teleportArrivalCellZ_))
        {
            teleportArrivalGuard_ = false; // he has left the cell he arrived in
        }
        if (GalaxyEggbert::BlockTypes::isTeleporter(aboveIcon) && !interaction_.ContactGraceActive() &&
            !teleportArrivalGuard_ && BlupiFocused() /* m_blupiFocus, Decor.cpp:5594 */ &&
            blupi_.TriggerTeleport(aboveIcon))
        {
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel71);
            // Real teleporter arc (plan.md VISUAL-010, ObjectType92,
            // Decor.cpp:5606) -- despite Def/ObjectType.hpp's own "charged
            // attack" doc comment, this is the real, only spawn site.
            interaction_.SpawnTeleportArc(worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ());
        }

        // Fan hazard (Decor.cpp:5459-5474, IsVentillo; WorldRuntime::TryConsumeFan): touching a
        // fan at its mouth destroys it with its wind. Every touch explodes (ObjectType11), shakes
        // the screen hard and plays channel 10; only the death is gated on Shield, Hide and
        // SuperBlupi, so a shielded Blupi survives the explosion he caused (BD-0042: until
        // 2026-11-03 the explosion, the shake and the sound came only with the death). The death
        // is BlupiDead(Clear1, Clear2), the 50/50 coinflip, without m_blupiRestart.
        if (const auto fanIcon = worldRuntime_.TryConsumeFan(blupi_.GetX(), blupi_.GetY(), blupi_.GetZ()))
        {
            // The fan's own piece flies away (ModifDecor's ByeByeAdd, BD-0049).
            const auto fanCell = worldRuntime_.LastConsumedFan();
            interaction_.ByeByeFan(*fanIcon, fanCell[0], fanCell[1], fanCell[2]);
            // Captured BEFORE triggerDeath() respawns Blupi: the flash and the ascent start
            // where he died.
            const float deathX = blupi_.GetX();
            const float deathY = blupi_.GetY();
            const float deathZ = blupi_.GetZ();
            cameraShake_.Trigger(CameraShakeType::Big);
            interaction_.SpawnFanHitFlash(worldRuntime_, deathX, deathY, deathZ);
            if (blupi_.IsInvincible())
            {
                sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel10);
            }
            else
            {
                const bool isClear2 = interaction_.RollClear2Coinflip();
                triggerDeath(GalaxyEggbert::Def::SoundChannel::SoundChannel10,
                             isClear2 ? GalaxyEggbert::Game::BlupiController::DeathCause::Clear2
                                      : GalaxyEggbert::Game::BlupiController::DeathCause::Clear1,
                             false);
                if (isClear2)
                {
                    triggerDeathAscend(GalaxyEggbert::Game::InteractionSystem::VoyageKind::Clear2Ascend, 300.0f,
                                        230, deathX, deathY, deathZ);
                }
            }
        }

        // Water Surf/Nage (plan.md E3D-MIG-148) -- transition sounds via
        // before/after comparison (wasSurf/wasNage captured before
        // Step() above), same idiom as every other status this session.
        // Real channel 25 (start-surfing) plays specifically on
        // Nage->Surf (resurfacing). Real vehicle exclusion fixed
        // 2026-07-16 -- this comment previously mis-described it as
        // "forced dismount on water entry"; verified directly against
        // Decor.cpp:5284-5285: it's not a dismount at all, water tiles
        // simply never register as Surf/Nage while riding/ballooned/
        // squashed (`canEnterWater` above, computed before Step()) -- a
        // vehicle just drives straight over/through water instead.
        //
        // **Corrected 2026-07-17** (water-splash research): channel 22
        // was wired backwards -- verified directly against every real
        // `PlaySound(SoundChannel22, ...)` call site (`Decor.cpp:5356/
        // 5378/5392/5405`): ALL four are in the water-EXIT paths, none
        // in entry. Real water ENTRY instead plays channel 23 ("Plouf",
        // `Decor::MoveObjectPlouf`, `ObjectType14`) via the two dry->
        // Surf/dry->Nage branches (`Decor.cpp:5298/5318`) -- previously
        // entirely unwired (no code/sound at all on entry). Also added:
        // the jump out of the water (BD-0041): at the jump itself, as the original
        // (Decor.cpp:5336-5357), channel 22 and the "Tiplouf" splash (channel 64,
        // `ObjectType35`, at most one at a time); leaving the water any other way, channel 22
        // alone (Decor.cpp:5359-5408). Rising out on the jump is not a second exit.
        const bool wasDry = !wasSurf && !wasNage;
        const bool nowDry = !blupi_.IsSurf() && !blupi_.IsNage();
        if (wasDry && !nowDry && !interaction_.HasActiveObjectOfType(worldRuntime_, GalaxyEggbert::Def::ObjectType::ObjectType14))
        {
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel23);
            interaction_.SpawnWaterSplash(worldRuntime_, GalaxyEggbert::Def::ObjectType::ObjectType14, blupi_.GetX(),
                                           blupi_.GetY(), blupi_.GetZ());
        }
        if (blupi_.JumpedOutOfWater())
        {
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel22);
            if (!interaction_.HasActiveObjectOfType(worldRuntime_, GalaxyEggbert::Def::ObjectType::ObjectType35))
            {
                sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel64);
                interaction_.SpawnWaterSplash(worldRuntime_, GalaxyEggbert::Def::ObjectType::ObjectType35, blupi_.GetX(),
                                               blupi_.GetY(), blupi_.GetZ());
            }
        }
        else if (!wasDry && nowDry && !blupi_.IsWaterExitJump())
        {
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel22);
        }
        if (wasNage && blupi_.IsSurf())
        {
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel25);
        }
        // Ambient rising bubbles while fully submerged (plan.md
        // PICKUP-079, ObjectType15/channel 24) -- real trigger
        // `m_time % ScaleTime(70) == 0 || == ScaleTime(28)` (twice per
        // ~3.5s cycle, `Decor.cpp:4611-4613`), reusing this engine's own
        // 1:1-tick `GetAnimPhase()` the same way every other real
        // `m_time %`-gated periodic effect already does.
        if (blupi_.IsNage())
        {
            const int animTick = worldRuntime_.GetAnimPhase();
            if (animTick % 70 == 0 || animTick % 70 == 28)
            {
                sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel24);
                interaction_.SpawnWaterBubble(worldRuntime_, worldRuntime_.GetWorld(), blupi_.GetX(), blupi_.GetY(),
                                               blupi_.GetZ());
            }
        }
        // Drowning (real GalaxyEggbert::Def::BlupiAction::Drown, channel 26 -- a dedicated
        // death sound distinct from every other cause's channel 8/51/75,
        // per 07-sounds.md's own note). Real Shield/Hide/SuperBlupi
        // immunity gates the death itself (plan.md E3D-MIG-170); the
        // gauge still depletes regardless (matches every other hazard's
        // "still triggers, only the death is gated" pattern); superBlupi
        // is the cheat's, in IsInvincible().
        if (blupi_.JustDrowned() && !blupi_.IsInvincible())
        {
            // Real GalaxyEggbert::Def::BlupiAction::Drown, m_blupiRestart=true (Decor.cpp:4640-4652).
            triggerDeath(GalaxyEggbert::Def::SoundChannel::SoundChannel26,
                         GalaxyEggbert::Game::BlupiController::DeathCause::Drown, true);
        }

        // Real 10-slot safe-position FIFO respawn (plan.md E3D-MIG-067,
        // see BlupiController::UpdateSafePosition()'s own comment) --
        // "safe" here additionally means not standing on any of the 6
        // real terrain hazard tiles (Lava/Spike/Saw/active-Blitz/Temp/
        // Drip), not currently under a teleporter trigger (`aboveIcon`, already
        // computed above), and not currently Nage (plan.md E3D-MIG-148 --
        // respawning mid-drown with an already-depleted gauge would be a
        // bad experience, same reasoning as excluding active hazards) --
        // and the rest of the original's gate below (BD-0048).
        {
            const auto safetyGroundBlock = blupi_.GetGroundBlockType(worldRuntime_.GetWorld());
            const bool onHazardTile =
                safetyGroundBlock == GalaxyEggbert::BlockTypes::Lava ||
                safetyGroundBlock == GalaxyEggbert::BlockTypes::Spike ||
                safetyGroundBlock == GalaxyEggbert::BlockTypes::Saw ||
                safetyGroundBlock == GalaxyEggbert::BlockTypes::Temp ||
                blupi_.GetBlockTypeAt(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Drip || // IsGoutte(.., true)
                (safetyGroundBlock == GalaxyEggbert::BlockTypes::Blitz &&
                 WorldRuntime::IsBlitzActiveAtPhase(worldRuntime_.GetAnimPhase()));
            const bool underTeleporter =
                aboveIcon == GalaxyEggbert::BlockTypes::Teleport1 || aboveIcon == GalaxyEggbert::BlockTypes::Teleport2 ||
                aboveIcon == GalaxyEggbert::BlockTypes::Teleport3 || aboveIcon == GalaxyEggbert::BlockTypes::Teleport4;
            // The rest of the original's gate (Decor.cpp:6467-6471; BD-0048): not with the shield
            // or Hide, not teetering at a ledge, not on a lift, not on (or in) a clay bridge, not
            // in a cell an object has been in or a shot has crossed (IsMoveTraj, IsBalleTraj).
            const auto animState = blupi_.GetAnimState();
            const bool atLedge = animState == BlupiController::AnimState::Vertigo ||
                                 animState == BlupiController::AnimState::Recede ||
                                 animState == BlupiController::AnimState::Advance;
            const bool onBridge = safetyGroundBlock == GalaxyEggbert::BlockTypes::Bridge ||
                                  blupi_.GetBlockTypeAt(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Bridge;
            const bool onTrajectory =
                interaction_.IsTrajectoryCell(worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ());
            blupi_.UpdateSafePosition(!onHazardTile && !underTeleporter && !blupi_.IsNage() && !blupi_.IsShielded() &&
                                      !blupi_.IsHidden() && !atLedge && !interaction_.IsRidingLift() && !onBridge &&
                                      !onTrajectory);
        }

        // Switches (plan.md E3D-MIG-142, see WorldRuntime::
        // TryActivateSwitch()'s own comment for the real 41-cell
        // switch-to-saw linking) -- Space ("Action"), edge-detected the
        // same way jumpPressed is above so holding it doesn't retoggle
        // every frame. `wasEcrased`/`wasOnGround`-style capture isn't
        // needed here: TryActivateSwitch() itself is the single
        // authoritative check (grounded + standing on a switch tile),
        // no separate before/after state to compare.
        // Captured into a local (not just the `if` condition below)
        // since Sucette/Drink's own real action-button gate (plan.md
        // `173`) needs this SAME edge-detected value passed into
        // interaction_.Update() further down, by which point
        // actionKeyWasDown_ has already been overwritten with this
        // frame's raw actionPressed (see its own assignment below).
        // Whether a switch, dynamite, the decoy or a vehicle used this press: if none did (nor a
        // dynamite stick taken in interaction_.Update()), he shakes his head (Non, below).
        if (actionPressedEdge)
        {
            // Real switch-activation gate (found 2026-07-16, Decor.cpp:5529-5531) also
            // excludes Overcraft/Jeep/Tank/Skateboard and Balloon -- NOT Helicopter, which
            // real source allows (a hovering Helicopter can still reach down and press a
            // switch, unlike the other 4 ground-committed/no-hands vehicle states). Same
            // "predates vehicle modeling" gap already found/fixed this session elsewhere
            // (pickups/TriggerTeleport/ground-jump/hazard immunity) -- TryActivateSwitch()
            // itself has no BlupiController access, so this is gated here in the caller
            // instead, matching that established decoupling. Scoped to JUST this call, NOT
            // the whole actionPressedEdge block below -- Dynamite/Perso/vehicle mount-dismount
            // share the same action-button press but have their own independent real gates
            // (in particular, dismounting a vehicle must still work while riding one).
            const bool canActivateSwitch =
                blupi_.GetVehicleMode() != GalaxyEggbert::Game::BlupiController::VehicleMode::Overcraft &&
                blupi_.GetVehicleMode() != GalaxyEggbert::Game::BlupiController::VehicleMode::Jeep &&
                blupi_.GetVehicleMode() != GalaxyEggbert::Game::BlupiController::VehicleMode::Tank &&
                blupi_.GetVehicleMode() != GalaxyEggbert::Game::BlupiController::VehicleMode::Skateboard &&
                !blupi_.IsBallooned() &&
                // Nor with the shield, Hide or SuperBlupi (`!m_blupiShield && !m_blupiHide &&
                // !m_bSuperBlupi`, the same gate; BD-0045).
                !blupi_.IsInvincible();
            if (canActivateSwitch)
            {
                if (const auto turnedOn = worldRuntime_.TryActivateSwitch(
                        blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(), blupi_.IsOnGround()))
                {
                    actionUsed = true;
                    ++autoplaySwitchPresses_;
                    autoplaySawsTurning_ = -1; // counted again for the autoplay's next state
                    sound_.Play(*turnedOn ? GalaxyEggbert::Def::SoundChannel::SoundChannel77
                                          : GalaxyEggbert::Def::SoundChannel::SoundChannel76);
                    // Real Switch animation (plan.md BLUPI-073, found
                    // 2026-07-18) -- purely cosmetic on top of the
                    // switch already having activated above; a no-op if
                    // some other freeze is already active, matching
                    // every other TriggerX() call in this file.
                    blupi_.TriggerOneShotAnim(BlupiController::AnimState::Switch,
                                               BlupiController::kSwitchDuration);
                }
            }

            // Dynamite/Perso PLACEMENT vehicle gate (found 2026-07-16, Decor.cpp:4792-4794):
            // real source excludes EVERY vehicle mode (unlike switches, Helicopter is NOT
            // exempt here) plus Balloon/Ecrase. Deliberately does NOT gate Perso PICKUP
            // (TryPerso()'s own internal pickup branch, real Decor.cpp:6088-6101 has no
            // vehicle clause at all) -- see PlaceDynamite()/TryPerso()'s own header comments.
            // Nor on a lift: the same block asks m_blupiTransport == -1 (Decor.cpp:4794, BD-0027).
            const bool blupiCanUseHands = !blupi_.IsInVehicle() && !blupi_.IsBallooned() && !blupi_.IsEcrased() &&
                                          !interaction_.IsRidingLift();

            // Dynamite placement (plan.md E3D-MIG-155) -- same action
            // button, independent gate (carrying one + grounded) from
            // the switch check above, so both can coexist on one press
            // without stepping on each other (real channel 61, shared
            // with skin-swap per 07-sounds.md, unrelated here).
            // Both feet on the ground to put one down (BlupiController::BothFeetOnGround, BD-0051).
            const bool feetDown = blupi_.BothFeetOnGround(worldRuntime_.GetWorld(), tempPassable);
            if (interaction_.PlaceDynamite(worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(),
                                            feetDown, blupiCanUseHands))
            {
                actionUsed = true;
                sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel61);
                // Real PutDynamite animation (plan.md BLUPI-076, found
                // 2026-07-18) -- same "cosmetic on top of an action
                // that already happened" shape as Switch above.
                blupi_.TriggerOneShotAnim(BlupiController::AnimState::PutDynamite,
                                           BlupiController::kPutDynamiteDuration);
            }
            else
            {
                // Perso decoy placement/pickup (plan.md HUD-017) --
                // real `else if (m_blupiPerso > 0)`, mutually exclusive
                // with dynamite above. Channel 61 is the real placement
                // sound (per 07-sounds.md), unaffected by Voyage timing.
                // The pickup case's own real touch-time sound (ch60)
                // and deferred completion sound (ch3, plan.md `158`)
                // are handled by ResolvePendingVoyage() below instead
                // -- PersoCount() no longer increments immediately, so
                // it can't be used to distinguish pickup from placement
                // anymore; a placement always DECREASES the count
                // immediately (unaffected by Voyage), so that's the
                // real distinguishing signal now.
                const int persoBefore = interaction_.PersoCount();
                const bool persoUsed = interaction_.TryPerso(worldRuntime_, blupi_.GetX(), blupi_.GetY(),
                                                             blupi_.GetZ(), feetDown, blupiCanUseHands);
                actionUsed = actionUsed || persoUsed || (persoBefore > 0 && blupiCanUseHands);
                if (persoUsed && interaction_.PersoCount() < persoBefore)
                {
                    // Putting the decoy down: Ouf2, held (Decor.cpp:4833-4840).
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel61);
                    blupi_.TriggerOneShotAnim(BlupiController::AnimState::Ouf2,
                                               BlupiController::kOuf2Duration);
                }
                else if (!persoUsed && persoBefore > 0 && blupiCanUseHands && interaction_.DynamiteCount() == 0)
                {
                    // It cannot go down here: his shrug, Ouf3 (Decor.cpp:4843-4848).
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel27);
                    blupi_.TriggerReaction(BlupiController::AnimState::Ouf3,
                                           BlupiController::kOuf3Duration);
                }
                ResolvePendingVoyage();
            }

            // Vehicle mount/dismount (plan.md E3D-MIG-171) -- same
            // action button. While already riding, dismounts (real:
            // voluntary dismount, no fixed duration otherwise) and
            // deposits the vehicle pickup back into the world at
            // Blupi's position, matching the real "deposits vehicle
            // pickup back into the world" behavior. While not riding,
            // scans for a nearby active vehicle pickup (real confirmed
            // mapping: ObjectType13->Helicopter, 19->Jeep, 28->Tank,
            // 24->Skateboard, 46->Overcraft) and mounts it -- done
            // directly here (not inside InteractionSystem::Update(),
            // which has no access to BlupiController::VehicleMode)
            // matching the same architecture as TryActivateSwitch/
            // PlaceDynamite above. Real "requires the action button
            // held/pressed at contact" gate is modeled via the same
            // proximity radius every other pickup here uses; the
            // helicopter floating over a gap is boarded without it
            // (`Decor::IsFloatingObject`, BD-0024, further below).
            if (blupi_.IsInVehicle())
            {
                // Only on the ground, off any lift, the helicopter and the overcraft on both feet,
                // the skateboard slow (OR-0020: Space in the air left the helicopter hanging).
                actionUsed = true; // riding, he is never in Stop: no head shake
                if (blupi_.CanLeaveVehicle(worldRuntime_.GetWorld(), interaction_.IsRidingLift(), tempPassable))
                {
                    DismountAndDepositVehicle();
                }
            }
            else if (!pendingSkate_.active)
            {
                // Touching it, as the original's boxes have it (the `bNear` contact the mount
                // sits in, Decor.cpp:5646-5760; it was a 1.1-cell sphere, BD-0024).
                for (auto& obj : worldRuntime_.GetMobileObjectsMutable())
                {
                    if (!obj.active) continue;
                    BlupiController::VehicleMode mode;
                    if (!ObjectTypeToVehicleMode(obj.type, mode))
                    {
                        continue;
                    }
                    const float mdx = obj.currentX - blupi_.GetX();
                    const float mdy = obj.currentY - blupi_.GetY();
                    const float mdz = obj.currentZ - blupi_.GetZ();
                    if (InteractionSystem::BlupiTouches(obj.type, mdx, mdy, mdz) &&
                        blupi_.TriggerMount(mode, blupi_.IsNage(), blupi_.IsSurf()))
                    {
                        actionUsed = true;
                        // Real TakeSkate anim (plan.md BLUPI-091 gap,
                        // table_blupi ID 42, wired 2026-07-19) -- same
                        // "only Skateboard has this" reasoning as
                        // DeposeSkate in DismountAndDepositVehicle().
                        if (mode == BlupiController::VehicleMode::Skateboard)
                        {
                            blupi_.TriggerOneShotAnim(BlupiController::AnimState::TakeSkate,
                                                       BlupiController::kTakeSkateDuration);
                            // The board leaves the ground at TakeSkate's tick 8, when his hands
                            // reach it (Decor.cpp:4510-4518; OR-0003).
                            pendingSkate_ = {};
                            pendingSkate_.active = true;
                            pendingSkate_.take = true;
                            pendingSkate_.seconds = 8.0f / 20.0f;
                            pendingSkate_.objectIndex = static_cast<std::size_t>(
                                &obj - worldRuntime_.GetMobileObjectsMutable().data());
                            break;
                        }
                        obj.active = false;
                        break;
                    }
                }
            }
        }

        // A helicopter floating over nothing (the cell under it passable, Decor::IsFloatingObject)
        // is boarded by touching it, without the button (Decor.cpp:5646-5650; BD-0024): he
        // jumps or drops into it.
        if (!blupi_.IsInVehicle() && !blupi_.IsBallooned() && !blupi_.IsEcrased() && !blupi_.IsNage() &&
            !blupi_.IsSurf() && !blupi_.IsSuspended() && !pendingSkate_.active)
        {
            const auto& world = worldRuntime_.GetWorld();
            for (auto& obj : worldRuntime_.GetMobileObjectsMutable())
            {
                if (!obj.active || obj.type != GalaxyEggbert::Def::ObjectType::ObjectType13)
                {
                    continue;
                }
                const int gx = static_cast<int>(std::lround(obj.currentX)) + WorldRuntime::kWorldCenterX;
                const int gz = static_cast<int>(std::lround(obj.currentZ)) + WorldRuntime::kWorldCenterZ;
                const int gy = static_cast<int>(std::floor(obj.currentY + 0.001f)) - 1;
                const int n = static_cast<int>(world.blocksPerAxis());
                const bool floating =
                    gx >= 0 && gx < n && gz >= 0 && gz < n && gy >= 0 && gy < n &&
                    GalaxyEggbert::GetBlockDefinition(
                        world.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                       static_cast<std::uint16_t>(gz)).type())
                            .collisionMask == 0;
                if (floating &&
                    InteractionSystem::BlupiTouches(obj.type, obj.currentX - blupi_.GetX(), obj.currentY - blupi_.GetY(),
                                                    obj.currentZ - blupi_.GetZ()) &&
                    blupi_.TriggerMount(BlupiController::VehicleMode::Helicopter, false, false))
                {
                    obj.active = false;
                    break;
                }
            }
        }

        return false;
    }


    // UpdateTickWorld's end: the camera (its mode, the chase, the look round) and his animation's clip and time.
    // True when the tick ends here.
    bool GalaxyEggbertGame::UpdateTickCamera(float dt, float simDt, const Microsoft::Xna::Framework::Input::KeyboardState& keys,
                                             bool inputPadClaimedMouse, bool crouchHeld, bool lookUpHeld, float moveInput,
                                             float turnInput)
    {
        using Microsoft::Xna::Framework::Input::Keys;
        // Camera-mode toggle (2026-07-09, NEXT.md §3) -- F3 since
        // 2026-10-25 (was "C"; letters are typed cheats), edge-detected
        // (same pattern as demo_avatar's Space-toggle) so a held key
        // doesn't flip modes every frame. Only actually switches to
        // third-person if a model loaded successfully
        // (blupiModelLoaded_) -- otherwise silently stays first-person,
        // there being nothing to show in third-person.
        const bool cameraModeKeyDown = keys.IsKeyDown(Keys::F3);
        if (cameraModeKeyDown && !cameraModeKeyWasDown_ && blupiModelLoaded_)
        {
            cameraMode_ = (cameraMode_ == CameraMode::FirstPerson)
                              ? CameraMode::ThirdPersonModel
                              : CameraMode::FirstPerson;
        }
        cameraModeKeyWasDown_ = cameraModeKeyDown;

        // Mouse drag-look (2026-07-10, user request): holding the left
        // button and dragging rotates the camera around Blupi without
        // turning him (yaw offset + clamped pitch offset on top of his
        // facing). Any movement input decays the offsets smoothly back
        // to zero, returning the camera behind him -- "look around,
        // then it snaps back when you walk". Drag (not free mouselook)
        // so it maps 1:1 onto touch input too (SDL reports touch drags
        // as mouse drags).
        //
        // inputPadClaimedMouse (2026-07-13, plan.md MENU-021..027)
        // forces lookHeld false whenever this frame's press landed on
        // an on-screen D-pad/Jump/Action/Pause control (computed
        // earlier this same Update(), before the Play-gate) -- without
        // it, dragging the on-screen D-pad would also spin the camera,
        // since both features read the same left-mouse-button state.
        {
            using Microsoft::Xna::Framework::Input::ButtonState;
            const auto mouse = CurrentMouseState();
            const int mouseX = mouse.getXProperty();
            const int mouseY = mouse.getYProperty();
            const bool lookHeld =
                mouse.getLeftButtonProperty() == ButtonState::Pressed && !inputPadClaimedMouse;
            if (lookHeld && mouseLookActive_)
            {
                constexpr float kLookRadiansPerPixel = 0.008f;
                constexpr float kPitchLimit = 1.2f;
                lookYawOffset_ += static_cast<float>(mouseX - lastMouseX_) * kLookRadiansPerPixel;
                lookPitchOffset_ += static_cast<float>(mouseY - lastMouseY_) * kLookRadiansPerPixel;
                lookPitchOffset_ = std::clamp(lookPitchOffset_, -kPitchLimit, kPitchLimit);
            }
            mouseLookActive_ = lookHeld;
            lastMouseX_ = mouseX;
            lastMouseY_ = mouseY;

            if ((moveInput != 0.0f || turnInput != 0.0f) && !lookHeld)
            {
                // Same framerate-independent exponential decay shape as
                // the camera damping below.
                constexpr float kLookReturnPerSecond = 5.0f;
                const float keep = std::exp(-kLookReturnPerSecond * dt);
                lookYawOffset_ *= keep;
                lookPitchOffset_ *= keep;
                if (std::fabs(lookYawOffset_) < 0.001f) lookYawOffset_ = 0.0f;
                if (std::fabs(lookPitchOffset_) < 0.001f) lookPitchOffset_ = 0.0f;
            }
        }

        const float yaw = blupi_.GetYaw() + lookYawOffset_;

        Microsoft::Xna::Framework::Vector3 rawEye;
        Microsoft::Xna::Framework::Vector3 rawTarget;

        if (cameraMode_ == CameraMode::FirstPerson)
        {
            // First-person/player-view camera (2026-07-05): look from
            // Blupi's eye position in his current facing direction (see
            // BlupiController's GetYaw() convention: 0 rad = facing
            // -Z). Crouching lowers the eye height; looking up tilts the
            // look target upward — neither is a real head/body pose
            // (still no first-person 3D model), just a rough
            // camera-only stand-in for the animation-indicator state.
            constexpr float kEyeHeight = 0.75f;
            constexpr float kCrouchEyeHeight = 0.4f;
            constexpr float kLookDistance = 5.0f;
            constexpr float kLookUpTilt = 2.5f;
            const float eyeHeight = crouchHeld ? kCrouchEyeHeight : kEyeHeight;
            const float lookYOffset = lookUpHeld ? kLookUpTilt : 0.0f;
            rawEye = Microsoft::Xna::Framework::Vector3(blupi_.GetX(), blupi_.GetY() + eyeHeight, blupi_.GetZ());
            chaseCamera_.Reset(blupi_.GetYaw()); // third person starts behind him
            // Mouse-look pitch: positive offset (drag down) tilts the
            // view down, standard non-inverted feel. Yaw offset is
            // already folded into `yaw` above.
            rawTarget = Microsoft::Xna::Framework::Vector3(
                rawEye.X + std::sin(yaw) * kLookDistance,
                rawEye.Y + lookYOffset - std::sin(lookPitchOffset_) * kLookDistance,
                rawEye.Z - std::cos(yaw) * kLookDistance);
        }
        else
        {
            // Third-person chase camera (2026-07-09) -- sits behind and
            // above Blupi (opposite his facing direction, same sin/-cos
            // yaw convention as the first-person target above) looking
            // slightly down at him. A fixed offset, not a real
            // spring-arm/collision-aware orbit camera -- sufficient to
            // show the placeholder model, not a final camera design.
            // The Setup's automatic zoom brings it in while he walks (chaseZoom_, OR-0010).
            // The cheat `zoom` takes the view out 2x, 4x, 8x, 16x (OR-0079).
            const float kChaseDistance = 4.0f * static_cast<float>(1 << cheatZoomStep_) / chaseZoom_;
            constexpr float kChaseLookHeight = 1.0f;
            // Base elevation reproduces the previous fixed offset
            // (height 1.2 over distance 4); mouse-look pitch orbits the
            // camera up/down around Blupi on top of it (drag down =
            // camera rises, view tilts down -- same direction sense as
            // the first-person branch).
            constexpr float kBaseElevation = 0.29f;
            // and raises it toward a bird's-eye view as it goes out (0.25 rad a step).
            const float elevation = std::clamp(
                kBaseElevation + lookPitchOffset_ + 0.25f * static_cast<float>(cheatZoomStep_), -1.2f, 1.45f);
            rawTarget = Microsoft::Xna::Framework::Vector3(
                blupi_.GetX(), blupi_.GetY() + kChaseLookHeight, blupi_.GetZ());
            // Jumping under a low ceiling (a three-high tunnel) lifts the look point into the
            // block above him, and a camera marched out from inside a block stays in it,
            // showing only the sky behind (reported by the world authoring, 2026-10-26):
            // look from just under that block instead.
            if (BlocksCamera(worldRuntime_.GetWorld(), rawTarget))
            {
                rawTarget.Y = std::max(blupi_.GetY() + 0.2f, std::round(rawTarget.Y) - 0.5f - 0.15f);
            }

            // Orbit (ChaseCameraYaw, the real-Blupi plan's phase E, user
            // decisions 2026-10-24): behind him while he moves, lagging on
            // turns so he shows his side for a moment; on his nearer open
            // side once he stands idle; on his right, the side the sprites
            // showed, for clips that turn him toward their viewer. A side
            // is open when a wall leaves the camera most of its distance.
            {
                constexpr float kHalfPi = 1.57079632679f;
                constexpr float kOpenFraction = 0.6f;
                const auto sideOpen = [&](float orbitYaw)
                {
                    const Microsoft::Xna::Framework::Vector3 direction(-std::sin(orbitYaw), 0.0f,
                                                                       std::cos(orbitYaw));
                    return RaymarchWallDistance(worldRuntime_.GetWorld(), rawTarget, direction, kChaseDistance) >=
                           kChaseDistance * kOpenFraction;
                };
                GalaxyEggbert::Game::ChaseCameraInput chaseInput;
                chaseInput.blupiYaw = blupi_.GetYaw();
                chaseInput.idle = blupi_.GetAnimState() == BlupiController::AnimState::Stop &&
                                  moveInput == 0.0f && turnInput == 0.0f;
                chaseInput.facesViewer = blupiModelIsReal_ && blupiViewerClips_.count(blupiActiveClipName_) != 0;
                chaseInput.rightOpen = sideOpen(blupi_.GetYaw() - kHalfPi);
                chaseInput.leftOpen = sideOpen(blupi_.GetYaw() + kHalfPi);
                chaseCamera_.Update(chaseInput, dt);
            }
            const float orbitYaw = chaseCamera_.GetYaw() + lookYawOffset_;
            const auto chaseOffset = [&](float e)
            {
                return Microsoft::Xna::Framework::Vector3(-std::sin(orbitYaw) * kChaseDistance * std::cos(e),
                                                          kChaseDistance * std::sin(e),
                                                          std::cos(orbitYaw) * kChaseDistance * std::cos(e));
            };
            // Over what blocks it behind him rather than onto his head (D18).
            // The room at an elevation is also how far the eye still sees his feet: walking down
            // a stair away from the camera, a step's edge passed under the look point (a block over
            // his feet) but hid all of him (092, the play pass of 2026-11-03); the camera then
            // rises over the edge as over a wall behind him, until his whole body shows (seeing
            // only his middle left the edge over his legs, 2026-11-04).
            // His feet are half a block under his position (the model is drawn there,
            // kPlaceholderModelYOffset): a hand over them.
            const Microsoft::Xna::Framework::Vector3 bodyMiddle(blupi_.GetX(), blupi_.GetY() - 0.35f,
                                                                blupi_.GetZ());
            const float chaseElevation = GalaxyEggbert::Game::ChaseCameraElevation(
                elevation, kChaseDistance,
                [&](float e)
                {
                    const auto offset = chaseOffset(e);
                    const float room = RaymarchWallDistance(worldRuntime_.GetWorld(), rawTarget,
                                                            offset * (1.0f / kChaseDistance), kChaseDistance);
                    const Microsoft::Xna::Framework::Vector3 toEye = rawTarget + offset - bodyMiddle;
                    const float length = toEye.Length();
                    if (length < 0.001f || BlocksCamera(worldRuntime_.GetWorld(), bodyMiddle))
                    {
                        return room;
                    }
                    const float seen = RaymarchWallDistance(worldRuntime_.GetWorld(), bodyMiddle,
                                                            toEye * (1.0f / length), length);
                    return std::min(room, kChaseDistance * seen / length);
                });
            Microsoft::Xna::Framework::Vector3 desiredOffset = chaseOffset(chaseElevation);

            // Wall collision (plan.md CAM-005, found 2026-07-17): pull
            // the eye in along the same direction if a solid block
            // sits between Blupi and the desired chase position, so
            // the camera never clips through a wall/tunnel ceiling to
            // end up looking back in from outside.
            const float desiredDistance = desiredOffset.Length();
            float safeDistance = desiredDistance;
            if (desiredDistance > 0.001f)
            {
                const auto direction = desiredOffset * (1.0f / desiredDistance);
                safeDistance = RaymarchWallDistance(worldRuntime_.GetWorld(), rawTarget, direction, desiredDistance);
            }
            // A wall right behind him (a dead end, a narrow turn, a lift in a shaft) pulled the eye
            // into his own body, which is not drawn from inside: he vanished from the picture (the
            // play pass of 2026-11-03, 064, 073, 075, 091, 092, 094, ...). Then the camera looks
            // from the side with the most room instead (beside him, aslant, in front, higher), when
            // that is clearly more; the controls turn him, not the camera, so they stay as they are.
            constexpr float kTooClose = 1.0f;
            if (desiredDistance > 0.001f && safeDistance < kTooClose)
            {
                float bestRoom = safeDistance;
                Microsoft::Xna::Framework::Vector3 bestOffset = desiredOffset;
                for (const float yawOffset : {0.0f, 0.7f, -0.7f, 1.4f, -1.4f, 2.2f, -2.2f, 3.14159265f})
                {
                    for (const float e : {chaseElevation, 0.7f, 1.1f})
                    {
                        const float y = orbitYaw + yawOffset;
                        const Microsoft::Xna::Framework::Vector3 offset(
                            -std::sin(y) * kChaseDistance * std::cos(e), kChaseDistance * std::sin(e),
                            std::cos(y) * kChaseDistance * std::cos(e));
                        const float room = RaymarchWallDistance(worldRuntime_.GetWorld(), rawTarget,
                                                                offset * (1.0f / kChaseDistance), kChaseDistance);
                        if (room > bestRoom + 1e-3f)
                        {
                            bestRoom = room;
                            bestOffset = offset;
                        }
                    }
                }
                constexpr float kClearlyMore = 0.5f;
                if (bestRoom > safeDistance + kClearlyMore)
                {
                    desiredOffset = bestOffset * (desiredDistance / bestOffset.Length());
                    safeDistance = std::min(bestRoom, desiredDistance);
                }
            }
            // Never onto the target itself (Camera::LookAt rejects that): at least a short
            // way back even when a wall stands right behind him (no further, or the eye
            // would be in that wall).
            constexpr float kMinChaseDistance = 0.05f;
            safeDistance = std::max(safeDistance, std::min(kMinChaseDistance, desiredDistance));
            rawEye = rawTarget + desiredOffset * (desiredDistance > 0.001f ? safeDistance / desiredDistance : 1.0f);
            // Pulled in close (a wall behind him, a low passage), the look point drops toward his
            // middle, so he stays in the picture (ChaseCameraYaw.hpp ChaseLookHeight).
            rawTarget.Y = std::min(rawTarget.Y,
                                   blupi_.GetY() + GalaxyEggbert::Game::ChaseLookHeight(safeDistance));
        }

        // Exponential damping toward the raw eye/target computed above
        // (2026-07-09) -- reported live: snapping the camera straight to
        // Blupi's position/facing every frame moves too fast/feels
        // jerky, especially on turns. kCameraDampingPerSecond is the
        // fraction of the remaining distance closed per second; framerate-
        // independent via the standard `1 - exp(-rate * dt)` alpha
        // (not a plain `rate * dt` lerp, which would vary with
        // framerate). Snaps instantly on the very first frame so
        // load-time doesn't start with a lerp-in from the origin.
        constexpr float kCameraDampingPerSecond = 8.0f;
        if (!cameraSmoothedInitialized_)
        {
            cameraEyeSmoothed_ = rawEye;
            cameraTargetSmoothed_ = rawTarget;
            cameraSmoothedInitialized_ = true;
        }
        else
        {
            const float alpha = 1.0f - std::exp(-kCameraDampingPerSecond * dt);
            cameraEyeSmoothed_ = Microsoft::Xna::Framework::Vector3::Lerp(cameraEyeSmoothed_, rawEye, alpha);
            cameraTargetSmoothed_ = Microsoft::Xna::Framework::Vector3::Lerp(cameraTargetSmoothed_, rawTarget, alpha);
            // Up and down it follows within a short lag: dropping a few cells (a ledge, a shaft)
            // he fell out of the picture's bottom before the damped camera came down after him
            // (second play pass of 2026-11-03: 092, 121, 122). The look point keeps within
            // kMaxLookLag of his own, the eye moved with it so the view keeps its slope.
            constexpr float kMaxLookLag = 0.35f;
            const float lag = cameraTargetSmoothed_.Y - rawTarget.Y;
            const float excess = lag > kMaxLookLag ? lag - kMaxLookLag : lag < -kMaxLookLag ? lag + kMaxLookLag : 0.0f;
            cameraTargetSmoothed_.Y -= excess;
            cameraEyeSmoothed_.Y -= excess;
        }
        // The smoothed eye trails the raw one, which the wall check placed: swinging round a
        // corner or a pillar it cut through what the raw eye went round, showing a wall from
        // inside or putting it between the eye and Blupi (the play pass of 2026-11-03: 092,
        // 094, 101, 102, 121, 122, ...). It is kept on this side of any wall along its own line
        // to him, as the raw eye is.
        if (cameraMode_ == CameraMode::ThirdPersonModel)
        {
            const Microsoft::Xna::Framework::Vector3 toEye = cameraEyeSmoothed_ - cameraTargetSmoothed_;
            const float eyeDistance = toEye.Length();
            if (eyeDistance > 0.06f)
            {
                const float safe = RaymarchWallDistance(worldRuntime_.GetWorld(), cameraTargetSmoothed_,
                                                        toEye * (1.0f / eyeDistance), eyeDistance);
                if (safe < eyeDistance)
                {
                    cameraEyeSmoothed_ = cameraTargetSmoothed_ + toEye * (std::max(safe, 0.05f) / eyeDistance);
                }
            }
        }

        // Real screen-shake/forced-pan effect (plan.md CAM-008..013,
        // see CameraShake.hpp's own comment) -- only ticks during
        // real Play (matches the real `m_bPause` freeze gate; no other
        // phase should animate it). Real (dx,dy) is a 2D screen-space
        // scroll-offset in pixels; the closest natural 3D adaptation
        // (CLAUDE.md's allowed "perspective camera" adaptations) is a
        // small bodily camera TRANSLATION (added equally to eye AND
        // target, preserving look direction) along world X/height,
        // using the same 64px-per-world-unit conversion already used
        // throughout this engine's own tile/atlas math.
        if (phase_ == GalaxyEggbert::Def::GamePhase::Play)
        {
            cameraShake_.Update(dt);
        }
        constexpr float kShakePixelsToWorldUnits = 1.0f / 64.0f;
        const Microsoft::Xna::Framework::Vector3 shakeOffset(
            cameraShake_.GetOffsetX() * kShakePixelsToWorldUnits,
            -cameraShake_.GetOffsetY() * kShakePixelsToWorldUnits, // real +Y is screen-down; this engine's +Y is up
            0.0f);
        // An eye on its target (the chase camera pulled all the way in
        // against a wall) has no view direction; the camera keeps the
        // last frame's view instead.
        const Microsoft::Xna::Framework::Vector3 eye = cameraEyeSmoothed_ + shakeOffset;
        const Microsoft::Xna::Framework::Vector3 target = cameraTargetSmoothed_ + shakeOffset;
        if (eye != target)
        {
            camera_.SetLookAt(eye, target);
            cameraTarget_ = target;
        }

        // Third-person placeholder model animation clip (2026-07-09) --
        // advanced regardless of camera mode so switching into
        // third-person mid-animation starts at a sensible position, not
        // always frame 0. Resets on a clip change since clips have very
        // different durations (Walk ~0.7s vs Survey ~3.4s) and
        // continuing from an old position could already be past a
        // shorter new clip's own end.
        if (blupiModelLoaded_)
        {
            std::string clipName = blupiModelIsReal_
                ? BlupiClipName(blupi_)
                : BlupiAnimStateToPlaceholderClipName(blupi_.GetAnimState());
            if (blupiModelIsReal_ && blupiModel_->Clips.find(clipName) == blupiModel_->Clips.end())
            {
                clipName = MissingClipFallback(blupi_);
            }
            // Charred, he stands still in his Stop pose (CharredBlupi.hpp); glued, that pose
            // flattened is the puddle under the glue model.
            const bool charred = BlupiCharredShape().charred || BlupiGlueModel() != nullptr;
            if (charred)
            {
                clipName = "Stop";
            }
            if (clipName != blupiActiveClipName_)
            {
                blupiActiveClipName_ = clipName;
                blupiClipTimeSeconds_ = 0.0;
            }
            blupiClipTimeSeconds_ = charred ? 0.0 : blupiClipTimeSeconds_ + static_cast<double>(simDt);
            // Air and Down stop at their last icon (BlupiController::GetAnimHoldFrame()): the
            // clip's key k sits at k / rate seconds.
            if (const int hold = blupi_.GetAnimHoldFrame(); blupiModelIsReal_ && hold >= 0 && blupiFaceRate_ > 0.0)
            {
                blupiClipTimeSeconds_ = std::min(blupiClipTimeSeconds_, static_cast<double>(hold) / blupiFaceRate_);
            }
        }
        return false;
    }


    // UpdateTickWorld's reactions to what the interactions did this tick: the shakes, the shots, the glue,
    // the crates' sound, the vehicle's motor, a life lost or the mission won, the lifts, the powers granted
    // and run out, the balloon. True when the tick ends here.
    bool GalaxyEggbertGame::UpdateTickEvents(int gameOverCountBeforeUpdate, bool wasBallooned)
    {
        // Real camera shake (plan.md CAM-008/009) -- generic-hazard
        // contact-kill (most types SmallShake, fish/bird BigShake) and
        // the dynamite blast's own center-tile SmallShake, both
        // signaled from InteractionSystem::Update() above.
        if (FindInteractionEvent(InteractionSystem::EventKind::SmallShakeTriggered))
        {
            cameraShake_.Trigger(CameraShakeType::Small);
        }
        if (FindInteractionEvent(InteractionSystem::EventKind::BigShakeTriggered))
        {
            cameraShake_.Trigger(CameraShakeType::Big);
        }

        // Real FireTank recoil anim (plan.md BLUPI-091 gap, table_blupi
        // ID 53, wired 2026-07-19) -- fires only the frame a bullet
        // actually launched (EventKind::TankFired, gated on canFire
        // already requiring Tank mode above), not the empty-clip click.
        if (FindInteractionEvent(InteractionSystem::EventKind::TankFired))
        {
            blupi_.TriggerOneShotAnim(BlupiController::AnimState::FireTank,
                                       BlupiController::kFireTankDuration);
        }
        // The helicopter's glue drop: HelicoGlu holds him while the ball leaves, and the
        // helicopter kicks up as it goes (Decor.cpp:3067, 10 px a tick added: it was the climb's
        // limit, which BD-0070 made 7).
        if (FindInteractionEvent(InteractionSystem::EventKind::GlueDropStarted))
        {
            blupi_.TriggerOneShotAnim(BlupiController::AnimState::HelicoGlu,
                                       BlupiController::kHelicoGluDuration);
        }
        if (FindInteractionEvent(InteractionSystem::EventKind::GlueDropped))
        {
            blupi_.KickUp(BlupiController::kHelicoGluKick);
        }
        // A patrolling enemy passed close by (Decor.cpp:5639-5645): Ouf4 if he stops soon,
        // on foot, out of the water, unshielded.
        if (FindInteractionEvent(InteractionSystem::EventKind::EnemyNearMiss) && !blupi_.IsInVehicle() &&
            !blupi_.IsBallooned() && !blupi_.IsEcrased() && !blupi_.IsNage() && !blupi_.IsSurf() &&
            !blupi_.IsShielded())
        {
            blupi_.ArmOuf(BlupiController::AnimState::Ouf4);
        }

        // The crate push's effort (Decor.cpp:6138/6147 start, `:3637` stop on leaving
        // `GalaxyEggbert::Def::BlupiAction::Push`) -- ch38, played ONCE as the push starts
        // (PlaySound -> PlayImage's bLoop defaults to false) and cut if the push ends first;
        // it was looped here, a sigh repeated for as long as he pushed (BD-0031, owner report
        // 2026-11-02). `CrateBeingPushedThisFrame()` is a plain per-frame fact; comparing it
        // with the previous frame's value gives the start and the end.
        const bool isPushingCrate = interaction_.CrateBeingPushedThisFrame();
        if (isPushingCrate && !wasPushingCrate_)
        {
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel38);
        }
        else if (!isPushingCrate && wasPushingCrate_)
        {
            sound_.Stop(GalaxyEggbert::Def::SoundChannel::SoundChannel38);
        }
        wasPushingCrate_ = isPushingCrate;

        // Real vehicle motor sound crossfade (plan.md SOUND-007/008, found 2026-07-17) --
        // see UpdateVehicleMotorSound()'s own comment. Runs every frame regardless of vehicle
        // state; it's the real per-mode `if`/`else if` chain inside the method itself that
        // decides whether a motor should be playing at all.
        UpdateVehicleMotorSound();

        // Real Win/Lost phase transitions (plan.md HUD-023): Lost
        // fires the instant GameOverCount() increments (real
        // DoorsLost()); Win fires the instant ExitReached() becomes
        // true (real IsTerminated(), already gated on holding every
        // treasure -- see InteractionSystem::Update()'s own exit
        // handling).
        if (interaction_.GameOverCount() > gameOverCountBeforeUpdate)
        {
            // Real MemorizeGamerProgress() checkpoint (2026-07-13,
            // plan.md MENU-040..045) -- confirmed via research to
            // fire automatically at exactly this real transition.
            // Skipped during a play-test session (plan.md EDITOR-108):
            // a sandbox custom world has no real mission number, so
            // persisting one here would corrupt real save progress.
            if (!editorPlayTestActive_)
            {
                saveData_.SetLives(interaction_.Lives());
                saveData_.SetMissionNumber(worldRuntime_.GetMissionNumber());
                saveData_.SetHasProgress(true);
                SavePlayerData();
            }
            SetPhase(GalaxyEggbert::Def::GamePhase::Lost);
        }
        else if (interaction_.ExitReached() && !winCheerActive_)
        {
            recordingResume_ = false;
            if (!replayingResume_) DeleteResumeFile();
            if (!editorPlayTestActive_)
            {
                saveData_.SetLives(interaction_.Lives());
                saveData_.SetMissionNumber(worldRuntime_.GetMissionNumber());
                saveData_.SetHasProgress(true);
                SavePlayerData();
            }
            // The original's exit (Decor.cpp:6160-6180, 6411; Game1::Update): Blupi leaves
            // his helicopter, cheers facing the viewer for 40 ticks and the next world loads
            // (the hub, or the main hub after a golden key) -- no win screen, as the video
            // shows. Only the true ending (199's exit) has the Win screen; play-tests and the
            // showcase keep it to get back to the editor or the menu.
            if (blupi_.IsInVehicle()) blupi_.TriggerDismount();
            // All exits finish the same forty reference ticks, including the ending,
            // showcase and editor play-tests. Those used to bypass the cheer entirely.
            if (blupi_.TriggerOneShotAnim(BlupiController::AnimState::Win, BlupiController::kWinDuration))
            {
                winCheerActive_ = true;
                const float dx = camera_.Position().X - blupi_.GetX();
                const float dz = camera_.Position().Z - blupi_.GetZ();
                blupi_.SetYaw(std::atan2(dx, -dz));
            }
        }
        if (winCheerActive_ && !blupi_.IsOneShotAnimPlaying())
        {
            winCheerActive_ = false;
            const int mission = worldRuntime_.GetMissionNumber();
            if (editorPlayTestActive_ || mission == 199 || mission == 999)
                SetPhase(GalaxyEggbert::Def::GamePhase::Win);
            else
                AdvanceAfterWin();
            return true;
        }

        // Platform lift riding (plan.md E3D-MIG-152): IsRidingLift()
        // reflects whether Blupi was standing on an active lift BEFORE
        // interaction_.Update() just advanced its patrol step -- see
        // InteractionSystem::IsRidingLift()'s own comment for the
        // delta-vs-snap rationale.
        if (interaction_.IsRidingLift())
        {
            blupi_.RideLift(worldRuntime_.GetWorld(), blupi_.GetX() + interaction_.RideDeltaX(),
                            interaction_.RideStandY(), blupi_.GetZ() + interaction_.RideDeltaZ());
        }

        // Secret power grants (plan.md E3D-MIG-170) -- Shield is real,
        // single-stage, instant (TriggerX()'s own internal gate should
        // agree with what InteractionSystem just checked, both read
        // the same GetSecretPower() state). Power/Cloud/Hide (Sucette/
        // Charge/Drink) are real 2-stage pickups -- see
        // ResolvePickupFreeze() below for their own grab/freeze/
        // complete handling (plan.md `173`, 2026-07-14).
        if (FindInteractionEvent(InteractionSystem::EventKind::ShieldGranted) && blupi_.TriggerShield())
        {
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel42);
            // Real m_blupiPosMagic reset (plan.md VISUAL-011-adjacent,
            // Decor.cpp:6022) -- the magic trail's first marker only
            // appears after a further real 40px of movement from here.
            interaction_.ResetMagicTrail(blupi_.GetX(), blupi_.GetY(), blupi_.GetZ());
        }

        // Invert/Mirror grant + expiry (plan.md PICKUP-011) -- real
        // channels 66 (activate) / 67 (expire), confirmed against
        // mobile-eggbert-reference/13-object-pickups.md's "Mirror/
        // Invert" section. No warning-threshold sound (unlike the 4
        // powers above) -- real source has no warning stage for this
        // buff either.
        if (FindInteractionEvent(InteractionSystem::EventKind::InvertGranted) && blupi_.TriggerInvert())
        {
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel66);
            interaction_.SpawnInvertBurst(worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(),
                                           /*isGrant=*/true);
        }
        if (blupi_.JustExpiredInvert())
        {
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel67);
            interaction_.SpawnInvertBurst(worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(),
                                           /*isGrant=*/false);
        }

        // Secret power warning sound (plan.md E3D-MIG-170) -- fires
        // once at the real per-power remaining-level threshold
        // (Shield@10=channel43, Power@20=channel45, Cloud@25=channel56,
        // Hide@20=channel63).
        if (blupi_.JustCrossedSecretPowerWarning())
        {
            switch (blupi_.GetSecretPower())
            {
                case GalaxyEggbert::Def::SecretPower::Shield:
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel43);
                    break;
                case GalaxyEggbert::Def::SecretPower::Power:
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel45);
                    break;
                case GalaxyEggbert::Def::SecretPower::Cloud:
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel56);
                    break;
                case GalaxyEggbert::Def::SecretPower::Hide:
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel63);
                    break;
                default:
                    break;
            }
        }

        // Real respawn/life-loss for hazard-contact deaths (EventKind::Died
        // still fires instantly on contact, for tests/other consumers) is
        // now deferred to the death-lock/life-loss-Voyage system --
        // resolved via ResolveDeathLock() above, not here (death-VFX
        // follow-up, 2026-07-14; this used to respawn instantly).

        // Wasp balloon status (plan.md E3D-MIG-135) -- TriggerBalloon()
        // is idempotent (a no-op while already ballooned, matching the
        // real `!m_blupiBalloon` re-trigger guard), so only play the
        // real entry sound (channel 40) on an actual new trigger.
        // ElectricShake (plan.md CAM-010) fires at this exact same
        // real site (Decor.cpp:5864, confirmed via direct source
        // read) -- despite the enum's own generic "electric field"
        // doc comment, the real trigger IS specifically wasp-sting/
        // balloon entry, not a separate electric-field-tile hazard.
        if (FindInteractionEvent(InteractionSystem::EventKind::BalloonTouched) && blupi_.TriggerBalloon())
        {
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel40);
            cameraShake_.Trigger(CameraShakeType::Electric);
        }
        if (FindInteractionEvent(InteractionSystem::EventKind::BalloonPopped))
        {
            blupi_.PopBalloon();
        }
        // Real recovery sound (channel 41) covers BOTH a natural
        // Step()-driven timeout AND the hazard-triggered pop just above
        // -- both funnel through the same IsBallooned() true->false
        // transition, so one comparison at the end of Update() (not
        // right after Step(), unlike wasEcrased's own check) catches
        // either cause.
        if (wasBallooned && !blupi_.IsBallooned())
        {
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel41);
            // The balloon bursts in a flash (ObjectType91, Decor.cpp:5165-5174, 5768-5777; BD-0054).
            interaction_.SpawnEffect(worldRuntime_, GalaxyEggbert::Def::ObjectType::ObjectType91, blupi_.GetX(), blupi_.GetY(),
                                     blupi_.GetZ());
        }

        return false;
    }


    // UpdateTickScreens in play: the on-screen pad, the mouse look and drag, the zoom, the cheats' keys.
    // True when the tick ends here.
    bool GalaxyEggbertGame::UpdateScreenPlay(const Microsoft::Xna::Framework::Input::KeyboardState& phaseKeys,
                                            const Microsoft::Xna::Framework::Graphics::Viewport& viewport,
                                            GalaxyEggbert::Game::InputPad::PlayInput& padPlayInput, bool& inputPadClaimedMouse,
                                            bool& mousePausePressed)
    {
        using Microsoft::Xna::Framework::Input::ButtonState;
        using Microsoft::Xna::Framework::Input::Keys;
            inputPad_.SetVehicleFireEnabled(blupi_.GetVehicleMode()==BlupiController::VehicleMode::Tank ||
                blupi_.GetVehicleMode()==BlupiController::VehicleMode::Helicopter);
            // Several fingers at once (owner 2026-11-02: touch devices). While fingers are on
            // the screen, and a few frames after, the mouse is left out: the platform also
            // turns a touch into mouse events, which would press Action twice.
            std::vector<GalaxyEggbert::Game::InputPad::TouchPoint> touches;
            using Microsoft::Xna::Framework::Input::Touch::TouchLocationState;
            for (const auto& t : tickInput_.touches)
            {
                const auto state = static_cast<TouchLocationState>(t.state);
                if (state == TouchLocationState::Invalid) continue;
                touches.push_back({t.id, t.x, t.y, state == TouchLocationState::Pressed,
                                   state == TouchLocationState::Released});
            }
            touchQuietFrames_ = touches.empty() ? std::max(0, touchQuietFrames_ - 1) : 15;
            const auto mouse = touchQuietFrames_ > 0 ? Microsoft::Xna::Framework::Input::MouseState{}
                                                     : CurrentMouseState();
            // Invisible desktop controls must not claim clicks intended for the camera.
            if (tickInput_.touchControlsEnabled)
            {
                inputPadClaimedMouse = inputPad_.UpdatePlay(
                    mouse, viewport.getWidthProperty(), viewport.getHeightProperty(), padPlayInput);
            }
            else
            {
                inputPad_.ResetTouchState();
            }
            {
                GalaxyEggbert::Game::InputPad::PlayInput touchInput;
                if (inputPad_.UpdatePlayTouches(touches, viewport.getWidthProperty(),
                                                viewport.getHeightProperty(), touchInput))
                {
                    inputPadClaimedMouse = true;
                }
                padPlayInput.turnInput = std::clamp(padPlayInput.turnInput + touchInput.turnInput, -1.0f, 1.0f);
                padPlayInput.moveInput = std::clamp(padPlayInput.moveInput + touchInput.moveInput, -1.0f, 1.0f);
                padPlayInput.jumpHeld = padPlayInput.jumpHeld || touchInput.jumpHeld;
                padPlayInput.fireHeld = padPlayInput.fireHeld || touchInput.fireHeld;
                padPlayInput.actionPressed = padPlayInput.actionPressed || touchInput.actionPressed;
                padPlayInput.pausePressed = padPlayInput.pausePressed || touchInput.pausePressed;
                padPlayInput.backUpHeld = padPlayInput.backUpHeld || touchInput.backUpHeld;
                padPlayInput.crouchHeld = padPlayInput.crouchHeld || touchInput.crouchHeld;
            }
            mousePausePressed = padPlayInput.pausePressed;

            // Hidden cheat menu (2026-07-13, plan.md CHEAT-001..009):
            // real gesture zones are checked only during real
            // Phase::Play, layered on top of (not instead of) the
            // normal Play controls above -- see InputPad::
            // UpdateCheatGesture()'s own class comment for the real-
            // behavior citation and the "layered, not exclusive"
            // input-sharing caveat.
            if (!cheatMenuShown_)
            {
                if (inputPad_.UpdateCheatGesture(mouse, viewport.getWidthProperty(), viewport.getHeightProperty()))
                {
                    cheatMenuShown_ = true;
                }
            }
            else
            {
                const int cheatPressed = inputPad_.UpdateCheatMenu(
                    mouse, viewport.getWidthProperty(), viewport.getHeightProperty());
                if (cheatPressed != 0)
                {
                    cheatMenuShown_ = false;
                    ApplyCheat(cheatPressed);
                }
            }

            // Real SECOND, independent cheat-entry method (plan.md
            // BLUPI-111, see InputPad::UpdateTypedGhostCheat()'s own
            // comment) -- typing "ghost" toggles Ghost mode, "quick"
            // unlocks F7/F8 (see gameSpeed_'s own member comment),
            // layered on top of (not instead of) the on-screen cheat
            // menu above, same "layered" idiom already established
            // there.
            const auto typedCheat = inputPad_.UpdateTypedGhostCheat(
                phaseKeys, /*isPlayPhase=*/true, tickInput_.cheatLetter);
            ApplyTypedCheat(typedCheat.cheat);
            if (typedCheat.cheat == InputPad::TypedCheat::Help)
            {
                inputPad_.ResetTouchState();
                mouseLookActive_ = false;
                return true;
            }
            if (typedCheat.ghostTyped)
            {
                blupi_.ToggleGhost(worldRuntime_.GetWorld());
            }
            // The persistent ones listed top-left (Mobile Eggbert's InputPad.cpp:842-855: each
            // typing toggles its name; ghost follows the real ghost state).
            {
                using TC = GalaxyEggbert::Game::InputPad::TypedCheat;
                const auto toggle = [this](const char* name) {
                    const auto it = std::find(activeCheats_.begin(), activeCheats_.end(), name);
                    if (it != activeCheats_.end()) activeCheats_.erase(it);
                    else activeCheats_.emplace_back(name);
                };
                switch (typedCheat.cheat)
                {
                    case TC::OpenDoors: toggle("opendoors"); break;
                    case TC::MegaBlupi: toggle("megablupi"); break;
                    case TC::ShowSecret: toggle("showsecret"); break;
                    case TC::Compass: toggle("compass"); break;
                    case TC::Debug: toggle("debug"); break;
                    case TC::Zoom:
                    {
                        // Listed with its step (the view's size as Mobile Eggbert's 100/50/25/12 %).
                        activeCheats_.erase(
                            std::remove_if(activeCheats_.begin(), activeCheats_.end(),
                                           [](const std::string& name) { return name.rfind("zoom", 0) == 0; }),
                            activeCheats_.end());
                        static const char* const kZoomLabels[5] = {"", "zoom 50%", "zoom 25%", "zoom 12%", "zoom 6%"};
                        if (cheatZoomStep_ > 0) activeCheats_.emplace_back(kZoomLabels[cheatZoomStep_]);
                        break;
                    }
                    default: break;
                }
                if (typedCheat.quickTyped) toggle("quick");
                if (typedCheat.ghostTyped)
                {
                    activeCheats_.erase(std::remove(activeCheats_.begin(), activeCheats_.end(), "ghost"),
                                        activeCheats_.end());
                    if (blupi_.IsGhost()) activeCheats_.emplace_back("ghost");
                }
            }
            if (typedCheat.quickTyped)
            {
                quickCheatEnabled_ = !quickCheatEnabled_;
                if (!quickCheatEnabled_ && gameSpeed_ > GalaxyEggbert::Def::GameSpeed::Fast)
                {
                    gameSpeed_ = GalaxyEggbert::Def::GameSpeed::Fast;
                }
            }

            // Real GalaxyEggbert::Def::GameSpeed key mapping (InputPad.cpp:596-684) --
            // F5/F6 always work; F7/F8 need quickCheatEnabled_ (the
            // "quick" typed cheat above). Real source ALSO has a
            // Shift-hold temporary boost, deliberately NOT ported here:
            // both LeftShift and RightShift are already this engine's
            // own crouch/look-up controls (a pre-existing, more central
            // design decision), so there is no free Shift key left for
            // it without conflicting with core movement -- documented
            // gap, not an oversight. Real safety net (InputPad.cpp:
            // 582-594): drop back to Normal if speed is still >Fast
            // with neither quick nor ghost active (this engine has no
            // Shift-boost to also check, per the above).
            if (!quickCheatEnabled_ && !blupi_.IsGhost() && gameSpeed_ > GalaxyEggbert::Def::GameSpeed::Fast)
            {
                gameSpeed_ = GalaxyEggbert::Def::GameSpeed::Normal;
            }
            const bool f5Down = phaseKeys.IsKeyDown(Keys::F5);
            if (f5Down && !f5KeyWasDown_)
            {
                gameSpeed_ = GalaxyEggbert::Def::GameSpeed::Normal;
            }
            f5KeyWasDown_ = f5Down;
            const bool f6Down = phaseKeys.IsKeyDown(Keys::F6);
            if (f6Down && !f6KeyWasDown_)
            {
                gameSpeed_ = GalaxyEggbert::Def::GameSpeed::Fast;
            }
            f6KeyWasDown_ = f6Down;
            if (quickCheatEnabled_)
            {
                const bool f7Down = phaseKeys.IsKeyDown(Keys::F7);
                if (f7Down && !f7KeyWasDown_)
                {
                    gameSpeed_ = GalaxyEggbert::Def::GameSpeed::Faster;
                }
                f7KeyWasDown_ = f7Down;
                const bool f8Down = phaseKeys.IsKeyDown(Keys::F8);
                if (f8Down && !f8KeyWasDown_)
                {
                    gameSpeed_ = GalaxyEggbert::Def::GameSpeed::Fastest;
                }
                f8KeyWasDown_ = f8Down;
            }
            // Real Tab: toggle Slow<->Normal (InputPad.cpp:657-676).
            const bool tabDown = phaseKeys.IsKeyDown(Keys::Tab);
            if (tabDown && !tabKeyWasDown_)
            {
                gameSpeed_ = (gameSpeed_ == GalaxyEggbert::Def::GameSpeed::Slow)
                                 ? GalaxyEggbert::Def::GameSpeed::Normal
                                 : GalaxyEggbert::Def::GameSpeed::Slow;
            }
            tabKeyWasDown_ = tabDown;
            // Real F12: a second, direct way to open/close the cheat
            // overlay (InputPad.cpp:678-683), alongside the existing
            // 10-tap gesture above (layered, not exclusive -- same
            // idiom as the typed-cheat methods).
            const bool f12Down = phaseKeys.IsKeyDown(Keys::F12);
            if (GalaxyEggbert::Game::kCheatsEnabled && !inputPad_.IsCheatEntryLocked() && f12Down && !f12KeyWasDown_)
            {
                cheatMenuShown_ = !cheatMenuShown_;
            }
            f12KeyWasDown_ = f12Down;
        return false;
    }

}
