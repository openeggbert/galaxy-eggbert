# NEXT.md — Galaxy Eggbert

_Last updated: 2026-07-13. Concise handoff document — full history lives in `git log` (502+
commits) and `plan.md`'s per-phase task lists. This file summarizes current state; it does not
replace either of those._

## 1. Project summary

**Galaxy Eggbert** is a faithful 3D remake of **mobile-eggbert** (itself a C++ port of *Speedy
Blupi*, a 2013 Windows Phone XNA game). Main goal: reimplement the same game logic, levels, and
assets in 3D (perspective camera, billboard sprites, 3D-rendered tiles) **without inventing new
mechanics** — see `CLAUDE.md`'s "Faithful Remake" rule, which overrides default behavior.

**Current development phase:** `GalaxyEggbertCNA` is the sole actively-developed target and has
reached a genuinely playable state — real terrain/objects/enemies/hazards/HUD/sound/save-system/
menus, though still missing a visible 3D Blupi model.

- **`GalaxyEggbertCNA`** (built directly on **CNA** + the **Easy3D** helper library) — opens a
  window, loads a hand-authored `.vwr` world (`worlds3d/world001.vwr`), renders real
  textured/animated terrain (4 confirmed tile render modes, all ~175 confirmed icons wired,
  face-culled), moves an invisible collision-point Blupi (tank controls + vehicles), and has a
  complete real gameplay loop: pickups, all 5 terrain hazards, the full enemy-AI/combat set,
  platform lifts/crates/bridges, doors & keys, secret powers/vehicles, bullet firing, a real
  `Def::Phase` state machine (Wait→Init→Play→Pause/Win/Lost/Setup/Resume, with real fade-out
  transitions between phases), a cross-restart save system (3 gamer slots), and a hidden cheat
  menu. No visible Blupi model yet (an invisible collision point in first-person; third-person
  shows a temporary placeholder model), no 3D world editor, one known-wrong render detail (Saw
  blade orientation).
- **`GalaxyEggbertSimple3D`** (built on `simple-3d` → U3D/Urho3D) — **historical reference only as
  of 2026-07-08, per user directive. Do not build, fix, or troubleshoot it.** Its build is broken
  in this environment (missing/incompatible U3D prebuilt) and that is intentionally left unfixed.

**Important architectural decisions:**

- **Direct CNA + Easy3D is the sole long-term target (locked 2026-07-05).** `GalaxyEggbertSimple3D`
  is scheduled for gradual removal as CNA reaches parity — its code stays intact for now (no
  removal without an explicit task), but is not built/fixed/verified going forward.
- **No Blupi 3D model exists yet — the user will provide one later.** Current stopgap: first-person
  camera only (no visible character) plus a small 2D animation-state sprite indicator; third-person
  mode shows a temporary CC0/CC-BY placeholder model via CNA's `AvatarRenderer`. Not a design
  decision about the eventual real model's look.
- Easy3D is a small helper library beside CNA (cameras, texture atlas, batching, mesh building) —
  must not hide CNA; game code calls CNA directly. Standing permission to modify `../easy-3d`
  (small, generic 3D-batching helpers only — no scene graph/ECS/engine creep).
- `../mobile-eggbert` is **read-only, never modified even temporarily**. Assets (PNGs, sounds,
  world files) are freely reused by direct path/build-time copy; code/data (`Decor.cpp`, tables,
  enum values, byte layouts) is reference-only and requires explicit user approval to copy — see
  `CLAUDE.md`'s reuse table. It has no CMake library target, so it cannot be linked as a dependency.
- No automated `.txt → .vwr` 2D-to-3D world converter (rejected direction) — worlds are
  hand-authored via `tools/GenerateSampleWorld3D.cpp`.
- **Terrain-tile identification is complete** — all 441 `object-m.png` icons accounted for across
  4 confirmed render modes (`DirectionalCube` 99, `InnerPillarBox` 3, `InnerFlatPlate` 63,
  `TripleCrossBillboard` 10), all wired up. No further tile-identification work remains except the
  Saw blade's own internal texture-orientation bug (see §4).
- **GESaveData/GEInputPad/GEInteractionSystem are NOT byte/format-compatible with real
  mobile-eggbert equivalents** (`GameData`, `InputPad`, `Decor`) — deliberately so, confirmed via
  research each time: the real formats are shaped around a 100+-level/3-gamer-slot/touch-hardware
  structure this engine's single hand-authored world doesn't have. Real *behavior* (formulas,
  gates, timings) is ported faithfully; byte layout is not.

