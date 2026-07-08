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
  non-air cell), moves an invisible collision-only Blupi with tank controls, and renders parsed
  `MoveObject`s (pickups/enemies) as real textured billboards. No 3D Blupi model, no HUD, no sound,
  no gameplay logic yet.

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
  mode is now implemented and wired up for 90 of ~99 confirmed icons (2026-07-08, §3)** — icon
  200/`Platform`, 48 "4 sides + top/bottom color-or-open" icons, the 4 fan tiles, and 37 "1 or 2 of
  4 sides textured" icons whose facing was read directly from their crop image (mobile-eggbert
  confirmed to have **no per-placement rotation metadata at all** — for any tile that varies by
  facing it just uses a different icon number, e.g. `FanLeft`/`FanRight`/`FanUp`/`FanDown` are 4
  distinct icons; the questionnaire's "uloženo v metadatech bloku" phrasing was the answerer's own
  guess while eyeballing crops, not a real mobile-eggbert source finding). Only 9 confirmed icons
  remain: 2 need real texture alpha, 3 need a missing grass asset, 4 (icons 15-18) need a
  two-part axis+side-asymmetry read their crops didn't give a confident answer for.
  **`InnerPillarBox`/`InnerFlatPlate`/`TripleCrossBillboard` are now also implemented (2026-07-08,
  §3), all 76 confirmed icons across the 3 modes wired up** — `InnerPillarBox` reuses
  `DirectionalCubeItem` directly (a smaller box instead of a full-size cube); `InnerFlatPlate` and
  `TripleCrossBillboard` are 2 new `Easy3D` mesh builders (a double-sided plate; 3 double-sided
  planes 60° apart). All 4 confirmed render modes from the tile-identification work are now real
  code. **Water (icons 91-98) also now renders correctly (2026-07-08, §3)** — semi-transparent
  alpha-blended `UniformCube`s (the user's chosen design, resolving the last open render-mode
  question), not solid opaque cubes. See §8 for what's left (mostly per-icon data-quality
  follow-ups, not new mechanisms).
- A real documentation bug was found and fixed during round 3: `02-tiles.md`'s "0/78 files, unused"
  claim only checked the static `Decor:`/`BigDecor:` grid, not `MoveObject:` records' own `icon=`
  field (which can reference the same `object-m.png` sheet). At least 32 of the "unused" icons are
  confirmed actually used this way (crates, water splash, breathing bubbles, a pickup animation,
  a conveyor-lift tread, bridge-build frames) — see §3 and `02-tiles.md`'s correction note.

## 2. Current status

### Build status
- `GalaxyEggbertCNA` — **last confirmed clean build+run today (2026-07-08)**, after implementing
  `InnerPillarBox`/`InnerFlatPlate`/`TripleCrossBillboard` (§3, the last of this session's 4
  render-mode implementations). Built `GenerateSampleWorld3D`, `VerifyBlupiMovement`, and
  `GalaxyEggbertCNA` itself from `build-cna/` — all succeeded.
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
- `GalaxyEggbertWorldsTests` — not rebuilt/re-run this session; last confirmed 54/54 via
  `ctest --test-dir build` on 2026-07-07. The `cmake-build-debug` profile's `ctest` discovery
  issue (`GalaxyEggbertWorldsTests_NOT_BUILT`) was also last checked 2026-07-07, not re-verified.
  Note: that `build` tree configures Simple3D too (default ON) — re-verifying this needs a
  `-DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF` tree per §9, not the existing `build`/`cmake-build-debug`.

