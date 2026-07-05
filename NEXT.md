# NEXT.md — Galaxy Eggbert

## 1. Project summary

**Galaxy Eggbert** is a faithful 3D remake of **mobile-eggbert** (a C++ port of *Speedy Blupi*, a
Windows Phone XNA game from 2013). Main goal: reimplement the same game logic, levels, and assets
in 3D — perspective camera, billboard sprites, 3D-rendered tiles — without inventing new mechanics
(see `CLAUDE.md`'s "Faithful Remake" rule).

**Current development phase:** mid-migration between two build targets:

- `GalaxyEggbertSimple3D` (built on `simple-3d` → U3D/Urho3D) — the **only playable** target,
  feature-complete enough to play through core mechanics end-to-end (see §2).
- `GalaxyEggbertCNA` (built directly on **CNA** + **Easy3D** helper library) — the **new
  long-term target**, opt-in and pre-parity. Opens a window, loads a genuinely 3D hand-authored
  `.vwr` world (`worlds3d/world001.vwr`), renders real textured/animated terrain (one cube per
  non-air cell), moves an invisible collision-only Blupi with tank controls, and renders parsed
  `MoveObject`s (pickups/enemies) as real textured billboards. No 3D Blupi model, no HUD, no sound,
  no gameplay logic yet.

**Important architectural decisions:**

- **Direct CNA + Easy3D is the sole long-term target (locked 2026-07-05).** Galaxy Eggbert will
  run only on CNA long-term. `GalaxyEggbertSimple3D` is transitional — scheduled for **gradual
  removal** as `GalaxyEggbertCNA` reaches parity, piece by piece, not kept indefinitely as
  reference. It remains the only playable target today and stays intact (no removal without an
  explicit task).
- **No Blupi 3D model exists yet; the user will provide one later.** Interim stopgap in
  `GalaxyEggbertCNA`: a first-person camera (no third-person view is possible without a visible
  character) plus a small 2D animation-state indicator (idle/walk/jump/crouch/look-up) in the
  screen's bottom-right corner, reusing existing 2D sprite frames. Not a design decision about the
  eventual 3D Blupi's look.
- Easy3D is a small helper library beside CNA (cameras, texture atlas, batching, mesh building,
  renderer adapters) — must not hide CNA; game code may call CNA directly. **User granted standing
  permission to modify `../easy-3d` (2026-07-06)** — no longer needs per-change approval, but stays
  scoped to small, generic 3D-batching helpers (no scene graph/ECS/engine creep).
- `../mobile-eggbert` is **read-only, never modified even temporarily** (clarified 2026-07-05 — if
  a task needs a working copy, copy the file into galaxy-eggbert first). Its assets (PNGs, sounds,
  world files) are freely reused by direct path/build-time copy; its code/data (`Decor.cpp`,
  tables, enum values, byte layouts) is reference-only and requires explicit approval to copy —
  see `CLAUDE.md`'s reuse table for the current exceptions (a prose gameplay-behavior spec was
  approved 2026-07-05, scoped to `mobile-eggbert-reference/`, not to game code). It has no CMake
  library target (only `add_executable`), so it cannot be linked as a dependency today.
- No automated `.txt → .vwr` 2D-to-3D world converter (rejected direction). `GalaxyEggbertCNA`
  defaults to a hand-authored `.vwr` world; `LoadFromMobileEggbertFile` is a secondary/reference
  path only.
- **Terrain-tile render mode is an active, unresolved investigation, not a settled design.** A
  2026-07-06 finding showed `UniformCube` (textured on all 6 faces) is wrong for roughly 100–140 of
  the 314 named tiles — some need `Billboard`, some a new `ThinMechanical` mode (geometry
  undecided), some a new `DirectionalCube` mode (per-face texture + fallback color/transparency,
  confirmed needed 2026-07-07), at least one a wholly new "thin-bar" geometry. See §3/§4 and
  `mobile-eggbert-reference/15-3d-render-mapping-design.md` §10–§11 for the full, still-growing
  detail. **Nothing from this investigation has been implemented in the renderer yet** — it is
  documentation/identification work in progress.

## 2. Current status

### Build status
- `GalaxyEggbertSimple3D` — last confirmed clean build+run this session on 2026-07-03/04. No
  source changes to this target since (all work since has been CNA/Easy3D/docs). **Caveat:** U3D's
  own prebuilt directory (`/rv/data/library/github.com/u3d-community/U3D/`, a separate dependency,
  not part of this repo) had no prebuilt `cmake-build-debug` in this environment as of 2026-07-04 —
  rebuilding it from scratch was explicitly deferred by the user. Confirm it exists before trusting
  a fresh Simple3D build.