## 2. Current status

### Build status
- **`GalaxyEggbertCNA` — last confirmed clean build on both graphics backends: 2026-07-13**
  (commit `63e020d`). Two build trees exist in this repo, both currently configured and buildable:
  - `build-cna/` — EasyGL backend (`CNA_GRAPHICS_BACKEND=EASYGL`, the default).
  - `build-cna-vulkan/` — Vulkan backend (`CNA_GRAPHICS_BACKEND=VULKAN`).
  - Current CMakeLists.txt defaults: `GALAXY_EGGBERT_BUILD_CNA=ON`, `GALAXY_EGGBERT_BUILD_SIMPLE3D=OFF`
    (i.e. a plain `cmake -S . -B build` with no flags now configures CNA, not Simple3D).
- **`GalaxyEggbertSimple3D` — not built, per user directive (2026-07-08).** Known broken in this
  environment (`find_package(Urho3D)` fails, U3D prebuilt missing/incompatible) — intentionally
  left unfixed. Do not attempt to build or fix it.

### Test status (all last run and passing, 2026-07-13, commit `63e020d`)
- `GalaxyEggbertWorldsTests` — 64/64 (gtest, engine-agnostic `World`/`Chunk`/`MoveObjectRecord`
  model, no CNA link needed).
- `VerifyBlupiMovement` — collision/step-up/gravity/vehicle-movement checks against
  `worlds3d/world001.vwr`.
- `VerifyInteractionSystem` — pickups, hazards, cheats, secret powers, bullet firing, Cloud aura
  (largest suite, links CNA).
- `VerifyGEInputPad` — on-screen control hit-testing (D-pad/Jump/Action/Pause/Setup/Init/cheat
  gesture), synthetic `MouseState`, no `GraphicsDevice` needed.
- `VerifyGESaveData` — `GESaveData` load/save round-trip, 3-gamer-slot isolation, `Reset()`.
- `VerifyMoveObjectTypesCna` — `MoveObject` parsing against real mobile-eggbert `.txt` level files.
- `VerifyBigDecorParsingCna` — `BigDecor:` parsing against real mobile-eggbert `.txt` level files.

### What works (high level — see `plan.md` for the exhaustive per-item checklist)
- Real textured/animated 3D terrain (4 render modes, face-culled), real background image per world.
- Tank-control Blupi (invisible collision point) + first-/third-person camera toggle.
- Full interactive-object system: platform lifts (patrol + riding), crates (linked-stack push),
  bridges, doors (key- and treasure-gated), all pickups (treasure/egg/keys/dynamite/bullets/Perso/
  secret-power pickups), level exit.
- Full enemy AI/combat: shared 8-type hazard kill-list, wasp balloon status, blupih/blupit
  projectile attacks, large-creature turn-dwell grab, follower wake+homing, all real patrol timing.
- All 5 real terrain hazards (lava/spikes/Blitz/saw+switches/crusher) + spring, Temp/vanishing
  tile, teleporter, water breath gauge.
- Secret powers (Shield/Power/Cloud/Hide) fully modeled including Cloud's offensive
  `BlupiElectro` aura (destroys small enemies within range) and all 5 vehicle mounts.
- Player-fired bullets while riding Tank (real cooldown/ammo gates).
- Real mobile-eggbert-faithful HUD (`GEHud`): lives/keys/treasure/bullets/dynamite/Perso icons,
  water and secret-power gauges, training-hint overlay — every element the real `Decor::DrawInfo`
  draws is implemented.
- Real `Def::Phase` state machine: `Wait` (progress gauge) → `Init` (gamer-select menu, 3 save
  slots) → `Play` ↔ `Pause` → `PlaySetup`/`MainSetup` (settings) , `Win`/`Lost` screens, `Resume`
  (offered after a checkpointed run). Real fade-out transitions between the 5 phases that actually
  defer (`Init`/`MainSetup`/`PlaySetup`/`Pause`/`Resume`) — Play↔Pause and Win/Lost are real
  instant cuts, confirmed via research, not merely fast.
- Cross-restart save system (`GESaveData`, plain text, not byte-compatible with real `GameData`):
  sound on/off + 3 independent gamer slots (lives/mission/hasProgress) + selected-gamer index.
