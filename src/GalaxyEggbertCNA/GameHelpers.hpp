#pragma once

// File-local helpers of GalaxyEggbertGame shared by its translation units (GalaxyEggbertGame*.cpp);
// not a public interface. Moved from GalaxyEggbertGame.cpp's anonymous namespaces.

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

#include "GameInternal.hpp"

namespace GalaxyEggbert::CNA::detail
{
    using namespace GalaxyEggbert::Game;

        inline const std::string& ResumeBuild()
        {
            // Function-local static: even a failed read is attempted only once per process.
            static const std::string build = []
            {
                std::string result = __DATE__ " " __TIME__;
                std::filesystem::path executable;
#if defined(__linux__) && !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
                executable = "/proc/self/exe";
#elif defined(_WIN32)
                std::vector<wchar_t> path(32768);
                const auto size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
                if (size > 0 && size < path.size()) executable = std::wstring(path.data(), size);
#elif defined(__APPLE__)
                std::uint32_t size = 0;
                _NSGetExecutablePath(nullptr, &size);
                std::vector<char> path(size);
                if (_NSGetExecutablePath(path.data(), &size) == 0) executable = path.data();
#endif
                if (!executable.empty())
                    if (const auto hash = ResumeLog::HashFile(executable))
                        result += " fnv1a64=" + std::to_string(*hash);
                return result;
            }();
            return build;
        }

        inline std::string MissionWorldPath(int mission)
        {
            char path[64];
            std::snprintf(path, sizeof(path), "worlds3d/world%03d.vwr", mission);
            return path;
        }

        inline std::optional<std::string> ReadWorldBytes(const std::string& path)
        {
            std::ifstream in(path, std::ios::binary);
            if (!in) return std::nullopt;
            std::ostringstream bytes;
            bytes << in.rdbuf();
            if (in.bad() || !bytes) return std::nullopt;
            return bytes.str();
        }


        // The first connected gamepad as keys (including a pad assigned to another XNA slot).
        // GamepadInput.hpp: the owner's request of 2026-11-02; Mobile Eggbert has no gamepad.
        inline GalaxyEggbert::Game::GamepadFrame ReadGamepadFrameImpl()
        {
            using namespace Microsoft::Xna::Framework::Input;
            GalaxyEggbert::Game::GamepadFrame f;
            GamePadState pad;
            using Microsoft::Xna::Framework::PlayerIndex;
            for (const auto player : {PlayerIndex::One, PlayerIndex::Two, PlayerIndex::Three, PlayerIndex::Four})
            {
                pad = GamePad::GetState(player);
                if (pad.getIsConnectedProperty()) break;
            }
            f.connected = pad.getIsConnectedProperty();
            if (!f.connected)
            {
                return f;
            }
            const auto down = [](ButtonState s) { return s == ButtonState::Pressed; };
            const auto& stick = pad.getThumbSticksProperty().getLeftProperty();
            f.leftX = stick.X;
            f.leftY = stick.Y;
            const auto& rightStick = pad.getThumbSticksProperty().getRightProperty();
            f.rightX = rightStick.X;
            f.rightY = rightStick.Y;
            f.leftTrigger = pad.getTriggersProperty().getLeftProperty();
            f.rightTrigger = pad.getTriggersProperty().getRightProperty();
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

        inline Microsoft::Xna::Framework::Input::KeyboardState ReadKeyboardState(bool suppressLiveInput, bool mergeGamepad = true, bool ctrlFires = false)
        {
            using namespace Microsoft::Xna::Framework::Input;
            if (suppressLiveInput)
            {
                return KeyboardState{};
            }
            const KeyboardState keyboard = GalaxyEggbert::Game::VehicleFireKeyboard(Keyboard::GetState(),ctrlFires);
            if (!mergeGamepad) return keyboard;
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
                {kPadConfirm, Keys::Enter}, {kPadFire, Keys::F}, {kPadCheatKeyboard, CheatKeyboard::kHoldKey}};
            for (const auto& [bit, key] : map)
            {
                if (pad & bit)
                {
                    keys.insert(key);
                }
            }
            return KeyboardState(keys);
        }

        inline Microsoft::Xna::Framework::Input::MouseState ReadMouseState(bool suppressLiveInput)
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
        inline const std::string& BlupiAnimStateToPlaceholderClipName(BlupiController::AnimState state)
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

        // kRealModelScale: GameInternal.hpp.