### Test status
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
  now implemented: `DirectionalCube` (90 of ~99 confirmed icons — 4 side faces always textured,
  top/bottom each independently open or a flat fallback color), `InnerPillarBox` (3 of 3 — outer
  cube fully transparent, a smaller inner box textured), `InnerFlatPlate` (63 of 63 — outer cube
  transparent, a single double-sided plate), `TripleCrossBillboard` (10 of 10 — 3 double-sided
  planes 60° apart through the block's center). Tank-control Blupi (invisible collision point),
  first-person camera + 2D animation-state HUD indicator, renders `MoveObject`s as billboards.
  Also now writes `screenshot.png` next to the binary on its first rendered frame
  (`GalaxyEggbertCnaGame::Draw`'s existing one-shot debug block) for visual verification.
- **Tile/object documentation**: `mobile-eggbert-reference/` — complete catalogs of all 441 tile
  icons (see §1), 204 `ObjectType`s, 93 sounds, 131 animation sequences, all backgrounds, plus a
  prose gameplay-behavior spec.
- **Sample world materials are now correct**: `worlds3d/world001.vwr`'s staircase (`RockPile`,
  icon 35) and walls/pillars (`BrickWall`, icon 261) use confirmed genuine bulk-material tiles
  instead of the mislabeled `StoneA`/`StoneB` (both turned out to be machine-piece graphics, not
  stone — see §3).

### What does not work yet
- `GalaxyEggbertCNA`: no visible 3D Blupi, no `BigDecor` rendering, no platform-lift/crate
  `UniformCube` object path, no HUD, no sound, no real gameplay logic (all expected at this phase).
- **All 4 confirmed render modes are now implemented; 166 of ~175 total confirmed icons across all
  4 are wired up** (99 `DirectionalCube` + 3 `InnerPillarBox` + 63 `InnerFlatPlate` + 10
  `TripleCrossBillboard`) — only 9 `DirectionalCube` icons remain (30/31 need real texture alpha, 107-109
  need a not-yet-sourced grass top texture, 15-18 need a two-part axis+side-asymmetry read their
  crops didn't give a confident answer for; see §8 task 1). `InnerFlatPlate`'s 63 icons all default
  to the same fixed plate axis/size (only a small sample of crops was individually checked, see
  §3) — good enough to render correctly, but not each icon's exact axis independently confirmed
  the way `DirectionalCube`'s icons were.
- Icon 107 (grass) needs a real top-face grass texture, which doesn't exist yet in the asset set —
  needs sourcing (license-compatible) or generating.
- Simple3D: build is currently broken in this environment (missing/incompatible U3D prebuilt, see
  §2) — **not to be fixed**, per user 2026-07-08: Simple3D is historical reference only now (§9).

## 3. Recent changes

Most recent first. Full history: `git log`.

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

**Tile-identification implementation is essentially done.** Tile identification itself (the
multi-session "active thread" through 2026-07-06/07) completed 2026-07-06/07 (§1); all 4 confirmed
render modes (`DirectionalCube`, `InnerPillarBox`, `InnerFlatPlate`, `TripleCrossBillboard`) are
now implemented in code and wired into `GETerrainRenderer`, and the water render mode is also
implemented (semi-transparent alpha-blended cube, the user's chosen design — 2026-07-08, §3) —
166 of ~175 total confirmed icons across the 4 tile-identification modes, plus water. What remains
is narrow: 9 specific `DirectionalCube` icons each blocked on a distinct small thing (texture
alpha, a missing grass asset, or a genuinely ambiguous crop — §8 task 1), not a new render-mode
mechanism. The next *new* mechanism work is `BigDecor`/platform-lift object rendering (§8 tasks
3-4).

## 5. Known bugs and limitations

| Status | Issue |
|---|---|
| incomplete | Icon 107 (grass) needs a real top-face grass texture — none exists yet; needs a license-compatible source or generation. |
| incomplete | 9 confirmed `DirectionalCube` icons still unwired: 30/31 (texture alpha), 107-109 (missing grass asset), 15-18 (ambiguous crop read) — see §8 task 1. |
| incomplete | `GalaxyEggbertCNA`: no Blupi/object-behavior rendering beyond billboards, no HUD, no sound, no gameplay logic (expected at this phase). No `BigDecor` rendering (parsed only). No platform-lift/crate `UniformCube` object path. |
| unverified this session | `GalaxyEggbertWorldsTests` 54/54 pass and the `cmake-build-debug` `ctest` discovery issue — both last checked 2026-07-07, not re-run today. |
| not to be fixed (per user, 2026-07-08) | Simple3D build fails at `find_package(Urho3D)` — U3D prebuilt missing/incompatible. `GalaxyEggbertSimple3D` is treated as historical reference only going forward; do not spend effort rebuilding/fixing it (see §2). |
| incomplete | `element.png` used for every `ObjectType` billboard, even though types 1/12 need `object-m.png` and 32/33 need `blupi1.png` (`DOC-007`, same gap in both targets). |
| incomplete | Simple3D: no per-zone fog, only `SetClearColor` per sky region. |
| incomplete | 7 `ObjectType`s (jeep/secret-exit/skateboard/suction-cup/mirror/balloon/dynamite) spawn with correct icons in Simple3D but have no real gameplay behavior on pickup/contact. |
| unknown | Simple3D Android/Web builds untested since the last engine change. |
| unknown | `GalaxyEggbertCNA`'s clean-exit-on-window-close path not separately exercised (only forced-kill/timeout tested). |
| needs verification | Simple3D: stomp bounce height (`kJumpSpeed * 0.65f`) vs. mobile-eggbert's real feel. |
| needs verification | Simple3D: crate push floor-support check only tested at y=0; mobile-eggbert links crate stacks vertically (`SearchLinkCaisse`) — whether galaxy-eggbert's port does too is unconfirmed. |
| risky assumption | `GalaxyEggbertCNA`'s world/texture loader uses relative paths — only works when run from its own build directory. |
| incomplete | `GETerrainRenderer` (CNA) has no face-culling/occlusion — fine at ~2700 blocks, will need revisiting for denser worlds. |

## 6. Architecture notes

### Main modules
```
include/GalaxyEggbert/Worlds/, src/GalaxyEggbert/Worlds/   — engine-agnostic voxel World (100×100
                                                               grid), Block/Chunk, .vwr save format.
                                                               Shared by BOTH targets.
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
                                GETerrainRenderer (World, all Y layers → static (opaque) +
                                animated (opaque) + water (semi-transparent, its own
                                m_waterRenderer/m_waterBlocks and RebuildAnimatedRenderer() call,
                                drawn last with BlendState::NonPremultiplied/
                                DepthStencilState::DepthRead) CubeMeshRenderer-backed meshes; per
                                block, one shared AppendSpecialGeometry() helper checks all 4
                                special render-mode tables in order — DirectionalCube,
                                InnerPillarBox, InnerFlatPlate, TripleCrossBillboard — falling back
                                to plain UniformCube; both the opaque-animated and water paths call
                                the same helper, keyed by animation-group base icon, not the
                                current frame's icon), GEDirectionalCubeTiles (90 of ~99 confirmed
                                DirectionalCube icons), GEInnerPillarBoxTiles (3 of 3),
                                GEInnerFlatPlateTiles (63 of 63), GETripleCrossBillboardTiles
                                (10 of 10), GESwatchUv (shared "flat fallback color" sampling
                                helper), GEBlupiController, GEObjectIcons, an Easy3D::Camera3D
                                (first-person).

tools/GenerateSampleWorld3D.cpp — builds worlds3d/world001.vwr. Ground floor/staircase/platform
                                use RockPile, walls/pillars use BrickWall (no more Ground/StoneA/
                                StoneB usage); a demo row of DirectionalCube/InnerPillarBox/
                                InnerFlatPlate/TripleCrossBillboard blocks was added near spawn
                                2026-07-08 (§3).

mobile-eggbert-reference/    — questionnaire-unidentified-tiles.md (round 1, 34 icons, DONE),
                                questionnaire-all-remaining-tiles.md (round 2, 280 icons, DONE),
                                questionnaire-unused-tiles.md (round 3, 97 icons, DONE) — together
                                account for all 441 object-m.png icons (see §1). 02-tiles.md is the
                                canonical catalog; its "Unused/unnamed icons" section now carries a
                                correction note about the MoveObject icon= finding (§3).

../easy-3d/                  — companion library beside CNA. Standing permission to modify.
                                CubeMesh/CubeMeshRenderer (terrain: UniformCube, DirectionalCube via
                                AppendDirectionalCubeMesh, PlateItem/AppendPlateMesh, TripleCrossItem/
                                AppendTripleCrossMesh — all added 2026-07-08, all emitting plain
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

# Build + run world-model unit tests:
cmake --build build --target GalaxyEggbertWorldsTests -j2
ctest --test-dir build --output-on-failure          # 54/54 expected (last confirmed 2026-07-07)

# Configure + build the CNA target (opt-in, off by default):
cmake -S . -B build-cna -DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF
cmake --build build-cna --target GalaxyEggbertCNA -j2
cd build-cna && ./GalaxyEggbertCNA        # must run from its own build dir (relative asset paths)

# Regenerate the hand-authored 3D sample world (after tools/GenerateSampleWorld3D.cpp changes):
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

1. **Backfill the remaining 9 confirmed `DirectionalCube` icons into
   `GEDirectionalCubeTiles.cpp`.** 90 of ~99 are done (2026-07-08, §3). mobile-eggbert was
   confirmed to have NO per-placement rotation metadata (it uses a separate icon ID per facing
   instead, e.g. the 4 fan icons) — so this is back to being pure per-icon work, no `World`/`Block`
   format change needed. What's left, grouped by the missing piece:
   - **Icons 30/31 need real texture alpha** ("textura má i průhlednost") — **no longer blocked**:
     confirmed 2026-07-08 (implementing the water render mode, same §3 entry) that CNA's terrain
     draw path DOES support alpha-blended geometry (`BlendState::NonPremultiplied` +
     `DepthStencilState::DepthRead`, see `GETerrainRenderer::Draw()`'s water pass) and
     `object-m.png` has real per-pixel alpha. These 2 icons likely need a similar dedicated
     semi-transparent pass (or possibly just fold into the existing water pass's blend/depth
     state if their alpha need is comparable) rather than a brand-new mechanism.
   - **Icons 107-109 need the not-yet-sourced grass top texture** (§8 task 2) before they can be
     wired up (icon 107 itself IS that grass-texture task).
   - **Icons 15-18 need a two-part read their crops didn't give a confident answer for**: an axis
     choice (which 2 opposite side faces are textured) AND which of the 2 remaining perpendicular
     side faces is open vs. flat-color (their crops are diagonal wedge shapes, not a clean
     axis-aligned cue like icon 49's). Needs the user to look at
     `mobile-eggbert-reference/images/tile-full-0{15,16,17,18}.png` directly and decide, the same
     way the fan-base question got resolved (see §3, 2026-07-08).
   **Files:** `src/GalaxyEggbertCNA/Game/GEDirectionalCubeTiles.cpp`. **Verification:** a live
   screenshot per batch added to the sample world plus a `VerifyBlupiMovement` regression check
   and the vertex/triangle-count arithmetic check (matches the pattern in §3's 2026-07-08 entries).
2. **Source or generate a grass-top texture for icon 107.** No such asset exists yet. **Files:**
   likely a new file under `mobile-eggbert-reference/` or a texture-atlas addition in
   `GETileAtlas`. **Verification:** visual check once icon 107 is added to `GEDirectionalCubeTiles`
   (task 1) with a real top-face texture.
3. **`BigDecor` billboard rendering for CNA** — `GEWorldRuntime` doesn't parse `BigDecor:` for CNA
   at all yet (Simple3D already does — reference its parsing logic read-only, do not build/run
   Simple3D itself, see §9). Recommended render mode: `Billboard`. **Files:**
   `src/GalaxyEggbertCNA/Game/GEWorldRuntime.*`, `GalaxyEggbertCnaGame.cpp`. **Verification:** a
   tool mirroring `VerifyMoveObjectTypesCna.cpp` against a real level with known `BigDecor:` cells.
4. **Platform-lift/crate `UniformCube` object path for CNA** — reuse existing terrain
   `CubeMesh`/`CubeMeshRenderer` machinery for the two approved "objects are cubes, not billboards"
   exceptions. **Files:** `src/GalaxyEggbertCNA/GalaxyEggbertCnaGame.cpp`.
5. **Re-verify `GalaxyEggbertWorldsTests` (54/54)** — last checked 2026-07-07, not re-run since.
   Use a Simple3D-OFF tree (e.g. `build-cna`, or a fresh configure with
   `-DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF`) — do **not** use `cmake-build-debug`, which configures
   Simple3D/U3D and is currently broken for unrelated reasons the user has said not to fix (§9).
   **Verification:** `ctest --test-dir <tree> --output-on-failure`.
6. **Add face-culling/occlusion to `GETerrainRenderer`** — needed once worlds get denser; not
   needed at the current ~2700-block scale.
7. **Verify `GalaxyEggbertCNA`'s clean-exit path** — close the window via the window manager (not
   a forced kill/timeout) and confirm the process exits 0 with no leaked resources.
8. **Optional polish: spot-check more of `GEInnerFlatPlateTiles`'s 63 icons' crops** for a
   different axis/size than the current uniform default — only 5 were sampled (77, 110, 114, 264,
   367), all consistent with "no reliable cue, default is fine," but not exhaustive like
   `DirectionalCube`'s per-icon backfill was. Low priority — the default already renders correctly,
   this would only improve fidelity for any icon that turns out to need a different axis.

## 9. Do not do yet

- **No building, fixing, or troubleshooting `GalaxyEggbertSimple3D`/U3D/Nova3D at all (updated
  2026-07-08, per user)** — it is a historical reference only now, not a maintained target. Its
  build is currently broken in this environment (missing/incompatible U3D prebuilt) and that is
  not to be fixed. Source may still be read for reference (e.g. porting `BigDecor` parsing logic to
  CNA), but do not configure/build/run it or spend effort on its build environment.
- No deleting/removing any `GalaxyEggbertSimple3D` code without an explicit removal task.
- No modifications to `../mobile-eggbert`, `../cna`, or `../simple-3d` without explicit user
  approval for that specific change. `../easy-3d` has standing permission (still scoped to small,
  generic 3D-batching helpers).
- No `.txt → .vwr` (or any) automated 2D-to-3D world converter — rejected.
- No Easy3D scope creep — no ECS, scene graph, physics, resource cache, editor, or Lua.
- No mass refactor of `include/GalaxyEggbert/Worlds/` unless a failing unit test justifies it.
- No `#ifdef GE_ENGINE_*` anywhere.
- No new gameplay mechanics not present in mobile-eggbert.
- **No adding a `GEDirectionalCubeTiles.cpp`/`GEInnerPillarBoxTiles.cpp`/`GEInnerFlatPlateTiles.cpp`/
  `GETripleCrossBillboardTiles.cpp` entry speculatively, without re-checking the exact per-icon
  answer recorded in `questionnaire-all-remaining-tiles.md`/`questionnaire-unused-tiles.md`/
  `02-tiles.md` first** — that's the whole point of having done direct user Q&A instead of agent
  guessing. 166 of ~175 confirmed icons across all 4 modes are wired up (2026-07-08, §3); only 9
  `DirectionalCube` icons remain (§8 task 1).
- No re-running the full 97-icon or 280-icon questionnaires again — both are done; only the small
  number of explicitly-open items (icon 107 grass texture, icons 15-18's face config) need further
  decisions. Water's render mode is decided and implemented (2026-07-08, §3) — not open anymore.
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
reasonable choice and record it, rather than blocking on it — unless it's one of the explicitly
open questions in §5 (grass texture, icons 15-18's face config), which need the user's actual
decision, not a guess.
After the change, run the verification command listed for that task.
Update NEXT.md when done: move the completed task into section 3 (Recent changes), remove it
from section 8, and add whatever new next-smallest task naturally follows.
```