- Hidden cheat menu: 10-tap gesture unlocks a 9-button overlay (OpenDoors/SuperBlupi/LayEgg/Reset/
  CleanAll/AllTreasure/EndGoal implemented; ShowSecret and the Trial toggle are documented
  gaps/no-ops).
- 93/93 real sound channels.

### What does not work / is not implemented
- **No visible 3D Blupi model** — invisible collision point in first-person, placeholder model in
  third-person. Blocked on the user providing a real model/rig; this is the single largest
  remaining gap (`plan.md E3D-MIG-069`).
- **Saw blade (icon 378) render orientation is still wrong** after 3 fix attempts — see §4.
- **No 3D world editor** — worlds are still hand-authored by editing
  `tools/GenerateSampleWorld3D.cpp` (`plan.md` §6, planned, not started).
- Several real buttons render at their correct position/icon/label but are intentionally inert —
  no meaningful desktop equivalent exists yet: `SetupJump`/`SetupZoom`/`SetupAccel` (touch-side/
  auto-zoom/accelerometer settings), `PauseBack` (hub-level navigation, no hub system exists),
  `InitRanking`/`InitBuy` (real gate resolves to "never shown by default" anyway), Init's own
  semi-transparent gamer-slot background panels (`MENU-014/015`, pure cosmetic).
- Cheat3 "ShowSecret" is a documented gap — its real citation conflicts with this engine's own
  already-confirmed `ObjectType12` (crate); deliberately not guessed through.
- Idle "fidget" periodic sounds (`plan.md #085`) — blocked on new `AnimState` values this engine
  doesn't have yet (same prerequisite as the 3D Blupi model work).
- `GalaxyEggbertSimple3D` — historical reference only, not buildable/maintained (see above).

## 3. Recent changes

Most recent first. Full history: `git log`. Everything below is from **2026-07-13** (one very
long session); each item is its own commit.

- **Fixed 3D-world render bleed-through during Wait/Init** — reported live by the user: at game
  start, the 3D terrain/background was visible in the pillarbox margins around (and, on
  non-4:3-aspect windows, alongside) the Wait loading-gauge screen and Init's gamer-select menu.
  Root cause: `GalaxyEggbertCnaGame::Draw()` always rendered the full 3D world (background/
  terrain/billboards) every frame regardless of phase; Wait/Init's own 2D UI only covers the
  centered 640×480 reference area, leaving the wider-aspect pillarbox bars showing whatever 3D
  content was drawn underneath. Fixed by skipping the entire 3D-world render block for exactly
  `Wait`/`Init` (the only two phases with no real game world to show — Wait is a fake progress
  screen since `LoadContent()` already loaded everything up front, and Init is the gamer-select
  menu entered before any level is being played); `Pause`/`Win`/`Lost`/`PlaySetup`/`MainSetup`/
  `Resume` are deliberately untouched since real mobile-eggbert legitimately freezes and shows the
  in-progress game world behind those overlays. Verified via live headless screenshots of both
  phases (temporary `kWaitDurationSeconds` debug override to reach Init immediately, reverted
  before commit) + full 7-tool regression suite. File:
  `src/GalaxyEggbertCNA/GalaxyEggbertCnaGame.cpp` (`Draw()`).
- **Cloud secret-power electric aura** (`plan.md #068`) — while `SecretPower::Cloud` is active,
  destroys small enemies (Bulldozer/blupih/blupit) within a real 40px aura, real sound channel 59.
- **Fall-death timing halved** — `kFallDeathY` -60.0f→-27.0f per user feedback (was ~6.7s, now
  ~3.4s); a game-feel tweak, not a faithfulness correction.
- **Real fade-out phase transitions** (`plan.md MENU-088/089`) — only `Init`/`MainSetup`/
  `PlaySetup`/`Pause`/`Resume` ever defer a transition (~1.0s exit animation); every other phase
  transition is a real instant cut. Also added Pause/Resume's own 0.75s entrance flourish and
  MainSetup/PlaySetup's speedyblupi slide-in + rotating gear decorations. A real bug (infinite
  re-defer loop from clearing `fadeOutPhase_` before recommitting) was caught and fixed during live
  verification.
- **Wait/Init boot flow + gamer-select menu** (`plan.md MENU-001..020`) — engine now starts in
  `Wait` (progress gauge) instead of directly in `Play`; `Init` renders a real 3-gamer-slot
  select screen. `GESaveData` extended from 1 implicit slot to 3 independent slots.