- `GalaxyEggbertCNA` — last confirmed clean build+run 2026-07-06 (billboard rendering added that
  day). No source changes since (only docs). Binary exists at `build-cna/GalaxyEggbertCNA`
  (2026-07-05 timestamp) and no source file under `src/GalaxyEggbertCNA/` is newer than it.
- `GalaxyEggbertWorldsTests` — **54/54 pass**, reconfirmed today (2026-07-07) via
  `ctest --test-dir build` and by running `./build/GalaxyEggbertWorldsTests` directly.
- **New observation today (2026-07-07), not yet investigated:** both `cmake-build-debug/` and
  `build-cna/` are missing `CMakeFiles/rules.ninja` — a fresh `cmake --build` in either currently
  fails immediately (`ninja: error: ... loading 'CMakeFiles/rules.ninja': No such file or
  directory`) instead of doing an incremental rebuild. The existing binaries in both directories
  predate this and are not stale relative to source (no `.cpp`/`.hpp` changed this session since
  they were built), so this doesn't currently block anything — but the **next** source change in
  either target will need a fresh `cmake -S . -B <dir> ...` reconfigure first, not a bare
  `cmake --build`. Cause unconfirmed (not investigated further — out of scope for a docs-only
  session). Same symptom in both directories suggests an environment/disk issue, not a per-target
  code problem.
- `../easy-3d` — last confirmed 2026-07-06 (when `BillboardMesh`/`BillboardMeshRenderer` were
  added): headers-only build 2/2 tests pass, CNA-linked build 6/6 tests pass. Not re-verified today.
- `ctest` does not discover `GalaxyEggbertWorldsTests` in the `cmake-build-debug` profile specifically
  (`ctest --test-dir cmake-build-debug` reports `GalaxyEggbertWorldsTests_NOT_BUILT`) — reconfirmed
  today, still open, see §5/§8.

### What works
- **`GalaxyEggbertSimple3D`**: world loading from mobile-eggbert `.txt`, textured/animated 3D
  terrain, Blupi billboard with state machine, mobile-object billboards (all real `ObjectType`s,
  including 12 previously-dropped ones fixed 2026-07-03), crate push, platform patrol, hazard
  detection, enemy stomp, respawn invincibility, pickups, exit-gate logic, HUD, save/load (3
  slots), 93-channel sound, 3rd-person orbit camera, 5 sky colors per region, real camera shake on
  death/hazard. Most of the 12 newly-spawning `ObjectType`s render but have no gameplay behavior
  yet on pickup/contact (7 partial-support types, tracked in `09-open-questions.md`).
- **`GalaxyEggbertCNA`**: loads `worlds3d/world001.vwr` (2729 non-air blocks, Y range [0,13]) by
  default via `GEWorldRuntime::LoadFromVwrFile`; `LoadFromMobileEggbertFile` for mobile-eggbert
  `.txt` exists as a secondary path. Renders every Y layer as textured cubes
  (`GETerrainRenderer` + `Easy3D::CubeMeshRenderer`), animated tiles included (lava/crusher/saw/
  spike/water×2/fan×4/marine/temp, 6 fps, CPU-rebuilt on phase change). Tank controls (Left/Right
  turn, Up/Down move along facing; Left Ctrl jumps; Space reserved for mobile-eggbert's Action key,
  read but unwired; LShift crouch, RShift look-up). First-person camera + 2D animation-state HUD
  indicator (no 3D Blupi model exists). Parses `MoveObject:` records into engine-agnostic
  `MobileObjSpec`s and renders every one as a real camera-facing textured billboard
  (`Easy3D::BillboardMesh`/`BillboardMeshRenderer` + `GEObjectIcons`), live-verified on a real
  mobile-eggbert level (67 objects). Known gap: uses `element.png` for every `ObjectType` (same
  `DOC-007` gap as Simple3D — types 1/12/32/33 actually need different sheets, not fixed here).
  Also parses (but does not render) `BigDecor:`.
- **`Easy3D` (`../easy-3d`)**: `CubeMesh`/`CubeMeshRenderer` (terrain), `BillboardMesh`/
  `BillboardMeshRenderer` (objects, added 2026-07-06, mirrors the cube path).
- **First hand-authored 3D world**: `worlds3d/world001.vwr` via `tools/GenerateSampleWorld3D.cpp` —
  ground floor, staircase, raised platform, walled room, two pillars.
- **Tile/object documentation**: `mobile-eggbert-reference/` — complete catalogs of all 441 tile
  icons, 204 `ObjectType`s, 93 sounds, 131 animation sequences, all backgrounds, plus a prose
  gameplay-behavior spec (movement/physics, hazards, pickups, doors/keys, crates/lifts/bridges,
  save/progression) — see §3 for the currently-active thread (terrain render-mode identification).

