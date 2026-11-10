#include "GalaxyEggbertGame.hpp"
#include "GameInternal.hpp"
#include "GameHelpers.hpp"
#include "NetGame.hpp"

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

namespace GalaxyEggbert::CNA
{
    using namespace GalaxyEggbert::Game;
    using namespace GalaxyEggbert::CNA::detail;


    GalaxyEggbert::Game::GamepadFrame detail::ReadGamepadFrame()
    {
        return ReadGamepadFrameImpl();
    }

    bool detail::TryGetPatrolIcon(GalaxyEggbert::Def::ObjectType type, bool patrolGoesLeftFromStart, int patrolStep,
                                  int patrolTimeTicks, int& outIcon)
    {
        return TryGetPatrolIconImpl(type, patrolGoesLeftFromStart, patrolStep, patrolTimeTicks, outIcon);
    }

    float detail::GameSpeedFactor(GalaxyEggbert::Def::GameSpeed speed) noexcept
    {
        return GalaxyEggbert::Def::SpeedFactor(speed);
    }

    // GetBackBufferData()'s no-rect overload reads the WHOLE backbuffer, and CNA sizes
    // that read from PresentationParameters.BackBufferWidth/Height. The three one-shot
    // captures below used to size their arrays from Viewport instead. Those are not the
    // same rectangle: EasyGL runs a FixedHeightDynamicWidth virtual resolution, so on a
    // window resize the Viewport becomes the LOGICAL size (height pinned to 480, width
    // following the window's aspect) while the backbuffer stays at its 800x480 virtual
    // resolution. Resize the window narrower than 800:480 mid-startup and the array came
    // out smaller than the backbuffer, so CNA threw "GetBackBufferData: data array too
    // small for requested region" -- an uncaught std::runtime_error, i.e. a hard SIGABRT
    // (user-reported 2026-08-21, reproduced by resizing during startup).
    //
    // Ask for the backbuffer's own size, which is what the call actually reads.
    void detail::GetBackBufferSize(
        const Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int& w, int& h)
    {
        const auto& pp = device.getPresentationParametersProperty();
        w = static_cast<int>(pp.getBackBufferWidthProperty());
        h = static_cast<int>(pp.getBackBufferHeightProperty());
    }