- **Hidden cheat menu** (`plan.md CHEAT-001..009`/`MENU-092..102`) — corrected several wrong
  draft descriptions against real source (e.g. "LayEgg" sets lives=9, doesn't spawn an egg;
  gesture is a 10-tap sequence, not 6).
- **Tank bullet firing** (`plan.md BULLET-001`) — confirmed real bullets are never a weapon
  against enemies (bullet-vs-Blupi contact only); dedicated "F" key, Tank-only, real
  cooldown/ammo gates.
- **Resume phase + save data extended to lives/mission** (`plan.md MENU-040..045`).
- **Minimal settings persistence** (`plan.md MENU-067`) — first version of `GESaveData`.
- **PlaySetup (settings) screen** (`plan.md MENU-058..069`).
- **Win/Lost screens + Pause button text labels** (`plan.md MENU-046..057`).
- **Pause screen + functional on-screen Play controls** (`plan.md MENU-021..027/028..039`) — new
  `GEInputPad` class, a mouse-driven port of the real `InputPad`.
- **Real `Def::Phase` state machine** (`plan.md HUD-023`) — the full real 13-value enum wired up,
  simulation gated behind `Phase::Play` exactly like the real `Decor::MoveStep()`.
- **Phase 9 (HUD) completed** — training-hint overlay (`HUD-024`), Perso decoy mechanic + counter
  (`HUD-017`), both `jauge.png` gauges (`HUD-008/012/018/019`), bullet/dynamite counters
  (`HUD-015/016`). A full read-through of the real `Decor::DrawInfo` confirmed every element it
  draws is now implemented.

**Earlier (before 2026-07-13, still true):** Phases 13–17 (enemy AI, hazards, crates/lifts/bridges,
doors & keys, secret powers/vehicles/buffs) all substantially/fully complete (2026-07-11/12) — see
`plan.md` for the itemized per-phase checklists. Terrain-tile identification, all 4 render modes,
face culling, real background images, third-person placeholder camera, `MoveObject`/`BigDecor`
billboard rendering, and the CNA target skeleton itself were built 2026-07-01 through 2026-07-10 —
see `git log` for the full commit-by-commit history of that earlier work.

## 4. Current blocker / main problem

**There is no build- or test-breaking blocker right now.** Both graphics backends build clean and
all 7 automated test/verify tools pass as of the last commit (2026-07-13, `63e020d`).

The one open, paused item is the **Saw blade's render orientation** (icon 378, `InnerFlatPlate`
render mode) — wrong after 3 fix attempts across live user feedback rounds. Per explicit user
direction (2026-07-12): **do not attempt a 4th blind fix** — this needs the user's own direct
visual judgment (a screenshot/crop review), not another autonomous guess. A new, not-yet-acted-on
finding exists: the real icon 378 texture's actual content occupies only the bottom half of its
64×64 tile, and there's an unverified theory that `Easy3D::AppendPlateMesh` vertically flips
whatever texture it's given. This is purely a visual/UV-mapping issue — it does not affect any
test or the build.

A separate, deeper architectural limitation was found (not fixed, not currently blocking anything):
`GEBlupiController::GroundHeightAt()` resolves a column's floor as the single topmost solid block
in that column, with no concept of "nearest solid surface at or below Blupi's own height." Any
roofed/enclosed interior with a ceiling above an open floor is unreachable via normal walking or a
raw `SetPosition()` into it (the ceiling gets misread as the floor). Hit twice this project so far
(the fan-hazard task, then Saw verification), both times worked around by placing the affected
demo geometry in open-sky rooms instead of fixing the root cause. Worth fixing properly before a
third feature needs to walk-test something inside an enclosed space — see §5.

## 5. Known bugs and limitations