### What does not work yet
- `GalaxyEggbertCNA`: no visible 3D Blupi (invisible collision point + first-person camera + 2D
  indicator stopgap, see §1). No `BigDecor` rendering (parsed only). No platform-lift/crate
  `UniformCube` object path. No HUD, no sound, no real gameplay logic.
- Terrain-tile render modes: **`UniformCube` is confirmed wrong for a large fraction of the 314
  named tiles** (§1/§3) — labeled in `02-tiles.md` but **not implemented**; the renderer still
  draws every terrain block as `UniformCube` today, including all flagged icons.
- `GalaxyEggbertSimple3D`: no per-zone fog; Android/Web builds untested since the last engine
  change.

## 3. Recent changes

Most recent first. `develop` is currently **in sync with `origin/develop`** (both at `caea94a`) —
pushed 2026-07-07. Full history: `git log`; older milestones below are compressed — see `plan.md`
§15/§16 and `mobile-eggbert-reference/` for full detail on anything summarized in one line here.

- **Tile render-mode Q&A, round 1 — 34 icons resolved (2026-07-07).** Instead of another agent
  guess, asked the user directly (`questionnaire-unidentified-tiles.md`, one block per icon with
  its crop image) what each of `15-3d-render-mapping-design.md` §10.6's 34 unidentified icons is
  and how to render it. Result (full table in that doc's new §11): 15 icons need a new
  `DirectionalCube` mode (per-face texture + fallback color/transparency) — reverses an earlier
  "nobody needs this" call; 3 of those (cobwebs, 401-403) need real per-block facing metadata, the
  first concrete use for bits reserved since §3 but never used. 4 icons were brick, not wood
  (stays `Billboard`, identity fixed). 11 icons turned out to be plain bulk material after all
  (back to `UniformCube`). Icon 202 needs a wholly new "thin-bar" geometry (a walkable rod, not
  cube/billboard/`DirectionalCube`). Icon 200 (`Platform`) confirmed likely misnamed. Applied to
  `02-tiles.md`; **round 2 questionnaire created same day** (`questionnaire-all-remaining-tiles.md`)
  covering all other ~280 named tiles at the user's chosen scope (everything, not just the
  ~137 already flagged) — **not yet answered**.
