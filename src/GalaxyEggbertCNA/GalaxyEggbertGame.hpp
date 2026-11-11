#pragma once

#include <GalaxyEggbert/Game/ResumeLog.hpp>
#include <GalaxyEggbert/Game/DemoReplay.hpp>
#include <GalaxyEggbert/Game/ReusablePickup.hpp>
#include <map>

#include <GalaxyEggbert/Game/WorldRuntime.hpp>
#include <GalaxyEggbert/HubDoors.hpp>
#include <GalaxyEggbert/Game/TileAtlas.hpp>
#include <GalaxyEggbert/Game/TerrainRenderer.hpp>
#include <GalaxyEggbert/Game/BlupiController.hpp>
#include <GalaxyEggbert/Game/CameraShake.hpp>
#include <GalaxyEggbert/Game/CharredBlupi.hpp>
#include <GalaxyEggbert/Game/ChaseCameraYaw.hpp>
#include <GalaxyEggbert/Game/ObjectIcons.hpp>
#include <GalaxyEggbert/Game/ObjectModelTable.hpp>
#include <GalaxyEggbert/Game/Sound.hpp>
#include <GalaxyEggbert/Game/InteractionSystem.hpp>
#include <GalaxyEggbert/Game/Hud.hpp>
#include <GalaxyEggbert/Game/TrainingHints.hpp>
#include <GalaxyEggbert/Game/InputPad.hpp>
#include <GalaxyEggbert/Game/FocusNavigator.hpp>
#include <GalaxyEggbert/Game/ScreenKeyboard.hpp>
#include <GalaxyEggbert/Net/NetProtocol.hpp>
#include <GalaxyEggbert/Editor/EditorDeviceControls.hpp>
#include <GalaxyEggbert/Editor/EditorCommandKeys.hpp>
#include <GalaxyEggbert/Game/ControlInput.hpp>
#include <GalaxyEggbert/Game/SaveData.hpp>
#include <GalaxyEggbert/Game/Autoplay.hpp>
#include <GalaxyEggbert/Editor/WorldEditor.hpp>

#include <GalaxyEggbert/Def/GamePhase.hpp>
#include <GalaxyEggbert/Def/GameSpeed.hpp>

#include <GalaxyEggbert/Game/TexturedMesh.hpp>
#include <GalaxyEggbert/Game/SceneFrame.hpp>
#include <GalaxyEggbert/Game/EffectParticles.hpp>
#include <GalaxyEggbert/Game/PostProcess.hpp>
#include <GalaxyEggbert/Game/UltraFinish.hpp>
#include <GalaxyEggbert/Game/WorldRenderer.hpp>
#include <CnaEngine/Graphics/Camera.hpp>
#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Graphics/SkinnedEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/SkinnedModelEXT.hpp>
#include <Microsoft/Xna/Framework/GraphicsDeviceManager.hpp>
#include <Microsoft/Xna/Framework/Input/KeyboardState.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Devices/Sensors/Accelerometer.hpp>
#include <Microsoft/Xna/Framework/Media/Song.hpp>