| Status | Issue |
|---|---|
| **open, needs_human — do not guess again** | Saw blade (icon 378) render orientation still wrong; see §4. |
| **real architectural limitation, not fixed** | `GEBlupiController::GroundHeightAt()` misreads a ceiling as the floor for any roofed/enclosed interior — see §4. Workaround so far: avoid placing new hazards/features inside enclosed spaces. |
| **CNA upstream bug, worked around — needs approval to fix upstream** | CNA's Vulkan backend records all `SpriteBatch` draws before all 3D draws each frame, so a sprite HUD drawn after the 3D scene gets painted over. `galaxy-eggbert` no longer uses `SpriteBatch` (`GEHud` draws real 3D quads instead), so it's unaffected — but any future `SpriteBatch` use would be. |
| **CNA quirk, worked around** | A `BasicEffect` draw with `Alpha < 1` renders on EasyGL but not at all on CNA's Vulkan backend. `GEHud`'s treasure panel uses opacity 1.0 instead of the real mobile-eggbert 0.6 until this is fixed upstream (`kPanelOpacity`). |
| **accepted limitation, user declined a fix (2026-07-09)** | `GalaxyEggbertCNA` exits with code 1 (not 0) when closed via a real window-manager close request — root cause is inside SDL's own X11 teardown (`../cna`/SDL), needs explicit approval to fix. Do not attempt without new approval. |
| **known, deliberately out of scope for now** | The pillarbox-margin 3D-world bleed-through fixed for Wait/Init (§3, 2026-07-13) still exists for `Pause`/`Win`/`Lost`/`PlaySetup`/`MainSetup`/`Resume` on non-4:3-aspect windows — NOT a bug for those phases (real mobile-eggbert legitimately shows the frozen game world behind them), but the pillarbox bars themselves (outside the centered 640×480 reference area) are unfilled letterboxing, a separate minor cosmetic gap from the one just fixed. Not reported by the user; only fix if asked. |
| **needs re-test** | The "texture distance washout" investigation (`texture-distance-washout-bug.md`) was conducted while an unrelated cube-winding bug was active; its "geometry proven correct" conclusion is void and the repro should be re-run on the current code before resuming. |
| **needs investigation** | Under Vulkan specifically, `screenshot_hud.png` (an end-of-frame diagnostic) has shown a plain blue background with un-blended white boxes around billboards, even though the same frame's terrain-visibility check reports real terrain color. Found 2026-07-11, not investigated — may be a `GetBackBufferData`/swapchain timing quirk. EasyGL's equivalent screenshot is unaffected. |
| **not reproduced via scripted collision test** | "Grass-topped cubes reported walkable-through" — a new `VerifyBlupiMovement` check (2026-07-13) drops Blupi onto all 9 icon-107/108/109 blocks in `worlds3d/world001.vwr`; all land correctly (`IsSolidAt()` is independent of which faces a render mode draws). Collision logic itself is not at fault for this world's blocks. Possible explanation: icon 107/108/109 intentionally leave their `PosY` face un-rendered (drawn instead by the separate `grass_top.png` overlay plate), which may visually read as an open hole from some camera angles even though the block is solid — an optical-illusion theory, not confirmed. Still needs a live repro with specific coordinates if the report recurs. |
| **risky assumption** | World/texture loading uses relative paths — `GalaxyEggbertCNA` only works when run from its own build directory. |
| **incomplete** | `GETerrainRenderer`'s face culling only covers the plain static `UniformCube` path — animated/water paths render every face unconditionally (fine while those are sparse decorative elements, not bulk fills). |
| **incomplete** | No per-zone fog (Simple3D only had `SetClearColor` per sky region; not evaluated for CNA). |
| **not to be fixed (per user, 2026-07-08)** | `GalaxyEggbertSimple3D` build broken (missing U3D prebuilt) — historical reference only, do not spend effort on it. |

## 6. Architecture notes