- **Large-scale terrain render-mode finding (2026-07-06).** Triggered by the user noticing the
  circular saw tile (`Saw`, icon 378) rendering as a `UniformCube` looked wrong (a thin blade
  tiled on all 6 faces). An 8-agent crop-inspection pass over all 314 named tiles found ~100 need
  something other than `UniformCube`: ~50 vertical post/pillar/marker-style icons → `Billboard`
  (doors, teleporters, `SecretPower` pedestals, signs — reverses an earlier "closed door stays
  `UniformCube`" call, since `Door1-3` turn out to be pillar-shaped too); ~25 thin mechanical/hazard
  tiles (saw, spring, switches, fans, pipes, grates, bridge, ladder) → new provisional
  `ThinMechanical` mode, geometry undecided; water/marine tiles need a special surface treatment;
  6-7 icons (391-400) look like a modular archway kit; ~34 icons needed identification before any
  call (resolved in the Q&A round above). **First-pass, not independently adversarially verified**
  — treat specific icon identities as probable until spot-checked. Labeled (not implemented) in
  `02-tiles.md`; full detail in `15-3d-render-mapping-design.md` §10.
- **`GalaxyEggbertCNA` renders `MoveObject`s as billboards (2026-07-06).** New
  `Easy3D::BillboardMesh`/`BillboardMeshRenderer` + `GEObjectIcons` (icon lookup ported from
  `GEDecorSystem::GetObjIcon`); wired into `GalaxyEggbertCnaGame::Draw()`. Screenshot-verified on
  `world065.txt` (67 objects — sign, hazard, treasure chest all correct).
- **`GoldPillar` rename + a real sample-world bug fix (2026-07-06, user-caught).** Icon 183
  (`BlockTypes::Wall`, described as "brick wall") is actually a golden pillar — renamed
  `GoldPillar`. This uncovered `tools/GenerateSampleWorld3D.cpp` using that same golden-pillar
  texture for ordinary room walls — fixed to `StoneB` (a genuinely common tile); regenerated
  `worlds3d/world001.vwr` (same 2729 blocks/geometry, texture only). First instance of the pattern
  the large-scale finding above generalized.
- **Jump remapped to Left Ctrl; Space reserved for Action (2026-07-05).** Matches mobile-eggbert's
  real key layout; Action itself isn't implemented in CNA yet.
- **Standing `../easy-3d` permission granted (2026-07-05)** — see §1.
- **CNA tank controls + `MoveObject:` parsing (2026-07-05).** Controls felt wrong (arrows were an
  absolute-direction pad) — fixed to match Simple3D exactly (turn/move-along-facing, crouch,
  look-up). Ported Simple3D's working `MoveObject:` parser to CNA (previously fully ignored) —
  data prerequisite for the billboard rendering above.
- **5 more `09-open-questions.md` items resolved (2026-07-05)** — doors, `BigDecor` (confirmed
  non-colliding, recommend `Billboard`), hazard-animation phase (no change needed), backgrounds
  (keep flat sky-color, no real skybox), teleporter pairing (keep implicit scan-based). See
  `15-3d-render-mapping-design.md` §9.
- **CNA-only direction locked + interim Blupi camera/HUD (2026-07-05)** — see §1.
- **Gameplay-behavior specification, `DOC-300`-`DOC-306` (2026-07-05).** User-approved prose spec
  (not pseudocode, not code transcription) covering Blupi's core mechanics, save/progression,
  hazards, pickups, and crates/lifts/bridges/effects — 7 agents drafted, 7 independently verified
  against `Decor.cpp`/`GameData.cpp`, found and fixed ~20 real errors (notably: bridge tiles are
  NOT purely cosmetic during construction — lose floor support for 136/157 build ticks; crate
  stacks do link vertically in mobile-eggbert, an open question for whether galaxy-eggbert's port
  does too). New files: `10-blupi-mechanics.md`, `11-save-and-progression.md`,
  `12-hazards-and-interactables.md`, `13-object-pickups.md`,
  `14-crates-lifts-bridges-effects.md`; extended `06-doors.md`/`04-enemy-behavior.md`.
- **Repo-wide documentation staleness sweep + `CLAUDE.md`/independent review pass (2026-07-05,
  ~15 commits).** Audited every `.md` file in the repo (not just `mobile-eggbert-reference/`);
  fixed `CLAUDE.md` (was still describing `GalaxyEggbertCNA` as nonexistent), deleted 3 dead docs
  describing a migration that no longer matches the codebase, fixed stale claims in `WINDOWS.md`/
  `README.md`/`ANDROID.md`/`World Format.md`/`CMakeLists.txt` comments. Separately, 6 agents
  independently re-checked all 10 `mobile-eggbert-reference/*.md` files against source and found +
  fixed 13 real errors (wrong sound-trigger claims, a mislabeled animation frame table, `Water1`/
  `Water2` frame mixup, `SawStopped` miscategorized as unused, animated-sequence count mismatch).
- **Documentation rework, `DOC-100`-`DOC-267` (~168 commits, 2026-07-03/04) — complete.** Full
  `mobile-eggbert-reference/` catalog build-out: all 441 tile icons, 204 `ObjectType`s, 93 sounds,
  131 animation sequences, all backgrounds. Found and fixed 2 real engine bugs along the way:
  `S3D-2` (`BlockTypes::tileUV()` ignored a real 1px inter-tile gap in `object-m.png`, bleeding
  neighboring icons) and `S3D-4` (`ObjectType47`/Chenille used the wrong sprite sheet). Also fixed
  a GIF-ghosting bug in the doc-generation tooling (129 GIFs regenerated). Full detail: `plan.md`
  §16.
- **Engine work predating the doc rework (commit `4cda53d`, 2026-07-03):** CNA gained an invisible
  collision-only Blupi (grid collision, step-up, gravity), a `BigDecor:` parsing fix (was silently
  dropped by both targets due to a shared row-counter bug), a hand-authored `.vwr` world pipeline
  (`tools/GenerateSampleWorld3D.cpp`) replacing mobile-eggbert `.txt` as CNA's default world
  source, and animated-tile rendering. Simple3D's `MoveObject` allowlist was expanded to spawn 12
  previously-dropped `ObjectType`s.
- **Earlier (pushed before this session's local batch):** real textured CNA terrain rendering
  (`GETerrainRenderer`, `GETileAtlas`, `Easy3D::CubeMesh`/`CubeMeshRenderer`), rejection of an
  auto 2D→3D world converter, initial mobile-eggbert asset/world-file integration, the
  `GalaxyEggbertCNA` skeleton target, Simple3D crate-push/platform-patrol fixes.

## 4. Current blocker / main problem

**No code blocker.** Both engine targets build and run as described in §2 as of their last
confirmed build (no source changes since). The one build-environment oddity found today (missing
`rules.ninja` in both `cmake-build-debug/` and `build-cna/`, §2) doesn't block anything right now
since no source has changed, but will need a reconfigure before the *next* code change in either
target builds cleanly.

**The active thread is documentation, not code:** terrain-tile render-mode identification
(§1/§3). A round-2 questionnaire (`mobile-eggbert-reference/questionnaire-all-remaining-tiles.md`,
~280 icons) is waiting on the user's answers. Nothing about `ThinMechanical`'s geometry, the
water-surface treatment, or the new `DirectionalCube`/"thin-bar" modes found in round 1 has been
implemented in the renderer yet — that implementation work is explicitly waiting on this
identification pass finishing first (see §8 task 1).

## 5. Known bugs and limitations

| Status | Issue |
|---|---|
| confirmed, environment-specific | `ctest` does not discover `GalaxyEggbertWorldsTests` in the `cmake-build-debug` profile (`GalaxyEggbertWorldsTests_NOT_BUILT`); works fine in a fresh `build/` dir (54/54 via `ctest --test-dir build`, reconfirmed 2026-07-07). |
| confirmed, unconfirmed cause (new, 2026-07-07) | Both `cmake-build-debug/CMakeFiles/` and `build-cna/CMakeFiles/` are missing `rules.ninja` — a fresh `cmake --build` fails immediately in either until reconfigured with `cmake -S . -B <dir>`. Existing binaries in both predate this and are not stale (no source changed since). Not investigated further this session. |
| incomplete | `GalaxyEggbertCNA`: no Blupi/object-behavior rendering beyond billboards, no HUD, no sound, no gameplay logic (expected at this phase). No `BigDecor` rendering (parsed only). No platform-lift/crate `UniformCube` object path. |
| incomplete, actively being resolved | Terrain-tile render modes: `UniformCube` wrong for ~100-140 of 314 named tiles (§1/§3/§4) — identification in progress via user Q&A, not yet implemented. |
| incomplete | `element.png` used for every `ObjectType` billboard, even though real mobile-eggbert data shows types 1/12 need `object-m.png` and 32/33 need `blupi1.png` (`DOC-007`, same gap in both targets). |
| incomplete | Simple3D: no per-zone fog, only `SetClearColor` per sky region. |
| incomplete | 7 `ObjectType`s (jeep/secret-exit/skateboard/suction-cup/mirror/balloon/dynamite) spawn with correct icons in Simple3D but have no real gameplay behavior on pickup/contact. |
| unknown | Simple3D Android/Web builds untested since the last engine change. |
| unknown | `GalaxyEggbertCNA`'s clean-exit-on-window-close path not separately exercised (only a forced kill was tested). |
| needs verification | Simple3D: stomp bounce height (`kJumpSpeed * 0.65f`) vs. mobile-eggbert's real feel. |
| needs verification | Simple3D: crate push floor-support check only tested at y=0; mobile-eggbert genuinely links crate stacks vertically (`SearchLinkCaisse`) — whether galaxy-eggbert's port does the same is unconfirmed. |
| risky assumption | `GalaxyEggbertCNA`'s world/texture loader uses relative paths (`"worlds3d/world001.vwr"`, `"Content/icons/object-m.png"`) — only works when run from its own build directory. |
| incomplete | `GETerrainRenderer` (CNA) has no face-culling/occlusion — fine at ~2700 blocks, will need revisiting for denser worlds. |
| build-unverified | Simple3D's `S3D-4` fix (`ObjectType47`/Chenille sprite-sheet bug) was verified by static review only — U3D's prebuilt directory was missing in this environment, so it has never been confirmed by an actual compile. |

## 6. Architecture notes

### Main modules
```
include/GalaxyEggbert/Worlds/, src/GalaxyEggbert/Worlds/   — engine-agnostic voxel World (100×100
                                                               grid), Block/Chunk, .vwr save format.
                                                               Shared by BOTH targets
                                                               (GE_SHARED_SOURCES in CMakeLists.txt).
include/GalaxyEggbert/BlockTypes.hpp                        — tile type constants; block type =
                                                               icon index in object-m.png;
                                                               fromMobileIconId() is the single
                                                               shared mobile-eggbert-icon→block
                                                               translation used by BOTH targets.

src/GalaxyEggbertSimple3D/   — full playable game (Simple3D/U3D). GalaxyEggbertSimpleGame owns
                                GEWorldRuntime, GETerrainRenderer, GEBlupiController,
                                GEDecorSystem, GEHud, GECameraRig, GESound, GEExploSystem,
                                GEBridgeSystem. Not touched by the CNA migration.

src/GalaxyEggbertCNA/        — GalaxyEggbertCnaGame (Microsoft::Xna::Framework::Game subclass)
                                owns Game/GEWorldRuntime (LoadFromVwrFile() default,
                                LoadFromMobileEggbertFile() secondary path; both now parse
                                MoveObject: into MobileObjSpec), Game/GETileAtlas (block type →
                                UV rect), Game/GETerrainRenderer (World, all Y layers → static +
                                animated CubeMeshRenderer-backed meshes), Game/GEBlupiController
                                (tank-control physics + coarse animation state), Game/GEObjectIcons
                                (ObjectType → element.png icon/UV lookup), an Easy3D::Camera3D
                                (first-person), a CNA Texture2D each of object-m.png/element.png,
                                and BasicEffects bound to them.

tools/GenerateSampleWorld3D.cpp — engine-agnostic CLI tool: builds a hand-authored 3D World in
                                memory and saves it via World::saveToFile(). No engine deps.
tools/VerifyBlupiMovement.cpp, VerifyMoveObjectTypesCna.cpp — engine-agnostic scripted checks
                                against real/sample world files (no CNA link needed).

worlds3d/                    — galaxy-eggbert's own hand-authored .vwr worlds (committed, unlike
                                mobile-eggbert's worlds/ which is copied at build time).
                                world001.vwr is the first one.

../easy-3d/                  — companion library beside CNA. Standing permission to modify
                                (granted 2026-07-06). Camera3D/OrbitCamera/FollowCamera,
                                TextureAtlas, CubeBatch/BillboardBatch/DebugDraw (CPU item queues),
                                CubeMesh/CubeMeshRenderer (terrain), BillboardMesh/
                                BillboardMeshRenderer (objects, added 2026-07-06, mirrors the cube
                                path). DebugDraw's own mesh/renderer: not started.
```

### Data flow
```
Hand-authored 3D worlds (worlds3d/*.vwr, engine-agnostic binary format):
  CNA (default): → GEWorldRuntime::LoadFromVwrFile() → World::loadFromFile() → World (all Y
                   layers) + MobileObjSpec list → GETerrainRenderer (terrain cubes) +
                   GEObjectIcons/BillboardMesh (object billboards) + Texture2D(object-m.png,
                   element.png) → real 3D textured/animated terrain + billboard objects; no 3D
                   Blupi model yet.

Mobile-eggbert worlds (worlds/worldXXX.txt, header + Decor: grid [+ MoveObject: lines], flat Y=0):
  Simple3D:      → GEWorldRuntime::LoadFromMobileEggbertFile() → World + MobileObjSpec list +
                   blupiSpawn → GETerrainRenderer / GEDecorSystem / GEBlupiController (renders
                   everything; this is Simple3D's only/default world source).
  CNA (secondary/reference path, not the default): → GEWorldRuntime::LoadFromMobileEggbertFile()
                   → same World + MobileObjSpec parsing as the .vwr path above → same
                   GETerrainRenderer/billboard rendering path.
```

### Important invariants
- Block type = icon index in `object-m.png` (20 cols, 64×64 px tiles) — no separate mapping table.
- World grid is 100×100; both `GEWorldRuntime` implementations independently define a
  `kWCX`/`kWCZ` (Simple3D) or `kWorldCenterX`/`kWorldCenterZ` (CNA) = 50 offset to center the grid.
- `ObjectType` (204 IDs) and `SoundChannel` (93 IDs) in `include/GalaxyEggbert/def/` are confirmed
  numerically identical to mobile-eggbert's versions — do not renumber.
- No `#ifdef GE_ENGINE_*` anywhere — engine differences belong inside Simple3D (for that target)
  or are simply separate code in `src/GalaxyEggbertCNA/` (for the new target).
- `CubeMesh`'s frame index / UV assignment convention (`AddGrid`'s row-major numbering ==
  `BlockTypes`' `icon = row*kSheetCols+col`) is load-bearing for `GETileAtlas` — do not change one
  side without the other.
- Build with `-j2` maximum (32 GB RAM constraint; crashes observed with more parallel jobs).

### Boundaries that must remain stable
- `BlockTypes::fromMobileIconId()` — must match the mobile-eggbert world file format exactly.
- `ObjectType`/`SoundChannel` numeric values — stored in level files, must never be renumbered.
- `GEDecorSystem::GetObjIcon(ObjectType, phase)` (Simple3D) / `GEObjectIcons::GetObjIcon` (CNA) —
  do not change without cross-referencing mobile-eggbert's `Decor.cpp`.
- `src/GalaxyEggbertSimple3D/` must not be mutated into the CNA implementation — new CNA/Easy3D
  code goes in `src/GalaxyEggbertCNA/` only.
- mobile-eggbert stays read-only, never modified even temporarily; no code/data copied from it
  without explicit user approval.
- `../cna` and `../simple-3d` are sibling repos: read freely when needed, modify only with
  explicit, per-change user approval. `../easy-3d` has **standing** permission to modify (granted
  2026-07-06) — still keep changes within its stated scope (small, generic 3D-batching helpers).
- Easy3D must not hide CNA (its APIs use CNA/XNA types directly) and must not grow into a scene
  graph / ECS / engine.

## 7. Useful commands

```bash
# Configure + build Simple3D (default target):
cmake -S . -B build
cmake --build build --target GalaxyEggbertSimple3D -j2
./build/GalaxyEggbertSimple3D

# Build + run world-model unit tests:
cmake --build build --target GalaxyEggbertWorldsTests -j2
ctest --test-dir build --output-on-failure          # 54/54 expected

# Configure + build the CNA target (opt-in, off by default):
cmake -S . -B build-cna -DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF
cmake --build build-cna --target GalaxyEggbertCNA -j2
cd build-cna && ./GalaxyEggbertCNA        # must run from its own build dir (relative asset paths)
# Expect on stdout: world load (2729 blocks, Y range [0,13]), GETileAtlas UV diagnostics, terrain
# texture load, terrain mesh upload (2729 blocks, 65496 vertices, 32748 triangles), a terrain
# on-screen visibility sample. A window opens showing textured terrain from an angled overhead view.

# Regenerate the hand-authored 3D sample world (if tools/GenerateSampleWorld3D.cpp changes):
cmake --build build-cna --target GenerateSampleWorld3D -j2
./build-cna/GenerateSampleWorld3D worlds3d/world001.vwr

# Scripted verification tools (engine-agnostic, no CNA link needed):
cmake --build build-cna --target VerifyBlupiMovement VerifyMoveObjectTypesCna -j2
./build-cna/VerifyBlupiMovement          # Blupi collision/step-up/gravity vs. worlds3d/world001.vwr
./build-cna/VerifyMoveObjectTypesCna     # MoveObject parsing vs. 12 real mobile-eggbert level files

# easy-3d: default (headers-only) build + tests
cmake -S ../easy-3d -B /tmp/e3d-build -DEASY3D_CNA_DIR=../cna
cmake --build /tmp/e3d-build -j2 && ctest --test-dir /tmp/e3d-build --output-on-failure

# easy-3d: CNA-linked build + tests (heavier — builds CNA/SHARP_RUNTIME/EasyGL too)
cmake -S ../easy-3d -B /tmp/e3d-build-cna -DEASY3D_LINK_CNA=ON -DEASY3D_CNA_BACKEND=EASY_GL -DEASY3D_CNA_DIR=../cna
cmake --build /tmp/e3d-build-cna -j2 && ctest --test-dir /tmp/e3d-build-cna --output-on-failure

# Reference: mobile-eggbert animation tables / gameplay logic (read-only)
grep -n "table_decor\|table_blupi" ../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Tables.cpp
less ../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Decor.cpp
```

No `.clang-format`/`.clang-tidy` config exists in this repo — no lint/format tooling to run.

## 8. Next smallest tasks

1. **Finish the terrain-tile render-mode identification.** Wait for/process the user's answers to
   `mobile-eggbert-reference/questionnaire-all-remaining-tiles.md` (~280 icons, same Q&A format as
   the completed 34-icon round) the same way round 1 was processed: update `02-tiles.md`'s
   category notes, fold new findings into `15-3d-render-mapping-design.md`. **Files:**
   `mobile-eggbert-reference/{02-tiles,15-3d-render-mapping-design,questionnaire-all-remaining-tiles}.md`.
   **Verification:** none code-side — this is a documentation task; spot-check a few processed
   rows against the user's actual answers.
2. **Only after task 1 settles, design + implement the render modes it converges on** — in
   particular the new `DirectionalCube` mode (per-face texture + fallback color/transparency,
   confirmed needed) needs an `Easy3D::CubeMesh`/`CubeItem` per-face extension (`../easy-3d`,
   standing permission already granted), `ThinMechanical`'s geometry is still undecided, and icon
   202's "thin-bar" is a wholly new shape. **Files:** likely `../easy-3d/include/Easy3D/CubeMesh.*`,
   `src/GalaxyEggbertCNA/Game/GETerrainRenderer.*`/`GETileAtlas.*`.
   **Verification:** new unit/compile-check tests mirroring the existing `CubeMesh`/`BillboardMesh`
   ones, plus a live screenshot showing at least one `DirectionalCube` tile rendering correctly.
3. **`BigDecor` billboard rendering for CNA** — `GEWorldRuntime` doesn't parse `BigDecor:` for CNA
   at all yet (Simple3D already does). Recommended render mode is `Billboard` (per
   `15-3d-render-mapping-design.md` §9.2). **Files:** `src/GalaxyEggbertCNA/Game/GEWorldRuntime.*`,
   `GalaxyEggbertCnaGame.cpp`. **Verification:** a tool mirroring `VerifyMoveObjectTypesCna.cpp`
   against a real level with known `BigDecor:` cells (e.g. `world013.txt`, 14 cells).
4. **Platform-lift/crate `UniformCube` object path for CNA** — the two approved exceptions to
   "objects are billboards" (`15-3d-render-mapping-design.md` §5) aren't implemented yet; reuse the
   existing terrain `CubeMesh`/`CubeMeshRenderer` machinery. **Files:**
   `src/GalaxyEggbertCNA/GalaxyEggbertCnaGame.cpp`.
5. **Investigate the missing `rules.ninja` in `cmake-build-debug/`/`build-cna/`** (§2/§5, found
   2026-07-07) before it silently blocks the next real code change. Likely fix: a fresh
   `cmake -S . -B <dir>` reconfigure; worth understanding *why* it went missing first (disk
   cleanup? interrupted reconfigure?) rather than just re-running configure blind.
   **Verification:** `cmake --build <dir> --target <any>` succeeds without a rules.ninja error.
6. **Fix `ctest` discovery in the `cmake-build-debug` profile** — investigate why
   `gtest_discover_tests` doesn't find `GalaxyEggbertWorldsTests` there (works fine in a fresh
   `build/` dir). **Files:** `CMakeLists.txt`, `cmake-build-debug/` config.
   **Verification:** `ctest --test-dir cmake-build-debug` reports 54 passed, not `_NOT_BUILT`.
7. **Chunk-radius world streaming (`E3D-MIG-057`)** — load/render only current + neighboring
   chunks, once real (denser) hand-authored worlds exist. Natural co-requisite with face-culling
   below.
8. **Expand `worlds3d/world001.vwr`, or author more `.vwr` worlds** — the current sample is a
   proof-of-concept (staircase + one room). Better done after task 1/2 settle so new worlds can use
   the corrected render modes from the start.
9. **Add face-culling/occlusion to `GETerrainRenderer`** — needed once worlds get denser (see task
   7/8); not needed at the current ~2700-block scale.
10. **Verify `GalaxyEggbertCNA`'s clean-exit path** — close the window via the window manager (not
    a forced kill) and confirm the process exits 0 with no leaked resources.