    // A saved world's thumbnail (ED-0013): the editor's view as drawn, the middle of the frame
    // -- a square clear of the palette (left), the status lines (top) and the bar (bottom) --
    // averaged down to 64 x 64, written beside the world (CustomWorldStorage ThumbnailPath).
    void detail::SaveEditorThumbnail(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                     const std::filesystem::path& path)
    {
        using Microsoft::Xna::Framework::Color;
        int w = 0, h = 0;
        GetBackBufferSize(device, w, h);
        if (w <= 0 || h <= 0)
        {
            return;
        }
        std::vector<Color> frame(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), Color(0, 0, 0, 0));
        device.GetBackBufferData(frame.data(), 0, static_cast<int>(frame.size()));
        constexpr int kSize = 64;
        const int side = std::min(w, h * 3 / 4);
        const int x0 = (w - side) / 2;
        const int y0 = std::min(h * 3 / 20, h - side);
        std::vector<Color> small(static_cast<std::size_t>(kSize * kSize), Color(0, 0, 0, 255));
        for (int ty = 0; ty < kSize; ++ty)
        {
            for (int tx = 0; tx < kSize; ++tx)
            {
                unsigned r = 0, g = 0, b = 0, n = 0;
                for (int y = y0 + ty * side / kSize; y < y0 + (ty + 1) * side / kSize; ++y)
                {
                    for (int x = x0 + tx * side / kSize; x < x0 + (tx + 1) * side / kSize; ++x)
                    {
                        const Color& c = frame[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
                                               static_cast<std::size_t>(x)];
                        r += c.getRProperty();
                        g += c.getGProperty();
                        b += c.getBProperty();
                        ++n;
                    }
                }
                if (n != 0)
                {
                    small[static_cast<std::size_t>(ty * kSize + tx)] =
                        Color(static_cast<int>(r / n), static_cast<int>(g / n), static_cast<int>(b / n), 255);
                }
            }
        }
        Microsoft::Xna::Framework::Graphics::Texture2D thumbnail(device, kSize, kSize);
        thumbnail.SetData(small.data(), static_cast<int>(small.size()));
        thumbnail.SaveAsPng(path.string());
    }

    GalaxyEggbertGame::GalaxyEggbertGame()
    {
        Game::getWindowProperty().setTitleProperty("Galaxy Eggbert");
        if (const char* perf = std::getenv("GALAXY_PERF"); perf != nullptr && *perf == '1')
        {
            perfEnabled_ = true;
        }
        if (const char* cull = std::getenv("GALAXY_MODEL_CULL"); cull != nullptr && *cull == '0')
        {
            modelCulling_ = false;
        }
        // Standard XNA pattern: the manager must be constructed and the
        // required profile selected before Run() initializes the device.
        // Galaxy's screenshot/golden-capture diagnostics use
        // GetBackBufferData, which is a HiDef-only XNA operation.  Leaving
        // CNA's default Reach profile selected made the first diagnostic
        // frame terminate both the OpenGL 3.3 and WebGL 2 builds.
        graphics_ = std::make_unique<Microsoft::Xna::Framework::GraphicsDeviceManager>(this);
        graphics_->setGraphicsProfileProperty(
            Microsoft::Xna::Framework::Graphics::GraphicsProfile::HiDef);
        saveData_.Load();
        graphics_->setIsFullScreenProperty(saveData_.GetFullscreen());
        camera_.SetLookAt(Microsoft::Xna::Framework::Vector3(0.0f, 2.0f, 5.0f), cameraTarget_);
        // The network game where CNA has networking (not the web): the menu's Multiplayer.
        inputPad_.SetMultiplayerShown(NetSession::Available());
        keyInputToken_ = Microsoft::Xna::Framework::Input::Keyboard::KeyDownEXT.Add([this](auto key) {
            using Microsoft::Xna::Framework::Input::Keys;
            // A demo or the menu's idle timer sees presses shorter than an update (DemoLiveInput);
            // a demo's typed letters are not cheats.
            if (!SuppressLiveInput() && (demoActive_ || phase_ == GalaxyEggbert::Def::GamePhase::Init))
            {
                if (demoKeyEvents_.size() < 64) demoKeyEvents_.push_back(static_cast<int>(key));
                if (demoActive_) return;
            }
            if (SuppressLiveInput() || controlsHelp_.shown || replayingResume_ ||
                fadeOutPhase_ != GalaxyEggbert::Def::GamePhase::None) return;
            if (phase_ == GalaxyEggbert::Def::GamePhase::Editor)
            {
                if (!worldEditor_.IsBrowsing()) editorCommandKeys_.Record(key);
                else if (worldEditor_.WantsText() && browserTypedKeys_.size() < 64)
                {
                    if (key == Keys::LeftShift || key == Keys::RightShift)
                    {
                        typedShiftPending_ = true;
                        return;
                    }
                    const auto keys = Microsoft::Xna::Framework::Input::Keyboard::GetState();
                    browserTypedKeys_.emplace_back(key, typedShiftPending_ || keys.IsKeyDown(Keys::LeftShift) ||
                                                            keys.IsKeyDown(Keys::RightShift));
                    typedShiftPending_ = false;
                }
                return;
            }
            if (NetWantsText())
            {
                // A line of the network game's chat being typed (the lobby).
                if (key == Keys::LeftShift || key == Keys::RightShift)
                {
                    typedShiftPending_ = true;
                    return;
                }
                const auto keys = Microsoft::Xna::Framework::Input::Keyboard::GetState();
                NetTypedKey(key, typedShiftPending_ || keys.IsKeyDown(Keys::LeftShift) || keys.IsKeyDown(Keys::RightShift));
                typedShiftPending_ = false;
                return;
            }
            if (demoNaming_ && demoSlotsShown_ && browserTypedKeys_.size() < 64)
            {
                // A demo's name being typed (the demo positions over Pause or from the menu).
                if (key == Keys::LeftShift || key == Keys::RightShift)
                {
                    typedShiftPending_ = true;
                    return;
                }
                const auto keys = Microsoft::Xna::Framework::Input::Keyboard::GetState();
                browserTypedKeys_.emplace_back(key, typedShiftPending_ || keys.IsKeyDown(Keys::LeftShift) ||
                                                        keys.IsKeyDown(Keys::RightShift));
                typedShiftPending_ = false;
                return;
            }
            if (!GalaxyEggbert::Game::kCheatsEnabled || phase_ != GalaxyEggbert::Def::GamePhase::Play) return;
            if (key >= Keys::A && key <= Keys::Z && pendingCheatLetters_.size() < 256)
                pendingCheatLetters_.push_back(static_cast<char>('a' + int(key) - int(Keys::A)));
        });
    }

    GalaxyEggbertGame::~GalaxyEggbertGame()
    {
        Microsoft::Xna::Framework::Input::Keyboard::KeyDownEXT.Remove(keyInputToken_);
    }

    void GalaxyEggbertGame::AdvanceAfterWin()
    {
        // Where a won mission leads (Decor.cpp:6411-6434, Game1::Update): the golden key ends
        // the world (Decor::OpenGoldsWin(), m_doors[180 + world]) and goes back to the main hub,
        // where one more gold bar opens and the world's sign changes; any other win opens the
        // next mission's bar in the world's hub (OpenDoorsWin(), m_doors[mission + 1]) and goes
        // there; mission 1's exit leads to 199 (ComputeWinExitTarget()).
        const int currentMission = worldRuntime_.GetMissionNumber();
        if (interaction_.ExitWasGoldenKey() && currentMission != 1 && currentMission != 199)
        {
            saveData_.UnlockMissionDoor(180 + currentMission / 10);
            LoadMission(1);
            return;
        }
        if (currentMission % 10 != 0)
        {
            saveData_.UnlockMissionDoor(currentMission + 1);
        }
        LoadMission(WorldRuntime::ComputeWinExitTarget(currentMission));
    }

    bool GalaxyEggbertGame::EnableAutoplay(const std::string& scriptPath)
    {
        auto autoplay = std::make_unique<GalaxyEggbert::Game::Autoplay>();
        std::string error;
        if (!autoplay->Load(scriptPath, error))
        {
            std::cout << "GalaxyEggbertCNA: --autoplay: " << scriptPath << ": " << error << std::endl;
            return false;
        }
        autoplayTrace_ = std::fopen("autoplay_trace.txt", "wb");
        if (!autoplayTrace_)
        {
            std::cout << "GalaxyEggbertCNA: --autoplay: cannot write autoplay_trace.txt" << std::endl;
            return false;
        }
        autoplay_ = std::move(autoplay);
        // Each Update() steps the game by a fixed 1/60 s (see Update()'s dt), so the run is the
        // same tick for tick however fast it goes; unthrottled it goes as fast as the machine draws.
        // AUTOPLAY_REALTIME=1 keeps the 60 Hz pacing, for watching or screenshotting a run
        // (tools/qa_live.sh with --autoplay).
        const char* realtime = std::getenv("AUTOPLAY_REALTIME");
        setIsFixedTimeStepProperty(realtime != nullptr && std::strcmp(realtime, "1") == 0);
        // AUTOPLAY_SHOTS=N saves the frame every N ticks into autoplay_shots/ (tick-numbered PNGs),
        // so a playthrough can be looked at as the player sees it along its whole route.
        if (const char* shots = std::getenv("AUTOPLAY_SHOTS"))
        {
            autoplayShotEvery_ = std::max(0, std::atoi(shots));
            if (autoplayShotEvery_ > 0)
            {
                std::error_code ignored;
                std::filesystem::create_directories("autoplay_shots", ignored);
            }
        }
        return true;
    }

    void GalaxyEggbertGame::UpdateBodyTilt(float dt)
    {
        constexpr float kDegToRad = 3.14159265358979f / 180.0f;
        constexpr float kTicksPerSecond = 20.0f;
        bodyTiltTicks_ += dt * kTicksPerSecond;
        const float x = blupi_.GetX(), y = blupi_.GetY(), z = blupi_.GetZ();
        const float dx = bodyHasLast_ ? x - bodyLastX_ : 0.0f;
        const float dy = bodyHasLast_ ? y - bodyLastY_ : 0.0f;
        const float dz = bodyHasLast_ ? z - bodyLastZ_ : 0.0f;
        bodyLastX_ = x;
        bodyLastY_ = y;
        bodyLastZ_ = z;
        bodyHasLast_ = true;
        const float approachStep = 10.0f * dt * kTicksPerSecond; // the original's 10 degrees a tick
        const auto approach = [approachStep](float value, float target)
        {
            return value < target ? std::min(value + approachStep, target) : std::max(value - approachStep, target);
        };
        float pitchDegrees = 0.0f;
        if (blupi_.IsNage())
        {
            // Decor.cpp:2276-2312: the direction he moved since the last tick picks the angle the
            // sprite turns to (up 0, up and sideways 45, sideways 90, down and sideways 135, down
            // 180, plus 15), approached 10 degrees a tick; real = that - 90, bobbing 20 degrees.
            constexpr float kMoved = 0.0004f; // cells in a frame
            const bool sideways = std::hypot(dx, dz) > kMoved;
            const int down = dy < -kMoved ? 1 : (dy > kMoved ? -1 : 0); // the original's Y points down
            float target = 0.0f;
            if (down >= 0 || sideways)
            {
                target = (down < 0 && sideways)    ? 45.0f
                         : (down == 0 && sideways) ? 90.0f
                         : (down > 0 && sideways)  ? 135.0f
                         : (down > 0)              ? 180.0f
                                                   : 0.0f;
            }
            target += 15.0f;
            bodySwimLogicDegrees_ = approach(bodySwimLogicDegrees_, target);
            pitchDegrees = bodySwimLogicDegrees_ - 90.0f + std::sin(bodyTiltTicks_ / 6.0f) * 20.0f;
        }
        else if (blupi_.IsSurf())
        {
            // Decor.cpp:2325-2330: the angle settles to 0, bobbing 10 degrees.
            bodySwimLogicDegrees_ = approach(bodySwimLogicDegrees_, 0.0f);
            pitchDegrees = bodySwimLogicDegrees_ + std::sin(bodyTiltTicks_ / 10.0f) * 10.0f;
        }
        else if (blupi_.GetVehicleMode() == BlupiController::VehicleMode::Helicopter)
        {
            // Decor.cpp:2161: rotation = horizontal speed (px a tick) x 2 degrees, leaning into the
            // way it goes (nose down forward); the speed here is cells a second, 20/64 per px a tick.
            pitchDegrees = blupi_.GetVehicleSpeed() * (64.0f / 20.0f) * 2.0f;
        }
        else
        {
            bodySwimLogicDegrees_ = approach(bodySwimLogicDegrees_, 15.0f);
        }
        // A 3D body turned past upright would tumble: held to a quarter turn either way.
        bodyPitchRadians_ = std::clamp(pitchDegrees, -90.0f, 90.0f) * kDegToRad;
    }

    Microsoft::Xna::Framework::Matrix GalaxyEggbertGame::BlupiModelWorld(const CharredShape& charred) const
    {
        constexpr float kPlaceholderModelScale = 0.009095f;
        constexpr float kPlaceholderModelYOffset = -0.5f;
        constexpr float kPlaceholderModelDepthOffset = 0.6f;
        // The real model (kRealModelScale) has its feet at local y = 0
        // like the fox's, so it shares kPlaceholderModelYOffset; it is
        // centred on Blupi, so no depth offset.
        const float modelScale = blupiModelIsReal_ ? kRealModelScale : kPlaceholderModelScale;
        const float modelDepthOffset = blupiModelIsReal_ ? 0.0f : kPlaceholderModelDepthOffset;
        // The real model faces +Z, while GetYaw() moves Blupi along
        // (sin yaw, 0, -cos yaw). CreateRotationY(a) turns +Z to
        // (sin a, 0, cos a), so a = pi - yaw makes him face the way he
        // walks; plain yaw agreed only at +-90 degrees and otherwise
        // walked him sideways or backwards, the symmetric egg body
        // showing the wrong turn as his face sliding around it.
        constexpr float kPi = 3.14159265358979f;
        // Hanging, along the bar's row with his right hand on his pipe (BarRows.hpp).
        const bool hanging = blupi_.IsSuspended() && blupiModelIsReal_;
        const float facingYaw = hanging ? blupi_.GetHangFacingYaw() : blupi_.GetYaw();
        const float modelYaw = blupiModelIsReal_ ? kPi - facingYaw : facingYaw;
        const float hangShift = hanging ? GalaxyEggbert::Game::HangingModelShift(blupi_.GetAnimIcon()) : 0.0f;
        // No render-only Balloon offset here: Blupi's own physics
        // now genuinely lift him while ballooned (BlupiController's
        // kBalloonRiseSpeed, a direct port of the real
        // Decor.cpp:4039-4058 rising block), so GetY() itself already
        // carries the whole effect. An earlier cosmetic sine "bob"
        // added here on 2026-07-18 -- while the physics were still
        // wrongly modelled as a fixed-height freeze -- was an invented
        // stand-in with no real-source basis, and is removed now that
        // the real behaviour is in place (CLAUDE.md's faithful-remake
        // rule: nothing that isn't in mobile-eggbert).
        // Hanging, he is drawn 6 px lower than standing in the bar's cell: the original sets
        // his position 12 px under the bar cell's top (BLUPISUSPEND, Decor.cpp:5442), where
        // standing in a cell puts it 6 px under (BLUPIOFFY); so his hands hold the pipe
        // (owner 2026-11-02: they were above it). Render only, as the original's sprite.
        constexpr float kSuspendModelDrop = 6.0f / 64.0f;
        const float suspendDrop = blupi_.IsSuspended() ? kSuspendModelDrop : 0.0f;
        return
            Microsoft::Xna::Framework::Matrix::CreateScale(modelScale) *
            Microsoft::Xna::Framework::Matrix::CreateScale(charred.spread, charred.height, charred.spread) *
            Microsoft::Xna::Framework::Matrix::CreateTranslation(hangShift, 0.0f, modelDepthOffset) *
            // The jeep tipping over an edge, nose down (+Z is the model's front; BD-0071).
            Microsoft::Xna::Framework::Matrix::CreateRotationX(blupi_.GetVehicleTilt()) *
            // The helicopter's lean and the swimmer's and surfer's turn, about his middle (OR-0080).
            Microsoft::Xna::Framework::Matrix::CreateTranslation(0.0f, -0.4f, 0.0f) *
            Microsoft::Xna::Framework::Matrix::CreateRotationX(bodyPitchRadians_) *
            Microsoft::Xna::Framework::Matrix::CreateTranslation(0.0f, 0.4f, 0.0f) *
            Microsoft::Xna::Framework::Matrix::CreateRotationY(modelYaw) *
            Microsoft::Xna::Framework::Matrix::CreateTranslation(
                blupi_.GetX() + blupiDrawOffset_.X, blupi_.GetY() + kPlaceholderModelYOffset - suspendDrop + blupiDrawOffset_.Y,
                blupi_.GetZ() + blupiDrawOffset_.Z);
    }

    bool GalaxyEggbertGame::FrameRateInterpolates() const noexcept
    {
        // The captures, the demos and the replays step once a loop pass, at its own pace.
        return saveData_.GetFrameRate() > 60 && !autoplay_ && !goldenCaptureMode_ && !goldenTraceMode_ &&
               !demoVerifying_ && !replayingResume_;
    }

    void GalaxyEggbertGame::ApplyFrameRate()
    {
        const bool interpolate = FrameRateInterpolates();
        const int rate = interpolate ? saveData_.GetFrameRate() : 60;
        if (interpolate != frameRateInterpolating_ || interpolate)
        {
            tickClock_ = 0.0;
            drawAlpha_ = 1.0f;
            previousEye_ = camera_.Position();
            previousTarget_ = cameraTarget_;
            previousBlupi_ = Microsoft::Xna::Framework::Vector3(blupi_.GetX(), blupi_.GetY(), blupi_.GetZ());
        }
        frameRateInterpolating_ = interpolate;
        if (!autoplay_)
        {
            setTargetElapsedTimeProperty(System::TimeSpan::FromSeconds(1.0 / static_cast<double>(rate)));
        }
        // Above 60 the screen's refresh must not hold the frames back (most are 60 Hz).
        if (graphics_ && graphics_->getSynchronizeWithVerticalRetraceProperty() != (rate <= 60))
        {
            graphics_->setSynchronizeWithVerticalRetraceProperty(rate <= 60);
            graphics_->ApplyChanges();
        }
        std::cout << "GalaxyEggbertCNA: frame rate " << rate << (interpolate ? " (the steps between interpolated)" : "")
                  << "." << std::endl;
    }

    Microsoft::Xna::Framework::Graphics::Texture2D* GalaxyEggbertGame::BlupiFaceTexture()
    {
        // The real model's face for this frame of the active clip.
        return BlupiFaceFor(blupiActiveClipName_, blupiClipTimeSeconds_);
    }

    Microsoft::Xna::Framework::Graphics::Texture2D* GalaxyEggbertGame::BlupiFaceFor(const std::string& clip, double seconds)
    {
        Microsoft::Xna::Framework::Graphics::Texture2D* face = nullptr;
        if (const auto faces = blupiClipFaces_.find(clip);
            blupiModelIsReal_ && faces != blupiClipFaces_.end() && !faces->second.empty())
        {
            const auto frame = static_cast<std::size_t>(std::max(0.0, seconds) * blupiFaceRate_)
                % faces->second.size();
            const int index = faces->second[frame];
            if (index >= 0 && index < static_cast<int>(blupiFaces_.size()))
            {
                face = &blupiFaces_[static_cast<std::size_t>(index)];
            }
        }

        return face;
    }

    void GalaxyEggbertGame::RecordHideGhost()
    {
        if (cameraMode_ != CameraMode::ThirdPersonModel || !blupiModelLoaded_ ||
            blupi_.IsDeathHidden() || blupi_.IsAnimIconInvisible() || BlupiDrawnAsDeathSprite()) return;
        // Hide's trail (BD-0056): a copy of his pose every 10 px he moves while hidden (the
        // original's Manhattan distance, over X, Y and Z here), each fading from 30 % to
        // nothing over its 20 ticks (UpdateTick ages them; Decor.cpp:5236-5250).
        const bool hidden = blupi_.GetSecretPower() == GalaxyEggbert::Def::SecretPower::Hide;
        if (hidden && !hideGhostTracking_)
        {
            hideGhostTracking_ = true;
            hideGhostX_ = blupi_.GetX();
            hideGhostY_ = blupi_.GetY();
            hideGhostZ_ = blupi_.GetZ();
        }
        hideGhostTracking_ = hidden;
        if (hidden && std::fabs(blupi_.GetX() - hideGhostX_) + std::fabs(blupi_.GetY() - hideGhostY_) +
                              std::fabs(blupi_.GetZ() - hideGhostZ_) >=
                          10.0f / 64.0f)
        {
            hideGhosts_.push_back({BlupiModelWorld(BlupiCharredShape()), blupiActiveClipName_,
                              blupiClipTimeSeconds_, BlupiFaceTexture(), 0.0f});
            hideGhostX_ = blupi_.GetX();
            hideGhostY_ = blupi_.GetY();
            hideGhostZ_ = blupi_.GetZ();
        }
    }

    void GalaxyEggbertGame::CaptureInput()
    {
        using namespace Microsoft::Xna::Framework::Input;
        tickInput_ = {};
        auto keys = autoplay_ ? autoplayKeys_ : ReadKeyboardState(SuppressLiveInput(),
            phase_ != GalaxyEggbert::Def::GamePhase::Editor && phase_ != GalaxyEggbert::Def::GamePhase::Init,
            phase_ == GalaxyEggbert::Def::GamePhase::Play &&
                blupi_.GetVehicleMode()==BlupiController::VehicleMode::Tank);
        if (!SuppressLiveInput() && phase_ == GalaxyEggbert::Def::GamePhase::Editor)
            keys = editorCommandKeys_.Next(keys);
        for (auto key : keys.GetPressedKeys()) tickInput_.keys.push_back(static_cast<int>(key));
        std::sort(tickInput_.keys.begin(), tickInput_.keys.end());
        const auto mouse = ReadMouseState(SuppressLiveInput());
        tickInput_.mouseX = mouse.getXProperty();
        tickInput_.mouseY = mouse.getYProperty();
        tickInput_.wheel = mouse.getScrollWheelValueProperty();
        const ButtonState buttons[] = {mouse.getLeftButtonProperty(), mouse.getMiddleButtonProperty(),
            mouse.getRightButtonProperty(), mouse.getXButton1Property(), mouse.getXButton2Property()};
        for (unsigned i = 0; i < 5; ++i)
            if (buttons[i] == ButtonState::Pressed) tickInput_.buttons |= 1u << i;
        const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
        tickInput_.viewportWidth = viewport.getWidthProperty();
        tickInput_.viewportHeight = viewport.getHeightProperty();
        if (!SuppressLiveInput())
        {
            tickInput_.touchControlsEnabled = Touch::TouchPanel::GetCapabilities().getIsConnectedProperty();
            for (const auto& touch : Touch::TouchPanel::GetState())
                tickInput_.touches.push_back({touch.getIdProperty(), touch.getPositionProperty().X,
                    touch.getPositionProperty().Y, static_cast<int>(touch.getStateProperty())});
        }

        // Physical presses arrive in event order, even if released between updates.
        // They work with a touch screen too, without starting an on-screen keyboard/IME.
        // Autoplay keeps its recorded keyboard-snapshot path.
        if (!SuppressLiveInput() && GalaxyEggbert::Game::kCheatsEnabled)
        {
            tickInput_.cheatLetter = 0;
            if (!pendingCheatLetters_.empty())
            {
                tickInput_.cheatLetter = pendingCheatLetters_.front();
                pendingCheatLetters_.pop_front();
            }
        }
    }

    Microsoft::Xna::Framework::Input::MouseState GalaxyEggbertGame::CurrentMouseState() const
    {
        using namespace Microsoft::Xna::Framework::Input;
        const auto button = [this](unsigned bit) {
            return (tickInput_.buttons & (1u << bit)) ? ButtonState::Pressed : ButtonState::Released;
        };
        return MouseState(tickInput_.mouseX, tickInput_.mouseY, tickInput_.wheel,
                          button(0), button(1), button(2), button(3), button(4));
    }

    void GalaxyEggbertGame::DeleteResumeFile() const noexcept
    {
        // A demo never touches the player's resume file (DemoReplay.hpp).
        if (demoActive_) return;
        std::error_code ignored;
        std::filesystem::remove(ResumeLog::Path(), ignored);
    }

    void GalaxyEggbertGame::OnDeactivated(System::Object* sender, const System::EventArgs& args)
    {
        pendingCheatLetters_.clear();
        editorCommandKeys_.Reset();
        if (demoActive_)
        {
            // A demo is not the player's game: nothing is written or deleted, and the menu returns.
            demoStopRequested_ = true;
            Game::OnDeactivated(sender, args);
            return;
        }
        // Game1.cpp:186-198: only deactivation during Play writes CurrentWrite.
        if (phase_ == GalaxyEggbert::Def::GamePhase::Play && missionStartPending_ &&
            fadeOutPhase_ == GalaxyEggbert::Def::GamePhase::None)
        {
            StartResumeLog();
            if (demoRecordArmed_) BeginDemoRecording(); // as Update() would have, from this start
        }
        if ((replayingResume_ || phase_ == GalaxyEggbert::Def::GamePhase::Play) &&
            recordingResume_ && !missionStartPending_)
        {
            // Keep the complete source log if deactivated while reconstruction is in progress.
            const std::string temporary = ResumeLog::Path() + ".tmp";
            std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
            out << resumeLog_.Serialize();
            out.close();
            std::error_code error;
            if (out) std::filesystem::rename(temporary, ResumeLog::Path(), error);
            if (!out || error)
            {
                std::cout << "GalaxyEggbertCNA: could not write resume log" << std::endl;
                std::filesystem::remove(temporary, error);
                DeleteResumeFile();
            }
        }
        else
        {
            DeleteResumeFile();
        }
        Game::OnDeactivated(sender, args);
    }

    void GalaxyEggbertGame::OnExiting(System::Object* sender, const System::EventArgs& args)
    {
        // A recording being made is written when the window closes (--record-demo).
        FinishDemoRecording();
        // Game1.cpp:205-208: a clean exit deletes CurrentWrite (not during a demo: DeleteResumeFile()).
        DeleteResumeFile();
        Game::OnExiting(sender, args);
    }

    void GalaxyEggbertGame::ResolvePendingVoyage()
    {
        const auto* voyageEvent = FindInteractionEvent(InteractionSystem::EventKind::VoyageRequested);
        if (!voyageEvent)
        {
            return;
        }
        auto viewport = getGraphicsDeviceProperty().getViewportProperty();
        viewport.setWidthProperty(tickInput_.viewportWidth);
        viewport.setHeightProperty(tickInput_.viewportHeight);
        const int viewportW = viewport.getWidthProperty();
        const int viewportH = viewport.getHeightProperty();

        float projectedX = 0.0f, projectedY = 0.0f;
        const bool worldIsStart = voyageEvent->voyageWorldIsStart;
        const bool projected = Hud::ProjectWorldToHudSpace(
            Microsoft::Xna::Framework::Vector3(voyageEvent->voyageWorldX, voyageEvent->voyageWorldY,
                                                voyageEvent->voyageWorldZ),
            camera_.View(), camera_.Projection(), viewportW, viewportH, projectedX, projectedY);
        // Clear2Ascend (real death-VFX coinflip, plan.md `158` follow-up):
        // BOTH endpoints derive from the SAME projected world point (start
        // = Blupi's projected position, end = straight up from there by a
        // fixed HUD-space offset) -- there is no independent "fixed" point
        // to fall back to if projection fails, so an un-projectable
        // ascend request is simply dropped (a dying Blupi behind the
        // camera is not a real scenario this needs to handle robustly).
        if (voyageEvent->voyageIsAscend)
        {
            if (projected)
            {
                interaction_.BeginVoyage(worldRuntime_, voyageEvent->voyageKind, voyageEvent->voyageIconId, false,
                                         projectedX, projectedY, projectedX,
                                         projectedY - voyageEvent->voyageAscendOffsetY, sound_,
                                         voyageEvent->voyageWorldX, voyageEvent->voyageWorldY,
                                         voyageEvent->voyageWorldZ);
            }
            return;
        }

        if (!projected)
        {
            // Behind the camera (shouldn't happen for a just-touched
            // pickup) -- fall back to collapsing this endpoint onto the
            // OTHER (fixed) one, giving a real total=0 (the reward applies
            // on the very next tick) rather than leaving the voyage stuck.
            projectedX = voyageEvent->voyageFixedX;
            projectedY = voyageEvent->voyageFixedY;
        }

        const float startX = worldIsStart ? projectedX : voyageEvent->voyageFixedX;
        const float startY = worldIsStart ? projectedY : voyageEvent->voyageFixedY;
        const float endX = worldIsStart ? voyageEvent->voyageFixedX : projectedX;
        const float endY = worldIsStart ? voyageEvent->voyageFixedY : projectedY;
        interaction_.BeginVoyage(worldRuntime_, voyageEvent->voyageKind, voyageEvent->voyageIconId,
                                 voyageEvent->voyageIsButtonChannel, startX, startY, endX, endY, sound_);
    }


    void GalaxyEggbertGame::DismountAndDepositVehicle()
    {
        if (!blupi_.IsInVehicle())
        {
            return;
        }
        const GalaxyEggbert::Def::ObjectType depositType = VehicleModeToObjectType(blupi_.GetVehicleMode());
        MobileObjSpec deposited;
        deposited.type = depositType;
        deposited.active = true;
        deposited.posStartX = deposited.posEndX = deposited.currentX = blupi_.GetX();
        deposited.posStartY = deposited.posEndY = deposited.currentY = blupi_.GetY();
        deposited.posStartZ = deposited.posEndZ = deposited.currentZ = blupi_.GetZ();
        // Real DeposeSkate anim (plan.md BLUPI-091 gap, table_blupi ID 43, wired 2026-07-19) --
        // Skateboard is the only vehicle with a dedicated dismount pose. Its board goes down on
        // the clip's tick 12 (Decor.cpp:4527-4533); the others appear at once (ObjectStart in the
        // same tick as the dismount).
        if (blupi_.GetVehicleMode() == BlupiController::VehicleMode::Skateboard)
        {
            blupi_.TriggerOneShotAnim(BlupiController::AnimState::DeposeSkate,
                                       BlupiController::kDeposeSkateDuration);
            blupi_.TriggerDismount();
            pendingSkate_ = {};
            pendingSkate_.active = true;
            pendingSkate_.take = false;
            pendingSkate_.seconds = 12.0f / 20.0f;
            pendingSkate_.deposit = deposited;
            return;
        }
        helicopterDeposited_ = blupi_.GetVehicleMode() == BlupiController::VehicleMode::Helicopter;
        blupi_.TriggerDismount();
        PlaceMobileObject(worldRuntime_, deposited);
    }

    float GalaxyEggbertGame::AccelTurnInput()
    {
        // A replay (resume or demo) reads the recorded accelerometer, never the device.
        if (!replayingResume_ && !demoActive_)
        {
            using Microsoft::Devices::Sensors::Accelerometer;
            // Whether there is one, asked once: CNA asks the platform each time (on Android SDL lists
            // the device's sensors), and this ran every tick.
            if (saveData_.GetAccelActive() && !autoplay_ && !accelSupportProbed_)
            {
                accelSupportProbed_ = true;
                accelSupported_ = Accelerometer::getIsSupportedProperty();
                std::cout << "GalaxyEggbertCNA: accelerometer " << (accelSupported_ ? "present" : "not present")
                          << "." << std::endl;
            }
            const bool wanted = saveData_.GetAccelActive() && !autoplay_ && accelSupported_;
            if (wanted && !accel_)
            {
                try
                {
                    accel_ = std::make_unique<Accelerometer>();
                    accel_->Start();
                }
                catch (const std::exception& error)
                {
                    std::cout << "GalaxyEggbertCNA: accelerometer did not start: " << error.what() << std::endl;
                    accel_.reset(); // no permission or no sensor after all (the original's StartAccel)
                    accelSupported_ = false;
                    return 0.0f;
                }
                catch (...)
                {
                    std::cout << "GalaxyEggbertCNA: accelerometer did not start." << std::endl;
                    accel_.reset();
                    accelSupported_ = false;
                    return 0.0f;
                }
                std::cout << "GalaxyEggbertCNA: accelerometer started." << std::endl;
                accelTicksWithoutData_ = 0;
                accelReported_ = false;
                tickInput_.accelReset = !resumeAccelWarmStart_;
                resumeAccelWarmStart_ = false;
            }
            else if (!wanted && accel_)
            {
                accel_->Stop();
                accel_.reset();
                resumeAccelWarmStart_ = false;
            }
            if (accel_ && accel_->getIsDataValidProperty())
            {
                tickInput_.accelValid = true;
                const auto acceleration = accel_->getCurrentValueProperty().getAccelerationProperty();
                // The original read Windows Phone's Y: the device's long side, fixed to it, which in the
                // landscape the game is held in runs left to right, so tilting the device like a wheel
                // moves it. CNA hands Android's reading in the landscape screen's frame instead (x right,
                // y up, z out of the screen; the device turned either way), where that is x, and y is how
                // far the screen leans back (about 0.5 g as one holds it, which never let it turn). Its
                // sign is the original's: the right side lowered reads x < 0, a turn right.
#if defined(__ANDROID__)
                tickInput_.accelY = acceleration.X;
#else
                tickInput_.accelY = acceleration.Y;
#endif
                if (!accelReported_)
                {
                    accelReported_ = true;
                    std::cout << "GalaxyEggbertCNA: accelerometer reading (" << acceleration.X << ", " << acceleration.Y
                              << ", " << acceleration.Z << ") g, turning by " << tickInput_.accelY << "." << std::endl;
                }
            }
            else if (accel_ && ++accelTicksWithoutData_ == 180)
            {
                std::cout << "GalaxyEggbertCNA: accelerometer started but no reading in 3 s." << std::endl;
            }
        }
        if (tickInput_.accelReset)
        {
            accelLastState_ = false;
            accelWaitZero_ = true;
        }
        if (!tickInput_.accelValid) return 0.0f;
        // HandleAccelSensorCurrentValueChanged: a dead zone from the sensitivity (GameData data[7],
        // 50 by default; the Setup's slider), narrower while already tilted, a speed from a quarter
        // up; nothing until the device has once been level (accelWaitZero).
        const float kSensitivity = static_cast<float>(saveData_.GetAccelSensitivity()) / 100.0f;
        const float y = tickInput_.accelY;
        const float threshold = (1.0f - kSensitivity) * 0.06f + 0.04f;
        const float adjusted = accelLastState_ ? threshold * 0.6f : threshold;
        float speed = 0.0f;
        if (y > adjusted)
        {
            speed = -std::min(y * 0.25f / threshold + 0.25f, 1.0f);
        }
        else if (y < -adjusted)
        {
            speed = std::min(-y * 0.25f / threshold + 0.25f, 1.0f);
        }
        const bool started = speed != 0.0f && !accelLastState_;
        accelLastState_ = speed != 0.0f;
        if (accelWaitZero_)
        {
            if (speed == 0.0f)
            {
                accelWaitZero_ = false;
                if (!replayingResume_ && !demoActive_)
                {
                    std::cout << "GalaxyEggbertCNA: accelerometer level (" << y << " g): tilting turns." << std::endl;
                }
            }
            return 0.0f;
        }
        if (started && !replayingResume_ && !demoActive_ && accelTurnsReported_ < 4)
        {
            ++accelTurnsReported_;
            std::cout << "GalaxyEggbertCNA: accelerometer turns " << (speed > 0.0f ? "right" : "left") << " (" << y
                      << " g)." << std::endl;
        }
        return speed;
    }

    GalaxyEggbert::Def::SecretPower GalaxyEggbertGame::BlupiAuraShown() const
    {
        using GalaxyEggbert::Def::SecretPower;
        const SecretPower power = blupi_.GetSecretPower();
        if (power != SecretPower::Shield && power != SecretPower::Power && power != SecretPower::Cloud)
        {
            return SecretPower::None;
        }
        if (cameraMode_ != CameraMode::ThirdPersonModel || blupi_.IsDeathLocked() || blupi_.IsTeleporting())
        {
            return SecretPower::None;
        }
        // m_blupiTimeShield > 25 || (m_time / ScaleDiv(1)) % 4 < 2
        const bool shown = blupi_.GetTimeShield() > 25 || worldRuntime_.GetAnimPhase() % 4 < 2;
        return shown ? power : SecretPower::None;
    }

    bool GalaxyEggbertGame::BlupiDrawnAsDeathSprite() const
    {
        // Clear1 (lightning, mines, dynamite: charred, crumbling to smoking ash), Clear3 (lava)
        // and Glu (glue) are drawn from element.png in the original (Decor.cpp:2403-2410), not
        // from Blupi's sheet; the 3D clips built from those icon numbers took Blupi-sheet icons
        // instead -- 46/47 are his raised-arms wave, so he seemed to cheer as he died (OR-0015).
        // The charred frames are his model now (BlupiCharredShape()); the original's sprite stands
        // in for the rest (Glu's).
        return cameraMode_ == CameraMode::ThirdPersonModel && blupiModelIsReal_ &&
               blupi_.GetAnimState() == BlupiController::AnimState::DeathLocked &&
               blupi_.AnimIconUsesElementSheet() && !blupi_.IsAnimIconInvisible() && !blupi_.IsDeathHidden() &&
               !BlupiCharredShape().charred && BlupiGlueModel() == nullptr;
    }

    const GalaxyEggbert::Game::ObjectModelPose* GalaxyEggbertGame::BlupiGlueModel() const
    {
        if (cameraMode_ != CameraMode::ThirdPersonModel || !blupiModelIsReal_ ||
            blupi_.GetAnimState() != BlupiController::AnimState::DeathLocked ||
            blupi_.GetDeathCause() != BlupiController::DeathCause::Glu || !blupi_.AnimIconUsesElementSheet() ||
            blupi_.IsAnimIconInvisible() || blupi_.IsDeathHidden())
        {
            return nullptr;
        }
        return objectModelTable_.Find(ObjectTextureSource::Element, blupi_.GetRawAnimIcon());
    }

    GalaxyEggbert::Game::CharredShape GalaxyEggbertGame::BlupiCharredShape() const
    {
        if (cameraMode_ != CameraMode::ThirdPersonModel || !blupiModelIsReal_ ||
            blupi_.GetAnimState() != BlupiController::AnimState::DeathLocked ||
            !blupi_.AnimIconUsesElementSheet() || blupi_.IsDeathHidden())
        {
            return {};
        }
        return GalaxyEggbert::Game::CharredModelShape(blupi_.GetRawAnimIcon());
    }

    void GalaxyEggbertGame::UpdateMockerySound(float dt)
    {
        using AnimState = GalaxyEggbert::Game::BlupiController::AnimState;
        if (blupi_.MockeryStarted())
        {
            // A taunt began this frame (one may follow another at once: Mockeryp has no cooldown).
            const auto channel = mockeryVariant_ == AnimState::Mockeryp ? GalaxyEggbert::Def::SoundChannel::SoundChannel47
                                                                        : GalaxyEggbert::Def::SoundChannel::SoundChannel65;
            if (mockeryVariant_ != AnimState::Stop && mockerySounded_)
            {
                sound_.Stop(channel);
            }
            mockeryVariant_ = blupi_.GetAnimState();
            mockeryElapsed_ = 0.0f;
            mockerySounded_ = false;
        }
        if (mockeryVariant_ == AnimState::Stop)
        {
            return;
        }
        // Mockery and Mockeryi play channel 65 (at ticks 1 and 6), Mockeryp channel 47 (at tick 4),
        // Decor.cpp:3147-3183; leaving the action stops it (6457-6470).
        const auto channel = mockeryVariant_ == AnimState::Mockeryp ? GalaxyEggbert::Def::SoundChannel::SoundChannel47
                                                                    : GalaxyEggbert::Def::SoundChannel::SoundChannel65;
        if (blupi_.GetAnimState() != mockeryVariant_)
        {
            if (mockerySounded_)
            {
                sound_.Stop(channel);
            }
            mockeryVariant_ = AnimState::Stop;
            return;
        }
        const float tick = mockeryVariant_ == AnimState::Mockery ? 1.0f : mockeryVariant_ == AnimState::Mockeryi ? 6.0f : 4.0f;
        mockeryElapsed_ += dt;
        if (!mockerySounded_ && mockeryElapsed_ * 20.0f >= tick)
        {
            sound_.Play(channel);
            mockerySounded_ = true;
        }
    }

    void GalaxyEggbertGame::UpdateMusic()
    {
        using Microsoft::Xna::Framework::Media::MediaPlayer;
        using Microsoft::Xna::Framework::Media::Song;
        using GalaxyEggbert::Def::GamePhase;
        const bool inWorld = phase_ == GamePhase::Play || phase_ == GamePhase::Pause ||
                             phase_ == GamePhase::PlaySetup ||
                             (phase_ == GamePhase::Editor && !worldEditor_.IsBrowsing());
        const std::uint32_t wanted =
            inWorld && sound_.IsEnabled() && !autoplay_ && !replayingResume_ && !demoVerifying_
                ? worldRuntime_.GetWorld().musicTrack() : 0u;
        if (wanted == musicPlaying_)
        {
            return;
        }
        if (music_)
        {
            MediaPlayer::Stop();
            music_.reset();
        }
        musicPlaying_ = wanted;
        if (wanted == 0)
        {
            return;
        }
        char path[48];
        std::snprintf(path, sizeof(path), "Content/music/music%03u.ogg", wanted - 1u);
#if defined(__EMSCRIPTEN__)
        // The web build does not pack the music (27 MB, OR-0014): it lies beside the page in
        // Content/music/ and a track is fetched into the virtual filesystem the first time a world
        // asks for it; until it is there, nothing plays and the next frame asks again.
        if (!std::filesystem::exists(path))
        {
            musicPlaying_ = 0;
            if (musicRequested_.insert(wanted).second)
            {
                std::error_code error;
                std::filesystem::create_directories("Content/music", error);
                emscripten_async_wget(
                    path, path, [](const char* file) { std::cout << "GalaxyEggbert: fetched " << file << std::endl; },
                    [](const char* file) { std::cout << "GalaxyEggbert: could not fetch " << file << std::endl; });
            }
            return;
        }
#endif
        try
        {
            music_.reset(Song::FromUri("music", std::string(path)));
            MediaPlayer::setIsRepeatingProperty(true);
            ApplyMusicVolume();
            MediaPlayer::Play(music_.get());
            std::cout << "GalaxyEggbert: music " << path << " playing." << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cout << "GalaxyEggbert: music " << path << " not played: " << e.what() << std::endl;
            music_.reset();
        }
    }

    void GalaxyEggbertGame::ApplyMusicVolume()
    {
        // Half volume under the effects (OR-0014's mix), scaled by the Setup's music slider.
        Microsoft::Xna::Framework::Media::MediaPlayer::setVolumeProperty(
            0.5f * static_cast<float>(saveData_.GetMusicVolume()) / 100.0f);
    }

    void GalaxyEggbertGame::WreckVehicle()
    {
        if (!blupi_.IsInVehicle())
        {
            return;
        }
        interaction_.SpawnWreck(worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ());
        blupi_.TriggerDismount();
        cameraShake_.Trigger(CameraShakeType::Small);
        sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel10);
    }

    void GalaxyEggbertGame::TickPendingSkate(float dt)
    {
        if (!pendingSkate_.active)
        {
            return;
        }
        pendingSkate_.seconds -= dt;
        if (pendingSkate_.seconds > 0.0f)
        {
            return;
        }
        pendingSkate_.active = false;
        auto& objects = worldRuntime_.GetMobileObjectsMutable();
        if (pendingSkate_.take)
        {
            if (pendingSkate_.objectIndex < objects.size() &&
                objects[pendingSkate_.objectIndex].type == GalaxyEggbert::Def::ObjectType::ObjectType24)
            {
                objects[pendingSkate_.objectIndex].active = false;
            }
        }
        else
        {
            PlaceMobileObject(worldRuntime_, pendingSkate_.deposit);
        }
    }

    void GalaxyEggbertGame::StopVehicleMotorSound()
    {
        using GalaxyEggbert::Def::SoundChannel;
        for (const auto channel : {SoundChannel::SoundChannel15, SoundChannel::SoundChannel16,
                                   SoundChannel::SoundChannel17, SoundChannel::SoundChannel18,
                                   SoundChannel::SoundChannel28, SoundChannel::SoundChannel29,
                                   SoundChannel::SoundChannel30, SoundChannel::SoundChannel31})
            sound_.Stop(channel);
        activeMotorLoop_ = SoundChannel::SoundChannel0;
    }

    void GalaxyEggbertGame::UpdateVehicleMotorSound()
    {
        using GalaxyEggbert::Def::SoundChannel;

        GalaxyEggbert::Def::SoundChannel desiredLoop = GalaxyEggbert::Def::SoundChannel::SoundChannel0;
        GalaxyEggbert::Def::SoundChannel startSound = GalaxyEggbert::Def::SoundChannel::SoundChannel0;
        GalaxyEggbert::Def::SoundChannel stopSound = GalaxyEggbert::Def::SoundChannel::SoundChannel0;
        switch (blupi_.GetVehicleMode())
        {
            case BlupiController::VehicleMode::Helicopter:
                desiredLoop = blupi_.IsVehicleMotorHigh() ? GalaxyEggbert::Def::SoundChannel::SoundChannel16 : GalaxyEggbert::Def::SoundChannel::SoundChannel18;
                startSound = GalaxyEggbert::Def::SoundChannel::SoundChannel15;
                stopSound = GalaxyEggbert::Def::SoundChannel::SoundChannel17;
                break;
            case BlupiController::VehicleMode::Jeep:
            case BlupiController::VehicleMode::Tank:
            case BlupiController::VehicleMode::Overcraft:
                desiredLoop = blupi_.IsVehicleMotorHigh() ? GalaxyEggbert::Def::SoundChannel::SoundChannel29 : GalaxyEggbert::Def::SoundChannel::SoundChannel31;
                startSound = GalaxyEggbert::Def::SoundChannel::SoundChannel28;
                stopSound = GalaxyEggbert::Def::SoundChannel::SoundChannel30;
                break;
            default:
                break; // None/Skateboard: no motor sound in real source either
        }

        if (activeMotorLoop_ == desiredLoop)
        {
            return;
        }
        if (activeMotorLoop_ == GalaxyEggbert::Def::SoundChannel::SoundChannel0 && desiredLoop != GalaxyEggbert::Def::SoundChannel::SoundChannel0)
        {
            sound_.Play(startSound);
        }
        if (activeMotorLoop_ != GalaxyEggbert::Def::SoundChannel::SoundChannel0 && desiredLoop == GalaxyEggbert::Def::SoundChannel::SoundChannel0)
        {
            sound_.Play(stopSound);
        }
        if (activeMotorLoop_ != GalaxyEggbert::Def::SoundChannel::SoundChannel0)
        {
            sound_.Stop(activeMotorLoop_);
        }
        activeMotorLoop_ = desiredLoop;
        if (activeMotorLoop_ != GalaxyEggbert::Def::SoundChannel::SoundChannel0)
        {
            sound_.Play(activeMotorLoop_, /*loop=*/true);
        }
    }

    void GalaxyEggbertGame::ResolveDeathLock()
    {
        // Starts a NEW lock for the 4 real trigger sites living inside
        // interaction_.Update() itself (see InteractionSystem::
        // EventKind::DeathLockRequested's own comment for the exact
        // shouldRespawn/PendingDeathKind->DeathCause mapping).
        if (const auto* deathLockEvent = FindInteractionEvent(InteractionSystem::EventKind::DeathLockRequested))
        {
            using GalaxyEggbert::Game::BlupiController;
            using GalaxyEggbert::Game::InteractionSystem;
            const InteractionSystem::PendingDeathKind pendingKind = deathLockEvent->deathLockKind;
            const BlupiController::DeathCause cause = (pendingKind == InteractionSystem::PendingDeathKind::Clear1)
                                                             ? BlupiController::DeathCause::Clear1
                                                         : (pendingKind == InteractionSystem::PendingDeathKind::Clear2)
                                                             ? BlupiController::DeathCause::Clear2
                                                             : BlupiController::DeathCause::Glu;
            blupi_.TriggerDeathLock(cause, deathLockEvent->deathLockShouldRespawn);
        }

        // Resolves an ALREADY-active lock (possibly started a prior frame)
        // that just elapsed -- see TriggerDeathLock()'s own comment for the
        // real mechanic (respawn, then the life-loss Voyage; or game over
        // with no Voyage at all if this was Blupi's last life).
        bool shouldRespawn = false;
        if (!blupi_.ConsumeDeathLockResolved(shouldRespawn))
        {
            return;
        }
        // In a network game every death starts again where he last stood safely (decblupi.cpp:4078,
        // `if (m_blupiRestart || m_bMulti)`).
        if (shouldRespawn || NetMatchActive())
        {
            PlaceBlupi(blupi_.GetValidX(), blupi_.GetValidY(), blupi_.GetValidZ());
        }
        // Real `if (m_nbVies > 0) { ...Voyage... } else { ...DoorsLost(), no
        // Voyage... }` (Decor.cpp:6382-6392) -- predict which branch BEFORE
        // calling anything that decrements, since LoseLife() itself is what
        // performs the decrement (inside BeginVoyage()'s own LifeLoss case
        // below, matching real VoyageInit's inline `m_nbVies--`).
        // The lives row shows the lives in reserve: with none left this death ends the game; with
        // one or more, one of them flies to him and he plays on (Decor.cpp:6379-6396).
        if (interaction_.Lives() <= 0)
        {
            interaction_.LoseLife();
            return;
        }

        auto viewport = getGraphicsDeviceProperty().getViewportProperty();
        viewport.setWidthProperty(tickInput_.viewportWidth);
        viewport.setHeightProperty(tickInput_.viewportHeight);
        float endX = 0.0f, endY = 0.0f;
        const bool projected = Hud::ProjectWorldToHudSpace(
            Microsoft::Xna::Framework::Vector3(blupi_.GetX(), blupi_.GetY(), blupi_.GetZ()), camera_.View(),
            camera_.Projection(), viewport.getWidthProperty(), viewport.getHeightProperty(), endX, endY);
        // Real `VoyageGetPosVie(m_nbVies)` uses the PRE-decrement life count (evaluated as a call
        // argument before VoyageInit's body runs) -- kLivesX/Y/Step match Hud.cpp's own
        // constants exactly (already reused this way for Egg's own dynamic end point in `158`).
        constexpr float kLivesX = 210.0f, kLivesY = 417.0f, kLivesStep = 16.0f;
        const float startX = kLivesX + kLivesStep * static_cast<float>(interaction_.Lives());
        if (!projected)
        {
            // Behind the camera -- collapse onto the start point, same
            // fallback idiom as ResolvePendingVoyage() above.
            endX = startX;
            endY = kLivesY;
        }
        interaction_.BeginVoyage(worldRuntime_, GalaxyEggbert::Game::InteractionSystem::VoyageKind::LifeLoss, 48,
                                 false, startX, kLivesY, endX, endY, sound_);
        // PK_DIE with the lives left (decblupi.cpp:4086), after VoyageInit's decrement.
        NetSendDie(interaction_.Lives());
    }

    const GalaxyEggbert::Game::InteractionSystem::Event* GalaxyEggbertGame::FindInteractionEvent(
        GalaxyEggbert::Game::InteractionSystem::EventKind kind) const noexcept
    {
        for (const auto& event : interaction_.EventsThisFrame())
        {
            if (event.kind == kind)
            {
                return &event;
            }
        }
        return nullptr;
    }

    void GalaxyEggbertGame::ResolvePickupFreeze()
    {
        using GalaxyEggbert::Game::BlupiController;
        using GalaxyEggbert::Game::InteractionSystem;
        using GalaxyEggbert::Def::ObjectType;
        using GalaxyEggbert::Def::SoundChannel;

        // Finish the previous action before accepting another pickup. Death cancels
        // the controller's completion event, but must still put its reusable item back.
        BlupiController::PickupFreezeKind kind{};
        if (blupi_.ConsumePickupFreezeResolved(kind))
        {
            switch (kind)
            {
                case BlupiController::PickupFreezeKind::Sucette:
                    blupi_.TriggerPower();
                    sound_.Play(SoundChannel::SoundChannel44);
                    interaction_.ResetMagicTrail(blupi_.GetX(), blupi_.GetY(), blupi_.GetZ());
                    break;
                case BlupiController::PickupFreezeKind::Drink:
                    blupi_.TriggerHide();
                    sound_.Play(SoundChannel::SoundChannel62);
                    break;
                case BlupiController::PickupFreezeKind::Charge:
                    blupi_.TriggerCloud();
                    sound_.Play(SoundChannel::SoundChannel55);
                    break;
            }
        }
        reusablePickup_.RestoreIfReady(worldRuntime_, interaction_, blupi_.IsPickupFrozen());

        // Taking dynamite: TakeDynamite for 18 ticks without focus (Decor.cpp:6116-6128).
        if (FindInteractionEvent(InteractionSystem::EventKind::DynamiteTaken))
            blupi_.TriggerOneShotAnim(BlupiController::AnimState::TakeDynamite, BlupiController::kTakeDynamiteDuration);

        const auto start = [&](InteractionSystem::EventKind eventKind, BlupiController::PickupFreezeKind pickupKind,
                               ObjectType type, SoundChannel sound)
        {
            const auto* event = FindInteractionEvent(eventKind);
            if (!event) return false;
            reusablePickup_ = {event->pickupX, event->pickupY, event->pickupZ, type};
            if (blupi_.TriggerPickupFreeze(pickupKind))
            {
                sound_.Play(sound);
                // Charge grants Cloud at contact too (Decor.cpp:6069-6087).
                if (pickupKind == BlupiController::PickupFreezeKind::Charge) blupi_.TriggerCloud();
            }
            // Also covers a hit in the very frame the item was taken: the action
            // cannot start while death-locked, yet the interaction already hid the item.
            reusablePickup_.RestoreIfReady(worldRuntime_, interaction_, blupi_.IsPickupFrozen());
            return true;
        };
        if (!start(InteractionSystem::EventKind::PowerGranted, BlupiController::PickupFreezeKind::Sucette,
                   ObjectType::ObjectType26, SoundChannel::SoundChannel50) &&
            !start(InteractionSystem::EventKind::HideGranted, BlupiController::PickupFreezeKind::Drink,
                   ObjectType::ObjectType30, SoundChannel::SoundChannel57))
            start(InteractionSystem::EventKind::CloudGranted, BlupiController::PickupFreezeKind::Charge,
                  ObjectType::ObjectType31, SoundChannel::SoundChannel58);
    }

    GalaxyEggbertGame::ParkedVehicleDraw GalaxyEggbertGame::ParkedVehicleModel(
        GalaxyEggbert::Def::ObjectType type) const
    {
        if (!blupiModelIsReal_)
        {
            return {};
        }
        for (const auto& parked : kParkedVehicles)
        {
            if (parked.type != type)
            {
                continue;
            }
            for (const auto& [name, model] : blupiVehicles_)
            {
                if (name == parked.model && model->Clips.count(parked.clip) != 0)
                {
                    return {model.get(), parked.clip};
                }
            }
        }
        return {};
    }

    bool GalaxyEggbertGame::ObjectSprite(const MobileObjSpec& obj, ObjectTextureSource& sheet, int& icon) const
    {
        const auto visual = ResolveObjectVisual(obj.type, static_cast<int>(obj.phase), obj.visualIcon);
        if (!obj.active || !visual.visible || visual.renderMode != ObjectRenderMode::Billboard ||
            ParkedVehicleModel(obj.type).model != nullptr)
        {
            return false;
        }
        sheet = visual.textureSource;
        icon = visual.icon;
        // The patrol enemies' per-direction/turn icon tables (see the
        // element.png billboard pass, the one place with their patrol state).
        // On element.png, and blupi1.png for the enemy Blupis (OR-0063).
        int patrolIcon = 0;
        if (TryGetPatrolIcon(obj.type, obj.posStartX > obj.posEndX, obj.patrolStep,
                             static_cast<int>(obj.patrolTime), patrolIcon))
        {
            icon = patrolIcon;
        }
        return true;
    }

    int GalaxyEggbertGame::CellQuarterTurns(float x, float y, float z) const
    {
        const long rx = std::lround(x) + WorldRuntime::kWorldCenterX;
        const long ry = std::lround(y);
        const long rz = std::lround(z) + WorldRuntime::kWorldCenterZ;
        const long n = static_cast<long>(worldRuntime_.GetWorld().blocksPerAxis());
        if (rx < 0 || ry < 0 || rz < 0 || rx >= n || ry >= n || rz >= n)
        {
            return 0;
        }
        return GalaxyEggbert::Game::QuarterTurns(worldRuntime_.GetWorld(), static_cast<std::uint16_t>(rx),
                                                 static_cast<std::uint16_t>(ry), static_cast<std::uint16_t>(rz));
    }

    int GalaxyEggbertGame::CellObjectQuarterTurns(float x, float y, float z) const
    {
        const long rx = std::lround(x) + WorldRuntime::kWorldCenterX;
        const long ry = std::lround(y);
        const long rz = std::lround(z) + WorldRuntime::kWorldCenterZ;
        const long n = static_cast<long>(worldRuntime_.GetWorld().blocksPerAxis());
        if (rx < 0 || ry < 0 || rz < 0 || rx >= n || ry >= n || rz >= n)
        {
            return 0;
        }
        return GalaxyEggbert::Game::ObjectQuarterTurns(worldRuntime_.GetWorld(), static_cast<std::uint16_t>(rx),
                                                       static_cast<std::uint16_t>(ry), static_cast<std::uint16_t>(rz));
    }

    bool GalaxyEggbertGame::EnemyBlupiModelDrawn(const MobileObjSpec& obj) const
    {
        return (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType32 ||
                obj.type == GalaxyEggbert::Def::ObjectType::ObjectType33) &&
               blupiEffect_ && !enemyBlupiTextures_.empty();
    }

    const ObjectModelPose* GalaxyEggbertGame::ConveyorLiftModel(const MobileObjSpec& obj) const
    {
        // The plain lift (1) too: Decor/techno29.rd4 for its tile 29 (lift29), else its bar and stem.
        if (obj.type != GalaxyEggbert::Def::ObjectType::ObjectType47 &&
            obj.type != GalaxyEggbert::Def::ObjectType::ObjectType48 &&
            obj.type != GalaxyEggbert::Def::ObjectType::ObjectType1)
        {
            return nullptr;
        }
        const auto visual = ResolveObjectVisual(obj.type, static_cast<int>(obj.phase), obj.visualIcon);
        if (!obj.active || !visual.visible)
        {
            return nullptr;
        }
        return objectModelTable_.Find(ObjectTextureSource::ObjectM, visual.icon);
    }

    void GalaxyEggbertGame::ToggleFullscreen()
    {
        if (!graphics_ || replayingResume_ || demoActive_) return;
        graphics_->ToggleFullScreen();
        saveData_.SetFullscreen(graphics_->getIsFullScreenProperty());
        SavePlayerData();
    }

    void GalaxyEggbertGame::ApplyCheat(int cheatNumber)
    {
        if (!GalaxyEggbert::Game::kCheatsEnabled || inputPad_.IsCheatEntryLocked()) return;
        switch (cheatNumber)
        {
            case 1: // OpenDoors: Decor::CheatAction flips m_bCheatDoors and calls AdaptDoors(): in a
                    // hub every door slides up (sound 33); elsewhere nothing opens (the typed
                    // word also gives the keys, ApplyTypedCheat()).
                cheatDoors_ = !cheatDoors_;
                AdaptHubDoors(worldRuntime_.GetMissionNumber());
                RefreshTerrainMeshIfChanged();
                break;
            case 2: // SuperBlupi -- pure invincibility toggle, NOT "all abilities" (an earlier draft was wrong)
                blupi_.SetCheatSuperBlupi(!blupi_.GetCheatSuperBlupi());
                break;
            case 3: // ShowSecret: draws the flight barrier (214), invisible in play (Decor.cpp:930)
                cheatShowSecret_ = !cheatShowSecret_;
                RefreshTerrainMeshIfChanged();
                break;
            case 4: // "LayEgg" -- real effect sets lives to 9 (the enum name is misleading, no egg is spawned)
                interaction_.SetLives(9);
                break;
            case 5: // Reset gamer progress
                saveData_.Reset();
                SavePlayerData();
                inputPad_.SetJumpRight(saveData_.GetJumpRight());
                break;
            case 6: // Trial mode simulate-toggle -- no observable effect here, Trial phase is unreachable
                break;
            case 7: // CleanAll
                if (interaction_.CheatCleanAll(worldRuntime_))
                {
                    cameraShake_.Trigger(CameraShakeType::Small);
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel10); // each enemy's explosion
                }
                break;
            case 8: // AllTreasure
                interaction_.CheatAllTreasure(worldRuntime_, sound_);
                break;
            case 9: // EndGoal -- teleports to the exit; the EXISTING exit-contact check
                    // re-evaluates the real win/no-win gate next frame, no forcing needed
            {
                float exitX = 0.0f, exitY = 0.0f, exitZ = 0.0f;
                if (interaction_.CheatFindExit(worldRuntime_, exitX, exitY, exitZ))
                {
                    blupi_.SetPosition(exitX, exitY, exitZ);
                }
                break;
            }
            default:
                break;
        }
    }

    void GalaxyEggbertGame::ApplyTypedCheat(GalaxyEggbert::Game::InputPad::TypedCheat cheat)
    {
        if (!GalaxyEggbert::Game::kCheatsEnabled) return;
        using TC = GalaxyEggbert::Game::InputPad::TypedCheat;
        if (inputPad_.IsCheatEntryLocked() && (inputPad_.IsCheatEntryLockedForRun() || cheat != TC::FairPlay)) return;
        using VM = BlupiController::VehicleMode;
        using GalaxyEggbert::Def::SoundChannel;
        const auto vehicle = [this](VM mode)
        {
            blupi_.CheatVehicle(mode, &worldRuntime_.GetWorld());
            activeCheats_.erase(std::remove(activeCheats_.begin(), activeCheats_.end(), "ghost"), activeCheats_.end());
        };
        // Sucette/Charge/Drink without a pickup: the clip, then the power (ResolvePickupFreeze(),
        // no item to put back).
        const auto action = [this](BlupiController::PickupFreezeKind kind, SoundChannel sound)
        {
            blupi_.CheatClearForAction();
            if (blupi_.TriggerPickupFreeze(kind))
            {
                sound_.Play(sound);
                reusablePickup_ = {};
            }
        };
        switch (cheat)
        {
            case TC::OpenDoors:
                ApplyCheat(1);
                interaction_.CheatAllKeys(); // InputPad.cpp:838-841
                break;
            case TC::CleanAll: ApplyCheat(7); break;
            case TC::MegaBlupi: ApplyCheat(2); break;
            case TC::LayEgg: ApplyCheat(4); break;
            case TC::KillEgg: break; // no case in Decor::CheatAction
            case TC::FunSkate: vehicle(VM::Skateboard); break;
            case TC::GiveCopter:
                vehicle(VM::Helicopter);
                std::cout << "GalaxyEggbertCNA: givecopter activated." << std::endl;
                break;
            case TC::JeepDrive: vehicle(VM::Jeep); break;
            case TC::DriveTank: vehicle(VM::Tank); break;
            case TC::IOvercraft: vehicle(VM::Overcraft); break;
            case TC::AllTreasure: ApplyCheat(8); break;
            case TC::EndGoal: ApplyCheat(9); break;
            case TC::ShowSecret: ApplyCheat(3); break;
            case TC::RoundShield:
                sound_.Play(SoundChannel::SoundChannel42);
                blupi_.CheatShield();
                interaction_.ResetMagicTrail(blupi_.GetX(), blupi_.GetY(), blupi_.GetZ());
                break;
            case TC::QuickLollipop:
                action(BlupiController::PickupFreezeKind::Sucette, SoundChannel::SoundChannel50);
                break;
            case TC::PowerCharge:
                action(BlupiController::PickupFreezeKind::Charge, SoundChannel::SoundChannel58);
                break;
            case TC::HideDrink:
                action(BlupiController::PickupFreezeKind::Drink, SoundChannel::SoundChannel57);
                break;
            case TC::TenBombs:
                interaction_.CheatSetPerso(10);
                sound_.Play(SoundChannel::SoundChannel60);
                break;
            case TC::BirdLime: interaction_.CheatSetBullets(10); break;
            case TC::UDynamite:
                interaction_.CheatSetDynamite(1);
                sound_.Play(SoundChannel::SoundChannel60);
                break;
            case TC::WeelKeys: interaction_.CheatAllKeys(); break;
            case TC::Zoom:
                cheatZoomStep_ = (cheatZoomStep_ + 1) % 5; // 1x, 2x, 4x, 8x, 16x (6 %), back to 1x
                break;
            case TC::Help:
                controlsHelp_.shown = true;
                controlsHelp_.cheats = true;
                break;
            case TC::Compass:
                cheatCompass_ = !cheatCompass_;
                break;
            case TC::Debug:
                cheatDebug_ = !cheatDebug_;
                break;
            case TC::Fps:
                cheatFps_ = !cheatFps_;
                break;
            case TC::XMission:
                // Speedy Blupi's XMISSION (BD-0118): the editor on this world, after this tick. Not in
                // a replay (a demo, a resume reconstruction), a recording or a play-test.
                if (!demoActive_ && !replayingResume_ && !demoRecording_ && !demoRecordArmed_ && !editorPlayTestActive_)
                    xmissionPending_ = true;
                break;
            case TC::Honest:
                // Every cheat off, as fairplay's lock, and locked until the game quits: no word is taken
                // any more (fairplay, honest again) (the owner, 2026-11-09). A demo's is its own.
                if (!demoActive_) noCheatsForRun_ = true;
                TurnCheatsOff(/*forRun=*/true);
                break;
            case TC::FairPlay:
                if (!inputPad_.IsCheatEntryLocked())
                    TurnCheatsOff(/*forRun=*/false);
                else
                {
                    inputPad_.SetCheatEntryLocked(false);
                    cheatMenuShown_ = false;
                    activeCheats_.erase(std::remove(activeCheats_.begin(), activeCheats_.end(), "fairplay"),
                                        activeCheats_.end());
                }
                break;
            case TC::Ghost:
            case TC::Quick:
            case TC::None:
                break; // ghost and quick: handled by the caller
        }
    }

    void GalaxyEggbertGame::TurnCheatsOff(bool forRun)
    {
        if (forRun) inputPad_.LockCheatEntryForRun();
        else inputPad_.SetCheatEntryLocked(true);
        cheatMenuShown_ = false;
        blupi_.DisablePersistentCheats(worldRuntime_.GetWorld());
        cheatDoors_ = cheatShowSecret_ = cheatCompass_ = cheatDebug_ = cheatFps_ = quickCheatEnabled_ = false;
        cheatZoomStep_ = 0;
        if (GalaxyEggbert::Def::IsBeyondFast(gameSpeed_))
            gameSpeed_ = GalaxyEggbert::Def::GameSpeed::Normal;
        activeCheats_.clear();
        activeCheats_.push_back(forRun ? "honest" : "fairplay");
        AdaptHubDoors(worldRuntime_.GetMissionNumber());
        RefreshTerrainMeshIfChanged();
    }

    GetTypeNameCPP(GalaxyEggbertGame, "GalaxyEggbertGame")
}