### Main modules
```
include/GalaxyEggbert/Worlds/, src/GalaxyEggbert/Worlds/
    Engine-agnostic voxel World (100×100 grid), Block/Chunk, .vwr save format. Shared by both
    targets. Chunk's sparse ChunkBlockMetadataRecord mechanism stores arbitrary per-block payloads
    (used by MoveObjectRecord below).

include/GalaxyEggbert/MoveObjectRecord.hpp, src/GalaxyEggbert/MoveObjectRecord.cpp
    Engine-agnostic: encodes/decodes a MoveObject (pickup/enemy/lift/crate) as a World
    block-extra-metadata payload, anchored at floor(posStart) in World's own raw grid space.
    PlaceMoveObject()/CollectMoveObjects().

include/GalaxyEggbert/BlockTypes.hpp
    Tile type constants; block type = icon index in object-m.png. Ground/StoneA/StoneB are
    CONFIRMED MISLABELED (machine-piece graphics, not terrain) — do not use for new bulk fills;
    use RockPile/BrickWall or another questionnaire-confirmed-genuine icon instead.

src/GalaxyEggbertSimple3D/
    Historical reference only as of 2026-07-08 — not built/maintained. Do not mutate into the CNA
    implementation.

src/GalaxyEggbertCNA/
    GalaxyEggbertCnaGame — owns GEWorldRuntime, GETileAtlas, GETerrainRenderer, GEBlupiController,
    GEInteractionSystem, GEObjectIcons, GEHud, GESound, GEInputPad, GESaveData, an Easy3D::Camera3D
    (first-person) + third-person AvatarRenderer toggle. Also owns bigDecorCells_/objectCubeEffect_
    for BigDecor/platform-lift/crate rendering.

    GETerrainRenderer: per block, one shared AppendSpecialGeometry() helper checks 4 special
    render-mode tables in order (DirectionalCube, InnerPillarBox, InnerFlatPlate,
    TripleCrossBillboard), falling back to plain UniformCube. 5 CubeMeshRenderer-backed meshes:
    static opaque (face-culled), static-but-transparent, grass-top overlay, animated opaque, water.

    GEInteractionSystem: platform lift/crate/bridge patrol+push, all pickups, all 5 terrain
    hazards, enemy AI/combat (shared kill-list, wasp balloon, blupih/blupit projectiles, large
    creature, follower), secret-power pickups/effects (including the Cloud aura), bullet firing,
    cheat effects (CheatOpenDoors/CheatCleanAll/CheatAllTreasure/CheatFindExit).

    GEBlupiController: movement/gravity/collision, vehicle modes, secret-power state, crouch/crush/
    balloon/teleport states.

    GEInputPad: mouse-driven port of the real InputPad — Play D-pad/Jump/Action/Pause,
    Pause/Setup/Resume/Init/WinLost screens, the hidden cheat gesture+menu overlay. Real
    drawBounds-relative button rects don't fit this engine's fixed 640×480 reference space — every
    rect here is a proportionally-adapted layout, not a literal port (except a few cases that
    coincidentally need zero adaptation at this engine's own 480 reference height).

    GESaveData: plain key=value text file, NOT byte-compatible with real GameData. soundEnabled
    (global) + 3 independent GamerSlot{lives, missionNumber, hasProgress} + selectedGamer index.

tools/GenerateSampleWorld3D.cpp
    Builds worlds3d/world001.vwr — the only hand-authored demo world. No 3D world editor exists;
    this is how new terrain/objects get added today.

mobile-eggbert-reference/
    Prose catalogs: 02-tiles.md (441 icons), ObjectType/sound/animation catalogs, a gameplay-
    behavior spec (10-blupi-mechanics.md etc.) — approved as documentation, not a license to copy
    code/data into galaxy-eggbert without separate approval each time.

../easy-3d/
    Companion library beside CNA. Standing permission to modify (small, generic 3D-batching
    helpers only). CubeMesh/CubeMeshRenderer, BillboardMesh/BillboardMeshRenderer.
```

### Important invariants
- Block type = icon index in `object-m.png` (20 cols, 64×64px tiles, 1px gap + 1px leading margin).
- World grid is 100×100; `GEWorldRuntime` defines a 50-offset world-center constant.
- `ObjectType` (204 IDs) and `SoundChannel` (93 IDs) are numerically identical to mobile-eggbert's
  — do not renumber.
- No `#ifdef GE_ENGINE_*` anywhere — each target speaks its own API directly.
- `BlockTypes::Ground`/`StoneA`/`StoneB` must not be used for new bulk-terrain fills (confirmed
  mislabeled). Only add a new icon to any `GE*Tiles.cpp` table with its exact confirmed answer from
  the questionnaire files in `mobile-eggbert-reference/` — never guess a render mode/face config.
- `PlateItem`/`TripleCrossItem`/`DirectionalCubeItem` are the only "special geometry" primitives —
  add a new one only for a genuinely new shape, not as a `GETerrainRenderer` workaround.
- The 5 real deferring phases (`Init`/`MainSetup`/`PlaySetup`/`Pause`/`Resume`) and their exact
  fade formulas are load-bearing for `GalaxyEggbertCnaGame::SetPhase()`/`Update()` — see that
  method's own class comment before changing phase-transition logic.
- Build with `-j2` maximum (32 GB RAM constraint; crashes observed with more parallel jobs).