#include <chrono>
#include <CnaEngine/Graphics/DebugDraw.hpp>
#include <cstdint>
#include <deque>
#include <optional>
#include <cstdio>
#include <filesystem>
#include <array>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace GalaxyEggbert::CNA
{
    // Direct CNA + cna-engine target: loads a hand-authored .vwr world, renders
    // real textured/animated 3D terrain, and moves an invisible,
    // collision-only Blupi placeholder through it (see NEXT.md for current
    // status). Two camera modes exist (2026-07-09): third-person, the
    // default since 2026-10-25 (user request), showing the real Blupi
    // converted from the Ray Dream scenes (avatars3d/blupi/) through CNA's
    // SkinnedModelEXT/SkinnedEffect APIs (see ../cna/docs/skinned-model-ext.md),
    // and first-person, Blupi's animation state shown via a small 2D
    // indicator in the screen's bottom-right corner. F3 toggles between
    // them: it was "C" until 2026-10-25, but letters are typed cheats
    // ("ghost", "quick").
    struct NetGame; // the network game's state (NetGame.hpp)

    class GalaxyEggbertGame final : public Microsoft::Xna::Framework::Game
    {
    public:
        GalaxyEggbertGame();
        ~GalaxyEggbertGame() override;

        void LoadContent() override;
        void OnDeactivated(System::Object* sender, const System::EventArgs& args) override;
        void OnExiting(System::Object* sender, const System::EventArgs& args) override;
        void Update(Microsoft::Xna::Framework::GameTime& gameTime) override;
        void Draw(const Microsoft::Xna::Framework::GameTime& gameTime) override;

        // INFRA-001 (plan.md §7, "Correctness Infrastructure" vision): a
        // permanent, committed deterministic golden-screenshot capture
        // mode, replacing the ad-hoc "xvfb-run + env-var-gated debug hook,
        // reverted before commit" pattern used throughout this project's
        // history (see NEXT.md §3 for recent examples) with a real,
        // always-available feature. Must be called before Run() (see
        // main.cpp's `--golden-capture` flag). Once enabled: skips
        // straight to Play (bypassing the menu), loads the fixed demo
        // world (`worlds3d/world999.vwr`, this engine's own established
        // "known working demo" — see NEXT.md §2), and lets the game's
        // already-deterministic fixed timestep (CNA's default
        // `IsFixedTimeStep=true`, 1/60s, confirmed in `Game::Game()`) run
        // for a fixed number of ticks. At each of a few fixed tick
        // indices (`kGoldenCaptureTicks` in the .cpp), captures a
        // screenshot to a well-known filename (`golden_frame_NNNN.png`),
        // then calls `Exit()` once the last one is captured — so this can
        // run unattended (e.g. under `xvfb-run`) and terminate on its
        // own, no external timeout needed. No golden-image diffing yet
        // (`INFRA-002`) — this step only proves the capture itself is
        // stable/reproducible run to run.
        void EnableGoldenCaptureMode() noexcept { goldenCaptureMode_ = true; }

        // INFRA-002's "behavioral trace" half of `REMAKE-ANALYSIS.md` P0-1
        // (plan.md §7) -- found missing 2026-07-22 during an external audit
        // of the already-"done" `INFRA-001`/`002` entries (see plan.md's own
        // corrected writeups). Unlike `EnableGoldenCaptureMode()` above (a
        // completely passive capture -- Blupi never receives any simulated
        // input, only the world's own autonomous objects move), this mode
        // ALSO drives Blupi through a small fixed, deterministic input
        // script (walk forward down the same "tested corridor" `VerifyBlupiMovement.cpp`
        // already depends on staying geometrically unchanged, jump partway
        // through, then stop) so real movement/collision code is actually
        // exercised, not just watched at rest. Deliberately a SEPARATE mode
        // from `EnableGoldenCaptureMode()` rather than added to it -- the
        // existing golden_frame_*.png references stay valid/unaffected by
        // this addition (2026-07-22 scoping decision, not adding scripted
        // input retroactively to the approved screenshot baseline). Records
        // one line per tick (position/velocity-Y/grounded/anim state) to an
        // in-memory buffer, writes it to `golden_trace.txt` once the fixed
        // tick count completes, then exits -- same "run once, write a
        // well-known file, exit" shape as the screenshot mode.
        void EnableGoldenTraceMode() noexcept { goldenTraceMode_ = true; }

        // Development/QA start (2026-10-25): `--mission N` skips the menus
        // and starts mission N in Play, the way PauseRestart loads one
        // (LoadMission()), so live checks reach a world without driving the
        // menus. Not a player-facing feature.
        void EnableStartMission(int missionNumber) noexcept { startMission_ = missionNumber; }
        // `--galaxy`: --mission N is the selected player's galaxy's (BD-0119; the QA autoplay plays it).
        void EnableGalaxyStart() noexcept { startGalaxy_ = true; }
        // `--spawn x,y,z` with `--mission`: Blupi starts at that raw-grid cell
        // (the coordinates `.vwr` files and tools/vwr_inspect.py use), facing
        // `--yaw` degrees, instead of the world's own spawn.
        void EnableStartSpawn(float x, float y, float z, float yawDegrees) noexcept
        {
            startSpawnSet_ = true;
            startSpawnX_ = x;
            startSpawnY_ = y;
            startSpawnZ_ = z;
            startYawDegrees_ = yawDegrees;
        }
        // `--autoplay <script>` with `--mission`: the QA autoplay (Autoplay.hpp) plays instead of
        // the keyboard and mouse, one fixed 1/60 s step per Update() as fast as the machine runs,
        // writes `autoplay_trace.txt` and exits when the script ends (code 0 when it passed). False
        // when the script cannot be read or parsed.
        bool EnableAutoplay(const std::string& scriptPath);
        [[nodiscard]] int AutoplayExitCode() const noexcept { return autoplayExitCode_; }
        // `--record-demo <file>` with `--mission` (and `--autoplay` for a scripted one): records the
        // mission from its start as a main-menu demo (DemoReplay.hpp) until Escape, the mission's
        // end, the autoplay's end, `--demo-ticks N` or the window closing, writes it and exits.
        // F-keys and Tab (game speed) are left out of the recording. tools/record_demo.sh.
        void EnableDemoRecording(const std::string& path, int maxTicks, const std::string& title);
        // `--verify-demo <file>`: replays that demo at once, undrawn and silent, compares its end
        // state with the recorded one and exits (code 0 when equal). tools/verify_demos.sh.
        void EnableDemoVerify(const std::string& path);

        GetTypeNameHPP()

    private:
        // Camera mode (2026-07-09, NEXT.md §3) -- toggled by F3, edge-
        // detected the same way as the demo_avatar example's Space-toggle
        // (cameraModeKeyWasDown_ below) so a held key doesn't rapid-fire.
        // Third-person by default; LoadContent() falls back to first-person
        // when no Blupi model loads.
        enum class CameraMode : std::uint8_t { FirstPerson, ThirdPersonModel };
        CameraMode cameraMode_ = CameraMode::ThirdPersonModel;
        bool cameraModeKeyWasDown_ = false;
        // Drives terrainEffect_'s View/Projection every frame. Its view is
        // always XNA's CreateLookAt of its position and cameraTarget_ (set
        // together through Camera::SetLookAt); it starts at (0, 2, 5) looking
        // at the origin (see the constructor).
        CnaEngine::Graphics::Camera camera_;
        Microsoft::Xna::Framework::Vector3 cameraTarget_{0.0f, 0.0f, 0.0f};

        // Smoothed camera eye/target (2026-07-09) -- both camera modes below
        // compute a raw "desired" eye/target from Blupi's live
        // position/yaw every frame; snapping camera_ straight to that
        // value feels too fast/jerky (reported live). Instead the raw
        // value is exponentially damped toward camera_'s actual
        // position/target each frame (see kCameraDampingPerSecond in the
        // .cpp). Initialized lazily on the first Update() so load-time
        // doesn't spend a visible lerp-in from the origin.
        bool cameraSmoothedInitialized_ = false;
        Microsoft::Xna::Framework::Vector3 cameraEyeSmoothed_{0.0f, 0.0f, 0.0f};
        Microsoft::Xna::Framework::Vector3 cameraTargetSmoothed_{0.0f, 0.0f, 0.0f};
        // Third-person orbit around Blupi (behind, lagging, side views; see
        // ChaseCameraYaw.hpp); reset behind him while in first person.
        GalaxyEggbert::Game::ChaseCameraYaw chaseCamera_;

        // Parses worlds/world001.txt (plan.md Phase 4).
        GalaxyEggbert::Game::WorldRuntime worldRuntime_;

        // Real mobile-eggbert background image for worldRuntime_'s skyRegion
        // (2026-07-09, NEXT.md §3 -- mobile-eggbert-reference/
        // 05-backgrounds.md/15-3d-render-mapping-design.md §9.5). Drawn as a
        // single huge camera-facing billboard quad placed far behind the
        // scene (same MeshBuilder::AddBillboard/GpuMesh
        // technique already used for MoveObjects/BigDecor, just one giant
        // quad sized to fill the whole view frustum at that distance) --
        // NOT a 2D SpriteBatch overlay. Tried SpriteBatch first (drawn
        // full-screen before the 3D scene, relying on it not writing depth);
        // that broke live (2026-07-09): EasyGLSpriteBatchBackend::Begin()
        // enables alpha blending and never restores it on End(), and more
        // fundamentally the SpriteBatch/BasicEffect draw paths turned out
        // not to compose safely when SpriteBatch runs BEFORE 3D draws in the
        // same frame (only ever exercised the other order before, e.g.
        // blupiIconBatch_ drawn last) -- the background ended up covering
        // the entire screen with no terrain visible at all. A real,
        // depth-tested 3D quad sidesteps this entirely: normal opaque 3D
        // terrain, being closer, correctly occludes it via the depth buffer
        // like any other geometry, no draw-order fragility. This
        // deliberately does NOT attempt a true wraparound 3D skybox --
        // §9.5 already found the source art (a flat 640x480 image designed
        // for a fixed 2D side-camera) a poor fit for that; one large flat
        // backdrop plane facing the camera is a different, smaller step
        // than projecting the image onto real skybox geometry.
        // `Content/backgrounds/decorNNN.png` is already copied next to this
        // binary (existing mobile-eggbert Content/ copy, CMakeLists.txt) --
        // no new asset-pipeline work needed. backgroundTexture_/Effect_ stay
        // default-constructed (unloaded) if the region's file doesn't exist
        // (e.g. one of the 4 ids no real level ever uses) or before
        // LoadContent() runs; Draw() falls back to the flat device.Clear()
        // color in that case.
        Microsoft::Xna::Framework::Graphics::Texture2D backgroundTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> backgroundEffect_;
        bool backgroundLoaded_ = false;

        // Maps block types to object-m.png UV rects (plan.md E3D-MIG-053).
        GalaxyEggbert::Game::TileAtlas tileAtlas_;
        // The terrain's own padded, mipmapped copy of object-m.png and its UVs (TileAtlas.hpp Layout::Padded).
        GalaxyEggbert::Game::TileAtlas terrainTileAtlas_{GalaxyEggbert::Game::TileAtlas::Layout::Padded};
        Microsoft::Xna::Framework::Graphics::Texture2D terrainMipTexture_;
        // Per object-m.png icon: drawn opaque, cut out or blended (TileCoverage.hpp).
        std::vector<GalaxyEggbert::Game::TileCoverage> tileCoverage_;

        // Static (non-animated) terrain mesh for the loaded world (plan.md
        // E3D-MIG-054). Lazily constructed in LoadContent() since it needs a
        // live GraphicsDevice.
        // Declared before the renderer, which reads it on worker threads: destroyed after it.
        GalaxyEggbert::Game::TerrainReliefTable terrainReliefs_;
        std::unique_ptr<GalaxyEggbert::Game::TerrainRenderer> terrainRenderer_;
        std::uint32_t terrainRevision_ = 0; // World::blockRevision() terrainRenderer_ was built from
        // The ShowSecret cheat (3, Decor::CheatAction's m_bDrawSecret): the flight barrier (214),
        // invisible in play, is drawn while it is on and in the editor (TerrainDrawSecret()).
        bool cheatShowSecret_ = false;
        bool terrainDrawSecret_ = false;   // whether terrainRenderer_ was built drawing it
        bool holdingCrate_ = false;        // crouching with a crate in front, to pull it (StopPop/Pop)
        bool winCheerActive_ = false;      // the Win cheer at the exit plays; then AdvanceAfterWin()
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> terrainEffect_;

        // object-m.png, loaded directly via CNA's own Texture2D (no
        // mobile-eggbert Pixmap reuse — that class is 2D SpriteBatch-coupled,
        // not usable for this 3D BasicEffect draw path; see easy3d.md §5.2).
        Microsoft::Xna::Framework::Graphics::Texture2D terrainTexture_;

        // Icon 107's grass-top overlay (NEXT.md §8 task 3) — a genuinely
        // separate, galaxy-eggbert-owned asset (textures3d/grass_top.png,
        // copied next to the binary like worlds3d/), NOT part of
        // object-m.png (which is fully re-copied from ../mobile-eggbert on
        // every build, so nothing can be added into it). Needs its own
        // BasicEffect since BasicEffect only binds one texture at a time;
        // drawn via TerrainRenderer::DrawGrass() right after the main
        // terrain Draw() call.
        Microsoft::Xna::Framework::Graphics::Texture2D grassTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> grassEffect_;

        // Invisible, collision-only movement placeholder (plan.md
        // E3D-MIG-060) — arrow keys + Space, first-person camera follows
        // its facing. No 3D sprite yet (E3D-MIG-061..063) — see blupiIcon_
        // below for the interim 2D stand-in.
        GalaxyEggbert::Game::BlupiController blupi_;

        // Sucette/Drink/Charge real 2-stage pickup delay (plan.md `173`) --
        // stashes the pickup's own position/type from contact time until
        // the freeze resolves (potentially several frames/seconds later,
        // past when interaction_'s own *ThisFrame() signal has reset), so
        // ResolvePickupFreeze() can re-spawn it at completion. Only one
        // freeze can be active at a time (BlupiController::
        // TriggerPickupFreeze() is a no-op while already frozen), so a
        // single pending slot is sufficient.
        GalaxyEggbert::Game::ReusablePickup reusablePickup_;

        // Real screen-shake/forced-pan camera effect (plan.md CAM-008..013,
        // see CameraShake.hpp's own comment for the full real-behavior
        // citation) -- ticked every Play-phase frame, its (dx,dy) pixel
        // offset applied as a small camera-space perturbation after the
        // normal eye/target damping (see Draw()'s own camera block).
        GalaxyEggbert::Game::CameraShake cameraShake_;

        // Real mobile-eggbert sound playback (2026-07-10, see Sound.hpp).
        // jumpKeyWasDown_ edge-detects the jump key the same way
        // cameraModeKeyWasDown_ edge-detects F3 above.
        GalaxyEggbert::Game::Sound sound_;
        bool jumpKeyWasDown_ = false;

        // Real "Bye" farewell freeze (plan.md BLUPI-049) -- the mission to
        // load once `blupi_.IsBye()` naturally clears; set only at the world-
        // select portal contact site (`Update()`), consumed by the
        // `wasBye`/`IsBye()` completion check right after `Step()`.
        int byePendingTarget_ = 0;

        // Switch/saw linking (plan.md E3D-MIG-142, see WorldRuntime::
        // TryActivateSwitch()) -- Space ("Action", already read as
        // actionPressed) edge-detected the same way jumpKeyWasDown_ is.
        bool actionKeyWasDown_ = false;

        // Real crate-push loop sound (found 2026-07-16, ch38) -- tracks last frame's
        // `interaction_.CrateBeingPushedThisFrame()` so the start/stop transition can be
        // detected (the flag itself carries no memory across frames).
        bool wasPushingCrate_ = false;

        // Real vehicle motor sound crossfade (plan.md SOUND-007/008, found 2026-07-17) -- mirrors
        // real `m_blupiMotorSound`: the channel of the currently-playing loop, or SoundChannel0
        // (the real sentinel) while none is playing. Needed because the desired loop channel can
        // change directly from one loop to another (e.g. moving->idle) without ever passing
        // through "no motor", so a plain this-frame/last-frame bool (like wasPushingCrate_ above)
        // isn't enough -- see UpdateVehicleMotorSound()'s own comment for the full crossfade.
        GalaxyEggbert::Def::SoundChannel activeMotorLoop_ = GalaxyEggbert::Def::SoundChannel::SoundChannel0;
        // The mockery's sound (Decor.cpp:3147-3183, 6457-6470): played at its variant's tick (Mockery
        // 1, Mockeryi 6, Mockeryp 4) and cut when he leaves it. The variant playing (Stop: none),
        // the seconds since it began, whether its sound has played.
        GalaxyEggbert::Game::BlupiController::AnimState mockeryVariant_ =
            GalaxyEggbert::Game::BlupiController::AnimState::Stop;
        float mockeryElapsed_ = 0.0f;
        // The helicopter lost, not put down (wrecked, stung, glued, dead, at the exit) flies away
        // (Decor::ByeByeHelico; BD-0049): last frame's mode, and whether the dismount parked it.
        bool wasInHelicopter_ = false;
        bool helicopterDeposited_ = false;
        // Hide's trail (ObjectType58, Decor.cpp:5236-5250, 870-881; BD-0056): every 10 px he moves
        // while hidden, a copy of his pose that fades from 30 % to nothing over 20 ticks.
        struct HideGhost
        {
            Microsoft::Xna::Framework::Matrix world;
            std::string clip;
            double clipSeconds = 0.0;
            Microsoft::Xna::Framework::Graphics::Texture2D* face = nullptr;
            float ageTicks = 0.0f;
        };
        std::vector<HideGhost> hideGhosts_;
        bool hideGhostTracking_ = false;
        float hideGhostX_ = 0.0f, hideGhostY_ = 0.0f, hideGhostZ_ = 0.0f;
        bool mockerySounded_ = false;

        // Platform lift patrol, crate push, and pickup collection
        // (2026-07-10, see InteractionSystem.hpp).
        GalaxyEggbert::Game::InteractionSystem interaction_;

        // Real GalaxyEggbert::Def::Phase state machine (2026-07-13, plan.md HUD-023),
        // verified directly against Def.hpp's own enum (already ported
        // verbatim as GalaxyEggbert::Def::GamePhase) and Game1.cpp's real
        // SetPhase()/Update() dispatch. Real source's ONLY phase that runs
        // simulation (Decor::MoveStep(), i.e. this class's own worldRuntime_/
        // blupi_/interaction_ Update() calls) is Play -- every other phase
        // freezes it, confirmed by Game1.cpp:394-438's own `if (phase==Play)`
        // gate.
        //
        // `Wait`/`Init` are now real AND reachable (2026-07-13, plan.md
        // MENU-001..020, a dedicated research pass) -- this engine starts
        // in `Wait` (see phase_'s own default member initializer below),
        // matching the real source's own `First`->`Wait` transition
        // (`First`'s synchronous asset-loading step already happens
        // unconditionally in this class's own `LoadContent()`, so `First`
        // itself is skipped as redundant, not a missing feature). `Wait`
        // shows the real wait.png/jauge.png progress gauge for a FIXED
        // 5.0s cosmetic timer (confirmed via research: real `waitProgress`
        // is wall-clock-based, decoupled from actual loading), then
        // transitions to Resume when resume.txt is valid, or when SaveData has checkpoint
        // progress (the fallback). ResumeLog replays the interrupted mission's input history
        // for Game1.cpp:255-270's CurrentRead behavior. Init renders
        // the real init.png/speedyblupi.png/blupiyoupie.png gamer-select
        // menu (3 independent slots via `SaveData`'s own 3-gamer-slot
        // extension) -- see `InputPad::UpdateInit()`/`DrawInit()`'s own
        // class comment for full detail, including what's deliberately
        // NOT ported (InitRanking/InitBuy, real exit-fade animations).
        //
        // `Trial`/`Ranking` are real enum values with NO trigger wired to
        // them yet (no upsell/ranking screens exist) -- reachable in
        // principle, unreachable in practice until those screens exist.
        // `MainSetup` is now ALSO reachable (2026-07-13, via Init's own
        // InitSetup button), sharing `InputPad::UpdateSetup()`/
        // `DrawSetup()` with `PlaySetup` (2026-07-13, plan.md
        // MENU-058..069, reachable via Pause's real Setup button) -- see
        // that method's own class comment for what is/isn't modeled on
        // that screen, including the one real difference between the two
        // (`SetupReset`, MainSetup-only). Resume restores the deactivated Play session
        // (Game1.cpp:186-208); without a valid log it loads the saved checkpoint.
        //
        // Real Pause trigger is gamepad-Back / a touch PlayPause button --
        // the real source has NO keyboard binding at all (an XNA/WP7 port);
        // Escape is this engine's own pick. Real PauseContinue resumes in
        // place (implemented); real PauseSetup opens PlaySetup
        // (implemented, 2026-07-13); real PauseRestart reloads the level
        // (simplified to an origin-respawn, see below); real PauseMenu
        // (return to main menu) now goes to `Init` (2026-07-13, now that
        // Init exists -- was previously unmodeled).
        //
        // Real fade-out phase transitions (plan.md MENU-088/089,
        // 2026-07-13, a dedicated research pass into the real
        // `fadeOutPhase` deferred-transition mechanic): `SetPhase()` now
        // takes an optional `bypassFade` param. Real source only defers
        // (plays a ~1.0s exit fade before actually committing) when the
        // CURRENT phase is one of exactly 5 real phases -- `Init`,
        // `MainSetup`, `PlaySetup`, `Pause`, `Resume` -- confirmed via
        // research; every other current phase (`Play`, `Win`, `Lost`,
        // `Wait`) commits INSTANTLY regardless of destination (Play<->Pause
        // is a real hard cut, not merely fast -- a documented correction,
        // not a simplification, since the real source genuinely has no
        // fade there). `fadeOutPhase_` tracks the pending destination
        // during that window; `Update()` freezes ALL input/simulation
        // while it's set (matching the real source's own early-return
        // before `inputPad.Update()`/`decor.MoveStep()`) and commits via
        // `SetPhase()` again once 1.0s elapses. `bypassFade=true` is the
        // real `mission==-2` sentinel (`ContinueMission()`) -- the ONE
        // real exception: Resume->Play (via ResumeContinue) is ALWAYS
        // instant even though Resume is one of the 5 deferring phases,
        // confirmed via research. See `InputPad::DrawPause()`/
        // `DrawResume()`/`DrawSetup()`/`DrawInit()`'s own class comments
        // for the exact real per-destination fade formulas.
        //
        // Real Lost trigger: a death animation completing while lives are
        // exhausted (`Decor.cpp:6374-6435`) -- this engine's own
        // `InteractionSystem::GameOverCount()` already increments at
        // exactly that real moment (`DoorsLost()`'s reset-to-3 behavior).
        // Real Win trigger: reaching the exit with all treasure
        // (`Decor::IsTerminated()`) -- already exactly what
        // `InteractionSystem::ExitReached()` gates on. Real WinLostReturn
        // input has no fixed auto-timer (explicit input only); this engine
        // reuses the Action key and resets Blupi to the origin spawn point
        // (not a full real level-reload, which needs infrastructure this
        // engine doesn't have -- lives/treasure/keys/etc. are NOT reset by
        // this, a documented simplification). Both Win and Lost also now
        // checkpoint `saveData_` (2026-07-13, plan.md MENU-040..045),
        // matching the real `MemorizeGamerProgress()` call sites
        // confirmed at exactly these same 2 real transition points.
        void SetPhase(GalaxyEggbert::Def::GamePhase next, bool bypassFade = false) noexcept;
        [[nodiscard]] const char* PhaseOverlayMessage() const noexcept;

        // Voyage (plan.md `158`) -- a no-op unless
        // `FindInteractionEvent(InteractionSystem::EventKind::VoyageRequested)` finds one. Projects
        // whichever endpoint is still a 3D world position (via
        // `Hud::ProjectWorldToHudSpace()`, using `camera_` and the real
        // GraphicsDevice's current viewport -- accessible here via
        // `getGraphicsDeviceProperty()`, the same base-Game accessor
        // already used elsewhere in this file for on-screen touch-control
        // hit-testing) and calls `interaction_.BeginVoyage()` with both
        // fully-resolved endpoints. Called right after BOTH
        // `interaction_.Update()` and `interaction_.TryPerso()` (idempotent
        // if neither set a pending request) since `TryPerso()`'s own call
        // site runs BEFORE `Update()`'s own per-frame flag reset.
        void ResolvePendingVoyage();

        // Death-lock/life-loss-Voyage follow-up -- a no-op unless
        // `FindInteractionEvent(InteractionSystem::EventKind::DeathLockRequested)` finds one
        // (the generic-hazard-contact/dynamite/projectile/large-creature-grab pending
        // signal from inside `interaction_.Update()`, see
        // `InteractionSystem::EventKind::DeathLockRequested`'s own
        // comment) OR `blupi_.ConsumeDeathLockResolved()` fires (an
        // ALREADY-active lock, possibly started a previous frame,
        // elapsing). Starts a new lock via `blupi_.TriggerDeathLock()` for
        // the former; for the latter, applies the real respawn (if
        // requested), predicts game-over via `interaction_.Lives() <= 1`
        // (skipping the Voyage entirely, matching real `DoorsLost()`'s "no
        // Voyage" path), and otherwise starts the real icon-48 life-loss
        // Voyage (`VoyageKind::LifeLoss`, which itself calls
        // `interaction_.LoseLife()` -- see `BeginVoyage()`'s own comment).
        // Called right after `interaction_.Update()` returns, alongside
        // `ResolvePendingVoyage()` above.
        void ResolveDeathLock();

        // Takes Blupi off the vehicle he rides with the action button and puts its pickup back
        // into the world where he is. The skateboard goes down 12 ticks into DeposeSkate, as the
        // original's (Decor.cpp:4527-4533): until then it is in his hands (OR-0003). A no-op if
        // not in a vehicle; plays no sound.
        void DismountAndDepositVehicle();
        // The vehicle he rides is wrecked: gone, an explosion (ObjectType9), a small shake, sound
        // 10 -- water (Decor.cpp:5415-5435), a spring (2835-2893) and the helicopter's ceiling
        // (5264-5283).
        void WreckVehicle();
        // Whether Blupi's death is shown by his element.png sprite instead of the model (OR-0015).
        [[nodiscard]] bool BlupiDrawnAsDeathSprite() const;
        // The mockery's sound at its tick, cut when he leaves it (mockeryVariant_).
        void UpdateMockerySound(float dt);
        // His charred frames (element.png 40-47, Clear1 and Clear3) drawn as his model
        // (CharredBlupi.hpp); not charred otherwise, or without the real model.
        [[nodiscard]] GalaxyEggbert::Game::CharredShape BlupiCharredShape() const;
        // The glue death's model for this frame (element 168-171 as Blupi/glu.rd4, recoloured green
        // at conversion as the sprites were), or null: then the sprite stands in. While it is
        // drawn, his own model lies flattened under it as the puddle of glue.
        [[nodiscard]] const GalaxyEggbert::Game::ObjectModelPose* BlupiGlueModel() const;
        // A conveyor lift's (47/48) tread model for its current frame (Decor/chenille.rds,
        // table_chenille/chenillei's 311-316), or the plain lift's (1, Decor/techno29.rd4), or
        // null: then it is drawn as a textured cube (the plain lift as its bar and stem).
        [[nodiscard]] const GalaxyEggbert::Game::ObjectModelPose* ConveyorLiftModel(
            const GalaxyEggbert::Game::MobileObjSpec& obj) const;
        // The skateboard on its way between the ground and his hands (OR-0003): put down at
        // TakeSkate's tick 8 is when the original deletes the board from the ground
        // (Decor.cpp:4510-4518), DeposeSkate's tick 12 when it starts it there.
        struct PendingSkate
        {
            bool active = false;
            bool take = false;        // true: delete `objectIndex`; false: place `deposit`
            float seconds = 0.0f;     // until it happens
            std::size_t objectIndex = 0;
            GalaxyEggbert::Game::MobileObjSpec deposit;
        };
        PendingSkate pendingSkate_;
        // The cell a teleporter put him in: no teleporter takes him while he stays in it. The
        // original's 30 px (m_blupiPosHelico) suffices there because its trigger is a 7 px band
        // (Decor::IsTeleporte, pos.X % 64 <= 6); Galaxy's is the whole cell under the pillar.
        bool teleportArrivalGuard_ = false;
        int teleportArrivalCellX_ = 0;
        int teleportArrivalCellZ_ = 0;
        // Automatic zoom on action (the Setup's SetupZoom, Decor::MoveHotSpot; OR-0010): the
        // chase camera's distance is divided by it (1.0 .. 1.3).
        float chaseZoom_ = 1.0f;
        float chaseZoomLingerSeconds_ = 0.0f;
        // The Setup's accelerometer (OR-0010): started while the setting is on and the device has
        // one; its sideways tilt steers as the pad's horizontal axis (InputPad.cpp:1932-1969).
        std::unique_ptr<Microsoft::Devices::Sensors::Accelerometer> accel_;
        // Whether the device has one (asked once, AccelTurnInput) and what was reported of it (the log).
        bool accelSupportProbed_ = false;
        bool accelSupported_ = false;
        bool accelReported_ = false;
        int accelTurnsReported_ = 0; // the first few turns it starts, logged
        int accelTicksWithoutData_ = 0;
        bool accelLastState_ = false;
        bool accelWaitZero_ = true;
        float AccelTurnInput();
        // The world's background music (OR-0014: World::musicTrack(), the original tracks as
        // Content/music/musicNNN.ogg): playing while in play (pause and setup included) and in
        // the editor, if sound is on; never in autoplay.
        std::unique_ptr<Microsoft::Xna::Framework::Media::Song> music_;
        std::uint32_t musicPlaying_ = 0;
        // Web only: the tracks asked of the server so far (UpdateMusic; OR-0014).
        std::set<std::uint32_t> musicRequested_;
        void UpdateMusic();
        // MediaPlayer's volume from the Setup's music slider (0.5 at 100, OR-0014's mix).
        void ApplyMusicVolume();
        void TickPendingSkate(float dt);

        // Hub/mission-progression system (plan.md `MENU-035`/`036`,
        // `SCORE-013`-`019`, found/implemented 2026-07-17). Rebuilds
        // `backgroundTexture_`/`backgroundEffect_` (skyRegion-dependent) and
        // `terrainRenderer_` from whatever's CURRENTLY loaded in
        // `worldRuntime_` -- shared by `LoadContent()`'s own initial load
        // (which handles `blupi_`'s spawn/`saveData_`'s load itself, at its
        // own point in the boot sequence) and `LoadMission()` below.
        void RebuildWorldPresentation();

        // Builds the terrain mesh again when the world's blocks changed since it was built
        // (`Worlds::World::blockRevision()`): doors opened in play, hub doors at load.
        void RefreshTerrainMeshIfChanged();
        // Whether the terrain draws the flight barrier now: the ShowSecret cheat, or the editor.
        [[nodiscard]] bool TerrainDrawSecret() const noexcept;

        // After a won mission: unlocks what it opens and loads where it leads (the hub, the main
        // hub after a golden key, 199 after the main hub's exit).
        void AdvanceAfterWin();

        // Loads `worlds3d/world{missionNumber:03d}.vwr`, rebuilds the
        // terrain/background presentation, resets `blupi_`/`interaction_`
        // to fresh per-level defaults (preserving lives -- the one real
        // state that survives a mission change, real `PlayPrepare()`
        // resets everything else: vehicle mode, secret powers, keys,
        // dynamite, treasure count), repositions Blupi at the spawn
        // convention every hand-authored world in this engine shares
        // (world (0,1,0)), and persists the new mission via `saveData_`.
        // Used by every real mission-transition trigger: world-select
        // portal contact, exit-reached (`WinLostReturn`), `PauseBack`,
        // `PauseRestart` (reloads the CURRENT mission fresh), Init's
        // `PLAY` (mission 1), and Resume's `CONTINUE` (the saved mission).
        // A no-op (logs and returns) if the target `.vwr` file doesn't
        // exist -- same graceful-failure shape as the initial `LoadContent()`
        // load.
        void LoadMission(int missionNumber, const std::string* validatedBytes = nullptr);
        // Spawn/respawn with the current terrain and object support checked before input.
        void PlaceBlupi(float x, float y, float z);

        // Loads a player-authored custom world (plan.md EDITOR-107) for
        // editing -- NOT a mission (no hub/door-gating scan, no
        // saveData_ mission-number persistence, unlike LoadMission()
        // above; these are sandbox worlds outside the real 78-mission
        // structure). Used by the browser's Open/New actions. A no-op
        // (logs and returns) if @p path fails to load, same graceful-
        // failure shape as LoadMission(). On success: rebuilds the
        // terrain/background presentation, starts the free-fly camera
        // above the world's own block centroid (or a fixed world-center
        // point for a brand-new all-air world), sets the editor's
        // world path to @p path, and calls worldEditor_.ExitBrowser().
        void LoadCustomWorldForEditing(const std::filesystem::path& path);

        // Loads @p path (already saved fresh by WorldEditor's Play-Test
        // button) for a real gameplay session (plan.md EDITOR-108) --
        // deliberately NOT a mission: no hub/door-gating scan, and
        // crucially no saveData_ persistence at all (a sandbox world has
        // no real mission number; writing one would corrupt real
        // progress). Resets interaction_/blupi_ to fresh defaults (no
        // "preserve lives" concept -- there's no real life total to
        // preserve for a test session) and spawns at the same fixed
        // (0,1,0) convention every hand-authored world shares. Sets
        // editorPlayTestActive_/editorPlayTestWorldPath_ so the existing
        // Win/Lost/PauseBack/PauseMenu transition sites (see their own
        // updated comments) route back to the editor instead of the real
        // hub when this is active.
        void LoadCustomWorldForPlayTest(const std::filesystem::path& path);
        // Galaxy Eggbert's title scene (2026-10-25, owner: an original title
        // screen instead of Mobile Eggbert's init.png): worlds3d/title.vwr, a
        // tile-built diorama with the real Blupi, drawn behind the main menu.
        // LoadTitleScene() is requested whenever the menu (Init) opens and
        // done at the next Update(); UpdateTitleScene() animates the world,
        // Blupi's idle clip and a slowly swaying camera. Without the file the
        // menu keeps the original's art.
        void LoadTitleScene();
        void UpdateTitleScene(float dt);

        // Real vehicle motor sound crossfade (plan.md SOUND-007/008, `BlupiController::
        // HasVehicleMotor()`/`IsVehicleMotorHigh()`, found 2026-07-17) -- ports
        // `Decor::AdaptMotorVehicleSound()` exactly: computes the desired loop channel (none,
        // or the mode's own high/low variant), and if it differs from `activeMotorLoop_`, plays
        // the one-shot start sound (silence->motor) or stop sound (motor->silence), stops the
        // old loop, and starts the new one. A no-op when the desired loop already matches the
        // active one (matches the real early-out exactly). Called once per frame.
        void UpdateVehicleMotorSound();
        void StopVehicleMotorSound();

        // INFRA-007 (plan.md §7, step 2/3): InteractionSystem's 11 simple/position-payload
        // *ThisFrame() signals are now one typed EventsThisFrame() queue instead of 11 parallel
        // booleans -- this finds the (at most one, per-Kind) event of a given Kind this frame, or
        // nullptr if none fired. Returns a pointer rather than bool so payload-carrying kinds
        // (PowerGranted/CloudGranted/HideGranted) can read their pickup position straight off the
        // returned Event, same call site, no separate position getter needed.
        [[nodiscard]] const GalaxyEggbert::Game::InteractionSystem::Event*
        FindInteractionEvent(GalaxyEggbert::Game::InteractionSystem::EventKind kind) const noexcept;

        // Sucette/Drink/Charge real 2-stage pickup delay (plan.md `173`) --
        // Starts when FindInteractionEvent() finds a PowerGranted/
        // CloudGranted/HideGranted event this frame (plays the real
        // immediate "grab" sound, starts `blupi_.TriggerPickupFreeze()`,
        // and stashes the event's own pickup position into
        // `reusablePickup_` since the freeze outlives this frame) OR
        // `blupi_.ConsumePickupFreezeResolved()` fires (plays the real
        // "complete" sound, grants Power/Hide via `blupi_.TriggerPower()`/
        // `TriggerHide()` -- Cloud's own buff already granted at contact,
        // matching real source -- and re-spawns the item at the stashed
        // position via `interaction_.RespawnPickupItem()`). Called right
        // after `interaction_.Update()` returns, alongside
        // `ResolveDeathLock()` above. Interruption returns the reusable item without granting a power.
        void ResolvePickupFreeze();

        // Dispatches cheat 1-9 (plan.md CHEAT-001..009) -- verified
        // directly against the real `Decor::CheatAction(Tables::
        // CheatCodes)`, correcting several wrong/imprecise draft
        // descriptions from an earlier, unverified plan.md pass (see
        // InteractionSystem.hpp's own per-cheat comments for exactly
        // what each one corrects). Cheat3 "ShowSecret" toggles m_bDrawSecret: the flight
        // barrier (214) is drawn (Decor.cpp:930). The original also stamps icon 214 over crates
        // (ObjectType12) whose icon is not 32-34 (Decor.cpp:896); Galaxy's crates are 3D models
        // without that icon field, so that half is not reproduced.
        void ToggleFullscreen();
        void ApplyCheat(int cheatNumber);
        // The original's typed cheats (InputPad::TypedCheat; `Decor::CheatAction` for each word,
        // OR-0007); the menu's nine go through ApplyCheat().
        void ApplyTypedCheat(GalaxyEggbert::Game::InputPad::TypedCheat cheat);
        // fairplay's lock (forRun false) or honest's (true, until the application quits): every cheat off.
        void TurnCheatsOff(bool forRun);
        // The persistent typed cheats now on, as Mobile Eggbert lists them top-left in Play.
        std::vector<std::string> activeCheats_;
        // The opendoors cheat (Decor's m_bCheatDoors): every hub door opens, at once and as each
        // hub loads (AdaptHubDoors()). Toggled by the cheat, kept for the session.
        bool cheatDoors_ = false;
        // Decor::AdaptDoors() for the hub loaded: slides up the doors unlocked (or all, with
        // cheatDoors_), sound 33, and shows the completed worlds' signs. Returns where Blupi
        // starts in a world hub: past the last door opened.
        std::optional<GalaxyEggbert::HubArrival> AdaptHubDoors(int missionNumber);
        // Enter (or a gamepad's A) held in the main menu last frame: Play fires on its press.
        bool initEnterWasDown_ = true;
        // Frames left in which the mouse is ignored after the last finger on the screen.
        int touchQuietFrames_ = 0;
        GalaxyEggbert::Def::GamePhase phase_ = GalaxyEggbert::Def::GamePhase::Wait;
        bool pauseKeyWasDown_ = false;
        GalaxyEggbert::Game::ControlsHelp controlsHelp_;
        bool menuConfirmWasDown_ = false;
        bool phaseReturnKeyWasDown_ = false;
        // Draw()'s own frame counter (NEXT.md §5, 2026-07-13) -- lets the
        // one-shot HUD screenshot diagnostic wait for a frame strictly
        // after the terrain diagnostic's mid-frame GetBackBufferData call,
        // avoiding the Vulkan-only same-frame corruption that call causes.
        int drawFrameIndex_ = 0;
        int terrainPixelPrintedFrame_ = -1;

        // INFRA-001 (plan.md §7) -- see EnableGoldenCaptureMode()'s own
        // comment for the full behavior. `goldenCaptureArmed_` latches the
        // one-time setup (phase skip + fixed world load); `goldenCaptureTick_`
        // is a plain per-Update() tick count (not phaseTimeSeconds_, which
        // resets on phase transitions). `goldenCaptureAwaitingDraw_` freezes
        // simulation at each requested tick until Draw() captures it, so a
        // skipped render cannot accidentally record a later simulation
        // state. `goldenCaptureNextIndex_` walks through
        // `kGoldenCaptureTicks` (GalaxyEggbertGame.cpp) in Draw().
        bool goldenCaptureMode_ = false;
        bool goldenCaptureArmed_ = false;
        bool goldenCaptureAwaitingDraw_ = false;
        int goldenCaptureTick_ = 0;
        int goldenCaptureNextIndex_ = 0;

        // Behavioral trace mode -- see EnableGoldenTraceMode()'s own
        // comment for the full behavior. Same latch/tick-count shape as
        // the golden-capture members above; goldenTraceLines_ accumulates
        // one line per tick, written out in one shot on completion.
        bool goldenTraceMode_ = false;
        bool goldenTraceArmed_ = false;
        // `--mission N` (EnableStartMission()); 0 means a normal start.
        int startMission_ = 0;
        bool startGalaxy_ = false;
        bool startSpawnSet_ = false;
        float startSpawnX_ = 0.0f, startSpawnY_ = 0.0f, startSpawnZ_ = 0.0f, startYawDegrees_ = 0.0f;
        bool goldenTraceWritten_ = false;
        // `--autoplay` (EnableAutoplay()): the script, the keys it holds this tick, its trace file.
        std::unique_ptr<GalaxyEggbert::Game::Autoplay> autoplay_;
        Microsoft::Xna::Framework::Input::KeyboardState autoplayKeys_;
        std::FILE* autoplayTrace_ = nullptr;
        int autoplayTick_ = 0;
        // The typed cheat `zoom` (OR-0079): the chase camera's step out, 0 (normal) to 4; the view is
        // 1x, 2x, 4x, 8x, 16x as far (Mobile Eggbert's 100, 50, 25, 12 %, and 6 % added at the owner's request). Reset when a level is loaded.
        int cheatZoomStep_ = 0;
        bool cheatCompass_ = false;
        bool cheatDebug_ = false;
        // The Setup's frame rate (SaveData::GetFrameRate): above 60 the loop draws that often while the
        // game still steps 60 times a second (Update's tick gate); the frames between two steps show the
        // camera and Blupi part way between them (drawAlpha_). Never in the captures, demos and
        // replays, whose ticks must be the loop's.
        void ApplyFrameRate();
        [[nodiscard]] bool FrameRateInterpolates() const noexcept;
        bool frameRateInterpolating_ = false;
        bool frameRateApplied_ = false;
        double tickClock_ = 0.0;      // seconds since the last step
        float drawAlpha_ = 1.0f;      // how far between the last two steps the frame is drawn
        Microsoft::Xna::Framework::Vector3 previousEye_, previousTarget_, previousBlupi_;
        // The steps taken while drawing between them; GALAXY_INTERP_TRACE=<file> writes each frame drawn
        // between two (the step, how far, the camera drawn with its shake, a moving object drawn part way).
        std::uint64_t interpolatedTicks_ = 0;
        Microsoft::Xna::Framework::Vector3 blupiDrawOffset_; // the interpolated Blupi's offset, while drawing
        // Every moving object where the last step began (type, active, place), to draw it part way too.
        struct ObjectPlace
        {
            GalaxyEggbert::Def::ObjectType type{};
            bool active = false;
            float x = 0.0f, y = 0.0f, z = 0.0f;
        };
        std::vector<ObjectPlace> previousObjects_;
        float particleLag_ = 0.0f; // while drawing between two steps: the part of a step not yet shown
        // The lightning's zaps (PlayBlitzZaps): the world's animation tick last heard.
        int blitzZapPhase_ = -1;
        void PlayBlitzZaps();
        // The "fps" cheat: the frames drawn a second (measured over half a second at a time), shown
        // as its entry in the cheats' list ("fps 60").
        bool cheatFps_ = false;
        int fpsFrames_ = 0;
        std::chrono::steady_clock::time_point fpsSince_{};
        float fpsShown_ = 0.0f;
        [[nodiscard]] std::vector<std::string> ActiveCheatNames() const;
        // The "position" cheat's axes: X, Y, Z as arrows turning with the camera, under the cheats' list.
        CnaEngine::Graphics::DebugDraw positionAxes_;
        void DrawPositionAxes(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW, int viewportH,
                              float left, float top);
        std::array<float, 3> debugVelocity_{};
        // The autoplay's `switches` / `saws` conditions: the switches pressed in this mission and the
        // saws 378 turning now (-1: not counted yet; counted again after each press).
        int autoplaySwitchPresses_ = 0;
        int autoplaySawsTurning_ = -1;
        int autoplaySwitchMission_ = -1;
        int autoplayExitCode_ = 0;
        // AUTOPLAY_SHOTS=N (EnableAutoplay()): a frame saved every N ticks of the run, for looking at
        // a whole playthrough as the player sees it; pending until the next drawn frame.
        int autoplayShotEvery_ = 0;
        bool autoplayShotPending_ = false;
        // Live keyboard and mouse are ignored by the capture modes and the autoplay.
        [[nodiscard]] bool SuppressLiveInput() const noexcept
        {
            return goldenCaptureMode_ || goldenTraceMode_ || autoplay_ != nullptr;
        }
        [[nodiscard]] Microsoft::Xna::Framework::Input::KeyboardState CurrentKeyboardState() const;
        void StepAutoplay();
        void UpdateTick();
        // UpdateTick's parts (GalaxyEggbertGameUpdate.cpp): every phase's screen input, then the world in
        // play. Each true when the tick ends there.
        bool UpdateTickScreens(float dt, GalaxyEggbert::Game::InputPad::PlayInput& padPlayInput, bool& inputPadClaimedMouse);
        bool UpdateTickWorld(float dt, float simDt, const GalaxyEggbert::Game::InputPad::PlayInput& padPlayInput, bool inputPadClaimedMouse);
        bool UpdateTickCamera(float dt, float simDt, const Microsoft::Xna::Framework::Input::KeyboardState& keys,
                              bool inputPadClaimedMouse, bool crouchHeld, bool lookUpHeld, float moveInput, float turnInput);
        bool UpdateTickEvents(int gameOverCountBeforeUpdate, bool wasBallooned);
        bool UpdateScreenPlay(const Microsoft::Xna::Framework::Input::KeyboardState& phaseKeys,
                              const Microsoft::Xna::Framework::Graphics::Viewport& viewport,
                              GalaxyEggbert::Game::InputPad::PlayInput& padPlayInput, bool& inputPadClaimedMouse,
                              bool& mousePausePressed);
        bool UpdateTickActions(bool actionPressedEdge, bool& actionUsed, bool& jumpPressed, float stepStartFeetY,
                               bool tempPassable, bool wasNage, bool wasSurf);
        void CaptureInput();
        [[nodiscard]] Microsoft::Xna::Framework::Matrix BlupiModelWorld(const GalaxyEggbert::Game::CharredShape& charred) const;
        // The body's lean, render only (Decor::BlupiSearchIcon, m_blupiRealRotation; OR-0080): the
        // helicopter leans into its horizontal speed, a swimmer turns to his direction and bobs, a
        // surfer bobs. Called after each Step.
        void UpdateBodyTilt(float dt);
        float bodyPitchRadians_ = 0.0f;     // nose down, about the model's middle
        float bodySwimLogicDegrees_ = 15.0f; // the original's m_blupiLogicRotation
        float bodyTiltTicks_ = 0.0f;        // the original's m_time, 20 a second
        float bodyLastX_ = 0.0f, bodyLastY_ = 0.0f, bodyLastZ_ = 0.0f;
        bool bodyHasLast_ = false;
        Microsoft::Xna::Framework::Graphics::Texture2D* BlupiFaceTexture();
        void RecordHideGhost();
        [[nodiscard]] Microsoft::Xna::Framework::Input::MouseState CurrentMouseState() const;
        void ResetMissionInput();
        void StartResumeLog();
        void ReadResumeLog();
        void DeleteResumeFile() const noexcept;
        bool BeginResume(GalaxyEggbert::Game::ResumeLog log);
        void ReplayBatch();
        void CheckResumeSelftest();
        [[nodiscard]] std::map<std::string, std::string> ResumeSignature() const;
        GalaxyEggbert::Game::ResumeLog resumeLog_;
        std::string loadedMissionPath_;
        std::uint64_t loadedMissionHash_ = 0;
        std::optional<GalaxyEggbert::Game::ResumeLog> pendingResume_;
        GalaxyEggbert::Game::ResumeInput tickInput_;
        std::deque<char> pendingCheatLetters_;
        std::uint64_t keyInputToken_ = 0;
        GalaxyEggbert::Editor::EditorCommandKeys editorCommandKeys_;
        bool missionStartPending_ = false;
        bool recordingResume_ = false;
        bool replayingResume_ = false;
        // "honest" typed (not by a demo): every cheat stays off until the application quits.
        bool noCheatsForRun_ = false;
        std::uint32_t missionSeed_ = 0;
        std::size_t replayRun_ = 0;
        std::uint32_t replayRepeat_ = 0;
        std::uint64_t resumeTicks_ = 0;
        bool resumeSelftestDone_ = false;

        // Main-menu demos (DemoReplay.hpp; Speedy Blupi's CEvent::Demo*): a recording replayed tick by
        // tick into the normal simulation (UpdateTick()) at the normal pace, drawn and heard as play
        // is. A mode of its own, apart from the resume's fast, silent, undrawn reconstruction
        // (replayingResume_) and from the QA autoplay. While it plays, the player's data is frozen:
        // SavePlayerData() and DeleteResumeFile() do nothing, OnDeactivated() writes nothing, and
        // StopDemo() puts back the profile, settings and cheats StartDemo() set aside. Loading
        // another mission or leaving Play ends the demo instead (LoadMission(), SetPhase()).
        struct DemoSavedState
        {
            GalaxyEggbert::Game::SaveData saveData;
            int cameraMode = 1;
            GalaxyEggbert::Def::GameSpeed gameSpeed = GalaxyEggbert::Def::GameSpeed::Normal;
            bool quickCheat = false, openDoors = false, showSecret = false, compass = false, debug = false;
            bool cheatsLocked = false, cheatsLockedForRun = false;
            int lives = 3;
            std::vector<std::string> activeCheats;
            int galaxyGamer = -1;
        };
        bool StartDemo(int number);
        bool StartDemoFile(const std::string& path, int number);
        // The world file a demo plays in: the campaign's, or a galaxy's (BD-0119) -- the recording
        // player's, else (a demo shared with its galaxy, maybe from another platform) any player's
        // galaxy whose world has the recording's hash; @p galaxy is that player, -1 the campaign.
        // A galaxy found under another path becomes the demo's own (demo.log.start.worldPath).
        [[nodiscard]] std::string ResolveDemoWorld(GalaxyEggbert::Game::DemoRecording& demo, int& galaxy) const;
        // The demos after demoNumber_ in turn (DemoOrderAfter()); false when none can play.
        bool StartNextDemo();
        // The demo's end: the next demo, or the menu.
        void EndOfDemo();
        // Back to the main menu with the player's state, from any demo moment.
        void StopDemo();
        void UpdateDemo();
        // One recorded tick; false when the demo ended (EndOfDemo() or StopDemo() ran).
        bool DemoTick();
        // The menu's idle timer and the input latch after a demo; true when a demo started.
        bool UpdateDemoMenu();
        [[nodiscard]] int CountDemos() const;
        [[nodiscard]] GalaxyEggbert::Game::DemoLiveInput ReadDemoLiveInput(bool fromTickInput);
        // SaveData::Save() unless a resume reconstruction or a demo is playing.
        void SavePlayerData();
        void BeginDemoRecording();
        bool FinishDemoRecording(); // true when the file was written
        bool demoActive_ = false;
        bool demoSessionLive_ = false; // its start is set up: leaving the mission or Play now ends it
        bool demoEndRequested_ = false;
        bool demoStopRequested_ = false; // the window was deactivated during the demo
        bool menuInputLatched_ = false;  // after a demo: menu input ignored until nothing is held
        GalaxyEggbert::Game::DemoRecording demo_;
        DemoSavedState demoSaved_;
        int demoNumber_ = 0;
        std::size_t demoRun_ = 0;
        std::uint32_t demoRepeat_ = 0;
        std::uint64_t demoTicks_ = 0;
        GalaxyEggbert::Game::DemoIdleTimer demoIdleTimer_;
        GalaxyEggbert::Game::DemoLiveInput demoLastLive_;
        std::vector<int> demoKeyEvents_; // native key presses since the last DemoLiveInput
        std::chrono::steady_clock::time_point demoClock_{};
        float demoClockSeconds_ = 0.0f; // real seconds since the previous Update(), at most 0.25
        bool demoClockStarted_ = false;
        std::string demoRecordPath_, demoRecordTitle_;
        int demoRecordMaxTicks_ = 0;
        bool demoRecordArmed_ = false, demoRecording_ = false;
        GalaxyEggbert::Game::ResumeLog demoRecordLog_;
        std::string demoVerifyPath_;
        bool demoVerifyPending_ = false, demoVerifying_ = false;

        // The player's own demo recordings (BD-0116): Pause's Record button shows the ten demo
        // positions (demoSlotsShown_); the one chosen records the mission again from its start
        // (demoRecordSlot_, into UserDemoPath()), as the developer's --record-demo does, but the
        // game goes on afterwards. A mission change that ends the recording is held back until
        // the recording is written (deferredMission_), so that the recording ends as its replay
        // will. demoNotice_ says what happened, for a few seconds.
        bool demoSlotsShown_ = false;
        std::vector<GalaxyEggbert::Game::InputPad::DemoSlotInfo> demoSlotInfos_; // read as they open
        // Pause shows Record: a mission of the campaign or a galaxy being played, nothing recorded or
        // replayed, no play-test.
        [[nodiscard]] bool CanRecordDemo() const noexcept
        {
            return !editorPlayTestActive_ && !demoActive_ && !replayingResume_ && !demoRecording_ &&
                   !demoRecordArmed_ && demoRecordPath_.empty() && !loadedMissionPath_.empty() &&
                   worldRuntime_.GetMissionNumber() > 0 && !autoplay_;
        }
        int demoRecordSlot_ = 0; // 1-10 while the player records; 0 the developer's recording
        int deferredMission_ = 0;
        std::string demoSlotsMessage_;
        std::string demoNotice_;
        float demoNoticeSeconds_ = 0.0f;
        void StartPlayerRecording(int slot);
        // After FinishDemoRecording() of the player's recording: the notice, the held-back mission.
        void EndPlayerRecording(bool written);
        // Which file demo position @p slot plays: the player's recording if it is there.
        [[nodiscard]] std::string DemoSlotPath(int slot) const;
        [[nodiscard]] std::vector<GalaxyEggbert::Game::InputPad::DemoSlotInfo> DemoSlotInfos() const;
        void ShowDemoNotice(std::string text);
        // Sharing recordings (2026-11-09): the positions' Export and Import change what a click on
        // a position does: 0 records, 1 exports its demo, 2 imports demoImports_[demoImportIndex_];
        // 3 shows Import's list of the shared .gdemo files (newest first, demoImportRows_ its lines)
        // in place of the positions, a row chosen goes on to 2.
        int demoSlotsMode_ = 0;
        std::vector<std::string> demoImports_;
        std::vector<std::string> demoImportRows_;
        std::size_t demoImportIndex_ = 0;
        int demoImportPage_ = 0;     // InputPad::kDemoListRows files a page
        int demoDeleteArmed_ = -1;   // the file (index in demoImports_) whose bin was clicked once
        void ListDemoImports();
        // The demo positions' screen (over Pause, or from the menu's Demos button: no recording there).
        void UpdateDemoSlotsScreen(const Microsoft::Xna::Framework::Input::MouseState& mouse, int viewportW,
                                   int viewportH);
        void DrawDemoSlotsScreen(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW,
                                 int viewportH);
        [[nodiscard]] int DemoImportPages() const noexcept;
        void HandleDemoSlotClick(int slot);
        void ToggleDemoImport();
        [[nodiscard]] std::string DemoImportPrompt() const;
        // The goodbye on quitting from the menu (2026-11-10, as Speedy Blupi's WM_PHASE_BYE): Exit and
        // Escape show it over the title scene; a new press of anything closes the game.
        bool goodbyeShown_ = false;
        float goodbyeSeconds_ = 0.0f;
        GalaxyEggbert::Game::DemoLiveInput goodbyeInputWas_;
        void StartGoodbye();

        // The network game (2026-11-10, the owner: Speedy Blupi 2's multiplayer;
        // GalaxyEggbertGameNet.cpp, plans/plan_gemultiplayer.md): the menu's Multiplayer opens its
        // screens over the menu (the games on the network, the lobby); a match is played in Play
        // with NetMatchActive(). net_ exists once Multiplayer was opened.
        std::unique_ptr<NetGame> net_;
        struct NetScreenLayout;
        [[nodiscard]] bool NetMatchActive() const noexcept;
        [[nodiscard]] bool NetScreenShown() const noexcept;
        [[nodiscard]] bool NetRemotePaused() const noexcept;
        int NetOthersPresent() const noexcept; // the other players still in the match (autoplay's netplayers)
        [[nodiscard]] static bool NetIsMovingLift(const GalaxyEggbert::Game::MobileObjSpec& obj) noexcept;
        void NetLogWorldHash() const; // autoplay's `hash`: this machine's world, to compare with the others'
        [[nodiscard]] bool NetWantsText() const noexcept;
        [[nodiscard]] int NetTeam() const noexcept; // his team's colour in a match, 0 otherwise
        void OpenMultiplayer();
        void CloseMultiplayer();
        void UpdateNet(); // every update: the session, what arrived, who came and went
        void NetHandle(std::uint8_t sender, const GalaxyEggbert::Net::Message& message);
        void NetPlayerLeft(std::uint8_t id);
        void NetSessionEnded(const std::string& reason);
        void NetBroadcastLobby();
        void NetNotice(const std::string& text);
        [[nodiscard]] std::vector<std::string> NetNotices() const;
        [[nodiscard]] NetScreenLayout BuildNetScreen(int viewportW, int viewportH) const;
        [[nodiscard]] std::vector<GalaxyEggbert::Game::QuadBatch::Rect> NetFocusTargets(int viewportW,
                                                                                      int viewportH) const;
        void UpdateNetScreen(const Microsoft::Xna::Framework::Input::MouseState& mouse, int viewportW, int viewportH,
                             bool escapePressed);
        void DrawNetScreen(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW, int viewportH);
        void NetStartTyping(int viewportW, int viewportH, bool address = false);
        // The host's address searched beyond the local network, kept in the user folder.
        void NetSetAddress(const std::string& address, bool save);
        void NetFinishTyping(bool send);
        void NetStartMatchTyping();     // Enter in a match: a chat line typed over the play (Q-022)
        void NetSay(const std::string& text); // that line typed and sent (autoplay's `say`)
        // Pause's "Message" in a match (touch screens: no Enter): the line on the screen's keyboard.
        void NetStartPauseTyping(int viewportW, int viewportH);
        void NetUpdateMatchKeyboard(const Microsoft::Xna::Framework::Input::MouseState& mouse, int viewportW, int viewportH);
        void DrawNetMatchKeyboard(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW, int viewportH);
        // The spectator (Q-022): on the Lost screen, watching another player still in the match.
        [[nodiscard]] bool NetSpectating() const noexcept;
        void NetWatch(int step);        // start watching, or the next (+1) / previous (-1) player
        void NetStopWatching();
        void NetSpectatorStep(float dt); // the world goes on under the watched one; the camera follows
        void NetSendLiftSyncs();        // the host's lifts that began a stop this step (NetAfterStep)
        // A world of the host's editor for the match (Q-022): chosen (index past the multiplayer
        // worlds), sent in pieces, received.
        void NetChooseWorld(int index);
        // A dropped player back in the match (Q-022): the host takes him at his place, sends him the
        // match and the world's changes; everyone has him back.
        void NetTryRejoin(std::uint8_t gamerId, const GalaxyEggbert::Net::Message& hello);
        void NetPeerBack(int slot, std::uint8_t gamerId);
        void NetMarkLeft(int slot);
        // Internet play (Q-022): the gamer services server typed and kept (used from the next start of
        // the multiplayer), and the host's address and it saved in the user folder.
        void NetSetServer(const std::string& typed);
        void NetSaveAddresses() const;
        void NetSendCustomWorld();
        void NetReceiveWorldData(const GalaxyEggbert::Net::Message& m);
        [[nodiscard]] bool NetHasLobbyWorld() const noexcept; // the lobby's world here (an editor's: arrived whole)
        bool netWatchLeftWas_ = false, netWatchRightWas_ = false, netWatchTouchWas_ = false;
        [[nodiscard]] bool NetTypingInMatch() const noexcept;
        void NetNoticeKill(std::uint8_t killer, int victimSlot);
        void NetTypedKey(Microsoft::Xna::Framework::Input::Keys key, bool shift);
        void NetLeaveLobby();
        bool NetHostGame();
        bool NetJoinGame(int index);
        void NetSetReady(bool ready);
        void NetStartAsHost();
        void NetTestStep(); // GALAXY_NET_TEST=host|join: the screens without a pointer
        void StartNetMatch(int world);
        // Around the step: the others' changes applied and a snapshot taken; then ours sent.
        void NetBeforeStep();
        void NetAfterStep();
        void NetAgePeers(float dt);
        void NetSendPose();
        void NetCheckWin();
        void NetDeclareWin();          // this player wins (the last survivor, or first when time is up)
        [[nodiscard]] bool NetHostHoldsClock() const noexcept; // the host out, the others' time still running
        void NetTimeUp();              // the host: the time limit reached, the ranks sent
        void NetApplyTimeUp(const std::array<std::uint8_t, GalaxyEggbert::Net::kMaxPlayers>& places);
        void NetSendDie(int livesLeft);
        void NetSendLost();
        void LeaveNetMatch();
        void NetPhaseChanged(GalaxyEggbert::Def::GamePhase previous, GalaxyEggbert::Def::GamePhase next);
        // Another player's aura (his shield, power or cloud, the original's m_netBlupiSec,
        // decblock.cpp:1696-1715): where his sprite's centre is and his rank (the cloud's sparks' offset).
        struct NetAura
        {
            Microsoft::Xna::Framework::Vector3 centre;
            GalaxyEggbert::Def::SecretPower power = GalaxyEggbert::Def::SecretPower::None;
            int rank = 0;
        };
        [[nodiscard]] std::vector<NetAura> NetPeerAuras() const;
        // Between a peer's last two poses (0 the older, 1 the newer), as his Blupi is drawn.
        [[nodiscard]] static float NetPoseBlend(const NetGame& n, std::size_t peer);
        // His Blupi met by another's cloud (decblupi.cpp:3838-3893): electrocuted, the cloud gone.
        void NetCheckCloudTouch();
        // The other players' names over their heads (GalaxyEggbertGameDraw.cpp).
        void DrawNetNames(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW, int viewportH);
        [[nodiscard]] int NetSlotOf(std::uint8_t gamerId) const noexcept; // a gamer's lobby place, -1
        // The match's results (Q-021): a death (and its killer's place), a player out; the lines
        // shown on the win and lost screens and in the lobby; back to the lobby after a match.
        void NetScoreDeath(int slot, std::uint8_t killer);
        void NetScoreOut(int slot);
        [[nodiscard]] std::vector<std::string> NetResultLines() const;
        void NetLogResults() const;
        [[nodiscard]] int NetPlaceOf(int slot) const;
        void NetReturnToLobby();
        // The other players' Blupis, in their colours (GalaxyEggbertGameScene.cpp).
        void AppendSceneNetPlayers(std::vector<GalaxyEggbert::Game::SceneModel>& opaque,
                                   std::vector<GalaxyEggbert::Game::SceneModel>& translucent);
        // A copy of @p source in a team's colour (the editor's starts' RecolorPlayerStartPixel), cached;
        // team 0 (yellow) is the texture itself.
        Microsoft::Xna::Framework::Graphics::Texture2D* TeamTexture(int team,
                                                                    Microsoft::Xna::Framework::Graphics::Texture2D* source);
        // The real model's face for a clip at a time (BlupiFaceTexture() for his own).
        Microsoft::Xna::Framework::Graphics::Texture2D* BlupiFaceFor(const std::string& clip, double seconds);
        // Naming a recording (2026-11-10): Name (mode 4), then a recorded position; the keyboard on
        // the screen (or the real one) types it; OK, Enter or a click off the keyboard keeps it
        // (RenameDemo()), Cancel or Escape leaves it.
        bool demoNaming_ = false;
        int demoNameSlot_ = 0;
        std::string demoNameDraft_;
        GalaxyEggbert::Game::ScreenKeyboard demoKeyboard_;
        // The Setup's Nickname (a Galaxy addition, the owner, 2026-11-11): the selected player's
        // nickname typed on a panel over the main Setup, on the real keyboard or the one on the
        // screen; OK or Enter keeps it (saved at once), Cancel or Escape leaves it as it was.
        bool nicknameEditing_ = false;
        std::string nicknameDraft_;
        GalaxyEggbert::Game::ScreenKeyboard nicknameKeyboard_;
        void StartNicknameEditing(int viewportW, int viewportH);
        void UpdateNicknameEditing(const Microsoft::Xna::Framework::Input::MouseState& mouse, bool escapePressed);
        void FinishNicknameEditing(bool keep);
        void DrawNicknameEditing(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW,
                                 int viewportH);
        void StartDemoNaming(int slot);
        void FinishDemoNaming(bool keep);
        // Leaving the screen a step: the naming, then Export/Import/Name, then the positions.
        void CancelDemoSlotsStep();
        // The demo positions' buttons for the arrows, as the screen shows them now.
        [[nodiscard]] std::vector<GalaxyEggbert::Game::QuadBatch::Rect> DemoSlotsFocusTargets(int viewportW,
                                                                                            int viewportH) const;
        // The arrows and A on the demo positions, the galaxy screen and the world screen (2026-11-10,
        // FocusNavigator.hpp): what they chose (drawn lit), and the keys as they were last update.
        GalaxyEggbert::Game::FocusNavigator focusNavigator_;
        std::optional<GalaxyEggbert::Game::QuadBatch::Rect> focusHighlight_;
        unsigned focusKeysWas_ = 0;
        int focusMouseX_ = -1, focusMouseY_ = -1;
        bool focusPadBWas_ = false;

        // The web's xnjttjpo edits undone (2026-11-09): Pause shows "Original" while the campaign
        // world played has an edit kept (CustomWorldStorage HasCampaignEdit(), only ever on the web);
        // its first click arms it ("Sure?"), the second puts the shipped world back and restarts.
        bool revertArmed_ = false;
        // Keys typed into the galaxy's name (the editor's galaxy screen), with Shift, as they came.
        std::vector<std::pair<Microsoft::Xna::Framework::Input::Keys, bool>> browserTypedKeys_;
        // A Shift press seen as an event: the next key typed is a capital (the frame's snapshot can
        // miss a Shift pressed and let go between two updates).
        bool typedShiftPending_ = false;
        // The players' galaxies' names for the menu's panel, read again on each return to the menu.
        std::array<std::string, 3> galaxyNames_{};
        bool galaxyNamesStale_ = true;
        [[nodiscard]] bool CanRevertWorld() const;
        void RevertWorld();

        // The xnjttjpo cheat (BD-0118, Speedy Blupi's XMISSION): the 3D editor on the world being
        // played, its Stop back to that mission from its start. editorMission_ is the mission
        // (0: an ordinary editor visit), with the lives it had.
        bool xmissionPending_ = false;
        int editorMission_ = 0;
        int editorMissionLives_ = 3;
        void OpenMissionInEditor();
        void LeaveMissionEditor();

        // The player's own galaxy (BD-0119, CustomWorldStorage GalaxyDir()): played with the
        // campaign's rules from its own folder, the gamer's galaxy progress in SaveData. -1: the
        // campaign. No resume file is written for it.
        int galaxyGamer_ = -1;
        // The file of @p mission: the galaxy's while one is played, the shipped one otherwise.
        [[nodiscard]] std::string MissionPath(int mission) const;
        // The menu's Galaxy button: its main hub (or where its player left it), or the editor's
        // galaxy screen while it has no main hub.
        void StartGalaxy();
        void LeaveGalaxy();
        bool resumeAccelWarmStart_ = false;
        std::map<std::string, std::string> resumeSelftestSignature_;
        int goldenTraceTick_ = 0;
        std::string goldenTraceLines_;

        // Real `fadeOutPhase` (plan.md MENU-088/089) -- see SetPhase()'s
        // own comment above for the full real deferred-transition
        // mechanic this drives. `None` means "not currently exiting"
        // (covers both Play and every other phase's own settled/idle
        // state); any other value is the real pending destination during
        // an active ~1.0s exit fade.
        GalaxyEggbert::Def::GamePhase fadeOutPhase_ = GalaxyEggbert::Def::GamePhase::None;

        // Real `phaseTime` (2026-07-13, plan.md MENU-046..057), verified
        // directly against `Game1.hpp`'s own doc comment ("phaseTime==0"
        // is a `SetPhase()` postcondition) and `Game1.cpp:237`'s
        // unconditional `phaseTime++` every Update() tick regardless of
        // phase. Ported as elapsed SECONDS since the last `SetPhase()`
        // call (not a raw frame counter) since the real formulas this
        // drives are all expressed as `phaseTime / Config::ScaleTime(N)`
        // at a real 20fps base rate -- dividing by N/20 seconds is exactly
        // equivalent and framerate-independent, unlike counting ticks.
        // Currently drives only the Win/Lost `blupiyoupie.png` animation
        // (`InputPad::DrawWinLost`); not used for the Play-phase
        // gameplay simulation, which has no timer concept of its own.
        float phaseTimeSeconds_ = 0.0f;

        // Real mobile-eggbert bottom HUD + the interim animation-state
        // indicator (2026-07-10, see Hud.hpp) -- replaces the earlier
        // SpriteBatch-based HUD entirely: on CNA's Vulkan backend every
        // SpriteBatch batch is recorded BEFORE every 3D draw within the
        // frame, so a sprite HUD gets painted over by the 3D scene (the
        // "icon visible for a second, then gone" live report); Hud draws
        // real 3D quads instead, recorded in genuine submission order on
        // both backends.
        GalaxyEggbert::Game::Hud hud_;

        // Real mobile-eggbert on-screen touch controls (2026-07-13,
        // plan.md MENU-021..027/028..039, see InputPad.hpp) -- mouse-
        // driven Pause row (real background/character art + 5 real
        // pad.png buttons, Continue/Restart functionally wired) and Play
        // on-screen D-pad/Jump/Action/Pause controls, OR'd with keyboard
        // input in the movement block below. UpdatePlay()'s/UpdatePause()'s
        // own return value tells this class whether the current mouse
        // press landed on a control, so the pre-existing mouse drag-look
        // camera code (below) can skip its own handling and avoid the two
        // features fighting over the same left-mouse-button input.
        GalaxyEggbert::Game::InputPad inputPad_;
        GalaxyEggbert::Editor::EditorDeviceControls editorDevices_;

        // Minimal settings persistence (2026-07-13, plan.md MENU-067, see
        // SaveData.hpp for why this is NOT byte-compatible with the
        // real mobile-eggbert GameData). Loaded once in LoadContent();
        // Save() is called right after the SetupSounds toggle, matching
        // the real source's own "write immediately on toggle press"
        // behavior.
        GalaxyEggbert::Game::SaveData saveData_;

        // In-game 3D world editor (plan.md section 6, EDITOR-1xx tasks) --
        // see WorldEditor's own class comment. Entered via GalaxyEggbert::Def::GamePhase::
        // Editor, from the Init screen's own Editor button.
        GalaxyEggbert::Editor::WorldEditor worldEditor_;

        // Play-test session state (plan.md EDITOR-108) -- true while
        // GalaxyEggbert::Def::GamePhase::Play is running a custom world launched from the
        // editor's Play-Test button rather than a real mission. Checked at
        // every real mission-transition site (Win/Lost return, PauseBack,
        // PauseMenu) to route back to the editor (LoadCustomWorldForEditing,
        // see its own comment) instead of the real hub, and to skip the
        // real saveData_ checkpoint (a sandbox world has no real mission
        // number to persist).
        bool editorPlayTestActive_ = false;
        bool titleSceneRequested_ = false;
        // worlds3d/title.vwr failed to load: the loading screen (OR-0009) and the editor's
        // world list (OR-0012) stop asking for the title scene and draw their plain screens.
        bool titleSceneUnavailable_ = false;
        bool titleSceneActive_ = false;
        float titleSceneTime_ = 0.0f;
        std::filesystem::path editorPlayTestWorldPath_;

        // Hidden cheat menu (plan.md CHEAT-001..009, 2026-07-13) -- real
        // gesture zones are checked only during real Phase::Play
        // (confirmed via research); once the real 10-tap sequence
        // completes, the overlay renders layered on top of the normal
        // Play view (no phase change, no background swap) until a cheat
        // button is pressed. See `InputPad::UpdateCheatGesture()`'s own
        // class comment for the full real-behavior citation.
        bool cheatMenuShown_ = false;

        // Real GalaxyEggbert::Def::GameSpeed (SCORE-009/010/011, added 2026-07-20, see
        // include/GalaxyEggbert/Def/GameSpeed.hpp and InputPad.cpp:582-684
        // for the full real key-mapping citations) -- F5/F6 always work,
        // F7/F8 need quickCheatEnabled_ (real "quick" typed cheat, see
        // InputPad::UpdateTypedGhostCheat()'s own TypedCheatResult), Tab
        // toggles Slow<->Normal, F12 gives a second way to open/close the
        // existing cheat-button overlay (alongside the already-implemented
        // 10-tap gesture) -- all real, all `#ifdef MODERN`-gated in real
        // source, which is mobile-eggbert's own default/active build mode
        // (not some unused legacy variant), so in scope here. Real source
        // ALSO has a Shift-hold temporary boost -- deliberately NOT ported:
        // both LeftShift/RightShift are already this engine's own crouch/
        // look-up controls (a pre-existing, more central design decision),
        // so there is no free Shift key for it without a real conflict.
        // Applied as a continuous dt-scale factor (0.5x/1x/2x/4x/8x) on
        // the core simulation calls only, NOT a literal discrete
        // N-ticks-per-frame repeat of the whole Update() block (deliberate
        // simplification, agreed with the user 2026-07-20 -- repeating the
        // whole ~1000-line Play-phase block would risk breaking a lot of
        // already-tuned once-per-frame sound/camera/HUD logic that assumes
        // "runs exactly once").
        GalaxyEggbert::Def::GameSpeed gameSpeed_ = GalaxyEggbert::Def::GameSpeed::Normal;
        bool quickCheatEnabled_ = false;
        bool f5KeyWasDown_ = false;
        bool f6KeyWasDown_ = false;
        bool f7KeyWasDown_ = false;
        bool f8KeyWasDown_ = false;
        bool f12KeyWasDown_ = false;
        bool tabKeyWasDown_ = false;

        // Mouse drag-look (2026-07-10, user request): while the left
        // button is held, mouse deltas rotate the camera around Blupi
        // (yaw + pitch offsets on top of his facing); as soon as Blupi
        // gets movement input, the offsets decay smoothly back to zero so
        // the camera returns behind him. Works for both camera modes and
        // maps to touch-drag as well (SDL reports touch as mouse).
        float lookYawOffset_ = 0.0f;
        float lookPitchOffset_ = 0.0f;
        int lastMouseX_ = 0;
        int lastMouseY_ = 0;
        bool mouseLookActive_ = false;

        // F11 fullscreen toggle (2026-07-10, user request), edge-detected
        // like the other key toggles. GraphicsDeviceManager is constructed
        // in the game constructor (standard XNA pattern) purely for
        // ToggleFullScreen() -- nothing else uses it yet.
        std::unique_ptr<Microsoft::Xna::Framework::GraphicsDeviceManager> graphics_;
        bool fullscreenKeyWasDown_ = false;

        // Billboard rendering for worldRuntime_'s parsed MoveObjects
        // (15-3d-render-mapping-design.md §5/§7) — element.png, static phase
        // (no animation yet). Rebuilt every frame since billboard vertex
        // positions depend on the camera (MeshBuilder::AddBillboard faces
        // the camera's right/up) — the WorldRenderer rebuilds its mesh every frame,
        // not lazily cached like terrainRenderer_.
        Microsoft::Xna::Framework::Graphics::Texture2D objectTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> objectEffect_;

        // Billboard rendering for the confirmed object-m.png-sourced
        // MoveObjects (ObjectDefinitionRegistry, NEXT.md §3,
        // 2026-07-09) -- same camera-facing billboard technique as
        // the element.png batch above, but reuses terrainTexture_
        // (object-m.png) via its own dedicated effect (BasicEffect only
        // binds one texture at a time), same reason as bigDecorEffect_.
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> objectMPngEffect_;

        // Billboard rendering for the confirmed explo.png-sourced
        // MoveObjects (ObjectDefinitionRegistry, NEXT.md §3,
        // 2026-07-09) -- explosions/visual effects, a genuinely new texture
        // (not previously loaded anywhere in GalaxyEggbertCNA), same
        // camera-facing billboard technique as the element.png batch above.
        Microsoft::Xna::Framework::Graphics::Texture2D exploTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> exploEffect_;

        // Billboard rendering for the confirmed Blupi-skin MoveObjects
        // (ObjectDefinitionRegistry, NEXT.md §3, 2026-07-09) --
        // ObjectType200 sources blupi.png, ObjectType201/202/203 source
        // blupi1.png -- two separate
        // textures/effects/renderers since BasicEffect only binds one
        // texture at a time. blupiObjectTexture_/blupiObjectEffect_ are
        // deliberately separate from the existing blupiIconTexture_/
        // blupiIconBatch_ above (that pair is 2D SpriteBatch-coupled for
        // the HUD indicator, not reusable for this 3D BasicEffect path),
        // even though both load the same blupi.png file.
        Microsoft::Xna::Framework::Graphics::Texture2D blupiObjectTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> blupiObjectEffect_;
        Microsoft::Xna::Framework::Graphics::Texture2D blupi1ObjectTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> blupi1ObjectEffect_;

        // Billboard rendering for worldRuntime_'s parsed BigDecor: cells
        // (NEXT.md §8 task 3, mobile-eggbert-reference/
        // 01-world-file-format.md §2.3 / 15-3d-render-mapping-design.md
        // §9.2 — confirmed non-colliding, Billboard render mode). Eggbert 2
        // draws this layer through CHEXPLO, so its ids sample explo.png,
        // not the identically numbered object-m.png terrain tiles. Only
        // ever non-empty when a world was loaded via
        // LoadFromMobileEggbertFile(); `.vwr` worlds now carry sparse 3D
        // BigDecorRecord entries too.
        // bigDecorCells_ is refreshed by RebuildWorldPresentation(), so
        // editor placement/removal appears immediately; only the
        // camera-facing billboard mesh is rebuilt per frame.
        struct BigDecorCell
        {
            float worldX;
            float worldY;
            float worldZ;
            std::uint16_t icon;
        };
        std::vector<BigDecorCell> bigDecorCells_;
        // The BigDecor models' shapes (WorldRuntime::SetBigDecorShapes), measured when they load.
        GalaxyEggbert::Game::BigDecorShapes bigDecorShapes_;
        bool graceWasInVehicle_ = false; // for the contact grace after leaving a vehicle
        // Tells the terrain which cells are drawn as models (the fan, BlockModels.hpp).
        void ApplyBlockModels();
        [[nodiscard]] std::vector<std::uint16_t> StillModelIcons() const;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> bigDecorEffect_;

        // Platform-lift/crate UniformCube object path (NEXT.md §8 task 3,
        // mobile-eggbert-reference/15-3d-render-mapping-design.md §5's two
        // confirmed "render as a cube, not a billboard" exceptions:
        // ObjectType1/47/48 platform lifts and ObjectType12 crates, see
        // ObjectDefinitionRegistry's SolidCube mode). Reuses terrainTexture_
        // (object-m.png, the confirmed-correct sheet for these types) via
        // its own effect, same reason as bigDecorEffect_ above. Rebuilt
        // every frame in Draw() (2026-07-09) using each MobileObjSpec's
        // real per-instance phase, same reason and pattern as the
        // billboard renderers below -- an uploaded GpuMesh has no in-place UV
        // update API, only construction from mesh data, so a fresh
        // mesh each frame is the only way for types 47/48's chenille icon
        // formulas to actually animate (cheap here, only a handful of cube
        // objects).
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> objectCubeEffect_;

        // Third-person-mode 3D model (2026-07-09, NEXT.md §3) -- CNA's
        // generic SkinnedModelEXT/SkinnedEffect path (../cna/docs/
        // skinned-model-ext.md), loaded via getContentProperty() with its
        // RootDirectory pointed at avatars3d/ (see LoadContent()).
        // Loads the real Blupi converted from the original Ray Dream scenes
        // (avatars3d/blupi/, tools/rd4_to_gltf/, 2026-10-23) when present,
        // else the temporary CC0/CC-BY placeholder fox
        // (avatars3d/blupi_placeholder/, see its own README.md).
        // blupiModelLoaded_ guards against avatars3d/ being absent (e.g. a
        // build that never ran the CMake copy step) -- degrades to staying
        // in first-person mode rather than crashing.
        std::shared_ptr<Microsoft::Xna::Framework::Graphics::SkinnedModelEXT> blupiModel_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::SkinnedEffect> blupiEffect_;
        bool blupiModelLoaded_ = false;
        bool blupiModelIsReal_ = false;

        // The real model's per-clip data (avatars3d/blupi/clips.txt, written
        // by tools/rd4_to_gltf/build_clip_library.py): one texture per
        // expression and, per clip, the expression of every frame at
        // BlupiController's kAnimFps, drawn on the part named blupiFacePart_;
        // the clips that turn him toward or away from the sprites' viewer,
        // which put the third-person camera on his right (chaseCamera_);
        // and the vehicles (blupiVehicles_).
        std::vector<Microsoft::Xna::Framework::Graphics::Texture2D> blupiFaces_;
        std::unordered_map<std::string, std::vector<int>> blupiClipFaces_;
        std::string blupiFacePart_;
        double blupiFaceRate_ = 8.0;
        std::unordered_set<std::string> blupiViewerClips_;
        // The vehicles he rides (clips.txt "vehicle" lines): separate models
        // with the vehicle clips' names, drawn with his transform and time.
        std::vector<std::pair<std::string, std::shared_ptr<Microsoft::Xna::Framework::Graphics::SkinnedModelEXT>>>
            blupiVehicles_;
        // The enemy Blupis (EnemyBlupi.hpp, OR-0063): a copy of every texture of his model, his
        // faces, his helicopter and his tank, the hue turned as blupi1.png's, keyed by the
        // original; built with the real model (BuildEnemyBlupiTextures), empty without it, when
        // they stay blupi1.png billboards.
        std::unordered_map<const Microsoft::Xna::Framework::Graphics::Texture2D*,
                           std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D>>
            enemyBlupiTextures_;
        // Editor-only variants of yellow body/face materials, keyed by the original texture.
        std::array<std::unordered_map<const Microsoft::Xna::Framework::Graphics::Texture2D*,
                                     std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D>>,3>
            editorStartTextures_;
        void BuildEnemyBlupiTextures(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        [[nodiscard]] bool EnemyBlupiModelDrawn(const GalaxyEggbert::Game::MobileObjSpec& obj) const;
        // A cell's quarter turns from the editor (PlateRotationMetadata.hpp, OR-0071); world coordinates.
        [[nodiscard]] int CellQuarterTurns(float x, float y, float z) const;
        // An object's or BigDecor's quarter turns at its cell (kObjectTurnMetadataType, OR-0071).
        [[nodiscard]] int CellObjectQuarterTurns(float x, float y, float z) const;
        // Draw()'s body; Draw() times it for GALAXY_PERF.
        void DrawFrame();
        // DrawFrame's parts (GalaxyEggbertGameDraw.cpp): the 3D world, then the HUD, menus and editor
        // over it, then the frame's captures (screenshots, golden frames, autoplay shots).
        void DrawWorld(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                       const GalaxyEggbert::Game::WorldRenderAssets& assets);
        // The game's loaded content the world is drawn with, lent to the renderer.
        [[nodiscard]] GalaxyEggbert::Game::WorldRenderAssets SceneAssets();
        void DrawOverlay(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, bool soulDrawnIn3D);
        void DrawFrameCaptures(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        void CheckTerrainVisibility(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        // The SceneFrame (GalaxyEggbertGameScene.cpp): what the 3D world draws this frame, by part.
        void BuildSceneFrame(GalaxyEggbert::Game::SceneFrame& frame, bool& soulDrawnIn3D);
        [[nodiscard]] bool SceneEditorSelected(int kind, float rawX, float rawY, float rawZ) const;
        void AppendSceneCubes(GalaxyEggbert::Game::SceneFrame& frame);
        void AppendSceneBlupi(std::vector<GalaxyEggbert::Game::SceneModel>& models);
        void AppendSceneVehicles(GalaxyEggbert::Game::SceneFrame& frame);
        void AppendSceneEnemyBlupis(std::vector<GalaxyEggbert::Game::SceneModel>& models);
        void AppendSceneObjectModels(GalaxyEggbert::Game::SceneFrame& frame, bool& soulDrawnIn3D);
        void AppendSceneDecals(GalaxyEggbert::Game::SceneFrame& frame);
        void AppendSceneEditorStarts(std::vector<GalaxyEggbert::Game::SceneModel>& models);
        void AppendSceneBillboards(GalaxyEggbert::Game::SceneFrame& frame);
        void AppendSceneEffects(GalaxyEggbert::Game::SceneFrame& frame);
        void AppendSceneMoments();
        // What AppendSceneMoments() saw last tick (landing, into the water, teleporting).
        bool momentWasGround_ = true;
        bool momentWasSwimming_ = false;
        bool momentWasTeleporting_ = false;
        int momentAirTicks_ = 0;
        GalaxyEggbert::Game::EffectParticles effectParticles_;
        GalaxyEggbert::Game::PostProcess postProcess_;
        GalaxyEggbert::Game::UltraFinish ultraFinish_; // Ultra's HDR finish in place of postProcess_
        // What Ultra lends the world's shaders (UltraAssets.hpp) and what it points at.
        GalaxyEggbert::Game::UltraSceneAssets ultraAssets_;
        GalaxyEggbert::Game::SkyEnvironment ultraSkyData_;
        // Ultra's light from around for the terrain (TerrainGi.hpp): the block types' colours, at load.
        GalaxyEggbert::Game::TerrainGiSettings terrainGi_;
        // Ultra: the light from around at each billboard and particle (SceneQuad::gi), by cell, cached for
        // the terrain it was found in (ApplySpriteGi).
        void ApplySpriteGi(GalaxyEggbert::Game::SceneFrame& frame);
        std::unordered_map<std::uint64_t, Microsoft::Xna::Framework::Vector3> spriteGiCache_;
        const void* spriteGiTerrain_ = nullptr;
        // The two waters' bodies for the effects' water shaders (SceneEffects::water), from the atlas.
        std::array<Microsoft::Xna::Framework::Vector4, 2> waterBodies_{};
        const GalaxyEggbert::Game::TerrainGiSettings* TerrainGiWanted();
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> ultraSky_;
        void BuildUltraSky(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const std::uint8_t* rgba, int width,
                           int height);
        void UploadUltraSky(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        // Ultra's terrain atlas, finer, with its normals and surface (TerrainMaps.hpp): on its first frame.
        void BuildUltraTerrainMaps(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        bool ultraTerrainMapsTried_ = false;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> ultraTerrainAlbedo_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> ultraTerrainNormals_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> ultraTerrainSurface_;
        // Ultra's lending for this frame (its textures made on its first frame); nullptr below Ultra.
        const GalaxyEggbert::Game::UltraSceneAssets* UltraSceneAssetsForFrame(
            Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        bool worldIntoTarget_ = false; // the world is being drawn into postProcess_'s target
        // The world's sky colour for the effects' distance haze: the middle of its background picture.
        Microsoft::Xna::Framework::Vector3 backgroundHaze_{0.0f, 35.0f / 255.0f, 98.0f / 255.0f};
        GalaxyEggbert::Game::SceneFrame sceneFrame_;
        GalaxyEggbert::Game::WorldRenderer worldRenderer_;
        // The editor's target, read once a frame for the scene's selection overlays.
        std::optional<GalaxyEggbert::Editor::WorldEditor::EditTarget> sceneEditTarget_;
        // A vehicle pickup's model and standing clip when the real vehicle
        // model replaces its billboard (kParkedVehicles); model is nullptr
        // otherwise.
        struct ParkedVehicleDraw
        {
            Microsoft::Xna::Framework::Graphics::SkinnedModelEXT* model = nullptr;
            const char* clip = nullptr;
        };
        [[nodiscard]] ParkedVehicleDraw ParkedVehicleModel(GalaxyEggbert::Def::ObjectType type) const;

        // Objects drawn as 3D models converted from the Ray Dream scenes
        // their sprites were rendered from (avatars3d/objects/, plan.md
        // Phase 7B): the models in index.txt's order, and which of them draws
        // which sprite icon. An icon objectModelTable_ lists is drawn as its
        // model instead of its billboard; any other icon stays a billboard.
        std::vector<std::shared_ptr<Microsoft::Xna::Framework::Graphics::SkinnedModelEXT>> objectModels_;
        // Each object model's reach: its vertices' farthest distance from its origin (blocks), for
        // the frustum test of the model pass (drawModel). Models out of view are not drawn: the
        // treasure chest is 68 068 triangles, a plant 182 496. GALAXY_MODEL_CULL=0 draws them all.
        std::vector<float> objectModelRadii_;
        // Per object model, its parts' Ray Dream materials (empty: none), for the effects' model shader.
        std::vector<std::vector<GalaxyEggbert::Game::ProceduralMaterial>> objectPartMaterials_;
        bool modelCulling_ = true;
        // GALAXY_PERF=1 (measurements): every 2 s on stdout the mean Draw time and the model pass's
        // models drawn, their triangles and the models culled, a frame each.
        bool perfEnabled_ = false;
        int perfFrames_ = 0;
        double perfDrawSeconds_ = 0.0;
        long long perfModelDraws_ = 0;
        long long perfModelTriangles_ = 0;
        long long perfModelCulled_ = 0;
        // A sign's painted face (its digit or world picture) as a decal on its model (MR-0006;
        // avatars3d/objects/<sign>/decals.txt): the texture and its rectangle in model space.
        struct SignDecal
        {
            std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> texture;
            float left = 0, right = 0, top = 0, bottom = 0, depth = 0;
        };
        std::unordered_map<int, SignDecal> signDecals_;
        struct SignDecalCell
        {
            std::uint16_t icon;
            float x, y, z, yaw;
        };
        std::vector<SignDecalCell> signDecalCells_; // this frame's signs with a decal
        // The electrode's sparks (Q-005): per icon 238-242 its painted sparks, drawn facing the camera
        // over its model (avatars3d/objects/electro238_electro/sparks/<icon>.png), and this frame's cells.
        std::unordered_map<int, std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D>> electrodeSparks_;
        std::vector<SignDecalCell> electrodeCells_;
        // Element 253-255: the lit fuse's painted star; 252 has no decal.
        std::unordered_map<int, std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D>> dynamiteSparks_;
        std::vector<SignDecalCell> dynamiteSparkCells_;
        // The glue model was drawn this frame: its strands follow (Q-006).
        bool glueStrandsPending_ = false;
        GalaxyEggbert::Game::ObjectModelTable objectModelTable_;
        // The power drawn on him (Decor.cpp:782-818): the shield's orb and sparkles, the power's
        // stars (element.png), the Cloud's three sparks (explo.png), shown while the shared count is
        // over 25 and every other pair of ticks after; behind him, as the original draws them
        // first. Returns the power to draw, None for none.
        [[nodiscard]] GalaxyEggbert::Def::SecretPower BlupiAuraShown() const;
        // The original's m_blupiFocus: false while he is busy -- taking a pickup, teleporting, waving
        // goodbye, cheering at the exit (Win, Decor.cpp:1904, 6176) -- on the ground (in the air he
        // counts as focused for the hazards, Decor.cpp:5788).
        [[nodiscard]] bool BlupiFocused() const
        {
            return !blupi_.IsOnGround() ||
                   !(blupi_.IsPickupFrozen() || blupi_.IsTeleporting() || blupi_.IsBye() || winCheerActive_);
        }
        // The original's m_blupiFocus itself (not "or in the air"): false dying, gone after it, taking a
        // pickup, teleporting, waving goodbye or cheering. The spike traps snap then (BD-0067).
        // The Setup's "3D terrain" (SaveData::GetTerrainRelief), or GALAXY_TERRAIN_RELIEF when set.
        [[nodiscard]] bool TerrainReliefWanted() const;
        // The Setup's "3D effects" (SaveData::GetEffects3D), or GALAXY_EFFECTS=0/1 for tests.
        [[nodiscard]] bool Effects3DWanted() const;
        // The level: 0 off, 1 low, 2 medium, 3 high (SaveData::GetEffectsLevel, or GALAXY_EFFECTS).
        [[nodiscard]] int EffectsLevel() const;
        // The terrain built with the effects' ambient occlusion (Effects3DWanted() and their shader compiles).
        [[nodiscard]] bool TerrainOcclusionWanted();
        // After the setting changed: the reliefs loaded or dropped, the terrain built again.
        void ApplyTerrainReliefSetting();
        [[nodiscard]] bool BlupiFocusRaw() const
        {
            return !(blupi_.IsDeathLocked() || blupi_.IsDeathHidden() || blupi_.IsPickupFrozen() ||
                     blupi_.IsTeleporting() || blupi_.IsBye() || winCheerActive_);
        }
        // Out of play for the shots (Decor.cpp:5914-5917): cheering at the exit (Win), waving goodbye
        // (Bye), dying (Clear*, Glu, Electro) or gone after it (Hide). Not the busy states: a shot
        // still meets him taking a pickup or teleporting.
        [[nodiscard]] bool BlupiOutOfPlay() const
        {
            return winCheerActive_ || blupi_.IsBye() || blupi_.IsDeathLocked() || blupi_.IsDeathHidden();
        }
        // The terrain tiles' reliefs (Q-001/Q-004/Q-012, avatars3d/terrain/), loaded on the first
        // terrain build; drawn within terrainReliefRadius_ of the camera's target.
        // GALAXY_TERRAIN_RELIEF=0 turns them off, GALAXY_TERRAIN_RELIEF_DIR reads another directory,
        // GALAXY_TERRAIN_RELIEF_RADIUS sets the radius, GALAXY_TERRAIN_RELIEF_OVERHANG=1 keeps the
        // overhangs into the air (Q-012 C).
        const GalaxyEggbert::Game::TerrainReliefTable* TerrainReliefs();
        bool terrainReliefsLoaded_ = false;
        bool terrainReliefOverhangs_ = false;
        float terrainReliefRadius_ = 8.0f;
        // A fresh terrain builds every near relief chunk at once (no cubes popping into reliefs).
        bool terrainReliefWarm_ = false;
        // The sheet and icon a MoveObject's billboard shows this frame, picked
        // as the billboard passes pick them; false when it shows none (hidden,
        // inactive, a cube, a parked vehicle, an invisible frame).
        [[nodiscard]] bool ObjectSprite(const GalaxyEggbert::Game::MobileObjSpec& obj,
                                        GalaxyEggbert::Game::ObjectTextureSource& sheet, int& icon) const;

        // Tracks which clip is currently playing so blupiClipTimeSeconds_
        // resets to 0 on a change instead of continuing from a position that
        // may be past the new clip's own duration (clips have different
        // lengths -- Walk is ~0.7s, Survey ~3.4s, see avatars3d/
        // blupi_placeholder/README.md). Accumulated in Update() regardless
        // of camera mode so switching into third-person mid-animation
        // starts a clip at a sensible position, not always frame 0.
        std::string blupiActiveClipName_;
        double blupiClipTimeSeconds_ = 0.0;

        // Down-arrow turn-around in progress (see Update()): seconds left, its
        // length (TurnAroundSeconds) and the yaw it started from; Down's
        // previous state, so one press turns once.
        float turnAroundRemaining_ = 0.0f;
        float turnAroundDuration_ = 0.3f;
        float turnAroundStartYaw_ = 0.0f;
        bool turnAroundDownWasHeld_ = false;
    };
}