        // Vehicle pickups the real vehicle models replace when parked: the
        // model (avatars3d/blupi/vehicles/<name>) and the clip whose first
        // frame is its standing pose.
        struct ParkedVehicle
        {
            GalaxyEggbert::Def::ObjectType type;
            const char* model;
            const char* clip;
        };
        inline constexpr ParkedVehicle kParkedVehicles[] = {
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
        inline float TurnAroundSeconds(BlupiController::AnimState state)
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
        inline std::string MissingClipFallback(const BlupiController& blupi)
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
        inline std::string BlupiClipName(const BlupiController& blupi)
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
        inline constexpr VehicleModeMapping kVehicleModeTable[] = {
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
        inline GalaxyEggbert::Def::ObjectType VehicleModeToObjectType(BlupiController::VehicleMode mode)
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

        inline bool ObjectTypeToVehicleMode(GalaxyEggbert::Def::ObjectType type, BlupiController::VehicleMode& outMode)
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

        inline int GetCreatureIconIgnoringDirection(bool /*patrolGoesLeftFromStart*/, int patrolStep, int patrolTimeTicks)
        {
            return GetCreatureIcon(patrolStep, patrolTimeTicks);
        }

        struct PatrolIconHandler
        {
            GalaxyEggbert::Def::ObjectType type;
            PatrolIconFn iconFn;
        };
        inline constexpr PatrolIconHandler kPatrolIconHandlers[] = {
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
        inline bool TryGetPatrolIconImpl(GalaxyEggbert::Def::ObjectType type, bool patrolGoesLeftFromStart, int patrolStep,
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
        inline constexpr float kWaitDurationSeconds = 5.0f;

        // Real fade-out phase-transition commit timer (plan.md
        // MENU-088/089, confirmed via research: `Config::ScaleTime(20)` =
        // 1.0s at this build's pinned 20fps) -- must match
        // InputPad.cpp's own kFadeDurationSeconds (kept as a
        // separately-owned constant there, same cross-file convention as
        // kWaitDurationSeconds above).
        inline constexpr float kFadeCommitDurationSeconds = 1.0f;
        // Into the Setup the menu waits a quarter of that (the owner, 2026-11-09: the Setup took two
        // seconds to come up, the menu's second and its own slide-in); the other screens keep the
        // original's second.
        inline constexpr float kSetupFadeCommitDurationSeconds = 0.25f;

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
        inline void ApplyTerrainLighting(Microsoft::Xna::Framework::Graphics::BasicEffect& effect)
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
        inline bool BlocksCamera(const GalaxyEggbert::Worlds::World& world, const Microsoft::Xna::Framework::Vector3& point)
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

        inline float RaymarchWallDistance(const GalaxyEggbert::Worlds::World& world,
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
        // A converted model's reach (objectModelRadii_): the farthest vertex from its origin over
        // its parts' vertex files (export_cna_avatar.py: 52-byte vertices, the position first),
        // its bind pose; animated parts stay near it (the frustum test adds a margin).
        inline float ModelFileReach(const std::string& directory)
        {
            constexpr std::size_t kStride = 52;
            float reach = 0.0f;
            std::error_code error;
            for (const auto& entry : std::filesystem::directory_iterator(directory, error))
            {
                const auto path = entry.path().string();
                if (path.size() < 10 || path.compare(path.size() - 10, 10, ".verts.bin") != 0)
                {
                    continue;
                }
                std::ifstream in(path, std::ios::binary);
                std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                for (std::size_t at = 0; at + 12 <= bytes.size(); at += kStride)
                {
                    float p[3];
                    std::memcpy(p, bytes.data() + at, sizeof(p));
                    reach = std::max(reach, std::sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]));
                }
            }
            return reach > 0.0f ? reach : 4.0f;
        }

        // A BigDecor model's shape (SolidBox.hpp BigDecorShapes): its vertex files' positions as the
        // game draws the model for `pose` (its own yaw, flip and sprite offset; standing on the
        // cell's floor, `halfBlocks` its picture's half height), one box per half block of height
        // over the vertices in it; a slice without vertices (along a trunk) takes the narrower of its
        // neighbours'. In blocks from the anchor cell's centre and floor.
        inline std::vector<GalaxyEggbert::Game::SolidBox> ModelFileShape(const std::string& directory,
                                                                  const GalaxyEggbert::Game::ObjectModelPose& pose,
                                                                  float halfBlocks)
        {
            constexpr std::size_t kStride = 52;
            constexpr float kSlice = 0.5f;
            constexpr float kDegrees = 3.14159265358979f / 180.0f;
            const float c = std::cos(pose.yawDegrees * kDegrees), s = std::sin(pose.yawDegrees * kDegrees);
            std::vector<std::array<float, 3>> points;
            std::error_code error;
            for (const auto& entry : std::filesystem::directory_iterator(directory, error))
            {
                const auto path = entry.path().string();
                if (path.size() < 10 || path.compare(path.size() - 10, 10, ".verts.bin") != 0)
                {
                    continue;
                }
                std::ifstream in(path, std::ios::binary);
                std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                for (std::size_t at = 0; at + 12 <= bytes.size(); at += kStride)
                {
                    float p[3];
                    std::memcpy(p, bytes.data() + at, sizeof(p));
                    // CreateRotationY (row vectors): x' = x c + z s, z' = -x s + z c; then the flip
                    // and the sprite offset, then up onto the floor.
                    float x = p[0] * c + p[2] * s;
                    float y = p[1];
                    const float z = -p[0] * s + p[2] * c;
                    if (pose.mirror == 1)
                    {
                        x = -x - GalaxyEggbert::Game::kFlipShift;
                    }
                    else if (pose.mirror == 2)
                    {
                        y = -y + GalaxyEggbert::Game::kFlipShift;
                    }
                    x += static_cast<float>(pose.offsetX) / 64.0f;
                    y += -static_cast<float>(pose.offsetY) / 64.0f + halfBlocks;
                    points.push_back({x, y, z});
                }
            }
            std::vector<GalaxyEggbert::Game::SolidBox> shape;
            if (points.empty())
            {
                return shape;
            }
            float top = 0.0f;
            for (const auto& p : points)
            {
                top = std::max(top, p[1]);
            }
            const int slices = std::max(1, static_cast<int>(std::ceil(top / kSlice)));
            std::vector<GalaxyEggbert::Game::SolidBox> slice(static_cast<std::size_t>(slices));
            std::vector<bool> filled(static_cast<std::size_t>(slices), false);
            for (const auto& p : points)
            {
                const int k = std::clamp(static_cast<int>(std::floor(p[1] / kSlice)), 0, slices - 1);
                auto& box = slice[static_cast<std::size_t>(k)];
                if (!filled[static_cast<std::size_t>(k)])
                {
                    box = {p[0], p[0], 0.0f, 0.0f, p[2], p[2]};
                    filled[static_cast<std::size_t>(k)] = true;
                }
                box.minX = std::min(box.minX, p[0]);
                box.maxX = std::max(box.maxX, p[0]);
                box.minZ = std::min(box.minZ, p[2]);
                box.maxZ = std::max(box.maxZ, p[2]);
            }
            const auto area = [](const GalaxyEggbert::Game::SolidBox& b) { return (b.maxX - b.minX) * (b.maxZ - b.minZ); };
            for (int k = 0; k < slices; ++k)
            {
                if (filled[static_cast<std::size_t>(k)])
                {
                    continue;
                }
                const GalaxyEggbert::Game::SolidBox* best = nullptr;
                for (int d = 1; d < slices && best == nullptr; ++d)
                {
                    const GalaxyEggbert::Game::SolidBox* below = k - d >= 0 && filled[static_cast<std::size_t>(k - d)]
                                                                     ? &slice[static_cast<std::size_t>(k - d)] : nullptr;
                    const GalaxyEggbert::Game::SolidBox* above = k + d < slices && filled[static_cast<std::size_t>(k + d)]
                                                                     ? &slice[static_cast<std::size_t>(k + d)] : nullptr;
                    best = below && above ? (area(*below) <= area(*above) ? below : above) : (below ? below : above);
                }
                if (best != nullptr)
                {
                    slice[static_cast<std::size_t>(k)] = *best;
                }
            }
            for (int k = 0; k < slices; ++k)
            {
                auto box = slice[static_cast<std::size_t>(k)];
                if (box.maxX - box.minX < 0.05f || box.maxZ - box.minZ < 0.05f)
                {
                    continue; // a flat leaf or nothing: no box
                }
                box.minY = kSlice * static_cast<float>(k);
                box.maxY = kSlice * static_cast<float>(k + 1);
                shape.push_back(box);
            }
            return shape;
        }


        // Puts `spec` in the first free slot of the world's mobile objects (or at the end).
        inline void PlaceMobileObject(GalaxyEggbert::Game::WorldRuntime& worldRuntime, const GalaxyEggbert::Game::MobileObjSpec& spec)
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
