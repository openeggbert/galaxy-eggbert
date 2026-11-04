#include "GalaxyEggbertGame.hpp"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif
#include <GalaxyEggbert/Game/GamepadInput.hpp>
#include <GalaxyEggbert/Game/BarRows.hpp>
#include <GalaxyEggbert/Game/BlockModels.hpp>
#include <GalaxyEggbert/Game/BoltMesh.hpp>
#include <GalaxyEggbert/Game/GlueStrands.hpp>

#include <fstream>
#include <sstream>
#include <GalaxyEggbert/Editor/CustomWorldStorage.hpp>
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

namespace GalaxyEggbert::CNA
{
    using namespace GalaxyEggbert::Game;

    namespace
    {
        constexpr int kGoldenCaptureTicks[] = {60, 120, 180};
        constexpr int kGoldenCaptureCount =
            static_cast<int>(sizeof(kGoldenCaptureTicks) / sizeof(kGoldenCaptureTicks[0]));

        // The first gamepad as the keys it stands for (GamepadInput.hpp: the owner's request of
        // 2026-11-02; Mobile Eggbert has no gamepad).
        GalaxyEggbert::Game::GamepadFrame ReadGamepadFrame()
        {
            using namespace Microsoft::Xna::Framework::Input;
            GalaxyEggbert::Game::GamepadFrame f;
            const GamePadState pad = GamePad::GetState(Microsoft::Xna::Framework::PlayerIndex::One);
            f.connected = pad.getIsConnectedProperty();
            if (!f.connected)
            {
                return f;
            }
            const auto down = [](ButtonState s) { return s == ButtonState::Pressed; };
            const auto& stick = pad.getThumbSticksProperty().getLeftProperty();
            f.leftX = stick.X;
            f.leftY = stick.Y;
            const auto& dpad = pad.getDPadProperty();
            f.dpadUp = down(dpad.getUpProperty());
            f.dpadDown = down(dpad.getDownProperty());
            f.dpadLeft = down(dpad.getLeftProperty());
            f.dpadRight = down(dpad.getRightProperty());
            const auto& b = pad.getButtonsProperty();
            f.a = down(b.getAProperty());
            f.b = down(b.getBProperty());
            f.x = down(b.getXProperty());
            f.y = down(b.getYProperty());
            f.leftShoulder = down(b.getLeftShoulderProperty());
            f.rightShoulder = down(b.getRightShoulderProperty());
            f.start = down(b.getStartProperty());
            f.back = down(b.getBackProperty());
            return f;
        }

        Microsoft::Xna::Framework::Input::KeyboardState ReadKeyboardState(bool suppressLiveInput)
        {
            using namespace Microsoft::Xna::Framework::Input;
            if (suppressLiveInput)
            {
                return KeyboardState{};
            }
            const KeyboardState keyboard = Keyboard::GetState();
            const unsigned pad = GalaxyEggbert::Game::GamepadKeys(ReadGamepadFrame());
            if (pad == 0u)
            {
                return keyboard;
            }
            const auto pressed = keyboard.GetPressedKeys();
            std::unordered_set<Keys> keys(pressed.begin(), pressed.end());
            using namespace GalaxyEggbert::Game;
            const std::pair<unsigned, Keys> map[] = {
                {kPadUp, Keys::Up}, {kPadDown, Keys::Down}, {kPadLeft, Keys::Left}, {kPadRight, Keys::Right},
                {kPadJump, Keys::LeftControl}, {kPadAction, Keys::Space}, {kPadBackUp, Keys::RightControl},
                {kPadCrouch, Keys::LeftShift}, {kPadLookUp, Keys::RightShift}, {kPadPause, Keys::Escape},
                {kPadConfirm, Keys::Enter}};
            for (const auto& [bit, key] : map)
            {
                if (pad & bit)
                {
                    keys.insert(key);
                }
            }
            return KeyboardState(keys);
        }

        Microsoft::Xna::Framework::Input::MouseState ReadMouseState(bool suppressLiveInput)
        {
            return suppressLiveInput
                ? Microsoft::Xna::Framework::Input::MouseState{}
                : Microsoft::Xna::Framework::Input::Mouse::GetState();
        }

        // Placeholder-model animation mapping (2026-07-09, NEXT.md §3) --
        // avatars3d/blupi_placeholder/'s 3 clips (Survey/Walk/Run) don't
        // correspond to BlupiController::AnimState's states at all (10 as
        // of 2026-07-11's Jump/Air split plus Ecrase/Balloon/Teleporting,
        // plan.md E3D-MIG-064), so this is a rough best-effort substitution,
        // not a faithful behavioral mapping -- see
        // avatars3d/blupi_placeholder/README.md's own mapping table for the
        // reasoning per state. Swap out entirely once a real Blupi model
        // with real matching clips exists.
        const std::string& BlupiAnimStateToPlaceholderClipName(BlupiController::AnimState state)
        {
            static const std::string kSurvey = "Survey";
            static const std::string kWalk = "Walk";
            static const std::string kRun = "Run";
            switch (state)
            {
                case BlupiController::AnimState::March:
                case BlupiController::AnimState::MarchEcrase:
                case BlupiController::AnimState::MarchHelico:
                case BlupiController::AnimState::MarchJeep:
                case BlupiController::AnimState::MarchTank:
                case BlupiController::AnimState::MarchSkate:
                case BlupiController::AnimState::MarchOver:
                case BlupiController::AnimState::MarchNage:
                case BlupiController::AnimState::MarchSurf:
                case BlupiController::AnimState::MarchSuspend:
                case BlupiController::AnimState::Push: return kWalk;
                case BlupiController::AnimState::Jump:
                case BlupiController::AnimState::Air:   return kRun;
                case BlupiController::AnimState::Stop:
                case BlupiController::AnimState::Down:
                case BlupiController::AnimState::Up:
                case BlupiController::AnimState::StopEcrase:
                case BlupiController::AnimState::Balloon:
                case BlupiController::AnimState::Teleporting:
                case BlupiController::AnimState::Bye:
                case BlupiController::AnimState::StopHelico:
                case BlupiController::AnimState::StopJeep:
                case BlupiController::AnimState::StopTank:
                case BlupiController::AnimState::StopSkate:
                case BlupiController::AnimState::StopOver:
                case BlupiController::AnimState::StopNage:
                case BlupiController::AnimState::StopSurf:
                case BlupiController::AnimState::StopSuspend:
                case BlupiController::AnimState::JumpSuspend:
                case BlupiController::AnimState::Non:
                case BlupiController::AnimState::Hide:
                case BlupiController::AnimState::Mockery:
                case BlupiController::AnimState::Mockeryi:
                case BlupiController::AnimState::Mockeryp:
                default:
                    return kSurvey;
            }
        }

        // The real model (avatars3d/blupi/, its vehicles too) is in metres
        // at 0.05 m per Ray Dream unit. The original game camera draws
        // 3.0309 px per unit (zoom 3.43654 x 60 px / frame height 68.03136)
        // and a block is 64 px (DIMOBJX), so a block is 21.116 units =
        // 1.0558 m, and the model keeps the sprites' size against blocks at
        // 1 / 1.0558.
        constexpr float kRealModelScale = 0.94714f;

        // Vehicle pickups the real vehicle models replace when parked: the
        // model (avatars3d/blupi/vehicles/<name>) and the clip whose first
        // frame is its standing pose.
        struct ParkedVehicle
        {
            GalaxyEggbert::Def::ObjectType type;
            const char* model;
            const char* clip;
        };
        constexpr ParkedVehicle kParkedVehicles[] = {
            {GalaxyEggbert::Def::ObjectType::ObjectType13, "helico", "StopHelico"},
            {GalaxyEggbert::Def::ObjectType::ObjectType19, "jeep", "StopJeep"},
            {GalaxyEggbert::Def::ObjectType::ObjectType24, "skate", "StopSkate"},
            {GalaxyEggbert::Def::ObjectType::ObjectType28, "tank", "StopTank"},
            {GalaxyEggbert::Def::ObjectType::ObjectType46, "over", "StopOver"},
        };

        // How long reversing direction takes: the length of the real Turn
        // action of the mode Blupi is in (table_blupi entries at 20 Hz:
        // Turn 6, TurnJeep 7, TurnOver 7, TurnHelico 10, TurnNage 10,
        // TurnSurf 10, TurnTank 12, TurnSkate 14). The 3D model turns for
        // real over that time; the sprites' turn icons were views of it.
        float TurnAroundSeconds(BlupiController::AnimState state)
        {
            using S = BlupiController::AnimState;
            constexpr float kTick = 1.0f / 20.0f;
            switch (state)
            {
                case S::StopJeep:
                case S::MarchJeep:
                case S::StopOver:
                case S::MarchOver:
                    return 7.0f * kTick;
                case S::StopHelico:
                case S::MarchHelico:
                case S::HelicoGlu:
                case S::StopNage:
                case S::MarchNage:
                case S::StopSurf:
                case S::MarchSurf:
                    return 10.0f * kTick;
                case S::StopTank:
                case S::MarchTank:
                case S::FireTank:
                    return 12.0f * kTick;
                case S::StopSkate:
                case S::MarchSkate:
                case S::JumpSkate:
                case S::AirSkate:
                case S::SlowdownSkate:
                    return 14.0f * kTick;
                default:
                    return 6.0f * kTick;
            }
        }

        // The clip for a state the model has none for yet: the Stop of the vehicle he rides, so
        // the vehicle is still drawn (it is drawn only with a clip it has), else Stop.
        std::string MissingClipFallback(const BlupiController& blupi)
        {
            switch (blupi.GetVehicleMode())
            {
                case BlupiController::VehicleMode::Helicopter: return "StopHelico";
                case BlupiController::VehicleMode::Jeep: return "StopJeep";
                case BlupiController::VehicleMode::Tank: return "StopTank";
                case BlupiController::VehicleMode::Skateboard: return "StopSkate";
                case BlupiController::VehicleMode::Overcraft: return "StopOver";
                case BlupiController::VehicleMode::None: break;
            }
            return "Stop";
        }

        // Real Blupi model clip names (avatars3d/blupi/, 2026-10-24): one clip
        // per AnimState, named after it, plus DeathLocked.<cause> and
        // PickupBusy.<kind>, each built from the state's own sprite icon table
        // at kAnimFps (tools/rd4_to_gltf/build_clip_library.py). The caller
        // falls back to MissingClipFallback() for a state with no clip yet.
        std::string BlupiClipName(const BlupiController& blupi)
        {
            using S = BlupiController::AnimState;
            switch (blupi.GetAnimState())
            {
                case S::Stop: return "Stop";
                case S::March: return "March";
                case S::Jump: return "Jump";
                case S::Air: return "Air";
                case S::Down: return "Down";
                case S::Up: return "Up";
                case S::StopEcrase: return "StopEcrase";
                case S::MarchEcrase: return "MarchEcrase";
                case S::Balloon: return "Balloon";
                case S::Teleporting: return "Teleporting";
                case S::Bye: return "Bye";
                case S::StopHelico: return "StopHelico";
                case S::MarchHelico: return "MarchHelico";
                case S::StopJeep: return "StopJeep";
                case S::MarchJeep: return "MarchJeep";
                case S::StopTank: return "StopTank";
                case S::MarchTank: return "MarchTank";
                case S::StopSkate: return "StopSkate";
                case S::MarchSkate: return "MarchSkate";
                case S::JumpSkate: return "JumpSkate";
                case S::AirSkate: return "AirSkate";
                case S::SlowdownSkate: return "SlowdownSkate";
                case S::StopOver: return "StopOver";
                case S::MarchOver: return "MarchOver";
                case S::StopNage: return "StopNage";
                case S::MarchNage: return "MarchNage";
                case S::StopSurf: return "StopSurf";
                case S::MarchSurf: return "MarchSurf";
                case S::StopSuspend: return "StopSuspend";
                case S::MarchSuspend: return "MarchSuspend";
                case S::JumpSuspend: return "JumpSuspend";
                case S::Non: return "Non";
                case S::Hide: return "Hide";
                case S::Push: return "Push";
                case S::StopPop: return "StopPop";
                case S::Pop: return "Pop";
                case S::Win: return "Win";
                case S::Turn: return "Turn";
                case S::Vertigo: return "Vertigo";
                case S::Recede: return "Recede";
                case S::Advance: return "Advance";
                case S::JumpAie: return "JumpAie";
                case S::Ouf1a: return "Ouf1a";
                case S::Ouf1b: return "Ouf1b";
                case S::Ouf3: return "Ouf3";
                case S::Ouf4: return "Ouf4";
                case S::Ouf5: return "Ouf5";
                case S::Ouf2: return "Ouf2";
                case S::StopMarch: return "StopMarch";
                case S::StopJump: return "StopJump";
                case S::Switch: return "Switch";
                case S::TakeDynamite: return "TakeDynamite";
                case S::PutDynamite: return "PutDynamite";
                case S::FireTank: return "FireTank";
                case S::HelicoGlu: return "HelicoGlu";
                case S::TakeSkate: return "TakeSkate";
                case S::DeposeSkate: return "DeposeSkate";
                case S::Mockery: return "Mockery";
                case S::Mockeryi: return "Mockeryi";
                case S::Mockeryp: return "Mockeryp";
                case S::DeathLocked:
                    switch (blupi.GetDeathCause())
                    {
                        case BlupiController::DeathCause::Clear1: return "DeathLocked.Clear1";
                        case BlupiController::DeathCause::Clear2: return "DeathLocked.Clear2";
                        case BlupiController::DeathCause::Clear3: return "DeathLocked.Clear3";
                        case BlupiController::DeathCause::Clear4: return "DeathLocked.Clear4";
                        case BlupiController::DeathCause::Glu: return "DeathLocked.Glu";
                        case BlupiController::DeathCause::Drown: return "DeathLocked.Drown";
                    }
                    break;
                case S::PickupBusy:
                    switch (blupi.GetPickupFreezeKind())
                    {
                        case BlupiController::PickupFreezeKind::Sucette: return "PickupBusy.Sucette";
                        case BlupiController::PickupFreezeKind::Drink: return "PickupBusy.Drink";
                        case BlupiController::PickupFreezeKind::Charge: return "PickupBusy.Charge";
                    }
                    break;
            }
            return "Stop";
        }

        // INFRA-006 (plan.md §7, `REMAKE-ANALYSIS.md` P1-2) handler-table
        // migration, 4th family: the vehicle GalaxyEggbert::Def::ObjectType<->VehicleMode
        // mapping, previously 2 separate 5-case switches (mount-scan and
        // dismount-deposit) doing the same bijection in opposite
        // directions. A pure 1:1 lookup with no per-type divergence --
        // Skateboard's real TakeSkate/DeposeSkate anim (the one thing that
        // varies per vehicle) stays its own explicit check at each call
        // site, deliberately kept outside this table since it's not part
        // of the type<->mode mapping itself.
        struct VehicleModeMapping
        {
            GalaxyEggbert::Def::ObjectType type;
            BlupiController::VehicleMode mode;
        };
        constexpr VehicleModeMapping kVehicleModeTable[] = {
            {GalaxyEggbert::Def::ObjectType::ObjectType13, BlupiController::VehicleMode::Helicopter},
            {GalaxyEggbert::Def::ObjectType::ObjectType19, BlupiController::VehicleMode::Jeep},
            {GalaxyEggbert::Def::ObjectType::ObjectType28, BlupiController::VehicleMode::Tank},
            {GalaxyEggbert::Def::ObjectType::ObjectType24, BlupiController::VehicleMode::Skateboard},
            {GalaxyEggbert::Def::ObjectType::ObjectType46, BlupiController::VehicleMode::Overcraft},
        };

        // Mirrors the old switch's own `default: -> Overcraft` fallback
        // exactly -- `IsInVehicle()` already gates every caller so `mode`
        // is never actually `None` here, and Overcraft is the only real
        // mode left unmatched by a linear scan miss.
        GalaxyEggbert::Def::ObjectType VehicleModeToObjectType(BlupiController::VehicleMode mode)
        {
            for (const auto& entry : kVehicleModeTable)
            {
                if (entry.mode == mode)
                {
                    return entry.type;
                }
            }
            return GalaxyEggbert::Def::ObjectType::ObjectType46;
        }

        bool ObjectTypeToVehicleMode(GalaxyEggbert::Def::ObjectType type, BlupiController::VehicleMode& outMode)
        {
            for (const auto& entry : kVehicleModeTable)
            {
                if (entry.type == type)
                {
                    outMode = entry.mode;
                    return true;
                }
            }
            return false;
        }

        // INFRA-006 (plan.md §7, `REMAKE-ANALYSIS.md` P1-2) handler-table
        // migration, 5th family: the 7 patrol enemies with a real
        // per-direction/turn-transition icon table (bulldozer/fish/bird/
        // blupih/blupit/wasp/creature) -- purely cosmetic icon-selection
        // for the billboard-render loop, NOT the kill/damage/contact logic
        // that ruled out migrating the wider enemy/hazard family (see that
        // family's own "investigated, not a fit" writeup). 6 of the 7 real
        // icon functions share one signature; `GetCreatureIcon()` alone
        // drops the direction bool, so it gets a thin same-signature
        // wrapper purely to fit the shared table -- not a behavior change.
        using PatrolIconFn = int (*)(bool patrolGoesLeftFromStart, int patrolStep, int patrolTimeTicks);

        int GetCreatureIconIgnoringDirection(bool /*patrolGoesLeftFromStart*/, int patrolStep, int patrolTimeTicks)
        {
            return GetCreatureIcon(patrolStep, patrolTimeTicks);
        }

        struct PatrolIconHandler
        {
            GalaxyEggbert::Def::ObjectType type;
            PatrolIconFn iconFn;
        };
        constexpr PatrolIconHandler kPatrolIconHandlers[] = {
            {GalaxyEggbert::Def::ObjectType::ObjectType4, GetBulldozerIcon},
            {GalaxyEggbert::Def::ObjectType::ObjectType17, GetFishIcon},
            {GalaxyEggbert::Def::ObjectType::ObjectType20, GetBirdIcon},
            {GalaxyEggbert::Def::ObjectType::ObjectType32, GetBlupihIcon},
            {GalaxyEggbert::Def::ObjectType::ObjectType33, GetBlupitIcon},
            {GalaxyEggbert::Def::ObjectType::ObjectType44, GetWaspIcon},
            {GalaxyEggbert::Def::ObjectType::ObjectType54, GetCreatureIconIgnoringDirection},
        };

        // Returns false (caller keeps its own `GetObjIcon()` fallback) for
        // any type not in this 7-member family, same as the old switch's
        // own `default` case.
        bool TryGetPatrolIcon(GalaxyEggbert::Def::ObjectType type, bool patrolGoesLeftFromStart, int patrolStep,
                               int patrolTimeTicks, int& outIcon)
        {
            for (const auto& entry : kPatrolIconHandlers)
            {
                if (entry.type == type)
                {
                    outIcon = entry.iconFn(patrolGoesLeftFromStart, patrolStep, patrolTimeTicks);
                    return true;
                }
            }
            return false;
        }

        // Real fixed 5.0s Wait-phase cosmetic timer (plan.md
        // MENU-001..005, confirmed via research: `Game1.cpp`'s real
        // `waitProgress = ticks/50,000,000`) -- must match
        // InputPad::DrawWait()'s own gauge-fill duration (kept as a
        // separately-owned constant there since that class doesn't
        // depend on this one).
        constexpr float kWaitDurationSeconds = 5.0f;

        // Real fade-out phase-transition commit timer (plan.md
        // MENU-088/089, confirmed via research: `Config::ScaleTime(20)` =
        // 1.0s at this build's pinned 20fps) -- must match
        // InputPad.cpp's own kFadeDurationSeconds (kept as a
        // separately-owned constant there, same cross-file convention as
        // kWaitDurationSeconds above).
        constexpr float kFadeCommitDurationSeconds = 1.0f;

        // Third-person chase-camera wall collision (plan.md CAM-005, found
        // 2026-07-17): keeps the camera from clipping through solid
        // geometry to end up outside a tunnel/room looking back in through
        // a wall. A simplified fixed-step raymarch from the look-at point
        // toward the desired eye position, stopping at the first solid
        // block -- not a literal cell-boundary DDA voxel traversal (the
        // task's own original wording), since the real max distance
        // involved is tiny (a few world units, the chase distance) and a
        // small fixed step is both simpler and plenty precise at that
        // scale; same "documented simplification, not the textbook
        // algorithm" category as several other collision adaptations this
        // session. Solidity uses a plain `!isAir()` check (this is a
        // visual-occlusion query, not Blupi's own movement-collision rule
        // set -- e.g. a real non-solid-for-Blupi Teleporter pillar still
        // visually blocks the camera's view through it, which is the
        // correct behavior here even though it would be wrong for
        // BlupiController's own `IsSolidAt`). Returns the safe distance
        // along `direction` (a unit vector), always `<= maxDistance`.
        // TerrainLighting.hpp's light on an effect that draws the terrain, its grass or the cube
        // objects in it (their meshes carry normals: MakeLitTexturedMeshBuilder()).
        void ApplyTerrainLighting(Microsoft::Xna::Framework::Graphics::BasicEffect& effect)
        {
            using Microsoft::Xna::Framework::Vector3;
            const auto& light = GalaxyEggbert::Game::kTerrainLight;
            effect.setLightingEnabledProperty(true);
            effect.setAmbientLightColorProperty(Vector3(light.ambient, light.ambient, light.ambient));
            effect.setSpecularColorProperty(Vector3::Zero);
            auto& key = effect.getDirectionalLight0Property();
            key.setEnabledProperty(true);
            key.setDiffuseColorProperty(Vector3(light.diffuse, light.diffuse, light.diffuse));
            key.setDirectionProperty(Vector3::Normalize(Vector3(light.directionX, light.directionY, light.directionZ)));
            key.setSpecularColorProperty(Vector3::Zero);
            effect.getDirectionalLight1Property().setEnabledProperty(false);
            effect.getDirectionalLight2Property().setEnabledProperty(false);
        }

        // Whether a point (render space) is in a block that stops the camera. Water and blocks
        // nothing collides with (machine parts, signs) leave the view open: the camera follows
        // Blupi under water. (Treating water as a wall pulled the eye onto the look-at point and
        // Camera::LookAt threw, 2026-10-25.)
        bool BlocksCamera(const GalaxyEggbert::Worlds::World& world, const Microsoft::Xna::Framework::Vector3& point)
        {
            const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
            const int gx = static_cast<int>(std::lround(point.X)) + WorldRuntime::kWorldCenterX;
            const int gy = static_cast<int>(std::lround(point.Y));
            const int gz = static_cast<int>(std::lround(point.Z)) + WorldRuntime::kWorldCenterZ;
            if (gx < 0 || gz < 0 || gy < 0 || gx >= blocksPerAxis || gz >= blocksPerAxis || gy >= blocksPerAxis)
            {
                return false; // out of bounds -- open air
            }
            const auto type = world.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                             static_cast<std::uint16_t>(gz))
                                  .type();
            const auto& block = GalaxyEggbert::GetBlockDefinition(type);
            return type != 0 && !block.water && block.collisionMask != 0;
        }

        float RaymarchWallDistance(const GalaxyEggbert::Worlds::World& world,
                                   const Microsoft::Xna::Framework::Vector3& origin,
                                   const Microsoft::Xna::Framework::Vector3& direction, float maxDistance)
        {
            constexpr float kStep = 0.1f;
            // Kept this far before a hit: the near plane (0.1, its corners about 0.12 away) must
            // not reach through the face, or the camera shows what lies behind it.
            constexpr float kFaceMargin = 0.15f;
            float traveled = 0.0f;
            while (traveled < maxDistance)
            {
                traveled = std::min(traveled + kStep, maxDistance);
                if (BlocksCamera(world, origin + direction * traveled))
                {
                    return std::max(0.0f, traveled - kStep - kFaceMargin);
                }
            }
            return maxDistance;
        }

