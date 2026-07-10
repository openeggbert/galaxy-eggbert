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
  non-air cell), moves an invisible collision-only Blupi with tank controls, and renders
  `MoveObject`s and `BigDecor:` cells (pickups/enemies/decor) as real textured billboards (plus
  `UniformCube`s for platform lifts/crates). **MoveObjects can now be embedded directly in the
  `.vwr` format itself** (2026-07-09, §3) — no mobile-eggbert `.txt` file needed to see any of
  them render. No 3D Blupi model, no HUD, no sound, no gameplay logic yet.

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
  model yet.
- **Tile/object documentation**: `mobile-eggbert-reference/` — complete catalogs of all 441 tile
  icons (see §1), 204 `ObjectType`s, 93 sounds, 131 animation sequences, all backgrounds, plus a
  prose gameplay-behavior spec.
- **Sample world materials are now correct**: `worlds3d/world001.vwr`'s staircase (`RockPile`,
  icon 35) and walls/pillars (`BrickWall`, icon 261) use confirmed genuine bulk-material tiles
  instead of the mislabeled `StoneA`/`StoneB` (both turned out to be machine-piece graphics, not
  stone — see §3).

### What does not work yet
- `GalaxyEggbertCNA`: no real Blupi model yet (a temporary placeholder exists in third-person mode
  only, §3), no HUD, no sound, no real gameplay logic (all expected at this phase — no interactive
  object system yet, so platform lifts/crates render correctly but don't move or respond to Blupi).
  `BigDecor:` rendering and the platform-lift/crate `UniformCube` object path are now both
  implemented (2026-07-09, §3).
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

**No code blocker.** `GalaxyEggbertCNA` builds and runs cleanly as of today's fix, and
`VerifyBlupiMovement` passes. The previously-reported missing-`rules.ninja` build issue did not
reproduce this session.

**Tile-identification implementation is complete.** Tile identification itself (the multi-session
"active thread" through 2026-07-06/07) completed 2026-07-06/07 (§1); all 4 confirmed render modes
(`DirectionalCube`, `InnerPillarBox`, `InnerFlatPlate`, `TripleCrossBillboard`) are implemented and
wired into `GETerrainRenderer`, the water render mode is implemented (semi-transparent
alpha-blended cube, the user's chosen design), icon 107 has a real procedurally-generated
grass-top texture, and — as of 2026-07-09 — **every one of the ~175 total confirmed icons across
all 4 modes is wired up**, including the last 6 `DirectionalCube` icons (15-18, 108-109), which
used this session's established ambiguous-icon default since their crops didn't resolve a facing
decision even after direct user review (§3). The previously-reported seam-line artifact is
root-caused and substantially (60%) mitigated, not fully eliminated (§5, §8 task 1). `BigDecor:`
billboard rendering, the platform-lift/crate `UniformCube` object path, 3D-format MoveObject
storage, and static-terrain face culling are all implemented too — no render-mechanism work
remains on the list; §8's remaining tasks are both explicitly optional/low-priority polish.

## 5. Known bugs and limitations

| Status | Issue |
|---|---|
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
  **Pushing** to `origin/develop` is still NOT standing authorization — only push on explicit
  request each time.

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
