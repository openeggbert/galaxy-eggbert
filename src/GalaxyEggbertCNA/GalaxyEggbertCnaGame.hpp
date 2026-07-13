#pragma once

#include "Game/GEWorldRuntime.hpp"
#include "Game/GETileAtlas.hpp"
#include "Game/GETerrainRenderer.hpp"
#include "Game/GEBlupiController.hpp"
#include "Game/GEObjectIcons.hpp"
#include "Game/GESound.hpp"
#include "Game/GEInteractionSystem.hpp"
#include "Game/GEHud.hpp"
#include "Game/GETrainingHints.hpp"
#include "Game/GEInputPad.hpp"
#include "Game/GESaveData.hpp"

#include <GalaxyEggbert/def/GamePhase.hpp>

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
    // (same key Simple3D already used for its own, differently-scoped
    // camera-mode toggle -- perspective/isometric there, first/third-person
    // here; same "switch camera view" idea).
    class GalaxyEggbertCnaGame final : public Microsoft::Xna::Framework::Game
    {
    public:
        GalaxyEggbertCnaGame();

        void LoadContent() override;
        void Update(Microsoft::Xna::Framework::GameTime& gameTime) override;
        void Draw(const Microsoft::Xna::Framework::GameTime& gameTime) override;

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

        // Real mobile-eggbert sound playback (2026-07-10, see GESound.hpp).
        // jumpKeyWasDown_ edge-detects the jump key the same way
        // cameraModeKeyWasDown_ edge-detects "C" above; stepSoundTimer_
        // paces footstep sounds while marching (reset whenever Blupi isn't
        // marching, so it doesn't fire immediately on the next step).
        GESound sound_;
        bool jumpKeyWasDown_ = false;
        float stepSoundTimer_ = 0.0f;

        // Switch/saw linking (plan.md E3D-MIG-142, see GEWorldRuntime::
        // TryActivateSwitch()) -- Space ("Action", already read as
        // actionPressed) edge-detected the same way jumpKeyWasDown_ is.
        bool actionKeyWasDown_ = false;

        // Platform lift patrol, crate push, and pickup collection
        // (2026-07-10, see GEInteractionSystem.hpp).
        GEInteractionSystem interaction_;

        // Real Def::Phase state machine (2026-07-13, plan.md HUD-023),
        // verified directly against Def.hpp's own enum (already ported
        // verbatim as GalaxyEggbert::GamePhase) and Game1.cpp's real
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
        void SetPhase(GalaxyEggbert::GamePhase next, bool bypassFade = false) noexcept;
        [[nodiscard]] const char* PhaseOverlayMessage() const noexcept;

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
        GalaxyEggbert::GamePhase phase_ = GalaxyEggbert::GamePhase::Wait;
        bool pauseKeyWasDown_ = false;
        bool phaseReturnKeyWasDown_ = false;

        // Real `fadeOutPhase` (plan.md MENU-088/089) -- see SetPhase()'s
        // own comment above for the full real deferred-transition
        // mechanic this drives. `None` means "not currently exiting"
        // (covers both Play and every other phase's own settled/idle
        // state); any other value is the real pending destination during
        // an active ~1.0s exit fade.
        GalaxyEggbert::GamePhase fadeOutPhase_ = GalaxyEggbert::GamePhase::None;

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

        // Hidden cheat menu (plan.md CHEAT-001..009, 2026-07-13) -- real
        // gesture zones are checked only during real Phase::Play
        // (confirmed via research); once the real 10-tap sequence
        // completes, the overlay renders layered on top of the normal
        // Play view (no phase change, no background swap) until a cheat
        // button is pressed. See `GEInputPad::UpdateCheatGesture()`'s own
        // class comment for the full real-behavior citation.
        bool cheatMenuShown_ = false;

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
        // §9.2 — confirmed non-colliding, Billboard render mode). Unlike
        // MoveObjects, BigDecor uses the SAME icon vocabulary as the main
        // terrain grid (object-m.png via tileAtlas_), not element.png — so
        // this reuses terrainTexture_ through its own dedicated effect
        // (BasicEffect only binds one texture at a time). Only ever
        // non-empty when a world was loaded via LoadFromMobileEggbertFile()
        // (the default .vwr world has no BigDecor concept). bigDecorCells_
        // is the filtered (non-air) cell list, computed once in
        // LoadContent() from the fixed 100x100 grid so Draw() doesn't have
        // to re-scan all 10000 cells every frame — only the camera-facing
        // billboard mesh itself is rebuilt per frame, same reason as
        // objectMeshRenderer_ above.
        struct BigDecorCell
        {
            float worldX;
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