        // Real GalaxyEggbert::Def::GameSpeed (SCORE-009/010/011) as a continuous dt-scale
        // factor -- see gameSpeed_'s own member comment for the "why a
        // factor, not a literal N-repeat" rationale. Slow's real "every
        // OTHER frame runs one tick" behavior is approximated as a
        // continuous 0.5x here (same average effect, no frame-parity
        // bookkeeping needed).
        float GameSpeedFactor(GalaxyEggbert::Def::GameSpeed speed) noexcept
        {
            return (speed == GalaxyEggbert::Def::GameSpeed::Slow) ? 0.5f
                                                               : static_cast<float>(GalaxyEggbert::Def::ToRaw(speed));
        }
    }

    namespace
    {
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
        void GetBackBufferSize(
            const Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int& w, int& h)
        {
            const auto& pp = device.getPresentationParametersProperty();
            w = static_cast<int>(pp.getBackBufferWidthProperty());
            h = static_cast<int>(pp.getBackBufferHeightProperty());
        }
    }

    GalaxyEggbertGame::GalaxyEggbertGame()
    {
        Game::getWindowProperty().setTitleProperty("Galaxy Eggbert");
        // Standard XNA pattern: the manager must be constructed and the
        // required profile selected before Run() initializes the device.
        // Galaxy's screenshot/golden-capture diagnostics use
        // GetBackBufferData, which is a HiDef-only XNA operation.  Leaving
        // CNA's default Reach profile selected made the first diagnostic
        // frame terminate both the OpenGL 3.3 and WebGL 2 builds.
        graphics_ = std::make_unique<Microsoft::Xna::Framework::GraphicsDeviceManager>(this);
        graphics_->setGraphicsProfileProperty(
            Microsoft::Xna::Framework::Graphics::GraphicsProfile::HiDef);
        camera_.SetLookAt(Microsoft::Xna::Framework::Vector3(0.0f, 2.0f, 5.0f), cameraTarget_);
    }

    void GalaxyEggbertGame::LoadContent()
    {
        // Default world source: a genuinely 3D, hand-authored .vwr world
        // (plan.md E3D-MIG-058), not a flat mobile-eggbert .txt layout.
        // LoadFromMobileEggbertFile() stays available on WorldRuntime as a
        // secondary/reference path (e.g. for later faithful-remake level
        // porting) but is no longer the default load here.
        const bool loaded = worldRuntime_.LoadFromVwrFile("worlds3d/world001.vwr");
        if (!loaded)
        {
            std::cout << "GalaxyEggbertCNA: worlds3d/world001.vwr not found or failed to load "
                         "(run the binary from its own build directory)." << std::endl;
            return;
        }

        const auto& world = worldRuntime_.GetWorld();
        int nonAirBlocks = 0;
        int minY = -1;
        int maxY = -1;
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        for (int y = 0; y < blocksPerAxis; ++y)
        {
            for (int z = 0; z < blocksPerAxis; ++z)
            {
                for (int x = 0; x < blocksPerAxis; ++x)
                {
                    if (!world.getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                         static_cast<std::uint16_t>(z)).isAir())
                    {
                        ++nonAirBlocks;
                        if (minY < 0) minY = y;
                        maxY = y;
                    }
                }
            }
        }

        std::cout << "GalaxyEggbertCNA: loaded worlds3d/world001.vwr — "
                  << nonAirBlocks << " non-air blocks, Y range [" << minY << ", " << maxY
                  << "]." << std::endl;

        // Tile-atlas sanity check (plan.md E3D-MIG-053) — nothing builds a
        // cube from this yet; this only proves TileAtlas resolves
        // known block types to plausible UV rects.
        const auto printTileUv = [this](const char* name, int blockType)
        {
            const auto uv = tileAtlas_.GetTileUv(blockType);
            std::cout << "GalaxyEggbertCNA: tile " << name << " (" << blockType
                      << ") UV = (" << uv.u0 << ", " << uv.v0 << ") - ("
                      << uv.u1 << ", " << uv.v1 << ")" << std::endl;
        };
        printTileUv("Ground", GalaxyEggbert::BlockTypes::Ground);
        printTileUv("Lava", GalaxyEggbert::BlockTypes::Lava);
        printTileUv("GoldPillar", GalaxyEggbert::BlockTypes::GoldPillar);

        // Static terrain mesh for the loaded world (plan.md E3D-MIG-054) —
        // one box per non-air cell, textured via TileAtlas.
        auto& device = getGraphicsDeviceProperty();

        // object-m.png was already copied next to this binary at build time
        // (plan.md E3D-MIG-030); loaded directly via CNA's own Texture2D —
        // no mobile-eggbert Pixmap reuse (SpriteBatch-coupled, not usable for
        // this 3D BasicEffect draw path).
        terrainTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D("Content/icons/object-m.png", device);
        std::cout << "GalaxyEggbertCNA: terrain texture loaded — "
                  << terrainTexture_.getWidthProperty() << "x"
                  << terrainTexture_.getHeightProperty() << " px." << std::endl;
        {
            // Which tiles are sprites on clear texels, for the terrain's passes (TileCoverage.hpp).
            const int width = terrainTexture_.getWidthProperty();
            const int height = terrainTexture_.getHeightProperty();
            std::vector<Microsoft::Xna::Framework::Color> texels(static_cast<std::size_t>(width) *
                                                                 static_cast<std::size_t>(height));
            terrainTexture_.GetData(texels.data(), static_cast<int>(texels.size()));
            std::vector<std::uint8_t> alpha(texels.size());
            for (std::size_t i = 0; i < texels.size(); ++i)
            {
                alpha[i] = texels[i].getAProperty();
            }
            tileCoverage_ = GalaxyEggbert::Game::ClassifyTileSheet(alpha.data(), width, height);
            GalaxyEggbert::Game::MarkSeeThroughTiles(tileCoverage_); // the cheese's holes (Q-010)
            // No black where an opaque tile's sprite is clear (TileCoverage.hpp).
            std::vector<std::uint8_t> rgba(texels.size() * 4);
            for (std::size_t i = 0; i < texels.size(); ++i)
            {
                rgba[i * 4 + 0] = texels[i].getRProperty();
                rgba[i * 4 + 1] = texels[i].getGProperty();
                rgba[i * 4 + 2] = texels[i].getBProperty();
                rgba[i * 4 + 3] = texels[i].getAProperty();
            }
            GalaxyEggbert::Game::FillOpaqueTilesClearTexels(rgba.data(), width, height, tileCoverage_);
            for (std::size_t i = 0; i < texels.size(); ++i)
            {
                texels[i] = Microsoft::Xna::Framework::Color(rgba[i * 4 + 0], rgba[i * 4 + 1], rgba[i * 4 + 2],
                                                             rgba[i * 4 + 3]);
            }
            terrainTexture_.SetData(texels.data(), static_cast<int>(texels.size()));
            // The terrain's padded copy with its mip levels (TileAtlas.hpp Layout::Padded): distant
            // floors no longer shimmer into rings.
            constexpr int kPadW = GalaxyEggbert::Game::TileAtlas::kPaddedWidth;
            constexpr int kPadH = GalaxyEggbert::Game::TileAtlas::kPaddedHeight;
            auto level = GalaxyEggbert::Game::BuildPaddedSheet(rgba.data(), width, height);
            terrainMipTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D(
                device, kPadW, kPadH, /*mipMap=*/true, Microsoft::Xna::Framework::Graphics::SurfaceFormat::Color);
            int levelW = kPadW, levelH = kPadH;
            const int levels = terrainMipTexture_.getLevelCountProperty();
            for (int mip = 0; mip < levels; ++mip)
            {
                std::vector<Microsoft::Xna::Framework::Color> colors(static_cast<std::size_t>(levelW) * levelH);
                for (std::size_t i = 0; i < colors.size(); ++i)
                {
                    colors[i] = Microsoft::Xna::Framework::Color(level[i * 4 + 0], level[i * 4 + 1], level[i * 4 + 2],
                                                                 level[i * 4 + 3]);
                }
                terrainMipTexture_.SetData(mip, nullptr, colors.data(), 0, static_cast<int>(colors.size()));
                if (mip + 1 < levels)
                {
                    level = GalaxyEggbert::Game::DownsampleHalf(level, levelW, levelH);
                    levelW = std::max(1, levelW / 2);
                    levelH = std::max(1, levelH / 2);
                }
            }
        }
        RebuildWorldPresentation();

        terrainEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        terrainEffect_->VertexColorEnabled = false;
        terrainEffect_->setTextureEnabledProperty(true);
        ApplyTerrainLighting(*terrainEffect_);
        terrainEffect_->setTextureProperty(&terrainMipTexture_);

        // Icon 107's grass-top overlay (NEXT.md §8 task 3) — genuinely
        // separate from object-m.png, copied next to this binary from
        // textures3d/ (like worlds3d/, see CMakeLists.txt's POST_BUILD
        // step), not from mobile-eggbert's Content/.
        grassTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D("textures3d/grass_top.png", device);
        std::cout << "GalaxyEggbertCNA: grass texture loaded — "
                  << grassTexture_.getWidthProperty() << "x"
                  << grassTexture_.getHeightProperty() << " px." << std::endl;
        grassEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        grassEffect_->VertexColorEnabled = false;
        grassEffect_->setTextureEnabledProperty(true);
        ApplyTerrainLighting(*grassEffect_);
        grassEffect_->setTextureProperty(&grassTexture_);

        // Real mobile-eggbert bottom HUD + the interim animation-state
        // indicator (2026-07-10, see Hud.hpp -- loads its own texture
        // instances, including text.png/pad.png which nothing else loads).
        hud_.LoadContent(device);

        // Real mobile-eggbert on-screen touch controls (2026-07-13, see
        // InputPad.hpp -- loads its own pad.png instance plus
        // Content/backgrounds/pause.png and blupiyoupie.png, which
        // nothing else loads).
        inputPad_.LoadContent(device);

        // Billboard rendering for MoveObjects (15-3d-render-mapping-design.md
        // §5/§7) — element.png from the approved mobile-eggbert assets.
        objectTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D("Content/icons/element.png", device);
        objectEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        objectEffect_->VertexColorEnabled = false;
        objectEffect_->setTextureEnabledProperty(true);
        objectEffect_->setTextureProperty(&objectTexture_);
        std::cout << "GalaxyEggbertCNA: " << worldRuntime_.GetMobileObjects().size()
                  << " MoveObject(s) parsed for billboard rendering." << std::endl;

        // Billboard rendering for object-m.png-sourced MoveObjects
        // (ObjectDefinitionRegistry, NEXT.md §3) -- reuses
        // terrainTexture_ (object-m.png), same reasoning as bigDecorEffect_.
        objectMPngEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        objectMPngEffect_->VertexColorEnabled = false;
        objectMPngEffect_->setTextureEnabledProperty(true);
        objectMPngEffect_->setTextureProperty(&terrainTexture_);

        // Billboard rendering for explo.png-sourced MoveObjects
        // (ObjectDefinitionRegistry, NEXT.md §3) -- explo.png isn't
        // loaded anywhere else in GalaxyEggbertCNA, so this is a genuinely
        // new texture load (already copied next to this binary at build
        // time, same mechanism as element.png/object-m.png).
        exploTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D("Content/icons/explo.png", device);
        exploEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        exploEffect_->VertexColorEnabled = false;
        exploEffect_->setTextureEnabledProperty(true);
        exploEffect_->setTextureProperty(&exploTexture_);

        // Billboard rendering for Blupi-skin MoveObjects
        // (ObjectDefinitionRegistry, NEXT.md §3) -- separate
        // Texture2D instances from blupiIconTexture_ above even though
        // ObjectType200 loads the same blupi.png file, since that one is
        // owned by the 2D SpriteBatch HUD path, not this 3D BasicEffect path.
        blupiObjectTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D("Content/icons/blupi.png", device);
        blupiObjectEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        blupiObjectEffect_->VertexColorEnabled = false;
        blupiObjectEffect_->setTextureEnabledProperty(true);
        blupiObjectEffect_->setTextureProperty(&blupiObjectTexture_);

        blupi1ObjectTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D("Content/icons/blupi1.png", device);
        blupi1ObjectEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        blupi1ObjectEffect_->VertexColorEnabled = false;
        blupi1ObjectEffect_->setTextureEnabledProperty(true);
        blupi1ObjectEffect_->setTextureProperty(&blupi1ObjectTexture_);

        // Billboard rendering for BigDecor: cells. Eggbert 2 renders this
        // layer through CHEXPLO (Decor.cpp's QuickIcon(channel=9,...)), so
        // ids such as Palmtree=16 must sample explo.png, not object-m.png
        // where icon 16 is an unrelated blue triangular mechanical tile.
        bigDecorEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        bigDecorEffect_->VertexColorEnabled = false;
        bigDecorEffect_->setTextureEnabledProperty(true);
        bigDecorEffect_->setTextureProperty(&exploTexture_);

        std::cout << "GalaxyEggbertCNA: " << bigDecorCells_.size()
                  << " BigDecor cell(s) parsed for billboard rendering." << std::endl;

        // Platform-lift/crate UniformCube object path (NEXT.md §8 task 3) --
        // reuses terrainTexture_ (object-m.png), the confirmed-correct sheet
        // for these ObjectTypes (see ObjectDefinitionRegistry's SolidCube
        // render mode).
        // Only the effect is set up here; the mesh itself is rebuilt every
        // frame in Draw() now (2026-07-09, NEXT.md §8 task 3 optional
        // follow-up), same reason as the billboards -- types 47/48
        // (chenille caterpillar tracks) have real phase-cycling icon
        // formulas that need a fresh icon/UV lookup each frame to actually
        // animate, unlike a build-once-and-cache mesh.
        {
            objectCubeEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
            objectCubeEffect_->VertexColorEnabled = false;
            objectCubeEffect_->setTextureEnabledProperty(true);
            ApplyTerrainLighting(*objectCubeEffect_);
            objectCubeEffect_->setTextureProperty(&terrainTexture_);

            int cubeObjectCount = 0;
            for (const auto& obj : worldRuntime_.GetMobileObjects())
            {
                cubeObjectCount +=
                    GetObjectDefinition(obj.type).renderMode == ObjectRenderMode::SolidCube ? 1 : 0;
            }
            std::cout << "GalaxyEggbertCNA: " << cubeObjectCount
                      << " platform-lift/crate cube object(s) found." << std::endl;
        }

        // Third-person-mode 3D model (2026-07-09, NEXT.md §3) -- CNA's
        // generic SkinnedModelEXT/SkinnedEffect path, see this class's own
        // header comment and ../cna/docs/skinned-model-ext.md.
        // ContentManager's RootDirectory is pointed at avatars3d/ (not the
        // default "Content", which is mobile-eggbert's own tree, fully
        // re-copied fresh on every build -- see the textures3d/ comment
        // above for the same reasoning) so Load<>'s asset name is relative
        // to avatars3d/ itself. Currently unused elsewhere in this class,
        // so repointing its RootDirectory here can't break anything else.
        {
            const std::string realModelManifest = "avatars3d/blupi/avatar.skinnedmodel.json";
            const std::string blupiModelManifest = "avatars3d/blupi_placeholder/avatar.skinnedmodel.json";
            blupiModelIsReal_ = std::filesystem::exists(realModelManifest);
            if (blupiModelIsReal_ || std::filesystem::exists(blupiModelManifest))
            {
                auto& content = getContentProperty();
#if defined(__ANDROID__)
                // On Android CNA takes a relative content path for a packaged asset, and every
                // name it misses (each texture's .xnb, tried before its .png) is looked for again
                // ignoring case by listing the APK's assets through JNI, one listing per path
                // component (upstream_bugs.md UB-006). The files are in internal storage, the
                // working directory (AndroidAssets.hpp): an absolute root reads them there.
                content.setRootDirectoryProperty((std::filesystem::current_path() / "avatars3d").string());
#else
                content.setRootDirectoryProperty("avatars3d");
#endif
                blupiModel_ = content.Load<std::shared_ptr<Microsoft::Xna::Framework::Graphics::SkinnedModelEXT>>(
                    blupiModelIsReal_ ? "blupi/avatar" : "blupi_placeholder/avatar");

                blupiEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::SkinnedEffect>(device);
                blupiEffect_->setAmbientLightColorProperty(
                    Microsoft::Xna::Framework::Vector3(0.35f, 0.35f, 0.35f));
                auto& keyLight = blupiEffect_->getDirectionalLight0Property();
                keyLight.setEnabledProperty(true);
                keyLight.setDiffuseColorProperty(
                    Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f));
                keyLight.setSpecularColorProperty(Microsoft::Xna::Framework::Vector3::Zero);
                keyLight.setDirectionProperty(
                    Microsoft::Xna::Framework::Vector3(-0.4f, -0.6f, -0.7f));
                blupiEffect_->getDirectionalLight1Property().setEnabledProperty(false);
                blupiEffect_->getDirectionalLight2Property().setEnabledProperty(false);
                blupiEffect_->setSpecularColorProperty(Microsoft::Xna::Framework::Vector3::Zero);
                blupiEffect_->setEmissiveColorProperty(Microsoft::Xna::Framework::Vector3::Zero);
                // Keep the placeholder's established warm fallback tint.
                blupiEffect_->setDiffuseColorProperty(
                    Microsoft::Xna::Framework::Color::NavajoWhite.ToVector3());
                if (blupiModelIsReal_)
                {
                    // The real model's part textures carry its colours
                    // (yellow body with the face, black eyelashes), so no
                    // tint. Ray Dream lit the sprites with an ambient 0.5
                    // grey plus one 0.5 light and a 0.5 highlight. The chase
                    // camera mostly sees his back, which that key light
                    // leaves at ambient only, so a weaker opposite light fills
                    // it in.
                    blupiEffect_->setDiffuseColorProperty(Microsoft::Xna::Framework::Vector3::One);
                    blupiEffect_->setAmbientLightColorProperty(
                        Microsoft::Xna::Framework::Vector3(0.45f, 0.45f, 0.45f));
                    keyLight.setDiffuseColorProperty(Microsoft::Xna::Framework::Vector3(0.55f, 0.55f, 0.55f));
                    keyLight.setSpecularColorProperty(Microsoft::Xna::Framework::Vector3(0.3f, 0.3f, 0.3f));
                    auto& fillLight = blupiEffect_->getDirectionalLight1Property();
                    fillLight.setEnabledProperty(true);
                    fillLight.setDiffuseColorProperty(Microsoft::Xna::Framework::Vector3(0.35f, 0.35f, 0.35f));
                    fillLight.setSpecularColorProperty(Microsoft::Xna::Framework::Vector3::Zero);
                    fillLight.setDirectionProperty(Microsoft::Xna::Framework::Vector3(0.4f, -0.6f, 0.7f));
                    blupiEffect_->setSpecularColorProperty(Microsoft::Xna::Framework::Vector3::One);
                    blupiEffect_->setSpecularPowerProperty(24.0f);
                }
                if (blupiModelIsReal_)
                {
                    // Expressions per clip frame and the clips facing the
                    // viewer (see blupiFaces_' declaration).
                    std::ifstream clipsFile("avatars3d/blupi/clips.txt");
                    std::string line;
                    while (std::getline(clipsFile, line))
                    {
                        std::istringstream in(line);
                        std::string kind;
                        in >> kind;
                        if (kind == "rate")
                        {
                            in >> blupiFaceRate_;
                        }
                        else if (kind == "part")
                        {
                            in >> blupiFacePart_;
                        }
                        else if (kind == "face")
                        {
                            std::string file;
                            in >> file;
                            blupiFaces_.emplace_back("avatars3d/blupi/" + file, device);
                        }
                        else if (kind == "clip")
                        {
                            std::string name;
                            in >> name;
                            std::vector<int> frames;
                            for (int face = 0; in >> face;)
                            {
                                frames.push_back(face);
                            }
                            blupiClipFaces_[name] = std::move(frames);
                        }
                        else if (kind == "viewer")
                        {
                            std::string name;
                            in >> name;
                            blupiViewerClips_.insert(name);
                        }
                        else if (kind == "vehicle")
                        {
                            std::string name;
                            in >> name;
                            blupiVehicles_.emplace_back(
                                name,
                                content.Load<std::shared_ptr<Microsoft::Xna::Framework::Graphics::SkinnedModelEXT>>(
                                    "blupi/vehicles/" + name + "/avatar"));
                        }
                    }

                    // The objects converted to 3D models (objectModelTable_):
                    // lit like him, so only with the real model.
                    std::ifstream objectIndex("avatars3d/objects/index.txt");
                    for (std::string name; std::getline(objectIndex, name);)
                    {
                        if (name.empty() || name[0] == '#')
                        {
                            continue;
                        }
                        std::ifstream icons("avatars3d/objects/" + name + "/icons.txt");
                        std::string error;
                        if (!objectModelTable_.AddModel(static_cast<int>(objectModels_.size()), icons, &error))
                        {
                            std::cerr << "GalaxyEggbertCNA: avatars3d/objects/" << name
                                      << "/icons.txt: bad line '" << error << "'" << std::endl;
                        }
                        objectModels_.push_back(
                            content.Load<std::shared_ptr<Microsoft::Xna::Framework::Graphics::SkinnedModelEXT>>(
                                "objects/" + name + "/avatar"));
                        // The electrode's sparks (Q-005; tools/rd4_to_gltf/electro_sparks.py).
                        for (int icon = 238; icon <= 242; ++icon)
                        {
                            const std::string spark =
                                "avatars3d/objects/" + name + "/sparks/" + std::to_string(icon) + ".png";
                            if (std::filesystem::exists(spark))
                            {
                                electrodeSparks_[icon] =
                                    std::make_unique<Microsoft::Xna::Framework::Graphics::Texture2D>(spark, device);
                            }
                        }
                        // A sign's painted face (sign_decals.py): decal <icon> <png> <x0> <y0> <x1> <y1> <z>.
                        std::ifstream decals("avatars3d/objects/" + name + "/decals.txt");
                        for (std::string line; std::getline(decals, line);)
                        {
                            std::istringstream fields(line);
                            std::string word, png;
                            int icon = 0;
                            float x0 = 0, y0 = 0, x1 = 0, y1 = 0, z = 0;
                            if (!(fields >> word >> icon >> png >> x0 >> y0 >> x1 >> y1 >> z) || word != "decal")
                            {
                                continue;
                            }
                            const std::string path = "avatars3d/objects/" + name + "/" + png;
                            if (!std::filesystem::exists(path))
                            {
                                continue;
                            }
                            SignDecal decal;
                            decal.texture = std::make_unique<Microsoft::Xna::Framework::Graphics::Texture2D>(path, device);
                            // Model space: 64 sprite pixels a block, the icon's centre the origin, +Y up.
                            decal.left = (x0 - 32.0f) / 64.0f;
                            decal.right = (x1 - 32.0f) / 64.0f;
                            decal.top = (32.0f - y0) / 64.0f;
                            decal.bottom = (32.0f - y1) / 64.0f;
                            decal.depth = z;
                            signDecals_[icon] = std::move(decal);
                        }
                    }
                    std::cout << "GalaxyEggbertCNA: " << objectModels_.size() << " object model(s) draw "
                              << objectModelTable_.Size() << " sprite icon(s) in 3D." << std::endl;
                    ApplyBlockModels();
                    // A terrain built before the models came leaves the still model icons in its
                    // mesh: build it again (the next frame).
                    --terrainRevision_;
                }
                blupiModelLoaded_ = true;
                std::cout << "GalaxyEggbertCNA: third-person model loaded — "
                          << (blupiModelIsReal_ ? "avatars3d/blupi/" : "avatars3d/blupi_placeholder/")
                          << " (F3 toggles third-/first-person)." << std::endl;
            }
            else
            {
                cameraMode_ = CameraMode::FirstPerson;
                std::cout << "GalaxyEggbertCNA: no third-person model found ("
                          << blupiModelManifest
                          << " not found) — third-person camera mode unavailable, staying first-person."
                          << std::endl;
            }
        }

        // Explicit .vwr start cells use raw-grid coordinates and are shifted
        // by WorldRuntime for rendering. Old worlds retain the historical
        // fixed sample-world default.
        if (worldRuntime_.HasExplicitSpawnPoint())
        {
            blupi_.SetPosition(
                worldRuntime_.GetSpawnRenderX(), worldRuntime_.GetSpawnRenderY(),
                worldRuntime_.GetSpawnRenderZ());
        }
        else
        {
            blupi_.SetPosition(0.0f, 1.0f, 0.0f);
        }

        // Real mobile-eggbert sound playback (2026-07-10, see Sound.hpp) --
        // loads whichever of Content/sounds/sound000.wav..sound092.wav
        // actually exist (already copied next to the binary by the
        // mobile-eggbert Content/ POST_BUILD step, same as icons/
        // backgrounds).
        sound_.LoadContent();
        std::cout << "GalaxyEggbertCNA: sound loaded — " << sound_.LoadedCount() << "/"
                  << Sound::kNumChannels << " channel(s)." << std::endl;

        // Minimal settings persistence (2026-07-13, plan.md MENU-067, see
        // SaveData.hpp) -- restores the one real, already-wired setting
        // (sound on/off) from the previous run, if any.
        saveData_.Load();
        sound_.SetEnabled(saveData_.GetSoundEnabled());
        sound_.SetEffectsVolume(static_cast<float>(saveData_.GetEffectsVolume()) / 100.0f);
        inputPad_.SetJumpRight(saveData_.GetJumpRight());

        // Real Wait->Resume-or-Init branch (plan.md MENU-001..020) now
        // decides this at the END of the real 5.0s Wait timer, in
        // Update(), not here at load time -- see phase_'s own default
        // member initializer (now Wait, not Play) and GalaxyEggbert::Def::GamePhase's own
        // class comment for the full reasoning.

        std::cout << "GalaxyEggbertCNA: terrain mesh uploaded — "
                  << terrainRenderer_->BlockCount() << " blocks ("
                  << terrainRenderer_->AnimatedBlockCount() << " animated, "
                  << terrainRenderer_->WaterBlockCount() << " water/semi-transparent), "
                  << terrainRenderer_->VertexCount() << " vertices, "
                  << terrainRenderer_->PrimitiveCount() << " triangles." << std::endl;
    }

    void GalaxyEggbertGame::RebuildWorldPresentation()
    {
        // Re-derive skyRegion/missionNumber/mobileObjects_ from the World
        // itself first (plan.md EDITOR-109): the world editor mutates the
        // live World in place, with no disk round-trip, so LoadFromVwrFile()
        // -- which used to be the only thing that populated these -- never
        // runs for an in-editor edit. Without this, an object placed in the
        // editor stays invisible until the world is saved and reloaded.
        // A no-op for every other caller, whose World hasn't changed since
        // it was loaded.
        worldRuntime_.ResyncFromWorld();
        bigDecorCells_.clear();
        for (const auto& cell : worldRuntime_.GetBigDecorCells())
        {
            bigDecorCells_.push_back({
                cell.worldX, cell.worldY, cell.worldZ, cell.icon});
        }

        auto& device = getGraphicsDeviceProperty();

        // Real mobile-eggbert background image for this world's skyRegion
        // (NEXT.md §3, 2026-07-09) -- direct filename formula, same
        // convention as mobile-eggbert's own Decor::LoadImages()
        // (05-backgrounds.md): "decor" + 3-digit zero-padded region id.
        // Content/backgrounds/ is already copied next to this binary (see
        // the mobile-eggbert Content/ POST_BUILD copy, CMakeLists.txt). Only
        // 28 of the 32 possible region ids (0-31) have a real file -- the 4
        // missing ones are confirmed never used by any real level
        // (05-backgrounds.md), but a hand-authored .vwr world could still
        // reference one, so this must degrade gracefully, not throw.
        {
            char backgroundPath[64];
            std::snprintf(backgroundPath, sizeof(backgroundPath),
                          "Content/backgrounds/decor%03u.png", worldRuntime_.GetSkyRegion());
            if (std::filesystem::exists(backgroundPath))
            {
                backgroundTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D(backgroundPath, device);
                backgroundEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
                backgroundEffect_->VertexColorEnabled = false;
                backgroundEffect_->setTextureEnabledProperty(true);
                backgroundEffect_->setTextureProperty(&backgroundTexture_);
                backgroundLoaded_ = true;
                std::cout << "GalaxyEggbertCNA: background loaded — " << backgroundPath << " ("
                          << backgroundTexture_.getWidthProperty() << "x"
                          << backgroundTexture_.getHeightProperty() << " px, region "
                          << worldRuntime_.GetSkyRegion() << ")." << std::endl;
            }
            else
            {
                backgroundLoaded_ = false;
                std::cout << "GalaxyEggbertCNA: no background image for region "
                          << worldRuntime_.GetSkyRegion() << " (" << backgroundPath
                          << " not found) — falling back to flat clear color." << std::endl;
            }
        }

        terrainDrawSecret_ = TerrainDrawSecret();
        terrainRenderer_ = std::make_unique<TerrainRenderer>(device, worldRuntime_.GetWorld(), terrainTileAtlas_, tileCoverage_,
                                                             terrainDrawSecret_, StillModelIcons(), TerrainReliefs());
        terrainRenderer_->SetReliefOverhangs(terrainReliefOverhangs_);
        terrainReliefWarm_ = false;
        ApplyBlockModels();
        terrainRevision_ = worldRuntime_.GetWorld().blockRevision();
        std::cout << "GalaxyEggbertCNA: world terrain built — " << terrainRenderer_->BlockCount() << " blocks, "
                  << terrainRenderer_->VertexCount() << " vertices." << std::endl;
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

    void GalaxyEggbertGame::RefreshTerrainMeshIfChanged()
    {
        // Blocks changed in play (a door opened, a hub door slid away), or the flight barrier's
        // visibility (ShowSecret, the editor): the terrain mesh is built from the blocks, so
        // build it again.
        if (terrainRenderer_ && (worldRuntime_.GetWorld().blockRevision() != terrainRevision_ ||
                                 TerrainDrawSecret() != terrainDrawSecret_))
        {
            terrainDrawSecret_ = TerrainDrawSecret();
            auto rebuilt =
                std::make_unique<TerrainRenderer>(getGraphicsDeviceProperty(), worldRuntime_.GetWorld(), terrainTileAtlas_,
                                                  tileCoverage_, terrainDrawSecret_, StillModelIcons(), TerrainReliefs());
            rebuilt->SetReliefOverhangs(terrainReliefOverhangs_);
            // The unchanged chunks keep their reliefs; the changed ones are built in the background.
            rebuilt->AdoptReliefs(*terrainRenderer_);
            terrainRenderer_ = std::move(rebuilt);
            ApplyBlockModels();
            terrainRevision_ = worldRuntime_.GetWorld().blockRevision();
        }
    }

    const GalaxyEggbert::Game::TerrainReliefTable* GalaxyEggbertGame::TerrainReliefs()
    {
        if (!terrainReliefsLoaded_)
        {
            terrainReliefsLoaded_ = true;
            const char* on = std::getenv("GALAXY_TERRAIN_RELIEF");
            if (on == nullptr || std::string(on) != "0")
            {
                const char* dir = std::getenv("GALAXY_TERRAIN_RELIEF_DIR");
                const int icons = terrainReliefs_.Load(dir != nullptr && *dir != '\0' ? dir : "avatars3d/terrain");
                std::cout << "GalaxyEggbertCNA: " << icons << " terrain tile(s) drawn as their relief near the camera."
                          << std::endl;
            }
            if (const char* radius = std::getenv("GALAXY_TERRAIN_RELIEF_RADIUS"))
            {
                terrainReliefRadius_ = std::max(0.0f, static_cast<float>(std::atof(radius)));
            }
            const char* overhang = std::getenv("GALAXY_TERRAIN_RELIEF_OVERHANG");
            terrainReliefOverhangs_ = overhang != nullptr && std::string(overhang) == "1";
        }
        return terrainReliefs_.Empty() ? nullptr : &terrainReliefs_;
    }

    std::vector<std::uint16_t> GalaxyEggbertGame::StillModelIcons() const
    {
        // The still terrain icons with a loaded model (BlockModels.hpp), drawn as it.
        std::vector<std::uint16_t> icons;
        std::vector<std::uint16_t> candidates(GalaxyEggbert::Game::kStillModelIcons.begin(),
                                              GalaxyEggbert::Game::kStillModelIcons.end());
        candidates.insert(candidates.end(), GalaxyEggbert::Game::kSignModelIcons.begin(),
                          GalaxyEggbert::Game::kSignModelIcons.end());
        for (const auto icon : candidates)
        {
            if (objectModelTable_.Find(ObjectTextureSource::ObjectM, icon) != nullptr)
            {
                icons.push_back(icon);
            }
        }
        return icons;
    }

    void GalaxyEggbertGame::ApplyBlockModels()
    {
        // The fan cells as the original's fan model (BlockModels.hpp, VIS-0127) when it is loaded;
        // without it (no avatars3d/objects) they stay textured cubes.
        if (!terrainRenderer_)
        {
            return;
        }
        std::vector<std::uint16_t> bases;
        if (objectModelTable_.Find(ObjectTextureSource::ObjectM, GalaxyEggbert::Game::kFanModelIcon) != nullptr)
        {
            bases.insert(bases.end(), GalaxyEggbert::Game::kFanModelBases.begin(),
                         GalaxyEggbert::Game::kFanModelBases.end());
        }
        // Animated tiles with their scene's model (BlockModels.hpp kAnimatedModelBases).
        for (const auto base : GalaxyEggbert::Game::kAnimatedModelBases)
        {
            if (objectModelTable_.Find(ObjectTextureSource::ObjectM, base) != nullptr)
            {
                bases.push_back(base);
            }
        }
        if (!bases.empty())
        {
            terrainRenderer_->SetModelDrawnBases(std::move(bases));
        }
        if (objectModelTable_.Find(ObjectTextureSource::ObjectM, GalaxyEggbert::BlockTypes::Saw) != nullptr)
        {
            terrainRenderer_->SetModelOverFloorBases(std::vector<std::uint16_t>(
                GalaxyEggbert::Game::kModelOverFloorBases.begin(), GalaxyEggbert::Game::kModelOverFloorBases.end()));
        }
    }

    bool GalaxyEggbertGame::TerrainDrawSecret() const noexcept
    {
        return cheatShowSecret_ || phase_ == GalaxyEggbert::Def::GamePhase::Editor;
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

    Microsoft::Xna::Framework::Input::KeyboardState GalaxyEggbertGame::CurrentKeyboardState() const
    {
        return autoplay_ ? autoplayKeys_ : ReadKeyboardState(SuppressLiveInput());
    }

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
        state.exitReached = interaction_.ExitReached();
        state.blitz = ((worldRuntime_.GetAnimPhase() % 100) + 100) % 100;
        state.temp = (((worldRuntime_.GetAnimPhase() / 4) % 20) + 20) % 20;
        state.crush = (((worldRuntime_.GetAnimPhase() / 3) % 10) + 10) % 10;
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

    void GalaxyEggbertGame::LoadMission(int missionNumber)
    {
        titleSceneActive_ = false;
        winCheerActive_ = false;
        pendingSkate_ = {};
        teleportArrivalGuard_ = false;
        accelWaitZero_ = true; // StartMission(): the first tilt waits for a level device
        char path[64];
        std::snprintf(path, sizeof(path), "worlds3d/world%03d.vwr", missionNumber);
        if (!worldRuntime_.LoadFromVwrFile(path))
        {
            std::cout << "GalaxyEggbertCNA: LoadMission(" << missionNumber << "): " << path
                       << " not found or failed to load — staying on the current mission." << std::endl;
            return;
        }

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
        blupi_ = BlupiController();
        wasInHelicopter_ = false;
        hideGhosts_.clear();
        hideGhostTracking_ = false;

        if (worldRuntime_.HasExplicitSpawnPoint())
        {
            blupi_.SetPosition(
                worldRuntime_.GetSpawnRenderX(), worldRuntime_.GetSpawnRenderY(),
                worldRuntime_.GetSpawnRenderZ());
        }
        else
        {
            blupi_.SetPosition(0.0f, 1.0f, 0.0f);
        }
        if (hubArrival)
        {
            blupi_.SetPosition(static_cast<float>(hubArrival->cell.x - WorldRuntime::kWorldCenterX),
                               static_cast<float>(hubArrival->cell.y),
                               static_cast<float>(hubArrival->cell.z - WorldRuntime::kWorldCenterZ));
            blupi_.SetYaw(hubArrival->yaw);
        }
        chaseCamera_.Reset(blupi_.GetYaw()); // a new world starts with the camera behind him
        cameraSmoothedInitialized_ = false;  // ... there at once, not easing over from the last world

        saveData_.SetMissionNumber(missionNumber);
        saveData_.Save();

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

        RebuildWorldPresentation();

        // Fresh defaults, no "preserve lives" concept (unlike LoadMission()) --
        // there's no real progression state to carry between play-test runs.
        interaction_ = InteractionSystem();
        blupi_ = BlupiController();
        wasInHelicopter_ = false;
        hideGhosts_.clear();
        hideGhostTracking_ = false;
        if (worldRuntime_.HasExplicitSpawnPoint())
        {
            blupi_.SetPosition(
                worldRuntime_.GetSpawnRenderX(), worldRuntime_.GetSpawnRenderY(),
                worldRuntime_.GetSpawnRenderZ());
        }
        else
        {
            // Preserve the established 3x3 starter-board default for old
            // custom worlds whose formerly-reserved header slots are zero.
            blupi_.SetPosition(
                GalaxyEggbert::Editor::kEditorStarterSpawnRenderX,
                GalaxyEggbert::Editor::kEditorStarterSpawnY,
                GalaxyEggbert::Editor::kEditorStarterSpawnRenderZ);
        }

        editorPlayTestActive_ = true;
        editorPlayTestWorldPath_ = path;
        SetPhase(GalaxyEggbert::Def::GamePhase::Play, /*bypassFade=*/true);

        std::cout << "GalaxyEggbertCNA: LoadCustomWorldForPlayTest(" << path << "): loaded." << std::endl;
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
        phase_ = next;
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

    void GalaxyEggbertGame::ResolvePendingVoyage()
    {
        const auto* voyageEvent = FindInteractionEvent(InteractionSystem::EventKind::VoyageRequested);
        if (!voyageEvent)
        {
            return;
        }
        const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
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

    namespace
    {
        // Puts `spec` in the first free slot of the world's mobile objects (or at the end).
        void PlaceMobileObject(GalaxyEggbert::Game::WorldRuntime& worldRuntime, const GalaxyEggbert::Game::MobileObjSpec& spec)
        {
            auto& objects = worldRuntime.GetMobileObjectsMutable();
            for (auto& slot : objects)
            {
                if (!slot.active)
                {
                    slot = spec;
                    return;
                }
            }
            objects.push_back(spec);
        }
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
        using Microsoft::Devices::Sensors::Accelerometer;
        const bool wanted = saveData_.GetAccelActive() && !autoplay_ && Accelerometer::getIsSupportedProperty();
        if (wanted && !accel_)
        {
            try
            {
                accel_ = std::make_unique<Accelerometer>();
                accel_->Start();
            }
            catch (...)
            {
                accel_.reset(); // no permission or no sensor after all (the original's StartAccel)
                return 0.0f;
            }
            accelLastState_ = false;
            accelWaitZero_ = true;
        }
        else if (!wanted && accel_)
        {
            accel_->Stop();
            accel_.reset();
        }
        if (!accel_ || !accel_->getIsDataValidProperty())
        {
            return 0.0f;
        }
        // HandleAccelSensorCurrentValueChanged: a dead zone from the sensitivity (GameData data[7],
        // 50 by default; the Setup's slider), narrower while already tilted, a speed from a quarter
        // up; nothing until the device has once been level (accelWaitZero).
        const float kSensitivity = static_cast<float>(saveData_.GetAccelSensitivity()) / 100.0f;
        const float y = accel_->getCurrentValueProperty().getAccelerationProperty().Y;
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
        accelLastState_ = speed != 0.0f;
        if (accelWaitZero_)
        {
            if (speed == 0.0f)
            {
                accelWaitZero_ = false;
            }
            return 0.0f;
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
            inWorld && sound_.IsEnabled() && !autoplay_ ? worldRuntime_.GetWorld().musicTrack() : 0u;
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
        if (shouldRespawn)
        {
            blupi_.SetPosition(blupi_.GetValidX(), blupi_.GetValidY(), blupi_.GetValidZ());
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

        const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
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

        const auto* powerEvent = FindInteractionEvent(InteractionSystem::EventKind::PowerGranted);
        const auto* hideEvent = FindInteractionEvent(InteractionSystem::EventKind::HideGranted);
        const auto* cloudEvent = FindInteractionEvent(InteractionSystem::EventKind::CloudGranted);

        // Starts a new freeze for whichever of the 3 real 2-stage pickups was touched this frame
        // (real immediate "grab" sound: Sucette ch50, Drink ch57, Charge ch58, Decor.cpp:
        // 6025-6087) -- the world object was already destroyed by interaction_.Update() itself.
        if (powerEvent && blupi_.TriggerPickupFreeze(BlupiController::PickupFreezeKind::Sucette))
        {
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel50);
            pendingPickupX_ = powerEvent->pickupX;
            pendingPickupY_ = powerEvent->pickupY;
            pendingPickupZ_ = powerEvent->pickupZ;
            pendingPickupType_ = GalaxyEggbert::Def::ObjectType::ObjectType26;
        }
        else if (hideEvent && blupi_.TriggerPickupFreeze(BlupiController::PickupFreezeKind::Drink))
        {
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel57);
            pendingPickupX_ = hideEvent->pickupX;
            pendingPickupY_ = hideEvent->pickupY;
            pendingPickupZ_ = hideEvent->pickupZ;
            pendingPickupType_ = GalaxyEggbert::Def::ObjectType::ObjectType30;
        }
        else if (cloudEvent && blupi_.TriggerPickupFreeze(BlupiController::PickupFreezeKind::Charge))
        {
            sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel58);
            // Real m_blupiCloud grants at CONTACT, not completion (confirmed via direct source
            // read, unlike Sucette/Drink) -- this engine's existing instant TriggerCloud() call
            // below (already correct) stays; only the freeze/grab-sound/completion-sound were
            // missing. Real source also redundantly re-arms the gauge again at the 64-tick
            // completion (same values) -- NOT re-applied here, a documented, minor simplification
            // (the gauge decays for those ~3.2s during the freeze instead of being refreshed).
            blupi_.TriggerCloud();
            pendingPickupX_ = cloudEvent->pickupX;
            pendingPickupY_ = cloudEvent->pickupY;
            pendingPickupZ_ = cloudEvent->pickupZ;
            pendingPickupType_ = GalaxyEggbert::Def::ObjectType::ObjectType31;
        }

        // Resolves an ALREADY-active freeze (possibly started a prior frame) that just elapsed --
        // grants the real deferred buff (Sucette/Drink only -- Cloud already granted above),
        // plays the real "complete" sound (ch44/ch62/ch55), and re-spawns the item.
        BlupiController::PickupFreezeKind kind{};
        if (!blupi_.ConsumePickupFreezeResolved(kind))
        {
            return;
        }
        switch (kind)
        {
            case BlupiController::PickupFreezeKind::Sucette:
                blupi_.TriggerPower();
                sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel44);
                // Real m_blupiPosMagic reset (plan.md VISUAL-011-adjacent) -- moved here from the
                // old instant-grant site since the Power buff (and thus the magic trail) only
                // actually starts now, at completion, not at contact.
                interaction_.ResetMagicTrail(blupi_.GetX(), blupi_.GetY(), blupi_.GetZ());
                break;
            case BlupiController::PickupFreezeKind::Drink:
                blupi_.TriggerHide();
                sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel62);
                break;
            case BlupiController::PickupFreezeKind::Charge:
                // The clip's end gives the Cloud (Decor.cpp:3048-3057): already on after a
                // pickup's contact, the cheat's only source (powercharge).
                blupi_.TriggerCloud();
                sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel55);
                break;
        }
        // A cheat's clip had no pickup to put back (ObjectType0).
        if (pendingPickupType_ != GalaxyEggbert::Def::ObjectType::ObjectType0)
        {
            interaction_.RespawnPickupItem(worldRuntime_, pendingPickupX_, pendingPickupY_, pendingPickupZ_,
                                            pendingPickupType_);
        }
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
        int patrolIcon = 0;
        if (sheet == ObjectTextureSource::Element &&
            TryGetPatrolIcon(obj.type, obj.posStartX > obj.posEndX, obj.patrolStep,
                             static_cast<int>(obj.patrolTime), patrolIcon))
        {
            icon = patrolIcon;
        }
        return true;
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

    void GalaxyEggbertGame::ApplyCheat(int cheatNumber)
    {
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
                saveData_.Save();
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
        using TC = GalaxyEggbert::Game::InputPad::TypedCheat;
        using VM = BlupiController::VehicleMode;
        using GalaxyEggbert::Def::SoundChannel;
        const auto vehicle = [this](VM mode)
        {
            blupi_.CheatVehicle(mode);
        };
        // Sucette/Charge/Drink without a pickup: the clip, then the power (ResolvePickupFreeze(),
        // no item to put back).
        const auto action = [this](BlupiController::PickupFreezeKind kind, SoundChannel sound)
        {
            blupi_.CheatClearForAction();
            if (blupi_.TriggerPickupFreeze(kind))
            {
                sound_.Play(sound);
                pendingPickupType_ = GalaxyEggbert::Def::ObjectType::ObjectType0;
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
            case TC::GiveCopter: vehicle(VM::Helicopter); break;
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
            case TC::Ghost:
            case TC::Quick:
            case TC::None:
                break; // ghost and quick: handled by the caller
        }
    }

    void GalaxyEggbertGame::Update(Microsoft::Xna::Framework::GameTime& gameTime)
    {
        Game::Update(gameTime);
        UpdateMusic();

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
            LoadMission(startMission_);
            startMission_ = 0;
            if (startSpawnSet_)
            {
                blupi_.SetPosition(startSpawnX_ - WorldRuntime::kWorldCenterX, startSpawnY_,
                                   startSpawnZ_ - WorldRuntime::kWorldCenterZ);
                blupi_.SetYaw(startYawDegrees_ * 3.14159265f / 180.0f);
                chaseCamera_.Reset(blupi_.GetYaw());
                cameraSmoothedInitialized_ = false;
            }
        }

        // Real `phaseTime` (see phaseTimeSeconds_'s own member comment) --
        // incremented unconditionally every Update() tick, regardless of
        // phase, matching the real source's own `phaseTime++` placement.
        if (autoplay_ && !autoplay_->Finished())
        {
            StepAutoplay();
            if (autoplayShotEvery_ > 0 && autoplayTick_ % autoplayShotEvery_ == 0)
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
        const float dt = autoplay_ ? 1.0f / 60.0f
                                   : static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());
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
        {
            using Microsoft::Xna::Framework::Input::ButtonState;
            using Microsoft::Xna::Framework::Input::Keys;
            const auto phaseKeys = CurrentKeyboardState();
            const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();

            bool mousePausePressed = false;
            bool mouseContinuePressed = false;
            bool mouseRestartPressed = false;
            bool mouseSetupPressed = false;
            bool mouseMenuPressed = false;
            bool mouseBackPressed = false;
            if (phase_ == GalaxyEggbert::Def::GamePhase::Play)
            {
                // Several fingers at once (owner 2026-11-02: touch devices). While fingers are on
                // the screen, and a few frames after, the mouse is left out: the platform also
                // turns a touch into mouse events, which would press Action twice.
                std::vector<GalaxyEggbert::Game::InputPad::TouchPoint> touches;
                if (!SuppressLiveInput())
                {
                    using Microsoft::Xna::Framework::Input::Touch::TouchLocationState;
                    for (const auto& t : Microsoft::Xna::Framework::Input::Touch::TouchPanel::GetState())
                    {
                        const auto state = t.getStateProperty();
                        if (state == TouchLocationState::Invalid)
                        {
                            continue;
                        }
                        GalaxyEggbert::Game::InputPad::TouchPoint p;
                        p.id = t.getIdProperty();
                        p.x = t.getPositionProperty().X;
                        p.y = t.getPositionProperty().Y;
                        p.pressed = state == TouchLocationState::Pressed;
                        p.released = state == TouchLocationState::Released;
                        touches.push_back(p);
                    }
                }
                touchQuietFrames_ = touches.empty() ? std::max(0, touchQuietFrames_ - 1) : 15;
                const auto mouse = touchQuietFrames_ > 0 ? Microsoft::Xna::Framework::Input::MouseState{}
                                                         : ReadMouseState(SuppressLiveInput());
                inputPadClaimedMouse = inputPad_.UpdatePlay(
                    mouse, viewport.getWidthProperty(), viewport.getHeightProperty(), padPlayInput);
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
                    padPlayInput.actionPressed = padPlayInput.actionPressed || touchInput.actionPressed;
                    padPlayInput.pausePressed = padPlayInput.pausePressed || touchInput.pausePressed;
                    padPlayInput.backUpHeld = padPlayInput.backUpHeld || touchInput.backUpHeld;
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
                const auto typedCheat = inputPad_.UpdateTypedGhostCheat(phaseKeys, /*isPlayPhase=*/true);
                ApplyTypedCheat(typedCheat.cheat);
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
                if (f12Down && !f12KeyWasDown_)
                {
                    cheatMenuShown_ = !cheatMenuShown_;
                }
                f12KeyWasDown_ = f12Down;
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::Pause)
            {
                const auto mouse = ReadMouseState(SuppressLiveInput());
                // Real conditional visibility: Back/Restart hidden on
                // mission 1 (can't go "back" from/replay the very first
                // level); Restart additionally hidden on decade-boundary
                // missions (real `mission % 10 != 0` gate).
                const int mission = worldRuntime_.GetMissionNumber();
                const bool showBack = mission != 1;
                const bool showRestart = mission != 1 && mission % 10 != 0;
                const auto pauseInput = inputPad_.UpdatePause(
                    mouse, viewport.getWidthProperty(), viewport.getHeightProperty(), showBack, showRestart);
                mouseContinuePressed = pauseInput.continuePressed;
                mouseRestartPressed = pauseInput.restartPressed;
                mouseSetupPressed = pauseInput.setupPressed;
                mouseMenuPressed = pauseInput.menuPressed;
                mouseBackPressed = pauseInput.backPressed;
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
                const auto mouse = ReadMouseState(SuppressLiveInput());
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
                    saveData_.Save();
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
                    saveData_.Save();
                    // A sample of the effects at the new volume (the Sounds toggle's own click).
                    sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel0);
                }
                // The other toggles (OR-0010), saved at once as the original's GameData::Write().
                if (setupInput.jumpToggled)
                {
                    saveData_.SetJumpRight(!saveData_.GetJumpRight());
                    inputPad_.SetJumpRight(saveData_.GetJumpRight());
                    saveData_.Save();
                }
                if (setupInput.zoomToggled)
                {
                    saveData_.SetAutoZoom(!saveData_.GetAutoZoom());
                    saveData_.Save();
                }
                if (setupInput.accelToggled)
                {
                    saveData_.SetAccelActive(!saveData_.GetAccelActive());
                    saveData_.Save();
                }
                if (setupInput.resetPressed)
                {
                    // Real SetupReset: `gameData.Reset(); gameData.Write();`
                    // -- the SAME full reset as Cheat5 (confirmed
                    // 2026-07-13 via Game1.cpp), only reachable from
                    // MainSetup (isMainSetup gates its rect/visibility).
                    saveData_.Reset();
                    saveData_.Save();
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
                    // Real Wait->Resume-or-Init branch -- ADAPTED trigger
                    // (see GalaxyEggbert::Def::GamePhase's own comment for the full
                    // real-vs-adapted reasoning, same one already
                    // established for Resume itself).
                    SetPhase(saveData_.GetHasProgress() ? GalaxyEggbert::Def::GamePhase::Resume
                                                         : GalaxyEggbert::Def::GamePhase::Init);
                }
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::Init)
            {
                // Real Init / gamer-select menu (2026-07-13, plan.md
                // MENU-006..020) -- see InputPad::UpdateInit()/
                // DrawInit()'s own class comment for full detail.
                const auto mouse = ReadMouseState(SuppressLiveInput());
                auto initInput = inputPad_.UpdateInit(
                    mouse, viewport.getWidthProperty(), viewport.getHeightProperty());
                // Enter (a gamepad's A, GamepadInput.hpp) plays as the Play button does: the menu
                // without a pointer (owner 2026-11-02: gamepads).
                {
                    const bool enterDown = ReadKeyboardState(SuppressLiveInput()).IsKeyDown(
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
                    saveData_.Save();
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
                // adapted-trigger reasoning; this is just the two real
                // buttons' behavior once already in the phase.
                const auto mouse = ReadMouseState(SuppressLiveInput());
                const auto resumeInput = inputPad_.UpdateResume(
                    mouse, viewport.getWidthProperty(), viewport.getHeightProperty());
                if (resumeInput.continuePressed)
                {
                    // Real ResumeContinue -> ContinueMission() ->
                    // SetPhase(Play,-2): restores the checkpointed
                    // mission (2026-07-17, now that SaveData's own
                    // already-persisted missionNumber -- written at every
                    // Win/Lost checkpoint -- is finally read back) and
                    // lives (no real mid-level position/treasure/key state
                    // exists to restore beyond that, same simplification
                    // as PauseRestart/WinLostReturn). The real `-2` mission
                    // sentinel BYPASSES the fade-defer mechanism entirely
                    // (plan.md MENU-088/089, confirmed via research) --
                    // Resume->Play is genuinely instant in the real game.
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
                const auto mouse = ReadMouseState(SuppressLiveInput());
                if (worldEditor_.IsBrowsing())
                {
                    const auto request = worldEditor_.UpdateBrowsing(
                        mouse, viewport.getWidthProperty(), viewport.getHeightProperty(),
                        phaseKeys.IsKeyDown(Keys::Escape));
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
                        // A new world in the empty slot the player clicked.
                        std::error_code ec;
                        std::filesystem::create_directories(request.createPath.parent_path(), ec);
                        GalaxyEggbert::Editor::CreateEditorStarterWorld().saveToFile(request.createPath);
                        LoadCustomWorldForEditing(request.createPath);
                    }
                }
                else
                {
                    worldEditor_.Update(phaseKeys, mouse, dt,
                                         viewport.getWidthProperty(), viewport.getHeightProperty(), camera_,
                                         worldRuntime_.GetWorldMutable());
                    cameraTarget_ = worldEditor_.CameraTarget();
                    // Everything animates while editing, as in blupi_recon's editor (OR-0013): the
                    // tiles' and the objects' animation clocks run. WorldRuntime::Update() moves
                    // nothing (patrols are InteractionSystem's, in play only), so what is placed
                    // stays where it was put.
                    worldRuntime_.Update(dt);
                    if (terrainRenderer_)
                    {
                        terrainRenderer_->Update(getGraphicsDeviceProperty(), worldRuntime_.GetAnimPhase(), BlupiFocusRaw());
                    }
                    if (worldEditor_.ConsumeNeedsPresentationRebuild())
                    {
                        RebuildWorldPresentation();
                    }
                    if (worldEditor_.ConsumePlayTestRequested())
                    {
                        // Update()'s own PlayTest handling already saved
                        // the world to GetWorldPath() before requesting
                        // this (plan.md EDITOR-108).
                        LoadCustomWorldForPlayTest(worldEditor_.GetWorldPath());
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
                    SetPhase(GalaxyEggbert::Def::GamePhase::Play);
                }
            }
            pauseKeyWasDown_ = pausePressed;

            if (mouseContinuePressed)
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
                const auto mouse = ReadMouseState(SuppressLiveInput());
                const bool mouseReturnPressed = inputPad_.UpdateWinLost(
                    mouse, viewport.getWidthProperty(), viewport.getHeightProperty());
                const bool returnPressed = phaseKeys.IsKeyDown(Keys::Space) || mouseReturnPressed;
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
                                blupi_.SetPosition(worldRuntime_.GetSpawnRenderX(), worldRuntime_.GetSpawnRenderY(),
                                                   worldRuntime_.GetSpawnRenderZ());
                            }
                            else
                            {
                                blupi_.SetPosition(0.0f, 1.0f, 0.0f);
                            }
                            cameraSmoothedInitialized_ = false;
                        }
                        SetPhase(GalaxyEggbert::Def::GamePhase::Play);
                    }
                }
                phaseReturnKeyWasDown_ = returnPressed;
            }
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

        if (terrainRenderer_)
        {
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
            const auto keys = CurrentKeyboardState();
            float turnInput = 0.0f;
            float moveInput = 0.0f;
            if (keys.IsKeyDown(Keys::Left))  turnInput -= 1.0f;
            if (keys.IsKeyDown(Keys::Right)) turnInput += 1.0f;
            if (keys.IsKeyDown(Keys::Up))    moveInput += 1.0f;
            if (keys.IsKeyDown(Keys::Down))  moveInput -= 1.0f;
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
            const bool pullingStance =
                crouchHeld && (holdingCrate_ || InteractionSystem::CrateInFront(
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
                if ((keys.IsKeyDown(Keys::RightControl) || padPlayInput.backUpHeld) && turnAroundRemaining_ <= 0.0f &&
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
            if (blupi_.IsInVehicle() && !blupi_.IsOneShotAnimPlaying() && !blupi_.IsTeleporting() &&
                !blupi_.IsDeathLocked() &&
                GalaxyEggbert::BlockTypes::isWater(blupi_.GetBlockTypeAt(worldRuntime_.GetWorld())))
            {
                WreckVehicle();
            }
            const bool canEnterWater = !blupi_.IsInVehicle() && !blupi_.IsBallooned() && !blupi_.IsEcrased();
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
                const bool canHold = crouchHeld && blupi_.IsOnGround() && !blupi_.IsInVehicle() &&
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
            blupi_.Step(worldRuntime_.GetWorld(), turnInput, moveInput, jumpPressed,
                        crouchHeld, lookUpHeld, simDt, tempPassable, inSurfWater, inDeepWater,
                        wasPushingCrate_, holdingCrate_);
            TickPendingSkate(simDt);
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
            if (blupi_.HelicopterHitsCeiling(worldRuntime_.GetWorld(), tempPassable) && !blupi_.IsDeathLocked() &&
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
                LoadMission(byePendingTarget_);
                return;
            }

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
                const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
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
            // GetGroundBlockType() already gates on IsOnGround() -- Lava
            // tiles are solid/walkable-on in this engine (Blupi stands on
            // lava rather than falling through it, matching the real 2D
            // game -- see BlockTypes.hpp's isMobileTransparent() comment on
            // why Lava is deliberately kept solid despite being
            // quart-passable in the real source).
            if (!blupi_.IsInvincible() &&
                blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Lava)
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
            // 379) is a separate BlockTypes value, so GetGroundBlockType()'s
            // exact match already excludes it with no extra check needed. A
            // saw starts active or stopped per however the world was
            // authored; TryActivateSwitch() below is what flips it between
            // the two.
            // m_blupiFocus too (Decor.cpp:5521-5522; BD-0066).
            if (!blupi_.IsInvincible() && !blupi_.HasVehicleHazardImmunity() && BlupiFocused() &&
                (blockUnder == GalaxyEggbert::BlockTypes::Saw || blockHere == GalaxyEggbert::BlockTypes::Saw))
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
                        worldRuntime_.GetMissionNumber(), GalaxyEggbert::BlockTypes::worldSelectIndex(groundBlock));
                    if (blupi_.TriggerBye())
                    {
                        byePendingTarget_ = target;
                        const float dx = camera_.Position().X - blupi_.GetX();
                        const float dz = camera_.Position().Z - blupi_.GetZ();
                        blupi_.SetYaw(std::atan2(dx, -dz));
                        sound_.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel32);
                    }
                    return;
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
            const bool actionPressedEdge = actionPressed && !actionKeyWasDown_;
            // Whether a switch, dynamite, the decoy or a vehicle used this press: if none did (nor a
            // dynamite stick taken in interaction_.Update()), he shakes his head (Non, below).
            bool actionUsed = false;
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
            actionKeyWasDown_ = actionPressed;

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
            // Real Tank "Fire" (2026-07-13, plan.md BULLET-001) -- a
            // dedicated key (real `GalaxyEggbert::Def::KeyPressFlags::Fire`), NOT the Action
            // button used for dynamite/Perso/switches/vehicle mount above.
            // "F" is this engine's own keyboard pick (no real keyboard
            // binding exists to match -- WP7 touch-only). Level state, not
            // edge-triggered (see InteractionSystem::Update()'s own
            // comment for why). The tank fires ahead; the helicopter drops
            // glue, only in the air (2026-10-26, Decor.cpp:3694-3708: flag2,
            // no ground under him and no lift).
            const bool firePressed = keys.IsKeyDown(Keys::F);
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

            // Ghost mode (plan.md BLUPI-111) real "no interactions"
            // behavior: real `MoveObjectDetect()` (the shared query nearly
            // every pickup/hazard/enemy-contact/lift-riding check in this
            // function is ultimately built on) unconditionally returns "no
            // object found" while ghosting -- every check in this
            // function is a proximity test against Blupi's own position
            // (see this class's own repeated "same simplification as
            // every other proximity test in this file" comments), so
            // substituting a position far outside the 100x100 world makes
            // every one of them correctly fail shut, with zero changes
            // needed inside InteractionSystem itself. Patrol/animation
            // logic (which never references Blupi's position at all)
            // continues normally, matching real behavior exactly.
            constexpr float kGhostSentinelPos = 100000.0f;
            const float interactionBlupiX = blupi_.IsGhost() ? kGhostSentinelPos : blupi_.GetX();
            const float interactionBlupiY = blupi_.IsGhost() ? kGhostSentinelPos : blupi_.GetY();
            const float interactionBlupiZ = blupi_.IsGhost() ? kGhostSentinelPos : blupi_.GetZ();
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
                if (crateOut.holdBlupi && !blupi_.IsDeathLocked())
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
            interaction_.Update(simDt, worldRuntime_, interactionBlupiX, interactionBlupiY, interactionBlupiZ,
                                 crateIntentDX, sound_, crouchHeld,
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
                                 BlupiFocused(), BlupiOutOfPlay());
            RefreshTerrainMeshIfChanged();

            // The action button with nothing to do, standing still: he shakes his head (Non, 18
            // ticks, channel 27; Decor.cpp:6366-6372, 3188). Moving cuts it short.
            if (actionPressedEdge && !actionUsed && !interaction_.ActionPressSpent() &&
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

            // Pollution puff (plan.md VISUAL-013) -- called unconditionally
            // every frame, matching the real function's own internal gate
            // (see TickPollutionPuff()'s own comment). isMoving reuses the
            // same blupiXBeforeStep delta already computed above for the
            // interaction system's crate-push direction inference, instead
            // of adding a new horizontal-speed accessor to BlupiController.
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
            // called unconditionally every frame; TickMagicTrail() itself
            // is the gate (a no-op unless Shield or Power is active).
            interaction_.TickMagicTrail(worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(),
                                        secretPower == GalaxyEggbert::Def::SecretPower::Shield,
                                        secretPower == GalaxyEggbert::Def::SecretPower::Power);

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
            // helicopter kicks up as it goes (Decor.cpp:3067, -10 a tick: its climbing speed).
            if (FindInteractionEvent(InteractionSystem::EventKind::GlueDropStarted))
            {
                blupi_.TriggerOneShotAnim(BlupiController::AnimState::HelicoGlu,
                                           BlupiController::kHelicoGluDuration);
            }
            if (FindInteractionEvent(InteractionSystem::EventKind::GlueDropped))
            {
                blupi_.KickUp(BlupiController::kHelicopterAscendSpeed);
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
                    saveData_.Save();
                }
                SetPhase(GalaxyEggbert::Def::GamePhase::Lost);
            }
            else if (interaction_.ExitReached() && !winCheerActive_)
            {
                if (!editorPlayTestActive_)
                {
                    saveData_.SetLives(interaction_.Lives());
                    saveData_.SetMissionNumber(worldRuntime_.GetMissionNumber());
                    saveData_.SetHasProgress(true);
                    saveData_.Save();
                }
                // The original's exit (Decor.cpp:6160-6180, 6411; Game1::Update): Blupi leaves
                // his helicopter, cheers facing the viewer for 40 ticks and the next world loads
                // (the hub, or the main hub after a golden key) -- no win screen, as the video
                // shows. Only the true ending (199's exit) has the Win screen; play-tests and the
                // showcase keep it to get back to the editor or the menu.
                const int mission = worldRuntime_.GetMissionNumber();
                if (!editorPlayTestActive_ && mission != 199 && mission != 999)
                {
                    if (blupi_.IsInVehicle())
                    {
                        blupi_.TriggerDismount();
                    }
                    winCheerActive_ = true;
                    if (blupi_.TriggerOneShotAnim(BlupiController::AnimState::Win, BlupiController::kWinDuration))
                    {
                        const float dx = camera_.Position().X - blupi_.GetX();
                        const float dz = camera_.Position().Z - blupi_.GetZ();
                        blupi_.SetYaw(std::atan2(dx, -dz));
                    }
                }
                else
                {
                    SetPhase(GalaxyEggbert::Def::GamePhase::Win);
                }
            }
            if (winCheerActive_ && !blupi_.IsOneShotAnimPlaying())
            {
                winCheerActive_ = false;
                AdvanceAfterWin();
                return;
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

            // F11 fullscreen toggle (2026-07-10, user request), edge-
            // detected like F3 above.
            const bool fullscreenKeyDown = keys.IsKeyDown(Keys::F11);
            if (fullscreenKeyDown && !fullscreenKeyWasDown_ && graphics_)
            {
                graphics_->ToggleFullScreen();
            }
            fullscreenKeyWasDown_ = fullscreenKeyDown;

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
                const auto mouse = ReadMouseState(SuppressLiveInput());
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
                const float kChaseDistance = 4.0f / chaseZoom_;
                constexpr float kChaseLookHeight = 1.0f;
                // Base elevation reproduces the previous fixed offset
                // (height 1.2 over distance 4); mouse-look pitch orbits the
                // camera up/down around Blupi on top of it (drag down =
                // camera rises, view tilts down -- same direction sense as
                // the first-person branch).
                constexpr float kBaseElevation = 0.29f;
                const float elevation = std::clamp(kBaseElevation + lookPitchOffset_, -1.2f, 1.45f);
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
                blupiClipTimeSeconds_ = charred ? 0.0 : blupiClipTimeSeconds_ + static_cast<double>(dt);
                // Air and Down stop at their last icon (BlupiController::GetAnimHoldFrame()): the
                // clip's key k sits at k / rate seconds.
                if (const int hold = blupi_.GetAnimHoldFrame(); blupiModelIsReal_ && hold >= 0 && blupiFaceRate_ > 0.0)
                {
                    blupiClipTimeSeconds_ = std::min(blupiClipTimeSeconds_, static_cast<double>(hold) / blupiFaceRate_);
                }
            }
        }

        if (goldenCaptureMode_ && goldenCaptureNextIndex_ < kGoldenCaptureCount &&
            goldenCaptureTick_ >= kGoldenCaptureTicks[goldenCaptureNextIndex_])
        {
            goldenCaptureAwaitingDraw_ = true;
        }
    }

    void GalaxyEggbertGame::Draw(const Microsoft::Xna::Framework::GameTime& gameTime)
    {
        (void)gameTime;
        auto& device = getGraphicsDeviceProperty();
        // The Clear2 death's soul drawn as its 3D model this frame: the HUD leaves out its sprite.
        bool soulDrawnIn3D = false;

        // Root-caused 2026-07-13 (NEXT.md §5's "screenshot_hud.png blue-
        // background anomaly", Vulkan-only): calling GetBackBufferData()
        // mid-frame -- as the terrain-visibility diagnostic below does --
        // corrupts the REST of that same frame's rendering on CNA's Vulkan
        // backend specifically (subsequent billboards lose alpha blending
        // and render as opaque white boxes, and the terrain itself reads
        // back as flat blue by the time the end-of-frame HUD screenshot
        // below captures it). Confirmed by temporarily disabling the
        // mid-frame readback: the HUD screenshot rendered perfectly on
        // Vulkan once that mid-frame GetBackBufferData call was removed.
        // Not fixable here (the bug is inside ../cna's Vulkan backend,
        // which needs separate approval to modify) -- worked around by
        // simply never letting both one-shot diagnostics fire within the
        // same frame: drawFrameIndex_ lets the HUD screenshot wait for a
        // frame strictly after whichever frame the terrain diagnostic
        // fired on (a fresh frame that never called GetBackBufferData
        // mid-draw), so it always captures an uncorrupted frame.
        ++drawFrameIndex_;

        // Dark navy (2026-07-18, user-reported: pillarbox margins around
        // Wait's/Init's menu screens should be dark blue, not the generic
        // XNA/MonoGame "CornflowerBlue" template default this was
        // previously left at). Matches Content/backgrounds/init.png's own
        // corner pixel color exactly (sampled directly: RGB(0,35,98)), so
        // the pillarbox seamlessly matches that background's own edge tone
        // instead of showing a visible seam between a lighter placeholder
        // blue and the actual (darker) menu artwork.
        //
        // Clears the DEPTH buffer as well, and must keep doing so: this is the
        // only Clear() in the whole frame, so without it stale depth values
        // survive from frame to frame and every depth-tested draw below --
        // terrain, billboards, the background quad, the whole 3D world -- gets
        // rejected, leaving just this clear colour under the depth-test-free
        // HUD (`SetDepthTestEnabled(false)` further down).
        //
        // That is exactly what happened (user-reported 2026-08-21: "nevidim 3d
        // svet, misto toho modrou barvu"). This used to read
        // `Clear(0.0f, 0.1373f, 0.3843f, 1.0f)`, and on EasyGL that four-float
        // overload cleared depth too -- as an undocumented side effect the
        // engine has since removed on purpose (`../cna` REMED-GFX-142,
        // 2026-07-28: "COLOUR ONLY ... it used to add ClearFlags::Depth -- so
        // asking XNA to clear only the colour target silently wiped the depth
        // buffer with it"). Upstream is right and this code was the one leaning
        // on the side effect, so the fix belongs here.
        //
        // The single-argument overload is real XNA's own
        // `Target | DepthBuffer | Stencil` at the viewport's MaxDepth, so it
        // states the intent instead of re-deriving flags. RGB(0,35,98) is the
        // same colour as the float triple it replaces (0.1373*255 = 35,
        // 0.3843*255 = 98), still init.png's sampled corner pixel.
        device.Clear(Microsoft::Xna::Framework::Color(0, 35, 98));
        device.SetDepthTestEnabled(true);

        // Reported live (2026-07-13): the 3D world was visible bleeding
        // through the pillarbox margins around Wait's loading-gauge screen
        // and Init's gamer-select menu -- both occur before any Play
        // session exists (Wait is a fake progress screen since
        // LoadContent() already loaded everything up front; Init is the
        // gamer-select menu, entered before any level/world is actually
        // being played), so unlike Pause/Win/Lost/Setup/Resume (which
        // legitimately freeze and show the in-progress game world behind
        // their overlay), there is no real game world to show here at all.
        // Skipping this whole block for those two phases removes the
        // bleed-through entirely instead of just papering over it with an
        // opaque quad. The Editor phase's own browser screen (plan.md
        // EDITOR-107, worldEditor_.IsBrowsing()) is the same situation --
        // no world is loaded yet while browsing -- so it's skipped too;
        // once a world is actually being edited, IsBrowsing() is false and
        // this block draws it same as any other phase.
        // The menu (Init) shows the world when it is Galaxy's title scene.
        if ((phase_ != GalaxyEggbert::Def::GamePhase::Wait || titleSceneActive_) &&
            (phase_ != GalaxyEggbert::Def::GamePhase::Init || titleSceneActive_) &&
            !(phase_ == GalaxyEggbert::Def::GamePhase::Editor && worldEditor_.IsBrowsing() && !titleSceneActive_))
        {
        // Real background image backdrop (NEXT.md §3, 2026-07-09) -- one
        // huge camera-facing billboard quad placed far behind the scene
        // (see backgroundMeshRenderer_'s header comment for why this
        // replaced an earlier SpriteBatch attempt). Sized so its edges sit
        // outside the view frustum at kBackgroundDistance regardless of
        // window aspect ratio (kBackgroundMargin > 1 covers the
        // diagonal/aspect slop), so it always fills the whole screen behind
        // real geometry. Rebuilt every frame like the other billboards
        // since it must keep following the camera. Falls back to the flat
        // device.Clear() color above when no background loaded
        // (backgroundLoaded_ false -- see LoadContent()).
        if (backgroundLoaded_ && backgroundEffect_)
        {
            const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.View());
            const auto cameraRight = invView.getRightProperty();
            const auto cameraUp = invView.getUpProperty();

            constexpr float kBackgroundDistance = 900.0f; // < camera_'s 1000.0f far plane
            constexpr float kBackgroundMargin = 1.3f;
            const float halfHeight = kBackgroundDistance *
                std::tan(camera_.FieldOfView() * 0.5f) * kBackgroundMargin;
            const float halfWidth = halfHeight * camera_.AspectRatio();

            const auto& camPos = camera_.Position();
            const auto& camTarget = cameraTarget_;
            Microsoft::Xna::Framework::Vector3 forward(
                camTarget.X - camPos.X, camTarget.Y - camPos.Y, camTarget.Z - camPos.Z);
            forward.Normalize();
            const Microsoft::Xna::Framework::Vector3 quadCenter(
                camPos.X + forward.X * kBackgroundDistance,
                camPos.Y + forward.Y * kBackgroundDistance,
                camPos.Z + forward.Z * kBackgroundDistance);

            // A distant panorama (owner 2026-11-02, OR-0053: it stood still on the screen as the
            // camera turned). The original tiles its 640 x 480 picture and scrolls it at 2/3 of
            // the world's scroll (Decor.cpp:650-700); here the picture is wrapped round the
            // horizon N times, N the whole number of screen widths in a turn (so it keeps about
            // the width it had), turning the camera scrolls it, and the camera's height moves it
            // up and down at 2/3, as the original's vertical scroll.
            constexpr float kTwoPi = 6.28318530718f;
            const float verticalFov = camera_.FieldOfView();
            const float horizontalFov = 2.0f * std::atan(std::tan(verticalFov * 0.5f) * camera_.AspectRatio());
            const float repeats = std::max(1.0f, std::round(kTwoPi / horizontalFov));
            const float imageAngleWide = kTwoPi / repeats;      // one picture's width, radians
            const float imageAngleHigh = imageAngleWide * 0.75f; // 640 x 480
            const float quadAngleWide = 2.0f * std::atan(halfWidth / kBackgroundDistance);
            const float quadAngleHigh = 2.0f * std::atan(halfHeight / kBackgroundDistance);
            const float yaw = std::atan2(forward.X, -forward.Z);
            const float pitch = std::asin(std::clamp(forward.Y, -1.0f, 1.0f));
            const float uWidth = quadAngleWide / imageAngleWide;
            const float vHeight = quadAngleHigh / imageAngleHigh;
            const float uCenter = yaw / imageAngleWide;
            constexpr float kParallax = 2.0f / 3.0f;
            const float vCenter = 0.5f - (camPos.Y * 64.0f * kParallax) / 480.0f - pitch / imageAngleHigh;
            auto mesh = MakeTexturedMeshBuilder();
            mesh.AddBillboard(quadCenter,
                              Microsoft::Xna::Framework::Vector2(halfWidth * 2.0f, halfHeight * 2.0f),
                              cameraRight, cameraUp,
                              CnaEngine::Graphics::UvRect{uCenter - uWidth * 0.5f, vCenter - vHeight * 0.5f,
                                                          uCenter + uWidth * 0.5f, vCenter + vHeight * 0.5f});

            if (!mesh.Mesh().Empty())
            {
                backgroundMeshRenderer_ = UploadTexturedMesh(device, mesh.Mesh());
                backgroundEffect_->View = camera_.View();
                backgroundEffect_->Projection = camera_.Projection();
                backgroundEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                // The panorama wraps (its UVs run past 0..1).
                device.getSamplerStatesProperty()[0] = Microsoft::Xna::Framework::Graphics::SamplerState::LinearWrap;
                // No CullNone override needed here -- fixed at the source
                // (2026-07-09, NEXT.md §8 task 0): billboards used to be
                // back-facing under the default CullCounterClockwise state;
                // easy-3d's AppendBillboardMesh winding was reversed then, and
                // cna-engine's MeshBuilder::AddBillboard took it over as is,
                // so every billboard renders under the default cull state.
                backgroundMeshRenderer_->Draw(device, *backgroundEffect_);
            }
        }

        if (terrainRenderer_ && terrainEffect_)
        {
            terrainEffect_->View = camera_.View();
            terrainEffect_->Projection = camera_.Projection();
            terrainEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            // The reliefs near the camera's target (Q-012), then the opaque terrain; its
            // semi-transparent pass (water) after the models below.
            {
                const auto reliefStart = std::chrono::steady_clock::now();
                const int shownBefore = terrainRenderer_->ReliefChunksShown();
                terrainRenderer_->UpdateRelief(device, cameraTarget_, terrainReliefRadius_, terrainReliefWarm_ ? 1 : -1);
                if (!terrainReliefWarm_ && std::getenv("GALAXY_TERRAIN_RELIEF_TIMING") != nullptr)
                {
                    const auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - reliefStart);
                    std::cout << "GalaxyEggbertCNA: relief: " << terrainRenderer_->ReliefChunksShown() - shownBefore
                              << " chunk(s), " << terrainRenderer_->ReliefPrimitiveCount() << " triangles built in "
                              << ms.count() << " ms." << std::endl;
                }
                terrainReliefWarm_ = true;
            }
            terrainRenderer_->DrawOpaque(device, *terrainEffect_);

            if (objectCubeEffect_)
            {
                // Platform-lift/crate UniformCube objects (NEXT.md §8 task 3)
                // -- opaque, drawn alongside the main terrain pass, same
                // texture sheet (terrainTexture_) as terrainEffect_ but its
                // own effect instance since World/View/Projection are set
                // independently per draw call. Rebuilt every frame
                // (2026-07-09) using each MobileObjSpec's real per-instance
                // phase, same reason and pattern as the billboard batches
                // below -- types 47/48's chenille icon formulas now actually
                // animate instead of being frozen at their first frame.
                // An uploaded GpuMesh has no in-place update API (only
                // construction from mesh data), so a fresh mesh is
                // the only way to reflect a new icon/UV per frame -- cheap
                // here since there are only a handful of cube objects.
                auto cubeMesh = MakeLitTexturedMeshBuilder();
                for (const auto& obj : worldRuntime_.GetMobileObjects())
                {
                    const auto visual = ResolveObjectVisual(
                        obj.type, static_cast<int>(obj.phase), obj.visualIcon);
                    if (!obj.active || !visual.visible ||
                        visual.renderMode != ObjectRenderMode::SolidCube)
                    {
                        continue;
                    }
                    if (ConveyorLiftModel(obj) != nullptr)
                    {
                        continue; // drawn as Decor/chenille.rds in the model pass
                    }
                    const Microsoft::Xna::Framework::Vector3 center(
                        obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ);
                    const auto tileUv = tileAtlas_.GetTileUv(visual.icon);
                    if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType1)
                    {
                        // The lift's bar and stem, not a cube of its mostly transparent tile.
                        const float du = tileUv.u1 - tileUv.u0;
                        const float dv = tileUv.v1 - tileUv.v0;
                        for (const auto& part : LiftVisualParts())
                        {
                            AddCenteredBox(cubeMesh, center + Microsoft::Xna::Framework::Vector3(0.0f, part.centerYOffset, 0.0f),
                                           Microsoft::Xna::Framework::Vector3(part.width, part.height, part.width),
                                           CnaEngine::Graphics::UvRect{tileUv.u0 + part.u0 * du, tileUv.v0 + part.v0 * dv,
                                                                       tileUv.u0 + part.u1 * du, tileUv.v0 + part.v1 * dv});
                        }
                        continue;
                    }
                    AddCenteredBox(cubeMesh, center, Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f), tileUv);
                }

                if (!cubeMesh.Mesh().Empty())
                {
                    objectCubeMeshRenderer_ = UploadLitTexturedMesh(device, cubeMesh.Mesh());
                    objectCubeEffect_->View = camera_.View();
                    objectCubeEffect_->Projection = camera_.Projection();
                    objectCubeEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                    objectCubeMeshRenderer_->Draw(device, *objectCubeEffect_);
                }
                // The crates blown away (InteractionSystem::ByeByePiece, BD-0049): each its cube,
                // spinning about the view axis where it flies.
                for (const auto& piece : interaction_.ByeByePieces())
                {
                    if (piece.helicopter || piece.fanIcon >= 0)
                    {
                        continue;
                    }
                    const auto visual = ResolveObjectVisual(piece.look.type, static_cast<int>(piece.look.phase),
                                                            piece.look.visualIcon);
                    if (!visual.visible || visual.renderMode != ObjectRenderMode::SolidCube)
                    {
                        continue;
                    }
                    auto pieceMesh = MakeLitTexturedMeshBuilder();
                    AddCenteredBox(pieceMesh, Microsoft::Xna::Framework::Vector3(0.0f, 0.0f, 0.0f),
                                   Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f), tileAtlas_.GetTileUv(visual.icon));
                    auto pieceRenderer = UploadLitTexturedMesh(device, pieceMesh.Mesh());
                    objectCubeEffect_->View = camera_.View();
                    objectCubeEffect_->Projection = camera_.Projection();
                    objectCubeEffect_->World =
                        Microsoft::Xna::Framework::Matrix::CreateRotationZ(-piece.rotationDegrees * 3.14159265f / 180.0f) *
                        Microsoft::Xna::Framework::Matrix::CreateTranslation(
                            piece.x, ResolveObjectVisualCenterY(piece.look.type, piece.y), piece.z);
                    pieceRenderer->Draw(device, *objectCubeEffect_);
                    objectCubeEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                }
            }

            if (grassEffect_)
            {
                // Icon 107's grass-top overlay (NEXT.md §8 task 3) — a
                // separate texture/effect (BasicEffect only binds one
                // texture at a time), drawn right after the main opaque
                // terrain pass so it composites correctly (both opaque, no
                // special blend/depth state needed, unlike water/icons
                // 30-31).
                grassEffect_->View = camera_.View();
                grassEffect_->Projection = camera_.Projection();
                grassEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                terrainRenderer_->DrawGrass(device, *grassEffect_);
            }

            // Third-person placeholder 3D model (2026-07-09, NEXT.md §3) --
            // only drawn in third-person mode (first-person mode has
            // nothing to show, same reasoning as the pre-2026-07-09 comment
            // this replaced). World transform bakes in a scale and a yaw
            // rotation to face Blupi's movement direction -- exact
            // forward-facing alignment for this specific placeholder asset
            // hasn't been visually verified, a real Blupi model may need a
            // different constant rotation offset here.
            //
            // kPlaceholderModelScale (2026-07-18, user-reported "~160% of
            // block height" bug): the placeholder fox mesh's own local-space
            // vertex Y span is -0.122..78.907 (measured directly from
            // avatars3d/blupi_placeholder/fox1.verts.bin, stride 52 bytes,
            // position = first 3 floats per convert_avatar.py's own
            // struct.pack("<3f3f2f4f4B", ...) layout), i.e. ~79.03 units
            // tall with its feet already at local Y~=0. Real mobile-eggbert
            // Blupi's own standing collision box (Decor::BlupiRect()'s
            // default case, Decor.cpp:2519-2520: Top=pos.Y+11,
            // Bottom=pos.Y+60-2, i.e. 47px inside a 60px DIMBLUPIY cell,
            // Def.hpp:154) brackets the user's own reported 71.875% figure
            // (exact source of that specific fraction not located as a
            // literal constant, likely the user's own sprite measurement) --
            // since 1 block = 1.0 world unit here (BlupiController::
            // GroundHeightAt() returns topmost-solid-block-Y + 1), target
            // height = 0.71875 world units, giving scale = 0.71875 / 79.029
            // =~ 0.009095.
            //
            // kPlaceholderModelYOffset (2026-07-18, user-reported "the fox
            // floats in the air" bug): BlupiController's own GetY() is
            // NOT flush with the terrain's rendered surface -- TerrainRenderer
            // renders a solid block at grid Y with Center.Y = Y directly (no
            // +0.5), so a column's topmost solid block at grid Y=g has its
            // visual top surface at world Y=g+0.5, while GroundHeightAt()
            // (by design, matches the MoveObject/BigDecor cube convention of
            // sitting a full unit above the floor block's own grid index,
            // see TerrainRenderer.cpp/CubeMesh.cpp) returns g+1 -- so
            // GetY() sits a constant 0.5 world units above the true visual
            // ground surface. This is invisible for the first-person camera
            // (kEyeHeight is just a feel-tuned offset, already screenshot-
            // verified against this same convention) and the collision-only
            // point that existed before this model, but is glaringly visible
            // now that a real body silhouette is rendered against the
            // terrain mesh -- confirmed live via a forced third-person
            // screenshot showing the fox's feet hovering above the floor
            // tiles. Deliberately NOT fixed by changing GetY()/
            // GroundHeightAt() itself (would ripple into jump/fall physics,
            // kFallDeathY, teleporter/water Y thresholds, and every already
            // screenshot-tuned camera constant that implicitly assumes this
            // exact convention) -- this is a render-only correction local to
            // this specific placeholder mesh's own translation, exactly like
            // kPlaceholderModelScale above.
            //
            // kPlaceholderModelDepthOffset (2026-07-21, live user bug report:
            // "liska... muze vejit skoro cela do zdi" -- the fox model can
            // walk almost entirely into a wall): Blupi's own collision is a
            // single POINT (not a rect), so it stops exactly flush with a
            // wall's face -- but the fox mesh has real nose-to-tail depth
            // (measured directly from fox1.verts.bin the same way as
            // kPlaceholderModelScale above: local Z span -88.095..66.625,
            // ~154.72 units, by far the largest of the 3 axes, consistent
            // with a quadruped's body-length axis -- X's 25.19 span is
            // shoulder width, Y's already-established 79.03 span is height).
            // With the model's own origin centered on Blupi's collision
            // point and no compensating offset, roughly half that real body
            // depth (~0.7 world units at kPlaceholderModelScale) visually
            // projected forward past the point where collision actually
            // stops, into any wall Blupi walked up to. Confirmed live
            // (forced third-person screenshot, Blupi placed flush against a
            // wall): baseline showed only a tiny sliver of the fox visible;
            // a local-space translation of +0.6 along the model's own
            // (pre-rotation) Z axis -- applied THROUGH the yaw rotation
            // below, not after it, so it always points opposite the model's
            // current facing regardless of which way Blupi is turned --
            // fully clears the mesh's front half of the wall while still
            // looking correctly grounded/positioned in open floor (both
            // re-verified live). Empirically tuned by eye, same category and
            // rigor as kPlaceholderModelScale/YOffset above, not a computed
            // value -- render-only, does not touch Blupi's own collision
            // point at all.
            //
            // Real `Hide` during the death-lock's own life-loss-Voyage window
            // (death-VFX follow-up, `IsDeathHidden()`'s own comment) -- first-
            // person mode already renders no Blupi model at all, so this is
            // the only render-side change needed. Likewise the real frame
            // tables' invisible frames (Clear2, Clear3's tail, Teleporting's
            // -1 entries) draw no real model.
            // The clip is chosen in Update()'s Play block, so a frame drawn
            // before the first Play tick (a mission just loaded from the
            // menu) has none yet: skip the model until it has one. Starting
            // in third person made this reachable (2026-10-25, the game
            // aborted with "Unknown animation clip").
            if (cameraMode_ == CameraMode::ThirdPersonModel && blupiModelLoaded_ && blupiEffect_ &&
                blupiModel_->Clips.count(blupiActiveClipName_) != 0 &&
                !blupi_.IsDeathHidden() && !(blupiModelIsReal_ && blupi_.IsAnimIconInvisible()) &&
                !BlupiDrawnAsDeathSprite())
            {
                constexpr float kPlaceholderModelScale = 0.009095f;
                constexpr float kCharredShade = 0.07f; // the sprites' soot black
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
                // Charred (Clear1, Clear3: CharredBlupi.hpp), shrunk toward his feet as the frame
                // crumbles and spread out as the heap of ash.
                auto charred = BlupiCharredShape();
                // Glued (BlupiGlueModel()): his model flattened and spread under the glue model, the
                // dark green puddle of the sprites (168-171: about a sixteenth of his height, half as
                // wide again).
                const bool gluePuddle = BlupiGlueModel() != nullptr;
                if (gluePuddle)
                {
                    charred = GalaxyEggbert::Game::CharredShape{true, 0.06f, 1.5f, false};
                }
                const auto world =
                    Microsoft::Xna::Framework::Matrix::CreateScale(modelScale) *
                    Microsoft::Xna::Framework::Matrix::CreateScale(charred.spread, charred.height, charred.spread) *
                    Microsoft::Xna::Framework::Matrix::CreateTranslation(hangShift, 0.0f, modelDepthOffset) *
                    Microsoft::Xna::Framework::Matrix::CreateRotationY(modelYaw) *
                    Microsoft::Xna::Framework::Matrix::CreateTranslation(
                        blupi_.GetX(), blupi_.GetY() + kPlaceholderModelYOffset - suspendDrop,
                        blupi_.GetZ());
                std::vector<Microsoft::Xna::Framework::Matrix> boneTransforms;
                blupiModel_->ComputeBoneTransformsEXT(
                    blupiActiveClipName_,
                    System::TimeSpan::FromSeconds(blupiClipTimeSeconds_),
                    /*loop=*/true,
                    boneTransforms);
                blupiEffect_->setWorldProperty(world);
                blupiEffect_->setViewProperty(camera_.View());
                blupiEffect_->setProjectionProperty(camera_.Projection());
                blupiEffect_->SetBoneTransforms(boneTransforms);
                // XNA's default culling, set explicitly so no earlier draw's state
                // applies: the real model's triangles are clockwise like XNA
                // content (tools/rd4_to_gltf/export_cna_avatar.py). Converted
                // straight from glTF they were counter-clockwise, this state
                // culled his front faces, and he drew inside out
                // (upstream_bugs.md UB-003).
                device.setRasterizerStateProperty(
                    Microsoft::Xna::Framework::Graphics::RasterizerState::CullCounterClockwise);

                // The real model's face for this frame of the active clip.
                Microsoft::Xna::Framework::Graphics::Texture2D* face = nullptr;
                if (const auto faces = blupiClipFaces_.find(blupiActiveClipName_);
                    blupiModelIsReal_ && faces != blupiClipFaces_.end() && !faces->second.empty())
                {
                    const auto frame = static_cast<std::size_t>(blupiClipTimeSeconds_ * blupiFaceRate_)
                        % faces->second.size();
                    const int index = faces->second[frame];
                    if (index >= 0 && index < static_cast<int>(blupiFaces_.size()))
                    {
                        face = &blupiFaces_[static_cast<std::size_t>(index)];
                    }
                }

                const auto drawParts = [&](const Microsoft::Xna::Framework::Graphics::SkinnedModelEXT& model,
                                           Microsoft::Xna::Framework::Graphics::Texture2D* faceTexture)
                {
                    for (const auto& part : model.Parts)
                    {
                        blupiEffect_->setTextureProperty(faceTexture != nullptr && part.Name == blupiFacePart_
                                                             ? faceTexture
                                                             : part.Texture);
                        blupiEffect_->Apply();
                        device.SetVertexBuffer(part.Part->getVertexBufferProperty());
                        device.SetIndexBuffer(part.Part->getIndexBufferProperty());
                        device.DrawIndexedPrimitives(
                            Microsoft::Xna::Framework::Graphics::PrimitiveType::TriangleList,
                            part.Part->getVertexOffsetProperty(),
                            0,
                            part.Part->getNumVerticesProperty(),
                            part.Part->getStartIndexProperty(),
                            part.Part->getPrimitiveCountProperty());
                    }
                };
                // The drink's Hide (Decor.cpp:828-838, OR-0016): his own clips, drawn at 30 %
                // opacity, flashing to full every other pair of ticks once the gauge is down to 25.
                // Not depth-written, so what lies behind him still shows through.
                float blupiAlpha = 1.0f;
                if (blupi_.GetSecretPower() == GalaxyEggbert::Def::SecretPower::Hide)
                {
                    const bool flash = blupi_.GetSecretPowerLevel() <= 25 && worldRuntime_.GetAnimPhase() % 4 >= 2;
                    blupiAlpha = flash ? 1.0f : 0.3f;
                }
                if (blupiAlpha < 1.0f)
                {
                    device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
                    device.setDepthStencilStateProperty(Microsoft::Xna::Framework::Graphics::DepthStencilState::DepthRead);
                }
                blupiEffect_->setAlphaProperty(blupiAlpha);
                if (charred.charred)
                {
                    // Burnt black: soot, no sheen; glued, the puddle's dark green.
                    blupiEffect_->setDiffuseColorProperty(
                        gluePuddle ? Microsoft::Xna::Framework::Vector3(0.20f, 0.32f, 0.03f)
                                   : Microsoft::Xna::Framework::Vector3(kCharredShade, kCharredShade, kCharredShade));
                    blupiEffect_->setSpecularColorProperty(Microsoft::Xna::Framework::Vector3::Zero);
                }
                drawParts(*blupiModel_, face);
                if (charred.charred)
                {
                    blupiEffect_->setDiffuseColorProperty(Microsoft::Xna::Framework::Vector3::One);
                    blupiEffect_->setSpecularColorProperty(Microsoft::Xna::Framework::Vector3::One);
                }

                // The vehicle he rides in this clip (helicopter, jeep, tank,
                // overcraft, skateboard): its own model, built from the same
                // Ray Dream frames in the same space (clips.txt "vehicle"
                // lines), so it takes his world transform and clip time.
                for (const auto& [vehicleName, vehicle] : blupiVehicles_)
                {
                    if (vehicle->Clips.find(blupiActiveClipName_) == vehicle->Clips.end())
                    {
                        continue;
                    }
                    vehicle->ComputeBoneTransformsEXT(
                        blupiActiveClipName_,
                        System::TimeSpan::FromSeconds(blupiClipTimeSeconds_),
                        /*loop=*/true,
                        boneTransforms);
                    blupiEffect_->SetBoneTransforms(boneTransforms);
                    drawParts(*vehicle, nullptr);
                }
                blupiEffect_->setAlphaProperty(1.0f);
                if (blupiAlpha < 1.0f)
                {
                    device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
                    device.setDepthStencilStateProperty(Microsoft::Xna::Framework::Graphics::DepthStencilState::Default);
                }

                // Hide's trail (BD-0056): a copy of his pose every 10 px he moves while hidden (the
                // original's Manhattan distance, over X, Y and Z here), each fading from 30 % to
                // nothing over its 20 ticks (TickHideGhosts ages them).
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
                    hideGhosts_.push_back({world, blupiActiveClipName_, blupiClipTimeSeconds_, face, 0.0f});
                    hideGhostX_ = blupi_.GetX();
                    hideGhostY_ = blupi_.GetY();
                    hideGhostZ_ = blupi_.GetZ();
                }
                if (!hideGhosts_.empty())
                {
                    device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
                    device.setDepthStencilStateProperty(Microsoft::Xna::Framework::Graphics::DepthStencilState::DepthRead);
                    for (const auto& ghost : hideGhosts_)
                    {
                        if (blupiModel_->Clips.count(ghost.clip) == 0)
                        {
                            continue;
                        }
                        blupiModel_->ComputeBoneTransformsEXT(ghost.clip, System::TimeSpan::FromSeconds(ghost.clipSeconds),
                                                              /*loop=*/true, boneTransforms);
                        blupiEffect_->setWorldProperty(ghost.world);
                        blupiEffect_->SetBoneTransforms(boneTransforms);
                        blupiEffect_->setAlphaProperty(std::max(0.0f, (20.0f - ghost.ageTicks) * 0.3f / 20.0f));
                        drawParts(*blupiModel_, ghost.face);
                    }
                    blupiEffect_->setAlphaProperty(1.0f);
                    device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
                    device.setDepthStencilStateProperty(Microsoft::Xna::Framework::Graphics::DepthStencilState::Default);
                }
            }

            // Parked vehicles: a helicopter, jeep, skateboard, tank or
            // overcraft pickup is drawn as its real vehicle model in the
            // first frame of its standing clip instead of its element.png
            // billboard (skipped there, ParkedVehicleModel), in both camera
            // modes. It stands on the floor of its cell, facing +X, the way
            // the sprites face right.
            if (blupiEffect_ && blupiModelIsReal_ && !blupiVehicles_.empty())
            {
                constexpr float kHalfPi = 1.57079632679f;
                std::vector<Microsoft::Xna::Framework::Matrix> boneTransforms;
                blupiEffect_->setViewProperty(camera_.View());
                blupiEffect_->setProjectionProperty(camera_.Projection());
                device.setRasterizerStateProperty(
                    Microsoft::Xna::Framework::Graphics::RasterizerState::CullCounterClockwise);
                for (const auto& obj : worldRuntime_.GetMobileObjects())
                {
                    const auto parked = ParkedVehicleModel(obj.type);
                    if (parked.model == nullptr || !obj.active ||
                        !ResolveObjectVisual(obj.type, static_cast<int>(obj.phase), obj.visualIcon).visible)
                    {
                        continue;
                    }
                    parked.model->ComputeBoneTransformsEXT(
                        parked.clip, System::TimeSpan::FromSeconds(0.0), /*loop=*/false, boneTransforms);
                    blupiEffect_->setWorldProperty(
                        Microsoft::Xna::Framework::Matrix::CreateScale(kRealModelScale) *
                        Microsoft::Xna::Framework::Matrix::CreateRotationY(kHalfPi) *
                        Microsoft::Xna::Framework::Matrix::CreateTranslation(
                            obj.currentX, obj.currentY - 0.5f, obj.currentZ));
                    blupiEffect_->SetBoneTransforms(boneTransforms);
                    for (const auto& part : parked.model->Parts)
                    {
                        blupiEffect_->setTextureProperty(part.Texture);
                        blupiEffect_->Apply();
                        device.SetVertexBuffer(part.Part->getVertexBufferProperty());
                        device.SetIndexBuffer(part.Part->getIndexBufferProperty());
                        device.DrawIndexedPrimitives(
                            Microsoft::Xna::Framework::Graphics::PrimitiveType::TriangleList,
                            part.Part->getVertexOffsetProperty(),
                            0,
                            part.Part->getNumVerticesProperty(),
                            part.Part->getStartIndexProperty(),
                            part.Part->getPrimitiveCountProperty());
                    }
                }
                // The lost helicopter flying away (ByeByeHelico, BD-0049): its parked model, turned
                // about its middle as the original's picture spins.
                const auto helicopter = ParkedVehicleModel(GalaxyEggbert::Def::ObjectType::ObjectType13);
                for (const auto& piece : interaction_.ByeByePieces())
                {
                    if (!piece.helicopter || helicopter.model == nullptr)
                    {
                        continue;
                    }
                    helicopter.model->ComputeBoneTransformsEXT(
                        helicopter.clip, System::TimeSpan::FromSeconds(0.0), /*loop=*/false, boneTransforms);
                    blupiEffect_->setWorldProperty(
                        Microsoft::Xna::Framework::Matrix::CreateScale(kRealModelScale) *
                        Microsoft::Xna::Framework::Matrix::CreateRotationY(kHalfPi) *
                        Microsoft::Xna::Framework::Matrix::CreateTranslation(0.0f, -0.5f, 0.0f) *
                        Microsoft::Xna::Framework::Matrix::CreateRotationZ(-piece.rotationDegrees * 3.14159265f / 180.0f) *
                        Microsoft::Xna::Framework::Matrix::CreateTranslation(piece.x, piece.y, piece.z));
                    blupiEffect_->SetBoneTransforms(boneTransforms);
                    for (const auto& part : helicopter.model->Parts)
                    {
                        blupiEffect_->setTextureProperty(part.Texture);
                        blupiEffect_->Apply();
                        device.SetVertexBuffer(part.Part->getVertexBufferProperty());
                        device.SetIndexBuffer(part.Part->getIndexBufferProperty());
                        device.DrawIndexedPrimitives(
                            Microsoft::Xna::Framework::Graphics::PrimitiveType::TriangleList,
                            part.Part->getVertexOffsetProperty(),
                            0,
                            part.Part->getNumVerticesProperty(),
                            part.Part->getStartIndexProperty(),
                            part.Part->getPrimitiveCountProperty());
                    }
                }
            }

            // Objects with a converted 3D model (objectModelTable_, plan.md
            // Phase 7B): drawn where their billboard would be (skipped in the
            // billboard passes), posed at their icon's frame, turned by its
            // yaw and flipped as the icon is (left-right or upside down,
            // about the sprite's centre line: kFlipShift). A model is in
            // blocks with its sprite's view toward +Z, as the parked vehicles
            // are. BigDecor cells with a model stand on their cell's floor at
            // the original's size: an explo.png icon is 64, 128 or 144 game
            // pixels (icons.txt), drawn 1:1 in the original (Pixmap.cpp),
            // where the billboard is 1 block.
            if (blupiEffect_ && !objectModels_.empty())
            {
                constexpr float kDegrees = 3.14159265358979f / 180.0f;
                std::vector<Microsoft::Xna::Framework::Matrix> boneTransforms;
                blupiEffect_->setViewProperty(camera_.View());
                blupiEffect_->setProjectionProperty(camera_.Projection());
                // The signs drawn this frame whose painted face is a decal (drawn after the models).
                auto& decalCells = signDecalCells_;
                decalCells.clear();
                electrodeCells_.clear();
                const auto drawModel = [&](const ObjectModelPose& pose, float x, float y, float z, float turnRadians = 0.0f,
                                           float rollRadians = 0.0f, float depthScale = 1.0f,
                                           bool boothFrontOnly = false, float topScale = 1.0f)
                {
                    auto& model = *objectModels_[static_cast<std::size_t>(pose.model)];
                    model.ComputeBoneTransformsEXT(
                        "Frames", System::TimeSpan::FromSeconds(pose.seconds), /*loop=*/false, boneTransforms);
                    // Mirroring turns the triangles' winding around (a negative depth scale too).
                    device.setRasterizerStateProperty(
                        (pose.mirror != 0) != (depthScale < 0.0f)
                            ? Microsoft::Xna::Framework::Graphics::RasterizerState::CullClockwise
                            : Microsoft::Xna::Framework::Graphics::RasterizerState::CullCounterClockwise);
                    blupiEffect_->setWorldProperty(
                        Microsoft::Xna::Framework::Matrix::CreateRotationY(pose.yawDegrees * kDegrees) *
                        Microsoft::Xna::Framework::Matrix::CreateScale(
                            pose.mirror == 1 ? -1.0f : 1.0f, pose.mirror == 2 ? -1.0f : 1.0f, 1.0f) *
                        Microsoft::Xna::Framework::Matrix::CreateTranslation(
                            (pose.mirror == 1 ? -kFlipShift : 0.0f) + static_cast<float>(pose.offsetX) / 64.0f,
                            (pose.mirror == 2 ? kFlipShift : 0.0f) - static_cast<float>(pose.offsetY) / 64.0f,
                            0.0f) *
                        Microsoft::Xna::Framework::Matrix::CreateScale(1.0f, 1.0f, depthScale) *
                        // Scaled about the cell's top (the drip's smallest drops, DripShrink).
                        Microsoft::Xna::Framework::Matrix::CreateTranslation(0.0f, -0.5f, 0.0f) *
                        Microsoft::Xna::Framework::Matrix::CreateScale(topScale) *
                        Microsoft::Xna::Framework::Matrix::CreateTranslation(0.0f, 0.5f, 0.0f) *
                        Microsoft::Xna::Framework::Matrix::CreateRotationZ(rollRadians) *
                        Microsoft::Xna::Framework::Matrix::CreateRotationY(turnRadians) *
                        Microsoft::Xna::Framework::Matrix::CreateTranslation(x, y, z));
                    blupiEffect_->SetBoneTransforms(boneTransforms);
                    for (const auto& part : model.Parts)
                    {
                        if (boothFrontOnly && GalaxyEggbert::Game::IsBoothBodyPart(part.Name))
                        {
                            continue; // a teleporter's other sides: its plate and lights only
                        }
                        blupiEffect_->setTextureProperty(part.Texture);
                        blupiEffect_->Apply();
                        device.SetVertexBuffer(part.Part->getVertexBufferProperty());
                        device.SetIndexBuffer(part.Part->getIndexBufferProperty());
                        device.DrawIndexedPrimitives(
                            Microsoft::Xna::Framework::Graphics::PrimitiveType::TriangleList,
                            part.Part->getVertexOffsetProperty(),
                            0,
                            part.Part->getNumVerticesProperty(),
                            part.Part->getStartIndexProperty(),
                            part.Part->getPrimitiveCountProperty());
                    }
                };
                // The clay bridge: its row of lumps across the whole cell (BlockModels.hpp).
                const auto drawBridgeRows = [&](const ObjectModelPose& pose, float x, float y, float z)
                {
                    for (int row = 0; row < GalaxyEggbert::Game::kBridgeRows; ++row)
                    {
                        const auto placement = GalaxyEggbert::Game::BridgeRowPlacement(row);
                        drawModel(pose, x, y, z + placement.dz, placement.yaw);
                    }
                };
                for (const auto& obj : worldRuntime_.GetMobileObjects())
                {
                    ObjectTextureSource sheet{};
                    int icon = 0;
                    const ObjectModelPose* pose =
                        ObjectSprite(obj, sheet, icon) ? objectModelTable_.Find(sheet, icon) : nullptr;
                    if (pose != nullptr && sheet == ObjectTextureSource::ObjectM &&
                        GalaxyEggbert::Game::IsClayBridgeIcon(icon))
                    {
                        drawBridgeRows(*pose, obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY),
                                       obj.currentZ);
                    }
                    else if (const ObjectModelPose* conveyor = ConveyorLiftModel(obj))
                    {
                        // The conveyor lift (47/48): its tread on the frame table_chenille(i) shows,
                        // stretched across the cell as the terrain conveyor (ModelDepthScale); the
                        // plain lift (1) its round platform on a funnel and stem, as modelled.
                        const bool plainLift = obj.type == GalaxyEggbert::Def::ObjectType::ObjectType1;
                        const float depth = plainLift ? 1.0f : GalaxyEggbert::Game::ModelDepthScale(311);
                        drawModel(*conveyor, obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY),
                                  obj.currentZ, 0.0f, 0.0f, depth);
                        if (!plainLift)
                        {
                            // The belt on both sides (BlockModels.hpp ModelMirroredInDepth).
                            drawModel(*conveyor, obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY),
                                      obj.currentZ, 0.0f, 0.0f, -depth);
                        }
                    }
                    else if (pose != nullptr)
                    {
                        // A sprite only faces left or right, along X, by its travel along X. One
                        // patrolling mainly along Z is turned from that facing to its real one.
                        const float dx = obj.posEndX - obj.posStartX;
                        const float dz = obj.posEndZ - obj.posStartZ;
                        float turn = 0.0f;
                        if (std::fabs(dz) > std::fabs(dx))
                        {
                            const float spriteFacing = obj.posStartX > obj.posEndX ? 3.14159265f : 0.0f;
                            turn = std::atan2(-dz, dx) - spriteFacing;
                        }
                        drawModel(*pose, obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ,
                                  turn);
                    }
                }
                // The pieces flying away with a model (BD-0049): enemies, the invert potion, a fan,
                // spinning about the view axis as the original's picture does.
                for (const auto& piece : interaction_.ByeByePieces())
                {
                    const float spin = -piece.rotationDegrees * kDegrees;
                    if (piece.helicopter)
                    {
                        continue; // the parked vehicles' pass
                    }
                    if (piece.fanIcon >= 0)
                    {
                        if (const ObjectModelPose* fan =
                                objectModelTable_.Find(ObjectTextureSource::ObjectM, GalaxyEggbert::Game::kFanModelIcon))
                        {
                            drawModel(*fan, piece.x, piece.y, piece.z, 0.0f,
                                      GalaxyEggbert::Game::FanModelRoll(static_cast<std::uint16_t>(piece.fanIcon)) + spin);
                        }
                        continue;
                    }
                    ObjectTextureSource sheet{};
                    int icon = 0;
                    if (ObjectSprite(piece.look, sheet, icon))
                    {
                        if (const ObjectModelPose* pose = objectModelTable_.Find(sheet, icon))
                        {
                            drawModel(*pose, piece.x, ResolveObjectVisualCenterY(piece.look.type, piece.y), piece.z,
                                      0.0f, spin);
                        }
                    }
                }
                // The Clear2 death's soul (OR-0047): element 230-241 as Blupi/blupi-ange.rd4, the
                // turn its icons show, rising 300 px from where he died (Decor::BlupiDead,
                // VoyageDraw), half transparent as the sheet's sprite (alpha 127).
                if (interaction_.VoyageActive() && interaction_.VoyageIsSoul())
                {
                    if (const ObjectModelPose* pose =
                            objectModelTable_.Find(ObjectTextureSource::Element, interaction_.VoyageIconId()))
                    {
                        constexpr float kSoulRise = 300.0f / 64.0f;
                        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
                        device.setDepthStencilStateProperty(
                            Microsoft::Xna::Framework::Graphics::DepthStencilState::DepthRead);
                        blupiEffect_->setAlphaProperty(0.5f);
                        drawModel(*pose, interaction_.VoyageWorldAnchorX(),
                                  interaction_.VoyageWorldAnchorY() + interaction_.VoyageProgress() * kSoulRise,
                                  interaction_.VoyageWorldAnchorZ());
                        blupiEffect_->setAlphaProperty(1.0f);
                        device.setDepthStencilStateProperty(
                            Microsoft::Xna::Framework::Graphics::DepthStencilState::Default);
                        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
                        soulDrawnIn3D = true;
                    }
                }
                // The glue death (element 168-171; Q-006): Blupi/glu.rd4's struggling pose, its yellow
                // made the sprites' green at conversion (texrecolour), turned the way he faces, its
                // icon's centre 30 px over his feet; his own model lies flattened under it as the
                // puddle (BlupiGlueModel()).
                if (const ObjectModelPose* glued = BlupiGlueModel())
                {
                    constexpr float kPi = 3.14159265358979f;
                    drawModel(*glued, blupi_.GetX(), blupi_.GetY() - 0.5f + 30.0f / 64.0f, blupi_.GetZ(),
                              kPi - blupi_.GetYaw());
                    glueStrandsPending_ = true;
                }
                // The fan cells (VIS-0127): the original fan model turned to blow the cell's way, its
                // rotor on the tile animation's frame.
                if (terrainRenderer_)
                {
                    for (const auto& cell : terrainRenderer_->ModelCells())
                    {
                        const bool overFloor =
                            std::find(GalaxyEggbert::Game::kModelOverFloorBases.begin(),
                                      GalaxyEggbert::Game::kModelOverFloorBases.end(),
                                      cell.base) != GalaxyEggbert::Game::kModelOverFloorBases.end();
                        if (overFloor)
                        {
                            // The saw standing on its floor cell (the cell listed is the one above),
                            // its blade along the corridor (openSide carries the placement's turn).
                            const int icon =
                                GalaxyEggbert::ResolveBlockAnimationIconAt(
                                    cell.base, worldRuntime_.GetAnimPhase(), static_cast<int>(std::lround(cell.x)),
                                    static_cast<int>(std::lround(cell.y)), static_cast<int>(std::lround(cell.z)));
                            if (icon >= 0)
                            {
                                if (const ObjectModelPose* pose = objectModelTable_.Find(
                                        ObjectTextureSource::ObjectM, static_cast<std::uint16_t>(icon)))
                                {
                                    drawModel(*pose, cell.x, cell.y, cell.z,
                                              GalaxyEggbert::Game::FrontModelYaw(cell.openSide));
                                }
                            }
                            continue;
                        }
                        if (std::find(GalaxyEggbert::Game::kAnimatedModelBases.begin(),
                                      GalaxyEggbert::Game::kAnimatedModelBases.end(),
                                      cell.base) != GalaxyEggbert::Game::kAnimatedModelBases.end())
                        {
                            // An animated tile as its scene's model, on the tile's frame (gone: none).
                            const int icon =
                                GalaxyEggbert::ResolveBlockAnimationIconAt(
                                    cell.base, worldRuntime_.GetAnimPhase(), static_cast<int>(std::lround(cell.x)),
                                    static_cast<int>(std::lround(cell.y)), static_cast<int>(std::lround(cell.z)));
                            if (icon >= 0)
                            {
                                const auto shrink = GalaxyEggbert::Game::DripShrink(icon);
                                if (const ObjectModelPose* pose = objectModelTable_.Find(
                                        ObjectTextureSource::ObjectM, shrink.modelIcon))
                                {
                                    drawModel(*pose, cell.x, cell.y, cell.z, 0.0f, 0.0f, 1.0f, false, shrink.scale);
                                }
                            }
                            continue;
                        }
                        if (std::find(GalaxyEggbert::Game::kFanModelBases.begin(), GalaxyEggbert::Game::kFanModelBases.end(),
                                      cell.base) == GalaxyEggbert::Game::kFanModelBases.end())
                        {
                            // A still icon drawn as its model (a pillar), or a lattice of its frame.
                            // The spring bounces through its frames (BlockModels.hpp SpringFrameIcon).
                            const std::uint16_t shown =
                                cell.base == GalaxyEggbert::BlockTypes::Spring
                                    ? GalaxyEggbert::Game::SpringFrameIcon(
                                          worldRuntime_.GetAnimPhase(),
                                          static_cast<int>(std::lround(cell.x)) + WorldRuntime::kWorldCenterX,
                                          static_cast<int>(std::lround(cell.z)) + WorldRuntime::kWorldCenterZ)
                                    : cell.base;
                            if (const ObjectModelPose* still = objectModelTable_.Find(ObjectTextureSource::ObjectM, shown))
                            {
                                if (std::find(GalaxyEggbert::Game::kLatticeModelIcons.begin(),
                                              GalaxyEggbert::Game::kLatticeModelIcons.end(),
                                              cell.base) != GalaxyEggbert::Game::kLatticeModelIcons.end())
                                {
                                    for (const auto& side : GalaxyEggbert::Game::kLatticeSides)
                                    {
                                        if ((cell.sameSides & side.bit) == 0)
                                        {
                                            drawModel(*still, cell.x + side.dx, cell.y, cell.z + side.dz, side.yaw);
                                        }
                                    }
                                }
                                else if (std::find(GalaxyEggbert::Game::kRowModelIcons.begin(),
                                                   GalaxyEggbert::Game::kRowModelIcons.end(),
                                                   cell.base) != GalaxyEggbert::Game::kRowModelIcons.end())
                                {
                                    // A bar is two pipes along its row, one under each of Blupi's hands
                                    // (BarRows.hpp); the model runs along X, turned where the row runs
                                    // along Z.
                                    const bool alongZ = GalaxyEggbert::Game::ClassifyBarRow(
                                                            (cell.sameSides & 3) != 0, (cell.sameSides & 12) != 0) ==
                                                        GalaxyEggbert::Game::BarRow::AlongZ;
                                    for (const float side : {-1.0f, 1.0f})
                                    {
                                        const float offset = side * GalaxyEggbert::Game::kBarPipeOffset;
                                        drawModel(*still, cell.x + (alongZ ? offset : 0.0f), cell.y,
                                                  cell.z + (alongZ ? 0.0f : offset), alongZ ? 1.57079632f : 0.0f);
                                    }
                                }
                                else if (std::find(GalaxyEggbert::Game::kFrontModelIcons.begin(),
                                                   GalaxyEggbert::Game::kFrontModelIcons.end(),
                                                   cell.base) != GalaxyEggbert::Game::kFrontModelIcons.end() ||
                                         GalaxyEggbert::Game::IsSignModelIcon(cell.base))
                                {
                                    drawModel(*still, cell.x, cell.y, cell.z,
                                              GalaxyEggbert::Game::FrontModelYaw(cell.openSide));
                                    if (GalaxyEggbert::Game::IsTeleporterIcon(cell.base))
                                    {
                                        // The letter plate and lights on its other three sides too
                                        // (BlockModels.hpp IsTeleporterIcon, OR-0056).
                                        for (int quarter = 1; quarter < 4; ++quarter)
                                        {
                                            const float yaw = GalaxyEggbert::Game::FrontModelYaw(cell.openSide) +
                                                              1.57079632f * static_cast<float>(quarter);
                                            const float out = GalaxyEggbert::Game::kBoothSideOffsets[
                                                static_cast<std::size_t>(quarter)];
                                            drawModel(*still, cell.x + out * std::sin(yaw), cell.y,
                                                      cell.z + out * std::cos(yaw), yaw, 0.0f, 1.0f,
                                                      /*boothFrontOnly=*/true);
                                        }
                                    }
                                    if (signDecals_.count(cell.base) != 0)
                                    {
                                        decalCells.push_back({cell.base, cell.x, cell.y, cell.z,
                                                              GalaxyEggbert::Game::FrontModelYaw(cell.openSide)});
                                    }
                                }
                                else if (GalaxyEggbert::Game::IsClayBridgeIcon(cell.base))
                                {
                                    drawBridgeRows(*still, cell.x, cell.y, cell.z);
                                }
                                else
                                {
                                    if (GalaxyEggbert::Game::IsElectrodeIcon(cell.base) &&
                                        electrodeSparks_.count(cell.base) != 0)
                                    {
                                        electrodeCells_.push_back({cell.base, cell.x, cell.y, cell.z, 0.0f});
                                    }
                                    drawModel(*still, cell.x, cell.y, cell.z, 0.0f, 0.0f,
                                              GalaxyEggbert::Game::ModelDepthScale(cell.base));
                                    if (GalaxyEggbert::Game::ModelMirroredInDepth(cell.base))
                                    {
                                        drawModel(*still, cell.x, cell.y, cell.z, 0.0f, 0.0f,
                                                  -GalaxyEggbert::Game::ModelDepthScale(cell.base));
                                    }
                                }
                            }
                            continue;
                        }
                        const int frame = GalaxyEggbert::ResolveBlockAnimationIcon(cell.base, worldRuntime_.GetAnimPhase()) -
                                          static_cast<int>(cell.base);
                        const ObjectModelPose* pose = objectModelTable_.Find(
                            ObjectTextureSource::ObjectM, GalaxyEggbert::Game::kFanModelIcon + std::max(0, frame));
                        if (pose != nullptr)
                        {
                            // A turned side fan blows along Z (M3D-0005): -X turned to -Z is -90 degrees.
                            drawModel(*pose, cell.x, cell.y, cell.z, cell.openSide == 0 ? -1.57079632f : 0.0f,
                                      GalaxyEggbert::Game::FanModelRoll(cell.base));
                        }
                    }
                }
                for (const auto& cell : bigDecorCells_)
                {
                    const ObjectModelPose* pose = objectModelTable_.Find(ObjectTextureSource::Explo, cell.icon);
                    if (pose != nullptr)
                    {
                        const float halfBlocks = static_cast<float>(pose->iconPixels > 0 ? pose->iconPixels : 128) / 128.0f;
                        drawModel(*pose, cell.worldX, cell.worldY - 0.5f + halfBlocks, cell.worldZ);
                    }
                }
                device.setRasterizerStateProperty(
                    Microsoft::Xna::Framework::Graphics::RasterizerState::CullCounterClockwise);
            }

            // The signs' painted faces (MR-0006): each decal on the sign's front and, mirrored so it
            // reads the right way round, on its back, a hair off the faces.
            if (!signDecals_.empty() && terrainRenderer_)
            {
                if (!decalEffect_)
                {
                    decalEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
                    decalEffect_->VertexColorEnabled = false;
                    decalEffect_->setTextureEnabledProperty(true);
                }
                decalEffect_->View = camera_.View();
                decalEffect_->Projection = camera_.Projection();
                decalEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
                // Each face only from its own side (AddBillboard winds a quad clockwise as seen from
                // the side its right x up points to): drawn from both, the plate on the far side of a
                // thin numbered post showed beside it mirrored, a 9 read as a 6 (play pass 2026-11-03).
                device.setRasterizerStateProperty(
                    Microsoft::Xna::Framework::Graphics::RasterizerState::CullCounterClockwise);
                for (const auto& cell : signDecalCells_)
                {
                    const auto found = signDecals_.find(cell.icon);
                    if (found == signDecals_.end())
                    {
                        continue;
                    }
                    const SignDecal& decal = found->second;
                    const float c = std::cos(cell.yaw), s = std::sin(cell.yaw);
                    // Model (x, y, z) to the world: CreateRotationY(yaw), then the cell.
                    const auto toWorld = [&](float x, float y, float z) {
                        return Microsoft::Xna::Framework::Vector3(cell.x + x * c + z * s, cell.y + y,
                                                                  cell.z - x * s + z * c);
                    };
                    const float midX = (decal.left + decal.right) * 0.5f;
                    const float midY = (decal.top + decal.bottom) * 0.5f;
                    const Microsoft::Xna::Framework::Vector2 size(decal.right - decal.left, decal.top - decal.bottom);
                    constexpr float kLift = 0.004f;
                    const Microsoft::Xna::Framework::Vector3 right(c, 0.0f, -s);
                    const Microsoft::Xna::Framework::Vector3 up(0.0f, 1.0f, 0.0f);
                    auto mesh = MakeTexturedMeshBuilder();
                    mesh.AddBillboard(toWorld(midX, midY, decal.depth + kLift), size, right, up,
                                      CnaEngine::Graphics::UvRect{0.0f, 0.0f, 1.0f, 1.0f});
                    mesh.AddBillboard(toWorld(-midX, midY, -decal.depth - kLift), size,
                                      Microsoft::Xna::Framework::Vector3(-c, 0.0f, s), up,
                                      CnaEngine::Graphics::UvRect{0.0f, 0.0f, 1.0f, 1.0f});
                    if (auto renderer = UploadTexturedMesh(device, mesh.Mesh()))
                    {
                        decalEffect_->setTextureProperty(decal.texture.get());
                        renderer->Draw(device, *decalEffect_);
                    }
                }
                device.setRasterizerStateProperty(
                    Microsoft::Xna::Framework::Graphics::RasterizerState::CullCounterClockwise);
                device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
            }

            // The glue death's strands (owner 2026-11-04, Q-006): the painted strands from his hands to the
            // puddle and its peaks, as thin tubes in the glue model's plane (GlueStrands.hpp), in their
            // green from element.png, turned and placed as the glue model is.
            if (glueStrandsPending_ && objectEffect_)
            {
                glueStrandsPending_ = false;
                const int frame = std::clamp(blupi_.GetRawAnimIcon() - 168, 0, 3);
                const auto iconUv = GetElementIconUv(168 + frame);
                const auto& texel = GalaxyEggbert::Game::kGlueTexel[static_cast<std::size_t>(frame)];
                const Microsoft::Xna::Framework::Vector2 uv(iconUv.U0 + texel[0] * (iconUv.U1 - iconUv.U0),
                                                            iconUv.V0 + texel[1] * (iconUv.V1 - iconUv.V0));
                constexpr float kPi = 3.14159265358979f;
                const float turn = kPi - blupi_.GetYaw();
                const float c = std::cos(turn), s = std::sin(turn);
                const Microsoft::Xna::Framework::Vector3 origin(blupi_.GetX(), blupi_.GetY() - 0.5f + 30.0f / 64.0f,
                                                                blupi_.GetZ());
                auto mesh = MakeTexturedMeshBuilder();
                GalaxyEggbert::Game::AppendPolylineTubes(
                    mesh, GalaxyEggbert::Game::kGluePoints.data(), GalaxyEggbert::Game::kGluePolylines.data(),
                    GalaxyEggbert::Game::kGlueFrames[static_cast<std::size_t>(frame)], uv,
                    Microsoft::Xna::Framework::Vector3(0.0f, 1.0f, 0.0f),
                    [&](const Microsoft::Xna::Framework::Vector3& p)
                    {
                        return Microsoft::Xna::Framework::Vector3(origin.X + p.X * c + p.Z * s, origin.Y + p.Y,
                                                                  origin.Z - p.X * s + p.Z * c);
                    });
                if (auto renderer = UploadTexturedMesh(device, mesh.Mesh()))
                {
                    objectEffect_->View = camera_.View();
                    objectEffect_->Projection = camera_.Projection();
                    objectEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                    renderer->Draw(device, *objectEffect_);
                }
            }

            // The electrode's sparks facing the camera over its model (Q-005), as its sprite shows them.
            if (!electrodeCells_.empty())
            {
                if (!decalEffect_)
                {
                    decalEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
                    decalEffect_->VertexColorEnabled = false;
                    decalEffect_->setTextureEnabledProperty(true);
                }
                const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.View());
                const auto right = invView.getRightProperty();
                const auto up = invView.getUpProperty();
                decalEffect_->View = camera_.View();
                decalEffect_->Projection = camera_.Projection();
                decalEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
                device.setRasterizerStateProperty(Microsoft::Xna::Framework::Graphics::RasterizerState::CullNone);
                for (const auto& cell : electrodeCells_)
                {
                    auto mesh = MakeTexturedMeshBuilder();
                    mesh.AddBillboard(Microsoft::Xna::Framework::Vector3(cell.x, cell.y, cell.z),
                                      Microsoft::Xna::Framework::Vector2(1.0f, 1.0f), right, up,
                                      CnaEngine::Graphics::UvRect{0.0f, 0.0f, 1.0f, 1.0f});
                    if (auto renderer = UploadTexturedMesh(device, mesh.Mesh()))
                    {
                        decalEffect_->setTextureProperty(electrodeSparks_.at(cell.icon).get());
                        renderer->Draw(device, *decalEffect_);
                    }
                }
                device.setRasterizerStateProperty(
                    Microsoft::Xna::Framework::Graphics::RasterizerState::CullCounterClockwise);
                device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
            }

            // The terrain's semi-transparent pass after Blupi and every model, so water covers
            // what lies in it (OR-0052: a chest under water looked dry from above).
            device.setRasterizerStateProperty(
                Microsoft::Xna::Framework::Graphics::RasterizerState::CullCounterClockwise);
            terrainRenderer_->DrawTransparent(device, *terrainEffect_);

            static bool terrainPixelPrinted = false;
            if (!terrainPixelPrinted)
            {
                terrainPixelPrinted = true;
                terrainPixelPrintedFrame_ = drawFrameIndex_;
                // Backbuffer, not Viewport -- see GetBackBufferSize()'s comment. This site
                // needs it for BOTH readbacks below: the whole-backbuffer capture, and the
                // 5x5 single-pixel grid, whose sample rectangles CNA validates against the
                // backbuffer bounds (a Viewport wider than the backbuffer made those throw
                // std::out_of_range instead).
                int w = 0, h = 0;
                GetBackBufferSize(device, w, h);

                // Sample a 5x5 grid over the central 60% of the screen —
                // more robust than one center pixel, which can miss terrain
                // if the exact screen center happens to fall between blocks.
                int nonBackgroundCount = 0;
                std::set<std::uint32_t> distinctTerrainColors;
                constexpr int kGridSize = 5;
                for (int gy = 0; gy < kGridSize; ++gy)
                {
                    for (int gx = 0; gx < kGridSize; ++gx)
                    {
                        const int px = w / 2 + (gx - kGridSize / 2) * (w / 10);
                        const int py = h / 2 + (gy - kGridSize / 2) * (h / 10);
                        const Microsoft::Xna::Framework::Rectangle sample(px, py, 1, 1);
                        Microsoft::Xna::Framework::Color pixel(0, 0, 0, 0);
                        device.GetBackBufferData(&sample, &pixel, 0, 1);
                        // Sky-blue clear color is (100, 149, 237); anything
                        // clearly different is terrain. Note (2026-07-09):
                        // when a real background image is loaded
                        // (backgroundLoaded_), sampled points can also
                        // legitimately differ from this flat reference by
                        // showing real sky/background art instead of solid
                        // blue -- this diagnostic's "non-background" count
                        // is a looser signal in that case (it can no longer
                        // assume everything non-blue is terrain), but is
                        // still useful together with distinctTerrainColors:
                        // real terrain plus a real photographic-ish
                        // background both produce many distinct colors,
                        // while a genuinely broken texture sample (a flat
                        // fallback tint) would not.
                        if (std::abs(static_cast<int>(pixel.getRProperty()) - 100) > 10 ||
                            std::abs(static_cast<int>(pixel.getGProperty()) - 149) > 10 ||
                            std::abs(static_cast<int>(pixel.getBProperty()) - 237) > 10)
                        {
                            ++nonBackgroundCount;
                            const std::uint32_t packed =
                                (static_cast<std::uint32_t>(pixel.getRProperty()) << 16) |
                                (static_cast<std::uint32_t>(pixel.getGProperty()) << 8) |
                                static_cast<std::uint32_t>(pixel.getBProperty());
                            distinctTerrainColors.insert(packed);
                        }
                    }
                }
                std::cout << "GalaxyEggbertCNA: terrain visibility check — "
                          << nonBackgroundCount << "/" << (kGridSize * kGridSize)
                          << " sampled screen points show non-background (terrain) color, "
                          << distinctTerrainColors.size() << " distinct color(s) among them"
                          << " (>1 means the texture is actually being sampled, not a flat fallback)."
                          << std::endl;

                // One-shot full-frame screenshot for manual/visual
                // verification (2026-07-08, DirectionalCube render mode,
                // NEXT.md §8 task 1) -- the pixel-sample check above proves
                // terrain is textured, but not that a specific tile's
                // geometry (e.g. an open top/bottom face) looks right; a
                // real image is the only way to confirm that by eye.
                std::vector<Microsoft::Xna::Framework::Color> backBuffer(
                    static_cast<std::size_t>(w) * static_cast<std::size_t>(h),
                    Microsoft::Xna::Framework::Color(0, 0, 0, 0));
                device.GetBackBufferData(backBuffer.data(), 0, static_cast<int>(backBuffer.size()));
                Microsoft::Xna::Framework::Graphics::Texture2D screenshot(device, w, h);
                screenshot.SetData(backBuffer.data(), static_cast<int>(backBuffer.size()));
                screenshot.SaveAsPng("screenshot.png");
                std::cout << "GalaxyEggbertCNA: wrote screenshot.png (" << w << "x" << h << ")."
                          << std::endl;
            }
        }

        // Real alpha transparency for every billboard drawn below (2026-07-09,
        // found live: element.png/object-m.png/explo.png/blupi.png icon
        // sheets all have a real, correct RGBA alpha channel -- confirmed by
        // direct pixel inspection, element.png's background pixels are
        // (0,0,0,0) -- but nothing before this enabled blending, so the GPU
        // ignored alpha and drew each billboard's transparent background as
        // solid opaque black. AlphaBlend restored to Opaque right after the
        // last billboard batch below (BigDecor) so it doesn't affect the
        // unrelated 2D SpriteBatch HUD indicator or any future opaque draw
        // call after this point.
        // NonPremultiplied, not AlphaBlend: Texture2D(path) loads PNGs with
        // straight alpha, while AlphaBlend expects the premultiplied colours
        // CNA's (and XNA's) content pipeline produces, which Mobile Eggbert
        // draws. With AlphaBlend every translucent pixel came out too bright:
        // the menu's dark blue button panel showed whitish (user report,
        // 2026-10-25).
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);

        // Billboard rendering for worldRuntime_'s parsed MoveObjects
        // (15-3d-render-mapping-design.md §5/§7, first pass 2026-07-06) —
        // now animated via each MobileObjSpec's real per-instance phase
        // (2026-07-09, WorldRuntime::Update()), with this pass selected by
        // ObjectDefinitionRegistry's Element source. Rebuilt
        // every frame since billboard vertex positions depend on the camera
        // (MeshBuilder::AddBillboard faces the camera's right/up) -- convenient,
        // since it also means the icon lookup naturally re-runs every frame
        // with the latest phase.
        if (objectEffect_ &&
            (!worldRuntime_.GetMobileObjects().empty() || BlupiDrawnAsDeathSprite() || BlupiCharredShape().smokes ||
             BlupiAuraShown() != GalaxyEggbert::Def::SecretPower::None))
        {
            // Camera-facing basis: the inverse view matrix's Right/Up rows
            // give the camera's world-space right/up vectors (standard
            // spherical-billboard technique).
            const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.View());
            const auto cameraRight = invView.getRightProperty();
            const auto cameraUp = invView.getUpProperty();

            auto mesh = MakeTexturedMeshBuilder();
            constexpr float kObjectSize = 1.0f;
            for (const auto& obj : worldRuntime_.GetMobileObjects())
            {
                // Platform lifts/crates render as solid cubes (see
                // objectCubeMeshRenderer_/NEXT.md §8 task 3), not billboards
                // -- skip here so they aren't drawn twice. object-m.png-,
                // explo.png-, and blupi.png/blupi1.png-sourced types (see
                // objectMPngMeshRenderer_/exploMeshRenderer_/
                // blupiObjectMeshRenderer_/blupi1ObjectMeshRenderer_ below)
                // are also skipped here -- GetElementIconUv() would compute
                // the wrong UV rect for them (element.png icon-index domain,
                // not theirs). ResolveObjectVisual() is phase-aware because
                // ObjectType38 spends part of its cycle on element.png --
                // this loop must pick it up during that window.
                const int objPhase = static_cast<int>(obj.phase);
                const auto visual =
                    ResolveObjectVisual(obj.type, objPhase, obj.visualIcon);
                if (!obj.active || !visual.visible ||
                    visual.renderMode != ObjectRenderMode::Billboard ||
                    visual.textureSource != ObjectTextureSource::Element ||
                    ParkedVehicleModel(obj.type).model != nullptr)
                {
                    continue;
                }
                // 7 patrol enemies with a real per-direction/turn-transition
                // icon table (ENEMY-013/016/018/024/026/041/042, bulldozer/
                // fish/bird/wasp/creature/blupih/blupit) get it here instead
                // of GetObjIcon()'s generic phase-indexed fallback -- this
                // is the one call site with real patrol state (posStart/
                // posEnd/patrolStep/patrolTime) available to feed it.
                const bool patrolGoesLeftFromStart = obj.posStartX > obj.posEndX;
                const int patrolTimeTicks = static_cast<int>(obj.patrolTime);
                int icon;
                if (!TryGetPatrolIcon(obj.type, patrolGoesLeftFromStart, obj.patrolStep, patrolTimeTicks, icon))
                {
                    icon = visual.icon;
                }
                if (objectModelTable_.Find(ObjectTextureSource::Element, icon) != nullptr)
                {
                    continue;  // drawn as its 3D model
                }
                const auto uv = GetElementIconUv(icon);
                mesh.AddBillboard(
                    Microsoft::Xna::Framework::Vector3(
                        obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ),
                    Microsoft::Xna::Framework::Vector2(kObjectSize, kObjectSize), cameraRight, cameraUp,
                    CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
            }
            // A piece flying away without a model (BD-0049): its sprite, turned as the original's.
            for (const auto& piece : interaction_.ByeByePieces())
            {
                ObjectTextureSource sheet{};
                int icon = 0;
                if (piece.helicopter || piece.fanIcon >= 0 || !ObjectSprite(piece.look, sheet, icon) ||
                    sheet != ObjectTextureSource::Element || objectModelTable_.Find(sheet, icon) != nullptr)
                {
                    continue;
                }
                const float angle = piece.rotationDegrees * 3.14159265f / 180.0f;
                const auto right = cameraRight * std::cos(angle) - cameraUp * std::sin(angle);
                const auto up = cameraRight * std::sin(angle) + cameraUp * std::cos(angle);
                const auto uv = GetElementIconUv(icon);
                mesh.AddBillboard(
                    Microsoft::Xna::Framework::Vector3(
                        piece.x, ResolveObjectVisualCenterY(piece.look.type, piece.y), piece.z),
                    Microsoft::Xna::Framework::Vector2(kObjectSize, kObjectSize), right, up,
                    CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
            }
            // His own death sprite (BlupiDrawnAsDeathSprite()): the 60 px icon, its centre 30 px
            // over his feet, as the original draws it. His feet are half a cell under GetY(), where
            // the model's are (kPlaceholderModelYOffset); centred 30 px over GetY() it hung half a
            // cell over the floor.
            if (BlupiDrawnAsDeathSprite())
            {
                const auto uv = GetElementIconUv(blupi_.GetDisplayAnimIcon());
                mesh.AddBillboard(
                    Microsoft::Xna::Framework::Vector3(blupi_.GetX(), blupi_.GetY() - 0.5f + 30.0f / 64.0f,
                                                       blupi_.GetZ()),
                    Microsoft::Xna::Framework::Vector2(60.0f / 64.0f, 60.0f / 64.0f), cameraRight, cameraUp,
                    CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
            }
            // The shield's orb (table_shield_blupi, 2 px higher) and sparkles (table_shieldloop), the
            // power's stars (table_magicloop), a frame every 2 ticks, on his sprite's place (its
            // centre 30 px over his feet), a little behind him from the camera: the original draws
            // them before him (Decor.cpp:782-806).
            if (const auto aura = BlupiAuraShown();
                aura == GalaxyEggbert::Def::SecretPower::Shield || aura == GalaxyEggbert::Def::SecretPower::Power)
            {
                static constexpr int kShieldBlupi[16] = {144, 145, 146, 147, 148, 149, 150, 151,
                                                         266, 267, 268, 269, 270, 271, 272, 273};
                static constexpr int kShieldLoop[5] = {274, 275, 276, 277, 278};
                static constexpr int kMagicLoop[5] = {152, 153, 154, 155, 156};
                const int frame = worldRuntime_.GetAnimPhase() / 2;
                const auto behind = invView.getForwardProperty() * 0.35f;
                const Microsoft::Xna::Framework::Vector3 centre(blupi_.GetX(), blupi_.GetY() - 0.5f + 30.0f / 64.0f,
                                                                blupi_.GetZ());
                const auto add = [&](int icon, float raise)
                {
                    const auto uv = GetElementIconUv(icon);
                    mesh.AddBillboard(centre + behind + Microsoft::Xna::Framework::Vector3(0.0f, raise, 0.0f),
                                      Microsoft::Xna::Framework::Vector2(60.0f / 64.0f, 60.0f / 64.0f), cameraRight,
                                      cameraUp, CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
                };
                if (aura == GalaxyEggbert::Def::SecretPower::Shield)
                {
                    add(kShieldBlupi[frame % 16], 2.0f / 64.0f);
                    add(kShieldLoop[frame % 5], 0.0f);
                }
                else
                {
                    add(kMagicLoop[frame % 5], 0.0f);
                }
            }
            // The smoke over his heap of ash (46/47, CharredBlupi.hpp): the icon's rows above the
            // heap, its bottom where the sprite's would be over the model's feet (GetY() - 0.5,
            // as the model's kPlaceholderModelYOffset).
            if (const auto charred = BlupiCharredShape(); charred.smokes)
            {
                const auto uv = GetElementIconUv(blupi_.GetRawAnimIcon());
                constexpr float kRows = static_cast<float>(GalaxyEggbert::Game::kCharredSmokeRows);
                const float feetY = blupi_.GetY() - 0.5f;
                const float bottom = feetY + GalaxyEggbert::Game::kCharredSmokeBottomPixels / 64.0f;
                mesh.AddBillboard(
                    Microsoft::Xna::Framework::Vector3(blupi_.GetX(), bottom + kRows / 128.0f, blupi_.GetZ()),
                    Microsoft::Xna::Framework::Vector2(60.0f / 64.0f, kRows / 64.0f), cameraRight, cameraUp,
                    CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V0 + (uv.V1 - uv.V0) * kRows / 60.0f});
            }

            if (!mesh.Mesh().Empty())
            {
                objectMeshRenderer_ = UploadTexturedMesh(device, mesh.Mesh());
                objectEffect_->View = camera_.View();
                objectEffect_->Projection = camera_.Projection();
                objectEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                objectMeshRenderer_->Draw(device, *objectEffect_);
            }

            // The drink's bubbles (Decor.cpp:882-895): over each bottle three bubbles (element
            // 274 + n % 5) rise 3 px a tick for 50 ticks, a third apart (table_drinkoffset 0, 7, 22),
            // fading from whole to nothing. The fade in five steps: one mesh and alpha each.
            {
                static constexpr int kDrinkOffsets[3] = {0, 7, 22};
                constexpr int kFadeSteps = 5;
                std::vector<CnaEngine::Graphics::MeshBuilder> fades;
                for (int k = 0; k < kFadeSteps; ++k)
                {
                    fades.push_back(MakeTexturedMeshBuilder());
                }
                bool any = false;
                for (const auto& obj : worldRuntime_.GetMobileObjects())
                {
                    if (!obj.active || obj.type != GalaxyEggbert::Def::ObjectType::ObjectType30)
                    {
                        continue;
                    }
                    const float centreY = ResolveObjectVisualCenterY(obj.type, obj.currentY);
                    for (const int offset : kDrinkOffsets)
                    {
                        const int n = (worldRuntime_.GetAnimPhase() + offset) % 50;
                        const int step = std::min(kFadeSteps - 1, n * kFadeSteps / 50);
                        const auto uv = GetElementIconUv(274 + n % 5);
                        fades[static_cast<std::size_t>(step)].AddBillboard(
                            Microsoft::Xna::Framework::Vector3(obj.currentX, centreY + static_cast<float>(n) * 3.0f / 64.0f,
                                                               obj.currentZ) +
                                cameraRight * (2.0f / 64.0f),
                            Microsoft::Xna::Framework::Vector2(60.0f / 64.0f, 60.0f / 64.0f), cameraRight, cameraUp,
                            CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
                        any = true;
                    }
                }
                if (any)
                {
                    objectEffect_->View = camera_.View();
                    objectEffect_->Projection = camera_.Projection();
                    objectEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                    for (int k = 0; k < kFadeSteps; ++k)
                    {
                        if (fades[static_cast<std::size_t>(k)].Mesh().Empty())
                        {
                            continue;
                        }
                        // (50 - n) / 50 at the step's middle.
                        objectEffect_->setAlphaProperty(1.0f - (static_cast<float>(k) + 0.5f) / static_cast<float>(kFadeSteps));
                        drinkBubbleRenderers_[static_cast<std::size_t>(k)] =
                            UploadTexturedMesh(device, fades[static_cast<std::size_t>(k)].Mesh());
                        drinkBubbleRenderers_[static_cast<std::size_t>(k)]->Draw(device, *objectEffect_);
                    }
                    objectEffect_->setAlphaProperty(1.0f);
                }
            }
        }

        // Billboard rendering for object-m.png-sourced MoveObjects
        // (ObjectDefinitionRegistry, NEXT.md §3, 2026-07-09) --
        // same camera-facing billboard technique as the element.png batch
        // above, but reuses terrainTexture_ (object-m.png) via
        // objectMPngEffect_, and looks up UVs through tileAtlas_ instead of
        // GetElementIconUv() (the same icon-index domain as terrain).
        if (objectMPngEffect_ && !worldRuntime_.GetMobileObjects().empty())
        {
            const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.View());
            const auto cameraRight = invView.getRightProperty();
            const auto cameraUp = invView.getUpProperty();

            auto mesh = MakeTexturedMeshBuilder();
            constexpr float kObjectSize = 1.0f;
            for (const auto& obj : worldRuntime_.GetMobileObjects())
            {
                const auto visual = ResolveObjectVisual(
                    obj.type, static_cast<int>(obj.phase), obj.visualIcon);
                if (!obj.active || !visual.visible ||
                    visual.renderMode != ObjectRenderMode::Billboard ||
                    visual.textureSource != ObjectTextureSource::ObjectM)
                {
                    continue;
                }
                if (objectModelTable_.Find(ObjectTextureSource::ObjectM, visual.icon) != nullptr)
                {
                    continue;  // drawn as its 3D model
                }
                const auto uv = tileAtlas_.GetTileUv(visual.icon);
                mesh.AddBillboard(
                    Microsoft::Xna::Framework::Vector3(
                        obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ),
                    Microsoft::Xna::Framework::Vector2(kObjectSize, kObjectSize), cameraRight, cameraUp,
                    uv);
            }

            if (!mesh.Mesh().Empty())
            {
                objectMPngMeshRenderer_ = UploadTexturedMesh(device, mesh.Mesh());
                objectMPngEffect_->View = camera_.View();
                objectMPngEffect_->Projection = camera_.Projection();
                objectMPngEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                objectMPngMeshRenderer_->Draw(device, *objectMPngEffect_);
            }
        }

        // Billboard rendering for explo.png-sourced MoveObjects
        // (ObjectDefinitionRegistry, NEXT.md §3, 2026-07-09) --
        // same camera-facing billboard technique as the batches above, but
        // exploTexture_ (a genuinely new texture) via exploEffect_, and
        // GetExploIconUv() instead of GetElementIconUv()/tileAtlas_.
        if (exploEffect_ &&
            (!worldRuntime_.GetMobileObjects().empty() || BlupiAuraShown() == GalaxyEggbert::Def::SecretPower::Cloud))
        {
            const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.View());
            const auto cameraRight = invView.getRightProperty();
            const auto cameraUp = invView.getUpProperty();

            auto mesh = MakeTexturedMeshBuilder();
            constexpr float kObjectSize = 1.0f;
            // The Cloud's three sparks round him (explo.png 48 + (t + k) % 6, 128 px centred on his
            // sprite: pos - 34, Decor.cpp:807-818), a little behind him as the original draws them first.
            if (BlupiAuraShown() == GalaxyEggbert::Def::SecretPower::Cloud)
            {
                const auto behind = invView.getForwardProperty() * 0.35f;
                const Microsoft::Xna::Framework::Vector3 centre(blupi_.GetX(), blupi_.GetY() - 0.5f + 30.0f / 64.0f,
                                                                blupi_.GetZ());
                for (int k = 0; k < 3; ++k)
                {
                    const auto uv = GetExploIconUv(48 + (worldRuntime_.GetAnimPhase() + k) % 6);
                    mesh.AddBillboard(centre + behind, Microsoft::Xna::Framework::Vector2(128.0f / 64.0f, 128.0f / 64.0f),
                                      cameraRight, cameraUp, CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
                }
            }
            for (const auto& obj : worldRuntime_.GetMobileObjects())
            {
                const auto visual = ResolveObjectVisual(
                    obj.type, static_cast<int>(obj.phase), obj.visualIcon);
                if (!obj.active || visual.renderMode != ObjectRenderMode::Billboard ||
                    visual.textureSource != ObjectTextureSource::Explo)
                {
                    continue;
                }
                // Real -1 "invisible frame" sentinel (plan.md VISUAL-009,
                // table_sploutch2/3's own leading delay ticks, fixed
                // 2026-07-14) -- skip drawing this object entirely this
                // tick, matching the real source's own blank-frame behavior.
                if (!visual.visible)
                {
                    continue;
                }
                if (objectModelTable_.Find(ObjectTextureSource::Explo, visual.icon) != nullptr)
                {
                    continue;  // drawn as its 3D model
                }
                const auto uv = GetExploIconUv(visual.icon);
                mesh.AddBillboard(
                    Microsoft::Xna::Framework::Vector3(
                        obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ),
                    Microsoft::Xna::Framework::Vector2(kObjectSize, kObjectSize), cameraRight, cameraUp,
                    CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
            }

            if (!mesh.Mesh().Empty())
            {
                exploMeshRenderer_ = UploadTexturedMesh(device, mesh.Mesh());
                exploEffect_->View = camera_.View();
                exploEffect_->Projection = camera_.Projection();
                exploEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                exploMeshRenderer_->Draw(device, *exploEffect_);
            }
        }

        // Billboard rendering for Blupi-skin MoveObjects
        // (ObjectDefinitionRegistry, NEXT.md §3, 2026-07-09) --
        // ObjectType200 via blupiObjectEffect_ (blupi.png), ObjectType201/
        // 202/203 via blupi1ObjectEffect_ (blupi1.png) -- two separate
        // batches since
        // BasicEffect only binds one texture at a time.
        if (blupiObjectEffect_ && blupi1ObjectEffect_ && !worldRuntime_.GetMobileObjects().empty())
        {
            const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.View());
            const auto cameraRight = invView.getRightProperty();
            const auto cameraUp = invView.getUpProperty();

            auto blupiMesh = MakeTexturedMeshBuilder();
            auto blupi1Mesh = MakeTexturedMeshBuilder();
            constexpr float kObjectSize = 1.0f;
            for (const auto& obj : worldRuntime_.GetMobileObjects())
            {
                const int objPhase = static_cast<int>(obj.phase);
                const auto visual =
                    ResolveObjectVisual(obj.type, objPhase, obj.visualIcon);
                if (!obj.active || !visual.visible ||
                    visual.renderMode != ObjectRenderMode::Billboard ||
                    (visual.textureSource != ObjectTextureSource::Blupi &&
                     visual.textureSource != ObjectTextureSource::Blupi1))
                {
                    continue;
                }
                if (objectModelTable_.Find(visual.textureSource, visual.icon) != nullptr)
                {
                    continue;  // drawn as its 3D model
                }
                const auto uv = GetBlupiIconUv(visual.icon);
                auto& mesh = visual.textureSource == ObjectTextureSource::Blupi1
                    ? blupi1Mesh : blupiMesh;
                mesh.AddBillboard(
                    Microsoft::Xna::Framework::Vector3(
                        obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ),
                    Microsoft::Xna::Framework::Vector2(kObjectSize, kObjectSize), cameraRight, cameraUp,
                    CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
            }

            if (!blupiMesh.Mesh().Empty())
            {
                blupiObjectMeshRenderer_ = UploadTexturedMesh(device, blupiMesh.Mesh());
                blupiObjectEffect_->View = camera_.View();
                blupiObjectEffect_->Projection = camera_.Projection();
                blupiObjectEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                blupiObjectMeshRenderer_->Draw(device, *blupiObjectEffect_);
            }

            if (!blupi1Mesh.Mesh().Empty())
            {
                blupi1ObjectMeshRenderer_ = UploadTexturedMesh(device, blupi1Mesh.Mesh());
                blupi1ObjectEffect_->View = camera_.View();
                blupi1ObjectEffect_->Projection = camera_.Projection();
                blupi1ObjectEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                blupi1ObjectMeshRenderer_->Draw(device, *blupi1ObjectEffect_);
            }
        }

        // Billboard rendering for worldRuntime_'s parsed BigDecor: cells.
        // The source renderer uses CHEXPLO for this layer, so the same
        // explo.png atlas/UV convention as explosion-channel MoveObjects is
        // intentional. Cells may come from either a mobile BigDecor: grid
        // or sparse 3D `.vwr` BigDecorRecord metadata.
        if (bigDecorEffect_ && !bigDecorCells_.empty())
        {
            const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.View());
            const auto cameraRight = invView.getRightProperty();
            const auto cameraUp = invView.getUpProperty();

            auto mesh = MakeTexturedMeshBuilder();
            auto marineMesh = MakeTexturedMeshBuilder();
            constexpr float kBigDecorSize = 1.0f;
            for (const auto& cell : bigDecorCells_)
            {
                if (cell.icon == GalaxyEggbert::BlockTypes::Marine)
                {
                    // BigDecor icon 203 is not an explo.png icon: the original
                    // draws the object channel's marine animation there
                    // (Decor.cpp: `num2 = table_marine[m_time / ScaleDiv(3) % 11];
                    // channel = Object`), the Marine block's animation.
                    float u = 0.0f, v = 0.0f, us = 0.0f, vs = 0.0f;
                    GalaxyEggbert::BlockTypes::tileUV(
                        GalaxyEggbert::ResolveBlockAnimationIcon(GalaxyEggbert::BlockTypes::Marine,
                                                                 worldRuntime_.GetAnimPhase()),
                        u, v, us, vs);
                    marineMesh.AddBillboard(
                        Microsoft::Xna::Framework::Vector3(cell.worldX, cell.worldY, cell.worldZ),
                        Microsoft::Xna::Framework::Vector2(kBigDecorSize, kBigDecorSize), cameraRight, cameraUp,
                        CnaEngine::Graphics::UvRect{u, v, u + us, v + vs});
                    continue;
                }
                if (objectModelTable_.Find(ObjectTextureSource::Explo, cell.icon) != nullptr)
                {
                    continue;  // drawn as its 3D model
                }
                const auto iconUv = GetBigDecorIconUv(static_cast<int>(cell.icon));
                mesh.AddBillboard(
                    Microsoft::Xna::Framework::Vector3(
                        cell.worldX, cell.worldY, cell.worldZ),
                    Microsoft::Xna::Framework::Vector2(kBigDecorSize, kBigDecorSize), cameraRight, cameraUp,
                    CnaEngine::Graphics::UvRect{
                        iconUv.U0, iconUv.V0, iconUv.U1, iconUv.V1});
            }

            if (!mesh.Mesh().Empty())
            {
                bigDecorMeshRenderer_ = UploadTexturedMesh(device, mesh.Mesh());
                bigDecorEffect_->View = camera_.View();
                bigDecorEffect_->Projection = camera_.Projection();
                bigDecorEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                bigDecorMeshRenderer_->Draw(device, *bigDecorEffect_);
            }
            if (objectMPngEffect_ && !marineMesh.Mesh().Empty())
            {
                bigDecorMarineMeshRenderer_ = UploadTexturedMesh(device, marineMesh.Mesh());
                objectMPngEffect_->View = camera_.View();
                objectMPngEffect_->Projection = camera_.Projection();
                objectMPngEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                bigDecorMarineMeshRenderer_->Draw(device, *objectMPngEffect_);
            }
        }
        } // phase_ != Wait && phase_ != Init (3D world render guard above)

        // Restore opaque state after the AlphaBlend block above (billboards
        // only) -- Hud manages its own blend state, but this keeps device
        // state predictable for anything drawn after this point.
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);

        // HUD quads must not be depth-tested against the 3D scene (Draw()
        // re-enables depth at the top of every frame regardless).
        device.SetDepthTestEnabled(false);

        // Real mobile-eggbert bottom HUD + interim animation-state
        // indicator (2026-07-10, see Hud.hpp for the full layout AND for
        // why this is real 3D quads instead of SpriteBatch -- CNA's Vulkan
        // backend records every SpriteBatch batch before every 3D draw, so
        // a sprite HUD is always painted over by the scene; that was the
        // real cause of the twice-reported "icon visible for a second,
        // then gone" bug, which the earlier depth-test fix addressed only
        // for EasyGL). Drawn LAST so it wins submission order on both
        // backends.
        {
            const auto& viewport = device.getViewportProperty();
            // Pause/Win/Lost (2026-07-13, plan.md MENU-028..039/046..057):
            // the real HUD is skipped entirely, replaced by
            // inputPad_.DrawPause()/DrawWinLost()'s real background/
            // character/buttons below -- fully hiding the normal HUD
            // outside Play via a dedicated draw call instead of a generic
            // text message.
            const bool phaseHasRealScreen = phase_ == GalaxyEggbert::Def::GamePhase::Pause ||
                                             phase_ == GalaxyEggbert::Def::GamePhase::Win ||
                                             phase_ == GalaxyEggbert::Def::GamePhase::Lost ||
                                             phase_ == GalaxyEggbert::Def::GamePhase::PlaySetup ||
                                             phase_ == GalaxyEggbert::Def::GamePhase::MainSetup ||
                                             phase_ == GalaxyEggbert::Def::GamePhase::Resume ||
                                             phase_ == GalaxyEggbert::Def::GamePhase::Wait ||
                                             phase_ == GalaxyEggbert::Def::GamePhase::Init ||
                                             phase_ == GalaxyEggbert::Def::GamePhase::Editor;
            if (!phaseHasRealScreen)
            {
                // Training-hint lookup (plan.md HUD-024): real grid position
                // (not render-centered, WorldRuntime::kWorldCenterX/Z offset
                // reversed, matching every other grid<->render conversion this
                // session).
                const int hintGridX = static_cast<int>(std::lround(blupi_.GetX())) + WorldRuntime::kWorldCenterX;
                const int hintGridZ = static_cast<int>(std::lround(blupi_.GetZ())) + WorldRuntime::kWorldCenterZ;
                // A 3D world that places its own hint zones (HintZoneRecord)
                // uses them; otherwise the original tables' 2D rects apply.
                const int hintGridY = static_cast<int>(std::floor(blupi_.GetY()));
                const auto& hintZones = worldRuntime_.GetHintZones();
                // The accelerometer's texts with it on (Decor.cpp:1287-1291).
                const bool hintAccel = saveData_.GetAccelActive();
                const std::string_view trainingHint = hintZones.empty()
                    ? FindTrainingHint(worldRuntime_.GetMissionNumber(), hintGridX, hintGridZ,
                                       interaction_.TreasuresCollected(), blupi_.IsInVehicle(),
                                       interaction_.DynamiteCount() > 0, hintAccel)
                    : FindTrainingHintInZones(worldRuntime_.GetMissionNumber(), hintZones, hintGridX, hintGridY,
                                              hintGridZ, interaction_.TreasuresCollected(), blupi_.IsInVehicle(),
                                              interaction_.DynamiteCount() > 0, hintAccel);
                // Real treasure-counter hub gate (SCORE-017, Decor.cpp:1236)
                // -- same "mission != 1 && mission % 10 != 0" expression
                // already used for the Pause-menu showRestart gate above.
                const int hudMission = worldRuntime_.GetMissionNumber();
                const bool showTreasureCounter = hudMission != 1 && hudMission % 10 != 0;
                hud_.Draw(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                          interaction_.Lives(),
                          interaction_.Key1Count() > 0, interaction_.Key2Count() > 0,
                          interaction_.Key3Count() > 0,
                          interaction_.TreasuresCollected(), interaction_.TotalTreasures(), showTreasureCounter,
                          interaction_.BulletCount(), interaction_.DynamiteCount(), interaction_.PersoCount(),
                          blupi_.IsNage(), blupi_.GetWaterGaugeLevel(),
                          // The yellow gauge shows the shared count for every timed state: a power,
                          // Invert, the balloon, the squash (Decor.cpp:5574, 5852, 6023, 6046:
                          // m_jauges[1].SetHide(false) as each starts).
                          blupi_.GetSecretPower() != GalaxyEggbert::Def::SecretPower::None || blupi_.IsInverted() ||
                              blupi_.IsBallooned() || blupi_.IsEcrased(),
                          blupi_.GetTimeShield(),
                          trainingHint,
                          PhaseOverlayMessage(),
                          cameraMode_ == CameraMode::FirstPerson ? blupi_.GetDisplayAnimIcon() : -1,
                          blupi_.AnimIconUsesElementSheet(),
                          // Not while the lava death's ascent holds (VoyageDraw draws icon 40 only
                          // once it moves, Decor.cpp:10328-10331; OR-0015): Blupi's own charred
                          // sprite is still on the spot then.
                          interaction_.VoyageActive() && interaction_.VoyageIconVisible() && !soulDrawnIn3D,
                          interaction_.VoyageIconId(),
                          interaction_.VoyageIsButtonChannel(), interaction_.VoyageIsBlupiChannel(),
                          interaction_.VoyageDrawX(), interaction_.VoyageDrawY(),
                          gameSpeed_);
            }

            // On-screen touch controls (2026-07-13, plan.md
            // MENU-021..027/028..039/046..057, see InputPad.hpp): the
            // real Pause screen (background/character/5 real buttons)
            // while paused, the real Win/Lost screen (background/
            // character animation/Return button) while won/lost, or the
            // real D-pad/Jump/Action/Pause overlay on top of the live 3D
            // scene + HUD while playing.
            if (phase_ == GalaxyEggbert::Def::GamePhase::Pause)
            {
                const int mission = worldRuntime_.GetMissionNumber();
                const bool showBack = mission != 1;
                const bool showRestart = mission != 1 && mission % 10 != 0;
                inputPad_.DrawPause(
                    device, viewport.getWidthProperty(), viewport.getHeightProperty(), showBack, showRestart,
                    phaseTimeSeconds_, fadeOutPhase_);
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::Win || phase_ == GalaxyEggbert::Def::GamePhase::Lost)
            {
                inputPad_.DrawWinLost(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                     phase_ == GalaxyEggbert::Def::GamePhase::Win, phaseTimeSeconds_);
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::PlaySetup ||
                     phase_ == GalaxyEggbert::Def::GamePhase::MainSetup)
            {
                inputPad_.DrawSetup(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                    sound_.IsEnabled(), saveData_.GetJumpRight(), saveData_.GetAutoZoom(),
                                    saveData_.GetAccelActive(), phase_ == GalaxyEggbert::Def::GamePhase::MainSetup,
                                    saveData_.GetSelectedGamer(), saveData_.GetEffectsVolume(),
                                    saveData_.GetMusicVolume(), saveData_.GetAccelSensitivity(), phaseTimeSeconds_,
                                    fadeOutPhase_);
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::Resume)
            {
                inputPad_.DrawResume(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                     phaseTimeSeconds_, fadeOutPhase_);
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::Wait)
            {
                if (titleSceneActive_)
                {
                    inputPad_.DrawGalaxyLoading(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                                phaseTimeSeconds_);
                }
                else
                {
                    inputPad_.DrawWait(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                       phaseTimeSeconds_);
                }
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::Init)
            {
                std::array<InputPad::InitGamerInfo, 3> gamers;
                for (int g = 0; g < 3; ++g)
                {
                    gamers[static_cast<std::size_t>(g)] = InputPad::InitGamerInfo{
                        saveData_.GetLivesForGamer(g), saveData_.GetMainGatesForGamer(g),
                        saveData_.GetSecondaryGatesForGamer(g)};
                }
                inputPad_.DrawInit(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                   phaseTimeSeconds_, saveData_.GetSelectedGamer(), gamers, fadeOutPhase_,
                                   /*overTitleScene=*/titleSceneActive_);
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::Play)
            {
                inputPad_.DrawPlay(device, viewport.getWidthProperty(), viewport.getHeightProperty());
                inputPad_.DrawActiveCheats(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                           activeCheats_);
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::Editor)
            {
                if (worldEditor_.IsBrowsing())
                {
                    worldEditor_.DrawBrowsing(device, viewport.getWidthProperty(), viewport.getHeightProperty());
                }
                else
                {
                    worldEditor_.Draw(device, camera_,
                                      viewport.getWidthProperty(), viewport.getHeightProperty());
                }
            }

            // Hidden cheat menu overlay (2026-07-13, plan.md
            // CHEAT-001..009) -- real source renders it as a transparent
            // overlay layered on top of whatever's already on screen, no
            // background swap and no phase change, so this draws LAST,
            // on top of the just-drawn Play controls above (only ever
            // shown during Play, see Update()'s own gating).
            if (cheatMenuShown_ && phase_ == GalaxyEggbert::Def::GamePhase::Play)
            {
                inputPad_.DrawCheatMenu(device, viewport.getWidthProperty(), viewport.getHeightProperty());
            }
        }

        // One-shot full-frame screenshot, taken here (2026-07-11) rather
        // than reusing the terrain-only screenshot.png above (captured
        // right after the opaque terrain pass, before billboards/HUD) --
        // that one is a deliberately earlier terrain-geometry diagnostic,
        // not meant to also cover the 2D HUD added above. This one runs
        // after every draw call in the frame, including the new HUD, so it
        // can be inspected for visual HUD verification the same way
        // screenshot.png already is for terrain.
        //
        // Gated on drawFrameIndex_ > terrainPixelPrintedFrame_ (NEXT.md §5,
        // 2026-07-13): the terrain diagnostic's own mid-frame
        // GetBackBufferData call corrupts the rest of THAT frame's
        // rendering on Vulkan (see Draw()'s own opening comment) -- waiting
        // for a later frame guarantees this screenshot captures a frame
        // that never had a mid-draw readback.
        static bool hudScreenshotWritten = false;
        if (!hudScreenshotWritten && drawFrameIndex_ > terrainPixelPrintedFrame_)
        {
            hudScreenshotWritten = true;
            int w = 0, h = 0;
            GetBackBufferSize(device, w, h);
            std::vector<Microsoft::Xna::Framework::Color> backBuffer(
                static_cast<std::size_t>(w) * static_cast<std::size_t>(h),
                Microsoft::Xna::Framework::Color(0, 0, 0, 0));
            device.GetBackBufferData(backBuffer.data(), 0, static_cast<int>(backBuffer.size()));
            Microsoft::Xna::Framework::Graphics::Texture2D hudScreenshot(device, w, h);
            hudScreenshot.SetData(backBuffer.data(), static_cast<int>(backBuffer.size()));
            hudScreenshot.SaveAsPng("screenshot_hud.png");
            std::cout << "GalaxyEggbertCNA: wrote screenshot_hud.png (" << w << "x" << h << ")."
                      << std::endl;
        }

        // AUTOPLAY_SHOTS (EnableAutoplay()): this frame, HUD and all, as the player would see it.
        if (autoplayShotPending_ && drawFrameIndex_ > terrainPixelPrintedFrame_)
        {
            autoplayShotPending_ = false;
            int w = 0, h = 0;
            GetBackBufferSize(device, w, h);
            std::vector<Microsoft::Xna::Framework::Color> backBuffer(
                static_cast<std::size_t>(w) * static_cast<std::size_t>(h),
                Microsoft::Xna::Framework::Color(0, 0, 0, 0));
            device.GetBackBufferData(backBuffer.data(), 0, static_cast<int>(backBuffer.size()));
            Microsoft::Xna::Framework::Graphics::Texture2D shot(device, w, h);
            shot.SetData(backBuffer.data(), static_cast<int>(backBuffer.size()));
            char filename[64];
            std::snprintf(filename, sizeof(filename), "autoplay_shots/%07d.png", autoplayTick_);
            shot.SaveAsPng(filename);
        }

        // INFRA-001 (plan.md §7) -- see EnableGoldenCaptureMode()'s own
        // comment. Fixed tick indices chosen to let the world settle
        // (terrain/camera/spawn) and cover a couple of seconds of
        // deterministic simulation at the default 60Hz fixed timestep.
        // Gated the same way as the HUD screenshot above (avoids the
        // Vulkan mid-draw-readback corruption).
        if (goldenCaptureMode_ && drawFrameIndex_ > terrainPixelPrintedFrame_)
        {
            if (goldenCaptureAwaitingDraw_ &&
                goldenCaptureNextIndex_ < kGoldenCaptureCount)
            {
                int w = 0, h = 0;
                GetBackBufferSize(device, w, h);
                std::vector<Microsoft::Xna::Framework::Color> backBuffer(
                    static_cast<std::size_t>(w) * static_cast<std::size_t>(h),
                    Microsoft::Xna::Framework::Color(0, 0, 0, 0));
                device.GetBackBufferData(backBuffer.data(), 0, static_cast<int>(backBuffer.size()));
                Microsoft::Xna::Framework::Graphics::Texture2D goldenShot(device, w, h);
                goldenShot.SetData(backBuffer.data(), static_cast<int>(backBuffer.size()));
                char filename[64];
                std::snprintf(filename, sizeof(filename), "golden_frame_%04d.png",
                              kGoldenCaptureTicks[goldenCaptureNextIndex_]);
                goldenShot.SaveAsPng(filename);
                std::cout << "GalaxyEggbertCNA: wrote " << filename << " (" << w << "x" << h << ")."
                          << std::endl;
                ++goldenCaptureNextIndex_;
                goldenCaptureAwaitingDraw_ = false;
                if (goldenCaptureNextIndex_ >= kGoldenCaptureCount)
                {
                    std::cout << "GalaxyEggbertCNA: golden capture complete, exiting." << std::endl;
                    Exit();
                }
            }
        }
    }

    GetTypeNameCPP(GalaxyEggbertGame, "GalaxyEggbertGame")
}
