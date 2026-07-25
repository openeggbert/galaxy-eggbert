#pragma once

#include "Game/GEWorldRuntime.hpp"
#include "Game/GETileAtlas.hpp"
#include "Game/GETerrainRenderer.hpp"
#include "Game/GEBlupiController.hpp"
#include "Game/GECameraShake.hpp"
#include "Game/GEObjectIcons.hpp"
#include "Game/GESound.hpp"
#include "Game/GEInteractionSystem.hpp"
#include "Game/GEHud.hpp"
#include "Game/GETrainingHints.hpp"
#include "Game/GEInputPad.hpp"
#include "Game/GESaveData.hpp"
#include <GalaxyEggbert/Editor/WorldEditor.hpp>

#include <GalaxyEggbert/Def/GamePhase.hpp>
#include <GalaxyEggbert/Def/GameSpeed.hpp>

#include <Easy3D/BillboardMeshRenderer.hpp>
#include <Easy3D/Camera3D.hpp>
#include <Easy3D/CubeMeshRenderer.hpp>
#include <Microsoft/Xna/Framework/GamerServices/AvatarRenderer.hpp>
#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Graphics/SkinnedModelEXT.hpp>
#include <Microsoft/Xna/Framework/GraphicsDeviceManager.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace GalaxyEggbert::CNA
{
    // Direct CNA + Easy3D target: loads a hand-authored .vwr world, renders
    // real textured/animated 3D terrain, and moves an invisible,
    // collision-only Blupi placeholder through it (see NEXT.md for current
    // status). Two camera modes now exist (2026-07-09): first-person
    // (default, unchanged since 2026-07-05 -- Blupi's current animation
    // state shown via a small 2D indicator in the screen's bottom-right
    // corner instead of an in-world sprite) and third-person, showing a
    // real GPU-skinned 3D model via CNA's AvatarRenderer real-rendering
    // extension (see ../cna/docs/avatar-real-rendering-ext.md) -- currently
    // a temporary placeholder model (avatars3d/blupi_placeholder/, see its
    // own README.md), not yet a real Blupi model. "C" toggles between modes
    // ("C" is the project's established switch-camera key).
    class GalaxyEggbertCnaGame final : public Microsoft::Xna::Framework::Game
    {
    public:
        GalaxyEggbertCnaGame();

        void LoadContent() override;
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

        GetTypeNameHPP()

    private:
        // Camera mode (2026-07-09, NEXT.md §3) -- toggled by "C", edge-
        // detected the same way as the demo_avatar example's Space-toggle
        // (cameraModeKeyWasDown_ below) so a held key doesn't rapid-fire.
        enum class CameraMode : std::uint8_t { FirstPerson, ThirdPersonModel };
        CameraMode cameraMode_ = CameraMode::FirstPerson;
        bool cameraModeKeyWasDown_ = false;
        // Drives terrainEffect_'s View/Projection every frame.
        Easy3D::Camera3D camera_;

        // Smoothed camera eye/target (2026-07-09) -- both camera modes below
        // compute a raw "desired" eye/target from Blupi's live
        // position/yaw every frame; snapping camera_ straight to that
        // value feels too fast/jerky (reported live). Instead the raw
        // value is exponentially damped toward camera_'s actual
        // position/target each frame (see kCameraDampingPerSecond in the
        // .cpp). Initialized lazily on the first Update() so load-time
        // doesn't spend a visible lerp-in from the origin.
        bool cameraSmoothedInitialized_ = false;
        Easy3D::Camera3D::Vector3 cameraEyeSmoothed_{0.0f, 0.0f, 0.0f};
        Easy3D::Camera3D::Vector3 cameraTargetSmoothed_{0.0f, 0.0f, 0.0f};

        // Parses worlds/world001.txt (plan.md Phase 4).
        GEWorldRuntime worldRuntime_;

        // Real mobile-eggbert background image for worldRuntime_'s skyRegion
        // (2026-07-09, NEXT.md §3 -- mobile-eggbert-reference/
        // 05-backgrounds.md/15-3d-render-mapping-design.md §9.5). Drawn as a
        // single huge camera-facing billboard quad placed far behind the
        // scene (same BillboardBatch/BuildBillboardMesh/BillboardMeshRenderer
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
        std::unique_ptr<Easy3D::BillboardMeshRenderer> backgroundMeshRenderer_;
        bool backgroundLoaded_ = false;

        // Maps block types to object-m.png UV rects (plan.md E3D-MIG-053).
        GETileAtlas tileAtlas_;

        // Static (non-animated) terrain mesh for the loaded world (plan.md
        // E3D-MIG-054). Lazily constructed in LoadContent() since it needs a
        // live GraphicsDevice.
        std::unique_ptr<GETerrainRenderer> terrainRenderer_;
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
        // drawn via GETerrainRenderer::DrawGrass() right after the main
        // terrain Draw() call.
        Microsoft::Xna::Framework::Graphics::Texture2D grassTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> grassEffect_;

        // Invisible, collision-only movement placeholder (plan.md
        // E3D-MIG-060) — arrow keys + Space, first-person camera follows
        // its facing. No 3D sprite yet (E3D-MIG-061..063) — see blupiIcon_
        // below for the interim 2D stand-in.
        GEBlupiController blupi_;

        // Sucette/Drink/Charge real 2-stage pickup delay (plan.md `173`) --
        // stashes the pickup's own position/type from contact time until
        // the freeze resolves (potentially several frames/seconds later,
        // past when interaction_'s own *ThisFrame() signal has reset), so
        // ResolvePickupFreeze() can re-spawn it at completion. Only one
        // freeze can be active at a time (GEBlupiController::
        // TriggerPickupFreeze() is a no-op while already frozen), so a
        // single pending slot is sufficient.
        float pendingPickupX_ = 0.0f, pendingPickupY_ = 0.0f, pendingPickupZ_ = 0.0f;
        GalaxyEggbert::Def::ObjectType pendingPickupType_ = GalaxyEggbert::Def::ObjectType::ObjectType0;

        // Real screen-shake/forced-pan camera effect (plan.md CAM-008..013,
        // see GECameraShake.hpp's own comment for the full real-behavior
        // citation) -- ticked every Play-phase frame, its (dx,dy) pixel
        // offset applied as a small camera-space perturbation after the
        // normal eye/target damping (see Draw()'s own camera block).
        GECameraShake cameraShake_;

        // Real mobile-eggbert sound playback (2026-07-10, see GESound.hpp).
        // jumpKeyWasDown_ edge-detects the jump key the same way
        // cameraModeKeyWasDown_ edge-detects "C" above; stepSoundTimer_
        // paces footstep sounds while marching (reset whenever Blupi isn't
        // marching, so it doesn't fire immediately on the next step).
        GESound sound_;
        bool jumpKeyWasDown_ = false;
        float stepSoundTimer_ = 0.0f;

        // Real "Bye" farewell freeze (plan.md BLUPI-049) -- the mission to
        // load once `blupi_.IsBye()` naturally clears; set only at the world-
        // select portal contact site (`Update()`), consumed by the
        // `wasBye`/`IsBye()` completion check right after `Step()`.
        int byePendingTarget_ = 0;

        // Switch/saw linking (plan.md E3D-MIG-142, see GEWorldRuntime::
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

        // Platform lift patrol, crate push, and pickup collection
        // (2026-07-10, see GEInteractionSystem.hpp).
        GEInteractionSystem interaction_;

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
        // transitions to `Resume` if `GESaveData::GetHasProgress()` is
        // true, else `Init` (an adaptation of the real `Wait`->`Resume`
        // branch, which for real is gated on a WP7-only OS-reactivation
        // snapshot this engine has no equivalent for -- same adapted
        // trigger already established for Resume itself). `Init` renders
        // the real init.png/speedyblupi.png/blupiyoupie.png gamer-select
        // menu (3 independent slots via `GESaveData`'s own 3-gamer-slot
        // extension) -- see `GEInputPad::UpdateInit()`/`DrawInit()`'s own
        // class comment for full detail, including what's deliberately
        // NOT ported (InitRanking/InitBuy, real exit-fade animations).
        //
        // `Trial`/`Ranking` are real enum values with NO trigger wired to
        // them yet (no upsell/ranking screens exist) -- reachable in
        // principle, unreachable in practice until those screens exist.
        // `MainSetup` is now ALSO reachable (2026-07-13, via Init's own
        // InitSetup button), sharing `GEInputPad::UpdateSetup()`/
        // `DrawSetup()` with `PlaySetup` (2026-07-13, plan.md
        // MENU-058..069, reachable via Pause's real Setup button) -- see
        // that method's own class comment for what is/isn't modeled on
        // that screen, including the one real difference between the two
        // (`SetupReset`, MainSetup-only). `Resume` is reachable (2026-07-13,
        // plan.md MENU-040..045) via an ADAPTED trigger (the `Wait`->
        // `Resume`-or-`Init` branch above), not the real `Game1::
        // OnActivated()` OS-reactivation event, which has no desktop
        // equivalent -- see `GEInputPad::UpdateResume()`'s own class
        // comment for full reasoning.
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
        // confirmed via research. See `GEInputPad::DrawPause()`/
        // `DrawResume()`/`DrawSetup()`/`DrawInit()`'s own class comments
        // for the exact real per-destination fade formulas.
        //
        // Real Lost trigger: a death animation completing while lives are
        // exhausted (`Decor.cpp:6374-6435`) -- this engine's own
        // `GEInteractionSystem::GameOverCount()` already increments at
        // exactly that real moment (`DoorsLost()`'s reset-to-3 behavior).
        // Real Win trigger: reaching the exit with all treasure
        // (`Decor::IsTerminated()`) -- already exactly what
        // `GEInteractionSystem::ExitReached()` gates on. Real WinLostReturn
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
        // `FindInteractionEvent(GEInteractionSystem::EventKind::VoyageRequested)` finds one. Projects
        // whichever endpoint is still a 3D world position (via
        // `GEHud::ProjectWorldToHudSpace()`, using `camera_` and the real
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
        // `FindInteractionEvent(GEInteractionSystem::EventKind::DeathLockRequested)` finds one
        // (the generic-hazard-contact/dynamite/projectile/large-creature-grab pending
        // signal from inside `interaction_.Update()`, see
        // `GEInteractionSystem::EventKind::DeathLockRequested`'s own
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

        // Forces Blupi off whatever vehicle he's currently riding and deposits the matching real
        // pickup back into the world at his position (real "deposits vehicle pickup back into
        // the world" behavior) -- shared by the voluntary action-button dismount and the real
        // spring-forces-a-dismount case (`Decor.cpp:2837-2893`, found 2026-07-16). A no-op if not
        // currently in a vehicle. Does NOT play any sound itself -- callers differ on which real
        // sound applies (or none).
        void DismountAndDepositVehicle();

        // Hub/mission-progression system (plan.md `MENU-035`/`036`,
        // `SCORE-013`-`019`, found/implemented 2026-07-17). Rebuilds
        // `backgroundTexture_`/`backgroundEffect_` (skyRegion-dependent) and
        // `terrainRenderer_` from whatever's CURRENTLY loaded in
        // `worldRuntime_` -- shared by `LoadContent()`'s own initial load
        // (which handles `blupi_`'s spawn/`saveData_`'s load itself, at its
        // own point in the boot sequence) and `LoadMission()` below.
        void RebuildWorldPresentation();

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
        void LoadMission(int missionNumber);

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

        // Real vehicle motor sound crossfade (plan.md SOUND-007/008, `GEBlupiController::
        // HasVehicleMotor()`/`IsVehicleMotorHigh()`, found 2026-07-17) -- ports
        // `Decor::AdaptMotorVehicleSound()` exactly: computes the desired loop channel (none,
        // or the mode's own high/low variant), and if it differs from `activeMotorLoop_`, plays
        // the one-shot start sound (silence->motor) or stop sound (motor->silence), stops the
        // old loop, and starts the new one. A no-op when the desired loop already matches the
        // active one (matches the real early-out exactly). Called once per frame.
        void UpdateVehicleMotorSound();

        // INFRA-007 (plan.md §7, step 2/3): GEInteractionSystem's 11 simple/position-payload
        // *ThisFrame() signals are now one typed EventsThisFrame() queue instead of 11 parallel
        // booleans -- this finds the (at most one, per-Kind) event of a given Kind this frame, or
        // nullptr if none fired. Returns a pointer rather than bool so payload-carrying kinds
        // (PowerGranted/CloudGranted/HideGranted) can read their pickup position straight off the
        // returned Event, same call site, no separate position getter needed.
        [[nodiscard]] const GalaxyEggbert::CNA::GEInteractionSystem::Event*
        FindInteractionEvent(GalaxyEggbert::CNA::GEInteractionSystem::EventKind kind) const noexcept;

        // Sucette/Drink/Charge real 2-stage pickup delay (plan.md `173`) --
        // a no-op unless FindInteractionEvent() finds a PowerGranted/
        // CloudGranted/HideGranted event this frame (plays the real
        // immediate "grab" sound, starts `blupi_.TriggerPickupFreeze()`,
        // and stashes the event's own pickup position into
        // `pendingPickup*_` below since the freeze outlives this frame) OR
        // `blupi_.ConsumePickupFreezeResolved()` fires (plays the real
        // "complete" sound, grants Power/Hide via `blupi_.TriggerPower()`/
        // `TriggerHide()` -- Cloud's own buff already granted at contact,
        // matching real source -- and re-spawns the item at the stashed
        // position via `interaction_.RespawnPickupItem()`). Called right
        // after `interaction_.Update()` returns, alongside
        // `ResolveDeathLock()` above.
        void ResolvePickupFreeze();

        // Dispatches cheat 1-9 (plan.md CHEAT-001..009) -- verified
        // directly against the real `Decor::CheatAction(Tables::
        // CheatCodes)`, correcting several wrong/imprecise draft
        // descriptions from an earlier, unverified plan.md pass (see
        // GEInteractionSystem.hpp's own per-cheat comments for exactly
        // what each one corrects). Cheat3 "ShowSecret" is NOT implemented
        // -- the real effect gates rendering of "hidden ObjectType12
        // secret-decor icons", but this engine's own ObjectType12 is
        // already a confirmed, unrelated real type (a pushable crate,
        // plan.md E3D-MIG-15x) -- whether the real citation means the
        // MoveObject enum value or an unrelated STATIC TILE icon that
        // happens to also be "12" was not resolved this session, and
        // guessing risks silently reusing the crate machinery for the
        // wrong feature. A documented gap, not a silent omission.
        void ApplyCheat(int cheatNumber);
        GalaxyEggbert::Def::GamePhase phase_ = GalaxyEggbert::Def::GamePhase::Wait;
        bool pauseKeyWasDown_ = false;
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
        // `kGoldenCaptureTicks` (GalaxyEggbertCnaGame.cpp) in Draw().
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
        bool goldenTraceWritten_ = false;
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
        // (`GEInputPad::DrawWinLost`); not used for the Play-phase
        // gameplay simulation, which has no timer concept of its own.
        float phaseTimeSeconds_ = 0.0f;

        // Real mobile-eggbert bottom HUD + the interim animation-state
        // indicator (2026-07-10, see GEHud.hpp) -- replaces the earlier
        // SpriteBatch-based HUD entirely: on CNA's Vulkan backend every
        // SpriteBatch batch is recorded BEFORE every 3D draw within the
        // frame, so a sprite HUD gets painted over by the 3D scene (the
        // "icon visible for a second, then gone" live report); GEHud draws
        // real 3D quads instead, recorded in genuine submission order on
        // both backends.
        GEHud hud_;

        // Real mobile-eggbert on-screen touch controls (2026-07-13,
        // plan.md MENU-021..027/028..039, see GEInputPad.hpp) -- mouse-
        // driven Pause row (real background/character art + 5 real
        // pad.png buttons, Continue/Restart functionally wired) and Play
        // on-screen D-pad/Jump/Action/Pause controls, OR'd with keyboard
        // input in the movement block below. UpdatePlay()'s/UpdatePause()'s
        // own return value tells this class whether the current mouse
        // press landed on a control, so the pre-existing mouse drag-look
        // camera code (below) can skip its own handling and avoid the two
        // features fighting over the same left-mouse-button input.
        GEInputPad inputPad_;

        // Minimal settings persistence (2026-07-13, plan.md MENU-067, see
        // GESaveData.hpp for why this is NOT byte-compatible with the
        // real mobile-eggbert GameData). Loaded once in LoadContent();
        // Save() is called right after the SetupSounds toggle, matching
        // the real source's own "write immediately on toggle press"
        // behavior.
        GESaveData saveData_;

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
        std::filesystem::path editorPlayTestWorldPath_;

        // Hidden cheat menu (plan.md CHEAT-001..009, 2026-07-13) -- real
        // gesture zones are checked only during real Phase::Play
        // (confirmed via research); once the real 10-tap sequence
        // completes, the overlay renders layered on top of the normal
        // Play view (no phase change, no background swap) until a cheat
        // button is pressed. See `GEInputPad::UpdateCheatGesture()`'s own
        // class comment for the full real-behavior citation.
        bool cheatMenuShown_ = false;

        // Real GalaxyEggbert::Def::GameSpeed (SCORE-009/010/011, added 2026-07-20, see
        // include/GalaxyEggbert/Def/GameSpeed.hpp and InputPad.cpp:582-684
        // for the full real key-mapping citations) -- F5/F6 always work,
        // F7/F8 need quickCheatEnabled_ (real "quick" typed cheat, see
        // GEInputPad::UpdateTypedGhostCheat()'s own TypedCheatResult), Tab
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
        // positions depend on the camera (see Easy3D::BillboardMeshRenderer's
        // header comment) — objectMeshRenderer_ is reconstructed in Draw(),
        // not lazily cached like terrainRenderer_.
        Microsoft::Xna::Framework::Graphics::Texture2D objectTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> objectEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> objectMeshRenderer_;

        // Billboard rendering for the 5 confirmed object-m.png-sourced
        // MoveObjects (GEObjectIcons::IsObjectMPngSourced, NEXT.md §3,
        // 2026-07-09) -- same camera-facing billboard technique as
        // objectMeshRenderer_ above, but reuses terrainTexture_
        // (object-m.png) via its own dedicated effect (BasicEffect only
        // binds one texture at a time), same reason as bigDecorEffect_.
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> objectMPngEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> objectMPngMeshRenderer_;

        // Billboard rendering for the 12 confirmed explo.png-sourced
        // MoveObjects (GEObjectIcons::IsExploPngSourced, NEXT.md §3,
        // 2026-07-09) -- explosions/visual effects, a genuinely new texture
        // (not previously loaded anywhere in GalaxyEggbertCNA), same
        // camera-facing billboard technique as objectMeshRenderer_ above.
        Microsoft::Xna::Framework::Graphics::Texture2D exploTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> exploEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> exploMeshRenderer_;

        // Billboard rendering for the 4 confirmed Blupi-skin MoveObjects
        // (GEObjectIcons::IsBlupiPngSourced, NEXT.md §3, 2026-07-09) --
        // ObjectType200 sources blupi.png, ObjectType201/202/203 source
        // blupi1.png (GEObjectIcons::UsesBlupi1Texture) -- two separate
        // textures/effects/renderers since BasicEffect only binds one
        // texture at a time. blupiObjectTexture_/blupiObjectEffect_ are
        // deliberately separate from the existing blupiIconTexture_/
        // blupiIconBatch_ above (that pair is 2D SpriteBatch-coupled for
        // the HUD indicator, not reusable for this 3D BasicEffect path),
        // even though both load the same blupi.png file.
        Microsoft::Xna::Framework::Graphics::Texture2D blupiObjectTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> blupiObjectEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> blupiObjectMeshRenderer_;
        Microsoft::Xna::Framework::Graphics::Texture2D blupi1ObjectTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> blupi1ObjectEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> blupi1ObjectMeshRenderer_;

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
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> bigDecorEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> bigDecorMeshRenderer_;

        // Platform-lift/crate UniformCube object path (NEXT.md §8 task 3,
        // mobile-eggbert-reference/15-3d-render-mapping-design.md §5's two
        // confirmed "render as a cube, not a billboard" exceptions:
        // ObjectType1/47/48 platform lifts and ObjectType12 crates, see
        // GEObjectIcons::IsUniformCubeObject). Reuses terrainTexture_
        // (object-m.png, the confirmed-correct sheet for these types) via
        // its own effect, same reason as bigDecorEffect_ above. Rebuilt
        // every frame in Draw() (2026-07-09) using each MobileObjSpec's
        // real per-instance phase, same reason and pattern as the
        // billboard renderers below -- CubeMeshRenderer has no in-place UV
        // update API, only construction from vertex/index data, so a fresh
        // mesh each frame is the only way for types 47/48's chenille icon
        // formulas to actually animate (cheap here, only a handful of cube
        // objects).
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> objectCubeEffect_;
        std::unique_ptr<Easy3D::CubeMeshRenderer> objectCubeMeshRenderer_;

        // Third-person-mode 3D model (2026-07-09, NEXT.md §3) -- CNA's
        // AvatarRenderer real-rendering extension (../cna/docs/
        // avatar-real-rendering-ext.md), loaded via getContentProperty()
        // with its RootDirectory pointed at avatars3d/ (see LoadContent()).
        // Currently loads avatars3d/blupi_placeholder/ (see its own
        // README.md) -- a temporary CC0/CC-BY placeholder, not a real Blupi
        // model. blupiModelLoaded_ guards against avatars3d/ being absent
        // (e.g. a build that never ran the CMake copy step) -- degrades to
        // staying in first-person mode rather than crashing.
        std::shared_ptr<Microsoft::Xna::Framework::Graphics::SkinnedModelEXT> blupiModel_;
        std::unique_ptr<Microsoft::Xna::Framework::GamerServices::AvatarRenderer> blupiAvatarRenderer_;
        bool blupiModelLoaded_ = false;

        // Tracks which clip is currently playing so blupiClipTimeSeconds_
        // resets to 0 on a change instead of continuing from a position that
        // may be past the new clip's own duration (clips have different
        // lengths -- Walk is ~0.7s, Survey ~3.4s, see avatars3d/
        // blupi_placeholder/README.md). Accumulated in Update() regardless
        // of camera mode so switching into third-person mid-animation
        // starts a clip at a sensible position, not always frame 0.
        std::string blupiActiveClipName_;
        double blupiClipTimeSeconds_ = 0.0;
    };
}
