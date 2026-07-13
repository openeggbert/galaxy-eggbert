# NEXT.md — Galaxy Eggbert

## 1. Project summary

**Galaxy Eggbert** is a faithful 3D remake of **mobile-eggbert** (a C++ port of *Speedy Blupi*,
a Windows Phone XNA game from 2013). Main goal: reimplement the same game logic, levels, and assets
in 3D — perspective camera, billboard sprites, 3D-rendered tiles — without inventing new mechanics
(see `CLAUDE.md`'s "Faithful Remake" rule).

**Current development phase:** mid-migration between two build targets:

- `GalaxyEggbertSimple3D` (built on `simple-3d` → U3D/Urho3D) — historical reference only as of
  2026-07-08 (per user): feature-complete enough to play through core mechanics end-to-end, but no
  longer built/fixed/verified going forward (see §2, §9). Its build is currently broken in this
  environment (missing/incompatible U3D prebuilt) and that is intentionally left unfixed.
- `GalaxyEggbertCNA` (built directly on **CNA** + **Easy3D** helper library) — the **new
  long-term target**, opt-in and pre-parity. Opens a window, loads a genuinely 3D hand-authored
  `.vwr` world (`worlds3d/world001.vwr`), renders real textured/animated terrain (one cube per
  non-air cell), moves an invisible collision-only Blupi with tank controls (plus mouse/touch
  drag-look and F11 fullscreen, 2026-07-10), and renders `MoveObject`s and `BigDecor:` cells
  (pickups/enemies/decor) as real textured billboards (plus `UniformCube`s for platform
  lifts/crates). **MoveObjects can now be embedded directly in the `.vwr` format itself**
  (2026-07-09, §3) — no mobile-eggbert `.txt` file needed to see any of them render. **As of
  2026-07-10/11, `GalaxyEggbertCNA` also has: a real mobile-eggbert-faithful 3D-quad HUD
  (`GEHud`, lives/keys/treasure), sound (93/93 channels), and a growing real interactive-object/
  gameplay system** (`GEInteractionSystem`/`GEBlupiController`/`GEWorldRuntime`) covering pickup
  collection, platform-lift/crate patrol+push, all 5 real terrain hazards (lava/spikes/blitz/
  saw+switches/crusher), the real shared enemy kill-list (8 types), the wasp's balloon status, the
  real shared patrol-turn state machine, blupih/blupit's projectile attacks, the large
  creature's turn-dwell-gated grab, and follower wake+homing — **Phase 13 (Enemy AI & combat) is
  now fully complete**; see §2/§3 for detail and `plan.md` Phase 14 for what's still open. No 3D
  Blupi model yet (still an invisible collision point).

**Important architectural decisions:**

- **Direct CNA + Easy3D is the sole long-term target (locked 2026-07-05).** Galaxy Eggbert will
  run only on CNA long-term. `GalaxyEggbertSimple3D` is transitional — scheduled for **gradual
  removal** as `GalaxyEggbertCNA` reaches parity, piece by piece, not kept indefinitely as
  reference. Its code stays intact (no removal without an explicit task), but as of 2026-07-08 it
  is historical reference only — not built/fixed/verified going forward (see §9).
- **No Blupi 3D model exists yet; the user will provide one later.** Interim stopgap in
  `GalaxyEggbertCNA`: a first-person camera (no third-person view is possible without a visible
  character) plus a small 2D animation-state indicator (idle/walk/jump/crouch/look-up) in the
  screen's bottom-right corner, reusing existing 2D sprite frames. Not a design decision about the
  eventual 3D Blupi's look.
- Easy3D is a small helper library beside CNA (cameras, texture atlas, batching, mesh building,
  renderer adapters) — must not hide CNA; game code may call CNA directly. Standing permission to
  modify `../easy-3d` (granted 2026-07-06) — no longer needs per-change approval, but stays scoped
  to small, generic 3D-batching helpers (no scene graph/ECS/engine creep).
- `../mobile-eggbert` is **read-only, never modified even temporarily**. Its assets (PNGs, sounds,
  world files) are freely reused by direct path/build-time copy; its code/data (`Decor.cpp`,
  tables, enum values, byte layouts) is reference-only and requires explicit approval to copy — see
  `CLAUDE.md`'s reuse table. It has no CMake library target (only `add_executable`), so it cannot
  be linked as a dependency today.
- No automated `.txt → .vwr` 2D-to-3D world converter (rejected direction). `GalaxyEggbertCNA`
  defaults to a hand-authored `.vwr` world; `LoadFromMobileEggbertFile` is a secondary/reference
  path only.
- **Terrain-tile identification is now COMPLETE (2026-07-08) — this was the multi-session "active
  thread" and it is finished.** All 441 `object-m.png` icons are accounted for: 314
  named/behavioral tiles (round 1: 34 icons + round 2: 280 icons, both via direct user Q&A) and
  97 of the 127 remaining "unused" icons (round 3, also direct user Q&A) — the other 30 are icon 0
  (`Air`), icon 440 (confirmed no real pixel data), and animation sub-frames that inherit their
  base icon's identity (Lava/Fan/Crusher/Temp/Saw/Water2 groups). **The `DirectionalCube` render
  mode is now implemented and wired up for ALL ~99 confirmed icons (2026-07-08/09, §3, completed
  2026-07-09)** — icon 200/`Platform`, 48 "4 sides + top/bottom color-or-open" icons, the 4 fan
  tiles, icons 30/31 (also "4 sides + top/bottom color," but with real texture alpha, reusing the
  water render mode's alpha-blend state via a new static-but-transparent pass), icon 107 (top face
  left open here on purpose — its real top surface is a separate `grass_top.png` overlay, see
  below), 37 "1 or 2 of 4 sides textured" icons whose facing was read directly from their crop
  image (mobile-eggbert confirmed to have **no per-placement rotation metadata at all** — for any
  tile that varies by facing it just uses a different icon number, e.g.
  `FanLeft`/`FanRight`/`FanUp`/`FanDown` are 4 distinct icons; the questionnaire's "uloženo v
  metadatech bloku" phrasing was the answerer's own guess while eyeballing crops, not a real
  mobile-eggbert source finding), and the last 6 icons — 15-18 (axis + side choice, defaulted
  2026-07-09) and 108-109 (own texture + icon 107's texture + open side + grass top, defaulted
  2026-07-09) — both needed a facing decision their crops didn't resolve even after direct user
  review, so both use this session's established ambiguous-icon tie-break default (see §3).
  **`InnerPillarBox`/`InnerFlatPlate`/`TripleCrossBillboard` are now also implemented (2026-07-08,
  §3), all 76 confirmed icons across the 3 modes wired up** — `InnerPillarBox` reuses
  `DirectionalCubeItem` directly (a smaller box instead of a full-size cube); `InnerFlatPlate` and
  `TripleCrossBillboard` are 2 new `Easy3D` mesh builders (a double-sided plate; 3 double-sided
  planes 60° apart). All 4 confirmed render modes from the tile-identification work are now real
  code. **Water (icons 91-98) also now renders correctly (2026-07-08, §3)** — semi-transparent
  alpha-blended `UniformCube`s (the user's chosen design, resolving the last open render-mode
  question), not solid opaque cubes. **Icon 107 now has a real grass-top texture too (2026-07-08,
  §3)** — a procedurally generated, galaxy-eggbert-owned `textures3d/grass_top.png` (the user's
  chosen approach, since `object-m.png` can't be extended — it's re-copied fresh from
  `../mobile-eggbert` on every build), rendered via a new `GETerrainRenderer::DrawGrass()` pass
  with its own texture/effect. See §8 for what's left (mostly per-icon data-quality follow-ups,
  not new mechanisms).
- A real documentation bug was found and fixed during round 3: `02-tiles.md`'s "0/78 files, unused"
  claim only checked the static `Decor:`/`BigDecor:` grid, not `MoveObject:` records' own `icon=`
  field (which can reference the same `object-m.png` sheet). At least 32 of the "unused" icons are
  confirmed actually used this way (crates, water splash, breathing bubbles, a pickup animation,
  a conveyor-lift tread, bridge-build frames) — see §3 and `02-tiles.md`'s correction note.

## 2. Current status

### Build status
- `GalaxyEggbertCNA` — **last confirmed clean build+run today (2026-07-09)**, after implementing
  face culling for the static terrain path (§3 — 67788→23012 vertices on the default world, a real
  66% reduction), on top of 3D-format MoveObject storage + full `ObjectType` catalog population,
  the platform-lift/crate `UniformCube` object path, and `BigDecor:` billboard rendering, all
  earlier the same day. Built `GalaxyEggbertCNA`, `GalaxyEggbertWorldsTests`, `VerifyBlupiMovement`,
  `VerifyMoveObjectTypesCna`, and `VerifyBigDecorParsingCna` from `build-cna/` — all succeeded.
- `../easy-3d` — **CNA-linked build rebuilt and all 6/6 tests passed today (2026-07-08)**, after
  adding `AppendPlateMesh`/`PlateItem`/`AppendTripleCrossMesh`/`TripleCrossItem` to
  `CubeMesh.hpp/.cpp` (§3, on top of the earlier `AppendDirectionalCubeMesh` addition same day).
  Command: `cmake -S ../easy-3d -B /tmp/e3d-build-cna -DEASY3D_LINK_CNA=ON -DEASY3D_CNA_BACKEND=EASY_GL -DEASY3D_CNA_DIR=../cna && cmake --build /tmp/e3d-build-cna -j2 && ctest --test-dir /tmp/e3d-build-cna`.
- `GalaxyEggbertSimple3D` — **not maintained going forward (per user, 2026-07-08): treated as a
  historical reference only, not to be built/fixed/verified.** Its build was found broken in this
  environment today (`cmake -S . -B cmake-build-debug` fails at `find_package(Urho3D)` —
  `URHO3D_BASE_INCLUDE_DIR-NOTFOUND`, U3D prebuilt at
  `/rv/data/library/github.com/u3d-community/U3D/cmake-build-debug` missing/incompatible), but per
  this instruction that is **not to be fixed** — do not spend effort rebuilding, reconfiguring, or
  troubleshooting U3D/Simple3D. Today's `GEWorldRuntime.cpp` fallback-world edit (§3) is a
  syntactically trivial enum-constant swap and was not run-verified for this reason; that is
  accepted, not a gap to close.
- `GalaxyEggbertWorldsTests` — **61/61 passing (2026-07-09, up from 54, §3)**, built and run from
  `build-cna` (Simple3D-OFF, per §9). Prefer running the binary directly
  (`./build-cna/GalaxyEggbertWorldsTests`) over `ctest --test-dir build-cna`, which also discovers
  5 unrelated `easy-gl`/`meta-gl` dependency-subproject tests that show as "Not Run" (their
  executables were never built by this session's targeted builds) — running the binary directly
  avoids that noise entirely rather than needing a `ctest -I` range workaround. The old
  `cmake-build-debug` profile's separate `ctest` discovery issue
  (`GalaxyEggbertWorldsTests_NOT_BUILT`) was not re-checked (that tree configures Simple3D, which
  is not to be built per §9) — not a gap, since `build-cna` is now the correct tree to use anyway.

### Test status
- `GalaxyEggbertWorldsTests` — **61/61 passed today (2026-07-09)**, up from 54 — 7 new tests for
  the `World` block-extra-metadata API and `MoveObjectRecord` (§3), including a regression test
  for the `Chunk::isEmpty()` data-loss bug found and fixed the same session.
- `../easy-3d`'s CNA-linked suite — **6/6 passed today (2026-07-08)**, including the new
  `cube_mesh` cases for `AppendDirectionalCubeMesh` (all-visible-faces parity with
  `AppendCubeMesh`, the icon-200 4-side/no-top-bottom case, all-invisible produces nothing,
  per-face UV independence).
- `VerifyBlupiMovement` — **run today (three times), ALL CHECKS PASSED** every time: after the
  Platform/Ground fix, and again after adding the `DirectionalCube` demo grate to
  `worlds3d/world001.vwr` (spawn, staircase step-up climb, standing on platform, wall collision,
  gravity/fall). Confirms the new floating grate (placed off the tested path on purpose, see §3)
  didn't affect collision.
- `VerifyMoveObjectTypesCna` — built today, not run this session (no MoveObject-parsing code
  changed, so not expected to differ from its last known-good state).
- A live `GalaxyEggbertCNA` run (5s, headless-terminated) after adding the `DirectionalCube` demo
  grate confirmed: loads `worlds3d/world001.vwr` (2754 non-air blocks, up from 2729 — the +25-block
  grate, Y range [0,13] unchanged), uploads the terrain mesh (65896 vertices/32948 triangles — up
  from 65496/32748 by exactly 400 vertices/200 triangles, i.e. 25 blocks × 4 visible faces × 16
  vertices/8 triangles each instead of 6 faces × 24/12 — deterministic proof the top/bottom faces
  were genuinely omitted, not just untextured), and a saved `screenshot.png` visually confirms the
  grate: real texture on its 4 side faces, sky-blue visible straight through the open top/bottom.
- **Re-run after backfilling 48 more `DirectionalCube` icons** (same session, §3): `VerifyBlupiMovement`
  all-pass again against the further-regenerated world (2784 blocks, +30 over the grate-only
  version — two more 3×1×5 demo rows, icons 2 and 25). Live run's vertex/triangle counts
  (66556/33278) are exactly +660/+330 over the grate-only baseline — 15 icon-2 blocks × 6 visible
  faces (24 vertices/12 triangles each, both top/bottom flat-color) + 15 icon-25 blocks × 5 visible
  faces (20 vertices/10 triangles each, top color/bottom open) — again deterministic proof the
  table's per-icon face counts are exactly what's rendered. `screenshot.png` shows the icon-200
  grate (open top/bottom) next to icon 2 (solid-looking, swatch-colored top and sides) with
  visibly different treatments.
- **Re-run again after the fan/rotation batch** (same session, §3): `VerifyBlupiMovement` all-pass
  a third time against the further-regenerated world (2829 blocks, +45 — three more 3×1×5 demo
  rows, icons 126/392/49). Live run reports 15 animated blocks (the icon-126 fan row) and
  vertex/triangle counts (67576/33788) exactly +1020/+510 over the previous baseline — 15 icon-392
  + 15 icon-49 blocks × 6 faces each + 15 icon-126 blocks × 5 faces (only after fixing
  `GETerrainRenderer::Update()` to also check `GEDirectionalCubeTiles` for animated blocks, see
  §3) — deterministic proof the fan-animation-path fix and the new single-face/axis entries render
  exactly as specified.
- **Re-run a final time after implementing `InnerPillarBox`/`InnerFlatPlate`/`TripleCrossBillboard`**
  (same session, §3): `VerifyBlupiMovement` all-pass a fourth time against the further-regenerated
  world (2833 blocks, +4 — one demo block each for icons 76/384/77/53). Live run's vertex/triangle
  counts (67648/33824) are exactly +72/+36 over the previous baseline — icon 76 (`InnerPillarBox`,
  4 faces = 16v/8t) + icon 384 (`InnerPillarBox`, 6 faces = 24v/12t) + icon 77 (`InnerFlatPlate`,
  1 double-sided plate = 8v/4t) + icon 53 (`TripleCrossBillboard`, 3 double-sided planes = 24v/12t)
  — deterministic proof all 3 new modes render exactly the declared geometry. A temporary debug
  spawn repositioning (reverted before committing, not part of the diff) got a real close-up
  screenshot of all 4 new blocks: icon 76 shows as 2 thin visible pillar edges, icon 384 as a
  small solid box, icon 77 clearly shows its actual Y-signpost texture on the plate (confirms UV
  mapping isn't flipped/misaligned), and icon 53 shows the correct small round/teal shape matching
  its source crop.

### What works
- **`GalaxyEggbertSimple3D`**: unchanged this session — world loading from mobile-eggbert `.txt`,
  textured/animated 3D terrain, Blupi billboard with state machine, mobile-object billboards, crate
  push, platform patrol, hazard detection, enemy stomp, respawn invincibility, pickups, exit-gate
  logic, HUD, save/load (3 slots), 93-channel sound, 3rd-person orbit camera, 5 sky colors per
  region, camera shake on death/hazard.
- **`GalaxyEggbertCNA`**: loads `worlds3d/world001.vwr` by default, renders every Y layer as
  textured `UniformCube`s (animated tiles included) *or*, per-icon, one of 4 special render modes
  now implemented: `DirectionalCube` (92 of ~99 confirmed icons — 4 side faces always textured,
  top/bottom each independently open or a flat fallback color), `InnerPillarBox` (3 of 3 — outer
  cube fully transparent, a smaller inner box textured), `InnerFlatPlate` (63 of 63 — outer cube
  transparent, a single double-sided plate), `TripleCrossBillboard` (10 of 10 — 3 double-sided
  planes 60° apart through the block's center). Tank-control Blupi (invisible collision point),
  first-person camera + 2D animation-state HUD indicator, renders `MoveObject`s as billboards (or
  `UniformCube`s for platform lifts/crates) — sourced either from a mobile-eggbert `.txt` file's
  `MoveObject:` lines, or now embedded directly in the `.vwr` format itself via
  `GalaxyEggbert::MoveObjectRecord` (2026-07-09, §3). Also now writes `screenshot.png` next to the
  binary on its first rendered frame (`GalaxyEggbertCnaGame::Draw`'s existing one-shot debug block)
  for visual verification. Also now renders the real mobile-eggbert background image
  (`Content/backgrounds/decorNNN.png`) behind the scene, selected per-world via the new `.vwr` v2
  `skyRegion` header field (2026-07-09, §3) — not a derived/flat color, and not a full 3D skybox,
  just a large camera-facing backdrop plane; falls back to a flat clear color if the region has no
  real background file. Also now has a second, third-person camera mode ("C" to toggle,
  2026-07-09, §3) showing a real GPU-skinned 3D model via CNA's `AvatarRenderer` extension —
  currently a temporary CC0/CC-BY placeholder (`avatars3d/blupi_placeholder/`), not a real Blupi
  model yet. Now also has a real `Def::Phase` state machine (Play/Pause/Win/Lost/PlaySetup/Resume,
  2026-07-13, §3) with a real Pause screen (background/character/5 labeled buttons, Continue/
  Restart/Setup all functional), real Win/Lost screens (background + real pulsing/spin-in
  `blupiyoupie.png` animation + functional Return button), a real PlaySetup settings screen
  (background + 6 real buttons, Sounds mute toggle + Return functional), a real Resume screen
  (offered at startup after a checkpointed Win/Lost, restores saved lives on Continue), minimal
  cross-restart settings/progress persistence (`GESaveData` — sound on/off, lives, mission,
  checkpointed the same way the real `GameData` is, NOT byte-compatible with it), and functional
  on-screen D-pad/Jump/Action/Pause controls (mouse-driven, 2026-07-13, §3, `GEInputPad`) usable
  alongside keyboard input. Riding a Tank can now also fire a real bullet (`ObjectType23`, a
  dedicated "F" key, 2026-07-13, §3, plan.md `BULLET-001`) — a hazard identical to the
  already-modeled enemy-fired kind, never a weapon against enemies (confirmed real behavior, not
  an oversight).
- **Tile/object documentation**: `mobile-eggbert-reference/` — complete catalogs of all 441 tile
  icons (see §1), 204 `ObjectType`s, 93 sounds, 131 animation sequences, all backgrounds, plus a
  prose gameplay-behavior spec.
- **Sample world materials are now correct**: `worlds3d/world001.vwr`'s staircase (`RockPile`,
  icon 35) and walls/pillars (`BrickWall`, icon 261) use confirmed genuine bulk-material tiles
  instead of the mislabeled `StoneA`/`StoneB` (both turned out to be machine-piece graphics, not
  stone — see §3).

### What does not work yet
- `GalaxyEggbertCNA`: no real Blupi model yet (a temporary placeholder exists in third-person mode
  only, §3; billboard rendering explicitly rejected 2026-07-10, see §3/plan.md `E3D-MIG-063`).
  **Riding a moving platform lift now works (2026-07-12, §3, plan.md `E3D-MIG-152`/`154`)** —
  `GEInteractionSystem::IsRidingLift()`/`RideDeltaX/Z()`/`RideStandY()` + `GEBlupiController::
  RideLift()`. A first interactive-object system now exists (2026-07-10, §3, `GEInteractionSystem`):
  platform lift patrol movement + riding, crate push (now linked-stack aware, `150`), and
  treasure/egg/key/level-exit pickup collection
  (with the real mobile-eggbert sound + removal behavior) all genuinely work — see §3 for exactly
  what is/isn't covered. `BigDecor:` rendering and the platform-lift/crate `UniformCube` object
  path are implemented (2026-07-09, §3). Real sound playback exists (2026-07-10, §3, `GESound`) —
  the same 93 real mobile-eggbert WAV files. A basic lives foundation exists (2026-07-11, §3), and
  all 5 real terrain hazard tiles are now implemented: 4 lethal (fall-off-world death, lava,
  spikes, Blitz, saw — the last with a real switch-linking mechanic,
  `GEWorldRuntime::TryActivateSwitch()`), and Crusher (non-lethal, a squash state,
  `GEBlupiController::TriggerCrush()`/`IsEcrased()`). The real shared kill list now covers 8
  `ObjectType`s (2/3/4/16/17/20/96/97 — patrol hazards, bulldozer, spider, fish, bird, follower,
  2026-07-11 §3), and the wasp (44, 2026-07-11 §3) inflicts a non-lethal "balloon" status
  instead (`GEBlupiController::TriggerBalloon()`/`IsBallooned()`/`PopBalloon()`) that changes how
  4 of those 8 shared-kill types behave (pop instead of kill). Every `MoveObject` (except lifts/
  crates) now genuinely patrols via the real shared 4-phase dwell/advance/dwell/recede cycle
  (2026-07-11 §3, `AdvancePatrolStep()`) instead of sitting frozen — the real prerequisite
  blupih/blupit and the large creature needed for their own dwell-frame-timed attacks/lethality
  windows, and both are now implemented too (2026-07-11 §3): blupih/blupit fire real
  `ObjectType23` projectiles during turn-dwell (their own body is harmless), and the large
  creature (`ObjectType54`) is lethal only during its own turn-dwell (safe mid-walk), never
  destroyed itself, with real balloon immunity modeled. Follower 96/97's real
  dormant-until-a-padded-wake-box, then 1px/tick homing-toward-Blupi movement is also done
  (2026-07-11 §3) — **Phase 13 (Enemy AI & combat) is now fully complete.** HUD is now
  minimal icon-based only (2026-07-11, §3: life icons, key icons) — no text rendering exists, so
  no numeric treasure counter/score.
  No 3D world editor exists yet either (plan.md §6, `EDITOR-*`, planned
  2026-07-11, not started)
  — worlds are still hand-authored by editing `tools/GenerateSampleWorld3D.cpp`.
- **All 4 confirmed render modes are now implemented; ALL ~175 total confirmed icons across all
  4 are wired up** (99 `DirectionalCube` + 3 `InnerPillarBox` + 63 `InnerFlatPlate` + 10
  `TripleCrossBillboard`) — the last 6 `DirectionalCube` icons (15-18, 108-109) were backfilled
  2026-07-09 using this session's established ambiguous-icon default, since their crops didn't
  give a confident facing read even after direct user review (see §3). `InnerFlatPlate`'s 63 icons: 58 use the
  same fixed default axis/size, 5 (icons 368-372) now use a horizontal axis per a 2026-07-09 crop
  spot-check (§3) — all 63 crops have now been reviewed at least once (via a montage), not just a
  sample, though the size (not just axis) of any of the 58 defaults hasn't been independently
  double-checked per icon the way `DirectionalCube`'s icons were.
- Simple3D: build is currently broken in this environment (missing/incompatible U3D prebuilt, see
  §2) — **not to be fixed**, per user 2026-07-08: Simple3D is historical reference only now (§9).

## 3. Recent changes

Most recent first. Full history: `git log`.

- **Tank bullet firing implemented, closing the `E3D-MIG-175` bullet-pack follow-up (2026-07-13,
  plan.md `BULLET-001`), per explicit user request ("implementovat střelbu").** A dedicated research
  pass into `Decor.cpp` overturned an initial assumption: real bullets are **never a weapon against
  enemies** — an exhaustive search of all ~25 real `ObjectType23` references found no code where a
  bullet damages an enemy `MoveObject`. The only real collision consequence is bullet-vs-Blupi
  (already implemented from an earlier session's blupih/blupit work, `E3D-MIG-134`) — real bullets
  are a hazard, identical whether enemy- or player-fired. Real trigger: a dedicated `Fire` key
  (`KeyPressFlags::Fire`, NOT the Action button), this engine's own "F" keyboard pick, usable only
  while riding **Tank** (Helicopter's own firing branch wasn't independently confirmed to spawn a
  projectile, so it's deliberately NOT modeled — a documented gap). Real gates: a 0.5s cooldown
  (ticked every frame, reset only on an actual shot) and an ammo check that plays a real
  out-of-ammo click (no cooldown reset) instead of firing when empty. Reuses the SAME
  `MakeBullet()`/`SearchAirDistance()` helpers blupih/blupit's own fired shots already use — no new
  projectile-physics code, just a new spawn trigger. Added a Tank pickup + a nearby wall to the
  sample world (right next to the existing bullet-pack demo) so the full pickup → mount → fire →
  hit-wall loop is genuinely playable. Verified: 6 new `VerifyInteractionSystem` assertions
  (caught and fixed a real test-design bug along the way — a leftover synthetic bullet pack from an
  earlier test silently re-topped ammo every frame, initially producing 2 unrelated-looking
  failures plus a cascaded failure in a much later test) and a live two-stage headless check
  confirming a real in-flight projectile spawns at Blupi's fire position and travels toward a
  placed wall, with the cooldown correctly gating repeat-fire in the live game loop (not just the
  isolated test) — full regression suite green on both backends.

- **Resume phase implemented, save data extended to lives/mission (2026-07-13, plan.md
  `MENU-040..045`), per explicit user request to extend the just-approved minimal save system.**
  `GESaveData` gained `lives`/`missionNumber`/`hasProgress`, checkpointed at the exact real
  Win/Lost transition points (matching the real `MemorizeGamerProgress()` call sites already
  confirmed during the earlier save-system research). `GEInteractionSystem` gained `SetLives()` to
  restore a checkpointed count. `GEInputPad` gained `UpdateResume()`/`DrawResume()`, reusing
  Pause's own `pause.png`/`blupiyoupie.png` background+character draw (confirmed shared in the
  real source: `Game1.cpp`'s `case Phase::Pause: case Phase::Resume: BackgroundCache("pause");`)
  with its own distinct 2-button row (ResumeMenu icon 11 / ResumeContinue icon 10, independently
  re-derived real rects, not reused from Pause's own row). ResumeContinue is fully functional
  (restores checkpointed lives + respawns at origin, same simplification already established for
  PauseRestart/WinLostReturn — no real mid-level position/treasure/key state exists to restore).
  The real trigger for entering Resume (`Game1::OnActivated()`, a WP7 app-reactivation OS
  lifecycle event gated on a real serialized mid-level snapshot, `Decor::Current*()` — a separate,
  heavier mechanism than `GameData` itself) has no desktop equivalent at all and is far beyond this
  engine's single-`.vwr`-world scope to replicate faithfully; adapted to trigger at startup
  whenever a previous run's save shows `hasProgress==true` — a documented simplification of WHEN
  Resume appears, not of the screen/buttons themselves. Verified via 2 new `VerifyGEInputPad`
  checks (32 total), 1 new `VerifyGESaveData` check (7 total), a live headless screenshot of the
  real Resume screen, and — critically — a live TWO-PROCESS integration test: forced a Win
  checkpoint in one run (confirmed `savedata.txt` written with `hasProgress=1`), then confirmed a
  completely separate second run correctly started in `Phase::Resume` with the exact saved lives
  count restored, proving the full save → restart → resume cycle actually works end to end, not
  just each piece in isolation — full regression suite green on both backends.

- **Minimal settings persistence implemented (2026-07-13, plan.md `MENU-067`), per explicit user
  request to propose a save-system design before implementing it.** A dedicated research pass into
  the real mobile-eggbert `GameData.hpp`/`.cpp` confirmed: (1) it's a fixed 640-byte binary blob (3
  gamer slots × lives/last-world/200 door flags + global sound/jump/zoom/accel settings), read/
  written via WP7's `IsolatedStorageFile` (no desktop equivalent at all); (2) `Write()` fires two
  ways in the real source — automatically at Win/Lost (lives/doors) and manually right after every
  Setup-screen toggle press; (3) byte-compatibility with this format has zero payoff for
  GalaxyEggbertCNA, since its own `GEInteractionSystem`/`GEWorldRuntime` already model lives/
  treasure/doors completely differently (one hand-authored `.vwr` world, not 100+ levels/200 door
  flags) — no real save file could ever cross between the two engines. Proposed and (after user
  confirmation) implemented a deliberately minimal, NOT-byte-compatible alternative: new
  `GESaveData` (`src/GalaxyEggbertCNA/Game/GESaveData.hpp`/`.cpp`), a plain `key=value` text file
  (no new dependency — no JSON library exists in this project) persisting just `soundEnabled` (the
  one setting already wired to real behavior via `GESound::SetEnabled()`/`IsEnabled()`). Loaded
  once in `LoadContent()`; `Save()` called immediately after the SetupSounds toggle, matching the
  real source's own write-on-toggle-press behavior exactly (not a timer or continuous autosave).
  Verified via a new `VerifyGESaveData` tool (3 checks: default-true-when-missing, round-trip for
  both bool states) plus a live two-process test (toggle off + save in one run, confirmed loaded
  back correctly at the start of a second, separate run) — full regression suite green on both
  backends. Lives/mission fields, if ever added, would follow the same automatic-Win/Lost-
  checkpoint pattern confirmed in the real source, not continuous autosave.

- **PlaySetup (settings) screen implemented (2026-07-13, plan.md `MENU-058..069`), continuing
  autonomously right after the Win/Lost task below.** Wired Pause's real Setup button
  (previously inert) to `SetPhase(PlaySetup)` (confirmed via `Game1.cpp`'s real
  `PauseSetup -> SetPhase(PlaySetup)` dispatch), and extended `GEInputPad` with
  `UpdateSetup()`/`DrawSetup()`: real `setup.png` full-screen background + 6 real buttons.
  Discovered a rare case where the real `InputPad.cpp` rects need ZERO proportional adaptation:
  `bsf2 = drawBoundsHeight*140/480` is EXACTLY 140 at this engine's own 480 reference height, so
  every Setup button rect is used completely unadapted (unlike the Pause row's `bsf1`-based
  layout, which needed real proportional shrinking). SetupSounds is fully functional — its icon
  really SWAPS (13 on / 21 off, a different real "pressed" convention from every other button in
  this class) and it's wired to the pre-existing `GESound::SetEnabled()`/`IsEnabled()`, a
  genuinely meaningful desktop equivalent of the real sound mute toggle. SetupReturn is fully
  functional (real destination confirmed via `Game1.cpp`: always resumes Play in place here, since
  PlaySetup's own MainSetup/Init branch is unreachable). SetupJump/SetupZoom/SetupAccel/SetupReset
  render at their real positions/icons/real English labels but are intentionally inert — no
  meaningful desktop equivalent for jump-button-side/auto-zoom/accelerometer, and Reset needs
  GameData (doesn't exist) plus a real gamer letter/number this engine has no concept of, so its
  own label is deliberately omitted rather than invented. Added a new
  `GEInputPad::AppendLeftAlignedLabel()` (real `Text::DrawTextRightButton()` semantics, distinct
  from the Pause row's centered labels). Explicitly out of scope, documented rather than invented:
  the speedyblupi.png slide-in and 2 rotating gear.png decorations (pure cosmetic, real formulas
  fully researched and written down for a future pass if ever wanted) and GameData-backed settings
  persistence. Verified via 3 new `VerifyGEInputPad` checks (30 total) plus a live headless
  screenshot confirming the exact real layout/labels/toggle-icon state — full regression suite
  green on both backends.

- **Win/Lost screens implemented, real Pause button text labels added (2026-07-13, plan.md
  `MENU-046..057`), continuing autonomously right after the Pause/on-screen-controls task below.**
  Extended `GEInputPad` with `UpdateWinLost()`/`DrawWinLost()`: real `win.png`/`lost.png`
  full-screen backgrounds (confirmed exact 640×480) plus the real `blupiyoupie.png` animations,
  verified directly against `Game1.cpp`'s actual `Draw()` phase branches (not guessed) — Win
  pulses forever between 0.5x/1.5x native size (`sin(phaseTime/0.15s)/2+1`, real position
  (418,238), no rotation); Lost grows from nothing to native size once over a real 5s with a
  decaying 6-turn spin that converges to exactly 0° as it reaches full size — both share the real
  `WinLostReturn` button (icon 3, a distinct/bigger rect from `PlayPause`'s despite sharing the
  icon, independently re-derived from `InputPad.cpp`'s own `bsf1=drawBoundsHeight/5` formula).
  Real destination is `Init` (confirmed via `Game1.cpp`'s `WinLostReturn -> SetPhase(Init)`, which
  doesn't exist here) — reuses the same return-to-Play-at-spawn simplification already
  established for the keyboard path (HUD-023), not a new one. Added
  `GalaxyEggbertCnaGame::phaseTimeSeconds_`, a real `phaseTime` port (`Game1.hpp`'s own
  "phaseTime==0 is a SetPhase() postcondition" doc comment, `Game1.cpp:237`'s unconditional
  per-tick increment) expressed as elapsed seconds rather than a frame counter. While researching
  the real Win/Lost draw code, also found and fixed a real gap in the already-shipped Pause
  screen: real per-button text labels ("Home"/"Back"/"Setup"/"Restart"/"Continue" — the real
  `PauseMenu` button's EN text is "Home", not "Menu") were missing; added via a new
  `GEInputPad::AppendCenteredLabel()` (own `text.png` instance, same glyph-is-ASCII-code
  convention `GEHud` already uses). Searched `Game1.cpp` directly for any mission-time/score/
  lives-remaining text on the Win/Lost screens (`MENU-049`/`050`/`051`/`056`/`057`) and found
  none anywhere in the real source — treated as unconfirmed rather than invented, extending the
  same rigor already applied to the earlier "score" finding (HUD-009). Verified via 3 new
  `VerifyGEInputPad` checks (27 total) plus live headless screenshots at 3 animation timepoints
  (Pause labels; Win mid-pulse; Lost at phaseTime 1.0s showing partial size + ~302° rotation
  exactly matching the formula's prediction, and 5.5s showing full size + 0° rotation) — full
  regression suite green (64/64 unit tests, all verify tools, both backends).

- **Pause screen + functional on-screen Play controls implemented (2026-07-13, plan.md
  `MENU-021..027`/`028..039`), per explicit user request to build them "vizuálně i s funkcemi"
  (visually AND functionally).** New `GEInputPad` (`src/GalaxyEggbertCNA/Game/GEInputPad.hpp`/
  `.cpp`), a mouse-driven port of the real `InputPad` class:
  - **Play**: on-screen D-pad (real discrete {-1,0,+1}-per-axis drag, 20px threshold, current-
    drag-point tracking), Jump (real LEVEL-triggered — fires every frame the pointer is inside its
    rect while held, no release needed), Action and Pause (real EDGE/release-triggered —
    single-fire on release regardless of release position, as long as the press started on the
    button). All OR'd with existing keyboard input (LCtrl/Space/Escape) — both work
    simultaneously.
  - **Pause**: real `pause.png` full-screen background (confirmed exact 640×480 match for the
    reference space) + real `blupiyoupie.png` character art (410×380, centered at real position
    (418,190), static — the real scale/rotate-in animation is a documented simplification) + 5
    real `pad.png` buttons (Menu/Back/Setup/Restart/Continue) with real conditional visibility
    (`Back`: mission≠1; `Restart`: mission≠1 AND mission%10≠0). Continue/Restart are functionally
    wired (resume in place / reset-to-spawn-and-resume); Menu/Back/Setup render at their real
    position/icon but are intentionally inert (no menu/hub/settings screens exist yet).
  - Real `InputPad.cpp` button rects are expressed in actual `drawBounds` pixel space, a
    genuinely different convention from this engine's fixed 640×480 reference space — confirmed
    a literal port is geometrically impossible (the real Pause row alone would span off both
    edges of a 640-wide space). Every rect here is a proportionally-adapted layout preserving
    real order/relative placement/icon choices instead.
  - Resolved the input conflict with the pre-existing mouse drag-look camera: `UpdatePlay()`
    returns whether the press landed on a control, which now suppresses drag-look for that press.
  - `PhaseOverlayMessage()`'s "PAUSED" text and `GEHud::Draw()`'s normal HUD are now both skipped
    during Pause (superseded by the real screen above) — Win/Lost still use the generic overlay.
  - Verified via a new scripted tool (`tools/VerifyGEInputPad.cpp`, 24 checks, synthetic
    `MouseState` values, no `GraphicsDevice` needed) plus live headless screenshots on both
    backends (D-pad/Jump/Action/Pause render at expected screen positions in Play; the Pause
    screen's background/character/buttons render correctly, with `Restart` correctly hidden for
    mission 0 via the real `mission%10!=0` gate) — full regression suite green (64/64 unit tests,
    all verify tools, both backends).
  - Out of scope this pass (documented gaps, not oversights): `PlayDown` (`MENU-026`, no crouch
    use case identified yet), every animated transition (`MENU-029`/`038`/`039`), real level-reload
    for `PauseRestart`/hub-navigation for `PauseBack` (`MENU-035`/`036`, no infrastructure yet).

- **Real `Def::Phase` state machine implemented (2026-07-13, plan.md `HUD-023`), per explicit
  user request for the FULL real enum, not a trimmed subset.** `GalaxyEggbert::GamePhase`
  (already ported verbatim in an earlier session, previously unused anywhere) is now wired up:
  `GalaxyEggbertCnaGame` gained a real `phase_`/`SetPhase()`. The real simulation gate (confirmed
  via research: `Decor::MoveStep()` is the ONLY thing gated behind `Phase::Play` in the real
  source) is ported as a single early-return wrapping this engine's entire existing gameplay-
  update block. Escape toggles Play<->Pause (the real source has no keyboard binding at all — an
  XNA/WP7 port — this is this engine's own pick). Win/Lost triggers map exactly onto state
  already tracked: real Lost is precisely `GEInteractionSystem::GameOverCount()` incrementing,
  real Win is precisely `ExitReached()` becoming true — no new gameplay logic needed for either.
  `GEHud::Draw()` gained an `overlayMessage` parameter: non-null skips the real HUD entirely
  (matching "HUD hidden outside Play" exactly) and shows one big centered message instead
  ("PAUSED"/"YOU WIN!"/"GAME OVER") — an honest minimal placeholder, not a claim of real menu-
  screen parity (those need score/level-slot/level-time infrastructure this engine doesn't have).
  `First`/`Wait`/`Init` are real but not part of this engine's own startup flow (no async
  loading step or main menu exists) — starts directly in `Play`. `Trial`/`MainSetup`/
  `PlaySetup`/`Resume`/`Ranking` are real enum values with no trigger wired to them yet.
  Verified live: default Play behavior unchanged, all 3 overlay messages render correctly, and
  the Play<->Pause toggle responds correctly to simulated input across multiple frames
  (temporary debug instrumentation, reverted before committing) + full suite (64/64 unit tests,
  all verify tools) + both backends.

- **Training-hint overlay implemented, Phase 9's real `DrawInfo` scope now 100% complete
  (2026-07-13, plan.md `HUD-024`).** Per explicit user approval: (1) implemented a real
  mission-number concept (`Worlds::World::missionNumber()`, using format v2's first
  already-reserved header field — no format/size change, existing `.vwr` files unaffected), (2)
  transcribed all 4 real `Tables::table_training1`-`4` arrays (43 hint records total) into new
  `GETrainingHints.hpp`/`.cpp`, verified against `Decor.cpp:1258-1310`/`1313-1340`, and (3) found
  and transcribed their real English hint text from `MyResource.cpp`'s `InitializeEN()`. Real
  gate semantics fully modeled: exact-treasure-count match, in/not-in-any-vehicle, carrying/not-
  carrying-dynamite — all driven by state this session already tracks. Inline real button-icon
  glyphs (the original text embeds control bytes `Text::DrawChar` renders as pictograms) aren't
  supported by this engine's text renderer; replaced with bracketed labels (`[Move]`/`[Jump]`/
  `[Action]`) instead of silently dropped. HUD: full-width top-of-screen panel, text centered and
  auto-shrunk to fit, matching the real layout exactly. The shared sample world is deliberately
  left at mission 0 (its layout is unrelated to the real tutorial world, so forcing mission 11
  would fire hints in nonsensical places) — verified instead via `VerifyInteractionSystem` +
  live headless screenshots with a temporary mission/position override (reverted before
  committing). **With this done, every element the real `Decor::DrawInfo` function draws is now
  implemented in `GalaxyEggbertCNA`** — Phase 9's `090`-`093` scope is complete; only the
  separately-flagged, not-found-in-`DrawInfo` `HUD-0NN` items remain, each needing its own
  verification before being treated as real work. Verified: 1 new `WorldSerializationTests` +
  12 new `VerifyInteractionSystem` assertions + full suite (64/64 unit tests, all verify tools)
  + both backends.

- **Perso decoy mechanic + HUD counter implemented (2026-07-13, plan.md `HUD-017`).** Researched
  what "Perso" actually is (previously unknown): a deployable `ObjectType200` decoy statue (same
  `blupi.png` look as Blupi), verified directly against `Decor.cpp:4818-4841`/`6088-6101`/
  `10291-10294`. New `GEInteractionSystem::TryPerso()` — a single call handles both real branches
  (picks up an already-placed decoy within range if one exists, matching the real source's own
  priority; otherwise places a new one if carrying at least one and grounded), mutually exclusive
  with dynamite on the same action-button press (real `else if`). Real cap 5. HUD: button.png
  icon 108 + "= N" text at real scale 0.7. **Real starting count is level-authored save data
  (`_blupiPerso_`, default 0) with no world-pickup that grants it at all** — this engine has no
  level-authored-starting-inventory concept yet, so the mechanic starts at 0 (matching the real
  default) and isn't reachable via a sample-world demo placement; verified entirely via 8 new
  `VerifyInteractionSystem` assertions using a synthetic decoy (full pickup->place round trip) +
  a live headless HUD screenshot (temporary forced count, reverted before committing). What
  placing a decoy actually DOES gameplay-wise (if anything beyond existing as a marker) wasn't
  found by a targeted search of enemy-AI code — not modeled as a gameplay effect. Full suite
  re-run (63/63 unit tests, all verify tools) + both backends.

- **Full plan.md HUD-0NN audit against the real `Decor::DrawInfo` (2026-07-13).** Read the entire
  real in-game HUD draw function (`Decor.cpp:1185-1311`) end to end and cross-checked every
  `HUD-0NN` plan.md entry against it — the first time this was done exhaustively rather than
  piecemeal. Result: every element `DrawInfo` actually draws is now either implemented (this
  session's life/key/treasure/bullet/dynamite/gauge work) or a correctly-identified real gap
  (`HUD-017` perso counter, `HUD-024` training hints — both blocked on separate not-yet-built
  mechanics). Every OTHER `HUD-0NN` entry not found in `DrawInfo` is now flagged `[?]` for
  independent re-verification rather than left looking like confirmed, simply-unstarted work —
  two turned out to rest on wrong premises entirely: `HUD-002`'s claimed 5-icon life cap doesn't
  exist (the real source is uncapped, matching what `GalaxyEggbertCNA` already does), and
  `HUD-009`/`025`'s "score display" has no real backing anywhere found (only a separate,
  unrelated high-score ranking MENU screen exists). See plan.md's Phase 9/§2.3 for the full
  per-item breakdown.

- **Both real `jauge.png` HUD gauges implemented (2026-07-13, plan.md `HUD-008`/`012`/`018`/`019`).**
  Researched the real `Jauge` widget (`Jauge.hpp`'s own fully-documented class comment: a
  124×22px two-layer sprite, empty background always drawn, colored fill cropped to
  `[6, 6+level*114/100]` pixels) and every real `Decor.cpp` call site, which corrected 2 stale
  plan.md entries: `HUD-018` ("second gauge... charge level") isn't a separate mechanic at all —
  it's `HUD-012`'s own water/Nage gauge switching from Blue to Red at the real low-air warning
  threshold (level<=25); and `HUD-008`/`019` (both describing "shield timer gauge") are the SAME
  single widget, shared by Shield/Power/Cloud/Hide alike (`Decor.cpp:5071-5137` — all 4 reuse one
  `m_blupiTimeShield` variable and gauge, not 4 separate ones). Implemented both real gauges in
  `GEHud`: the water/Nage breath gauge (position (90,450)) wired to the already-implemented
  `GEBlupiController::IsNage()`/`GetWaterGaugeLevel()`, and the Shield/Power/Cloud/Hide countdown
  gauge (position (90,428), always Yellow) wired to `GetSecretPower()`/`GetSecretPowerLevel()` —
  no new gameplay logic needed, both pieces of state already existed from earlier this session.
  Verified live via a headless HUD screenshot (temporary hardcoded gauge levels for the shot,
  reverted before committing) — both gauges render at the correct position with correct
  proportional fill widths and the right color (confirmed the water gauge switches to red below
  the warning threshold). Full suite re-run (63/63 unit tests, all verify tools) + both backends.

- **Phase 9 (HUD) started per explicit user direction (2026-07-13): bullet/dynamite HUD counters
  added (plan.md `HUD-015`/`016`), and a stale Phase 9/§2.3 documentation gap closed.** While
  scoping this, found that `GEHud`'s treasure-counter text rendering (`HUD-003`/`004`, real
  `text.png` glyph-sheet + `pad.png` panel) was ALREADY implemented (2026-07-10) — plan.md's own
  Phase 9 top-level bullet had gone stale, still claiming "treasure/score text needs text
  rendering, not started" and "using CNA `SpriteBatch`" (it's real 3D quads, for the same Vulkan
  submission-order reason documented in `GEHud.hpp`'s own class comment) well after that work
  shipped; corrected. New work: `GEHud::Draw()` gained `bullets`/`dynamite` parameters, verified
  directly against the real `Decor::DrawInfo` (`Decor.cpp:1197-1201`/`1212-1217`): element.png
  icon 176 × bullets held at (570,442) advancing X+=4 (the same heavily-overlapping "fanned" row
  look as the life icons), and element.png icon 252 at (505,414) shown only while carrying
  dynamite (no counter loop — the real game only ever holds at most 1, matching this engine's own
  cap). Both wired directly to `GEInteractionSystem::BulletCount()`/`DynamiteCount()`, already
  implemented earlier this session — no new gameplay logic needed. Verified live via a headless
  HUD screenshot (temporary hardcoded bullet=5/dynamite=1 values for the shot, reverted before
  committing) — both new icons render at the correct position/texture, bullets forming the
  expected overlapping row. Full suite re-run (63/63 unit tests, all verify tools) + both
  backends.

- **Footstep terrain-remap implemented + a real pre-existing landing-sound bug fixed
  (2026-07-12, plan.md `E3D-MIG-084`).** New `GESound::FootstepChannelFor(icon)` covers all 7 real
  `Decor::SoundEnviron()` terrain-specific footstep ranges (channels 78/80/82/84/86/88/90),
  falling back to the generic channel 3 outside them; wired into `PlayStep()`/`PlayLand()` keyed
  off `GEBlupiController::GetGroundBlockType()`. **Found and fixed a real bug while researching
  this**: `PlayLand()` played channel 4, but channel 4 is real head-bump/ceiling-hit — a distinct
  event, not landing (channel 3 covers both footstep AND landing in the real game) — a mistake
  ported verbatim from `GalaxyEggbertSimple3D`'s own `GESound` before the later, independently-
  verified channel research existed. Head-bump itself stays unwired (no ceiling-hit detection
  exists in `GEBlupiController`), a separate, not-yet-implemented mechanic, out of scope here.
  Verified: 10 new `VerifyInteractionSystem` assertions + full suite (63/63 unit tests, all verify
  tools) + both backends (live headless run, no crash/regression).

- **`E3D-MIG-177` researched, not implemented (2026-07-12) — split into a documented non-goal and
  a `needs_human` blocker.** Ecrase/pancake collision-box mode: the real source uses a genuine
  AABB collision system (squashing shrinks Blupi's hitbox to a distinct 50×19 pancake, letting him
  fit under low gaps); `GalaxyEggbertCNA`'s single-point collision model has no hitbox to shrink,
  so this is now a documented non-goal, not a fidelity gap. Suspended (hanging-on-a-bar) mode: a
  real, fully-speced, cheap mechanic (verified against `Decor.cpp:4732-4790`/`GetTypeBarre`, every
  constant known — direct `speedX*5` move with no accel ramp, 10-tick jump wind-up then fixed
  `vitesseY=-11` launch, 5-tick no-regrab grace) but blocked on a pending render-geometry decision
  for icon 202 ("thin-bar" — flagged in `02-tiles.md` as new, not-yet-implemented geometry) that
  the user explicitly asked to skip guessing at this session (same `needs_human` category as the
  Saw blade). See plan.md's `E3D-MIG-177` entry for full detail. **Recommended next step once a
  human makes that render-geometry call: implement Suspended mode using the already-known
  constants above.**

- **Bullet pack pickup implemented (2026-07-12, plan.md `E3D-MIG-175`).** New
  `GEInteractionSystem::BulletCount()`, same shape as `dynamiteCount_`: automatic on contact (no
  button), caps at 10, gated exactly like the real source (`m_blupiBullet < 10` — touching a pack
  already at the cap is a genuine no-op, object stays in the world). One demo pack added to the
  sample world, right next to the Jeep demo room. The actual firing mechanic (`ObjectType23`
  projectile spawn from Helicopter/Tank's fire button) is explicitly NOT modeled yet — this only
  tracks the ammo count; firing needs a live player-fired projectile system that doesn't exist
  yet (the existing blupih/blupit projectiles are enemy-fired via the existing patrol/dwell
  mechanism, not general-purpose). Verified: 6 new `VerifyInteractionSystem` assertions + full
  suite (63/63 unit tests, all verify tools) + both backends.

- **Teleporter double-tip render bug fixed (2026-07-12, plan.md `E3D-MIG-147` follow-up).** User
  report (§4's former open item 2): the pyramid tip appeared to render TWICE per pillar, one copy
  too far below the cube. Root-caused via a live headless screenshot with the world isolated down
  to just one pillar (background disabled too, to rule out every other on-screen object first):
  the cube's own 4 side faces were reusing the WHOLE tile texture (shared `kSymmetricEntries`
  convention), which includes the same lower-two-thirds "post/spike" graphic the real 3D tip mesh
  already crops out separately — its alpha-cutout silhouette made the cube's own flat face look
  like a second, fake, flattened spike sitting immediately below the panel, right next to the
  real 3D tip. Fixed with a teleporter-specific UV override (new `TeleporterPanelUv()`, crops the
  side faces to just the top-third panel graphic) in `GETerrainRenderer.cpp`'s
  `AppendSpecialGeometry()`. The real 3D tip itself needed no repositioning — a vertex-level dump
  confirmed it was already a single, correctly-flush instance (cube Y=[1.5,2.5], tip Y=[0.9,1.5]).
  Verified live from 3 angles/distances before and after (temporary debug: world-isolation filter
  + background disabled, reverted before committing) — confirmed the duplicate is gone and the
  remaining tip renders cleanly flush under the cube from both a straight-on and 3/4 view. Full
  suite re-run (63/63 unit tests, all 4 verify tools) + both backends.

- **Vehicle mounts implemented + a real, general falling-movement collision bug found and fixed
  (2026-07-12, plan.md `E3D-MIG-171`, Phase 17 continued).** New `GEBlupiController::VehicleMode`
  (Helicopter/Jeep/Tank/Skateboard/Overcraft) + `TriggerMount()`/`TriggerDismount()`, confirmed
  against `mobile-eggbert-reference/10-blupi-mechanics.md` §6 and `13-object-pickups.md`'s
  "Vehicle mounts" section: real pickup mapping `ObjectType13`->Helicopter, `19`->Jeep, `28`->
  Tank, `24`->Skateboard, `46`->Overcraft (confirmed **not** "Balloon" — touching 46 just sets
  `m_blupiOver`, a previously-documented discrepancy; no confirmed pickup grants a real standalone
  Balloon vehicle, so it isn't modeled). Real gate: blocked only while already riding any vehicle
  or while Nage/Surf swimming — confirmed real oddity that Shield/Power do NOT block mounting;
  mounting silently cancels active Cloud/Hide but leaves Shield/Power untouched. Real per-mode
  max-speed/accel/decel have no established px->engine conversion factor, so the REAL RELATIVE
  proportions between vehicles were preserved instead (Jeep fastest, Tank/Overcraft slowest,
  Skateboard/Helicopter between), anchored to an arbitrary "vehicles feel faster than walking"
  baseline (same technique as `kSpringBounceHeld`). Helicopter/Overcraft get real free vertical
  flight (ascend/descend reusing the same crouch/lookUp inputs that mean camera pitch on foot)
  instead of gravity; Jeep/Tank/Skateboard reuse the existing ground gravity/jump path unchanged.
  Mount/dismount scan-and-deposit logic lives in `GalaxyEggbertCnaGame.cpp` (not
  `GEInteractionSystem`, which has no access to the `GEBlupiController`-only `VehicleMode` enum),
  bound to the same action button as switches/dynamite. NOT modeled: standalone Balloon vehicle,
  tilt easing, Overcraft's real altitude-cap/inverted-accel-over-gaps nuance, the real
  floating-object auto-mount-without-a-button exception, per-vehicle hazard immunity. One Jeep
  demo pickup added to the sample world.
  **Found and fixed a real, pre-existing, non-vehicle-specific collision bug while live-testing
  this**: `GEBlupiController::TryMoveAxis()` silently froze ALL horizontal movement once `m_y`
  fell far enough negative during a sustained fall through a floorless column (past roughly -2) —
  its step-up check compared the destination column's ground height directly against Blupi's own
  deeply-negative Y, misreading "I'm far below because I'm falling" as "that's an unclimbable
  wall." Every prior fall test only ever dropped straight down with zero horizontal input, so this
  went undetected until vehicles made "hold a direction while falling for several seconds" an
  actual gameplay scenario. Fixed by skipping the step-up restriction entirely while airborne
  (`!m_onGround`) — it only makes sense while grounded. Verified live (Jeep drives continuously
  across varied terrain, including off a ledge, with no freezing) and via a new dedicated
  regression test. Verified overall: 15 new `VerifyBlupiMovement` assertions (mount/dismount
  gates, Cloud/Hide-cancel-on-mount with Shield/Power preserved, real accel ramp, coasting,
  Helicopter ascend/descend) + 1 new TryMoveAxis regression test + full suite (63/63 unit tests,
  all verify tools, 116/116 in `VerifyBlupiMovement`) + live headless verification (temporary
  debug instrumentation, reverted before committing) + both backends.

- **Secret powers implemented + a real documentation error corrected (2026-07-12, plan.md
  `E3D-MIG-170`/`172`/`174`, Phase 17 start).** Direct `Decor.cpp` research (not just the
  reference doc) found the project's own earlier "Sp0-Sp7" icon research was WRONG: tile icons
  158-165 are hub-screen world-select markers (`Decor::IsWorld()`), unrelated to Blupi's own
  secret-power buffs — the real `SecretPower` enum only has 5 values (None/Shield/Power/Cloud/
  Hide, confirmed in `def/SecretPower.hpp`), granted instead by 4 `MoveObject` pickups
  (ObjectType25 Shield, 26 Sucette->Power, 30 Drink->Hide, 31 Charge->Cloud). Corrected
  `mobile-eggbert-reference/02-tiles.md`'s icon 158-165 rows and re-scoped `E3D-MIG-515`
  accordingly (now a hub-screen task, not a secret-power one). Implemented the real mechanic:
  new `GEBlupiController::SecretPower` state (mutually exclusive, matching the documented
  invariant) with each power's own exact real gauge decrement rate and warning threshold
  (Shield 0.25s/level+warn@10, Power 0.15s/level+warn@20, Cloud/Hide 0.2s/level+warn@25/20 —
  all direct transcriptions). **Most valuable part**: `IsInvincible()` (Shield or Hide) now
  genuinely protects Blupi, confirmed identical across ~15 separate real hazard/enemy call sites
  in `Decor.cpp` (lava, spikes, saw, blitz, crusher, dynamite, fan, the shared 8-type kill list,
  wasp, large creature, blupih/blupit projectiles) — this resolves the "Shield/Hide/SuperBlupi
  immunity NOT modeled" caveat left on essentially every hazard implemented earlier this session
  (superBlupi itself remains unmodeled, no such concept exists). The real 2-stage delay/
  animation-lock before Power/Hide/Cloud actually activate (Sucette/Drink/Charge) is NOT modeled
  — all 4 grant instantly on contact instead, a documented simplification. New secret-powers demo
  (one of each pickup) added to the sample world. Verified: 19 new `VerifyBlupiMovement`
  assertions (trigger gates, exact real decrement rates, expiry, warning threshold) + 5 new
  `VerifyInteractionSystem` assertions (pickup grant gating, hazard-immunity integration) + full
  suite (63/63 unit tests, all verify tools) + live headless verification (temporary debug
  instrumentation, reverted before committing, confirmed the exact real 0.25s/level Shield rate
  live in the actual game loop) + both backends.

- **Doors & keys implemented, Phase 16 substantially done (2026-07-12, plan.md `E3D-MIG-160`/
  `161`/`162`).** Doors (icons 334-336) now open automatically when Blupi approaches holding the
  matching key (probing his own cell AND one cell ahead in his facing direction, matching real
  `Decor::IsDoor` exactly) — tile removed, a transient sliding-up `ObjectType22` plays over the
  real 2.5s, key consumed on use (not on pickup). Treasure-gated doors (icon `420+N`) open ALL
  qualifying doors level-wide the instant a treasure pickup completes, matching
  `Decor::OpenDoorsTresor` exactly. New key-gated + treasure-gated door demo added to the sample
  world (2 short corridors, each with one real wall + door gap). Rendering doors as a
  transparent billboard (`163`) and the hub/menu-screen door logic (`164`/`165`) are explicitly
  deferred — the former is a new visual-design decision the user asked to skip this session, the
  latter needs menu/hub screens that don't exist yet. Verified: 11 new `VerifyInteractionSystem`
  assertions + full suite (63/63 unit tests, all verify tools) + both backends.

- **Dynamite implemented (2026-07-12, plan.md `E3D-MIG-155`).** Real pickup (caps at 1 carried)
  + action-button placement (gated on carrying one + grounded) + the exact real 9-blast fuse
  sequence, verified directly against `Decor.cpp` (not just the reference doc's rounded
  summary) — exact real blast ticks/pixel-offsets and the exact real 28-type destructible-object
  list were read straight from the source. Each blast clears Saw hazard tiles and destroys
  overlapping objects (crates via the same linked-group logic as `150`) in its 2x2-tile area, and
  kills Blupi if caught. No debris/particle visuals (no such system exists — a documented
  simplification, same as every other missing-particle-effect gap this session). New dynamite
  pickup + target added to the sample world (lift room B, next to the linked-crate demo) for a
  genuinely playable scenario. Verified: 10 new `VerifyInteractionSystem` assertions + full suite
  (63/63 unit tests, all verify tools) + both backends.

- **Riding a moving platform lift now works (2026-07-12, plan.md `E3D-MIG-152`/`154`) — a
  long-standing "not yet working" limitation is closed.** `GEInteractionSystem` detects (before
  its own per-frame lift patrol step) whether Blupi is standing on an active lift's surface,
  then reports the lift's own displacement that tick (`IsRidingLift()`/`RideDeltaX/Z()`/
  `RideStandY()`) for the caller to apply via new `GEBlupiController::RideLift()` — X/Z apply as
  a delta (so his own walking input isn't overridden), Y snaps absolutely each frame (matching
  the real source's own drift-correction approach), and he's marked grounded so gravity doesn't
  immediately re-trigger a fall. Types 47/48's real conveyor nudge (`154`) is folded in as a
  constant offset. Verified live (temporary debug instrumentation, reverted before committing):
  Blupi's Y tracked the north-hill lift's full ping-pong patrol, staying grounded throughout.
  New `VerifyInteractionSystem` assertions + full suite (63/63 unit tests, all verify tools) +
  both backends re-verified.

- **Linked-crate flood-fill implemented (2026-07-12, plan.md `E3D-MIG-150`, Phase 15 start).**
  Pushing a crate now flood-fills every touching crate (real `SearchLinkCaisse`: 1 grid unit in X
  or Y, same Z, restricted to crates at or above the seed's own row) and moves the whole linked
  group atomically — floor-support is only re-checked for members at the seed's own row, matching
  the real source exactly; any one member blocked cancels the whole push. New linked-crate demo
  (2 side-by-side + 1 stacked, in the water-pool room) added to the sample world for live
  verification. Reduced-push-speed-scales-with-stack-size (`151`) is NOT modeled — this engine's
  crate push is a discrete per-frame grid-cell snap, not the real continuous px/tick system, so
  there's no existing "speed" dial to scale down; deferred along with the "pop" push variant
  (needs `GEBlupiController` fall-state coordination that doesn't exist yet). Verified: 3 new
  `VerifyInteractionSystem` assertions + full suite (63/63 unit tests, all verify tools).

- **Water breath gauge implemented, Phase 14 now 10/10 complete (2026-07-12, plan.md
  `E3D-MIG-148`).** Real 3-state Surf(surface)/Nage(submerged)/dry machine + ~25s breath gauge +
  drowning, verified directly against `Decor.cpp` via `mobile-eggbert-reference/
  12-hazards-and-interactables.md`'s "Water depth state machine" section. Required a real
  architectural fix first: water blocks were solid-for-collision (Blupi always rested ON TOP of
  the topmost water layer, like land), making genuine submersion structurally unreachable — fixed
  by making water ALWAYS non-solid in `GEBlupiController::GroundHeightAt()` (same precedent as the
  teleporter pillar/fan head), so Blupi now genuinely sinks through any depth of water to the real
  floor beneath it. New `GEBlupiController::GetBlockTypeAt()` (the tile at his own resting cell,
  mirroring `GetGroundBlockType()`/`GetBlockTypeAbove()`) plus the existing `GetBlockTypeAbove()`
  reproduce the real `IsSurfWater`/`IsDeepWater` distinction (water-with-dry-above = Surf, water-
  with-water-above = Nage). New `IsSurf()`/`IsNage()`/`GetWaterGaugeLevel()`/`JustDrowned()`, fed
  by two new `Step()` parameters (`inSurfWater`/`inDeepWater`, caller-computed each frame exactly
  like `tempPassable`). Gauge ticks 100→0 over the real ~25s (`kWaterGaugeTickSeconds=0.25s/level`,
  a direct `Config::ScaleTime(5)`-at-20Hz transcription, same technique as `kTeleportDuration`),
  resets to full the instant Nage ends. Drowning plays the real dedicated channel 26 (distinct from
  every other death cause, per `07-sounds.md`) via the existing shared `triggerDeath()` lambda;
  channel 22 (splash) plays entering Surf/Nage from dry, channel 25 on Nage→Surf (resurfacing).
  Nage also gets reduced "floaty" gravity and a swim-up jump (both documented approximations, same
  shape as the wasp balloon's own gravity multiplier) instead of normal ground-jump physics. Real
  jump-launches-you-out-of-the-water sub-tile nuance, vehicle dismount, and Shield/Hide/SuperBlupi
  immunity are NOT modeled (Phase 17 dependencies, same simplification pattern as every other Phase
  14 mechanic). **Found and fixed a real content bug while implementing this**: the sample world's
  existing tunnel water crossing sat directly on the world floor (y=0) with nothing beneath it —
  once water became non-solid this would have turned a shallow wade into a bottomless-pit death
  trap; moved the water to y=1 with the floor intact at y=0. Added a new genuinely 2-layer-deep
  pool (open-sky room, grid x=61-67/z=71-77, same "no walls/ceiling" pattern as the teleporter/fan/
  lift rooms) so Nage/drowning is live-playable, not just unit-tested. Verified: 6 new
  `VerifyBlupiMovement` assertions (shallow pool = Surf, deep pool = Nage, full gauge-to-drowning
  cycle at the correct ~25s+fall-time mark, gauge-reset-on-resurface) + full suite (63/63 unit
  tests, all 5 verify tools) + a live headless run with temporary debug instrumentation (spawn
  override + periodic position/state log, reverted before committing) confirming the exact real-
  time sequence end to end — both EasyGL and Vulkan backends re-verified.

- **Platform lift clipping fixed + 2 more lifts added (2026-07-12, NEXT.md §8 old task 3, user
  request 2026-07-11: "jako dalsi ukol si uloz aby ten demo svet mel vice presouvacich bloku a ten
  soucasny presouvaci blok je pod deskou tak ze se to presouva skrze desku, je to k nicemu").**
  Root-caused via a temporary debug camera override (reverted, not part of the diff) plus a
  temporary "parked at posEndY" world regeneration (also reverted): the north-hill lift's
  destination (the crow's-nest floor, `fill(49,51,8,8,27,29)`) was a FULLY SOLID slab with no shaft
  opening at the lift's own column (x=50,z=28) — confirmed live, the lift could only ever rise to
  touch the slab's solid underside, with no visible passage, matching the literal complaint. Fixed
  by (1) carving a 1-cell shaft opening (`Air`) at the lift's exact column, and (2) lowering
  `posEndY` from `8.0f` to `7.0f` so the lift's own rendered top face
  (`GalaxyEggbertCnaGame.cpp`'s `kObjectCubeGroundOffset=+1.0` plus the cube's own 0.5 half-height)
  lands exactly flush with the surrounding floor's top face instead of sitting proud on solid rock
  by a full unit. Verified live via headless EasyGL screenshots at 3 vantage points (default state,
  temporarily forced "at rest in the hole", from multiple angles) — the platform now plugs the
  shaft flush, no floating box, no visible clip into solid terrain. **2 new lift rooms added** south
  of the existing fan rooms (x=45-51 and x=53-59, z=71-77, same "open sky, no walls/ceiling"
  pattern as the teleporter/fan rooms — deliberately avoids the known `GroundHeightAt()`
  roofed-interior limitation, §5), each with its own carved shaft + flush-tuned `posEndY`, bringing
  the world to 3 total platform lifts (was 1). Verified: 63/63 `GalaxyEggbertWorldsTests`, all 5
  verify tools (`VerifyBlupiMovement`, `VerifyInteractionSystem`, `VerifyMoveObjectTypesCna`,
  `VerifyBigDecorParsingCna` — run from repo root, not `build-cna`, per §7's own note — plus
  `easy-3d`'s own suite untouched by this change), both EasyGL and Vulkan backends live (headless,
  `11 platform-lift/crate cube object(s) found`, up from 9, matching the 2 new lifts).

- **Fourth round of live feedback (2026-07-11, plan.md `E3D-MIG-142`): the Saw blade is STILL
  wrong, teleporter tip confirmed correct/settled.** User (Czech, verbatim, with
  `Screenshot From 2026-07-11 17-56-53.png`): "synu zase jsi to zkurvil pila je spatne ... rovna
  strana pily musi byt u zeme a kolo se zuby nahoru a ohledne tepeporteru to je beze zmeny priste
  ty ohledne teleporteru napisu jak si to predstavuji" — the flat/straight side of the saw must be
  at ground level, the wheel-with-teeth must point up; teleporter tip needs no further changes for
  now (user will write a fresh, detailed spec for it next time if anything). **No code changed this
  round** — the user explicitly asked to stop guessing live and document instead. New analysis
  (not yet acted on): direct alpha-channel inspection of the real icon 378 crop found its actual
  content occupies only the BOTTOM HALF of the 64×64 tile (`y:[32,63]` of 64, top half fully
  transparent) — the current code passes the WHOLE tile's UV uncropped, and there's an unverified
  theory that `Easy3D::AppendPlateMesh`'s corner/UV convention vertically flips whatever texture
  it's given (world-bottom of the plate shows the source image's own top row, and vice versa).
  Full technical writeup and recommended next steps are in §8's newest task entry — read that
  before attempting another fix.

- **Third round of live feedback with an actual screenshot (2026-07-11, plan.md `E3D-MIG-142`/
  `147`/`149`): fixed the Saw's ANCHOR direction and reverted the teleporter tip to a genuine
  pyramid.** User (Czech, verbatim, with `Screenshot From 2026-07-11 16-35-24.png`): "ta pila je
  obracene reze do zeme ale mela by rezat nahoru dale ty teleportery zadni teleporter se renderuje
  dopredu je to rozbite ... renderovani teleporteru je jen z casti v poradku ... dale je [vidět] ze
  ty hroty 4 textury trojuhelniku nejsou dole svazane k sobe" — found the screenshot locally
  (`/home/robertvokac/Pictures/Screenshots/`) and inspected it directly.
  - **Saw**: the previous fix (bottom-anchored, flush with the block's own `-0.5` bottom face) was
    still wrong — neighboring floor tiles' own walkable surface sits at `+0.5` (their solid tops),
    not `-0.5`, so the blade sat entirely BELOW the visible floor line, reading as buried/cutting
    into the ground rather than poking up where Blupi actually walks. Re-anchored to the block's
    TOP face instead, extending downward from there — confirmed live (screenshot shows the blade
    now at the top of its recessed "pit," flush with the surrounding floor, not at the bottom).
  - **Teleporter tip**: reverted from the non-tapering box (added in round 2) back to a genuine
    tapering pyramid — `Easy3D::PyramidTipItem`/`AppendPyramidTipMesh()` re-added to `../easy-3d`
    (removed in round 2, now restored with its own test). The box's 4 flat side faces (each
    showing a triangle via an alpha cutout) don't share a common vertex the way a real pyramid's 4
    triangular faces do, so adjacent faces' triangle graphics visibly failed to connect at the
    block's 4 vertical edges — confirmed directly in the screenshot. A genuine pyramid's faces
    share one apex vertex by construction, so it's structurally seamless; combined with the
    already-fixed alpha blending (round 2, unrelated to box-vs-pyramid), this also keeps the
    "black background" fix. Verified live: both real gameplay teleporter rooms AND the exact
    exhibition row shown in the user's own screenshot now render cleanly (no black, no seams,
    proper single-point convergence) on a fresh screenshot at the same location.
  - The "zadní teleporter renderuje dopředu" (rear teleporter renders forward) glitch was not
    independently reproduced after the pyramid revert — both real teleporter rooms and the
    exhibition row all rendered correctly, so it's treated as a symptom of the box design (same
    root cause as the seam issue) rather than a separate bug, though not 100% conclusively ruled
    out as distinct.

- **Fixed the Saw's plate positioning + added per-placement rotation metadata (2026-07-11, plan.md
  `E3D-MIG-142`/`149`), per a second round of live user feedback on the render-mode fix above.**
  User (Czech, verbatim): "ta pila ma byt obracene u zeme a ne ve vzduchu nyni je to nesmysl pila
  bude vzdy u zeme jenom pomoci metadat bude v bloku ulozen smera jeden bit bude stacit tedy zda
  otocit o 90 stupnu" — two real, separate issues.
  - **Ground anchoring**: the plate was centered mid-block like every other confirmed
    `InnerFlatPlate` icon (signposts/screens, where that's correct), reading as "floating" for a
    blade that should emerge from a floor-level slot. Fixed with a Saw-specific
    `kSawPlateHeight=0.5` (shorter than the shared `kInnerFlatPlateHeight=0.9` — a full-height
    panel still reads as floating even bottom-anchored, since its top would sit almost as high as
    a neighboring floor tile's own top) and bottom-anchored positioning (`IsGroundAnchoredPlateIcon()`
    in `GETerrainRenderer.cpp`) — flush with the block's own bottom face. Every other confirmed
    `InnerFlatPlate` icon is unaffected (still full-height and centered).
  - **Per-placement rotation metadata**: the earlier fix hardcoded Saw's own default axis to `X`
    (matching this one placement's corridor), but that's wrong for any future Saw placed in a
    Z-running corridor — real mobile-eggbert's 2D sprite has no axis concept at all to derive a
    single correct per-icon default from. New `GEPlateRotationMetadata.hpp/.cpp`
    (`src/GalaxyEggbertCNA/Game/`, deliberately Easy3D/CNA-independent, matching
    `GEBlupiController`/`GEWorldRuntime`'s own established precedent for this tree) stores a
    1-byte "rotated 90°" flag as sparse block extra-metadata (reusing `Worlds::World`'s existing
    `setBlockExtraMetadata`/`collectExtraMetadata` mechanism, already proven by
    `MoveObjectRecord`'s own use of it — `kPlateRotationMetadataType=2`, next after
    `kMoveObjectMetadataType=1`). Saw's own default axis reverted to the shared `Z` default;
    `GetInnerFlatPlateAxis()` now takes a `rotated` bool and swaps X<->Z when true.
    `GETerrainRenderer` collects every rotated position once per (re)build
    (`CollectRotatedPlatePositions()`) into a packed-key hash set for O(1) per-block lookup across
    all 3 `AppendSpecialGeometry()` call sites (static/transparent-static/animated — Saw itself is
    always animated, so only the 3rd call site actually exercises this for it today).
    `tools/GenerateSampleWorld3D.cpp`'s real switch+saw pair now calls `SetPlateRotated(world, 70,
    0, 67, true)` explicitly instead of relying on a hardcoded icon default.
  - Verified: metadata round-trips through `.vwr` save/load (confirmed via a standalone check —
    `CollectRotatedPlatePositions()` correctly returns the one rotated position after a fresh
    `loadFromFile()`), full 5-tool suite + both backends re-verified, and live via headless EasyGL
    screenshots at multiple distances (temporary camera-pose debug override, reverted before
    committing) — the blade now sits low near the floor with visible wall space above it, a
    clearly different and correct silhouette from the earlier "floating full-height panel" state.

- **Fixed the Saw tile's render mode (2026-07-11, plan.md `E3D-MIG-142`/`149`), per live user
  feedback.** User (Czech, verbatim): "ta pila saw se renderuje spatne nema to byt na krychly pila
  bude staticky billboard tedy uprosred daneho bloku se textura nanese na obe strany jakoby
  neviditelen desky v puli krychle" — Saw/SawStopped (378/379) were falling through every
  special-geometry table into the generic fully-textured `UniformCube` fallback.
  - Added both icons to `GEInnerFlatPlateTiles.cpp`'s confirmed table (genuinely thin
    double-sided plate through the block's middle, outer 6 faces never drawn), superseding an
    earlier session's undecided "ThinMechanical" placeholder categorization —
    `mobile-eggbert-reference/02-tiles.md`/`questionnaire-all-remaining-tiles.md` both updated to
    record the revision (note: an even earlier pass had already answered "Billboard, not
    ThinMechanical" — this revision specifies exactly which billboard-family mode, not a
    contradiction).
  - `GetInnerFlatPlateAxis()` special-cases Saw/SawStopped to `PlateAxis::X` (every other
    confirmed `InnerFlatPlate` icon uses a `PlateAxis::Z` default) — the one real placement (the
    switch+saw pair, `worlds3d/world001.vwr`) sits in a corridor Blupi walks along X, and a
    Z-axis plate is invisible edge-on from that approach. Real mobile-eggbert's 2D sprite has no
    axis concept at all, so this is a 3D placement adaptation, not a faithfulness question.
  - **Verifying this live surfaced a real debugging trap worth recording**: the switch+saw pair
    sits inside the south tunnel's roofed interior, which is exactly where the pre-existing
    `GroundHeightAt()` ceiling limitation (§5) applies — `blupi_`'s own Y kept resolving to 4 (on
    top of the roof) instead of 1 (the real floor) every time a debug spawn was placed there via
    `SetPosition()`, silently invalidating over a dozen camera-angle attempts before this was
    diagnosed via a `[CAM DEBUG]` log line comparing intended vs. actual camera pose. Worked
    around for this one screenshot by setting `camera_`'s position/target directly, independent
    of `blupi_`'s corrupted Y (reverted before committing, same as all debug instrumentation).
  - Final live screenshot confirms a genuine thin plate showing the real jagged circular-blade
    texture, with the tunnel's red wall visible around/through it (proving the cube's outer faces
    are truly not drawn, not just retextured). Full 5-tool suite + both backends re-verified.

- **Fixed the teleporter tip's shape AND a real transparency bug (2026-07-11, plan.md
  `E3D-MIG-147`, live user re-check after the earlier pyramid-tip pass).** User reported the
  earlier pyramid-tip geometry showed black filling the lower/wider part of each triangular face,
  and clarified the real shape is a "čtyřhranol" (four-sided/rectangular prism), not a cone
  tapering to a point.
  - **Root-caused with a direct pixel/alpha-channel inspection of `object-m.png`**: the "black"
    area is genuine alpha=0 (fully transparent) around the real teal cone/spike graphic, not
    painted black — confirmed via a small script sampling specific pixels (e.g. `(10,50)` reads
    `[0,0,0,0]`, `(32,50)` reads `[31,90,109,255]`). The pyramid's tapering triangles had also been
    stretching UV coordinates across areas the real (non-tapering) artwork was never meant to
    cover, compounding the same underlying transparency bug.
  - **Shape fix**: replaced `Easy3D::PyramidTipItem`/`AppendPyramidTipMesh()` (removed entirely
    from `../easy-3d` — hpp/cpp/test, since nothing else used it) with a second, narrower
    `Easy3D::DirectionalCubeItem` box (4 straight side faces only, no tapering) appended right
    after the teleporter's own cube in `GETerrainRenderer.cpp`'s `AppendSpecialGeometry()` — the
    same primitive/approach `InnerPillarBox` already established elsewhere, just a one-off
    attachment rather than a confirmed render-mode table entry.
  - **Transparency fix**: added Teleport1-4 to `NeedsAlphaBlend()` (previously only icons 30/31),
    routing the whole teleporter icon — cube sides AND the new tip box, both using the same
    texture — through the existing alpha-blended transparent-static render path
    (`BlendState::NonPremultiplied`) instead of the opaque one.
  - Verified live via headless EasyGL screenshots at multiple camera distances/angles (temporary
    debug spawn overrides, reverted before committing): the tip now shows a clean, properly
    alpha-blended teal spike with genuine see-through transparency where the source art is
    transparent, no black anywhere. Full 5-tool suite + both backends re-verified.

- **Implemented the fan hazard (2026-07-11, plan.md `E3D-MIG-149`), the last of Phase 14's original
  10 mechanics besides the water breath gauge (`148`).** Verified directly against real
  `Decor::IsVentillo` source (`Decor.cpp:7667-7752`), which found and corrected a real inaccuracy
  in `mobile-eggbert-reference/12-hazards-and-interactables.md`'s own prior summary (icons
  127/128/130/131/133/134/136/137 are the 4 head icons' own idle animation frames, NOT "air-column/
  trail tiles" as previously documented — the real trail-continuation icons are 110/114/118/122, a
  wholly separate not-yet-render-decided tile family never placed anywhere yet, so only the
  head-tile consumption is ported, not the further trail-walk).
  - New `GEWorldRuntime::TryConsumeFan()`: checked one cell above Blupi (matching
    `GetBlockTypeAbove()`'s convention), detects a real fan head icon, clears it to `Air`
    (`ModifDecor(pos,-1)`), and returns the icon so the caller can trigger death (real channel 10).
    Fan head icons are now always non-solid for collision (`GroundHeightAt`'s own new
    `BlockTypes::isFan()` skip). Real sub-tile-band gating and focus/shield/hide/SuperBlupi
    immunity are not modeled — contact anywhere in the cell is unconditionally lethal.
  - **Found a real, deeper collision-architecture limitation while verifying this live (not fixed,
    see §5): `GroundHeightAt()` always resolves a column's floor as the single topmost solid block
    in the WHOLE column**, with no concept of "nearest solid surface at or below Blupi's own
    height. The sample world's south tunnel has a solid ceiling over its own walkable interior —
    that interior's real floor is completely unreachable via normal walking as a result (same root
    cause as the original teleporter bug, this time triggered by a ceiling instead of a pillar).
    Not caught by any earlier test in this session (including the same tunnel's own already-shipped
    switch/saw pair) because none of them walk-tested a genuine multi-column path into that
    specific enclosed interior. Worked around by placing the fan hazard in 2 new open rooms (no
    walls/ceiling, same pattern as the teleporter rooms) instead of the tunnel's own roofed
    interior, which keeps its 2 fans as pure visual confirmation only.
  - New tests in `VerifyBlupiMovement.cpp` (fan non-solid collision) and
    `VerifyInteractionSystem.cpp` (`TryConsumeFan()` behavior) — both pass, plus the full 5-tool
    suite and both backends re-verified. Verified live end-to-end via a headless EasyGL run with
    temporary debug instrumentation (reverted before committing): Blupi genuinely walked into the
    open fan room at `onGround=true`, the fan triggered at the correct position, lives dropped
    3→2, the fan tile became `Air`, and the FIFO respawn correctly kicked in afterward. See
    `plan.md`'s `E3D-MIG-149` entry for full detail.

- **Expanded the bottom-right Blupi animation indicator further with StopEcrase/MarchEcrase/
  Balloon/Teleporting (2026-07-11, plan.md `E3D-MIG-064`), after the user approved transcribing
  their real `table_blupi` icon-frame records.** See §8's newest item and `plan.md`'s `E3D-MIG-064`
  entry for full detail — includes a real bug found and fixed while wiring this up: `Step()`'s
  `m_teleporting` early-return skipped `UpdateAnim()` entirely, so the new `Teleporting` state
  would never have actually displayed during a real teleport.

- **Expanded the bottom-right Blupi animation indicator with a Jump/Air split (2026-07-11, plan.md
  `E3D-MIG-064`, last item of the 2026-07-11 live-playtest feedback batch).** Investigated all 82
  real `BlupiAction` states beyond the indicator's original 5 (Stop/March/Jump/Down/Up) against
  `mobile-eggbert-reference/08-animations.md` and `GEBlupiController`'s own mechanically-tracked
  status flags (`m_ecrase`/`m_balloon`/`m_teleporting`, all already implemented for gameplay
  purposes but never reflected in the indicator). Found exactly ONE state addable without a fresh
  mobile-eggbert data transcription: `Air` (falling, real `BlupiAction` ID 5, distinct from `Jump`
  ID 4/ascending) — `GalaxyEggbertSimple3D::GEBlupiController.cpp` (this same repo, already
  shipped/approved) already has real frame data for it (`kAirFrames = {169, 26, 170, 170, 27}`),
  so porting it is the same "already-approved, not a fresh transcription" precedent the original 5
  states used, not new mobile-eggbert data. Implemented as a velocity-sign split
  (`m_velocityY > 0` while airborne = `Jump`, `<= 0` = `Air`) rather than Simple3D's own
  fixed-3-frame trigger window, since this class models continuous velocity rather than a discrete
  frame-counted state machine. New assertions in `tools/VerifyBlupiMovement.cpp` (Jump→Air
  transition on the way up/down, exact icon values 17/169) — all pass, plus the existing suite
  still passes. Verified building on both backends.
  **User approved transcribing the other 3 candidate states (2026-07-11)**: `StopEcrase`(72)/
  `MarchEcrase`(73)/`Balloon`(66)/`Teleporte`(74)'s real `table_blupi` icon-frame records
  (parsed directly out of `../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Tables.cpp` — frame
  counts cross-checked exactly against `mobile-eggbert-reference/08-animations.md`'s
  already-documented counts: 1/24/16/128, 67 of the 128 Teleporte frames being the real `-1`
  "invisible" sentinel) are now added. `UpdateAnim()`'s precedence checks
  `m_teleporting`/`m_balloon`/`m_ecrase` before the ground/air cascade (real `BlupiAction` has
  only one animation per status regardless of grounded/airborne). **Found and fixed a real bug
  while wiring `Teleporting` in**: `Step()`'s early-return for `m_teleporting` skipped
  `UpdateAnim()` entirely, so the new state would never have actually displayed during a real
  teleport — fixed. New `VerifyBlupiMovement.cpp` assertions cover all 3 states' exact icon
  values; full 5-tool suite + both backends re-verified. See `plan.md`'s `E3D-MIG-064` entry for
  full detail.

- **Added the teleporter pyramid-tip render geometry (2026-07-11, plan.md `E3D-MIG-147`, last
  item of the same user-reported live-playtest feedback batch as the teleporter/fall-death/
  respawn fixes above).** The user's live description of the real icon 330-333 crop ("kolem
  textury teleporteru je černá barva, část teleporteru červené/žluté tlačítko a písmeno alfa...
  modré pozadí by měla být stále renderována na stranu krychle, ale zbytek ten hrot teleporteru by
  měl být renderován pod teleporterem, nanést se na 3d model čtverec a pod ním 4 trojúhelníky")
  both confirmed the existing `DirectionalCube` treatment for the pillar's 4 sides and specified a
  brand-new attachment: a separate 3D "hrot" (tip/spike) hanging below the block.
  - **New Easy3D primitive**: `Easy3D::PyramidTipItem`/`AppendPyramidTipMesh()`
    (`../easy-3d/include/Easy3D/CubeMesh.hpp`, `../easy-3d/src/CubeMesh.cpp`) — an inverted square
    pyramid (1 square top face, matching `AppendFace`'s -Y winding, + 4 triangular sides tapering
    to a single apex point below): 16 vertices, 18 indices (`AppendFace` always emits 4 fresh
    vertices for the square; none are shared with the 4×3 fresh triangle vertices — the doc
    comment's first draft incorrectly said 8, corrected during test-writing). Winding hand-verified
    via cross product, reusing the exact same corner order as `ComputeFaceCorners`'s own -Y face
    (already proven correct elsewhere in the file).
  - **New unit test** in `../easy-3d/tests/test_cube_mesh.cpp` — vertex/index counts, the square
    face sitting flush at `Center.Y`, all 4 apex vertices at the expected position, and the square
    face's winding. This test only actually RUNS when built with `-DEASY3D_LINK_CNA=ON` (plain
    `easy3d_test_cube_mesh_compilecheck` only type-checks it) — built and ran it standalone this
    way to confirm: `easy3d cube mesh test: OK`.
  - **Wired into `GETerrainRenderer.cpp`**: new `IsPyramidTipIcon()` (icons 330-333) and
    `PyramidTipUv()` (crops the tile's own UV rect to its lower ~2/3 — the dark cone/spike part;
    the top ~1/3 is the flat dots+letter panel, not reused for the tip) helpers, plus an
    `AppendPyramidTipMesh` call inlined into `AppendSpecialGeometry()` right after the teleporter's
    existing `DirectionalCube` append (`kPyramidTipBaseSize=0.7`, `kPyramidTipHeight=0.6`, centered
    at the cube's bottom face).
  - **Revised `GEDirectionalCubeTiles.cpp`'s icon 330-333 comment** and both
    `mobile-eggbert-reference/02-tiles.md` and `questionnaire-all-remaining-tiles.md` (added
    dated revision notes, kept the original "Billboard" answer visible for history) to record that
    this session's live "cube + tip" description supersedes an earlier session's questionnaire
    pass, which had called the same 4 icons "Billboard".
  - **Verified live**: regenerated `worlds3d/world001.vwr`, rebuilt `GalaxyEggbertCNA` on both
    backends (EasyGL/Vulkan) and all 5 verification tools (all pass), then took a headless EasyGL
    screenshot at the sample world's teleporter room (temporary spawn-position debug override,
    reverted before committing) — confirms correct rendering: blue cube sides with the real
    dots/emblem-letter texture, a distinct dark teal cone hanging cleanly below the pillar with a
    visible gap above the floor (no clipping).
  - Known first-pass cosmetic tradeoff (documented in `GEDirectionalCubeTiles.cpp`, accepted, not
    fixed here): the cube's 4 side faces still use the WHOLE tile UV (existing convention for every
    entry in that table), so they show a squished copy of the same cone graphic the tip already
    renders in 3D — a minor overlap, not a correctness bug.

- **Fixed fall-death TIMING — it fired almost instantly, should take several seconds
  (2026-07-11, plan.md `E3D-MIG-067`, found while verifying the last-safe-position respawn task
  below, same overall feedback batch).** The user recalled real mobile-eggbert giving a real,
  noticeable multi-second fall (unsure of the exact duration) rather than near-instant death, and
  asked me to verify against the source rather than guess.
  - **Root-caused by directly inspecting a real level file**
    (`../mobile-eggbert/worlds/world001.txt`): its real terrain (`Decor:` grid) occupies only rows
    0-21 of the real 100-row grid, leaving a deliberate ~77-row (~4928 real px) completely empty
    margin before the real row-99 death check (`(end.Y+30)/64 >= 99`, `Decor.cpp:2754`) ever
    fires — a generous "pit of doom" buffer, not a tight threshold, confirming the user's
    recollection was correct.
  - **Also re-verified the real gravity constants directly** (`Decor.cpp:2966-2968`: `end.Y +=
    (int)(m_blupiVitesseY * 2.0); if (m_blupiVitesseY < 20.0) m_blupiVitesseY += 2.0;`, starting
    at 1.0) — a genuinely new finding: real terminal fall velocity is actually 21, not literally
    20 as the existing reference doc's rounded summary suggested (the un-clamped `+=2.0`
    increment overshoots the `<20` gate by one step) — 840 real px/sec (13.125 tiles/sec) at the
    real 20Hz reference rate.
  - Falling the observed real ~4928px margin at these real constants computes to ~6.1 seconds
    (a ~0.5s ramp-up covering the first ~200px, then ~5.6s more at terminal velocity for the
    remaining distance) — several seconds, matching the user's general recollection even if not
    their exact "~4s" guess, and light-years from the old sub-1-second death.
  - **Fix**: `GalaxyEggbertCnaGame.cpp`'s `kFallDeathY` moved from `-5.0f` to `-60.0f`. This
    reproduces a comparable ~6.2s fall using THIS engine's own already-tuned
    `kGravity=25`/`kFallLimit=-10` (not the real tick-domain values, which per plan.md `065`
    aren't cross-checked against this engine's own constants) — matched as an equivalent NUMBER
    OF SECONDS, not the same literal unit distance, since this hand-authored world's own terrain
    (Y range [0,13]) is far shorter than a real level's, so matching the real game's FEEL (a fall
    you genuinely notice) makes more sense here than matching its absolute pixel margin.
  - **Verified live** (temporary debug instrumentation — a spawn override over a genuinely
    floorless column, plus death-detection logging via a lives-counter comparison, both reverted
    before committing): death fired at 6.72 seconds, closely matching the researched ~6.1-6.2s
    estimate, and the last-safe-position respawn (the task that surfaced this issue) correctly
    kicked in afterward.
  - No new automated test — `kFallDeathY` lives in `GalaxyEggbertCnaGame.cpp`, which the existing
    unit-testable surface (`GEBlupiController`/`GEWorldRuntime`/`GEInteractionSystem`) doesn't
    cover; the live verification above is the check for this one, same as the exit-code/window-
    close investigation's own precedent for game-loop-level behavior.

- **Implemented the real last-safe-position FIFO respawn (2026-07-11, plan.md `E3D-MIG-067`,
  same user-reported feedback batch as the teleporter/fall-death fixes).** Verified directly
  against `Decor.cpp:6467-6478` (the update gate) and `6654-6673` (`BlupiAddFifo`).
  - New `GEBlupiController::UpdateSafePosition(bool externallySafe)`/`GetValidX/Y/Z()` — a real
    10-slot FIFO of recent positions, updated once per frame while grounded and not ballooned/
    squashed, plus a caller-supplied `externallySafe` flag (this class only knows its own
    grounded/balloon/ecrase state, not terrain hazard types or teleporter-trigger occupancy — the
    caller computes that from state it already has). Sets the tracked valid respawn position to
    the FIFO's OLDEST entry BEFORE pushing the current one (the real order — this is exactly what
    gives the "don't respawn right where you died" buffer), deduping consecutive identical
    positions the same way the real FIFO does, extended to all 3 axes here (real mobile-eggbert's
    own 2-axis dedup is a direct consequence of having no Z axis at all, not a deliberate 2-of-3
    choice for a 3D engine).
  - `GalaxyEggbertCnaGame::Update()` computes `externallySafe` from the SAME `GetGroundBlockType()`
    (not on Lava/Spike/Saw/Temp/active-Blitz) and `GetBlockTypeAbove()` (not under a teleporter
    trigger) checks already used for the hazard/teleporter logic — real vehicle/shield/hide/
    ledge-teeter/transport-riding/projectile-path checks aren't modeled, same simplification as
    every other mechanic so far.
  - Every respawn call site (the shared `triggerDeath` lambda used by fall/lava/spikes/Blitz/saw,
    and `GEInteractionSystem::DiedThisFrame()`'s handler for enemy/hazard-object contact) now uses
    `GetValidX/Y/Z()` instead of the hardcoded spawn point `(0,1,0)`.
  - **Verified live**: temporary debug instrumentation (forced continuous forward movement +
    periodic position/valid-position logging, reverted before committing) confirmed the tracked
    valid position genuinely lags behind Blupi's live position while walking, and correctly stops
    updating the moment he walks into a wall and stays still (the dedup working as intended) — not
    just synthetic-test behavior.
  - 6 new `VerifyBlupiMovement` assertions (default-before-any-safe-frame equals the spawn point,
    no-op while airborne, no-op when the caller reports unsafe, and the FIFO's lag/buffer
    property across 13 distinct positions — more than the 10-slot capacity, proving the shift-out-
    the-oldest behavior too). Full suite (63/63 unit tests, all 4 verify tools) and live headless
    runs on both EasyGL and Vulkan backends all clean.
  - **A further user-reported item opened from testing this fix**: fall-death currently fires
    almost instantly (well under 1 second) after falling through a gap near ground level, but the
    user recalls it taking noticeably longer (maybe ~4s) in real mobile-eggbert and isn't sure of
    the exact real duration — tracked as a new, separate task (§8) since the real mechanism turns
    out to be an absolute-grid-row check, not a fixed timer, so the real "how long" depends on
    real gravity constants and how far real levels' terrain typically sits above that row; needs
    research against `Decor.cpp`/a real level file before fixing, not yet started.

- **Fixed fall-off-world death, unreachable via normal walking until now (2026-07-11, plan.md
  `E3D-MIG-067`, user-reported in the same live-playtest feedback batch as the teleporter bug).**
  - **Root cause**: `GEBlupiController::GroundHeightAt()`'s fallback for "no solid block anywhere
    in this column" returned `0`, which both its callers (the main vertical landing check in
    `Step()`, and `TryMoveAxis`'s horizontal step-up gate) silently treated as solid ground AT
    Y=0 rather than "there is no floor, keep falling." Confirmed live with temporary debug
    instrumentation (a spawn override placing Blupi over a genuinely floorless column, plus a
    periodic stdout position log — both reverted before committing): dropped from Y=20, Blupi
    fell smoothly down to exactly Y=0.000 and then stopped dead (`onGround=true`) instead of
    continuing to fall — meaning the already-implemented `kFallDeathY=-5.0f` check
    (`GalaxyEggbertCnaGame::Update()`) was never actually reachable through ordinary play.
  - **Fix**: a new `kNoGround` sentinel (`-1`, unambiguously distinct from every real returned
    height, which is always `>= 1`) that `GroundHeightAt()` now returns instead of `0`, with the
    main landing check explicitly excluding it from the clamp condition. `TryMoveAxis`'s own
    step-up gate needed no change at all — `-1 <= m_y + kStepLimit` and `-1 > m_y` already
    evaluate correctly as "allow walking into the column, don't snap up onto a fake floor"
    without any special-casing, a pleasant surprise that kept the fix small.
  - **Re-verified live after the fix**: the same drop now shows Blupi's Y genuinely going
    negative (observed down to -8 over a 3-second fall) and the existing fall-death sound/respawn
    firing correctly once he crosses the threshold.
  - **A real, second-order bug found in the process**: `tools/VerifyBlupiMovement.cpp`'s own
    "faller" test (from `E3D-MIG-060`, one of the very first collision tests written for this
    engine) had been dropping Blupi over a location with NO real floor the entire time — meaning
    it was unknowingly asserting on the OLD BUGGY fallback behavior all along, and would have
    started failing the moment the real fix landed. Moved it to a location with a genuine floor
    (to keep testing real gravity/landing physics), and added a new, separate test asserting the
    opposite property at the original location: Blupi now correctly never lands there and his Y
    goes negative — a direct regression test for this exact bug.
  - Full suite (63/63 unit tests, all 4 verify tools) and live headless runs on both EasyGL and
    Vulkan backends all clean.

- **Fixed the teleporter for real (2026-07-11), redesigning it the same day it shipped, after a
  live-playtest user bug report.** The user tested `E3D-MIG-147` (below) and reported it freezes
  Blupi permanently and never transports him. Re-verified directly against `Decor.cpp:7378-7394`/
  `5593-5606`/`6349-6358`/`7406-7429` a second time.
  - **The user's own diagnosis was correct and is what fixed it**: the shipped design
    (`GEBlupiController::GetBlockTypeInFront()`) checked the cell Blupi FACES — a workaround for a
    real collision-model conflict (a solid pillar can't float above open floor in the same column
    in this engine's simplified collision, confirmed empirically the first time this task was
    attempted). The user explicitly asked for the REAL design instead: move the pillar one block
    up, with open space beneath it, triggering when Blupi walks into that space. This is only
    possible if the pillar itself doesn't participate in ground-height resolution at all — so
    teleporter icons (330-333) are now ALWAYS non-solid for collision (new `IsTeleporterIcon()`
    check inside `GEBlupiController::GroundHeightAt`'s solid-block scan), reproducing real mobile-
    eggbert's genuinely per-tile-independent 2D collision for this one purpose. Detection reverted
    to `GetBlockTypeAbove()` (one cell above Blupi's own position), matching the real geometry
    exactly instead of adapting around it.
  - **A second real bug was found and fixed in the same pass, before this was reported working**:
    testing live (see below) revealed that landing exactly beneath the destination pillar
    immediately re-satisfied the same trigger condition, producing an infinite teleport-back-and-
    forth ping-pong the instant Blupi arrived. Fixed by offsetting `GEWorldRuntime::
    FindTeleportDestination()`'s landing position one cell away (+Z) from directly beneath the
    matched pillar, instead of landing exactly on top of/beneath it.
  - Rebuilt the sample world's two teleporter rooms as open floors with a floating pillar overhead
    (no walls at all needed anymore, since the pillar's own non-solid status is what makes it
    reachable).
  - **Verified live this time, not just via unit tests** — the exact gap that let the original bug
    ship undetected (the first design's unit tests all passed despite the real live bug). Used
    temporary debug instrumentation (a spawn-position override placing Blupi directly under the
    Room A pillar, plus a periodic stdout log of position/grounded/teleporting state — both
    reverted before committing) run headless for several seconds: confirmed the transit fires at
    the correct ~6.4s mark, lands at the exact expected destination coordinates, and — after the
    second fix — settles there with `teleporting=0` and a stable position, no re-trigger loop.
  - Updated 10 `VerifyBlupiMovement` assertions (now includes a direct regression test: Blupi must
    land on the real floor beneath a floating pillar, not on/blocked by it) and 3
    `VerifyInteractionSystem` assertions (destination math updated for the new landing offset).
    Full suite (63/63 unit tests, all 4 verify tools) and live headless runs on both EasyGL and
    Vulkan backends all clean (6245-block/84-MoveObject world load).
  - **This whole redesign was written down as a tracked task first** (per the user's explicit
    request to capture all of their feedback as tasks before starting work) — see the other 4
    tasks from the same feedback batch in §8 (fall-off-world death, last-safe-position respawn,
    teleporter render geometry, animation-indicator richness), still open.

- **Implemented the teleporter (2026-07-11, plan.md `E3D-MIG-147`, picked as the natural
  continuation of Phase 14 after Temp).** Verified directly against `Decor.cpp:7378-7394`
  (`IsTeleporte`), `5593-5606` (trigger), `6349-6358` (completion), and `7406-7429`
  (`SearchTeleporte`, pairing).
  - **A real architectural mismatch was found and resolved**: the real check tests the tile one
    row ABOVE Blupi's feet, which relies on real mobile-eggbert's genuinely per-tile-independent
    2D collision (a tile's solidity has no bearing on the tile below it in the same column). This
    engine's simplified 3D collision instead treats the TOPMOST solid block in a column as that
    whole column's floor (`GroundHeightAt`), which makes "a solid tile floating directly above
    Blupi's own open, walkable column" physically unreachable here — confirmed empirically (a
    first attempt placed a pillar directly above a floor block in the same test column; Blupi
    landed ON the pillar at Y=3, not at Y=1 beneath it as intended). Resolved by checking the
    cell Blupi is FACING instead — new `GEBlupiController::GetBlockTypeInFront()` (yaw-based,
    mirrors `Step()`'s own forward-vector convention) — a natural 3D adaptation: solid teleporter
    pillars (already documented in `BlockTypes.hpp` as "solid pillars") are wall-mounted archways
    you walk UP TO, not overhead tiles you walk UNDER, fully reachable via this engine's normal
    horizontal collision (which has no ceiling/headroom concept to conflict with).
  - New `GEBlupiController::TriggerTeleport(icon)`/`IsTeleporting()`/`GetTeleportIcon()` —
    idempotent, same shape as `TriggerCrush()`/`TriggerBalloon()`/`TriggerSpringBounce()`, gated
    on grounded + not ballooned/squashed (real `!m_blupiAir && !m_blupiBalloon && !m_blupiEcrase`
    — vehicles/focus aren't modeled). Real `kTeleportDuration=6.4s` (`Config::ScaleTime(128)` at
    the 20Hz reference rate, a direct transcription, not an approximation). While teleporting,
    `Step()` fully freezes Blupi via an early return (no turning/movement/jump/gravity at all) —
    matches the real source's own effective behavior, since `m_blupiFocus=false` gates
    essentially every other per-frame block and nothing sets `m_blupiAir` during the transit.
  - New `GEWorldRuntime::FindTeleportDestination()` (real `SearchTeleporte`) — a full-grid scan
    for another cell of the same icon, excluding candidates within a fixed radius of Blupi's own
    position (replacing the real source's exact entry-tile-equality skip, since this engine's
    "adjacent, not inside/above" entry convention doesn't map to an exact tile match the way the
    real 2D "you're standing right there" check does). Returns the matched pillar's own position
    offset one cell in -Z (a fixed, documented landing convention — level data must place a
    walkable cell there, mirroring the real "keep teleporter icons in matched pairs" authoring
    requirement, which this project's own placement below follows). Real cosmetic entry/arrival
    particle effects (`ObjectType92`/`27`) are NOT modeled, same simplification as every other
    hazard/enemy's own real particle effects throughout this session; real channel 71 covers both
    entry and arrival.
  - **Real playable placement, not just proven synthetically**: two small rooms south of the
    tunnel (previously-empty space), each a short walkway ending in a wall with exactly ONE
    `Teleport1` cell embedded in otherwise-`BrickWall` (deliberately not a whole teleporter wall —
    a wide multi-cell match would risk self-matching within the same room instead of finding the
    other one, a real risk this task's own test setup ran into first and fixed the same way).
    Icon 330 is deliberately excluded from the tile exhibition's generic per-icon loop to
    guarantee exactly 2 total occurrences in the world; icons 331-333 remain in the exhibition as
    genuine lone specimens, faithfully demonstrating the real "no partner found -> regains control
    in place" no-op path.
  - **Verification**: 10 new `VerifyBlupiMovement` assertions (facing-detection, trigger/
    idempotency/gating, full freeze during transit, auto-completion after 6.4s) and 3 new
    `VerifyInteractionSystem` assertions (matched-pair destination lookup and the no-partner-found
    path) — both using non-real test icon values (998/999) to avoid interference from the sample
    world's own real teleporter pair, a real test-authoring fix needed after the pair was added
    (the first version used real `Teleport1`/`Teleport2` constants and started failing once the
    real placement existed in the loaded world). Full suite (63/63 unit tests, all 4 verify tools)
    and live headless runs on both EasyGL and Vulkan backends all clean (6257-block/84-MoveObject
    world load, up from 6146, on both).

- **Implemented the Temp/vanishing tile (2026-07-11, plan.md `E3D-MIG-146`, picked as the natural
  continuation of Phase 14 after spring).** Verified directly against `Decor.cpp:7503-7538`
  (`IsPassIcon`/`IsBlocIcon`'s icon-324 special case).
  - Solid for cycle buckets 0-17, passable (Blupi falls through, no damage) only for buckets 18-19
    of a raw, un-scaled `m_time`-driven 20-value cycle (`m_time / 4 % 20 >= 18`) — NOT
    `Config::ScaleDiv`-normalized like almost every other timer in the real source, an explicit
    exception the reference doc calls out (same category as Crusher's own raw `m_time`). **A real
    finding**: at this project's Fps20 reference rate, `Config::ScaleDiv(N) == N` exactly, so raw
    `m_time` and `GEWorldRuntime`'s own 20-ticks/sec `animPhase_` are numerically identical —
    unlike Crusher's own equivalent note, this is an EXACT reuse, not an approximation. New
    static/pure `GEWorldRuntime::IsTempPassableAtPhase(int)`, same shape as
    `IsBlitzActiveAtPhase`/`IsCrusherActiveAtPhase`. No per-cell phase offset in the real source,
    so every Temp tile in a level blinks in perfect lockstep — a single global phase check is
    correct, not a per-cell one.
  - **Architecturally different from every hazard/mechanic implemented so far**: all of those are
    checked post-`Step()` via `GetGroundBlockType()` (a "what am I standing on" query). Temp
    changes whether the tile IS solid ground at all — a collision-shape question — so it had to be
    threaded directly into the collision path itself: `GEBlupiController::Step()` gained a new
    `tempPassable` parameter (default `false`, so every one of the ~20 existing call sites in
    `VerifyBlupiMovement.cpp` is unaffected), passed down into the private `GroundHeightAt()`'s
    own solid-block scan (skips a `Temp` cell and keeps scanning downward when passable, both for
    the main landing check and `TryMoveAxis`'s step-up gate) — so Blupi genuinely falls through to
    whatever's beneath, or keeps falling if there's nothing there.
  - `GEBlupiController` itself stays fully engine-agnostic/decoupled from `GEWorldRuntime` (its
    own stated class-comment design goal, so `Step()` stays scriptable without a live world
    runtime) — the caller (`GalaxyEggbertCnaGame::Update()`) pre-computes the bool from
    `GEWorldRuntime::IsTempPassableAtPhase(worldRuntime_.GetAnimPhase())` once per frame and passes
    it in, the same pattern already used for `blupiCrouching`/`blupiBallooned` in
    `GEInteractionSystem::Update()`.
  - Already playable with no new placement needed — icon 324 is part of the tile exhibition's full
    1..440 icon range, so a real Temp specimen already sits in the sample world.
  - **Verification**: 4 new `VerifyBlupiMovement` assertions (solid-window ground/type detection,
    then a genuine fall-through onto a real Ground block placed one cell beneath, once passable —
    the first attempt used a loop-exit condition (`GetY() > 1.5f`) that stopped too early, right as
    Blupi started falling rather than once he'd actually landed; fixed to loop on
    `!(IsOnGround() && GetY() < 1.9f)`, matching the file's own existing "faller" test's
    `!IsOnGround()`-driven loop shape) and 5 new `VerifyInteractionSystem` phase-boundary
    assertions (bucket 17 solid, 18/19 passable, wraps back to solid at bucket 0 of the next
    80-phase cycle). Full suite (63/63 unit tests, all 4 verify tools) and live headless runs on
    both EasyGL and Vulkan backends all clean (identical 6146-block/84-MoveObject world load on
    both, unchanged since no new world placement was needed).

- **Implemented the spring/bounce tile (2026-07-11, plan.md `E3D-MIG-145`, the first Phase 14 pick
  after Phase 13 closed out).** Verified directly against `Decor.cpp:2835-2911` (the trigger +
  bounce) and `7312-7320` (`IsRessort`, icon 211 detection).
  - **Not a hazard** — the first Phase 14 mechanic implemented so far that launches Blupi upward
    instead of costing a life. Gated on grounded + not already airborne (real
    `!m_blupiNage && !m_blupiSurf && !m_blupiSuspend && !m_blupiAir` — only the last clause is
    relevant here, since swimming/surfing/suspended-on-a-bar don't exist in this engine yet).
  - New `GEBlupiController::TriggerSpringBounce(bool jumpHeld)` — idempotent (a no-op while
    already airborne, matching the real `!m_blupiAir` re-trigger guard), same shape as
    `TriggerCrush()`/`TriggerBalloon()`. Sets one of two real magnitudes depending on whether Jump
    is held at the moment of contact: held=-19, not-held=-10 in the real source's own down-
    positive convention (the two Power/SecretPower-boosted magnitudes, -25/-16, don't apply since
    Power isn't modeled yet, Phase 17).
  - **A real, documented value-conversion technique**: `kJumpSpeed` (this engine's existing normal-
    jump speed) has no documented real-pixel derivation anywhere in this codebase — it was tuned
    by feel, not transcribed. Rather than inventing new absolute numbers for the spring, the two
    new constants (`kSpringBounceHeld`/`kSpringBounceNotHeld`) preserve the REAL PROPORTION
    between the spring's two magnitudes and the real baseline ground-jump velocity
    (`IsNormalJump`'s held+noPower=-16) applied on top of `kJumpSpeed` — the same technique
    `GalaxyEggbertSimple3D`'s own stomp bounce already used (`kJumpSpeed * 0.65f`), not a fresh
    re-derivation.
  - Real vehicle-dismount-first branches (Helico/Over/Jeep/Tank/Skate landing on a spring gets
    forcibly dismounted before the bounce applies) are NOT modeled — no vehicle concept exists in
    this engine yet, same simplification as every hazard/enemy implemented so far.
  - Also ported: the real generic landing-thud sound is suppressed specifically when landing on a
    spring (`Decor.cpp` ~2984, `if (!IsRessort(end))`) — the bounce sound (real channel 41)
    already covers it, so playing both would be redundant.
  - **Already playable with no new placement needed** — icon 211 is part of the tile exhibition's
    full 1..440 icon range (added in an earlier session), so it already sits in the sample world as
    a real walkable specimen; the new bounce behavior applies to it automatically. Deliberately did
    NOT add a bespoke hand-carved placement the way blupih/blupit/the large creature needed specific
    terrain shapes — a spring needs no special surrounding geometry to be demonstrable, unlike those.
  - **Verification**: 6 new `VerifyBlupiMovement` assertions (ground-block detection, trigger
    returns true while grounded, launches airborne immediately, no-op while already airborne, and a
    held-vs-not-held magnitude comparison over 10 steps proving the held bounce launches noticeably
    higher). Full suite (63/63 unit tests, all 4 verify tools) and live headless runs on both
    EasyGL and Vulkan backends all clean (identical 6146-block/84-MoveObject world load on both,
    unchanged since no new world placement was needed).

- **Implemented follower (`ObjectType96`/`97`) wake + homing (2026-07-11, plan.md `E3D-MIG-137`),
  completing Phase 13 (Enemy AI & combat) entirely.** Verified directly against
  `Decor.cpp:9646-9678` (`MoveObjectFollow`, the wake box) and `8025-8064`
  (`MoveObjectStepLine`'s `ObjectType97` branch, the homing step).
  - A dormant `96` promotes to the homing `97` the instant Blupi comes within its padded
    detection box, playing the real wake sound (channel 92) exactly once at the transition. Real
    box is an axis-aligned rect test (the follower's tile padded ±100px on all sides vs. a narrow
    strip through Blupi's own hitbox) — approximated as a circular distance check
    (`kFollowerWakeRadius`, ≈2.06 grid units: 100px padding + a 32px half-tile converted via the
    same 64px/cell convention used throughout `GEInteractionSystem`), same simplification as every
    other proximity test in that file.
  - Once awake, it steps X and Y independently (Chebyshev-style — each axis moves toward Blupi on
    its own, NOT a normalized diagonal) toward Blupi's live position at the real speed of exactly
    1 real px/tick (`kFollowerHomingSpeed` ≈0.3125 grid-units/sec, a direct, non-approximated
    transcription via the 64px/cell conversion). `currentZ` is deliberately left untouched — real
    mobile-eggbert has no Z axis at all (only X/horizontal and Y/vertical exist in the 2D source),
    matching blupih/blupit's own shots, which never touch Z either.
  - **Self-destructs rather than passing through walls**: if the next step would land in a solid
    cell (checked via a local single-point `IsSolidAt` duplicate of `GEBlupiController`'s own
    private helper, same precedent as `ToGridCell`'s existing duplication), the follower goes
    inactive and plays the real explosion sound (channel 10) — the cosmetic `ObjectType9` debris
    object itself is NOT spawned, since no one-shot/auto-expiring decorative-effect system exists
    in this engine at all yet (every hazard death so far already omits its own real debris/shake
    effects for the same reason).
  - Collapses `posStart`/`posEnd` to the new position on every successful step (matching the real
    source's own `posStart = posEnd = end`), which naturally makes the generic shared patrol-turn
    block (`AdvancePatrolStep()`, `E3D-MIG-131`) a no-op for a homing follower via its own existing
    `posStart != posEnd` guard — no separate type exclusion needed, mirroring how the real source's
    generic dwell/advance/dwell/recede block becomes a no-op immediately after this same real
    function's homing branch runs.
  - Contact-kill/pop against the shared kill list was already covered (folded into `E3D-MIG-132`'s
    widened check) — this pass only adds the movement itself.
  - The object exhibition's existing static `ObjectType96` specimen (open ground, no adjacent
    walls) already makes this genuinely playable in the sample world — no dedicated hand-carved
    placement was needed the way blupih/blupit/the large creature needed specific terrain shapes.
  - **Verification**: 3 new `VerifyInteractionSystem` assertions (wake transition on contact,
    gradual ~0.3125-units/sec homing progress over 1s of unobstructed open air — proving it creeps
    rather than teleports, and a hand-carved one-wall corridor proving the blocked-path self-
    destruct) — the first attempt at these caught two real test-authoring bugs: (1) the existing
    wasp-balloon test's synthetic follower injection happened to sit inside real solid tunnel-wall
    geometry at its arbitrary test coordinates, which the new homing logic's blocked-path check
    now actually notices (fixed by moving it to Y=20, above the sample world's real Y range
    [0,13]); (2) a lookup helper matched a homing follower by its `posStartX`, which the homing
    step itself overwrites every frame it moves — fixed to match by `currentZ` instead, which
    homing never touches. Full suite (63/63 unit tests, all 4 verify tools) and live headless runs
    on both EasyGL and Vulkan backends all clean (identical 6146-block/84-MoveObject world load on
    both, unchanged from the large creature task since no new world placement was needed).

- **Implemented the large creature's (`ObjectType54`) turn-dwell-gated grab (2026-07-11, plan.md
  `E3D-MIG-136`, the natural continuation of Phase 13 after blupih/blupit).** Verified directly
  against `Decor.cpp:5867-5913`.
  - Contact is lethal ONLY while the creature is paused mid-turn (`patrolStep` 1 or 3, the real
    `step != 2 && step != 4` gate, implementable now that `131`'s patrol-step state machine
    exists) — walking into it while it's actually mid-walk (`patrolStep` 2 or 4) is completely
    safe. This is the opposite shape from every hazard implemented so far: a *window* of safety
    instead of unconditional lethality.
  - The creature itself is never destroyed by the contact (no `ObjectDelete` in the real branch,
    unlike the shared kill-list types) — it always survives to keep guarding.
  - **Real balloon immunity is modeled**: `blupiBallooned` blocks the whole branch (matching the
    real `!m_blupiBalloon` gate) — unlike the 4 balloon-poppable hazard types (3/16/96/97), there
    is no separate pop path for type 54; contact while ballooned does nothing at all. This is the
    one immunity flag from the real gate (`!m_blupiBalloon && !m_blupiShield && !m_blupiHide &&
    !m_bSuperBlupi`, plus `m_blupiFocus`) that actually exists in this engine — the rest
    (shield/hide/superBlupi/focus) don't, so they're not modeled, same simplification as every
    other hazard.
  - Real contact also destroys Blupi's current vehicle instead of killing him outright if he's
    riding one (channel 10 + `SmallShake`) — NOT modeled, no vehicle concept exists yet, so
    contact always takes the real no-vehicle branch (channel 51). The real unconditional taunt
    icon (mockery `83` regardless of facing) is also NOT modeled — no idle-taunt animation system
    exists in this engine at all yet (cosmetic, same status as type2's taunt-suppression).
  - **Real breaking-adjacent fix, not just new capability**: the sample world's existing large
    creature placement (`tools/GenerateSampleWorld3D.cpp`'s "walled room" guardian, added in an
    earlier session) used the zero-patrol-range `place()` helper — same real guard the platform
    lift and blupih/blupit needed their own dedicated `PlaceMoveObject` fix for earlier, since
    `posStart == posEnd` means `patrolStep` never leaves 1, so the creature would have been either
    always-lethal or (if the gate were read differently) permanently frozen rather than genuinely
    demonstrating the safe/lethal window. Converted to a real 4-unit patrol path
    (x=12..16, between the room's two decorative pillars, directly between the chest and the
    doorway) so both windows are actually playable.
  - **Verification**: 12 new `VerifyInteractionSystem` assertions (safe at `patrolStep` 2 and 4,
    lethal at 1 and 3, creature survives its own lethal contact, balloon immunity blocks it
    entirely with no pop, and the real placement has a genuine patrol range) — the last one caught
    a real test-authoring bug first: naively selecting "first `ObjectType54`" found the object
    exhibition's deliberately-static specimen instead of the playable guardian (`CollectMoveObjects`'s
    spatial ordering, the exact same pitfall already documented for `findPatrollingLift`'s own
    `ObjectType1` selection) — fixed by matching on `posStartX != posEndX` instead. Full suite
    (63/63 unit tests, all verify tools) and live headless runs on both EasyGL and Vulkan backends
    all clean (identical 6146-block/84-MoveObject world load on both).

- **Implemented blupih/blupit stationary shooters (2026-07-11, plan.md `E3D-MIG-134`, picked as
  the next task off `plan.md`).** Verified directly against `Decor.cpp:8878-8969` (attack timing)
  and `7794-7869`/`8095-8098` (the real `ObjectStart` raycast/travel-distance encoding and a fired
  projectile's self-destruct-on-arrival special case) — not just the reference doc, which had one
  detail backwards (see below).
  - Blupih (`ObjectType32`) drops one `ObjectType23` projectile straight down at dwell-frame 21
    during a turn-dwell (`patrolStep` 1 or 3). Blupit (`ObjectType33`) fires two horizontal shots
    per turn-dwell: frame 3 away from the upcoming walk direction, frame 21 toward it.
    **Correction vs. this repo's own `mobile-eggbert-reference/04-enemy-behavior.md`**, which
    described frame 3 as "aimed toward the side it's about to walk" — the actual source does the
    opposite (re-verified twice against the exact if/else condition and speed sign).
  - New `SearchAirDistance()` (`GEInteractionSystem.cpp`) is a real grid-cell raycast (one cell ==
    one real 64px tile) reproducing `SearchDistRight`'s "count clear cells to the next wall"
    behavior, including a real, easy-to-miss nuance: a 0-distance result (no room to travel) still
    plays the attack sound — `ObjectStart` returns a valid slot even on that cancelled path, and
    its sound gate only ever checks for a free object-pool slot, never whether the raycast found
    room. `stepAdvanceTicks = 5 * dist` is a direct transcription of the real
    `ScaleTime(abs(speed*dist/64))` formula (one grid cell already equals the real formula's
    `dist/64` term), not an invented pacing constant.
  - Body contact is deliberately NOT lethal (blupih/blupit are NOT in `IsGenericHazard()`) — only
    the fired projectile is, always fatal on contact (real shield/hide/superblupi immunity gates
    NOT modeled, same simplification as every other hazard so far — none of those concepts exist
    in this engine yet).
  - New `GEWorldRuntime::GetWorldMutable()` accessor (mirrors the existing
    `GetMobileObjectsMutable()` precedent) so test tooling can hand-carve a guaranteed-shape test
    column (ledge-over-a-pit, walled corridor) instead of depending on incidental terrain shape
    elsewhere in the loaded world.
  - Added a real, playable blupih ("turret perch": a 3x3 ledge with a notch open over a 3-cell
    drop) and blupit ("sentry corridor": a narrow walled passage) to `worlds3d/world001.vwr`,
    south of the tunnel (previously-empty space) — not just unit-tested. The blupih's `posEnd`-side
    dwell sits back over solid ledge, so one shot per full cycle is a real "no room" cancellation,
    not a bug; watching a full cycle shows both a live shot and a cancelled one.
  - Verified via 10 new `VerifyInteractionSystem` assertions (vertical raycast distance and
    contact-kill, the "no room" cancellation case, both horizontal shots' distances/directions via
    a hand-carved asymmetric corridor) + full suite (63/63 unit tests, all 4 verify tools) + live
    headless runs on both EasyGL and Vulkan backends.

- **Added an exhibition area to the sample world (2026-07-10, user request): a browsable museum
  of everything the renderer supports.** `tools/GenerateSampleWorld3D.cpp` gained two new slabs
  in previously-empty space, world regenerated (2470 → 6120 non-air blocks, 18 → 82
  `MoveObject`s):
  - **Tile exhibition** (north strip, grid z=1..24): the ENTIRE icon range 1..440, one
    freestanding block per icon at a 2-cell pitch, each rendering via whatever mode the terrain
    renderer assigns it (DirectionalCube/InnerPillarBox/InnerFlatPlate/TripleCrossBillboard/
    water/UniformCube fallback). Hazard tiles are genuinely lethal to step ON — part of the
    exhibition. Reachable by jumping down from the north-hill plateau's north edge (4-block
    drop, well short of the fall-death limit).
  - **Object exhibition** (east slab, grid x=76..97 z=25..63, seamlessly walkable from the
    corridor's east end): every `ObjectType` the renderer has an icon for — 64 types, enumerated
    at generation time via `GEObjectIcons::GetObjIcon` (the renderer's own source of truth,
    linked into the generator the same way `VerifyBlupiMovement` links `GEBlupiController`)
    instead of a hand-duplicated list. Static exhibits (`posEnd == posStart`, nothing patrols),
    but real behaviors stay live: exhibition pickups are collectable (the chest exhibit raises
    the level's treasure total to 5), shared-kill-list hazards kill on touch, the wasp balloons.
  - `VerifyInteractionSystem`'s lift check now selects the lift with `posStart != posEnd`
    (the exhibition adds a deliberately-static `ObjectType1` exhibit, and `CollectMoveObjects`
    ordering is spatial, so "first ObjectType1" could find the stationary one). Full suite +
    both backends re-verified clean.
  - Incidental observation: the unresolved texture-distance-washout bug is STRONGLY visible on
    the tile exhibition (rows fade to white within ~10 units) — the exhibition doubles as a
    ready-made reproduction field for that investigation (§5).

- **Root-caused the SECOND (real) cause of the disappearing HUD, replaced the SpriteBatch HUD
  with a real mobile-eggbert-faithful 3D-quad HUD (`GEHud`), and added mouse drag-look + F11
  fullscreen (2026-07-10).** User re-reported "the icon shows for a second, then disappears"
  after the earlier depth-test fix — that fix was real but only cured EasyGL.
  - **The Vulkan cause (found by frame-150 screenshot instrumentation on both backends — EasyGL
    kept the HUD, Vulkan lost it): CNA's Vulkan backend records ALL SpriteBatch batches BEFORE
    all 3D draws within each frame's render pass** (`VulkanGraphicsBackend::RecordCommandBuffer`
    calls `drawSpritesFor(...)` then `draw3DFor(...)`; the 2D pipeline has depth test AND write
    disabled), so any sprite HUD is painted over by the 3D scene regardless of what the game
    does. `feature/graphics` has the same ordering (its Task 803 SpriteBatch fix addresses a
    different, DepthStencilState-defaulting issue). Fixing the ordering itself is a CNA change
    (needs approval, flagged in §5) — galaxy-eggbert instead **stopped using SpriteBatch
    entirely**: the new `GEHud` draws real 3D quads (BasicEffect + orthographic pixel-space
    projection via the existing `Easy3D::BillboardMeshRenderer` path), submitted last in the
    frame — genuine 3D draws are recorded in true submission order on both backends. Verified by
    the same frame-150 instrumentation: HUD now present on Vulkan at frame 150.
  - **`GEHud` replicates the real mobile-eggbert bottom HUD** (verified directly against
    `Decor::DrawInfo`, `Decor.cpp:1185-1249`, in its original 640×480 reference space, uniformly
    scaled/centered to the real viewport): one `blupi.png` icon 48 per life at (210,417)
    advancing X+=16 (the real fanned row), held keys as `element.png` 215/222/229 at
    (520/530/540, 418), and the treasure counter — `pad.png` icon-15 panel over
    (410,445)-(510,480) with centered "N/M" text at (460,450). **First real text rendering**:
    glyphs come from `Content/icons/text.png`, whose sheet index IS the character's ASCII code
    for the printable range — read directly off the asset's own 16-column/32px grid layout
    (row 2 starts with ' ' at index 32, row 3 with '0' at 48), NOT transcribed from
    mobile-eggbert's `table_char` (per CLAUDE.md's no-table-copying rule; the proportional
    advance widths in `table_char_width` are likewise not copied — a fixed 17px digit advance is
    a documented approximation). The interim bottom-right animation-state indicator also moved
    into `GEHud`.
  - **One CNA quirk found and worked around**: a `BasicEffect` draw with `Alpha < 1` renders
    fine on EasyGL but not at all on Vulkan (verified empirically — the identical panel quad
    appears at 1.0, vanishes at 0.6). The treasure panel therefore uses opacity 1.0 for now
    instead of the real 0.6 (documented at the constant; restore once fixed in CNA — §5).
  - **Mouse drag-look** (user request): holding LMB and dragging rotates the camera around Blupi
    (yaw offset + clamped pitch offset on top of his facing) in both camera modes — orbit in
    third-person, head-turn in first-person; any movement input decays the offsets smoothly back
    to zero so the camera returns behind him. Drag-based (not free mouselook) so it maps 1:1
    onto touch input. **F11** toggles fullscreen via a `GraphicsDeviceManager` (now constructed
    in the game constructor, standard XNA pattern). Neither could be functionally verified
    headlessly — user should test both live.
  - Camera-follow damping itself was left unchanged (the "camera should react with a delay"
    request — it already lags via `kCameraDampingPerSecond=8`; retune later if it still feels
    stiff in play).

- **ROOT-CAUSED AND FIXED the "front/top/side faces don't render" family of bugs (2026-07-10,
  `../easy-3d` commit `44393f5`): `Easy3D::CubeMesh` wound its 4 side faces backwards for XNA.**
  User re-reported the missing-faces symptom with two screenshots (a brick pillar showing only
  2 faces, a staircase see-through to the background from the side) and noted a separate CNA app
  (MeshCraft) shows similar artifacts, suggesting the bug lives in CNA — analysis found otherwise:
  - **CNA's culling is correct.** Under XNA's default `RasterizerState::CullCounterClockwise`, the
    triangles that survive are the ones that appear **visually clockwise on screen** — verified
    three independent ways: (1) CNA's own contrast-checked `easygl_rasterizerstate_cullmode_test`
    (runs on both backends, checks both windings under all 3 cull modes) asserts exactly this;
    (2) real XNA/FNA semantics — FNA SpriteBatch quads are visually CW under that same default,
    as is XNA's canonical tutorial triangle; (3) live in galaxy-eggbert itself — billboards were
    invisible until re-wound to visually-CW (`easy-3d@a26df1a`), through the same
    BasicEffect/default-rasterizer path terrain uses. Also checked: CNA's `feature/graphics`
    branch (125 commits ahead, separate `../cna_graphics` checkout) does NOT touch the cull
    mapping — the suspected "maybe already fixed there" turned out to be a no.
  - **The actual defect**: `Easy3D::CubeMesh`'s `ComputeFaceCorners` wound all faces with the
    OpenGL-textbook **CCW-from-outside** convention — the exact opposite of XNA's. Every cube
    side face was therefore culled whenever viewed from outside; scenes "looked right" only
    because closed cubes showed the **mirrored interiors of their opposite faces** instead
    (indistinguishable on symmetric brick/stone textures). It broke visibly wherever no opposite
    face covered the sightline: pillar front faces missing, staircase side views see-through to
    the background (sightlines exiting through the top faces' culled undersides). Cross-product
    math on the corner table proved the post-`8ab3854` state was internally inconsistent (4 side
    faces CCW-from-outside, ±Y CW-from-outside) — geometrically impossible for all 6 to render
    correctly under any one-sided cull, which was the key tell.
  - **The two earlier easy-3d reversals (`a26df1a` billboards, `8ab3854` ±Y) were partial fixes
    of this same root cause** — both "empirically fixed, not root-caused" per their own comments.
    This completes the set: all 6 cube faces are now CW-from-outside (XNA front-face convention).
    The chosen corner order (top-left, top-right, bottom-right, bottom-left as seen from outside)
    also lands U0-left/V0-top on the visible face, so side-face textures now render upright and
    unmirrored — previously walls showed tiles flipped via the mirrored far face (unnoticeable on
    bricks, but it means every "looks right" judgment ever made on a side face was made on the
    wrong face).
  - **Verified live** (temporary spawn overrides, reverted): the pillar now renders convex with
    both near faces + correct perspective; the staircase side view is solid stone with no
    see-through; the tunnel fan's outer face renders upright and readable (fan blades + button
    panel). Full suite passes (63/63 unit tests + all 4 verify tools), live runs clean on both
    EasyGL and Vulkan (25/25 terrain samples each). easy-3d's own tests updated (winding
    regression test now pins all 6 inward normals with the real rationale) and pass 6/6.
  - **Follow-ups flagged**: (1) the `texture-distance-washout-bug.md` investigation must be
    re-run — its "geometry/winding proven correct" conclusion was reached while side faces were
    showing their mirrored opposites (addendum added to that doc); (2) the FanLeft/FanRight
    base/open face assignments were tuned by live user feedback *under* the winding bug
    (2026-07-10) and may now be swapped/mirrored — the tunnel fan looks correct in the
    verification screenshot, but the user should re-check fans in play; (3) MeshCraft's own
    similar artifacts are consistent with the same GL-convention winding mistake in its own
    mesher (not analyzed, per user request — but CNA itself needs no fix).

- **Fixed two real, user-reported bugs (2026-07-11): the animation-state icon disappearing
  after ~1s, and a sound-related crash after a while of play.**
  - **Icon disappearing**: `device.SetDepthTestEnabled(true)` (set once near the top of
    `Draw()`, for the 3D scene) was never disabled before the 2D `SpriteBatch` overlay draws at
    the end of the function — the icon's screen-space quad was being depth-tested against
    whatever 3D geometry had already written to that pixel's depth buffer. Right at spawn,
    looking down an open area, the depth buffer at that screen corner happens to be far/empty
    enough for the icon to still pass; as soon as the camera turns toward nearer terrain/walls,
    the same screen position gets a much closer depth value and the icon silently fails the
    depth test — matching the exact reported symptom ("visible for about the first second, then
    disappears"). Fixed by calling `device.SetDepthTestEnabled(false)` right before both
    `SpriteBatch` blocks (the debug indicator and the HUD), matching the real, confirmed CNA
    convention for 3D-then-2D-overlay draws (`../cna/examples/demo_avatar_appearance_tint_studio/
    src/TintStudioDemo.cpp`'s own `Draw()` does exactly this). Verified two ways: (1) frame-1
    screenshot unchanged/still correct, (2) a temporary spawn reposition right in front of a
    brick wall (reverted after the check) — the icon rendered correctly on top of close-up
    geometry, the exact scenario that would have failed before the fix.
  - **Sound crash**: the user supplied a debugger backtrace (from their own build) pinpointing
    `GESound::PlayStep` → `GESound::Play` → `SoundEffectInstance::setIsLoopedProperty`, which
    throws `System::InvalidOperationException` if the instance has already been played
    (`SoundEffectInstance.hpp`'s own doc comment: "@throws ... if the instance has already been
    played" — `hasStarted_` is set the first time `Play()` runs and never reset, not even by
    `Stop()`). `GESound::Play()` called `setIsLoopedProperty()` unconditionally on every call,
    including when reusing an already-started instance (the existing "channel already playing,
    reuse the instance" branch) — the second time ANY channel played (e.g. the second footstep
    while walking), it threw, uncaught, and crashed the whole game. Fixed by only calling
    `setIsLoopedProperty()` once, right after actually constructing a fresh instance. **Verified
    empirically, not just by inspection**: before the fix, 3 consecutive live runs (no keyboard
    input, so some sound source other than footsteps must have been double-playing even
    passively) all crashed identically; after the fix, 3 consecutive runs all completed cleanly.
    No automated regression test added — `GESound::LoadContent()` needs a real audio device,
    which every existing scripted verification tool deliberately avoids exercising (see
    `VerifyInteractionSystem`'s own `GESound` comment); the empirical live-run check is the
    verification here instead.
  - Both fixes are engine-agnostic C++ logic, not backend-specific — re-confirmed clean on both
    EasyGL and Vulkan. **A separate, likely pre-existing Vulkan-specific oddity was spotted while
    checking**: the second diagnostic screenshot (`screenshot_hud.png`, captured at the very end
    of the frame) shows a plain blue background with no terrain and un-blended white boxes
    around every billboard under Vulkan specifically, even though the earlier same-frame
    terrain-visibility pixel-sample check (a more rigorous check than eyeballing a screenshot)
    reports 25/25 samples showing real terrain color. Not investigated further — out of scope
    for this fix, flagged for a future pass.

- **Implemented the real shared patrol-turn state machine, unblocking blupih/blupit/large-
  creature (2026-07-11).** The "bigger task" flagged when the wasp work finished — the
  prerequisite most of Phase 13's remaining named enemy types actually need.
  - **Verified directly against `Decor.cpp:8005-8141`** (`Decor::MoveObjectStepLine`), not just
    the reference doc: a real 4-phase cycle — dwell at `posStart` for `timeStopStart` ticks,
    advance to `posEnd` over `stepAdvance` ticks, dwell at `posEnd` for `timeStopEnd` ticks,
    recede back over `stepRecede` ticks, then loop — driven by 4 real per-instance fields that
    were already fully documented in `01-world-file-format.md`'s field table but never actually
    parsed/stored/simulated anywhere in `GalaxyEggbertCNA`.
  - **New `AdvancePatrolStep()`** in `GEInteractionSystem.cpp` implements this exactly, using
    normalized linear interpolation on the elapsed-time fraction (equivalent to the real integer-
    pixel math — can't drift, same reason the real source uses it) at the real 20Hz reference
    tick rate. Applies to every `MoveObject` except platform lifts/crates, which keep their
    existing separate speed-based ping-pong patrol untouched — a deliberate scope choice (already
    shipped/tested, the visual difference for symmetric timing is likely small), not an
    oversight; worth revisiting if that assumption turns out wrong once more content is authored.
  - **Real breaking format change**: `MoveObjectRecord`'s binary payload grew from 29 to 45 bytes
    to carry the 4 new timing fields (`stepAdvanceTicks`/`stepRecedeTicks`/
    `timeStopStartTicks`/`timeStopEndTicks`), with placeholder defaults (2s dwell/3s traversal)
    for hand-authored `.vwr` worlds that don't set them explicitly — real values are level-
    authored per instance, not a per-type constant, so there's no "real" default to transcribe.
    `worlds3d/world001.vwr` regenerated. The mobile-eggbert `.txt` parser
    (`LoadFromMobileEggbertFile`) also now actually captures `stepRecede`/`timeStopStart`/
    `timeStopEnd` instead of discarding them with `%*s` — a real fix, not just new capability:
    real levels' patrol-hazard/enemy `MoveObject`s were previously frozen at `posStart` forever
    even when loaded from a real mobile-eggbert file, not just in hand-authored `.vwr` worlds.
  - Direction-mirrored animation-table selection (which visual table is picked by `posStart.X >
    posEnd.X`) is NOT modeled — no directional walk/turn sprite tables exist for these types yet,
    only simple icon-cycling, so there's nothing for this to select between yet.
  - **This is the real prerequisite `E3D-MIG-134`/`136` (blupih/blupit's dwell-frame-timed
    projectile attacks, the large creature's turn-dwell-gated lethality) both need** — neither
    was attempted in this same pass (both still need their own additional logic beyond just the
    step/time state now being tracked), but both are now unblocked rather than blocked on missing
    infrastructure.
  - **Verification**: found and fixed a real bug in the new `VerifyInteractionSystem` test while
    writing it — the first version used a zero-dwell patrol object and checked position at a
    fixed frame count, but with zero dwell the object immediately starts receding the instant it
    reaches `posEnd` (no stable resting point to sample), so a frame count chosen to land "after
    the advance" actually landed partway through the *next* recede phase, producing a confusing
    partial-distance reading instead of a clean pass/fail. Fixed by using a symmetric ~4s cycle
    with real (non-zero) dwell at both ends, giving wide, timing-forgiving sampling windows.
    Extended `MoveObjectRecordTests`' existing round-trip test to cover the 4 new fields too.
    Full suite and both backends' live runs re-confirmed clean -- the app itself loaded and ran
    correctly under Vulkan (identical block/object/sound counts to EasyGL, no errors), but
    `timeout 8` didn't actually terminate that particular run (still running 5+ minutes later,
    had to `kill -9` it manually) -- a one-off process-cleanup quirk, not a regression from this
    change (every earlier `timeout`-bounded run this session, including Vulkan ones, worked as
    expected).

- **Implemented the wasp's "balloon" status and its hazard-pop interaction (2026-07-11).**
  Continuing Phase 13, still on "these bigger tasks" per the user's standing request.
  - **Verified directly against `Decor.cpp:5826-5863`** (the trigger) **and `5766-5781`** (the
    interaction with other hazards while ballooned) — not just the reference doc.
  - **New `GEBlupiController::TriggerBalloon()`/`IsBallooned()`/`PopBalloon()`**: same
    idempotent-re-trigger-guard shape as `TriggerCrush()` (a no-op while already ballooned,
    matching the real `!m_blupiBalloon` check), real ~10s duration (**a real finding**: the
    "100-tick" figure in the reference doc is the raw counter value, not literal ticks — it
    decrements every `Config::ScaleTime(2)` ticks, the exact same pattern as Crusher's own
    timer, so the real duration is 10s, not 5s as a literal reading would suggest). Reduced
    gravity while active approximates "floats rather than dying" — the real source sets this up
    as a pure status flag other code branches key off of, no specific fall-speed constant to
    transcribe.
  - **Contact does not kill Blupi or destroy the wasp** — `GEInteractionSystem` gained
    `BalloonTouchedThisFrame()` (signaled every frame Blupi overlaps a wasp; the real
    once-per-trigger guard lives in `TriggerBalloon()` itself, not here, since
    `GEInteractionSystem` has no access to Blupi's current balloon state).
  - **The real hazard-pop interaction is implemented, not just the status flag**: while
    ballooned, touching exactly 4 of the 8 shared-kill-list types — `3`/`16`/`96`/`97`, confirmed
    via the real source's if/else-if chain (the pop check comes first and is mutually exclusive
    with the kill check right after it) — pops the balloon instead of killing, and does **not**
    destroy the popping hazard either (the real pop branch has no `ObjectDelete` call, a detail
    that would have been easy to get wrong by assuming symmetry with the kill branch). Types
    `2`/`4`/`17`/`20` are never eligible for the pop branch at all — they still kill Blupi even
    while ballooned. New `IsBalloonPoppableHazard()` helper and `BalloonPoppedThisFrame()` signal
    implement this precisely.
  - Real entry sound (channel 40) and recovery sound (channel 41 — **confirmed the same channel
    Crusher's own recovery already uses**, so this is a generic "timed status expired" cue, not
    hazard-specific) are both wired; the recovery sound is played from a single
    before/after `IsBallooned()` comparison at the end of `Update()` (not right after `Step()`,
    unlike the analogous Crusher check) since the balloon can end either from `Step()`'s own
    natural timeout or from `interaction_.Update()` calling `PopBalloon()` later the same frame —
    one comparison point catches both causes correctly.
  - Already playable — a wasp was already placed on the north-hill plateau in
    `worlds3d/world001.vwr` from an earlier session's world-redesign work, no new placement
    needed.
  - **Verification**: 9 new `VerifyBlupiMovement` assertions (trigger/idempotency/reduced-
    gravity/pop/auto-recovery) and 9 new `VerifyInteractionSystem` assertions (wasp/follower/
    bulldozer injected synthetically, proving the pop-vs-kill split is type-correct). Found and
    fixed a real test-design bug while writing the bulldozer assertion: `VerifyInteractionSystem`
    shares one `GEInteractionSystem` instance across the whole file, so by the time this section
    ran, `Lives()` had already been driven down by earlier sections' deaths — the naive
    `Lives() == before - 1` assertion could legitimately wrap through another game-over reset
    instead, same as the already-handled case earlier in the file; fixed by accepting either
    outcome. Full suite and both backends' live runs re-confirmed clean.

- **Widened the generic hazard contact-kill to the real full shared kill list (2026-07-11),
  starting Phase 13 (named enemy behavior).** Re-reading `Decor.cpp:5782-5816` while
  double-checking the earlier ObjectType2/3 work revealed that source block is the real shared
  contact-death check for **8 types**, not 2: `ObjectType2`/`3` (already covered) plus `4`
  (bulldozer), `16` (spider), `17` (fish), `20` (bird), and `96`/`97` (follower, both dormant and
  awake). The only difference the real source makes between them is purely cosmetic (17/20 get a
  bigger screen-shake + a different explosion `ObjectType`), not behavioral — so
  `GEInteractionSystem`'s `IsGenericHazard()` now covers all 8 with one check instead of 8
  separate branches that would all do the identical thing.
  - Real per-type quirks NOT modeled (documented, not oversights): type2's wider "thrown object"
    anticipation box, type17/20's bigger explosion effect, spider's specific 9-frame crawl (may
    already be covered by existing generic animation handling, not separately verified), and
    **follower 96/97's real homing-toward-Blupi movement** — a genuinely separate feature from
    contact-death; followers are currently just static/patrol `MobileObjSpec`s like any other
    placed object, so an un-homing follower still correctly kills Blupi if he walks into it, but
    it won't chase him.
  - Added a real spider (`ObjectType16`) placement to `worlds3d/world001.vwr`'s south tunnel so
    this is genuinely playable, not just proven by a synthetic-injection test.
  - **Verification**: a new `VerifyInteractionSystem` block injects a synthetic `ObjectType16`
    instance directly into the loaded world's `MobileObjSpec` list (via the existing
    `GetMobileObjectsMutable()` accessor) to prove the widened check works before the real
    placement above existed — confirms 1 life lost and the spider destroyed, identical to the
    already-verified `ObjectType2`/`3` behavior. Full suite and both backends' live runs
    re-confirmed clean.
  - Remaining Phase 13 work: blupih/blupit (need real projectile-spawn logic, not a plain contact
    check), wasp (non-lethal status effect instead of a kill), the large creature (lethal only
    during a specific window), and follower 96/97's homing AI.

- **Implemented the Saw hazard and its real switch-linking mechanic (2026-07-11), completing all
  5 real terrain hazard tiles.** Continuing "these bigger tasks" per the user's explicit request.
  - **Verified directly against `mobile-eggbert/.../Decor.cpp:7131-7148`** (not just the
    reference doc): `ActiveSwitch(bState, cel)` toggles the switch tile's own icon (384
    on/385 off), plays channel 77 ("on") or 76 ("off"), then scans a **fixed 41-cell window**
    (the switch's X ±20, same Y and Z) toggling every matching Saw (378 active) /SawStopped (379)
    tile it finds to match.
  - **New `GEWorldRuntime::TryActivateSwitch(blupiX, blupiY, blupiZ, blupiOnGround)`**: a no-op
    (`std::nullopt`) unless Blupi is grounded and standing directly on a switch tile; otherwise
    toggles it to the opposite state and performs the real 41-cell saw scan, returning the new
    state so the caller can play the matching sound. Wired to Space ("Action", edge-detected the
    same way jump is) in `GalaxyEggbertCnaGame::Update()` — this key was read since 2026-07-05
    but never used until now.
  - **The real X ±20/same-Y/same-Z mapping was already documented in `BlockTypes.hpp`'s existing
    `isSwitch()` comment before this task started** — confirming it as the established convention
    for translating this specific 2D mechanic into the 3D grid, not a fresh design decision made
    here.
  - **Saw contact death** (the other half of `E3D-MIG-142`, alongside the switch mechanic):
    reuses the existing `triggerDeath()` pattern with the real channel 75 (the "cut apart" cue) —
    only the active `Saw` type is lethal, `SawStopped` is a separate `BlockTypes` value so
    `GetGroundBlockType()`'s exact match already excludes it with no extra logic needed.
  - **Added a real switch+saw pair to `worlds3d/world001.vwr`'s south tunnel** (grid (65,0,67)
    switch, (70,0,67) saw, 5 cells apart, within the real ±20 window) — the saw starts safe
    (`SawStopped`) until the switch is pressed, so this is a genuinely playable interactive
    element in the sample world, not just something proven by a unit test.
  - **Verification**: 9 new `VerifyInteractionSystem` assertions against that real placement
    (no-op away from any switch, no-op while airborne, on-toggle, off-toggle, and the linked
    saw's state checked after each). Found a stale-build-artifact issue while testing: regenerating
    `worlds3d/world001.vwr` at the repo root didn't automatically refresh the copy next to
    `build-cna`'s binaries (the CMake `POST_BUILD` copy step only reliably re-runs when the
    target that owns it actually recompiles, not just because a data file it doesn't track by an
    explicit dependency changed) — resolved by copying the regenerated file directly; not a
    product bug, a build-tooling quirk to remember for next time a `worlds3d/` regeneration
    doesn't seem to take effect. Full suite and both backends' live runs re-confirmed clean.
  - All 5 of `BlockTypes::isHazard()`'s real terrain tile types (lava, spikes, Crusher, saw,
    Blitz) are now implemented. Only named-enemy contact (Phase 13, spider/fish/bird/blupih/
    blupit/wasp/creature/follower) remains as unimplemented "touching something hurts you"
    behavior.

- **Implemented the Crusher hazard (2026-07-11).** Autonomous continuation of `plan.md`'s
  backlog, at the user's explicit request to keep going on "these bigger tasks" (Crusher/Saw,
  which need real machinery beyond the simple ground-block-check pattern the earlier
  lava/spikes/Blitz hazards used). Crusher is the first *non-lethal* hazard implemented — unlike
  every prior hazard, touching it squashes Blupi rather than costing a life.
  - **Verified directly against `mobile-eggbert/.../Decor.cpp:5549-5597`, `5180-5197`, and
    `7277-7288`** (not just `mobile-eggbert-reference/12-hazards-and-interactables.md`, though
    that pointed at the right lines).
  - **New `GEBlupiController::TriggerCrush()`/`IsEcrased()`**: real `!m_blupiEcrase` re-trigger
    guard (idempotent — a no-op, returns `false`, while already squashed), reduced move speed
    (`kEcraseSpeedMultiplier=0.5`, a documented approximation — the real movement table's "×4 of
    `m_blupiSpeedX`" figure doesn't cleanly convert to a fraction of `kMoveSpeed` without further
    research), jump blocked entirely while squashed, ~10s auto-recovery
    (`kEcraseDuration=10.0f`, matches the real 100-ticks-decremented-every-2nd-tick duration
    exactly, just as a plain real-time countdown instead of a tick-based decrement). Real entry
    sound (channel 70) and recovery sound (channel 41 — **a real finding**: the same "buff
    expired" channel other timed states like balloon already reuse on their own recovery, not a
    dedicated crusher-only sound) are both wired.
  - **New `GEWorldRuntime::IsCrusherActiveAtPhase()`** approximates the real 3-out-of-10 danger
    window (`m_time/3%10<=2`, confirmed directly from `Decor.cpp:7277-7288`). **A genuinely
    unusual real-source detail found while reading it**: unlike every other timer in the game
    (including Blitz's, added earlier this session), this one runs on `m_time` — a raw frame
    counter that is explicitly NOT passed through `Config::ScaleDiv`, so its real-world duration
    isn't normalized to a fixed reference tick rate the way virtually everything else is. Since
    this class's `animPhase_` only exists at a fixed 20-ticks/sec rate, there's no exact
    equivalent — approximated by reusing `animPhase_` for the same divisor shape (still a ~30%
    duty cycle), documented as a simplification rather than treated as exactly faithful.
  - No vehicle/focus gating modeled — same reason as spikes (neither concept exists in
    `GalaxyEggbertCNA` yet, Phase 17).
  - **Verification**: a new `VerifyBlupiMovement` block exercising `GEBlupiController`'s squash
    state machine directly (trigger/idempotency/reduced-speed/jump-blocked/auto-recovery) — found
    and fixed a real bug in the test itself while writing it (checked `GetX()` deltas for forward
    movement, but at the default yaw=0 forward movement changes `Z`, not `X` — the "reduced
    speed" assertion failed until switched to `GetZ()`). Plus 5 new `VerifyInteractionSystem`
    phase-boundary assertions for `IsCrusherActiveAtPhase()`. Full suite and both backends' live
    runs re-confirmed clean.
  - Saw (the one remaining terrain hazard tile) needs a full switch/toggle system
    (`Decor::ActiveSwitch`, action-button handling, world-grid icon mutation) — a bigger, separate
    piece of work, not attempted in this same pass.

- **Implemented the first enemy contact behavior: generic hazard types 2/3 (2026-07-11).**
  Autonomous continuation of `plan.md`'s backlog — the first `GalaxyEggbertCNA` behavior where
  touching a `MoveObject` (not a terrain tile) actually harms Blupi.
  - **Real behavior verified directly against `mobile-eggbert/.../Decor.cpp:5782-5816`/
    `6547-6614`** (not just `mobile-eggbert-reference/04-enemy-behavior.md`, though that's what
    pointed at the right lines): `ObjectType2`/`ObjectType3` share a kill list — contact kills
    Blupi and destroys the hazard. **A real finding from reading the source directly**: the death
    sound is a genuine 50/50 coinflip (`BlupiDead(Clear1, Clear2)` picks one of the two
    randomly), and `BlupiDead()` itself only plays a sound (channel 74) on the `Clear2` branch —
    `Clear1` plays nothing. Simplified to always channel 74 rather than modeling the coinflip
    (documented, not an invented value — channel 74 is real and correct roughly half the time).
  - **Type3's real duck-immunity IS modeled**: `GEInteractionSystem::Update()` gained a
    `blupiCrouching` parameter (default `false`, so every existing caller/test is unaffected),
    skipping type3 contact entirely while true, matching `MoveObjectDetect`'s real behavior.
    Type2's own quirks (wider "anticipation" hitbox, taunt-suppression, described in the
    reference doc as possibly "a thrown/rolling hazard rather than a walking creature") are NOT
    modeled — cosmetic/reaction polish, not required for the kill itself.
  - **New `GEInteractionSystem::DiedThisFrame()`**: the system has no access to
    `GEBlupiController` (only raw position floats), so it can decrement lives/destroy the hazard/
    play the sound itself, but can't respawn Blupi — `GalaxyEggbertCnaGame::Update()` checks this
    flag after calling `interaction_.Update()` and applies the same fixed-spawn-point respawn the
    terrain hazards already use.
  - **Verification**: new assertions in `VerifyInteractionSystem` against the real sample world's
    two placed `ObjectType2` instances — confirms exactly 1 life lost, the hazard is destroyed
    (not double-counted), and `DiedThisFrame()` correctly clears itself the very next frame (not
    sticky). Full suite + both backends' live runs re-confirmed clean.
  - Every other enemy type (spider/fish/bird/blupih/blupit/wasp/creature/follower) still needs
    its own real per-type behavior (Phase 13) — this covers only the generic 2/3 case.

- **Implemented the Blitz hazard (2026-07-11).** Autonomous continuation of `plan.md`'s backlog.
  Unlike lava/spikes, Blitz needed no immunity-related simplification to implement faithfully —
  `mobile-eggbert-reference/12-hazards-and-interactables.md` confirms "No vehicle immunity — same
  lethality profile as lava", so the only real behavior to replicate was its 100-tick flicker
  timing (`Decor::BlitzActif`): lethal only on even ticks within the first half of the cycle (25%
  duty, ~2.5s of a ~5s period). New `GEWorldRuntime::IsBlitzActiveAtPhase(int)` — static/pure,
  reuses the same 20-ticks/sec `animPhase_` the per-tile animation-divisor system already
  advances at (`Config::ScaleTime(1)`, no new timing mechanism needed) — lets
  `GalaxyEggbertCnaGame` gate the death check on it, and lets a tool test the cycle math directly
  without a live `Game`/`GraphicsDevice`. The real cosmetic emitter tile (icon 304, times a zap
  sound cue only) is NOT implemented — audio polish, not the hazard itself. Verified via 6
  phase-value assertions in `VerifyInteractionSystem` (0/1/48/50/99/100, covering both parities
  and both cycle halves, plus the wraparound at 100). Full suite + both backends' live runs
  re-confirmed clean. Remaining hazard tiles (crusher, saw) both need real machinery beyond a
  simple ground-block check — a survivable squash state and a switch/toggle system respectively
  — so neither was attempted in this same pass.

- **Implemented the spikes hazard (2026-07-11).** Autonomous continuation of `plan.md`'s backlog
  (user request: pick tasks autonomously). Same shape as the just-added lava hazard
  (`GetGroundBlockType() == Spike` → shared `triggerDeath()`), but real channel 51 (the Glu-death
  sound), not lava/fall's channel 8 — `triggerDeath()` now takes the channel as a parameter.
  Real vehicle+focus-gated immunity and the narrow central x-band restriction within the tile are
  both NOT modeled: neither vehicles nor a "focus" concept exist in `GalaxyEggbertCNA` yet
  (Phase 17), and the single-point 3D collision model has no sub-tile position to test against —
  both are documented simplifications, not oversights. Drip (icons 404/410, grouped with spikes
  in `plan.md`'s `E3D-MIG-141`) deliberately NOT implemented — those icons aren't even a
  `BlockTypes` constant yet, blocked on the still-undecided `ThinMechanical` render geometry
  (`E3D-MIG-510`). New synthetic-world test in `VerifyBlupiMovement`; full suite + both backends'
  live runs re-confirmed clean.

- **Planned a 3D world editor tool in `plan.md`, then implemented the lava hazard
  (2026-07-11).** User request: add the editor to `plan.md`, then continue the backlog.
  - **`plan.md` §6 "Development Tooling — 3D World Editor"** (new): a phased task list
    (`EDITOR-000`-`010`) for an interactive tool to build `.vwr` worlds, replacing today's
    workflow of hand-editing C++ calls in `tools/GenerateSampleWorld3D.cpp` and rebuilding — real
    but slow and error-prone (that workflow already produced 2 real bugs this session: the
    `extraMetadata_` data-loss bug and the zero-patrol-range lift bug, both only caught by
    scripted verification after the fact). Not a mobile-eggbert feature, so the faithful-remake
    rule doesn't govern it; doesn't conflict with the standing "no `.txt`→`.vwr` auto-converter"
    rule either (that rejects *automatic* conversion, this is the *interactive hand-authoring*
    tool that rule already assumes exists). Built on infrastructure that already exists and
    should be reused: `World::loadFromFile()`/`saveToFile()`, `GETerrainRenderer`/`GETileAtlas`,
    `Easy3D::Camera3D`, `MoveObjectRecord`'s embed mechanism — only block-picking (raycast) and a
    palette UI are genuinely new work. Not started yet.
  - **Lava hazard** (plan.md `E3D-MIG-140`): the simplest of the 5 real `isHazard()` tile types to
    implement faithfully — deterministic death, no immunity of any kind, per
    `mobile-eggbert-reference/12-hazards-and-interactables.md`. New
    `GEBlupiController::GetGroundBlockType()` (returns the block type directly beneath Blupi's
    feet, `Air` if not grounded or out of range) lets `GalaxyEggbertCnaGame::Update()` detect
    standing on lava without duplicating grid-conversion math, and lets a tool test the detection
    logic directly without a live `Game`/`GraphicsDevice`. Triggers the same death consequence as
    the fall-off-world case (both now share a `triggerDeath()` lambda: channel 8 sound,
    `LoseLife()`, respawn at the fixed spawn point).
  - **Deliberately does NOT reuse `BlockTypes::isHazard()`** — a `GalaxyEggbertSimple3D`-only
    helper (found unused in `GalaxyEggbertCNA` before this change) that treats Lava/Spike/
    Crusher/Saw/Blitz as uniformly instant-death. Per `12-hazards-and-interactables.md`,
    Spike/Drip have vehicle+focus-gated immunity, Crusher is a *survivable* squash state (not
    instant death at all), Saw is switch-togglable, and Blitz is only lethal on even-ticks of a
    100-tick cycle — reusing the uniform helper would have been unfaithful for all 4. Each stays
    its own dedicated `plan.md` task (`E3D-MIG-141`-`144`), not folded into one generic check.
  - **Verification**: new synthetic-world test in `VerifyBlupiMovement.cpp` (a small
    programmatically-built `World`, not `worlds3d/world001.vwr`, which has no lava placed yet) —
    confirms `GetGroundBlockType()` correctly identifies ordinary ground vs. lava vs. airborne
    (including directly above lava while jumping, which must NOT trigger death). Full suite
    re-run and passing: `GalaxyEggbertWorldsTests` 63/63, `VerifyBlupiMovement` (now 8 checks),
    `VerifyInteractionSystem`, live headless runs on both `build-cna` (EasyGL) and
    `build-cna-vulkan` clean (no errors/exceptions).

- **Rewrote `plan.md` for the Direct CNA + Easy3D direction, then added a lives foundation +
  basic icon-based HUD to `GalaxyEggbertCNA` (2026-07-10/11).** User request: start working
  through `plan.md`'s backlog.
  - **`plan.md` rewrite**: it still planned around the dead Simple3D → U3D/Urho3D/Nova3D
    direction. Rewrote around CNA + Easy3D, grounded in the full `mobile-eggbert-reference/`
    behavioral spec and this file's current status. Dropped the `S3D-*` Simple3D milestones and a
    ~940-line historical documentation-regeneration log (both still in git history); kept and
    re-scoped the ~700-item mobile-eggbert feature-parity checklist against `GalaxyEggbertCNA`
    specifically (old checkboxes recorded Simple3D's status, irrelevant to CNA's); added new
    forward-looking phases (tile geometry decisions, enemy AI/combat, hazards, full crate/lift
    fidelity, doors/keys, secret powers/vehicles/remaining pickups) with no prior home in the plan.
  - **User caught a real design flaw**: `plan.md`'s `E3D-MIG-063` planned to render Blupi himself
    as a billboard. A billboard always rotates to face the camera — under a free-orbiting
    third-person camera Blupi would always visually "face the player" regardless of his actual
    movement direction, since `blupi.png` only has left/right side-view frames drawn for one fixed
    angle (the same defect already flagged for enemy billboards). Decided: reject the billboard
    approach outright. First-person (default camera) never sees Blupi's own model anyway; third-
    person keeps its placeholder Fox model until a real 3D model (`E3D-MIG-069`, now non-optional)
    is scoped.
  - **Lives foundation** (`GEInteractionSystem::Lives()`/`LoseLife()`, plan.md `E3D-MIG-130`):
    starts at 3 (real `GameData` default), egg pickup now grants +1 up to the existing
    `MAX_EGG_COUNT=10` cap (previously tracked but not actually applied to a life count), and
    `LoseLife()` replicates the real `DoorsLost()` behavior — resets to 3 on game-over rather than
    a permanent depletion (no real Lost-screen UI exists yet to make game-over itself visible, so
    a `GameOverCount()` diagnostic exists for a caller/verification tool to observe it happened).
  - **Fall-off-world death** (plan.md `E3D-MIG-067` subset): the one hazard needing no per-tile-
    type work at all — mobile-eggbert-reference/10-blupi-mechanics.md §8 confirms it's checked
    before hazard tiles are even considered. `GalaxyEggbertCnaGame::Update()` now checks Blupi's Y
    against a threshold (a simplification of the real grid-row check, safely below every real
    floor/hazard in the sample world), and on trigger: channel 8 (`07-sounds.md`'s real shared
    "you died" sound for exactly this cause), `interaction_.LoseLife()`, respawn at the fixed spawn
    point (not yet the real 10-slot last-safe-position FIFO). No other hazard/enemy type calls
    `LoseLife()` yet — this is the only wired death cause.
  - **Basic icon-based HUD** (plan.md HUD-001/005/006/007): life icons (`blupi.png` icon 48,
    Blupi's head) one per life, bottom-left; key icons (`element.png` 215/222/229, red/green/blue)
    top-left, shown only while held. No text rendering exists anywhere yet (no `text.png`
    glyph-atlas layout has been identified — see plan.md MENU-083..087), so numeric HUD elements
    (treasure "N/total" counter, score, timer) remain unimplemented; counts are shown via icon
    repetition instead. Reuses `blupiIconBatch_`/`blupiIconTexture_` and the already-loaded
    `objectTexture_` (element.png) — no new texture loads.
  - **New diagnostic**: a second one-shot screenshot, `screenshot_hud.png`, written after the full
    frame (including the new HUD/billboards) — the pre-existing `screenshot.png` is captured
    earlier, right after the opaque terrain pass, and deliberately doesn't include HUD/billboards.
  - **Verification**: `VerifyInteractionSystem` extended with lives/game-over assertions (found and
    fixed a real bug in the new test itself while writing it — a `for (int i = 0; i < interaction.
    Lives(); ++i)` loop re-evaluated `Lives()` every iteration, so it under-counted once `LoseLife()`
    started changing it mid-loop; fixed by snapshotting the count once before the loop). Full suite
    re-run and passing: `GalaxyEggbertWorldsTests` 63/63, `VerifyBlupiMovement`,
    `VerifyMoveObjectTypesCna`, `VerifyBigDecorParsingCna`, `VerifyInteractionSystem`. Live headless
    runs on both `build-cna` (EasyGL) and `build-cna-vulkan` confirmed clean (no errors/exceptions,
    matching block/object/sound counts), `screenshot_hud.png` visually confirms life icons render.

- **Added the first interactive-object system to `GalaxyEggbertCNA` (2026-07-10) — platform lift
  patrol, crate push, and pickup collection now actually work.** User request: start the
  interactive-object system. New `src/GalaxyEggbertCNA/Game/GEInteractionSystem.{hpp,cpp}`.
  - **`MobileObjSpec` gained live state** (`GEWorldRuntime.hpp`): `current{X,Y,Z}` (starts equal to
    posStart, moves independently — every render loop in `GalaxyEggbertCnaGame.cpp` now reads these
    instead of the static `posStart{X,Y,Z}`, so movement is actually visible), `direction` (patrol
    ping-pong), `active` (false once a one-shot pickup is collected — skipped by every render loop
    too, so it stops rendering). `GetMobileObjectsMutable()` added alongside the existing const
    accessor.
  - **A real, live bug found and fixed the same session it was introduced**: the platform lift added
    in today's earlier world-redesign commit was placed via a `place()` helper that always sets
    `posEnd == posStart` — zero patrol range, so it would have sat permanently stationary despite
    the new patrol code. Caught by the new `VerifyInteractionSystem` tool (see below), not by eye;
    fixed in `tools/GenerateSampleWorld3D.cpp` with a real path (plateau y=4 to crow's-nest y=8),
    `.vwr` regenerated.
  - **A real `ObjectType12` identity question resolved before implementing crate push**: `mobile-
    eggbert-reference/03-objects.md` labels `ObjectType12` "Explosion/visual effect", which would
    have made "crate push" an invented mechanic for a wrong type. Resolved by checking
    `GalaxyEggbertSimple3D`'s already-shipped, real `GEDecorSystem.cpp`, whose crate-push code
    explicitly cites the real mobile-eggbert function name `TestPushCaisse` for `ObjectType12` —
    trusted over the reference doc's icon/channel-only guess, since it cites an actual verified
    function name, not an inference.
  - **Behavior ported from 2 sources, not 1**: patrol ping-pong movement and crate push (X-axis
    only, adjacency/lane/floor-support/occupancy checks, 0.4s-equivalent cooldown via per-frame
    re-checking) come from `GEDecorSystem.cpp`'s proven logic. Pickup semantics (which sound, whether
    the object is actually removed) come from the more carefully-researched `mobile-eggbert-
    reference/13-object-pickups.md`/`07-sounds.md` instead, which **corrects 2 real mistakes**
    `GEDecorSystem.cpp`/its own `GESound` shortcuts would have propagated: (1) mobile-eggbert deletes
    every pickup immediately on contact ("the world object is deleted immediately" — Simple3D's
    `GEDecorSystem` never actually removes treasure/exit, apparently a real gap in that code, not
    intentional), so treasure now correctly disappears too, unlike Simple3D; (2) the real sound
    channels are 11 (treasure/key, or 19 for the set-completing treasure) and 3 (egg), not `GESound`'s
    own `PlayCollect()`/`PlayKey()` → channel 10 and `PlayLife()` → channel 42 (42 is Shield
    activation, unrelated) — this implementation calls `sound.Play(SoundChannel::SoundChannelN)`
    directly with the correct channels rather than reusing those specific shortcuts. Egg pickup also
    replicates the real `MAX_EGG_COUNT = 10` cap (`Decor.cpp:96` — at the cap, touching an egg does
    nothing at all, not even removed). Level-exit goal is gated on having all treasure (win fanfare,
    channel 14, vs. rejection sound, channel 13), debounced to fire once per contact rather than
    every frame, and — unlike the pickups — stays active/visible, since it's a goal marker you touch,
    not a consumable.
  - **Explicitly NOT implemented yet** (see `GEInteractionSystem.hpp`'s class comment for the full
    reasoning): enemy hit/stomp/hazard (no lives/gauge/respawn system exists to make it meaningful),
    riding a moving platform lift (`GEBlupiController`'s collision only tests the static terrain
    grid, not `MobileObjSpec` objects — needs its own collision-system work), and every `IsPickup()`
    type beyond treasure/egg/keys/exit (helicopter, shield, drink, vehicles, etc. — not placed in
    today's sample world, deferred).
  - **New `tools/VerifyInteractionSystem.cpp`** (registered in `CMakeLists.txt`, links `CNA` for
    `GESound`'s audio types but never calls `LoadContent()` so no live audio device is exercised) —
    scripted checks against the real sample world: platform lift `currentY` actually changes over
    simulated time, egg/chest/key each collect exactly once (counter increments, `active` flips
    false, doesn't re-trigger on subsequent frames), crate `currentX` increases after a simulated
    push approach. This is what caught the platform-lift zero-patrol-range bug above.
  - **Verified**: clean build; `GalaxyEggbertWorldsTests` (63/63); `VerifyBlupiMovement`/
    `VerifyMoveObjectTypesCna`/`VerifyBigDecorParsingCna`/`VerifyInteractionSystem` (all `ALL CHECKS
    PASSED`); live headless run, no crash/errors, matching MoveObject/block counts.

- **Added real sound playback to `GalaxyEggbertCNA` (2026-07-10) — first sound support on this
  target.** User request: reuse mobile-eggbert's sounds directly, then integrate them. New
  `src/GalaxyEggbertCNA/Game/GESound.{hpp,cpp}`.
  - **Assets**: the same 93 real WAV files (`Content/sounds/sound000.wav..sound092.wav`) already
    copied build-time from mobile-eggbert alongside icons/backgrounds (`CLAUDE.md`'s asset-reuse
    table already lists sounds as approved direct reuse) — no new asset-copying setup needed, they
    were already landing next to the binary.
  - **Playback**: CNA's real `Microsoft::Xna::Framework::Audio::SoundEffect`/`SoundEffectInstance`
    (the same API `../cna/examples/demo_sound` uses), loaded via the direct-path `SoundEffect(path)`
    constructor — not `ContentManager`, since that's already repointed to `avatars3d/` for the
    third-person placeholder model and repointing it again for `Content/sounds/` would conflict.
  - **Data reused, not re-transcribed**: the per-channel volume table and conflict policy (a channel
    already playing isn't restarted, except channel 10) are the exact same real mobile-eggbert data
    already in `GalaxyEggbertSimple3D`'s shipped `GESound.cpp` (itself "ported verbatim from
    mobile-eggbert Sound.hpp tableVolumePitch") — copied as the same real data, not freshly
    transcribed. Pitch is intentionally NOT applied (matches Simple3D's own "(ignored in Simple3D)"
    simplification) — the table's pitch encoding/units were never independently verified against
    real mobile-eggbert source, so applying it blind risked audibly wrong pitch shifts.
  - **Wired up**: the only 3 events currently triggerable given CNA has no interactive-object system
    yet — jump (`SoundChannel1`, edge-detected on the jump key, gated on `wasOnGround`), landing
    (`SoundChannel4`, `onGround` false→true transition), and a footstep loop while marching
    (`SoundChannel3`, paced by a `kStepSoundInterval` — explicitly flagged in-code as a
    reasonable-sounding approximation, NOT verified against mobile-eggbert's real march-cycle
    timing). Pickup/key/hazard sounds aren't wireable yet (see Simple3D's `GESound` for the full
    real channel list once the interactive-object system exists).
  - **Verified**: clean build on both `build-cna` (EasyGL) and `build-cna-vulkan` (Vulkan);
    `GalaxyEggbertWorldsTests` (63/63); `VerifyBlupiMovement`/`VerifyMoveObjectTypesCna`/
    `VerifyBigDecorParsingCna` (all `ALL CHECKS PASSED`); live headless runs on both backends report
    "sound loaded — 93/93 channel(s)"; a temporary smoke test called `PlayJump()`/`PlayStep()`/
    `PlayLand()` directly and confirmed no crash/exception before being removed.

- **Redesigned `worlds3d/world001.vwr` from a tech-demo showroom into an actual small playable
  level (2026-07-10).** User request: "should look like a normal future Galaxy Eggbert world —
  Blupi walking through, collecting chests, facing enemies, collecting eggs — no 60x60 flat area,
  needs real underground/height variation and movable blocks; look at mobile-eggbert's 2D worlds for
  inspiration, but this is 3D." Implemented in `tools/GenerateSampleWorld3D.cpp`, regenerated the
  `.vwr`.
  - **What changed**: the old 41x41 flat `RockPile` floor is gone, replaced by a narrow walkable
    path. A new terraced north hill (5 steps up to a small plateau, then a second platform lift up
    to a "crow's-nest") adds real height variation. A new south tunnel is a genuinely enclosed
    corridor — floor, side walls, AND a ceiling (grid y 1-3), not just a lower Y — since the world
    grid can't go negative; enclosure is what reads as "underground", the same way a real cave does
    regardless of absolute elevation. The staircase/wall-collision room from before is unchanged.
  - **What's populated, and why those exact numbers**: not invented — checked real MoveObject:
    type= counts directly across `../mobile-eggbert/worlds/world011/013/014/021/022/023.txt` (188
    real objects total). Crates (`ObjectType12`) are the single most common type in real levels (35
    of 188), then standard patrol enemies (`ObjectType2`, 31), eggs (`ObjectType6`, 22), chests
    (`ObjectType5`, 21), platform lifts (`ObjectType1`, 20). The new world places 4 crates lining the
    tunnel, 4 eggs, 4 chests, 2 platform lifts (the existing staircase-area one is untouched, plus
    the new hill one), 3 enemies (1 standard patrol `ObjectType2` x2, 1 wasp `ObjectType44`, 1 large
    creature `ObjectType54`), 1 key (`ObjectType49`), and 1 level-exit goal (`ObjectType7`) in the
    crow's-nest — a real, level-shaped mix, not the old flat 67-type catalog dump.
  - **What was removed**: the old isolated "demo row" of ~14 one-of-each render-mode specimen
    blocks (icons 2/25/126/392/49/76/384/77/53/368/15/16/17/18) and the mega-grid placing all 67
    confirmed `ObjectType`s side by side. A few were relocated into real structural roles instead of
    dropped outright: the platform grate (icon 200) is now a real floor grate over a shallow pit at
    the tunnel's west end, 2 fans (`FanLeft`/`FanRight`) are built into the tunnel's south wall as
    ventilation, the water hazard (`Water1`) sits on the tunnel floor, and the grass-top demo (icons
    107/108/109) moved from a flat floor patch onto the new hilltop, where grass actually makes
    sense. The rest (11 single-face/axis specimen icons with no obvious structural role) were cut
    rather than force-fit somewhere — a scope reduction the user's own request asked for, not a
    silent one.
  - **What's UNCHANGED, deliberately**: the exact spawn point, staircase (grid x 29..20, z 45-54),
    and the wall-collision corridor out to grid x=5 — `tools/VerifyBlupiMovement.cpp` hardcodes
    precise coordinates against this one path, so it stays geometrically identical; only the
    surrounding area (previously a flat 41x41 plain) was free to redesign. The walled room on the
    hill is also unchanged, just no longer empty — it now has 1 chest guarded by a large creature and
    1 more patrol enemy near the doorway.
  - **Verified**: clean build (`build-cna` EasyGL and `build-cna-vulkan` Vulkan); `GalaxyEggbertWorldsTests`
    (63/63, engine-agnostic, unaffected by world content either way); `VerifyBlupiMovement` (all
    checks pass — the tested corridor's exact behavior, Y thresholds, and wall-collision X are all
    unchanged); `VerifyMoveObjectTypesCna`/`VerifyBigDecorParsingCna` (unaffected, test against real
    mobile-eggbert `.txt` files directly, not this `.vwr`); live headless runs on both backends
    confirm the new block/MoveObject counts (2470 blocks, was 2842; 17 MoveObjects, was 69) and no
    crashes/errors; debug-camera screenshots of the hill/plateau/crow's-nest, a wide overview, and
    the tunnel interior all show the new terrain rendering as intended; all debug camera overrides
    reverted before commit (confirmed via `git diff`).

- **Fixed ObjectType5/6/7's animation divisors — mistranscribed as each real value x3, so the
  treasure chest and 2 neighboring collectibles animated 3x too slowly (2026-07-10).** User report:
  "the electric-arc billboard's slowness is faithful to mobile-eggbert (confirmed, see the entry
  below) — but the chest ('truhla') isn't this slow in real mobile-eggbert." Dispatched a fresh,
  independent re-verification against the real mobile-eggbert source rather than trusting the
  earlier session's own transcription:
  - `ObjectType38` (the one investigated for the "slow" report) is confirmed to really be
    `Decor.cpp`'s electric-arc effect, not the chest — its `kElectro[90]` table and divisor (every
    tick, `ScaleDiv(1)`) both match real mobile-eggbert byte-for-byte; that animation's chunky look
    (each icon held for 2-5 ticks per the table's own real, repeated entries) is genuinely faithful,
    not a bug.
  - The real "truhla" is `ObjectType5` ("Collectible (treasure)", `mobile-eggbert-reference/
    03-objects.md`). Real `Decor.cpp:8297-8304` divides its phase by `Config::ScaleDiv(3)`;
    `GEObjectIcons.cpp` had `(p / 9) % 22` — divisor 9, exactly 3x the real value 3. The same
    transcription error (real value x3) was found on both neighbors: `ObjectType6` ("extra-life
    egg", real `ScaleDiv(4)` at `Decor.cpp:8309`, had `/12`) and `ObjectType7` ("level-exit goal",
    real `ScaleDiv(3)` at `Decor.cpp:8313`, had `/9`) — a systematic error across all 3, not 3
    independent mistakes.
  - **Fix** (`GEObjectIcons.cpp`): `ObjectType5` `/9`→`/3`, `ObjectType6` `/12`→`/4`, `ObjectType7`
    `/9`→`/3`.
  - **Verified**: clean build; `GalaxyEggbertWorldsTests` (63/63); `VerifyBlupiMovement`/
    `VerifyMoveObjectTypesCna`/`VerifyBigDecorParsingCna` (all `ALL CHECKS PASSED`); live headless
    run, no crash/errors.

- **New confirmed bug, NOT yet fixed: side-face textures wash out to a flat, wrong gray at a
  distance (2026-07-10).** Live follow-up after the `+Y`/`-Y` winding fix directly below — user
  reported "two side walls missing" on a fresh screenshot of the sample-world demo row. Investigated
  and found something more specific than missing geometry:
  - Debug prints confirmed the affected face (icon 25, `NegZ`) has `Visible=true` and a correct UV
    rect that maps to the real, detailed source texture (confirmed via direct `object-m.png` pixel
    sampling — a blue/white-striped/dark-barred design, no flat gray anywhere in it).
  - A live A/B distance test on the exact same face (camera on the same line, only distance changed)
    showed the real texture rendering correctly at ~5 units, degrading to a uniform flat
    `(211,211,211)` gray at ~12 units — confirmed via direct rendered-pixel sampling, not just visual
    impression.
  - **Ruled out**: geometry/winding (already proven correct — this is a texture-sampling symptom, the
    face's shape/position is right), classic texture-atlas mip-bleed (the atlas texture's `MinFilter`
    is plain `Linear`, not a mipmap variant — no mip chain is even generated, per `../easy-gl`'s
    `Texture.cpp`), fog (`BasicEffect::fogEnabled_` defaults `false` and galaxy-eggbert never sets it).
  - **Not yet root-caused.** Went as far as tracing `../cna`'s EasyGL backend's
    `ApplySamplerState`/`Texture.cpp` (texture creation always sets non-mipmap `Linear` min/mag
    filter + `ClampToEdge`; the default `SamplerState` filter value also resolves to non-mipmap
    `Linear` there, not a mipmap variant as first suspected — that specific theory was checked and
    disproven, not confirmed). The exact mechanism causing a *correct* nearby sample to degrade into a
    flat, unrelated color specifically as view distance increases remains unexplained. Going further
    means debugging `../cna`'s own graphics backend, which — per project memory — a separate Claude
    Code agent is actively working on in a sibling directory; deliberately stopped here rather than
    touching that repo without checking with the user first.
  - All debug prints and camera overrides used for this investigation were reverted before finishing
    (confirmed via `git diff`/`git status` — clean, nothing committed for this entry, investigation
    only).

- **Found and fixed the real "see inside the cube" root cause: `+Y`/`-Y` `Easy3D::CubeMesh` faces
  were wound backwards, invisible under CNA's real default cull state (2026-07-10, fixed in
  `../easy-3d`).** The prior day's isolated-geometry test (below) only used `RasterizerState::
  CullNone`, which masked exactly this — both windings render under `CullNone`, so it could never
  have caught a real backface issue. Root-caused after the user pointed at a live screenshot
  (`Screenshot From 2026-07-09 22-36-25.png`) and, once I couldn't spot it myself even zoomed in, at
  a freestanding decorative pillar's missing top face specifically ("nevidím... horní stěnu té
  krychle") — reproduced live from 2 angles (near-vertical top-down: a hollow square showing the
  floor "through" the pillar; a normal eye-level angle: the 2 visible side faces meeting at a point
  instead of a flat cap). A debug print confirmed `GETerrainRenderer`'s occlusion-culling logic
  computed `PosY.Visible=true` correctly for the pillar's top cell — ruling out
  `GETerrainRenderer.cpp`/`GEDirectionalCubeTiles.cpp` entirely and pointing at `Easy3D::CubeMesh`'s
  geometry itself.
  - **Fix** (`../easy-3d/src/CubeMesh.cpp`): reversed the `+Y`/`-Y` corner order in
    `ComputeFaceCorners`. The 4 side faces (`+-X`/`+-Z`) already used "CCW as seen from outside" and
    render correctly; `+Y`/`-Y` used the SAME textbook convention, which is exactly what silently
    failed to rasterize. The fix makes `+Y`/`-Y` the opposite of textbook-CCW — not yet root-caused
    at the view-matrix/projection level (a plausible but unconfirmed guess: an up-axis-specific
    handedness quirk, `../cna-craft`'s `ChunkMesher.cpp` documents CNA as left-handed elsewhere), but
    empirically confirmed correct on both EasyGL and Vulkan.
  - **New regression test** (`../easy-3d/tests/test_cube_mesh.cpp`): checks all 6 `CubeFace` values'
    actual triangle winding against the empirically-correct expected normal (not the textbook one for
    `+Y`/`-Y`) — every previous `AppendDirectionalCubeMesh` test only checked vertex/index counts and
    position bounds, never real winding, which is why this shipped unnoticed. Documented clearly so a
    future "textbook consistency" cleanup doesn't silently reintroduce the bug.
  - **Verified**: `easy3d_test_cube_mesh` and the rest of `../easy-3d`'s test suite (built with
    `-DEASY3D_BUILD_TESTS=ON -DEASY3D_LINK_CNA=ON`) all pass; `GalaxyEggbertWorldsTests` (63/63);
    `VerifyBlupiMovement`/`VerifyMoveObjectTypesCna`/`VerifyBigDecorParsingCna` (all `ALL CHECKS
    PASSED`); live debug-camera screenshots of the same pillar, before/after, on BOTH `build-cna`
    (EasyGL) and `build-cna-vulkan` (Vulkan) — identical fix confirmed on both backends (hollow
    square → solid brick top face); default-spawn screenshot re-checked for regressions (floor/
    platform/walls all still render correctly); debug prints and camera overrides reverted before
    commit (confirmed via `git diff`).
  - **Also reported live, not yet reproduced**: grass-topped cubes are walkable-through (no
    collision) — `GEBlupiController::IsSolidAt` has zero icon-specific exceptions (`!isAir()` only),
    so no obvious code-level cause found yet; needs the specific block's coordinates/icon to
    investigate further. User called this "probably a different problem" from the winding fix above.

- **GalaxyEggbertCNA now defaults to the Vulkan graphics backend instead of EasyGL (2026-07-09),
  and an isolated-geometry unit test ruled out a DirectionalCube face-winding bug.** User request,
  in response to the ongoing "still see inside the cube" investigation: try building/running on
  Vulkan to see whether the issue is backend-specific.
  - **CMakeLists.txt change**: added `set(CNA_GRAPHICS_BACKEND "VULKAN" CACHE STRING ...)` right
    before `add_subdirectory(CNA_HOME)`, deliberately WITHOUT `FORCE` so an explicit
    `-DCNA_GRAPHICS_BACKEND=...` (or an existing build dir's cache) still wins — this only changes
    the *default* CNA picks for a fresh configure (previously EASYGL on Linux, CNA's own default).
    New build dir: `cmake -S . -B build-cna-vulkan -DGALAXY_EGGBERT_BUILD_CNA=ON
    -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF`. Requires the Vulkan SDK/loader + a driver (present in this
    environment: `libvulkan-dev`, `mesa-vulkan-drivers`, AMD Radeon 780M/RADV). Builds clean, all 63
    `GalaxyEggbertWorldsTests` pass identically.
  - **Result: identical visual bug on both backends** — user confirmed live ("vypada stejne jako na
    easy gl"/"looks the same as on EasyGL"). This is a real, useful negative result: it rules out a
    backend-specific GPU driver or graphics-API bug (OpenGL/EasyGL vs. Vulkan are two completely
    independent implementations under CNA), meaning whatever is causing the "see inside the cube"
    perception lives in shared, backend-agnostic code -- either `Easy3D::CubeMesh`'s geometry
    construction (shared by both backends) or `GalaxyEggbertCNA`/`GETerrainRenderer`'s own tile/
    blend logic.
  - **Isolated-geometry test (built and torn down the same day, not committed)**: a standalone
    `Game` subclass built 4 fully-isolated `DirectionalCubeItem`s directly via
    `Easy3D::AppendDirectionalCubeMesh` (bypassing `GETerrainRenderer`/`GEWorldRuntime` entirely),
    with `RasterizerState::CullNone` and depth testing OFF to eliminate every other variable: a
    control full 6-face cube, `CubeFace::NegX` with a full tile UV (matching icon 49's confirmed-
    working pattern), `NegX` with a `SwatchUv`-cropped UV (matching the fan's "base" face
    assignment), and `CubeFace::PosZ` with the same crop. All 4 rendered as solid, correctly-shaped,
    opaque quads (visually confirmed via a zoomed screenshot crop) — no winding/culling bug found at
    the geometry level, for either face orientation or UV size. (A same-day earlier attempt at this
    exact test wrongly reported `NegX` as invisible -- traced to the test's own camera sitting just
    outside the view frustum, not a real rendering defect; re-verified after widening the camera.)
  - **Not yet found**: the actual root cause of "vidím texturu jen zevnitř" / "stále vidím dovnitř
    krychlí" remains open. A default-spawn screenshot (both backends) of the sample world's Platform
    grate (icon 200, `worlds3d/world001.vwr` via `tools/GenerateSampleWorld3D.cpp` line ~112) was
    inspected closely and, once zoomed in past thumbnail resolution, turned out to be correctly
    rendered (individual baluster-shaped pillars with genuine gaps showing the floor behind them,
    not a black void) -- another false lead from insufficient zoom, not a confirmed bug site. Needs
    a specific screenshot from the user's own live session pinpointing exactly where/what they still
    see wrong, since every angle tested here (isolated unit test, default spawn, multiple debug-
    camera positions in the demo row) has rendered correctly on inspection.
  - **Verified**: `GalaxyEggbertWorldsTests` (63/63) on both `build-cna` (EasyGL) and
    `build-cna-vulkan` (Vulkan); live headless runs on both, no crashes/errors, matching terrain-
    visibility diagnostics.

- **Fixed FanLeft/FanRight's base/open faces (reported reversed after the axis fix), rotated their
  newly-textured top face 180 degrees, reverted Fan's animation divisor (reported too fast), and
  root-caused why "elementy" still looked slow (2026-07-09).** Direct live follow-up to the axis fix
  and the per-type divisor fix, both above.
  - **Base/open reversed, confirmed and fixed**: `kFanEntries`' `BaseFace`/`OpenFace` for FanLeft
    (126)/FanRight (129) were swapped (base is now the face AWAY from the blow direction, open is
    the face the fan blows toward) — the opposite of the first axis fix's assignment, corrected the
    same day after live visual feedback.
  - **Top face 180 degree rotation, added**: putting a real texture on `CubeFace::PosY` for the
    first time (previously only ever holding `SwatchUv` flat colors, never a directional texture)
    exposed that face's UV corner order doesn't read upright there. New `FanEntry::RotateTop180`
    flips `PosY`'s `UvRect` (`U0<->U1`, `V0<->V1` — a true 180 degree turn, not a mirror, per
    `AppendFace`'s corner-to-UV mapping) for the 2 fans where `PosY` is now one of the
    always-textured faces.
  - **Swatch alpha ruled out as a "see inside" cause**: direct pixel sampling of `object-m.png` at
    icon 126/129's `kMidSwatchV` sample point confirms full opacity (alpha 255) either way — the
    base/blue face was never at risk of the animated-tile alpha-blend fix (above) making it
    see-through too.
  - **Fan divisor reverted, root cause of the original mismatch found**: the terrain-speed fix above
    matched Fan's divisor to `table_decor_ventg/ventd/venth/ventb`'s divisor 1 (50ms) — reported live
    as "now too fast." Re-reading `Decor.cpp` directly: those 4 tables animate icons **110-125**, a
    separate wind "particle stream" decor effect — NOT the FanLeft/Right/Up/Down BLOCK icons
    (126-137) this file actually renders, which have no dedicated `table_decor_*` entry in
    `Decor.cpp` at all. Reverted Fan to divisor 3 (150ms, same as Water1/Crusher/Marine) for lack of
    a real per-type source value — same-day mismatch, not a persisting unknown.
  - **"elementy" still slow, investigated but not further changed**: directly cross-checked every
    other animated type's new divisor against its real `Decor.cpp` source line (`table_decor_lave`
    div 2, `_eau1`/`_ecraseur`/`table_marine` div 3, `_piege1`/`_temp` div 4, `_scie` div 1) — all
    match exactly, unlike the fan mismatch above. No further code bug found for this complaint;
    flagged to the user as needing a specific icon/screenshot to keep investigating, since the
    verified-correct rates ARE mobile-eggbert's real (slow, by original-hardware design) rates and
    matching them exactly is what "faithful remake" requires — further speedup without new evidence
    of an actual bug would violate that rule.
  - **Verified**: clean build; `GalaxyEggbertWorldsTests` (63/63); `VerifyBlupiMovement`/
    `VerifyMoveObjectTypesCna`/`VerifyBigDecorParsingCna` (all `ALL CHECKS PASSED`); live
    debug-camera screenshots (reverted before commit, confirmed via `git diff`) of FanLeft from
    outside its base (west) face show a solid opaque blue riveted metal panel, distinct from the
    grille-textured side faces — visually consistent with the corrected face assignment.

- **Fixed animated-terrain speed (flat 6fps -> real per-type divisors) and animated-terrain
  transparency (opaque -> alpha-blended) (2026-07-09).** User report, after the billboard alpha fix
  and fan-axis fix above: element animations are *still* too slow and terrain cubes *still* show
  black-instead-of-transparent / "see inside the cube." Both prior fixes addressed real but
  different bugs (MoveObject billboards, fan face axis) — these two were never actually
  root-caused. Investigated fresh (dedicated fork):
  - **Too slow, confirmed**: `GEWorldRuntime::Update()` advanced one shared `animPhase_` at a flat
    6fps (166ms/frame) for every animated tile type. Real mobile-eggbert (`../mobile-eggbert`
    `Decor.cpp`) divides a 20fps base tick per-type via `Config::ScaleDiv(N)` — Saw and all 4 Fan
    variants tick every **50ms** (divisor 1, 3.3x faster than the old flat rate — the fan axis fix
    above made the *direction* right but not the *speed*), Lava every 100ms (divisor 2), Water1/
    Crusher/Water2/Marine every 150ms (divisor 3), Spike/Temp every 200ms (divisor 4). Fixed:
    `GEWorldRuntime`'s raw tick now advances at the real 20fps base (same rate `MobileObjSpec::phase`
    already used) instead of a precomputed 6fps phase; `GETerrainRenderer.cpp`'s new `AnimDivisor()`
    divides that raw tick per-type before indexing each type's frame table in `AnimIcon()`. Water2/
    Marine's real per-*instance* ripple offset (`3 + position%3`) is not modeled — every instance of
    a type still shares one phase; only the wrong base speed (the actual "too slow" complaint) is
    fixed.
  - **Black instead of transparent, confirmed — different cause than the earlier (correct, but
    incomplete) terrain investigation found**: `GETerrainRenderer::Draw()`'s animated-but-non-water
    tiles (`m_animRenderer` — lava/crusher/saw/spike/fan/marine/temp) drew fully opaque, never
    entering the alpha-blend pass that icons 30/31 and water already used. Direct pixel sampling of
    `object-m.png` shows these tiles are genuinely 44-81% transparent pixels (lava 62%, crusher
    53-66%, saw 69%, temp 81%, marine 75%, fan side faces 44%) — not the near-opaque case icons 30/31
    originally motivated that pass for. Drawing them opaque rendered most of each tile as solid
    black. Fixed: `m_animRenderer` now draws inside the existing `NonPremultiplied`/`DepthRead`
    alpha-blend block alongside water and icons 30/31, instead of in the earlier opaque block.
    DirectionalCube face winding/culling and the "open face" geometry-omission mechanism were
    re-verified and are NOT implicated — both correct, as the prior session already found.
  - **Verified**: clean build; `GalaxyEggbertWorldsTests` (63/63); `VerifyBlupiMovement`/
    `VerifyMoveObjectTypesCna`/`VerifyBigDecorParsingCna` (all `ALL CHECKS PASSED`, run from repo
    root); live headless run, no crash/errors, terrain-visibility diagnostic unchanged (still real
    textured samples); debug-camera screenshot of the sample world's fan/demo row before vs. after
    shows the background bleeding through gaps between tile shapes instead of solid black, confirming
    the alpha-blend fix visually; debug-camera repositioning reverted before commit (confirmed via
    `git diff`).

- **Smoothed camera + fixed FanLeft/FanRight's base/open face axis (2026-07-09).** User report:
  the camera moves too fast/snaps, and the horizontal fans (větráky) need work.
  - **Camera damping**: both `FirstPerson` and `ThirdPersonModel` cameras in
    `GalaxyEggbertCnaGame::Update()` used to call `camera_.SetPosition()`/`SetTarget()` straight from
    Blupi's live position/yaw every frame — an instant snap, not a real camera. Each branch now only
    computes a "raw" desired eye/target; both feed into one shared, framerate-independent exponential
    damping step (`1 - exp(-kCameraDampingPerSecond * dt)`, `kCameraDampingPerSecond = 8`) that lerps
    `camera_`'s actual position/target toward the raw value. New `cameraEyeSmoothed_`/
    `cameraTargetSmoothed_`/`cameraSmoothedInitialized_` members hold the smoothed state across
    frames; the first `Update()` snaps instantly (no lerp-in from the origin at load).
  - **Fan base/open axis fix**: `GEDirectionalCubeTiles.cpp`'s `kFanEntries` hardcoded the
    "base"/"open" face pair to the Y axis (top/bottom) for all 4 fan tiles. That's correct for
    FanUp (132, base at bottom, open at top) and FanDown (135, base at top, open at bottom) — both
    unchanged here — but wrong for FanLeft (126)/FanRight (129), which blow horizontally and had
    defaulted to the same Y-axis treatment for lack of an up/down cue in their crops (2026-07-08).
    Fixed: `FanEntry` now stores explicit `BaseFace`/`OpenFace` `CubeFace` values instead of a
    `BaseIsTop` bool, and the fan-processing loop iterates all 6 `CubeFace` values (was hardcoded to
    indices 0-3, the Y-axis case's 4 side faces) skipping the entry's own base/open pair. FanLeft
    now uses `PosX` (base, blue-tinted swatch) / `NegX` (open, blows left); FanRight uses `NegX` base
    / `PosX` open (blows right) — matching the same left→`NegX`/right→`PosX` convention this same
    file already documents for `kDirectionalEntries`. The horizontal base face has no "shora"/"zdola"
    wording to anchor a swatch row to, so it uses `kMidSwatchV` instead of `kTopSwatchV`/
    `kBottomSwatchV`.
  - Also confirmed, not changed: fan tile animation (3-frame cycling per icon, `GETerrainRenderer.cpp`
    lines ~131-134) was already correct and independent of this axis fix; the "blue tint" swatch
    color was already genuinely blue (verified by direct pixel sampling in the prior session), so the
    only real defect was the axis choice.
  - **Verified**: clean build; `GalaxyEggbertWorldsTests` (63/63); `VerifyBlupiMovement`/
    `VerifyMoveObjectTypesCna`/`VerifyBigDecorParsingCna` (all `ALL CHECKS PASSED`); live headless run
    with no crash/errors and the existing terrain-visibility diagnostic still reporting real textured
    samples; debug-camera repositioning (to inspect the sample world's `kFanEntries` demo row) and a
    temporary world-scan print used to confirm block coordinates, both reverted before commit
    (confirmed via `git diff`).

- **Fixed real billboard transparency — element.png/object-m.png/explo.png/blupi.png icon sheets
  render with solid opaque black squares instead of transparent backgrounds (2026-07-09).** Found
  live from a user screenshot: every `MoveObject` billboard showed a black box around its sprite.
  Confirmed via direct pixel inspection that `element.png`'s background pixels are real, correct
  `(0,0,0,0)` RGBA — the texture data was never the problem. No billboard/object effect
  (`objectEffect_`, `objectMPngEffect_`, `exploEffect_`, `blupiObjectEffect_`/`blupi1ObjectEffect_`,
  `bigDecorEffect_`) ever enabled blending, so the GPU ignored alpha and drew the fully-transparent
  background pixels as opaque black. Fixed by bracketing the whole billboard-drawing region in
  `Draw()` with `device.setBlendStateProperty(BlendState::AlphaBlend)`, restored to `BlendState::
  Opaque` right after (before the unrelated 2D SpriteBatch HUD indicator). Does not touch
  `terrainEffect_`/`objectCubeEffect_` (opaque geometry, drawn in a separate, untouched block).
  **Verified**: clean build; live run (no crash, default first-person view visually unchanged);
  debug-camera check confirms a billboard (crate/chest sprite) now renders with no black box, real
  transparency against the background; `GalaxyEggbertWorldsTests` (63/63); debug-camera
  repositioning reverted before commit (empty `git diff` on that line).

- **Third-person camera mode with a real GPU-skinned 3D model, via CNA's `AvatarRenderer` real-
  rendering extension — first version, placeholder model (2026-07-09).** User request: analyze how
  to add a real 3D Blupi model with animations alongside the existing first-person mode (their own
  guess: probably glTF), and implement the camera-mode-switching infrastructure now using a free
  placeholder model, before a real Blupi model exists.
  - **Format answer**: glTF/GLB is correct, but only as the *authoring* format, not what
    `GalaxyEggbertCNA` loads at runtime. CNA already has a complete, previously-unused (by
    galaxy-eggbert) GPU-skinned animation system —`SkinnedModelEXT`/`SkinnedEffect`/`AvatarRenderer`
    (`../cna/docs/avatar-real-rendering-ext.md`, proven end-to-end in `../cna/examples/demo_avatar/`)
    — with its own native `.skinnedmodel.json`/`.skeleton.bin`/`.clip.bin` bundle format, produced
    *offline* from a glTF/GLB source via `../cna/tools/avatar_asset_pipeline/convert_avatar.py
    --embedded-clips` (a one-time, not-run-by-the-engine conversion step). This was a genuinely
    unexpected, much-better-than-assumed finding — not "build a new engine subsystem," but "wire up
    an existing one for the first time."
  - **Placeholder model**: Khronos glTF-Sample-Models "Fox" (CC0 model by PixelMannen, CC-BY 4.0
    rig/animation by @tomkranis — attribution given in `avatars3d/blupi_placeholder/README.md`),
    3 embedded clips (Survey/Walk/Run). Its single mesh primitive had no index accessor (non-indexed
    geometry), which `convert_avatar.py` doesn't handle — patched with a synthetic sequential index
    buffer before conversion (no geometry change) rather than modifying CNA's own tooling. Converted
    successfully: 24 bones, 1 part, 3 clips. Committed to `avatars3d/blupi_placeholder/` (new
    top-level dir, copied next to the binary via a new CMake `POST_BUILD` step, same pattern as
    `worlds3d/`/`textures3d/`).
  - **Camera-mode toggle**: "C" (edge-detected) switches `GalaxyEggbertCnaGame`'s new
    `cameraMode_` between `FirstPerson` (unchanged default) and `ThirdPersonModel` — only actually
    switches if the model loaded (`blupiModelLoaded_`), else silently stays first-person. Third-
    person is a fixed-offset chase camera (behind + above Blupi, looking at him), not a real spring-
    arm/collision-aware orbit camera. `GEBlupiController::AnimState` (Stop/March/Jump/Down/Up) maps
    to the placeholder's 3 clips via a rough, explicitly-documented best-effort substitution (no
    real correspondence exists) — see `BlupiAnimStateToPlaceholderClipName()` and the placeholder's
    own README for the exact mapping table.
  - **Build wiring**: `AvatarRenderer` lives in a separate `CNA_GamerServices` static library, gated
    behind CNA's own `CNA_ENABLE_NET` option (default `ON`) — not linked by `GalaxyEggbertCNA`
    before this; added `target_link_libraries(GalaxyEggbertCNA PRIVATE CNA_GamerServices)`. Model
    loaded via `ContentManager` with its `RootDirectory` repointed at `avatars3d/` (previously
    unused by this class — `getContentProperty()` had zero prior call sites — so repointing it
    can't conflict with anything).
  - **Verified**: clean build (after the `CNA_GamerServices` link fix); live run, no crash, model
    loads successfully (`third-person placeholder model loaded`); temporarily forced
    `cameraMode_`'s default to `ThirdPersonModel` to visually confirm the model actually renders
    (textured, lit, roughly correctly positioned/scaled — not just "no crash") before reverting the
    default back to `FirstPerson` (confirmed via empty `git diff` on that line); default first-person
    view re-confirmed unchanged after the revert. `GalaxyEggbertWorldsTests` (63/63),
    `VerifyBlupiMovement`/`VerifyMoveObjectTypesCna`/`VerifyBigDecorParsingCna` (all `ALL CHECKS
    PASSED`). **Not independently verified**: the exact placeholder scale (0.02, hand-picked from the
    Fox's ~79-unit native height vs. Blupi's ~1.6-unit eye-height scale) and rotation-offset
    alignment (whether the model's own rest-pose "forward" actually matches
    `GEBlupiController::GetYaw()`'s facing convention) — cosmetic tuning, not correctness, and
    explicitly moot once a real Blupi model replaces this placeholder anyway.

- **`UniformCube` platform-lift/crate objects now animate too, closing §8's optional follow-up
  (2026-07-09).** Direct continuation of the previous entry below — the one remaining icon lookup
  still frozen at phase=0. `CubeMeshRenderer` has no in-place UV update API (only construction from
  vertex/index data, unlike a texture-only re-upload), so the cube mesh is now rebuilt every frame in
  `Draw()` (moved out of the one-time `LoadContent()` build) using each `MobileObjSpec`'s real
  per-instance `phase` — same pattern the billboard renderers already used. Types 47/48's
  `kChenille`/`kChenillei` caterpillar-track icon cycles now actually advance; types 1/12 are
  unaffected (static single-icon regardless of phase). Cheap in practice: only a handful of cube
  objects in the sample world. **Verified**: clean build; live run (no crash, `4 platform-lift/crate
  cube object(s) found`); debug-camera check confirms the 2 cube objects still render correctly, no
  visual regression from the LoadContent()→Draw() move; `GalaxyEggbertWorldsTests` (63/63),
  `VerifyBlupiMovement`/`VerifyMoveObjectTypesCna`/`VerifyBigDecorParsingCna` (all `ALL CHECKS
  PASSED`); debug-camera repositioning reverted before commit (empty `git diff` on that line).

- **Real per-instance MoveObject animation timers, and `ObjectType38`'s (electric arc) full
  two-channel behavior, sourced directly from mobile-eggbert with explicit user approval
  (2026-07-09).** Direct continuation of the previous entry below — closes its "element.png channel
  has no representation yet, needs a real animation timer" residual gap.
  - **`MobileObjSpec::phase`** (`GEWorldRuntime.hpp`): new per-instance `float` field, advanced by
    `GEWorldRuntime::Update(dt)` at 20 ticks/second — mobile-eggbert's own reference tick rate for
    MoveObject animation (`Config::ScaleTime(1)==1` at that rate; a genuinely different, faster rate
    than the shared 6fps terrain-tile clock — two distinct animation systems in mobile-eggbert
    itself, not a galaxy-eggbert simplification). Every `GetObjIcon(obj.type, 0)` call site in
    `GalaxyEggbertCnaGame.cpp`'s per-frame billboard loops now passes `static_cast<int>(obj.phase)`
    instead of a hardcoded `0` — every phase-indexed formula already written in `GetObjIcon()` this
    session (most with comments anticipating exactly this) now actually animates, not just
    `ObjectType38`. The once-built `UniformCube` platform-lift/crate path intentionally still uses
    phase=0 (built once in `LoadContent()`, not rebuilt per frame — animating it would need
    per-frame UV re-upload, a separate, not-yet-done change).
  - **`table_electro[90]`**: the real mobile-eggbert animation table (`Tables.cpp`) transcribed with
    explicit user approval this turn ("30-89 animační timery najdeš v mobile eggbertu") — ticks 0-29
    alternate icons 266/267 on `blupi1.png`, ticks 30-89 cycle icons 40-47 on `element.png`, matching
    `Decor.cpp`'s exact channel-switch logic (`~line 8988-9004`: `phase < ScaleTime(30)` →
    `PixmapChannel::Blupi1_12`, else `PixmapChannel::Element`). Real mobile-eggbert behavior is a
    one-shot (the object despawns once `phase>=90`) — simplified to a continuous `% 90` loop here,
    matching every other multi-frame type's existing convention in this function.
  - **`IsBlupiPngSourcedAtPhase(type, phase)`** (new): the phase-aware sibling of the existing
    phase-blind `IsBlupiPngSourced()`. For `ObjectType200`-`203` identical (always `blupi`-sourced).
    For `ObjectType38`, true only while `phase % 90 < 30` — the renderer needs this because a single
    `MoveObject` now genuinely switches texture sheets mid-animation, something no render path could
    do before. Both the element.png billboard loop's skip condition and the blupi-billboard loop's
    inclusion condition in `GalaxyEggbertCnaGame.cpp` now use this instead of the plain, phase-blind
    predicate, so `ObjectType38` correctly migrates from the `blupi1ObjectMeshRenderer_` batch to the
    plain `objectMeshRenderer_`/element.png batch partway through its cycle every frame.
  - **Verified**: clean build; live run (no crash across an 8-second run, ~1.8 full 90-tick cycles at
    the real 20 ticks/sec rate); `GalaxyEggbertWorldsTests` (63/63),
    `VerifyBlupiMovement`/`VerifyMoveObjectTypesCna`/`VerifyBigDecorParsingCna` (all `ALL CHECKS
    PASSED`); debug-camera repositioning toward `ObjectType38`'s catalog slot attempted for a direct
    visual confirmation, but couldn't reliably pinpoint the small billboard at that distance/angle in
    a screenshot — correctness relies on the code-level match against `Decor.cpp`/`Tables.cpp`'s
    exact logic and values (not just a plausible guess) plus the crash-free multi-cycle live run,
    not a visual confirmation this time. Reverted before commit (empty `git diff` on that line).

- **`ObjectType38` (electric arc) now has a real icon — 65/69 → 66/69, closing the icon-coverage
  follow-up from §8 task 1 (2026-07-09).** The "element.png-only simplification, undecided" question
  §8 flagged turned out to be moot: `GetObjIcon()` is always called with `phase=0` (no per-instance
  animation timers exist yet), and tick 0 of the real two-channel animation (`blupi1.png` ticks
  0-29, then `element.png` ticks 30-89) is unambiguously the `blupi1.png` channel — confirmed via
  `03-objects.md`'s own crop filename for this type, explicitly labeled
  `object-type038-icon266-electro-blupi1channel.png`, i.e. icon 266 on `blupi1.png`'s grid, not
  element.png's. Added `ObjectType38` to `IsBlupiPngSourced()`/`UsesBlupi1Texture()` (reusing the
  existing `blupi1ObjectEffect_` render path from the background-image work, no new infrastructure)
  and a `GetObjIcon()` case returning `266`. The `element.png` channel (ticks 30-89) still has no
  representation — needs a real animation timer plus genuine dual-texture billboard support first —
  left as an explicit residual gap, not silently dropped. **Verified**: clean build; live run (no
  crash, `69 MoveObject(s) parsed`, screenshot written); `GalaxyEggbertWorldsTests` (63/63),
  `VerifyBlupiMovement`/`VerifyMoveObjectTypesCna`/`VerifyBigDecorParsingCna` (all `ALL CHECKS
  PASSED`).

- **Fixed `Easy3D::AppendBillboardMesh`'s backface-culling bug at the source (2026-07-09, §8 task
  0) — `MoveObjects`/`BigDecor` billboards are now actually visible on screen, not just correct in
  vertex math.** Direct follow-up to the discovery in the previous entry below. Reversed the fixed
  triangle winding (`0,1,2 / 0,2,3` → `0,2,1 / 0,3,2`, vertex data/UVs unchanged) in
  `../easy-3d/src/BillboardMesh.cpp` (committed there separately, `easy-3d@a26df1a`, own repo/remote
  — not a galaxy-eggbert submodule); updated the pinned `easy-3d/tests/test_billboard_mesh.cpp`
  assertions to match (`easy3d_test_billboard_mesh`: OK, run standalone with
  `-DEASY3D_LINK_CNA=ON`). Removed the local `RasterizerState::CullNone` workaround from the
  background quad (§3 above) — it renders identically without it now, confirming the source fix
  works. **Correction to the previous entry's live confirmation**: the debug-camera check that
  seemed to show "only cube objects visible" was itself flawed — the sample world's MoveObject
  catalog (`GenerateSampleWorld3D.cpp`) happens to place the 2 `UniformCube` types (`47`/`48`) at
  the very first 2 grid slots, so a narrow camera view centered there was never going to show
  billboards regardless of whether they worked. Re-tested with a wider view over more catalog
  slots: a billboard-rendered object is now visible alongside the cubes (was not, pre-fix, per the
  original SpriteBatch/background investigation). Real root cause was still correctly identified
  (confirmed unambiguously via the background quad's clean before/after: required `CullNone` before
  this fix, renders correctly without it after) — only the *supporting* catalog-camera evidence in
  the previous entry was weaker than claimed.
  - **Verified**: `easy3d_test_billboard_mesh` passes (standalone easy-3d build,
    `-DEASY3D_LINK_CNA=ON`); `GalaxyEggbertCNA` clean build/live run (no crash, background still
    renders correctly without `CullNone`); `GalaxyEggbertWorldsTests` (63/63),
    `VerifyBlupiMovement`/`VerifyMoveObjectTypesCna`/`VerifyBigDecorParsingCna` (all `ALL CHECKS
    PASSED`); temporary debug-camera repositioning (twice, to find a view that actually covers
    billboard-type catalog slots) used to confirm live, reverted before commit (empty `git diff` on
    that line).

- **Real mobile-eggbert background images now render in `GalaxyEggbertCNA`, plus a world-level
  `skyRegion` `.vwr` header field to select them — breaking format change (2026-07-09).** User
  request: analyze how to bring mobile-eggbert backgrounds into Galaxy Eggbert (their own hypothesis:
  probably new world-format metadata), then implement it rendering the real PNG, not a derived flat
  color, with an explicit breaking `.vwr` format change and 4 new reserved header fields.
  - **`.vwr` header v2, breaking change**: `VoxelConfig::FormatVersion` 1 → 2;
    `World::loadFromFile` now rejects any non-`2` version outright (`std::runtime_error`) — **old v1
    `.vwr` files no longer load and must be regenerated** (not a silent dual v1/v2 reader, a hard
    cutover, per explicit user request). Header grew 32 → 40 bytes: one of the 3 old reserved
    `uint32` fields became a real `skyRegion` field (`World::skyRegion()`/`setSkyRegion()`, direct
    pass-through of mobile-eggbert's `region=` value, 0-31), and 4 brand-new reserved `uint32` fields
    were added on top (old reserved fields were read-but-discarded and not validated even before this
    change, so repurposing one was safe for old files' zeroed bytes — but the version bump still
    rejects them outright per the user's explicit "break compatibility" instruction, rather than
    silently reinterpreting). 4 reserved fields chosen as a round, modest amount of headroom for
    plausible near-future world-level metadata (e.g. spawn point, still with no `.vwr` equivalent)
    without needing another version bump soon — see `World Format.md`. `GEWorldRuntime::LoadFromVwrFile()`
    now reads the real field instead of always resetting to 0. `tools/GenerateSampleWorld3D.cpp` sets
    `skyRegion=3` and `worlds3d/world001.vwr` was regenerated (required — old file no longer loads).
  - **Rendering, real PNG not a color**: first attempt drew `Content/backgrounds/decorNNN.png` as a
    full-screen `SpriteBatch` backdrop before the 3D scene each frame — broke live:
    `EasyGLSpriteBatchBackend::Begin()` enables alpha blending and never restores it on `End()`, and
    more fundamentally `SpriteBatch`/`BasicEffect` draws don't compose safely when `SpriteBatch` runs
    *before* 3D draws in the same frame (every existing usage, `blupiIconBatch_`, only ever ran it
    last) — the background ended up covering the whole screen with zero terrain visible. Replaced
    with the same proven `BillboardBatch`/`BuildBillboardMesh`/`BillboardMeshRenderer` technique
    already used for `MoveObjects`/`BigDecor`: one huge camera-facing quad (sized from FOV/aspect to
    fill the whole frustum with margin) placed 900 units in front of the camera (< the 1000-unit far
    plane), rebuilt every frame to keep following the camera — real depth-tested 3D geometry, so
    opaque terrain naturally occludes it via the depth buffer with no draw-order fragility.
  - **Major incidental discovery, confirmed live**: this exposed a real, previously-undiscovered
    backface-culling bug likely affecting EVERY Easy3D billboard (`MoveObjects`, `BigDecor`, not just
    this new background quad) — `Easy3D::AppendBillboardMesh`'s fixed vertex winding is back-facing
    under this engine's default `CullCounterClockwise` rasterizer state for a camera-facing quad in
    front of the camera. Confirmed directly: with the debug camera pointed at the sample world's
    MoveObject catalog grid, only the 2 `UniformCube`-rendered platform lifts were visible — every
    nearby billboard-rendered object (the vast majority of the 65 mapped `ObjectType` icons) was
    invisible. Worked around locally for just the background quad (`RasterizerState::CullNone`,
    explicitly restored to `CullCounterClockwise` right after so no other draw call is affected) —
    deliberately NOT fixed at the `Easy3D::BillboardMesh.cpp` source in this pass, since that winding
    is pinned by `easy-3d`'s own `test_billboard_mesh.cpp` and a global fix needs its own
    care/re-verification across every billboard consumer, not a side effect of a background-rendering
    task. Tracked as a new, high-value follow-up in §5/§8 — **likely explains why MoveObjects/BigDecor
    have never visibly appeared correct in any screenshot taken this session**, only verified via
    vertex/index math and non-crash live runs until now.
  - **Verified**: clean build; live run (no crash, background PNG visibly loads and renders through
    open sky, terrain/objects still correctly occlude it, screenshot confirms real
    `Content/backgrounds/decor003.png` art visible, not a flat color); `GalaxyEggbertWorldsTests`
    (63/63, 2 new: `SaveAndLoadPreservesSkyRegion`, `LoadRejectsV1FormatVersion`),
    `VerifyBlupiMovement`/`VerifyMoveObjectTypesCna`/`VerifyBigDecorParsingCna` (all `ALL CHECKS
    PASSED`); temporary debug-camera repositioning used to directly confirm the billboard-culling
    finding, reverted before commit (confirmed via empty `git diff` on that line).

- **Filled in the remaining 16 `ObjectType`s missing from `GEObjectIcons::GetObjIcon()` — icon
  coverage 49/69 → 65/69 confirmed types (2026-07-09).** Direct continuation of the previous entry
  below's follow-up (§8 task 1).
  - **12 `explo.png`-sourced types added** (8/9/10/11/53/90/91/92/93/98/99/100 — explosions/visual
    effects): `explo.png` wasn't loaded anywhere in `GalaxyEggbertCNA` before this, so this is a
    genuinely new texture (confirmed by direct file inspection: 1440×1440 px, 144×144 px tiles, 10
    cols x 10 rows, 100 icons 0-99 — matches `08-animations.md`'s 144×144/10-col claim and pins
    down the previously-unconfirmed row count). Added `GEObjectIcons::IsExploPngSourced()` +
    `GetExploIconUv()`, and a third billboard render path in `GalaxyEggbertCnaGame.cpp`
    (`exploTexture_`/`exploEffect_`/`exploMeshRenderer_`). `53`/`92` (45/128 documented frames)
    would run off the 100-icon grid under a consecutive-icon cycle — first-frame icon only, same
    reasoning as `56`/`57`/`52` in the previous pass. `99`/`100` also return their first REAL-frame
    icon only (both have real leading invisible ticks in mobile-eggbert — a fall-height delay before
    the splash appears — that a static return can't represent without a per-instance animation
    timer, which doesn't exist yet: `GetObjIcon` is always called with `phase=0` today).
  - **4 Blupi-skin types added** (200/201/202/203): confirmed by direct file inspection that
    `blupi.png`/`blupi1.png` are both 600×2040 px, 60×60 tiles, 10 cols x 34 rows, 340 icons,
    identical layout (matches `03-objects.md` line 228). Added `GEObjectIcons::IsBlupiPngSourced()`
    + `UsesBlupi1Texture()` + `GetBlupiIconUv()`. `ObjectType200` sources `blupi.png`,
    `201`/`202`/`203` source `blupi1.png` — two separate textures/effects/renderers
    (`blupiObjectTexture_`/`blupiObjectEffect_`/`blupiObjectMeshRenderer_` and their `blupi1Object*`
    counterparts), deliberately NOT reusing the existing `blupiIconTexture_` (that one is owned by
    the 2D SpriteBatch HUD indicator, not this 3D `BasicEffect` path) even though `200` loads the
    same file. No per-instance tinting exists yet, so `201`/`202`/`203` render identically to each
    other (matches mobile-eggbert's own raw-crop behavior — tint is a render-time effect there too,
    confirmed via `Pixmap::GetBitmap()` returning identical bitmaps for all three channels).
  - **Deliberately still NOT added, still `default: return 0`**: `ObjectType38` (electric arc) —
    the only remaining gap. Real behavior needs BOTH `blupi1.png` and `element.png` in one animation
    (ticks 0-29 vs 30-89); `03-objects.md` explicitly flags the element.png-only simplification as
    undecided, so this needs a decision (accept the simplification and build a genuinely dual-
    texture billboard, or leave it out) before implementing — a smaller, standalone follow-up (see
    §8), not blocked on any missing data the way the explo.png/blupi.png types were.
  - **Verified**: clean build; live run (no crash, `69 MoveObject(s) parsed`, screenshot written);
    `GalaxyEggbertWorldsTests` (61/61), `VerifyBlupiMovement`, `VerifyMoveObjectTypesCna`,
    `VerifyBigDecorParsingCna` (all `ALL CHECKS PASSED`, run from repo root); UV-math sanity check
    (icon 99 on `explo.png`'s grid lands exactly at UV (0.9,0.9)-(1.0,1.0), confirming the 100-icon
    bound). Final coverage: 65/69 confirmed `ObjectType`s have a real icon — only `ObjectType38`
    remains, plus the 4 types (`0`/`18`/`22`/`58`) that correctly have no icon in source data.

- **Filled in 18 of the 38 `ObjectType`s missing from `GEObjectIcons::GetObjIcon()` — icon coverage
  31/69 → 49/69 confirmed types (2026-07-09).** Prompted by the user asking whether all
  mobile-eggbert objects/elements can now be rendered — answer at the time was no: infrastructure
  (storage/placement/render-path-selection) was complete for all 69 confirmed `ObjectType`s, but
  only 31 had a real icon; the other 38 fell through `GetObjIcon()`'s `default: return 0` (a
  wrong/placeholder icon). Confirmed via `grep` that `GalaxyEggbertSimple3D`'s own
  `GEDecorSystem::GetObjIcon()` has the identical 31 cases — this was a genuine original gap in
  both targets, not a porting gap, so there was nothing to port; new entries are grounded in
  `mobile-eggbert-reference/03-objects.md`'s already-approved Category B table (icon + frame count
  only, `table_X[0]=N` notation — not a `Tables.cpp` per-frame array transcription).
  - **13 element.png-native types added** (23/27/28/29/34/36/37/39/41/42/56/57/97 — of the 14
    element.png-native candidates, `38`'s electric arc was deliberately excluded, see below):
    consecutive-icon-cycle formulas matching
    this function's existing simple-animation convention, except `56`/`57` where the documented
    frame count would run off element.png's 290-icon grid (253+99=352, 274+19=293) — those return
    the documented first-frame icon only (no animation) rather than guessing an unverifiable cycle.
  - **5 object-m.png-sourced types added** (14/15/31/35/52 — water plouf/bubble, charge power-up,
    bridge construction): these are billboards, not `UniformCube`s, but need `object-m.png`, not
    `element.png` — same problem `IsUniformCubeObject` solved for the 4 cube types. Added a new
    `GEObjectIcons::IsObjectMPngSourced(ObjectType)` predicate and a second billboard render path in
    `GalaxyEggbertCnaGame.cpp` (`objectMPngEffect_`/`objectMPngMeshRenderer_`, mirroring
    `bigDecorEffect_`) that looks up UVs via `tileAtlas_.GetTileUv()` instead of
    `GetElementIconUv()`. `52` (bridge, 157 documented frames) would exceed object-m.png's 440-icon
    grid (365+156=521) — first-frame icon only, same reasoning as `56`/`57` above.
  - **Deliberately NOT added, still `default: return 0`** (documented as a follow-up, not a silent
    drop — see §8): `38` (electric arc — real behavior is two-channel, `blupi1.png` ticks 0-29 then
    `element.png` ticks 30-89; `03-objects.md` flags the element.png-only simplification as "under
    consideration," not decided, so a wrong-channel guess would be worse than the existing
    fallback); 12 `explo.png`-sourced types (8/9/10/11/53/90/91/92/93/98/99/100 — needs a brand-new
    texture load + UV function, `explo.png` isn't loaded anywhere in CNA today); 4
    `blupi.png`/`blupi1.png`-sourced types (200/201/202/203, Blupi skin variants — same "new texture
    usage" reason). `0`/`18`/`22`/`58` correctly stay `default: return 0` — confirmed no fixed icon
    exists in mobile-eggbert source data for any of them (not a gap).
  - **Verified**: clean build; live run (`./GalaxyEggbertCNA`, no crash, `69 MoveObject(s) parsed`,
    screenshot written); `GalaxyEggbertWorldsTests` (61/61), `VerifyBlupiMovement`,
    `VerifyMoveObjectTypesCna`, `VerifyBigDecorParsingCna` (all `ALL CHECKS PASSED`, run from the
    repo root — these 3 binaries resolve `../mobile-eggbert/...` relative to their *working
    directory*, not their own location, a pre-existing quirk unrelated to this change).

- **Backfilled the last 6 confirmed `DirectionalCube` icons — 15/16/17/18/108/109 — completing §8
  task 1 (2026-07-09).** All ~99 confirmed `DirectionalCube` icons are now wired up.
  - **Icons 15-18** ("2 protilehlé strany + shora barva + zdola průhledné + z zbylých 2 bočních
    stran jedna průhledná a druhá barva") needed an axis choice plus which of the other axis' 2
    faces is open vs. colored. Reviewed the crops directly with the user (conducted over the
    Claude Android app, not a PC — inline image rendering via the `Read` tool didn't display for
    them; `SendUserFile` delivering the same PNGs as attachments did work). The user confirmed a
    real relationship (icons 15/17 are a horizontal mirror pair of each other) but that doesn't
    resolve either question for a single block's own 6 faces — per the user's explicit go-ahead,
    defaulted to the same tie-break approach already used for ~35 other ambiguous icons this
    session: Z axis textured, `NegX`=open, `PosX`=color. Added a new
    `Pattern::AxisTopColorBottomOpenSideColorSideOpen` case to `GEDirectionalCubeTiles.cpp`.
  - **Icons 108-109** ("2 boční strany vlastní textura, 1 boční strana textura ikony 107, 1 boční
    strana průhledná, zdola hnědá barva, shora textura trávy") needed a facing decision with the
    same inconclusive-crop outcome, so defaulted the same way (own texture on `PosZ`/`NegZ`, icon
    107's texture on `PosX`, `NegX` open). This is the first `DirectionalCube` entry that needs a
    **second icon's own UV** (icon 107's) on one of its own faces — required threading an
    `icon107Uv` parameter through `TryGetDirectionalCubeFaces()`/`AppendSpecialGeometry()` (both
    call sites in `GETerrainRenderer.cpp`: the static constructor loop and
    `RebuildAnimatedRenderer()`), computed once via `tileAtlas.GetTileUv(107)`. Extended
    `IsGrassTopIcon()` to also cover 108/109, so `GETerrainRenderer`'s existing grass-top overlay
    plate (built for icon 107) now also fires for these two — no new grass-rendering code needed.
  - Added demo blocks for all 6 icons to `tools/GenerateSampleWorld3D.cpp` (15-18 in the existing
    off-path `DirectionalCube` demo row; 108-109 on the ground floor next to icon 107's own patch,
    so their real grass-top surface is naturally visible like icon 107's).
  - **Verified**: clean build; `VerifyBlupiMovement`/`VerifyMoveObjectTypesCna`/
    `VerifyBigDecorParsingCna`/`GalaxyEggbertWorldsTests` (61/61) all pass; live run's
    vertex/triangle counts (23124/11562) are exactly +112/+56 over the previous baseline — 4 icon
    15-18 blocks × 4 visible faces (axis pair + top + one side) = 64v/32t, 2 icon 108-109 blocks ×
    4 visible faces (own-texture pair + icon-107-texture side + bottom) = 32v/16t, plus 2 grass-top
    plates = 16v/8t — deterministic proof every face count matches the design exactly. Live
    screenshots (temporary debug camera, reverted before committing) confirmed icons 15-18 render
    as real 3D wedge shapes with texture on the axis faces and genuine open gaps elsewhere, and
    icons 108-109 show the expected green grass-top overlay (matching icon 107's own established
    correct appearance); the side-face own-texture/icon-107-texture/open split specifically wasn't
    isolated in a clean close-up shot (camera-angle framing proved difficult, same class of
    difficulty as earlier sessions' billboard-framing attempts) but is covered by the exact
    vertex/triangle math above.
- **Found and (substantially, not 100%) fixed the thin blue/dark seam-line artifact (2026-07-09,
  §8 task 2).** Root cause: `GETileAtlas::GetTileUv()` (CNA) computed each tile's UV rect at the
  exact pixel boundary, with no inset — bilinear texture filtering at oblique/close angles samples
  slightly outside that exact rect, bleeding in the atlas's 1px inter-tile gap (mostly transparent
  pixels) and producing partially/fully transparent output right at tile edges, which reads as a
  "seam" showing the sky-blue clear color through. **This exact bug was already found, precisely
  diagnosed, and fixed once before** — in `GalaxyEggbertSimple3D`'s `GETerrainRenderer.cpp`, via
  `BlockTypes::tileUV()` (`include/GalaxyEggbert/BlockTypes.hpp`), which applies a documented
  half-texel UV inset for exactly this reason. CNA's `GETileAtlas` never used it — it independently
  reimplemented UV lookup via a plain `Easy3D::TextureAtlas` grid registration (correct on tile
  *pitch*, i.e. no cross-tile pixel drift, but missing the additional inset) when it was written,
  so the fix simply never carried over. Rewrote `GETileAtlas::GetTileUv()` to call
  `BlockTypes::tileUV()` directly instead (same proven math, not a new implementation) — removed
  the now-unused `Easy3D::TextureAtlas m_atlas` member and its `AddGrid()`-based constructor
  entirely (dead code once nothing calls into it), kept a plain bounds check (`0 < blockType <
  20×22`) so out-of-range icons still return an empty `UvRect` as before.
  - **Verified precisely, not just "looks about the same"**: a controlled before/after comparison
    (`git stash` isolating just this fix, same debug-camera position, same close/oblique staircase
    angle the user's original report used) found the artifact is a measurable, real effect, not a
    subjective one — raw pixel sampling at the exact seam location showed **fully transparent
    (alpha=0) "hole" pixels in the unfixed build that become non-zero (alpha≈3-4%) after the fix**
    at the same coordinates. A full-screenshot statistical comparison (800×480 = 384000 pixels)
    found: pixels with alpha<255 (any non-full-opacity) dropped from 27839 to 19448 (-30%);
    alpha<100 from 6621 to 4953 (-25%); alpha<10 (near-fully-transparent) from 1752 to 1027 (-41%);
    **alpha<1 (fully-transparent "holes" — the actual visible blue-seam pixels) from 1217 to 486,
    a 60% reduction.** `VerifyBlupiMovement` and `GalaxyEggbertWorldsTests` (61/61) both unaffected
    (this only changes UV math, not geometry or collision).
  - **Not fully eliminated — residual transparency remains** (alpha<1 dropped 60%, not to zero).
    Plausible remaining causes, not yet investigated further: (a) standard MSAA/silhouette-edge
    antialiasing produces genuine partial pixel coverage at any triangle edge regardless of texture
    UVs — likely a separate, benign effect this fix was never going to eliminate; (b) the half-texel
    inset may not be quite enough at the most extreme grazing angles/highest mip levels, where the
    sampled footprint can exceed half a texel. Downgraded from "not yet root-caused" to "root-caused
    and substantially mitigated" in §5 rather than closed outright — a future pass could try a
    larger inset or investigate mip-level-specific behavior if the residual seam still bothers
    someone in practice.
- **Implemented face-culling/occlusion for `GETerrainRenderer`'s static terrain mesh (2026-07-09,
  §8 old task 3).** Scoped to the plain, non-animated `UniformCube` path only (the dominant case
  for bulk terrain — ground floor, walls, staircase) — the animated/water paths are untouched, a
  deliberate scope limit (see below). Every plain block now computes real per-face visibility by
  checking its 6 neighbors via a new `IsOccluderBlock()` — a neighbor only counts as a face
  occluder when it's **definitely** a plain, fully opaque 1×1×1 cube itself: non-air, in bounds,
  not water, not animated (conservatively — some animated icons ARE full cubes, but excluding all
  of them keeps the check simple and never over-culls), not alpha-blended, and not itself a
  special-geometry icon (`DirectionalCube`/`InnerPillarBox`/`InnerFlatPlate`/`TripleCrossBillboard`
  — smaller-than-block or intentionally holed, must never occlude a neighbor). Reuses
  `Easy3D::DirectionalCubeItem`/`AppendDirectionalCubeMesh` (already used for holed render modes)
  instead of the old `Easy3D::CubeBatch`/`BuildCubeMesh` path — with all 6 faces `Visible=true` and
  the same `Uv` on each, it's byte-for-byte equivalent to the old plain-cube output (`../easy-3d`'s
  own test suite already guarantees this "all-visible-faces parity with `AppendCubeMesh`"), so a
  block with no solid neighbor (every isolated demo block in this sample world) renders identically
  to before — only genuinely interior faces between two solid blocks get culled. `staticBatch`/
  `Easy3D::CubeBatch` usage removed from the constructor entirely (dead code once every plain block
  routes through the per-face path).
  - **Verified**: clean build; a live run of the default `.vwr` world shows the terrain mesh vertex
    count dropping from 67788 to **23012** (66% reduction) and triangles from 33894 to **11506**
    — real, substantial savings from the ground floor/staircase/walls' interior faces, exactly the
    kind of bulk fill this task exists for. `VerifyBlupiMovement` all-pass (face culling only
    changes what's drawn, never `World::getBlock()`-based collision, so this couldn't affect
    physics even in principle) and `GalaxyEggbertWorldsTests` 61/61 unaffected (engine-agnostic,
    untouched by this CNA-only rendering change). Two close-up screenshots (reverted debug camera
    positions) — one of the default spawn view, one an oblique close-up of the staircase/wall
    corner (deliberately chosen as the densest, most culling-affected region) — confirmed no
    visible holes, gaps, or missing faces anywhere; every surface a player could actually see still
    renders solid.
  - **Deliberately not done**: face culling for the animated/water paths (`RebuildAnimatedRenderer`,
    covering lava/spike/crusher/saw/fan/temp/marine/water) — these are typically sparse decorative
    elements, not bulk fills, so the payoff is much smaller, and correctly reasoning about which
    animation frames are "definitely full cubes" (some are, some use special/holed geometry) adds
    real complexity for comparatively little benefit. `IsOccluderBlock()` already treats every
    animated/water neighbor as non-occluding unconditionally, so this is safe (never over-culls at
    the boundary between static and animated terrain) — just leaves animated regions unoptimized. A
    natural follow-up if a future world's animated-tile density ever warrants it.
- **Populated the sample world with all 67 remaining confirmed/named `ObjectType`s (2026-07-09,
  user request, §8 old task 4 — "populate the rest of the ~68 confirmed types").** The storage
  mechanism itself (previous entry below) needed no changes — placing a new type is pure data
  entry, one `PlaceMoveObject` call per type. Added a `CatalogEntry{type, name}` table in
  `tools/GenerateSampleWorld3D.cpp` covering every named entry in
  `include/GalaxyEggbert/def/ObjectType.hpp` except `ObjectType0` (null slot) and the
  "Unidentified/reserved" block (kept contiguous for level-file round-trips only, no confirmed
  real behavior to place faithfully) — 47/48 (platform-lift variants), 2/3/96/97 (patrol/follow
  enemies), 4 (bulldozer), 5/7/21/39 (collectibles), 49/50/51 (keys), 13/19/24/25/26/28/29/30/31/
  40/46/55 (vehicle/power-up pickups), 8/9/10/11/12/36/37/38/41/42/53/90/91/92/93/98/99/100
  (explosions/effects, including `ObjectType12` = crate — the first actual crate placed in the demo
  world), 14/15/34/35 (water/goo effects), 23 (projectile), 16/17/18/20/32/33/44/54 (patrol walker
  enemies), 22/27/52/56/57/58 (moving level objects), 200/201/202/203 (Blupi skin variants). Laid
  out in a dedicated 9-column × 8-row grid (grid x=10.., z=75.., 3 units apart, y=1 fixed height,
  no path — `posEnd == posStart` for all of them, matching the egg's simplification, since
  rendering doesn't consume `posEnd` yet regardless) — far from every other demo feature and
  Blupi's tested spawn/staircase/wall-collision path. Types 12/47/48 join the existing type-1 lift
  in `GEObjectIcons::IsUniformCubeObject`'s `UniformCube` path; the other 64 render as billboards.
  **Verified**: clean build; `VerifyBlupiMovement` all-pass against the regenerated world (2838
  blocks, unchanged — these are `MoveObject`s, not terrain); a live run of the default `.vwr` world
  now reports "69 MoveObject(s) parsed for billboard rendering" (67 catalog + the pre-existing egg
  + lift, exactly as expected) and "4 platform-lift/crate cube object(s) built" (types 1/12/47/48,
  exactly as expected) — deterministic proof the full catalog round-trips correctly, with no
  crash. Several temporary debug-camera surveys (reverted before committing) directly confirmed
  multiple distinct catalog entries rendering with correct, recognizable textures at different
  positions in the grid (the crate's wood-crate texture, two tread-textured lift variants with
  distinct icon content) — not all 67 were individually eyeballed (impractical at this scale, and
  unnecessary: the exact-count match already proves every single one loaded and dispatched to the
  right render path; per-type icon *correctness* was separately proven for the general billboard/
  `UniformCube` machinery well before this task).
- **MoveObjects (pickups, enemies, platform lifts, crates) can now be embedded directly in the 3D
  `.vwr` world format itself (2026-07-09, user request, not from §8's numbered list) — no more
  requiring a mobile-eggbert `.txt` file to see any of them render.** Previously only terrain
  blocks were representable in `.vwr`; `GetMobileObjects()` was always empty for a `.vwr`-sourced
  world (only `BigDecor:` had this limitation documented — MoveObjects had it too, silently).
  - **Design (confirmed with user first)**: reuse `Worlds::Chunk`'s existing sparse
    `ChunkBlockMetadataRecord` mechanism (`setExtraMetadata`/`extraMetadata()`,
    `(localBlockIndex, metadataType) → arbitrary payload bytes`, already round-trip serialized) —
    **not** a new top-level file section. New engine-agnostic `GalaxyEggbert::MoveObjectRecord`
    (`include/GalaxyEggbert/MoveObjectRecord.hpp` + `.cpp`, deliberately outside `Worlds/` so that
    tree stays uncoupled from `ObjectType`) mirrors `MobileObjSpec`'s already-simplified shape
    (`type`, `posStart`/`posEnd` X/Y/Z, `speed` — no separate step/timing fields). `PlaceMoveObject`
    encodes a fixed 29-byte payload (`objectType` + 7 float32s) anchored at
    `floor(posStartX/Y/Z)`; `CollectMoveObjects` decodes every such record in a `World`. Reserved
    `metadataType = 1` for all MoveObjectRecord payloads. **Positions are `Worlds::World`'s own raw
    grid space** (`[0, blocksPerAxis())`), matching `World::setBlock`'s coordinate convention —
    *not* the CNA-side `-kWorldCenterX/Z` render/camera-centered space `MobileObjSpec` uses
    (documented prominently in the header after getting this backwards once, see below).
  - **New `World` API**: `setBlockExtraMetadata(x,y,z,type,payload)` and
    `collectExtraMetadata(type) → vector<{x,y,z,type,payload}>` (resolves chunk-local indices back
    to world coordinates) — thin wrappers so callers don't duplicate chunk/local-index math.
    Required exposing `Chunk::linearIndex()` as `public` (was `private`) — a pure visibility change,
    no behavior change.
  - **Found and fixed a real, pre-existing data-loss bug while testing the round trip**:
    `Chunk::isEmpty()` only checked the block palette (`palette_.size()==1 && palette_.front().isAir()`),
    completely ignoring `extraMetadata_`. `World::saveToFile()` uses `isEmpty()` to skip serializing
    a chunk entirely — so a chunk that was all-air but carried sparse metadata (exactly the
    MoveObject-on-an-otherwise-empty-chunk case) had its metadata **silently dropped on save**, an
    old bug unrelated to today's new code that just happened to be the first thing to actually
    combine metadata with an all-air chunk through the file-level API (`Chunk`'s own direct
    `write`/`read` round-trip test bypassed `isEmpty()` entirely, so it never caught this). Fixed:
    `isEmpty()` now also requires `extraMetadata_.empty()`. Added
    `ChunkTests.AllAirChunkWithExtraMetadataIsNotEmpty` as a direct regression test, plus
    `WorldSerializationTests.SaveAndLoadPreservesBlockExtraMetadata` for the file-level case that
    surfaced it. No other `isEmpty()` caller (Simple3D's `GETerrainRenderer.cpp` render-skip
    optimization) is affected by the stricter definition.
  - **Also made and fixed a real coordinate-space bug of my own while wiring up the sample-world
    demo**: first attempt stored `MoveObjectRecord` positions in the CNA-side centered
    (`-kWorldCenterX/Z`) convention directly (matching `MobileObjSpec`'s own space, since that's
    what rendering consumes) — `PlaceMoveObject` immediately threw `std::out_of_range` on a
    negative/large coordinate, since `World::setBlockExtraMetadata` validates against raw grid
    bounds `[0, blocksPerAxis())`. Fixed by keeping `MoveObjectRecord` in `World`'s own raw grid
    space (consistent with every other `Worlds::World` coordinate in the codebase) and applying the
    `-kWorldCenterX/Z` shift once, in `GEWorldRuntime::LoadFromVwrFile()`'s
    `MoveObjectRecord`→`MobileObjSpec` conversion loop (same shift the `.txt` loader and
    `GetBigDecor()` conversion already apply) — documented prominently in
    `MoveObjectRecord.hpp` to prevent recurrence.
  - **`GEWorldRuntime::LoadFromVwrFile()`** now populates `mobileObjects_` via `CollectMoveObjects()`
    (previously always cleared to empty for `.vwr` sources) — `GetMobileObjects()` works identically
    from either load source now; no rendering-side changes needed at all (`GalaxyEggbertCnaGame`'s
    existing billboard/`UniformCube`-object code already consumes `GetMobileObjects()` generically).
  - Added 2 demo `MoveObjectRecord`s to `tools/GenerateSampleWorld3D.cpp` next to the icon-368 demo
    block: `ObjectType6` (egg, static, `posStart==posEnd`) and `ObjectType1` (platform lift, real
    `posStart != posEnd` — proves the "moving object" case round-trips too, though nothing consumes
    the path/interpolates motion yet — no interactive-object system exists, per §5).
  - **Verified**: `GalaxyEggbertWorldsTests` 61/61 (54 original + 7 new — 2 `World`
    extra-metadata tests, 1 `World` file round-trip test, 3 `MoveObjectRecord` tests, 1 `Chunk`
    regression test); `VerifyBlupiMovement`/`VerifyMoveObjectTypesCna`/`VerifyBigDecorParsingCna`
    all still pass (unrelated-regression checks); a live run of the default `.vwr` world now prints
    "2 MoveObject(s) parsed for billboard rendering" and "1 platform-lift/crate cube object(s)
    built" (previously always 0/0 for `.vwr` sources) with **no crash** — the platform lift's cube
    was directly screenshotted and clearly visible (2 faces of its icon-29 texture, matching the
    already-confirmed-correct platform-lift rendering from earlier today). The egg's specific
    billboard was not independently isolated in a screenshot (camera-framing proved fiddly in the
    sparse synthetic demo world — a billboard directly below eye level falls out of frame faster
    than expected, see §5's seam-artifact-adjacent note on camera FOV math) but the data path is
    proven correct two other ways: a temporary debug print confirmed the exact right position/type
    reaches `GEWorldRuntime` (`type=6 pos=(34,3,-8)`), and loading a real mobile-eggbert level
    (`world022.txt`, which also has `type=6` objects) during the same investigation showed a
    billboard rendering with real, distinct texture content at that position — proving the
    billboard-rendering code path itself works correctly, independent of the new storage mechanism.
- **`GEInnerFlatPlateTiles` axis spot-check (2026-07-09, §8 old task 5): found and fixed one real
  case, icons 368-372.** Built a labeled contact-sheet montage (via `montage`/`convert`) of all 58
  not-yet-individually-checked `InnerFlatPlate` crops (the earlier sample of 5 — 77, 110, 114, 264,
  367 — had found no cue) to review them efficiently in a couple of screens instead of one-by-one.
  53 of the 58 are consistent with the existing vertical (`PlateAxis::Z`) default — small
  bracket/frame-shaped decorations meant to be viewed face-on, same pattern as the original sample.
  **Icons 368-372 stood out clearly**: an elongated, horizontally-lying segmented shape sitting
  mid-tile, visually distinct from every other icon in the set — plausible either as mobile-eggbert
  reference docs' code-traced "`table_bridge` construction-frame" identity (`02-tiles.md`) or the
  round-3 questionnaire's crop-only "okraj hroudy hlíny" (dirt-clump edge) reading, and both
  readings point the same way: a ground-lying element, not a wall-mounted signpost. The
  questionnaire's own `InnerFlatPlate` answer for these 5 doesn't specify an axis at all (same gap
  as the other 58), so this is a legitimate crop-read fill-in, not a re-litigation of the confirmed
  render mode — same technique already used throughout this session for `DirectionalCube` facing.
  Added `GEInnerFlatPlateTiles::GetInnerFlatPlateAxis(int)` (new, alongside the existing
  `IsInnerFlatPlateIcon`) returning `PlateAxis::Y` for icons 368-372 and `PlateAxis::Z` for the
  other 58; `GETerrainRenderer.cpp`'s `AppendSpecialGeometry()` now calls it instead of
  hardcoding `PlateAxis::Z`. Added an icon-368 demo block to `tools/GenerateSampleWorld3D.cpp`.
  **Verified**: clean build; `VerifyBlupiMovement` all-pass against the regenerated world (2838
  blocks, +1); live run's vertex/triangle counts (67788/33894) are exactly +8/+4 over the previous
  baseline (one `InnerFlatPlate` block's fixed geometry cost — 8 vertices/4 triangles regardless of
  axis, per `AppendPlateMesh`'s doc comment — deterministic proof the axis change didn't alter
  geometry count, only orientation); a temporary debug camera reposition (reverted before
  committing, confirmed via empty `git diff` on that file) — first attempt used too steep a
  downward angle and missed the plate's frustum (0/25 background-sample points, blank sky in the
  screenshot, a real dead-end worth recording: a purely level-look camera needs `atan(ΔY/distance)`
  within the ~22.5° half-FOV to see a horizontal plate below eye level) — a corrected
  angle/distance then clearly showed the plate rendering horizontally (visible from above, matching
  the crop's lying-down shape) instead of the old vertical treatment.
- **Verified `GalaxyEggbertCNA`'s clean-exit path (2026-07-09, §8 old task 4) — found a real,
  reproducible bug, not fixed (out of scope: SDL/`../cna`, not galaxy-eggbert code).** Previously
  only forced-kill/timeout was tested (§5). Found that this session's sandbox already has an
  isolated `Xvfb :99` virtual display plus `xdotool` available — used them to do a genuine
  window-manager close test: launched `GalaxyEggbertCNA` with `DISPLAY=:99 SDL_VIDEODRIVER=x11`
  (the default video driver under a bare `DISPLAY=:99` env turned out NOT to create a real X11
  window at all — `SDL_VIDEODRIVER=x11` was needed to force it, confirmed via `xwininfo -tree`
  showing 0 windows without it vs. a real "Galaxy Eggbert (CNA)" window with it), found its window
  via `xdotool search --name`, then `xdotool windowclose <id>` (sends a real ICCCM
  `WM_DELETE_WINDOW` ClientMessage, which SDL translates to `SDL_EVENT_QUIT` →
  `Game::Exit()`) instead of an external `SIGTERM`/`timeout` kill. Result: the process **exits
  with code 1, not 0** — reproducible identically 2/2 runs, same `X Error of failed request:
  BadWindow (invalid Window parameter)` on the `XInputExtension` major opcode referencing the
  just-destroyed window's resource id, printed immediately after the final `Draw()` frame's
  output. No leaked process/zombie afterward (checked `ps aux` post-exit) — the process does fully
  terminate, just with the wrong exit code. Traced `Game::Exit()` in `../cna`'s `Game.cpp`
  (read-only) to confirm it's a 2-line flag-setter (`RunApplication = false; suppressDraw_ =
  true;`) — the actual X11 window-teardown/XInput cleanup that provokes the error happens deeper
  in SDL's own X11 backend, not in any galaxy-eggbert or even CNA-level code path, so fixing this
  would mean modifying `../cna` or SDL, which needs explicit user approval per `CLAUDE.md` and
  wasn't sought for this verification task. Documented as a new confirmed bug in §5 rather than
  attempted.
- **Re-verified `GalaxyEggbertWorldsTests` (2026-07-09, §8 old task 3) — still 54/54, no code
  changes needed.** Last checked 2026-07-07; built and ran from `build-cna` (the Simple3D-OFF
  tree, per §9 — not the broken `cmake-build-debug`). `ctest --test-dir build-cna` also discovers
  5 `easy-gl`/`meta-gl` dependency-subproject tests that report "Not Run" since their executables
  were never built by this session's targeted build — unrelated pre-existing noise, isolated via
  `ctest --test-dir build-cna -I 1,54` (the `GalaxyEggbertWorldsTests` test-number range) to
  confirm the real result cleanly: 100% (54/54) passed.
- **Implemented the platform-lift/crate `UniformCube` object path for CNA (2026-07-09, §8 old
  task 3, `mobile-eggbert-reference/15-3d-render-mapping-design.md` §5's two confirmed "render as
  a cube, not a billboard" exceptions).** Platform lifts (`ObjectType1`/`47`/`48` — Blupi
  physically stands and rides on top, a flat billboard would look wrong for something
  load-bearing) and crates (`ObjectType12` — a pushable box is naturally a cube in every
  direction) both source `object-m.png` (confirmed via `03-objects.md`'s `channel=1`/
  `PixmapChannel::Object` entries), the same sheet terrain already uses — not `element.png` like
  every other `MoveObject` billboard.
  - Added `GEObjectIcons::IsUniformCubeObject(ObjectType)` (true for types 1/12/47/48 only).
    **Also fixed a real pre-existing gap while wiring this up**: `GetObjIcon()`'s switch had no
    case for `ObjectType48` at all (fell through to `default: return 0`) — added `kChenillei`
    (`{316,315,314,313,312,311}`, the reverse-order `table_chenillei` track per `03-objects.md`'s
    `"table_chenillei[0]=316"` note) and the missing `case ObjectType48`.
  - `GalaxyEggbertCnaGame` gained `objectCubeEffect_`/`objectCubeMeshRenderer_`, built **once** in
    `LoadContent()` (unlike the billboard renderers, a world-space cube's vertices don't depend on
    the camera) by looping `worldRuntime_.GetMobileObjects()`, calling
    `Easy3D::AppendCubeMesh()` per matching object with `Center = (posStartX, posStartY +
    kObjectCubeGroundOffset, posStartZ)` — the same `+1.0f` ground-offset convention the existing
    MoveObject billboards already use at the same position, so a cube object sits exactly where an
    already-visually-verified billboard would. The billboard-building loop now skips
    `IsUniformCubeObject()` types so they aren't drawn twice; `Draw()` renders the cube mesh in the
    same opaque pass as the main terrain (right after `terrainRenderer_->Draw()`), same texture
    (`terrainTexture_`) via its own effect instance.
  - **Verified**: clean build; `VerifyBlupiMovement` all-pass against the unchanged default `.vwr`
    world (prints "0 platform-lift/crate cube object(s) built." there, as expected — no
    `MoveObject`s at all in that hand-authored world); `VerifyMoveObjectTypesCna` re-run as an
    unrelated-regression check, still 12/12 pass. A temporary debug swap to
    `LoadFromMobileEggbertFile("worlds/world022.txt")` (which has both types — 8 platform lifts, 15
    crates, confirmed by direct `grep -c` against the source `.txt`) plus two repositioned-camera
    screenshots (both reverted before committing) showed: (1) two crates rendering as genuine
    textured 3D cubes (visibly different shading per face, not a flat sprite) next to a brick wall
    section, and (2) the platform lift rendering as a solid cube using icon 29's real texture (a
    teal/blue funnel shape, cross-checked directly against
    `mobile-eggbert-reference/images/tile-full-029.png` to confirm the match). The live count line
    read "23 platform-lift/crate cube object(s) built." — exactly 8+15, deterministic proof the
    per-type filter is neither over- nor under-matching.
- **Implemented `BigDecor:` billboard rendering for CNA (2026-07-09, §8 old task 3).** Parsing was
  already done (`GEWorldRuntime::LoadFromMobileEggbertFile()` fully parses `BigDecor:` into
  `bigDecor_`, confirmed from source reading — the task description was stale about that); only
  rendering was missing. `GalaxyEggbertCnaGame` gained `bigDecorCells_`/`bigDecorEffect_`/
  `bigDecorMeshRenderer_`, modeled directly on the existing `MoveObject` billboard pattern but
  reusing `terrainTexture_`/`tileAtlas_.GetTileUv()` instead of `objectTexture_`/
  `GetElementIconUv()`, since `BigDecor` shares the main terrain grid's icon vocabulary
  (`object-m.png`), not `element.png`. `LoadContent()` filters the full 100×100 `GetBigDecor()`
  grid once into `bigDecorCells_` (non-air only) so `Draw()` doesn't rescan 10000 cells every
  frame; `Draw()` rebuilds the camera-facing billboard mesh every frame, same reasoning as the
  `MoveObject` billboards.
  - **Found and fixed a real segfault** (live-tested before committing): `LoadFromVwrFile()` (the
    default `.vwr` world-loading path) clears `bigDecor_` to empty (`.clear()`) — only
    `LoadFromMobileEggbertFile()` populates the full 10000-element grid. The first version of the
    cell-caching loop indexed `bigDecor[row*100+col]` unconditionally for row/col 0..99 with no
    bounds check, an out-of-bounds `operator[]` read on the empty vector when loading the default
    world — caught by an actual live run (crashed right after "0 MoveObject(s)..." printed, before
    "...BigDecor cell(s)..." could print). Fixed by guarding the scan behind
    `bigDecor.size() == 100*100`, skipping it entirely for `.vwr`-sourced worlds (which have no
    `BigDecor` concept at all). This is now covered by an explicit regression case in the new
    verification tool (below), not just tribal knowledge in a comment.
  - **New `tools/VerifyBigDecorParsingCna.cpp`** (registered in `CMakeLists.txt`, mirrors
    `VerifyMoveObjectTypesCna.cpp`'s pattern): checks 3 real non-air `BigDecor:` cells in
    `../mobile-eggbert/worlds/world013.txt` (icons 25/18/16 at specific row/col, found by directly
    parsing the `.txt` grid) land at the expected grid position via `GetBigDecor()`, plus a
    sanity check that `LoadFromVwrFile()`'s world source has an empty `BigDecor` grid as
    expected — exercising the segfault-fix guard directly. All 4 checks pass.
  - **Verified**: clean build; `VerifyBlupiMovement` all-pass against the default `.vwr` world
    (unchanged, 2837 blocks); a live 8s headless run of the default world no longer crashes (was
    SIGSEGV before the fix) and prints "0 BigDecor cell(s) parsed for billboard rendering." (the
    default world has none, as expected); a temporary debug swap to
    `LoadFromMobileEggbertFile("worlds/world013.txt")` plus a repositioned spawn (both reverted
    before committing, confirmed via empty `git diff` on this point) loaded a real level with 14
    non-air `BigDecor` cells (20 raw non-zero grid entries, 6 of which map to `Air` via
    `fromMobileIconId`'s passable/decorative filter — expected, not a bug) and a live screenshot
    showed two billboards rendering with real, distinct textures at the correct position, standing
    up from the terrain as expected. `VerifyMoveObjectTypesCna` re-run as an unrelated-regression
    check, still 12/12 pass.
- **Generated and wired up a grass-top texture for icon 107 (2026-07-08, §8 task 3, user's chosen
  approach: procedurally generate rather than source externally).** `object-m.png` can't be
  extended with a new region — `GalaxyEggbertCNA`'s build re-copies it fresh from
  `../mobile-eggbert` every time (confirmed by reading the CMake POST_BUILD step), so anything
  added there would be silently wiped on the next build. Instead: a new `textures3d/` directory
  (git-tracked, copied next to the binary exactly like `worlds3d/`, added to `CMakeLists.txt`)
  holds a genuinely new, galaxy-eggbert-owned `grass_top.png` (64×64, procedurally generated via a
  small Python/Pillow script — per-pixel green color variation plus scattered darker/lighter
  flecks for texture, deterministically seeded). `GEDirectionalCubeTiles.cpp`'s icon 107 entry
  sets its top face `Visible=false` on purpose (not a hole — the real top surface is drawn
  separately) while keeping its 4 sides textured and bottom flat-brown via `SwatchUv` (confirmed
  correct since icon 107's own texture genuinely is grass-over-dirt already, verified from its
  crop). Added `Easy3D::PlateAxis::Y` (horizontal plate, spans X/Z instead of the existing
  Z/X vertical orientations — a real generalization, not a one-off hack: any future
  horizontal-surface effect can reuse it) to `../easy-3d`'s `CubeMesh.hpp/.cpp`, plus a test.
  `GETerrainRenderer` gained `IsGrassTopIcon()`/`m_grassRenderer` (built once, additive to the
  normal DirectionalCube pass, not exclusive) and a new public `DrawGrass()` method, since
  `BasicEffect` only binds one texture at a time and grass needs a genuinely separate
  texture/effect from the main `object-m.png`-bound one — `GalaxyEggbertCnaGame` now owns a
  `grassTexture_`/`grassEffect_` pair and calls `DrawGrass()` right after the main terrain
  `Draw()`. Added a 3×3 icon-107 patch directly on the ground floor in
  `tools/GenerateSampleWorld3D.cpp` (replacing existing `RockPile` tiles, not adding new ones —
  total block count unchanged). Verified: `VerifyBlupiMovement` all-pass; live run's
  vertex/triangle counts (67780/33890) are exactly +36/+18 over the previous baseline (9 blocks ×
  net +4v/+2t each — `DirectionalCube`'s 5 visible faces [20v/10t] + the grass plate [8v/4t] = 28v/
  14t, replacing the old `UniformCube`'s 24v/12t); a temporary debug spawn reposition (reverted,
  not part of this commit) got a real screenshot clearly showing the green grass patch rendering
  correctly, distinct from the surrounding stone floor. Icons 108-109 need the same texture but
  more per-side work (their own answer mixes their own texture, icon 107's texture, and an open
  face across the 4 sides) — not done yet, tracked in §8 task 1.
- **Wired up icons 30/31 (the last "needs real texture alpha" `DirectionalCube` icons) by reusing
  the water render mode's alpha-blend state (2026-07-08, §8 task 1).** Their confirmed answer is a
  normal "4 sides + top/bottom both flat color" `DirectionalCube` face pattern (added to
  `GEDirectionalCubeTiles.cpp`'s existing symmetric-entries table, no new face logic) — the only
  new piece is that these 2 icons' own side-face texture has real per-pixel alpha, so they need
  the same semi-transparent draw pass as water even though they're NOT animated. Added
  `GETerrainRenderer::NeedsAlphaBlend()` plus a new `m_transparentStaticRenderer` (built once in
  the constructor, unlike water's per-animation-phase rebuild) that reuses the exact same
  `BlendState::NonPremultiplied`/`DepthStencilState::DepthRead` block in `Draw()` as water — one
  state change now covers both. Added a `BrickWall`-behind-icon-30 demo block to
  `tools/GenerateSampleWorld3D.cpp` (same technique as the water demo). Verified:
  `VerifyBlupiMovement` all-pass against the regenerated world (2837 blocks, +2); live run's
  vertex/triangle counts (67744/33872) are exactly +48/+24 over the previous baseline (2 opaque
  6-face `UniformCube`-shaped blocks — `BrickWall` fully opaque, icon 30 in the new transparent
  pass); a temporary debug spawn reposition (reverted, not part of this commit) got a real
  screenshot showing icon 30 rendering as a genuinely semi-transparent yellow sign, `BrickWall`
  staying fully solid next to it. Icons 30/31 are editor-only markers (moveable-object start
  position) per the questionnaire — never placed in real gameplay worlds, wired up for
  completeness.
- **Implemented the water render mode: a semi-transparent alpha-blended cube (2026-07-08, §8
  task 2, user's design decision).** `Water1`/`Water2` blocks were previously routed through the
  same opaque animated path as lava/spikes/fans (a solid, fully-opaque animated `UniformCube` —
  visually wrong, "a swimming pool isn't a solid turquoise brick" per `15-3d-render-mapping-
  design.md` §10.4). Split water out of `GETerrainRenderer`'s animated-block classification into
  its own `m_waterBlocks`/`m_waterRenderer`, rebuilt every animation-phase change exactly like the
  opaque animated group (factored the shared rebuild logic into one
  `RebuildAnimatedRenderer()` private method to avoid duplicating it a third time). `Draw()` now
  renders water in a separate pass after the opaque static+animated geometry, with
  `BlendState::NonPremultiplied` and `DepthStencilState::DepthRead` (depth-tested but not
  depth-written, the standard opaque-then-transparent technique), restoring `BlendState::Opaque`/
  `DepthStencilState::Default` afterward. No shader changes needed: confirmed via source research
  that CNA's `BasicEffect` already multiplies the sampled texel's alpha by `Alpha` (default 1.0)
  into the output alpha whenever `TextureEnabled=true`, and `object-m.png` genuinely has per-pixel
  alpha (PNG color type 6, real anti-aliased cutouts, not a flat-opaque sheet) — so simply enabling
  blending was sufficient. Geometry is unchanged (still a plain 6-face `UniformCube`, per the
  user's chosen option) — this is purely a rendering-state change. Added a `BrickWall` block
  directly behind a `Water1` block in `tools/GenerateSampleWorld3D.cpp` to demo it. Verified:
  `VerifyBlupiMovement` all-pass against the regenerated world (2835 blocks, +2); live run's
  vertex/triangle counts (67696/33848) are exactly +48/+24 over the previous baseline (2 opaque
  6-face `UniformCube`-shaped blocks, water's geometry didn't change); a temporary debug spawn
  reposition (reverted before committing) got a real screenshot showing the `BrickWall` block
  clearly visible *through* the `Water1` block in front of it — genuine see-through transparency,
  not a solid tinted cube.
- **Implemented `InnerPillarBox`/`InnerFlatPlate`/`TripleCrossBillboard`, the last 3 of the 4
  confirmed render modes (2026-07-08, §8 task 2).**
  - **`InnerPillarBox` needed no new Easy3D code at all** — it's just a `DirectionalCubeItem` with
    a smaller `Size` (a "post" instead of a full block) instead of a new mesh type; reused
    `AppendDirectionalCubeMesh` directly. New `GEInnerPillarBoxTiles.{hpp,cpp}` covers all 3
    confirmed icons: 76 (4 side faces textured, top/bottom open — unstated in the questionnaire
    answer, treated as open like icon 200) and 384/385 (`BlockTypes::Switch`/`SwitchOff` — 1 side
    textured, other 5 flat color, same `SwatchUv` approach, defaulting to `PosZ` facing like
    `DirectionalCube`'s symmetric icons).
  - **`InnerFlatPlate` and `TripleCrossBillboard` needed real new geometry**: added
    `Easy3D::PlateItem`/`AppendPlateMesh` (a double-sided quad on a fixed axis — emits both
    triangle windings so it's visible from both sides regardless of the renderer's cull state, no
    z-fighting since both windings share the exact same vertex positions) and
    `Easy3D::TripleCrossItem`/`AppendTripleCrossMesh` (3 `PlateMesh`-style double-sided planes,
    60° apart around Y) to `CubeMesh.hpp/.cpp`. Both still emit plain `CubeVertex` (position+UV)
    so the existing `CubeMeshRenderer`/`BasicEffect` draw path needed no changes. Added 6 new
    `easy3d_test_cube_mesh` cases (all 6/6 `../easy-3d` tests pass).
  - **Extracted the DirectionalCube "flat color" swatch-sampling helper into a shared
    `GESwatchUv.{hpp,cpp}`** (was private to `GEDirectionalCubeTiles.cpp`) so
    `GEInnerPillarBoxTiles.cpp` could reuse it without duplicating the logic.
  - **`InnerFlatPlate` (63 confirmed icons) and `TripleCrossBillboard` (10 confirmed icons)** are
    both fully backfilled: new `GEInnerFlatPlateTiles.{hpp,cpp}`/`GETripleCrossBillboardTiles.{hpp,cpp}`
    list every confirmed icon. `TripleCrossBillboard` needed no per-icon facing decision at all —
    its 3-plane arrangement is rotationally symmetric by construction, so every icon uses the same
    fixed angles. `InnerFlatPlate` icons all default to the same fixed axis/size — a sample of 5
    crops (77, 110, 114, 264, 367) showed the same "small, front-facing or symmetric decoration,
    no reliable axis cue" pattern already found for most single-face `DirectionalCube` icons, so a
    uniform default was used rather than checking all 63 individually.
  - **`GETerrainRenderer.cpp` refactored**: the per-block "which render mode does this icon use"
    logic (previously `DirectionalCube`-only, inline in both the static-block loop and
    `Update()`'s animated-block loop) is now one shared `AppendSpecialGeometry()` helper checking
    all 4 modes in order, called from both loops — avoids re-duplicating the
    static-vs-animated-lookup-icon distinction (§3's earlier fan-animation-path fix) a third and
    fourth time.
  - Added 1 demo block each (icons 76, 384, 77, 53) to `tools/GenerateSampleWorld3D.cpp`.
    Verified: `VerifyBlupiMovement` all-pass against the regenerated world (2833 blocks, up from
    2829); live run's vertex/triangle counts (67648/33824) are exactly +72/+36 over the previous
    baseline, matching each mode's declared face/plane count exactly; a temporary debug spawn
    reposition (reverted before committing) got a real screenshot of all 4 new blocks — icon 77's
    plate clearly shows its actual Y-signpost texture (confirms correct, non-flipped UV mapping)
    and icon 53's triple-cross shows the correct small round/teal shape matching its source crop.
- **Resolved the fan base ambiguity, discovered mobile-eggbert has no rotation metadata, and
  backfilled 41 more `DirectionalCube` icons (2026-07-08, §8 task 1).**
  - **Fans (126/129/132/135):** user re-examined the crops and confirmed which of top/bottom is
    the "základna" (base, flat color) vs. open: icon 132 (FanUp) has a visible pedestal touching
    the bottom of the image, icon 135 (FanDown) a mount hanging from the top (mirrored); 126/129
    (FanLeft/Right, side-mounted, no clear up/down cue) default to "base = bottom" too. Added
    `kFanEntries` to `GEDirectionalCubeTiles.cpp`.
  - **Investigated where mobile-eggbert stores the "směr v metadatech bloku" (facing stored in
    block metadata) the ~40 single/double-face icons' answers referenced — it doesn't exist.**
    `Decor.hpp`'s `Cellule` struct is just `{ icon; }`; the world-file grid format
    (`Worlds::GetDecorField`) reads one plain decimal integer per cell, no bit-packing; and
    `15-3d-render-mapping-design.md` (already in this repo) independently confirms mobile-eggbert
    encodes facing as a *separate icon ID* for every tile that actually needs it (exactly the
    `FanLeft`/`FanRight`/`FanUp`/`FanDown` pattern). The questionnaire's "metadata" phrasing was
    the answerer's own boilerplate guess while looking at isolated crops, not a real source
    finding — galaxy-eggbert's `World`/`Block` format needs no new orientation field.
  - **Backfilled 37 of the ~40 "1 or 2 of 4 sides textured" icons** by determining each one's
    facing directly from its crop image (2D top/bottom → `CubeFace::PosZ`/`NegZ`, 2D left/right →
    `CubeFace::NegX`/`PosX`, same convention the fan icons already confirm). In practice, almost
    all of these turned out to be small symmetric/abstract decorative icons (a plain ball, a grid
    of dots) with **no real directional cue** — for those, any one fixed face is equally valid
    (not a guess about content, since a symmetric texture looks identical on any face), so they
    default to `CubeFace::PosZ`. Only icons 392/393 (a matched light/dark stone-trim pair, left
    vs. right edge of the crop) and 49 (a clear horizontal axle/dumbbell shape) had a genuinely
    confident directional read. Added `Pattern`/`DirectionalEntry`/`kDirectionalEntries` to
    `GEDirectionalCubeTiles.cpp` covering 6 distinct face patterns (single-face-rest-color,
    single-face-top-color-rest-open, single-face-top-bottom-color-rest-open, single-face-rest-open,
    axis-rest-color, axis-plus-top-bottom-other-sides-color). Icons 15-18 were deliberately **not**
    added — their answer needs both an axis choice AND which of the 2 remaining perpendicular
    sides is open vs. colored, and their crops (diagonal wedge shapes) didn't give a confident read
    for either part.
  - **Found and fixed a real bug while verifying:** `GETerrainRenderer::Update()` (the *animated*
    tile path) never consulted `GEDirectionalCubeTiles` at all — the 4 fan icons are both animated
    (blade-spin) and `DirectionalCube`, and the animated path's `IsAnimated()` check runs *before*
    the static path's `TryGetDirectionalCubeFaces()` call, so fan blocks always fell through to a
    plain 6-face `UniformCube`, silently making the new fan table entries dead code. Fixed by
    adding the same `TryGetDirectionalCubeFaces` check to `Update()`'s per-frame loop — looked up
    by `block.base` (the animation group's base icon, e.g. `FanLeft`=126), not by the current
    animated frame's icon (126/127/128), since the face pattern must stay constant across the
    spin animation while only the sampled texture changes frame to frame. Caught by a live
    block/vertex-count mismatch on the icon-126 demo block (expected 5 visible faces, initially
    got 6).
  - Added 3 more demo blocks (icons 126, 392, 49) to `tools/GenerateSampleWorld3D.cpp`.
    Verified: `VerifyBlupiMovement` all-pass against the regenerated world (2829 blocks, up from
    2784); live run's vertex/triangle counts (67576/33788, 15 animated blocks) are exactly
    +1020/+510 over the previous baseline — 15 icon-392 blocks × 6 faces + 15 icon-49 blocks × 6
    faces + 15 icon-126 blocks × 5 faces (after the animated-path fix) — deterministic proof both
    the new table entries and the fan fix render exactly as specified. The new blocks sit outside
    the default camera's field of view in `screenshot.png` (same fixed spawn-facing shot as
    before) — not individually screenshotted, but the exact vertex/triangle math is equivalent
    proof for a purely geometric, no-alpha, no-new-shader render mode already visually proven on
    icons 200/2/25.
- **Backfilled 48 more confirmed `DirectionalCube` icons into `GEDirectionalCubeTiles.cpp`
  (2026-07-08, §8 task 1).** All 48 answered "krychle, textura na (všech/4) bočních stranách" in
  the questionnaires — i.e. all 4 side faces always show the real texture; only top/bottom vary,
  independently, between "flat fallback color" and "open" — so no per-placement rotation is needed
  and each could be wired up exactly as confirmed, no guessing. Added `SwatchUv()`: since every
  named fallback color in the questionnaire answers is per-icon (icon 2's answer literally says
  "modrý odstín, stejný jako pozadí ikony" — "same as the icon's own background"), a "flat color"
  face's `Uv` samples a small swatch of the *same* tile's own texture near its top or bottom edge
  (matching which face — "shora"/"zdola" — asked for it) instead of a hardcoded RGB; this also
  reproduces icons with genuinely different top vs. bottom colors (e.g. icon 193: gray top / beige
  bottom) since the two edges are sampled independently. Screened all ~98 confirmed
  `DirectionalCube` icons from `questionnaire-all-remaining-tiles.md`/`questionnaire-unused-tiles.md`
  and excluded ~49 that need a capability not built yet: ~40 need per-placement face rotation
  ("směr v metadatech bloku") that `World`/`Block` has no field for; icons 30/31 need real texture
  alpha; icons 126/129/132/135's "5. strana (základna)" doesn't say whether the base is top or
  bottom; icons 107-109 need a not-yet-sourced grass texture — see §5's expanded list. Added 2 more
  demo blocks (icons 2 and 25) next to the existing grate in
  `tools/GenerateSampleWorld3D.cpp`, regenerated `worlds3d/world001.vwr` (2784 blocks, up from
  2754). Verified: `VerifyBlupiMovement` all-pass; live run's vertex/triangle counts
  (66556/33278) are exactly +660/+330 over the previous baseline (15 icon-2 blocks × 6 faces + 15
  icon-25 blocks × 5 faces) — deterministic proof the table's face counts render exactly as
  specified; `screenshot.png` shows the icon-200 grate next to icon 2's solid, swatch-colored
  block, visibly different treatments.
- **Implemented the `DirectionalCube` render mode and proved it on icon 200/`Platform`
  (2026-07-08).** Added `Easy3D::DirectionalCubeItem`/`DirectionalCubeFace`/
  `CubeFace`/`AppendDirectionalCubeMesh` to `../easy-3d`'s `CubeMesh.hpp/.cpp` — a cube where each
  of the 6 faces independently picks its own UV region and whether it's emitted at all (a
  `Visible == false` face is a genuine geometric hole, not just untextured). Deliberately no
  color/vertex-format changes: a "flat fallback color" face (most of the ~100 other confirmed
  `DirectionalCube` icons) is just a UV pointing at a suitable swatch of the existing atlas texture
  when that's wired up later, not a new shader path — keeps Easy3D's vertex format
  (`VertexPositionTexture`) and `CubeMeshRenderer`/`BasicEffect` draw path completely unchanged.
  Added 4 new `easy3d_test_cube_mesh` cases (all 6/6 `../easy-3d` CNA-linked tests pass). New
  `src/GalaxyEggbertCNA/Game/GEDirectionalCubeTiles.{hpp,cpp}` holds the per-icon face table
  (only icon 200 so far — 4 side faces textured, top/bottom `Visible=false`, confirmed by round-1
  Q&A and 02-tiles.md's icon 200 entry); `GETerrainRenderer.cpp`'s block loop now checks this table
  per non-animated block and calls `AppendDirectionalCubeMesh` (appended after the main
  `BuildCubeMesh` static batch, into the same vertex/index arrays — no new `CubeMeshRenderer`/draw
  call needed). Added a small floating icon-200 demo grate to
  `tools/GenerateSampleWorld3D.cpp` (grid x=48-52,y=3,z=40-44 — deliberately off Blupi's tested
  spawn/staircase/wall-collision path) and a one-shot `screenshot.png` capture to
  `GalaxyEggbertCnaGame.cpp`'s existing first-frame debug block (via `Texture2D::SaveAsPng`) for
  visual verification. Verified: `../easy-3d` 6/6 tests, `VerifyBlupiMovement` all-pass against the
  regenerated world (2754 blocks, up from 2729), and a real screenshot showing the grate's 4
  textured side faces with sky-blue visible straight through the open top/bottom — plus a
  deterministic vertex/triangle-count check (65896/32948, exactly +400/+200 over the
  non-directional baseline = 25 blocks × 4 visible faces, not 6).
- **Fixed `Platform`/`Ground` misuse in the sample world and Simple3D's fallback world (2026-07-08,
  §8 old task 1).** Icon 200 (`Platform`) was confirmed by round-1 Q&A to be a passable grate/grid
  graphic (`DirectionalCube`: 4 textured side faces, top/bottom genuinely open — not a real solid
  floor), same bug class as the `StoneA`/`StoneB` fix below. `tools/GenerateSampleWorld3D.cpp`'s
  raised platform floor now uses `RockPile` (icon 35) instead, matching the staircase it sits flush
  with. Also found and fixed the same file's ground floor (line ~34) still using `BlockTypes::Ground`
  — missed by the earlier `StoneA`/`StoneB` fix (`59c2e61`) even though it's the same file/invariant
  violation; now `RockPile` too. `src/GalaxyEggbertSimple3D/Game/GEWorldRuntime.cpp`'s
  `BuildDemoWorld()` fallback world had the same three-way `Ground`/`StoneA`/`StoneB` bug — now
  `RockPile` (floor) and `BrickWall` (raised block + lava-adjacent markers). Added a warning comment
  on `BlockTypes::Platform` in `BlockTypes.hpp` mirroring the existing `Ground`/`StoneA`/`StoneB`
  one. Regenerated `worlds3d/world001.vwr` (same 2729 blocks, Y range [0,13] — texture only).
  Verified: clean `GenerateSampleWorld3D`/`VerifyBlupiMovement`/`GalaxyEggbertCNA` build,
  `VerifyBlupiMovement` all-pass, live `GalaxyEggbertCNA` run shows real terrain texture (10 distinct
  sampled colors). `GalaxyEggbertSimple3D` itself could not be rebuilt to verify the
  `GEWorldRuntime.cpp` change compiles+runs — see §2's build-status note (pre-existing U3D-prebuilt
  environment gap, unrelated to this edit); the edit is a syntactically trivial enum-constant swap.
- **Fixed `StoneA`/`StoneB` misuse in the sample world (2026-07-08, `59c2e61`).** Both were
  confirmed (via the round-2 Q&A below) to actually be machine-piece graphics, not bulk stone
  material — the same mistake `StoneB` itself was introduced to fix on 2026-07-06. Added
  `BlockTypes::RockPile` (icon 35) and `BlockTypes::BrickWall` (icon 261) as confirmed-genuine bulk
  terrain (`UniformCube`, all 6 faces), switched `tools/GenerateSampleWorld3D.cpp`'s
  staircase/walls/pillars to use them, regenerated `worlds3d/world001.vwr` (same 2729 blocks/Y
  range — texture only). Verified: clean build, `VerifyBlupiMovement` all-pass, live
  `GalaxyEggbertCNA` run renders real terrain texture. Found but **not fixed** in the same pass:
  icon 200 (`Platform`) has an analogous bug, and Simple3D's `GEWorldRuntime.cpp` fallback world
  has the same `Ground`/`StoneA`/`StoneB` issue (tracked as a new open task, §8).
- **Fixed `02-tiles.md`'s incorrect "unused" claim (2026-07-08, `a0df5ae`).** The original
  "0/78 files" scan only checked `Decor:`/`BigDecor:` grids. Cross-referencing
  `Decor.cpp`/`Tables.cpp` found `MoveObject:` records' own `icon=` field can reference the same
  `object-m.png` sheet (`PixmapChannel::Object` → `bitmapObject` → `"object-m"`), and confirmed
  real use for 32 "unused" icons: 32-34 (crates, `ObjectType12`), 99-102+244 (water splash,
  `table_plouf`/`table_tiplouf`), 103-106 (Blupi breathing bubbles, `table_blup`), 238-243 (Charge
  pickup, `table_charge`), 311-316 (conveyor-lift tread, `table_chenille`/`table_chenillei`),
  365-372 (bridge-build frames, `table_bridge`). All 32 match the user's independent
  identifications in `questionnaire-unused-tiles.md` below. The remaining ~65 "unused" icons were
  not individually re-checked — flagged as unconfirmed, not proven wrong.
- **Round-3 tile questionnaire generated and fully resolved (2026-07-08, `572bc2a`).**
  `questionnaire-unused-tiles.md` — 97 icons (`02-tiles.md`'s "unused/unnamed" bucket, minus
  `Air`/icon 440/already-covered animation sub-frames). Crops regenerated with the same formula as
  the existing 313. All 97 answered via direct user Q&A (mostly `UniformCube`/`Billboard`/
  `InnerFlatPlate`, a few left as open questions — water, one "nevím").
- **Round-2 tile questionnaire fully resolved (2026-07-08, `168fcd7`).** All 280 icons in
  `questionnaire-all-remaining-tiles.md` answered via direct user Q&A (not agent guessing). Found
  several real corrections to the earlier 8-agent first-pass guesses: `Ground`/`StoneA`/`StoneB`
  are machine-piece graphics, not terrain; icons 30/31 are level-editor-only markers (start/end
  position for a moveable object), never rendered in actual gameplay; several "Billboard" tips were
  actually plain `UniformCube` or `DirectionalCube`. Introduced new render modes beyond the
  original four: `InnerPillarBox` (small textured box inside a transparent cube), `InnerFlatPlate`
  (double-sided textured plate inside a transparent cube — used for non-rotating "billboard-like"
  objects such as fan-wind indicators), `TripleCrossBillboard` (3 textured planes at 60° angles,
  used for screw/bolt icons). Water render mode (icons 91/92/96) explicitly left open per user
  request.
- **Earlier (pushed before this session, still true):** `GalaxyEggbertCNA` renders `MoveObject`s as
  billboards (2026-07-06); `GoldPillar` rename + sample-world texture bug fix (2026-07-06);
  CNA-only direction locked (2026-07-05); tank controls + `MoveObject:` parsing for CNA
  (2026-07-05); gameplay-behavior specification `DOC-300`-`DOC-306` (2026-07-05); full
  `mobile-eggbert-reference/` catalog build-out, `DOC-100`-`DOC-267` (2026-07-03/04). See `plan.md`
  §15/§16 for full detail on anything summarized in one line here.

## 4. Current blocker / main problem

**Open issue: the Saw blade's ORIENTATION is still wrong after 3 fix attempts (4th round of live
user feedback, 2026-07-11) — do not attempt a 4th blind fix, read §8's newest task entry's full
technical analysis first.** User: "rovná strana pily musí být u země a kolo se zuby nahoru" (the
flat/straight side of the saw must be at ground level, the wheel-with-teeth must point up). New
finding not yet acted on: the real icon 378 texture's actual content occupies only the bottom half
of its 64×64 tile, and there's an unverified theory that `Easy3D::AppendPlateMesh` vertically
flips whatever texture it's given — see §3's newest entry and §8's newest task for the full
writeup. **The teleporter tip's SHAPE was confirmed correct/settled** in that same round of
feedback ("beze změny", no change) — but a SEPARATE duplicate-render bug (the cube's own side
faces baking in a second, fake, flattened copy of the tip's spike graphic) was reported and fixed
2026-07-12; see §3's relevant entry and plan.md's `E3D-MIG-147` follow-up. The shape itself was
never wrong, so this doesn't contradict the "beze změny" feedback above.

Separately, a real, deeper architectural collision limitation was found and tracked, not fixed —
see §5's relevant row (`GroundHeightAt()` treats a column's topmost solid block as the floor no
matter Blupi's own height, making any roofed/enclosed interior with a ceiling above an open floor
unreachable via normal walking). It bit twice this session (fan hazard task, then the Saw
render-mode work — same south tunnel both times) — worth fixing properly if a THIRD task needs to
walk-test something inside that tunnel. `GalaxyEggbertCNA` builds and runs cleanly on both the
EasyGL and Vulkan backends as of the most recent work (Saw anchor direction fix + teleporter tip
reverted to a pyramid, 2026-07-11, see §3's 2nd-newest entry), all 63/63
`GalaxyEggbertWorldsTests` pass, and all 5 verify tools (`VerifyBlupiMovement`,
`VerifyMoveObjectTypesCna`, `VerifyBigDecorParsingCna`, `VerifyInteractionSystem`, plus
`../easy-3d/tests/test_cube_mesh.cpp` built with `-DEASY3D_LINK_CNA=ON`) pass — none of these are
affected by the still-open Saw orientation issue, which is purely visual/UV-mapping.

**Terrain-tile identification and all 4 confirmed render modes are complete** (§1), plus the
teleporter's own extra pyramid-tip attachment geometry (a 5th, narrowly-scoped attachment, not one
of the 4 confirmed modes — went through a non-tapering box detour, reverted back to a pyramid
after a screenshot showed the box's 4 faces didn't visually connect at the bottom, now confirmed
settled by the user) and Saw/SawStopped wired into the `InnerFlatPlate` table with top-anchored
(floor-level) positioning and per-placement rotation metadata, but its internal texture
ORIENTATION is still wrong (see above) — no OTHER render-mechanism work remains outstanding on the
confirmed-icon front. **All 5 items from the 2026-07-11 live-playtest user feedback batch are
substantially addressed, except this one open Saw-orientation detail.**

**2026-07-12 autonomous session: Phases 13-16 are now fully/substantially complete, and Phase 17
is well underway** — a large single-session push through the whole gameplay-mechanics backlog, per
the user's own explicit priority order that day. **Phase 13 (Enemy AI) and Phase 14 (Hazards,
10/10, water breath gauge `148` done) are fully complete.** **Phase 15 (crates/lifts/bridges)** is
substantially done: linked-crate flood-fill (`150`), platform-lift riding (`152`/`154`, closing the
long-standing "no riding a moving platform" gap), and dynamite (`155`, the real 9-blast sequence)
are all done; `151`/`153`/`156`/`157`/`158` are deferred with documented reasons (see plan.md).
**Phase 16 (doors & keys)** is substantially done: key-gated (`160`/`161`) and treasure-gated
(`162`) doors both work; `163`-`165` deferred (render decision / hub-menu dependency). **Phase 17 (secret powers/vehicles/buffs) is now substantially complete** — `170`/`171`/`172`/
`174`/`175`/`178` all done. See §3's entries for: the "Sp0-Sp7" tile-icon documentation-error
correction (secret powers actually come from 4 separate pickups, now implemented, including real
Shield/Hide hazard immunity retrofitted onto essentially every hazard from earlier this session);
vehicle mounts (`171`, all 5 confirmed non-Balloon vehicle types), which also surfaced and fixed a
real general `TryMoveAxis()` falling-movement collision bug (unrelated to vehicles specifically,
but only exposed by them); the bullet pack pickup (`175`); and the teleporter double-tip render
fix (`E3D-MIG-147` follow-up). Every remaining Phase 17 item (`173`/`176`/`177`/`179`) is
deferred-by-design or genuinely blocked, not simply unstarted — see that phase's plan.md entries.

**Beyond Phase 17, a broad plan.md consistency sweep (2026-07-12/13) found and fixed several more
real, safe wins**: Phase 13's stale "nothing here is started" intro (contradicted by its own
already-complete task list) and Phase 10's `101`/`102`/`105` bullets (still pointing at "not
started" for phases that are now done) were corrected; `E3D-MIG-086` (buff activate/warning sound
pairs) turned out to already be fully implemented under `170`/`172`/`174`, just tracked under a
stale, never-existed `E3D-MIG-190` reference — fixed. **`E3D-MIG-084` (terrain-specific footstep
sound remap) was implemented**, and researching it surfaced a real, separate, pre-existing bug:
`GESound::PlayLand()` played channel 4 (real head-bump/ceiling-hit) instead of channel 3 (real
footstep-AND-landing), a mistake ported verbatim from `GalaxyEggbertSimple3D`'s own `GESound`
before the later, independently-verified channel research existed — fixed. `E3D-MIG-177`
(Ecrase collision-box mode + Suspended movement mode) and `E3D-MIG-085` (idle fidget sounds) were
researched and found genuinely blocked (documented non-goal for Ecrase's hitbox concept this
engine's single-point collision doesn't have; `needs_human` for Suspended/fidget, both blocked on
icon 202's pending "thin-bar" render-geometry decision, `E3D-MIG-513`).

**Phase 9 (HUD) started 2026-07-13 per explicit user direction ("začni fázi 9")** — see §3's
newest entries. Life/key/treasure text (already done 2026-07-10) plus new work this pass: bullet/
dynamite counters (`HUD-015`/`016`) and both real `jauge.png` gauges — water/Nage breath
(`HUD-012`/`018`) and the shared Shield/Power/Cloud/Hide countdown (`HUD-008`/`019`). A full
read-through of the real `Decor::DrawInfo` end to end confirmed every element it draws is now
either implemented or a correctly-identified real gap (`HUD-017` perso counter, blocked on an
unported "Perso" mechanic; `HUD-024` training hints, blocked on a "mission" concept + table
transcription approval) — **Phase 9's real, `DrawInfo`-backed scope is now essentially exhausted**
short of those 2 blocked items. Every other `HUD-0NN` entry (score, world/timer, hit-flash,
exit-open popup, etc.) was NOT found in `DrawInfo` and is flagged `[?]` for independent
verification, not assumed real — see plan.md's Phase 9/§2.3 for the full breakdown.
**Recommended next step**: the Saw blade orientation is still the one item needing the user's own
visual judgment (see below — do not guess again); beyond that, further Phase 9 work needs either
resolving the `[?]`-flagged items' real source first, or picking up `HUD-017`/`024`'s own
blockers (a "Perso" mechanic, a "mission" concept + transcription approval) — both are
independent research/scoping tasks, not quick continuations of what's already done.

**1 open item, see §8's newest task entries for full detail**:
1. **Saw blade orientation — paused, needs careful re-investigation before the next attempt** (not
   a quick fix, see §3/§8's detailed writeup — don't guess again live without reading it first).
   Per explicit user direction (2026-07-12): do NOT guess a fix autonomously — this needs direct
   user visual judgment (reading the crop / a screenshot), same category as the other "new visual
   design decision" items below. Left paused, not attempted this session.

(The platform lift clipping item and the teleporter double-tip render bug that used to be listed
here are both DONE — see §3's two newest entries, 2026-07-12. The double-tip bug was the cube's
own side faces baking in a duplicate, flattened copy of the same spike graphic the real 3D tip
mesh already used — not a duplicate-append/loop bug as originally suspected; see plan.md's
`E3D-MIG-147` follow-up entry for the full root cause. This likely also explains the previously-
unreproduced 2026-07-11 report "zadní teleportér renderuje dopředu, je to rozbité" — same
underlying visual artifact.)

**New visual/artistic render-geometry decisions are deliberately SKIPPED this session, per explicit
user direction (2026-07-12)**, given the Saw's own history of repeated wrong live guesses:
`ThinMechanical` geometry for ~25 icons (`E3D-MIG-510`), water/liquid surface treatment
(`E3D-MIG-512`), architectural-kit assembly (`E3D-MIG-514`), the hub-screen world-select icon
render (`E3D-MIG-515`, re-scoped 2026-07-12 — its old "secret-power render" premise was wrong,
see §3's newest entry; it's hub/menu-screen territory now, not blocked on `170` anymore since
`170` itself is resolved), and the enemy billboard walk-cycle direction mismatch
(`E3D-MIG-179`, no resolution proposed anywhere — do not attempt one without the user). These are
`needs_human` — left exactly as `plan.md` already marks them (`[?]`), not attempted, not guessed.
Non-visual gameplay-logic tasks (Phases 14-17) are being worked instead, per the user's explicit
priority order (2026-07-12): continue gameplay mechanics in phase order before any Menu/HUD/Save/
Score UI work.

- **Done (Phase 13, complete)**: lives/respawn foundation (`130`), the real shared patrol-turn
  state machine (`131`, unblocked `134`/`136`), the widened shared enemy kill-list covering 8
  types (`132`/`133`/`137` contact-death), the wasp's balloon status + hazard-pop interaction
  (`135`), blupih/blupit's projectile attacks (`134`), the large creature's turn-dwell-gated grab
  (`136`), and follower wake+homing (`137`). Also done outside Phase 13/14: the real
  mobile-eggbert-faithful `GEHud`, sound, mouse-look + F11 fullscreen, the `CubeMesh` winding
  root-cause fix, and the sample world's tile+object exhibition areas.
- **Done (Phase 14, 9/10)**: all 5 real terrain hazard tiles (lava/spikes/blitz/saw+switches/
  crusher, `140`-`144`), the spring/bounce tile (`145`), the Temp/vanishing tile (`146`), the
  teleporter (`147`, including its render geometry; the first Phase 14 mechanic that needed a
  genuine 3D-adaptation redesign, since the real "tile above Blupi" detection turned out to
  require teleporter icons to be non-solid for collision, resolved via `IsTeleporterIcon()`'s
  exclusion in `GroundHeightAt()` rather than the original solid-pillar/facing-based workaround,
  which shipped a real live bug before being fixed), and fans (`149`, see §3's newest entry —
  found and worked around a related, deeper `GroundHeightAt()` limitation with roofed interiors,
  tracked in §5, not fixed), and the water breath gauge (`148`, 2026-07-12, see §3's newest entry
  — required making water non-solid for collision, a real architectural fix in the same family as
  the teleporter/fan exclusions, plus a content-bug fix to the tunnel's own pre-existing water
  crossing). **Phase 14 (Hazards) is now fully complete, 10/10.**
- **Next up: Phase 15** (crates/lifts/bridges full fidelity, `E3D-MIG-150`-`158`). Phases 16
  (doors/keys) and 17 (secret powers/vehicles) are not started at all — see `plan.md` for the
  itemized task lists.

## 5. Known bugs and limitations

| Status | Issue |
|---|---|
| **CNA bug, worked around in galaxy-eggbert (2026-07-10) — needs an upstream CNA fix (approval required)** | **CNA's Vulkan backend records all SpriteBatch batches BEFORE all 3D draws within each frame** (`RecordCommandBuffer`: `drawSpritesFor` then `draw3DFor`, 2D pipeline depth test+write disabled), so a sprite HUD drawn after the 3D scene is painted over by it — the real cause of the twice-reported "HUD icon visible for a second, then gone" (EasyGL draws in submission order and only had the separate, already-fixed depth-test issue). `feature/graphics` has the same ordering. galaxy-eggbert no longer uses SpriteBatch at all (`GEHud` draws real 3D quads), so it is no longer affected — but any future SpriteBatch use would be. |
| **CNA quirk, worked around (2026-07-10)** | A `BasicEffect` draw with `Alpha < 1` renders on EasyGL but not at all on CNA's Vulkan backend (verified: identical quad appears at Alpha=1.0, vanishes at 0.6). `GEHud`'s treasure panel uses 1.0 instead of the real mobile-eggbert 0.6 until this is fixed upstream (documented at `kPanelOpacity`). |
| **resolved (2026-07-10, root-caused)** | **"Front/top/side faces don't render" — `Easy3D::CubeMesh` wound its cube faces with the OpenGL CCW-from-outside convention, but CNA implements genuine XNA culling (visually-clockwise triangles survive the default `CullCounterClockwise`), so every side face was invisible from outside; scenes showed the mirrored interiors of opposite faces instead, breaking visibly wherever no opposite face covered the sightline (pillars missing front faces, staircases see-through to background).** Fixed at the source (`easy-3d@44393f5` — all 6 faces now CW-from-outside, completing what `a26df1a`/`8ab3854` started empirically); side-face textures now also render upright/unmirrored. Verified live on both backends (§3). CNA itself needs no fix — its cull semantics match real XNA, confirmed against its own cullmode golden test and FNA SpriteBatch winding; CNA `feature/graphics` doesn't change culling either. |
| **needs re-test after the winding fix (2026-07-10)** | The **texture distance washout** investigation (`texture-distance-washout-bug.md`) was conducted while side faces showed their mirrored opposites — its "geometry/winding proven correct" conclusion is void and the repro must be re-run on top of `easy-3d@44393f5` before resuming (addendum added to the doc). The washout symptom may or may not still exist. |
| **needs user re-check (2026-07-10)** | FanLeft/FanRight base/open face assignments (`GEDirectionalCubeTiles.cpp`) were tuned via live user feedback *while the winding bug was active* — i.e., judged on mirrored opposite faces. The tunnel fan's outer face looks correct in the post-fix verification screenshot, but fans should be re-checked in play and the assignments swapped back if wrong. |
| resolved (2026-07-09) | `Easy3D::AppendBillboardMesh`'s fixed vertex winding was back-facing under `GalaxyEggbertCNA`'s default `CullCounterClockwise` rasterizer state, making `MoveObject`/`BigDecor` billboards invisible on screen despite correct vertex/index math. Fixed at the source (`easy-3d@a26df1a`, winding reversed, pinned test updated) — no more per-call `RasterizerState::CullNone` workaround needed anywhere, confirmed via the background quad rendering identically without it (§3). |
| resolved (2026-07-09) | All ~99 confirmed `DirectionalCube` icons wired up — the last 6 (15-18, 108-109) used this session's ambiguous-icon default since their crops didn't give a confident facing read (§3). |
| root-caused and substantially mitigated (2026-07-09), not fully eliminated | **Thin blue (sky-clear-color) seam lines along block edges in `GalaxyEggbertCNA`**, originally reported 2026-07-08. Root cause: `GETileAtlas::GetTileUv()` had no UV inset, so bilinear filtering bled the atlas's 1px inter-tile gap in at oblique/close angles (§3) — fixed by reusing `BlockTypes::tileUV()`'s already-proven half-texel inset (previously used only by the historical Simple3D target, never ported to CNA). Measured fix: fully-transparent "hole" pixels at a reproduction screenshot dropped 60% (1217→486 of 384000 total pixels). Residual transparency remains, plausibly ordinary MSAA/silhouette antialiasing (a separate, likely-benign effect) or an inset that's still slightly too small at extreme grazing angles — not investigated further; see §3 for exact numbers. |
| incomplete | `GalaxyEggbertCNA`: no Blupi/object-behavior rendering beyond billboards/cubes, no HUD, no sound, no gameplay logic, no interactive object system (expected at this phase) — platform lifts/crates render but don't move or respond to Blupi yet. |
| resolved (re-verified 2026-07-09) | `GalaxyEggbertWorldsTests` — 54/54 still pass (via `build-cna`, see §7). The `cmake-build-debug` `ctest` discovery issue is unrelated to that tree and not re-checked (not to be built, per §9). |
| not to be fixed (per user, 2026-07-08) | Simple3D build fails at `find_package(Urho3D)` — U3D prebuilt missing/incompatible. `GalaxyEggbertSimple3D` is treated as historical reference only going forward; do not spend effort rebuilding/fixing it (see §2). |
| incomplete | `element.png` used for every remaining `ObjectType` billboard, even though types 32/33 need `blupi1.png` (`DOC-007`). Types 1/12/47/48 (as `UniformCube`s) and 14/15/31/35/52 (as `IsObjectMPngSourced` billboards) are no longer part of this gap in `GalaxyEggbertCNA` — they correctly source `object-m.png` instead (2026-07-09, §3); `GalaxyEggbertSimple3D`'s equivalent bug remains unfixed (historical reference only, not built/fixed going forward). |
| resolved (2026-07-09) | `GEObjectIcons::GetObjIcon()` icon coverage is now 66/69 confirmed `ObjectType`s (up from 31 at session start), and (as of the same day, a later pass) every `MoveObject` billboard actually animates via a real per-instance phase timer, not just a frozen phase=0 snapshot — including `38` (electric arc)'s full two-channel behavior (`blupi1.png` ticks 0-29, `element.png` ticks 30-89, real `table_electro` data). `0`/`18`/`22`/`58` are correctly `default: return 0` (no icon exists in source data for them) — no known gaps remain among confirmed types or their animation. |
| incomplete | Simple3D: no per-zone fog, only `SetClearColor` per sky region. |
| incomplete | 7 `ObjectType`s (jeep/secret-exit/skateboard/suction-cup/mirror/balloon/dynamite) spawn with correct icons in Simple3D but have no real gameplay behavior on pickup/contact. |
| unknown | Simple3D Android/Web builds untested since the last engine change. |
| **accepted as a known limitation (found 2026-07-09, user declined to approve a `../cna`/SDL fix 2026-07-09)** | **`GalaxyEggbertCNA` exits with code 1, not 0, when closed via a real window-manager close request** (verified via `xdotool windowclose` against a real X11 window under Xvfb, see §3/§7) — reproducible 100% (2/2 runs, identical X error each time): an `X Error of failed request: BadWindow (invalid Window parameter)` on the `XInputExtension` major opcode, referencing the just-closed window's resource id, printed right after the final frame's `Draw()` output. No leaked process/zombie afterward — the process does fully terminate, just with the wrong exit code. Root cause is inside SDL's own X11 backend window-teardown sequence (CNA's `Game::Exit()` is a 2-line flag-setter; galaxy-eggbert's `GalaxyEggbertCnaGame`/`main.cpp` do no custom shutdown) — fixing it needs `../cna`/SDL changes, which needs explicit approval per `CLAUDE.md`; asked, user chose to leave this as a known limitation rather than approve that work. **Do not attempt a fix without new, explicit approval.** |
| needs verification | Simple3D: stomp bounce height (`kJumpSpeed * 0.65f`) vs. mobile-eggbert's real feel. |
| needs verification | Simple3D: crate push floor-support check only tested at y=0; mobile-eggbert links crate stacks vertically (`SearchLinkCaisse`) — whether galaxy-eggbert's port does too is unconfirmed. |
| risky assumption | `GalaxyEggbertCNA`'s world/texture loader uses relative paths — only works when run from its own build directory. |
| incomplete | `GETerrainRenderer` (CNA) face culling only covers the plain static `UniformCube` path (2026-07-09, §3) — the animated/water paths render every face unconditionally, fine while those are sparse decorative elements rather than bulk fills. |
| resolved (2026-07-11) | `GalaxyEggbertCNA`'s animation-state icon (bottom-right corner) disappeared after ~1s of play — depth testing was never disabled before the 2D `SpriteBatch` overlay draws, so the icon's screen-space quad was depth-tested against whatever 3D geometry had already written to that pixel once the camera turned toward nearby terrain/walls. Fixed by disabling depth test before both `SpriteBatch` blocks (§3). |
| resolved (2026-07-11) | `GalaxyEggbertCNA` crashed after a while of play with an uncaught `System::InvalidOperationException` from `SoundEffectInstance::setIsLoopedProperty` (user-supplied debugger backtrace pinpointed `GESound::PlayStep`/`Play`) — that call is only valid before an instance's first `Play()`, but `GESound::Play()` called it unconditionally even when reusing an already-started instance. Fixed by only calling it once, right after constructing a fresh instance (§3). Verified empirically (3/3 live runs crashed before the fix, 3/3 clean after) rather than via an automated test, since exercising `GESound::LoadContent()` needs a real audio device every other scripted verification tool deliberately avoids. |
| needs investigation | `GalaxyEggbertCNA` under Vulkan specifically: the end-of-frame diagnostic screenshot (`screenshot_hud.png`) shows a plain blue background with no visible terrain and un-blended white boxes around every billboard, even though the same frame's earlier terrain-visibility pixel-sample check reports 25/25 real terrain color. Found 2026-07-11 while verifying an unrelated fix; not investigated — may be a `GetBackBufferData`/swapchain timing quirk specific to calling it twice in one frame under Vulkan, or something else entirely. EasyGL's equivalent screenshot is unaffected. |
| **real architectural limitation, found 2026-07-11 while building the fan hazard (plan.md `E3D-MIG-149`), not fixed** | **`GEBlupiController::GroundHeightAt()` always resolves a column's "floor" as the single TOPMOST solid block in that entire column** (scanning from the top of the world downward), with no concept of "the nearest solid surface AT OR BELOW my own current height." A solid ceiling anywhere above an otherwise-open interior (e.g. the south tunnel's own `y=3` `BrickWall` roof over its walkable `y=1` interior) makes that interior's REAL floor completely unreachable via normal walking — `TryMoveAxis`'s target-column check sees the ceiling as the floor (`y=4`, not `y=1`) and blocks the step-up, or a raw `SetPosition()` teleport into the interior resolves `onGround` at the ceiling's height instead of the real floor beneath it. Same root cause as the original (later-fixed) teleporter bug — a floating solid block anywhere above Blupi in a column poisons that whole column's ground-height query — except triggered by a ceiling instead of a pillar, and NOT fixed here (unlike the teleporter/fan icons themselves, which are narrowly excluded from the solid-block scan): a general fix needs the scan to consider Blupi's own current Y, not just "topmost solid in the column," which could subtly affect every other already-shipped hazard/mechanic's collision behavior and wasn't attempted given the scope. Confirmed via a standalone scripted walk test (not just single-position `Step()` calls, which is all every hazard's own existing verification — including the tunnel's own already-shipped switch/saw pair — actually exercises; none of them walk-tested entering that specific enclosed interior either). Worked around for the fan task by placing its 2 reachable hazard instances in open-sky rooms (matching the teleporter's own precedent) instead of the tunnel's roofed interior — see `tools/GenerateSampleWorld3D.cpp`'s fan-alcove comment. **Any future placement of a walk-under/floating hazard inside a roofed/enclosed space should avoid this until fixed.** **Hit again 2026-07-11 (same day) live-verifying the Saw render-mode fix** — the switch+saw pair sits in this exact same tunnel interior; a debug `SetPosition()` there again resolved `blupi_`'s Y to 4 (roof-top) instead of 1 (real floor), silently invalidating over a dozen camera-angle attempts before being diagnosed via a `[CAM DEBUG]` log comparing intended vs. actual camera pose — worked around that time by setting `camera_`'s position/target directly for the one verification screenshot, bypassing `blupi_`'s corrupted Y entirely. Two independent hits in one session on the same tunnel is a good sign this is worth fixing properly before a third task needs to walk-test anything inside it. |

## 6. Architecture notes

### Main modules
```
include/GalaxyEggbert/Worlds/, src/GalaxyEggbert/Worlds/   — engine-agnostic voxel World (100×100
                                                               grid), Block/Chunk, .vwr save format.
                                                               Shared by BOTH targets. Chunk's sparse
                                                               ChunkBlockMetadataRecord mechanism
                                                               (setExtraMetadata/extraMetadata(),
                                                               World::setBlockExtraMetadata/
                                                               collectExtraMetadata wrappers added
                                                               2026-07-09) stores arbitrary per-block
                                                               payloads -- now actively used by
                                                               MoveObjectRecord (below), not just
                                                               theoretical capability.
include/GalaxyEggbert/MoveObjectRecord.hpp,
src/GalaxyEggbert/MoveObjectRecord.cpp                      — engine-agnostic (added 2026-07-09,
                                                               §3), deliberately outside Worlds/:
                                                               encodes/decodes a MoveObject
                                                               (pickup/enemy/lift/crate) as a
                                                               World block-extra-metadata payload
                                                               (metadataType=1), anchored at
                                                               floor(posStart) in World's own RAW
                                                               GRID space (not CNA's centered render
                                                               space -- see its own doc comment).
                                                               PlaceMoveObject()/CollectMoveObjects().
                                                               Shared by BOTH targets (only CNA's
                                                               GEWorldRuntime consumes it so far).
include/GalaxyEggbert/BlockTypes.hpp                        — tile type constants; block type =
                                                               icon index in object-m.png. Now
                                                               includes RockPile(35)/BrickWall(261)
                                                               as confirmed-genuine bulk material,
                                                               alongside Ground/StoneA/StoneB which
                                                               are explicitly documented as WRONG
                                                               names (machine pieces, not terrain)
                                                               — do not use them for new bulk fills.

src/GalaxyEggbertSimple3D/   — historical reference only as of 2026-07-08 (not built/maintained).
                                Its `GEWorldRuntime.cpp` fallback-world Ground/StoneA/StoneB fix
                                (§3) went in but was not build-verified (§2/§9).

src/GalaxyEggbertCNA/        — GalaxyEggbertCnaGame owns GEWorldRuntime, GETileAtlas,
                                GETerrainRenderer (World, all Y layers → 5 CubeMeshRenderer-backed
                                meshes: static opaque, static-but-transparent (icons 30/31, built
                                once, never rebuilt by Update() — NeedsAlphaBlend()), grass-top
                                overlay (icon 107, also built once — IsGrassTopIcon(), drawn via a
                                SEPARATE public DrawGrass() call with its own texture/effect since
                                BasicEffect only binds one texture at a time), animated opaque, and
                                water (animated + transparent, its own m_waterRenderer/
                                m_waterBlocks and RebuildAnimatedRenderer() call); the 2 transparent
                                renderers draw last, sharing one BlendState::NonPremultiplied/
                                DepthStencilState::DepthRead state change. Per block, one shared
                                AppendSpecialGeometry() helper checks all 4 special render-mode
                                tables in order — DirectionalCube, InnerPillarBox, InnerFlatPlate,
                                TripleCrossBillboard — falling back to plain UniformCube; both the
                                opaque-animated and water paths call the same helper, keyed by
                                animation-group base icon, not the current frame's icon; the
                                static-opaque path's plain-UniformCube fallback now face-culls via
                                IsOccluderBlock() (2026-07-09, §3) -- a real DirectionalCubeItem
                                with all 6 faces Visible/same Uv, not the old CubeBatch path, so a
                                face is only omitted when a definitely-solid neighbor covers it;
                                the animated/water paths are NOT face-culled yet, see §8 task 3),
                                GEDirectionalCubeTiles (all ~99 confirmed DirectionalCube icons,
                                completed 2026-07-09 -- icons 108/109 are the only entries needing
                                a SECOND icon's own UV, icon 107's, threaded via a new icon107Uv
                                parameter on TryGetDirectionalCubeFaces/AppendSpecialGeometry),
                                GEInnerPillarBoxTiles (3 of 3), GEInnerFlatPlateTiles (63 of 63 --
                                58 use the default PlateAxis::Z, icons 368-372 use PlateAxis::Y
                                per a 2026-07-09 crop spot-check, §3),
                                GETripleCrossBillboardTiles (10 of 10), GESwatchUv (shared "flat
                                fallback color" sampling helper), GEBlupiController, GEObjectIcons,
                                an Easy3D::Camera3D (first-person). GalaxyEggbertCnaGame also owns
                                bigDecorCells_/bigDecorEffect_/bigDecorMeshRenderer_ (added
                                2026-07-09, §3) — BigDecor: cells rendered as camera-facing
                                billboards via terrainTexture_/tileAtlas_, same technique as the
                                MoveObject billboards but object-m.png's icon vocabulary instead of
                                element.png's. Only ever non-empty when a world was loaded via
                                LoadFromMobileEggbertFile() — the default .vwr world's GetBigDecor()
                                is empty (LoadFromVwrFile() clears it), guarded explicitly since a
                                2026-07-09 segfault (see §3). Also owns objectCubeEffect_/
                                objectCubeMeshRenderer_ (added 2026-07-09, §3) — the platform-lift/
                                crate UniformCube object path, built once (not per-frame) from
                                worldRuntime_.GetMobileObjects() filtered by
                                GEObjectIcons::IsUniformCubeObject(), same terrainTexture_ sheet as
                                BigDecor/terrain. MoveObjects of these types are skipped in the
                                billboard-building loop so they aren't drawn twice.

tools/GenerateSampleWorld3D.cpp — builds worlds3d/world001.vwr. Ground floor/staircase/platform
                                use RockPile, walls/pillars use BrickWall (no more Ground/StoneA/
                                StoneB usage); a demo row of DirectionalCube/InnerPillarBox/
                                InnerFlatPlate/TripleCrossBillboard blocks near spawn, plus a
                                separate icon-107 grass patch on the ground floor itself — all
                                added 2026-07-08 (§3).

textures3d/                  — galaxy-eggbert-owned 3D-specific textures NOT sourced from
                                mobile-eggbert (added 2026-07-08) — grass_top.png (procedurally
                                generated, icon 107's grass-top overlay). Copied next to
                                GalaxyEggbertCNA at build time exactly like worlds3d/ (see
                                CMakeLists.txt) — deliberately separate from Content/, which gets
                                fully re-copied fresh from ../mobile-eggbert every build and so
                                cannot hold anything galaxy-eggbert-owned.

mobile-eggbert-reference/    — questionnaire-unidentified-tiles.md (round 1, 34 icons, DONE),
                                questionnaire-all-remaining-tiles.md (round 2, 280 icons, DONE),
                                questionnaire-unused-tiles.md (round 3, 97 icons, DONE) — together
                                account for all 441 object-m.png icons (see §1). 02-tiles.md is the
                                canonical catalog; its "Unused/unnamed icons" section now carries a
                                correction note about the MoveObject icon= finding (§3).

../easy-3d/                  — companion library beside CNA. Standing permission to modify.
                                CubeMesh/CubeMeshRenderer (terrain: UniformCube, DirectionalCube via
                                AppendDirectionalCubeMesh, PlateItem/AppendPlateMesh — now with
                                PlateAxis::Y for horizontal surfaces, added 2026-07-08 for the grass
                                overlay — TripleCrossItem/AppendTripleCrossMesh, all emitting plain
                                CubeVertex so CubeMeshRenderer/BasicEffect needed no changes),
                                BillboardMesh/BillboardMeshRenderer (objects). InnerPillarBox reuses
                                DirectionalCubeItem directly (no new Easy3D code needed).
```

### Important invariants
- Block type = icon index in `object-m.png` (20 cols, 64×64 px tiles, 1px gap + 1px leading
  margin) — no separate mapping table.
- World grid is 100×100; both `GEWorldRuntime` implementations independently define a
  50-offset world-center constant.
- `ObjectType` (204 IDs) and `SoundChannel` (93 IDs) are confirmed numerically identical to
  mobile-eggbert's versions — do not renumber.
- No `#ifdef GE_ENGINE_*` anywhere.
- `CubeMesh`'s frame index / UV assignment convention is load-bearing for `GETileAtlas` — do not
  change one side without the other.
- **New invariant (2026-07-08): `BlockTypes::Ground`/`StoneA`/`StoneB` must not be used for new
  bulk-terrain fills** — all three are confirmed mislabeled (machine-piece graphics, not stone/
  ground). Use `RockPile`/`BrickWall` (or another icon confirmed genuine via the round 2/3
  questionnaires) instead.
- **New invariant (2026-07-08): only add an icon to `GEDirectionalCubeTiles.cpp`/
  `GEInnerPillarBoxTiles.cpp`/`GEInnerFlatPlateTiles.cpp`/`GETripleCrossBillboardTiles.cpp` with its
  exact confirmed answer from the questionnaire files** (`questionnaire-all-remaining-tiles.md` /
  `questionnaire-unused-tiles.md` / `02-tiles.md`) — never guess a new icon's render mode or face
  config (see §9).
- **New invariant (2026-07-08): `PlateItem`/`TripleCrossItem`/`DirectionalCubeItem` are the only
  "special geometry" primitives — add a new one only for a genuinely new shape**, not as a
  workaround inside `GETerrainRenderer`. `InnerPillarBox` deliberately has NO dedicated Easy3D type
  — it's just `DirectionalCubeItem` with a smaller `Size`.
- Build with `-j2` maximum (32 GB RAM constraint; crashes observed with more parallel jobs).

### Boundaries that must remain stable
- `BlockTypes::fromMobileIconId()` — must match the mobile-eggbert world file format exactly.
- `ObjectType`/`SoundChannel` numeric values — stored in level files, must never be renumbered.
- `GEDecorSystem::GetObjIcon`/`GEObjectIcons::GetObjIcon` — do not change without cross-referencing
  mobile-eggbert's `Decor.cpp`.
- `src/GalaxyEggbertSimple3D/` must not be mutated into the CNA implementation.
- mobile-eggbert stays read-only, never modified even temporarily; no code/data copied from it
  without explicit user approval.
- `../cna` and `../simple-3d` are sibling repos: read freely when needed, modify only with
  explicit, per-change approval. `../easy-3d` has standing permission (scoped to small, generic
  3D-batching helpers).

## 7. Useful commands

```bash
# Configure + build Simple3D (default target):
cmake -S . -B build
cmake --build build --target GalaxyEggbertSimple3D -j2
./build/GalaxyEggbertSimple3D

# Build + run world-model unit tests (prefer build-cna, see below -- this `build` tree also
# configures Simple3D, which is not to be built per §9):
cmake --build build --target GalaxyEggbertWorldsTests -j2
ctest --test-dir build --output-on-failure

# Configure + build the CNA target (opt-in, off by default):
cmake -S . -B build-cna -DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF
cmake --build build-cna --target GalaxyEggbertCNA -j2
cd build-cna && ./GalaxyEggbertCNA        # must run from its own build dir (relative asset paths)

# Build + run world-model unit tests from the Simple3D-OFF tree (preferred -- 61/61 expected,
# last confirmed 2026-07-09; run the binary directly, not `ctest --test-dir build-cna`, which also
# discovers unrelated easy-gl/meta-gl dependency-subproject tests in this tree):
cmake --build build-cna --target GalaxyEggbertWorldsTests -j2
./build-cna/GalaxyEggbertWorldsTests

# Regenerate the hand-authored 3D sample world (after tools/GenerateSampleWorld3D.cpp changes):
cmake --build build-cna --target GenerateSampleWorld3D -j2
./build-cna/GenerateSampleWorld3D worlds3d/world001.vwr

# Scripted verification tools (engine-agnostic, no CNA link needed):
cmake --build build-cna --target VerifyBlupiMovement VerifyMoveObjectTypesCna VerifyBigDecorParsingCna -j2
./build-cna/VerifyBlupiMovement          # Blupi collision/step-up/gravity vs. worlds3d/world001.vwr
./build-cna/VerifyMoveObjectTypesCna     # MoveObject parsing vs. 12 real mobile-eggbert level files
./build-cna/VerifyBigDecorParsingCna     # BigDecor: parsing vs. real mobile-eggbert level files (run from repo root, not build-cna — uses ../mobile-eggbert relative paths)

# Scripted verification tools that link CNA (platform lifts/crates/pickups, training hints,
# on-screen controls) -- run from repo root too, same ../mobile-eggbert-relative-path quirk as
# VerifyBigDecorParsingCna above:
cmake --build build-cna --target VerifyInteractionSystem VerifyGEInputPad -j2
./build-cna/VerifyInteractionSystem      # platform lift/crate/pickup/training-hint behavior vs. worlds3d/world001.vwr
./build-cna/VerifyGEInputPad             # on-screen D-pad/Jump/Action/Pause + Pause-row hit-testing (synthetic MouseState, no GraphicsDevice needed)

# easy-3d: default (headers-only) build + tests
cmake -S ../easy-3d -B /tmp/e3d-build -DEASY3D_CNA_DIR=../cna
cmake --build /tmp/e3d-build -j2 && ctest --test-dir /tmp/e3d-build --output-on-failure

# easy-3d: CNA-linked build + tests (heavier — builds CNA/SHARP_RUNTIME/EasyGL too)
cmake -S ../easy-3d -B /tmp/e3d-build-cna -DEASY3D_LINK_CNA=ON -DEASY3D_CNA_BACKEND=EASY_GL -DEASY3D_CNA_DIR=../cna
cmake --build /tmp/e3d-build-cna -j2 && ctest --test-dir /tmp/e3d-build-cna --output-on-failure

# Reference: mobile-eggbert animation tables / gameplay logic (read-only)
grep -n "table_decor\|table_blupi" ../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Tables.cpp
less ../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Decor.cpp

# Real window-manager-close test (found 2026-07-09, §3/§5 — this environment already has an
# isolated Xvfb :99 running plus xdotool installed; DISPLAY=:99 alone does NOT create a real X11
# window, SDL_VIDEODRIVER=x11 is required to force one):
cd build-cna
DISPLAY=:99 SDL_VIDEODRIVER=x11 ./GalaxyEggbertCNA &
GAME_PID=$!
sleep 2
WIN_ID=$(DISPLAY=:99 xdotool search --name "Galaxy Eggbert")
DISPLAY=:99 xdotool windowclose "$WIN_ID"   # real WM_DELETE_WINDOW -> SDL_EVENT_QUIT -> Game::Exit()
wait "$GAME_PID"; echo "EXIT_CODE=$?"       # currently 1, not 0 -- see §5's known-bug entry
```

No `.clang-format`/`.clang-tidy` config exists in this repo — no lint/format tooling to run.

## 8. Next smallest tasks

**Autonomous session priority order, set explicitly by the user (2026-07-12) — supersedes older
priority notes below until this list is exhausted:**

1. Platform lift clipping + more lifts — **DONE**, see §3's newest entry / old task 3 below.
2. Water breath gauge (`E3D-MIG-148`) — **DONE**, see §3's newest entry. Phase 14 now 10/10 complete.
3. Phase 15 (`E3D-MIG-150`-`158`) — **substantially done** (150/152/154/155 done; 151/153/156/157/
   158 deferred with documented reasons, see plan.md).
4. Phase 16 (`E3D-MIG-160`-`165`) — **substantially done** (160/161/162 done; 163/164/165 deferred).
5. Phase 17 (`E3D-MIG-170`-`179`) — **substantially complete**: `170`/`171`/`172`/`174`/`175`/`178`
   done (secret powers + real hazard immunity + vehicle mounts + bullet pack pickup; vehicles also
   yielded a real, general `TryMoveAxis()` falling-movement collision-bug fix; `178`'s "movement
   table" was the same deliverable as `171`'s own speed/accel constants, not separate work — see
   §3's newest entries). Every remaining item is deferred/blocked, not just unstarted: `173`
   (2-stage pickup delay, a deliberate documented simplification), `176` (sparkle-fx, needs a
   particle system that doesn't exist), `177` (Ecrase collision-box mode is now a documented
   non-goal — this engine's single-point collision has no hitbox to shrink; Suspended movement
   mode is fully researched/speced but `needs_human` — blocked on icon 202's pending
   render-geometry decision, see §3's newest entry), `179` (needs_human, enemy billboard mismatch).
   Bullet FIRING itself (vs. the ammo-count pickup done in `175`) needs a new player-fired
   projectile system and is tracked as its own not-yet-scoped follow-up. **No further Phase 17
   work is safely actionable without either human input (render-geometry calls) or a new,
   separately-scoped subsystem (particle effects, player-fired projectiles).**
6. Interleaved as time allows: Saw investigation (paused, needs the user's own visual judgment per
   their 2026-07-12 direction, not attempted). The teleporter double-tip bug is **DONE** — see
   §3's newest entry and plan.md's `E3D-MIG-147` follow-up.
7. Explicitly SKIPPED per the user (2026-07-12): any new visual/artistic render-geometry decision
   (`E3D-MIG-510`/`512`/`514`/`515`, `179`) — mark `needs_human`, do not guess. Menu/HUD/Save/Score
   UI work (plan.md §2.2 "PRIORITY" label) is explicitly LOWER priority than the phases above this
   session — the label predates the 2026-07-10 CNA rewrite reset and wasn't re-affirmed.

**User-reported live-playtest feedback (2026-07-11) — these supersede the Phase 14 water-gauge/
fans pick as the current priority; work through them in this order (P1 items are real bugs, not
polish):**

- **P1 — DONE (2026-07-11): teleporter fixed.** Was broken (froze Blupi permanently, never
  transported him) — root cause: the shipped design (`GEBlupiController::GetBlockTypeInFront()`,
  from `E3D-MIG-147`) checked the cell Blupi is FACING, requiring a solid wall he walks up to.
  Redesigned per the user's explicit direction back to the real "one tile above his feet" check:
  teleporter icons (330-333) are now ALWAYS non-solid for collision (`GroundHeightAt`'s own
  `IsTeleporterIcon()` skip), so Blupi genuinely walks into the open space beneath a floating
  pillar; detection reverted to `GetBlockTypeAbove()`. **A second real bug was found and fixed in
  the same pass**: landing exactly beneath the destination pillar immediately re-triggered another
  teleport (an infinite ping-pong) — fixed by offsetting the landing position one cell away from
  directly beneath the matched pillar. Rebuilt the sample-world teleporter rooms as open floors
  with a floating pillar overhead (no walls). **Verified live this time** (temporary debug
  instrumentation — spawn override + periodic position/state log, reverted before committing —
  confirmed the exact 6.4s transit timing, correct destination coordinates, and no re-trigger
  loop), not just via unit tests, since the unit tests for the FIRST (broken) design all passed
  despite the real bug. See `plan.md`'s `E3D-MIG-147` entry for full detail.
- **P1 — DONE (2026-07-11): fall-off-world death fixed.** Was confirmed unreachable via normal
  walking (`GEBlupiController::GroundHeightAt()`'s "no solid block in this column" fallback
  silently acted as solid ground at Y=0) — fixed via a new `kNoGround` sentinel, verified live
  (Blupi now genuinely falls, goes Y-negative, and the existing `kFallDeathY=-5.0f` death/respawn
  fires correctly). See §3's newest entry for full detail.
- **P2 — DONE (2026-07-11): real last-safe-position FIFO respawn implemented.** See §3's newest
  entry for full detail.
- **P1 — DONE (2026-07-11): fall-death timing fixed.** Was firing in well under 1 second after
  falling through a gap; the user correctly recalled the real game gives a real, multi-second
  fall. Root-caused against a real level file (`../mobile-eggbert/worlds/world001.txt` has a
  deliberate ~77-row/~4928px empty margin below its real terrain before the real row-99 death
  check fires) and the real gravity constants (`Decor.cpp:2966-2968` — terminal velocity is
  actually 21, not 20, a genuinely new correction to the existing reference doc's rounded
  summary). `kFallDeathY` moved from `-5.0f` to `-60.0f`, giving a comparable ~6.2s fall using
  this engine's own gravity constants — verified live at 6.72s. See §3's newest entry for full
  detail.
- **P3 — DONE (2026-07-11): teleporter pyramid-tip render geometry added.** New Easy3D primitive
  `Easy3D::PyramidTipItem`/`AppendPyramidTipMesh()` (`../easy-3d/{include,src}/Easy3D/
  CubeMesh.{hpp,cpp}`) — a square top face + 4 triangles tapering to an apex below (16 vertices, 18
  indices; winding hand-verified via cross product, reusing `ComputeFaceCorners`' own -Y face
  corner order), unit-tested in `../easy-3d/tests/test_cube_mesh.cpp` (vertex/index counts, apex
  position, winding — run via `-DEASY3D_LINK_CNA=ON`, since plain compile-checking doesn't execute
  assertions). Wired into `GETerrainRenderer.cpp`'s `AppendSpecialGeometry()`, appended right after
  the teleporter's existing `DirectionalCube` cube (new `IsPyramidTipIcon()`/`PyramidTipUv()`
  helpers) — the tip hangs from the cube's bottom face (`Center.Y - 0.5`), `BaseSize=0.7`,
  `Height=0.6`, textured with the tile's own lower ~2/3 (the dark cone/spike graphic; the top ~1/3
  is the flat dots+letter panel and isn't reused here). Also revised
  `GEDirectionalCubeTiles.cpp`'s icon 330-333 table entries and both
  `mobile-eggbert-reference/02-tiles.md` and `questionnaire-all-remaining-tiles.md` to record that
  this session's live "krychle + hrot" (cube + tip) description supersedes an earlier session's
  "Billboard" questionnaire answer for the same 4 icons. Verified live via a headless EasyGL
  screenshot at the sample world's teleporter room (temporary spawn-position debug override,
  reverted before committing) — renders correctly: blue cube sides with the real dots/emblem-letter
  texture, a distinct dark teal cone hanging cleanly below without clipping the floor. See
  `plan.md`'s `E3D-MIG-147` entry for full detail.
- **P3 — DONE (2026-07-11): expanded the bottom-right Blupi animation indicator.** Added `Air`
  (falling, pre-approved Simple3D-ported frame data) plus, after explicit user approval to
  transcribe the real `table_blupi` icon data, `StopEcrase`/`MarchEcrase`/`Balloon`/`Teleporting`
  — every real `BlupiAction` state this engine can actually reach today is now reflected. Every
  OTHER real `BlupiAction` (Turn, Glu, Electro, Win, Bye, Clear1-8, the vehicle/swim/skateboard/
  tank/helicopter modes, etc.) is not reachable at all — those mechanics don't exist in
  `GalaxyEggbertCNA` yet, so their animations are out of scope regardless of data-transcription
  approval; revisit if/when any of those mechanics get implemented. See §3's 2 newest entries and
  `plan.md`'s `E3D-MIG-064` entry for full detail.

**With the whole 2026-07-11 feedback batch above done, resumed Phase 14's own remaining mechanics
(not part of that batch, picked next per §4):**

- **DONE (2026-07-11): fan hazard (`E3D-MIG-149`).** See §3's newest entry and `plan.md`'s
  `E3D-MIG-149` entry for full detail — includes a real, deeper `GroundHeightAt()` collision
  limitation found and tracked (not fixed) in §5, discovered while live-verifying this task.
- **Remaining: water breath gauge (`E3D-MIG-148`)** — Phase 14's last mechanic, a genuinely new
  Surf/Nage swimming movement mode rather than a hazard-timer variant, likely needs more scoping
  than a single-task cycle.
- **DONE (2026-07-11): teleporter tip shape + real alpha-transparency bug fixed**, found via a live
  user re-check of the render geometry above. See §3's newest entry and `plan.md`'s `E3D-MIG-147`
  entry for full detail — the tip is now a straight-sided box (not a tapering pyramid), and the
  whole teleporter icon (cube + tip) is now genuinely alpha-blended, since the "black" the user
  saw was real alpha=0 transparency in the source texture being rendered opaque, not painted
  black. `PyramidTipItem`/`AppendPyramidTipMesh()` removed entirely from `../easy-3d` (unused after
  the box replacement).

**3 tasks queued by the user (2026-07-11), 1 done + 1 paused + 1 not started:**

1. **Teleporter tip: DONE, confirmed no further changes needed for now.** See §3's entries and
   `plan.md`'s `E3D-MIG-147` entry for full detail — reverted from a non-tapering box back to a
   genuine pyramid (the box's 4 flat faces didn't share a vertex, so their triangle-shaped alpha
   cutouts visibly failed to connect at the block's edges). User's exact words on this (2026-07-11,
   round 4): "ohledne tepeporteru to je beze zmeny priste ty ohledne teleporteru napisu jak si to
   predstavuji" (regarding the teleporter, no change for now — next time I'll write up how I
   picture it) — i.e., don't touch the teleporter tip again until the user provides a fresh,
   detailed spec for it. Treat it as settled, not as "still broken."
2. **Saw blade: STILL WRONG after 3 rounds of fixes — do NOT attempt another blind fix, see the
   detailed analysis below before touching this again.** User (Czech, round 4, verbatim, with
   `Screenshot From 2026-07-11 17-56-53.png`): "synu zase jsi to zkurvil pila je spatne ... ty to
   nevidis rovna strana pily musi byt u zeme a kolo se zuby nahoru" — "you screwed it up again,
   the saw is wrong ... you don't see it? the FLAT/STRAIGHT side of the saw must be at the ground,
   and the WHEEL with teeth must be UP." The user explicitly asked to stop and document rather
   than guess again live, so this task is paused pending either a clearer spec from the user or a
   more careful investigation before the next attempt.
   - **What's been tried so far** (all confirmed insufficient by live user feedback):
     round 1 — added Saw/SawStopped to the `InnerFlatPlate` table (fixed "rendered as a plain
     cube" — this part IS correct and unchanged since). Round 2 — shrank the plate
     (`kSawPlateHeight=0.5`) and bottom-anchored it to the block's own `-0.5` face — user said it
     looked buried ("cuts into the ground"). Round 3 — re-anchored to the block's TOP face
     (`+0.5`, extending down) — user says it's STILL wrong, now specifically calling out
     ORIENTATION (flat side vs. toothed wheel), not just vertical position.
   - **New finding from this session, not yet acted on**: direct alpha-channel inspection of the
     real icon 378 crop (`../mobile-eggbert/Content/icons/object-m.png`, 64×64 tile) shows the
     non-transparent content's bounding box is `x:[3,59] y:[32,63]` — i.e., the actual
     gear/blade graphic occupies ONLY THE BOTTOM HALF of the tile (`y>=32`); the top half is
     fully transparent. The current code passes the WHOLE tile's UV uncropped
     (`item.Uv = tileUv` in the generic `InnerFlatPlate` branch, no Saw-specific crop) — meaning
     half the mapped texture space is empty.
   - **A second, unverified but plausible theory**: `Easy3D::AppendPlateMesh`'s corner/UV mapping
     (`corners[0]`/`[1]` = world BOTTOM, mapped to `V0`; `corners[2]`/`[3]` = world TOP, mapped to
     `V1`) combined with this atlas's own convention (`V0` = the source image's TOP row, `V1` =
     its BOTTOM row, confirmed elsewhere this session for the teleporter tip's own UV cropping)
     means a `PlateItem` always renders its source image VERTICALLY FLIPPED: world-bottom shows
     the image's own top, world-top shows the image's own bottom. Combined with the bbox finding
     above (real content is in the image's bottom half, i.e. high V), this would put the actual
     gear content at the plate's WORLD-TOP (matching where it's currently appearing, near/above
     floor level) — but potentially mirrored vertically from its natural orientation, which could
     explain "flat side up, teeth down" instead of the requested "flat side down, teeth up" if the
     source graphic itself has an identifiable flat edge on one side of the gear (not conclusively
     confirmed by the bbox alone — the crop visually reads as a gear/cog with teeth on the visible
     arc; no obviously flat edge was identified by eye, but wasn't ruled out either — see the saved
     crop for direct re-inspection: `icon378_onred.png` was generated this session, composited on
     red to show the alpha boundary clearly, but not saved outside the scratchpad — regenerate via
     the same script if needed, see this entry's own repo history / ask the user to describe the
     crop again if unclear).
   - **Also observed but not diagnosed**: the live screenshot shows the Saw's EXHIBITION specimen
     (not the real switch+saw gameplay pair) overlapping/positioned oddly relative to a
     neighboring exhibit (a "6" trophy) — possibly just an unfortunate camera angle, possibly a
     sign the blade is now sitting too high given the exhibition's own per-icon spacing/pedestal
     assumptions (which differ from the real gameplay placement's floor-embedded context). Worth
     checking BOTH the exhibition specimen and the real gameplay placement when re-attempting this
     fix, not just one.
   - **Recommended next steps**: (1) re-inspect the real icon 378 crop very carefully for a
     genuinely flat/straight edge (re-run the red-composite script, look closely, maybe ask the
     user to point it out precisely if still ambiguous); (2) determine definitively whether
     `AppendPlateMesh` really does vertically flip its source image (write a tiny standalone
     check rendering a asymmetric test texture through it, or reason through `../easy-3d`'s
     `AppendFace`/`ComputeFaceCorners` conventions directly); (3) if confirmed, either crop+flip
     the Saw's own UV rect (a per-icon fix, like `TeleporterTipUv()`) or reconsider whether ANY
     other `InnerFlatPlate` icon is silently also flipped but just never noticed (65 icons use
     this same path — Saw is the first with a strongly asymmetric, orientation-sensitive graphic);
     (4) get a live screenshot BEFORE claiming done this time, and compare the visible teeth
     direction against the user's literal description before considering this resolved.
3. **DONE (2026-07-12): platform lift clipping fixed + 2 more lifts added.** See §3's newest entry
   for the full root-cause/fix writeup (carved shaft opening + flush-tuned `posEndY`, 2 new lift
   rooms). All 63/63 unit tests + all 5 verify tools + both backends re-verified live.

Older, lower-priority polish tasks (unaffected by the above, still valid, just less urgent now):

0. **Follow-up: replace `avatars3d/blupi_placeholder/` with a real Blupi model, once one exists**
   (§3, 2026-07-09). Not urgent — the placeholder proves the third-person camera mode/`AvatarRenderer`
   wiring works end-to-end, but a fox is obviously not Blupi. When a real model exists (exported as
   glTF/GLB, ideally with named clips matching `GEBlupiController::AnimState`'s 5 states so
   `BlupiAnimStateToPlaceholderClipName()`'s rough substitution can become a real 1:1 mapping):
   convert via `../cna/tools/avatar_asset_pipeline/convert_avatar.py --embedded-clips` (or
   `--body`/`--clip` if clips are separate files), replace `avatars3d/blupi_placeholder/` wholesale,
   and re-verify/tune `kPlaceholderModelScale` and the yaw rotation offset in
   `GalaxyEggbertCnaGame::Draw()` against the real model's actual proportions and rest-pose facing
   direction (both were hand-picked/unverified for the fox placeholder, see §3).
1. **Optional: investigate the residual seam transparency left after the 2026-07-09 UV-inset fix**
   (§3/§5 — 60% reduction in fully-transparent seam pixels, not 100%). Two untested hypotheses: (a)
   ordinary MSAA/silhouette-edge antialiasing producing genuine partial pixel coverage at any
   triangle edge, independent of texture UVs — likely benign and possibly not worth chasing further;
   (b) the half-texel inset (`BlockTypes::tileUV()`) isn't quite enough at the most extreme grazing
   angles/highest mip levels. Low priority — the fix already shipped a large, measured improvement;
   this is about closing the remaining gap, not an open regression. **Verification:** repeat the
   pixel-level before/after methodology from §3 (raw pixel sampling + alpha<1 count across a full
   screenshot) at the same close/oblique staircase angle.
2. **Optional: extend face culling (§3, 2026-07-09) to the animated/water paths** in
   `RebuildAnimatedRenderer` — currently every animated/water block emits all 6 faces
   unconditionally regardless of neighbors. Low priority: these are typically sparse decorative
   elements (fans, lava pockets, water pools), not bulk fills, so the payoff is much smaller than
   the static-path win already banked, and correctly distinguishing "definitely a full cube this
   frame" from "uses holed geometry this frame" per animated icon adds real complexity.

Also done (2026-07-09): all 6 remaining `DirectionalCube` icons (15-18, 108-109, §3) backfilled —
`DirectionalCube`/`InnerPillarBox`/`InnerFlatPlate`/`TripleCrossBillboard` are now ALL fully
wired up, no confirmed icon across any of the 4 render modes remains unimplemented;
`GEInnerFlatPlateTiles` axis spot-check found and fixed icons 368-372; `BigDecor:` billboard
rendering, platform-lift/crate `UniformCube` objects, `GalaxyEggbertWorldsTests` re-verification,
the clean-exit-path investigation, 3D-format MoveObject storage, populating the sample world with
all 67 remaining confirmed `ObjectType`s, face culling for the static terrain path (67788→23012
vertices, a real 66% reduction), and the seam-line artifact (root-caused, 60% reduction in
fully-transparent seam pixels) are all complete.

## 9. Do not do yet

- **No building, fixing, or troubleshooting `GalaxyEggbertSimple3D`/U3D/Nova3D at all (updated
  2026-07-08, per user)** — it is a historical reference only now, not a maintained target. Its
  build is currently broken in this environment (missing/incompatible U3D prebuilt) and that is
  not to be fixed. Source may still be read for reference (e.g. porting `BigDecor` parsing logic to
  CNA), but do not configure/build/run it or spend effort on its build environment.
- No deleting/removing any `GalaxyEggbertSimple3D` code without an explicit removal task.
- No modifications to `../mobile-eggbert`, `../cna`, or `../simple-3d` without explicit user
  approval for that specific change. `../easy-3d` has standing permission (still scoped to small,
  generic 3D-batching helpers). **Concrete case (2026-07-09, §5): the clean-exit-path exit-code-1
  bug is a confirmed `../cna`/SDL issue — user was asked and declined to approve a fix, so it stays
  a documented known limitation. Do not attempt to fix it without new, explicit approval.**
- No `.txt → .vwr` (or any) automated 2D-to-3D world converter — rejected.
- No Easy3D scope creep — no ECS, scene graph, physics, resource cache, editor, or Lua.
- No mass refactor of `include/GalaxyEggbert/Worlds/` unless a failing unit test justifies it.
- No `#ifdef GE_ENGINE_*` anywhere.
- No new gameplay mechanics not present in mobile-eggbert.
- **No adding a `GEDirectionalCubeTiles.cpp`/`GEInnerPillarBoxTiles.cpp`/`GEInnerFlatPlateTiles.cpp`/
  `GETripleCrossBillboardTiles.cpp` entry speculatively, without re-checking the exact per-icon
  answer recorded in `questionnaire-all-remaining-tiles.md`/`questionnaire-unused-tiles.md`/
  `02-tiles.md` first** — that's the whole point of having done direct user Q&A instead of agent
  guessing. ALL confirmed icons across all 4 modes are now wired up (2026-07-09, §3) — this
  invariant still applies to any future new icon that gets confirmed.
- No re-running the full 97-icon or 280-icon questionnaires again — both are done, and so are the
  6 previously-open `DirectionalCube` icons (15-18, 108-109, backfilled with a default 2026-07-09,
  §3). Water's render mode is decided and implemented (2026-07-08, §3) — not open anymore.
- Commit after each finished task (standing instruction) — one commit per task, not batched.
  **Pushing** to `origin/develop` is now standing authorization too (updated 2026-07-11, per
  user: "pushni (vzdy automaticky)") — push to `origin/develop` automatically right after each
  commit, no need to ask each time.

## 10. Resume prompt

```
Read NEXT.md first. Then inspect only the files needed for the first task in section 8.
Do not refactor unrelated code and do not expand scope beyond that one task.
Make one small, verified improvement — implement the task goal as described, nothing more.
Cross-reference mobile-eggbert source at ../mobile-eggbert (read-only) before implementing any
gameplay or world-format behavior. If the task needs a design decision, make the smallest
reasonable choice and record it, rather than blocking on it, unless it's the kind of decision
that genuinely needs the user's direct judgment (e.g. reading a crop image) — ask first in that
case, but don't assume the user is unavailable: they may be reachable via a different client (the
Claude Android app, not just a PC) where inline `Read`-rendered images may not display, but
`SendUserFile`-delivered attachments do (confirmed 2026-07-09, see §3).
After the change, run the verification command listed for that task.
Update NEXT.md when done: move the completed task into section 3 (Recent changes), remove it
from section 8, and add whatever new next-smallest task naturally follows.
```