### Boundaries that must remain stable
- `BlockTypes::fromMobileIconId()` — must match the mobile-eggbert world file format exactly.
- `ObjectType`/`SoundChannel` numeric values — stored in level files, must never be renumbered.
- `src/GalaxyEggbertSimple3D/` must not be mutated into the CNA implementation.
- mobile-eggbert stays read-only, never modified even temporarily; no code/data copied from it
  without explicit user approval per use.
- `../cna` and `../simple-3d` are sibling repos: read freely, modify only with explicit per-change
  approval. `../easy-3d` has standing permission (scoped to small, generic 3D-batching helpers).

## 7. Useful commands

```bash
# Configure + build GalaxyEggbertCNA, EasyGL backend (default):
cmake -S . -B build-cna -DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF
cmake --build build-cna --target GalaxyEggbertCNA -j2
cd build-cna && ./GalaxyEggbertCNA        # must run from its own build dir (relative asset paths)

# Configure + build GalaxyEggbertCNA, Vulkan backend:
cmake -S . -B build-cna-vulkan -DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF -DCNA_GRAPHICS_BACKEND=VULKAN
cmake --build build-cna-vulkan --target GalaxyEggbertCNA -j2

# Regenerate the hand-authored 3D sample world (after tools/GenerateSampleWorld3D.cpp changes):
cmake --build build-cna --target GenerateSampleWorld3D -j2
./build-cna/GenerateSampleWorld3D worlds3d/world001.vwr

# Build + run all tests/verify tools (run from repo ROOT, not build-cna — several tools use
# ../mobile-eggbert-relative paths that only resolve correctly from the root):
cmake --build build-cna --target GalaxyEggbertWorldsTests VerifyBlupiMovement VerifyInteractionSystem \
    VerifyGEInputPad VerifyGESaveData VerifyMoveObjectTypesCna VerifyBigDecorParsingCna -j2
./build-cna/GalaxyEggbertWorldsTests
./build-cna/VerifyBlupiMovement
./build-cna/VerifyInteractionSystem
./build-cna/VerifyGEInputPad
./build-cna/VerifyGESaveData
./build-cna/VerifyMoveObjectTypesCna
./build-cna/VerifyBigDecorParsingCna

# easy-3d: CNA-linked build + tests
cmake -S ../easy-3d -B /tmp/e3d-build-cna -DEASY3D_LINK_CNA=ON -DEASY3D_CNA_BACKEND=EASY_GL -DEASY3D_CNA_DIR=../cna
cmake --build /tmp/e3d-build-cna -j2 && ctest --test-dir /tmp/e3d-build-cna --output-on-failure

# Reference: mobile-eggbert animation tables / gameplay logic (read-only, never modify)
grep -n "table_decor\|table_blupi" ../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Tables.cpp
less ../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Decor.cpp

# GalaxyEggbertSimple3D — historical reference only, NOT to be built/fixed (shown for completeness):
# cmake -S . -B cmake-build-debug -DGALAXY_EGGBERT_BUILD_SIMPLE3D=ON
# cmake --build cmake-build-debug --target GalaxyEggbertSimple3D -j2
```

No `.clang-format`/`.clang-tidy` config exists in this repo — no lint/format tooling to run.

## 8. Next smallest tasks

1. **Resolve the Saw blade (icon 378) render orientation** — needs the user's own visual judgment
   first (a fresh screenshot/crop review), not another blind guess. Once the user confirms the
   correct orientation: adjust `GETerrainRenderer`'s `InnerFlatPlate` UV/rotation handling for this
   icon (check the "vertical flip in `AppendPlateMesh`" theory noted in §4 first).
   Files: `src/GalaxyEggbertCNA/Game/GETerrainRenderer.cpp`, `../easy-3d`'s `CubeMesh.cpp`
   (`AppendPlateMesh`). Verify: live headless screenshot at the Saw demo block + `VerifyBlupiMovement`.

2. **Re-run the texture-distance-washout repro on current code.** The original investigation
   (`texture-distance-washout-bug.md`) predates the cube-winding fix and its conclusions are void.
   Files: read `texture-distance-washout-bug.md` first, then reproduce with a live headless
   screenshot at ~12 units distance from a textured wall. Verify: visual comparison, update the doc
   with a fresh conclusion either way.