## 9. Do not do yet

- No further investment in Simple3D/U3D/Nova3D beyond bug fixes on the existing playable target —
  scheduled for gradual removal, not indefinite retention, per §1.
- No deleting/removing any `GalaxyEggbertSimple3D` code without an explicit removal task.
- No modifications to `../mobile-eggbert`, `../cna`, or `../simple-3d` without explicit user
  approval for that specific change. **Exception: `../easy-3d` has standing permission** (still
  scoped to small, generic 3D-batching helpers, not a scene graph/ECS/engine).
- No `.txt → .vwr` (or any) automated 2D-to-3D world converter — rejected, see §1.
- No Easy3D scope creep — no ECS, scene graph, physics, resource cache, editor, or Lua.
- No mass refactor of `include/GalaxyEggbert/Worlds/` unless a failing unit test justifies it.
- No `#ifdef GE_ENGINE_*` anywhere.
- No new gameplay mechanics not present in mobile-eggbert (no coins, coyote time, combo
  multipliers, star ratings, time bonuses).
- Do not implement any of §8 task 2's render modes before task 1 (the identification pass) is
  actually done — the whole point of the Q&A rounds is to avoid guessing render modes from
  unverified agent impressions again (that's what caused the `GoldPillar`/`Saw` misses).
- Commit after each finished task (standing instruction) — one commit per task, not batched.
  **Pushing** to `origin/develop` is still NOT standing authorization — only push on explicit
  request each time.

## 10. Resume prompt

```
Read NEXT.md first. Then inspect only the files needed for the first task in section 8.
Do not refactor unrelated code and do not expand scope beyond that one task.
Make one small, verified improvement — implement the task goal as described, nothing more.
Cross-reference mobile-eggbert source at ../mobile-eggbert (read-only) before implementing any
gameplay or world-format behavior. If the task needs a design decision (e.g. how to handle
per-frame tile animation), make the smallest reasonable choice and record it, rather than
blocking on it.
After the change, run the verification command listed for that task.
Update NEXT.md when done: move the completed task into section 3 (Recent changes), remove it
from section 8, and add whatever new next-smallest task naturally follows.
```