3. **Investigate the Vulkan-only `screenshot_hud.png` blue-background anomaly** (§5). Files:
   `src/GalaxyEggbertCNA/GalaxyEggbertCnaGame.cpp` (the screenshot capture code),
   `cna_backend_graphics_vulkan`. Verify: compare `GetBackBufferData` output between EasyGL and
   Vulkan builds at the same frame.

4. ~~Try to reproduce "grass-topped cubes walkable through" with a scripted test.~~ **DONE
   2026-07-13** — added a `VerifyBlupiMovement` check that drops Blupi onto all 9 icon-107/108/109
   blocks in `worlds3d/world001.vwr`; all 9 land correctly, not reproduced via scripted collision.
   See §5 for the updated conclusion; still open if a live repro with coordinates ever recurs.

5. **Fix `GEBlupiController::GroundHeightAt()`'s roofed-interior limitation** (§4/§5) — make it
   consider Blupi's own current Y instead of always using the column's topmost solid block. Scope
   carefully: this could subtly affect every other already-shipped hazard/mechanic's collision, so
   needs a full regression pass (`VerifyBlupiMovement`, `VerifyInteractionSystem`) after the change,
   not just a walk-test of the specific tunnel that surfaced it. Files:
   `src/GalaxyEggbertCNA/Game/GEBlupiController.cpp`/`.hpp`.

6. ~~Add Init's semi-transparent gamer-slot background panels~~ **DONE 2026-07-13**
   (`plan.md MENU-014/015`) — `GEInputPad::DrawInit()` now draws a `pad.png` icon-15 panel (same
   convention as `GEHud`'s `DrawInfo` panel) behind each gamer row and behind the Setup/Play
   buttons. Drawn at opacity 1.0, not true transparency, per the same CNA/Vulkan `BasicEffect`
   Alpha<1 workaround already used by `GEHud::kPanelOpacity` (§5). Verified via a live headless
   EasyGL screenshot (temporary `kWaitDurationSeconds` debug override to reach Init immediately,
   reverted before commit) + full 7-tool suite.

Each remaining task above is independently small and verifiable; do them in any order except #5,
which should wait until nothing more urgent depends on the current (wrong-but-stable) collision
behavior.

## 9. Do not do yet

- **Do not guess at the Saw blade orientation a 4th time without the user's own visual input** —
  3 prior autonomous attempts were wrong; this specifically needs human judgment.
- **Do not build, fix, or troubleshoot `GalaxyEggbertSimple3D`** — historical reference only, per
  explicit user directive (2026-07-08).
- **Do not modify `../mobile-eggbert`**, not even temporarily — copy files out first if a working
  copy is genuinely needed.
- **Do not copy mobile-eggbert code/data (tables, enums, byte layouts) into galaxy-eggbert**
  without explicit user approval, even when it looks like "just data."
- **Do not invent new gameplay mechanics.** Every feature must trace to something confirmed in
  mobile-eggbert — verify against real source or `mobile-eggbert-reference/` before implementing,
  not just plausibility.
- **Do not modify `../cna` or `../simple-3d`** without explicit per-change approval (the exit-code-1
  and Vulkan SpriteBatch-ordering bugs in §5 both need this and were explicitly deferred).
- **Do not attempt the exit-code-1-on-window-close fix** — the user already explicitly declined it.
- **Do not design or build the real 3D Blupi model** — blocked on the user providing an asset;
  don't speculate about its look/rig in the meantime.
- **Do not broadly refactor `GEInputPad`/`GEInteractionSystem`/`GEBlupiController`** — they are
  large, working, and have many call sites; prefer small, targeted, well-tested additions over
  restructuring.
- **Do not re-inflate this file back into a multi-thousand-line narrative.** Keep §3 to the current
  session plus a one-paragraph "earlier, still true" summary; rely on `git log`/`plan.md` for full
  historical detail.

## 10. Resume prompt

```
Read NEXT.md first (this file). Pick the FIRST unstarted item in §8 "Next smallest tasks" (or the
one the user names) and inspect only the files it lists — do not read or refactor unrelated code.
Make one small, verified improvement: implement it, run the verification command the task
specifies, and confirm it actually passes (don't just assume). Do not touch `GalaxyEggbertSimple3D`,
`../mobile-eggbert`, `../cna`, or `../simple-3d` unless the task explicitly requires it and you have
explicit approval. When done, update NEXT.md: move the completed task out of §8, add one factual
bullet to the top of §3 (dated), and update §2 if the change affects build/test status. Keep the
update concise — do not restore removed historical detail.
```
