# NEXT.md — Galaxy Eggbert

## 1. Project summary

**Galaxy Eggbert** is a faithful 3D remake of **mobile-eggbert** (a C++ port of *Speedy Blupi*, a
Windows Phone XNA game from 2013). Main goal: reimplement the same game logic, levels, and assets
in 3D — perspective camera, billboard sprites, 3D-rendered tiles — without inventing new mechanics.

**Current development phase:** mid-migration between two build targets:

- `GalaxyEggbertSimple3D` (built on `simple-3d` → U3D/Urho3D) — the **only playable** target,
  feature-complete enough to play through core mechanics end-to-end (see §2).
- `GalaxyEggbertCNA` (built directly on **CNA** + **Easy3D** helper library) — the **new
  long-term target**. Currently: opens a window, loads a genuinely 3D, hand-authored `.vwr` world
  (`worlds3d/world001.vwr` — real Y variation, not a flat mobile-eggbert layout), and renders real,
  textured terrain across all Y layers (one cube per non-air world cell, using `object-m.png`) with
  working animated tiles (lava/crusher/saw/spike/water/fan/marine/temp), and moves an invisible,
  collision-only Blupi placeholder around it (arrow keys + Space, grid collision with step-up
  traversal and gravity — no sprite yet). No object/pickup rendering, no HUD, no sound, no real
  gameplay yet.

**Important architectural decisions** (recorded in `plan.md`/`easy3d.md`/`CLAUDE.md`):

- Direct CNA + Easy3D supersedes the old Simple3D → U3D → Nova3D direction as the long-term
  target. `GalaxyEggbertSimple3D` is **not being deleted** — it stays as the working reference
  until the CNA/Easy3D path reaches feature parity with it.
- Easy3D is a small helper library beside CNA (cameras, texture atlas, batching, mesh building,
  renderer adapters) — it must not hide CNA; game code may call CNA directly.
- `../mobile-eggbert` is **read-only**. Its assets (PNGs, sounds, world files) are freely reused
  by direct file path / build-time copy. Its code (`Pixmap`, `Sound`, `Decor.cpp`, tables, enum
  values) is reference-only — copying or linking requires explicit user approval first.
  mobile-eggbert currently has **no CMake library target** (only `add_executable`), so it cannot
  be linked as a dependency today regardless of approval.
- Mobile-eggbert's world files are flat 2D data (Y=0 everywhere) and are reference/inspiration
  only for world design — they are **not** the intended long-term data source for
  `GalaxyEggbertCNA`. There is **no** and will be **no** automated `.txt → .vwr` converter that
  "promotes" a 2D level into a 3D one (rejected direction, unchanged). `GalaxyEggbertCNA` now
  defaults to loading a genuinely 3D, hand-authored `.vwr` world (`worlds3d/world001.vwr`,
  generated via `tools/GenerateSampleWorld3D.cpp`); `GEWorldRuntime::LoadFromMobileEggbertFile`
  stays in the code only as a secondary/reference path (e.g. for later faithful-remake level
  porting), not the default anymore.
- New CPU-side mesh builders and CNA renderer adapters (e.g. `Easy3D::CubeMesh`,
  `Easy3D::CubeMeshRenderer`) live inside `../easy-3d` itself, not as galaxy-eggbert-local
  adapters — decided because that work is generic 3D-batching plumbing with zero Eggbert-specific
  knowledge, matching Easy3D's own stated role. Modifying `../easy-3d` still requires explicit,
  per-change user approval; it is not blanket-authorized.
- **Open design question, not decided yet (2026-07-03):** how should mobile-eggbert's objects/
  elements (all `ObjectType`s) actually be rendered in 3D — billboard, or a textured cube (with
  untextured faces filled by a per-texture fallback color instead of left blank), or a mix depending
  on the object? Whichever objects use the cube approach will need per-block metadata for render
  mode, fallback color, and likely a 4-direction facing/rotation for directional textures. Full
  writeup: `mobile-eggbert-reference/09-open-questions.md` (first bullet) — this is a generalization
  of the already-noted doors-as-billboards question, not a separate decision to make independently.

## 2. Current status

### Build status
- `GalaxyEggbertSimple3D` — **builds clean** (`cmake -S . -B build && cmake --build build -j2`).
- `GalaxyEggbertWorldsTests` — **54/54 tests pass** (`ctest --test-dir build`).
- `GalaxyEggbertCNA` — **builds clean** (opt-in: `-DGALAXY_EGGBERT_BUILD_CNA=ON`), **runs and
  renders real, textured terrain with working animated tiles**.
- `../easy-3d` — default (headers-only) build and CNA-linked build both green; 2/2 and 5/5 tests
  respectively (the newest class, `CubeMeshRenderer`, is compile-check-only — see §6).

### What works
- **`GalaxyEggbertSimple3D`** (unchanged by this migration): world loading from mobile-eggbert
  `.txt`, textured/animated 3D terrain, Blupi billboard with state machine, mobile-object
  billboards, crate push, platform patrol, hazard detection, enemy stomp, respawn invincibility,
  pickups (treasure/keys/shield/egg/drink), exit-gate logic, HUD, save/load (3 slots), 93-channel
  sound, 3rd-person orbit camera, 5 sky colors per region.
- **`GalaxyEggbertCNA`**: CMake option `GALAXY_EGGBERT_BUILD_CNA` (default OFF) builds it
  alongside/instead of Simple3D; links `CNA` + `easy3d::easy3d`; copies
  `../mobile-eggbert/Content/` + `worlds/` and this repo's own `worlds3d/` next to its binary at
  build time. By default loads `worlds3d/world001.vwr` — a genuinely 3D, hand-authored world
  (`GEWorldRuntime::LoadFromVwrFile`, real Y variation) — into the shared
  `GalaxyEggbert::Worlds::World` (`LoadFromMobileEggbertFile` for mobile-eggbert `.txt` files still
  exists as a secondary/reference path). Maps block types to `object-m.png` UV rects
  (`GETileAtlas`); walks every Y layer of the loaded world and renders one textured cube per
  non-air cell (`GETerrainRenderer` + `Easy3D::CubeMeshRenderer`), textured with a real CNA
  `Texture2D` of `object-m.png`, animated tiles included (see below). Verified by a real run on
  `worlds3d/world001.vwr`: `2749 blocks, 65976 vertices, 32988 triangles` uploaded, Y range
  [0, 13]; a 5x5 on-screen pixel sample found 21/25 sampled points showing terrain color with 10
  distinct colors among them (confirms real texture sampling, not a placeholder).
- **`Easy3D::CubeMesh`** (`AppendCubeMesh`/`BuildCubeMesh`): turns `CubeBatch` items into
  vertex/index arrays (24 vertices + 36 indices per cube).
- **`Easy3D::CubeMeshRenderer`**: uploads that data to CNA `VertexBuffer`/`IndexBuffer` once and
  issues `DrawIndexedPrimitives` via a caller-configured `BasicEffect` — the real CNA draw path,
  matching CNA's own `examples/house3d_demo.cpp` pattern.
- **`GalaxyEggbertCNA` animated tiles**: `GETerrainRenderer` splits blocks into a static mesh
  (built once) and an animated subset (lava/crusher/saw/spike/water1/water2/fan×4/marine/temp)
  rebuilt into a second `CubeMeshRenderer` whenever the anim phase changes, driven by
  `GEWorldRuntime::Update(dt)`/`GetAnimPhase()` (6 fps, matches Simple3D/mobile-eggbert) called
  from `GalaxyEggbertCnaGame::Update(GameTime&)`. Verified with mobile-eggbert's `world024.txt`
  (137 animated blocks, loaded via the secondary `.txt` path as a build-artifact-only test, not
  committed): ran 8s / ~48 rebuild cycles with no crash. The default `worlds3d/world001.vwr` has
  0 animated tiles (none of its block types are hazard/animated ones).
- **First hand-authored 3D world**: `worlds3d/world001.vwr`, generated by
  `tools/GenerateSampleWorld3D.cpp` (new CMake tool target, engine-agnostic) — ground floor,
  10-step ascending staircase, raised platform, walled room with doorway, two pillars; 2749
  non-air blocks, Y range [0, 13]; round-trip-verified via `World::loadFromFile()`.

### What does not work yet
- `GalaxyEggbertCNA`: no Blupi *rendering* (it's an invisible collision point — see §3), no
  object/pickup rendering, no HUD, no sound, no real gameplay. `Easy3D::BillboardBatch`/`DebugDraw`
  have no vertex builder or renderer adapter at all yet (needed for Blupi's sprite and objects).
- `GalaxyEggbertSimple3D`: camera shake is a no-op; no per-zone fog; Android/Web builds untested
  since the last engine change.

## 3. Recent changes

Most recent first. Committed and pushed to `origin/develop`: `galaxy-eggbert` commit `a636649`
("feat: render real, textured terrain in GalaxyEggbertCNA from world data"), `../easy-3d` commit
`b4c52c0` ("feat: add CubeMesh vertex builder and CubeMeshRenderer CNA draw adapter"). Everything
below this line is committed locally (one commit per `DOC-1xx` task, per user instruction) but
**not pushed** to `origin/develop` yet (see §9 — pushing needs explicit request each time).

- **Fixed a second GIF-tooling bug: translucent content vanishing entirely (DOC-105, new,
  committed locally)**: found while regenerating `tile-anim-water1.gif`. Water1's source icons are
  genuinely translucent (alpha 0-127, never above ~50%) — GIF can only store binary transparency,
  and ImageMagick's automatic per-image heuristic collapsed the whole frame to a single, fully
  transparent color (confirmed the **already-committed** GIF had this exact defect too, pre-existing).
  A near-identical tile, Water2, did not collapse — the heuristic is inconsistent, not a clean
  threshold. Per user decision: since GIF can't show partial alpha here either way, show the
  sprite's real saturated color rather than pre-blending against an arbitrarily-chosen backdrop.
  Fixed in `make-gif.sh` (`-channel A -threshold 1% +channel` per frame before assembly) — verified
  no meaningful regression on the already-shipped lava/spike/crusher/saw GIFs (~2-3/255 alpha shift,
  visually unchanged), so those were left as-is. Full writeup in `plan.md`'s `DOC-105`.
- **Fixed a real tile-atlas UV bug shared by both engine targets (S3D-2 follow-up, committed
  locally)**: found while regenerating `mobile-eggbert-reference/tile-anim-lava.gif` (`DOC-101`)
  — the user spotted a thick blue bar and a seam cutting the lava sprite in a screenshot.
  `BlockTypes::tileUV()` (`include/GalaxyEggbert/BlockTypes.hpp`, shared by `GalaxyEggbertSimple3D`
  and `GalaxyEggbertCNA`) assumed a flat, contiguous 64px grid in `object-m.png`; the real sheet
  (confirmed from mobile-eggbert's own `Pixmap::GetSrcRectangle`, `PixmapChannel::Object`:
  `srcGap=1`) is packed on a 65px pitch (64px icon + 1px gap) with a 1px leading margin, causing a
  cumulative 1px/column/row drift that bled neighboring icons in by later rows/columns. Fixed by
  adding `BlockTypes::kSheetGap=1` and using it in `tileUV()`'s pixel math, and passing the same
  gap to `Easy3D::TextureAtlas::AddGrid` in `src/GalaxyEggbertCNA/Game/GETileAtlas.cpp` (which
  already supported gap/offset params — no `../easy-3d` change needed); also corrected its row
  count from ceil(1431/64)=23 to the pitch-aware floor((1431-1)/65)=22 (verified nothing used icons
  in the old bogus row — highest named `BlockTypes` constant is 413). **Verified**: both
  `GalaxyEggbertSimple3D` and `GalaxyEggbertCNA` rebuild clean; `GalaxyEggbertWorldsTests` still
  54/54; a real `GalaxyEggbertCNA` run logs UV values matching the corrected formula by hand, and
  its terrain-visibility check still shows real, multi-color textured terrain (25/25, 7 distinct
  colors) — no regression. Full writeup in `plan.md` under `S3D-2`. Files:
  `include/GalaxyEggbert/BlockTypes.hpp`, `src/GalaxyEggbertCNA/Game/GETileAtlas.cpp`.
- **Invisible collision-only Blupi placeholder (E3D-MIG-060, new)**: added
  `GEBlupiController` (`src/GalaxyEggbertCNA/Game/GEBlupiController.hpp/.cpp`) — arrow keys move,
  Space jumps, grid-based collision against `Worlds::World` with step-up traversal (climbs up to 1
  block) and real gravity/landing. Deliberately engine-agnostic (only depends on `Worlds::World`,
  no CNA types) so it's independently testable: added `tools/VerifyBlupiMovement.cpp` (new CMake
  tool, no CNA link needed) which scripts input against the real `worlds3d/world001.vwr` and
  checks spawning grounded, climbing the staircase, being blocked by a wall, and falling under
  gravity — all passed. This caught a real bug in `tools/GenerateSampleWorld3D.cpp` (the platform
  floor double-stacked a block on `x=20`, creating an unclimbable 2-block cliff at the top of the
  staircase) — fixed and regenerated `worlds3d/world001.vwr` (2729 blocks now, was 2749).
  `GalaxyEggbertCnaGame` now spawns Blupi at world `(0,1,0)` and the camera follows it every frame
  instead of a fixed terrain-centroid shot. No sprite/visual yet (`E3D-MIG-061..063`).
- **Chunk-radius streaming re-scheduled (E3D-MIG-057)**: user overrode the earlier "not needed
  now" call — galaxy-eggbert's real hand-authored 3D worlds are intended to be much denser/more
  three-dimensional than the current placeholder sample, so this is now marked **scheduled** in
  `plan.md` (not yet implemented).

- **First hand-authored 3D world, fully wired (E3D-MIG-058, new, not committed)**: added
  `tools/GenerateSampleWorld3D.cpp` (new CMake target `GenerateSampleWorld3D`, engine-agnostic,
  only depends on `include/GalaxyEggbert/Worlds/`) and ran it once to produce
  `worlds3d/world001.vwr` — a ground floor, a 10-step solid ascending staircase, a raised platform,
  a walled room with a doorway, and two pillars, built with `World::setBlock()`/`saveToFile()` and
  round-trip-verified with `World::loadFromFile()`: 2749 non-air blocks, Y range [0, 13] — real
  height variation, not another flat Y=0 layout. This directly addresses the user's flag that
  mobile-eggbert's flat 2D `.txt` files should not be galaxy-eggbert's real world-data source.
  Wired in per user decisions: added `GEWorldRuntime::LoadFromVwrFile()` and made it
  `GalaxyEggbertCnaGame`'s default load (kept `LoadFromMobileEggbertFile` as a secondary/reference
  path, not removed). Fixed the real gap this surfaced: `GETerrainRenderer` previously only ever
  read `world.getBlock(x, 0, z)` — it now walks every Y layer, and `GETerrainRenderer`/
  `GalaxyEggbertCnaGame`'s camera framing gained a `CentroidY()` instead of assuming Y=0. Added
  `worlds3d/` to the CMake asset-copy step. **Verified with a real run:**
  `loaded worlds3d/world001.vwr — 2749 non-air blocks, Y range [0, 13]` → `terrain mesh uploaded —
  2749 blocks (0 animated), 65976 vertices, 32988 triangles` → visibility check `21/25` sampled
  points showing textured terrain, 10 distinct colors; ran 8s with no crash.
- **`GalaxyEggbertCNA` animated tiles (E3D-MIG-055, new, not committed)**: `GETerrainRenderer` now
  splits blocks into a static mesh (built once, unchanged behavior) and an animated subset
  (lava/crusher/saw/spike/water1/water2/fan-left/right/up/down/marine/temp), rebuilt into a second
  `Easy3D::CubeMeshRenderer` only when the animation phase changes. Frame tables and
  `animIcon`/`isAnimated` logic ported 1:1 from `GalaxyEggbertSimple3D`'s already-shipped
  `GETerrainRenderer.cpp` (same repo, already-approved data — not a fresh mobile-eggbert
  transcription). Added `GEWorldRuntime::Update(dt)`/`GetAnimPhase()` (6 fps tick, matches
  Simple3D/mobile-eggbert) and a `GalaxyEggbertCnaGame::Update(GameTime&)` override to drive it.
  Verified with `worlds/world001.txt` (0 animated tiles — proves the empty-set path is crash-safe)
  and, via a build-artifact-only substitution of mobile-eggbert's `world024.txt` (not committed,
  restored after), with 137 real animated blocks — ran 8s (~48 rebuild cycles at 6 fps) with no
  crash. Files: `src/GalaxyEggbertCNA/Game/GETerrainRenderer.hpp/.cpp`,
  `src/GalaxyEggbertCNA/Game/GEWorldRuntime.hpp/.cpp`,
  `src/GalaxyEggbertCNA/GalaxyEggbertCnaGame.hpp/.cpp`. No `../easy-3d` changes needed. Decided and
  recorded in `plan.md` (E3D-MIG-057, not scheduled): world data stays loaded whole (no
  chunk-radius streaming) — worlds are small (100×100, hundreds of blocks) and mobile-eggbert
  itself has no such concept.
- **`GalaxyEggbertCNA` terrain texturing**: loaded `object-m.png` (1301×1431 px, matches
  `BlockTypes::kSheetW/kSheetH`) via CNA's own `Texture2D(assetName, GraphicsDevice&)` constructor
  and bound it to the terrain's `BasicEffect` (`TextureEnabled=true`). Deliberately **not** via
  mobile-eggbert's `Pixmap` class (2D-`SpriteBatch`-coupled, not usable for this 3D `BasicEffect`
  path, and mobile-eggbert has no library target to link anyway). Files:
  `src/GalaxyEggbertCNA/GalaxyEggbertCnaGame.hpp/.cpp`.
- **`GETerrainRenderer` (new)**: `src/GalaxyEggbertCNA/Game/GETerrainRenderer.hpp/.cpp`. Walks the
  full 100×100 `World` grid, queues one `CubeBatch` item (1×1×1, UV from `GETileAtlas`) per
  non-air block, builds it once via `Easy3D::BuildCubeMesh` + `Easy3D::CubeMeshRenderer` (no
  per-frame rebuild — static/non-animated). Tracks the block-position centroid, used to aim the
  camera reliably (the spawn tile itself is usually the open-air cell Blupi stands in, not a
  solid block). Replaced the temporary hardcoded debug cube in `GalaxyEggbertCnaGame` entirely.
- **`Easy3D::CubeMeshRenderer` (new, in `../easy-3d`)**:
  `include/Easy3D/CubeMeshRenderer.hpp` + `src/CubeMeshRenderer.cpp`. CNA renderer adapter —
  uploads `CubeMesh` vertex/index data to CNA `VertexBuffer`/`IndexBuffer` once, then
  `Draw(GraphicsDevice&, BasicEffect&)` issues `DrawIndexedPrimitives`. Along the way, fixed a
  real bug: `../easy-3d`'s headers-only default build was missing `../sharp-runtime/include`
  (needed transitively by `GraphicsDevice.hpp`/`BasicEffect.hpp`) — added a new
  `EASY3D_SHARP_RUNTIME_DIR` CMake cache variable mirroring `EASY3D_CNA_DIR`. New
  `tests/test_cube_mesh_renderer.cpp` — compile-check only (unconditionally, not gated by
  CNA-link), since the class needs a live `GraphicsDevice` a plain test `main()` can't produce.
- **`GETileAtlas` (new)**: `src/GalaxyEggbertCNA/Game/GETileAtlas.hpp/.cpp`. Maps block types to
  `object-m.png` UV rects via `Easy3D::TextureAtlas::AddGrid`, exploiting that its row-major frame
  numbering already matches `BlockTypes`' `icon = row*kSheetCols+col` convention.
- **`Easy3D::CubeMesh` (new, in `../easy-3d`)**: `include/Easy3D/CubeMesh.hpp` +
  `src/CubeMesh.cpp`. CPU-side vertex/index builder for `CubeBatch` items (24 vertices + 36
  indices per cube, 4 vertices per face so each face carries its own UV corners). New
  CNA-link-gated `tests/test_cube_mesh.cpp`.
- **Decision recorded**: Easy3D's CPU-side vertex builders and CNA renderer adapters belong
  inside `../easy-3d` itself (its own roadmap Phase 3/4), not as a galaxy-eggbert-local adapter —
  recorded in `plan.md` and `easy3d.md` §12 Q5.
- **External fix (not ours)**: `../cna` shipped commit `e1939bc` fixing a `StorageDevice`/
  `IAsyncResult` interface mismatch (from an unrelated `../sharp-runtime` change) that had briefly
  broken the full `GalaxyEggbertCNA` build. Confirmed resolved by a clean rebuild.
- Earlier, already-committed history: `3293a3d` (reject 2D→3D world auto-converter),
  `0f01490` (mobile-eggbert asset build-time copy + world-file parsing for `GalaxyEggbertCNA`,
  `ObjectType`/`SoundChannel` ID parity confirmed identical to mobile-eggbert),
  `c6b6456` (`GalaxyEggbertCNA` skeleton target), `f579824`/`ebea834` (direction-lock docs),
  `ac1d8d1` (Simple3D crate push + platform patrol fix).

Files changed this session and not yet committed (`galaxy-eggbert` only — no `../easy-3d` changes
this session): `NEXT.md`, `plan.md`, `CMakeLists.txt`; modified
`src/GalaxyEggbertCNA/GalaxyEggbertCnaGame.hpp/.cpp`,
`src/GalaxyEggbertCNA/Game/GETerrainRenderer.hpp/.cpp`,
`src/GalaxyEggbertCNA/Game/GEWorldRuntime.hpp/.cpp`; new `tools/GenerateSampleWorld3D.cpp` and
`worlds3d/world001.vwr`. `src/GalaxyEggbertSimple3D/` was not touched. `../mobile-eggbert`,
`../cna`, `../simple-3d`, and `../easy-3d` all remain untouched this session.

## 4. Current blocker / main problem

**Documentation track (not code): `mobile-eggbert-reference/`'s GIFs are corrupted and the whole
effort has been reopened — this is the actual current blocker for that track.** The user visually
inspected `08-animations.md` and found that every animated GIF's frames accumulate the previous
frame's opaque pixels instead of clearing ("frame 2 shows frame 1 ghosted into its background").
Confirmed programmatically (coalesce each GIF, measure mean alpha per frame — should fluctuate with
the real source frame data, not monotonically converge):
- `blupi-action-02-march.gif`: 80.0→85→86.2→86.3→86.6→86.7, plateaus (bug present).
- `object-anim-type05-treasure.gif`: 133.7→...→153.6, plateaus (bug present).
- `explosion-anim-explo1.gif`: 9.3→...→161.9, plateaus (bug present).
- `tile-anim-temp.gif`: alpha correctly oscillates, hits exactly 0 on the two genuinely-blank
  frames (no symptom) — but tile content is fully opaque, so this test can't rule out the same root
  cause being invisible there; **do not assume tiles are exempt**.

All 129 animated GIFs need regeneration with fixed tooling. The user also asked whether `DOC-002`
(tiles)/`DOC-003` (objects) can be trusted given this — their *text/classification* content is
unaffected (it's not image-specific), but every generated image is being re-verified rather than
assumed correct, since 11 sprite-channel bugs have already turned up in that same effort. Per the
user's explicit instruction, this has been broken into ~168 small, single-purpose tasks
(`plan.md` §16, `DOC-100`–`DOC-267`) instead of a few large opaque passes, since the large-pass
approach is part of how these gaps went undetected in the first place.

**`DOC-100` (root cause) is now fixed and verified** — the old workflow assembled GIFs with plain
`convert -delay D -loop 0 frame*.png out.gif`, which leaves every frame's GIF disposal method as
`Undefined` (confirmed via `identify -format "%D"` on the existing broken GIFs). Without an
explicit disposal, each new frame composites on top of whatever is still on the canvas from the
previous one instead of a cleared background, so transparent/semi-transparent pixels let prior
opaque pixels bleed through — mean alpha climbs then plateaus at saturation, matching the reported
symptom exactly. Fix: `-dispose Background`, so each frame clears to a transparent canvas first.
Verified with a synthetic 6-frame repro: broken path reproduced the climbing pattern
(17.6→21.3→24.9→28.5→32.1→35.7), fixed path gave a flat 17.6 on every coalesced frame. Landed as
`mobile-eggbert-reference/tools/make-gif.sh` (a real script this time, not an ad hoc `convert`
invocation, so the same mistake can't silently recur across 129 regenerations).

**`DOC-101` (`tile-anim-lava.gif`) is also done** — regenerated with the fixed tool, and along the
way turned up + fixed the real `BlockTypes::tileUV` gap/pitch engine bug (see §3's top entry and
`plan.md`'s `S3D-2`). `DOC-102` (`tile-anim-spike.gif`, high row indices 17-18 of 22), `DOC-103`
(`tile-anim-crusher.gif`, last-column + row-wrap edge cases), `DOC-104` (`tile-anim-saw.gif`,
another row-wrap case), `DOC-105` (`tile-anim-water1.gif`, which also found + fixed the
translucent-content-vanishing bug — see this section's top entry), `DOC-106`
(`tile-anim-water2.gif` — a full-bleed uniformly-translucent tile, no bug, just a test-methodology
false alarm, see `plan.md`), `DOC-107` (`tile-anim-temp.gif` — 18 real icons + 2 genuinely blank
`-1`-sentinel frames, all clean), `DOC-108` (`tile-anim-marine.gif`), `DOC-109`
(`tile-anim-fanleft.gif`), `DOC-110` (`tile-anim-fanright.gif`), `DOC-111`
(`tile-anim-fanup.gif`), and `DOC-112` (`tile-anim-fandown.gif`) are done — **all 12 tile
animations (`DOC-101`-`DOC-112`) are now regenerated and verified.** `DOC-113`
(`blupi-action-01-stop.gif`, 330 real frames) is also done — landed a new tool,
`mobile-eggbert-reference/tools/extract-blupi-action.py`, which reads mobile-eggbert's
`table_blupi` live (not copied into galaxy-eggbert) to get each Blupi action's real frame list;
see `plan.md`'s `DOC-113` for the full writeup. `DOC-114` (`blupi-action-02-march.gif` — the exact
GIF that first surfaced the `DOC-100` ghosting bug report — is confirmed fixed, alpha now
fluctuates instead of climbing). **Running tally (full per-task detail lives in `plan.md` §16,
not repeated here): `DOC-100`–`DOC-141` done** (root cause + translucency fix + `tileUV` engine
bug + all 12 tile animations + 29 Blupi actions, one of which -- action 12/Set -- has no
table_blupi entry and needs no GIF). 88 of the 129 GIFs (`DOC-142`–`DOC-229`)
still need regenerating — next is `DOC-142` (`blupi-action-31-stopsuspend.gif`).

**Engine/code track: no blocker.** Real, textured terrain with working animated tiles renders end-to-end from the
actual loaded world file — `GEWorldRuntime` → `GETerrainRenderer` → `Easy3D::CubeMesh`/
`CubeMeshRenderer` + `Texture2D` → visible, textured pixels on screen, verified by geometry counts
and by on-screen color sampling. The remaining gaps (Blupi, objects, HUD, sound) are new
capabilities to build, not bugs to fix.

**Direction question resolved (2026-07-03):** the user flagged that loading mobile-eggbert's
*flat 2D* `.txt` world files as the primary/only data source for `GalaxyEggbertCNA` was the wrong
end-state — mobile-eggbert's 2D layouts should stay reference/inspiration only, and galaxy-eggbert
needed at least one genuinely 3D, hand-authored `.vwr` world as the actual playable-world source.
This does not contradict the existing "no automated `.txt`→`.vwr` converter" rule (§1) — it was
the natural next step of it. Resolved in full (E3D-MIG-058, §3):
`worlds3d/world001.vwr` (2749 blocks, Y range [0, 13]) is now `GalaxyEggbertCNA`'s default loaded
world; `LoadFromMobileEggbertFile` stays available as a secondary/reference path;
`GETerrainRenderer` now walks all Y layers (previously Y=0 only — a real gap found and fixed along
the way). Verified with a real run showing textured terrain across the full structure.

Known follow-up, not blocking: no face-culling/occlusion in `GETerrainRenderer` — fine at 2749
blocks, will matter for denser/taller hand-authored worlds later.

**Not committed yet:** the animated-tile work and the hand-authored-3D-world work (both described
at the top of §3) — only the older `a636649`/`b4c52c0` batch is committed and pushed so far. A
future session should re-check `git status`/`git log` in both repos first before assuming any of
this is current, since this environment has shown other concurrent sessions landing changes in
sibling repos (e.g. the `../cna` fix noted above).

## 5. Known bugs and limitations

| Status | Issue |
|---|---|
| confirmed | Simple3D: `GECameraRig::StartShake()` is a no-op (Simple3D has no camera-offset API) |
| confirmed, environment-specific | `ctest` does not discover `GalaxyEggbertWorldsTests` when configured in the pre-existing `cmake-build-debug` CLion profile (binary runs fine manually). Not reproduced in a fresh `build/` directory — `ctest --test-dir build` correctly finds and runs all 54 tests there. Likely a stale/IDE-specific config issue in `cmake-build-debug`, not a general CMake problem. |
| incomplete | Simple3D: no per-zone fog, only `SetClearColor` per sky region |
| incomplete | `GalaxyEggbertCNA`: no Blupi/object rendering, no HUD, no sound, no gameplay (expected at this phase, not a bug) |
| unknown | Simple3D Android/Web builds untested since the last engine change |
| unknown | `GalaxyEggbertCNA`'s clean-exit-on-window-close path was not separately exercised — only a forced external `timeout`/kill was tested during verification |
| needs verification | Simple3D: stomp bounce height (`kJumpSpeed * 0.65f`) — does it match mobile-eggbert's feel? |
| needs verification | Simple3D: crate push floor-support check only tested at y=0; stacked crates (y=1) untested |
| risky assumption | `GalaxyEggbertCNA`'s world loader uses a relative path (`"worlds3d/world001.vwr"`, `"Content/icons/object-m.png"`) — only works if the binary is run from its own build directory; fails silently (world) or presumably throws (texture) otherwise |
| incomplete | `GETerrainRenderer` (CNA) has no face-culling/occlusion — draws one full cube per non-air block regardless of neighbors. Fine at the current sample world's size (2749 blocks); will need revisiting for denser/taller hand-authored worlds |
| confirmed, documentation only, tool now fixed | 88 of 129 animated GIFs in `mobile-eggbert-reference/images/` still ghost/accumulate previous frames instead of clearing (confirmed via alpha-channel analysis on coalesced frames — see §4). Root cause fixed (`DOC-100`) plus a second tool fix for translucent content vanishing (`DOC-105`); `DOC-100`–`DOC-141` regenerated and verified so far (see `plan.md` §16 for per-task detail) — the other 88 (`DOC-142`–`DOC-229`) still need regenerating with the fixed tool. |
| fixed (2026-07-04) | ~~`BlockTypes::tileUV()` assumed a flat 64px grid in `object-m.png`, missing the sheet's real 1px inter-tile gap (65px pitch) — bled neighboring icons in by later rows/columns~~. Fixed in both `GalaxyEggbertSimple3D` and `GalaxyEggbertCNA` — see §3's top entry and `plan.md`'s `S3D-2`. |

## 6. Architecture notes

### Main modules
```
include/GalaxyEggbert/Worlds/, src/GalaxyEggbert/Worlds/   — engine-agnostic voxel World (100×100
                                                               grid), Block/Chunk, .vwr save format.
                                                               Shared by BOTH targets below
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
                                LoadFromMobileEggbertFile() secondary/reference path),
                                Game/GETileAtlas (block type → UV rect), Game/GETerrainRenderer
                                (World, all Y layers → static + animated CubeMeshRenderer-backed
                                meshes), an Easy3D::Camera3D (aimed at the terrain's 3D block
                                centroid), a CNA Texture2D of object-m.png, and a BasicEffect
                                bound to it.

tools/GenerateSampleWorld3D.cpp — engine-agnostic CLI tool (CMake target
                                GenerateSampleWorld3D): builds a hand-authored 3D World in memory
                                and saves it via World::saveToFile(). Only depends on
                                include/GalaxyEggbert/Worlds/ — no engine deps, runs standalone.

worlds3d/                    — galaxy-eggbert's own hand-authored .vwr worlds (committed to this
                                repo, unlike mobile-eggbert's worlds/ which is copied at build
                                time). world001.vwr is the first one (E3D-MIG-058).

../easy-3d/                  — companion library beside CNA (not touched by default; two
                                approved edits this migration). Camera3D/OrbitCamera/FollowCamera,
                                TextureAtlas, BillboardBatch/CubeBatch/DebugDraw (CPU-side item
                                queues), CubeMesh (vertex/index builder), CubeMeshRenderer (CNA
                                draw-call adapter). Billboard/DebugDraw builders+adapters: not
                                started.
```

### Data flow
```
Hand-authored 3D worlds (worlds3d/*.vwr, engine-agnostic binary format):
  CNA (default): → GEWorldRuntime::LoadFromVwrFile() → World::loadFromFile() → World (all Y
                   layers) → GETerrainRenderer → Easy3D::CubeBatch/CubeMesh/CubeMeshRenderer +
                   Texture2D(object-m.png) (renders real 3D, textured, animated terrain; no
                   Blupi/object rendering yet).

Mobile-eggbert worlds (worlds/worldXXX.txt, header + Decor: grid [+ MoveObject: lines], flat Y=0):
  Simple3D:      → GEWorldRuntime::LoadFromMobileEggbertFile() → World + MobileObjSpec list +
                   blupiSpawn → GETerrainRenderer / GEDecorSystem / GEBlupiController (renders
                   everything; this is Simple3D's only/default world source).
  CNA (secondary/reference path only, not the default anymore):
                 → GEWorldRuntime::LoadFromMobileEggbertFile() → World (MoveObject: lines still
                   ignored — that's a later phase) → same GETerrainRenderer path as above.
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
- `GEDecorSystem::GetObjIcon(ObjectType, phase)` (Simple3D) — do not change without
  cross-referencing mobile-eggbert's `Decor.cpp`.
- `src/GalaxyEggbertSimple3D/` must not be mutated into the CNA implementation — new CNA/Easy3D
  code goes in `src/GalaxyEggbertCNA/` only.
- mobile-eggbert stays read-only; no code/data copied from it without explicit user approval.
- `../easy-3d`, `../cna`, and `../simple-3d` are sibling repos: read freely when needed, modify
  only with explicit, per-change user approval.
- Easy3D must not hide CNA (its APIs use CNA/XNA types directly) and must not grow into a scene
  graph / ECS / engine — new helpers stay small and generic.

## 7. Useful commands

```bash
# Configure + build Simple3D (default target):
cmake -S . -B build
cmake --build build --target GalaxyEggbertSimple3D -j2
./build/GalaxyEggbertSimple3D

# Build + run world-model unit tests:
cmake --build build --target GalaxyEggbertWorldsTests -j2
ctest --test-dir build --output-on-failure

# Configure + build the new CNA target (opt-in, off by default):
cmake -S . -B build-cna -DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF
cmake --build build-cna --target GalaxyEggbertCNA -j2

# Run CNA target — must run from its own build directory (relative asset paths):
cd build-cna && ./GalaxyEggbertCNA
# Expect on stdout, in order:
#   "GalaxyEggbertCNA: loaded worlds3d/world001.vwr — 2749 non-air blocks, Y range [0, 13]."
#   GETileAtlas UV diagnostics (Ground/Lava/Wall)
#   "GalaxyEggbertCNA: terrain texture loaded — 1301x1431 px."
#   "GalaxyEggbertCNA: terrain mesh uploaded — 2749 blocks (0 animated), 65976 vertices, 32988 triangles."
#   (this sample world has no animated tiles; run GalaxyEggbertCNA on a mobile-eggbert-derived
#   .txt world via GEWorldRuntime::LoadFromMobileEggbertFile for a non-zero animated count, e.g.
#   mobile-eggbert's world024.txt has 137)
#   "GalaxyEggbertCNA: terrain visibility check — N/25 sampled screen points show non-background (terrain) color, M distinct color(s) among them..."
# A window opens showing textured cube terrain (object-m.png tiles) from an angled overhead view.

# Regenerate the hand-authored 3D sample world (if tools/GenerateSampleWorld3D.cpp changes):
cmake --build build-cna --target GenerateSampleWorld3D -j2
./build-cna/GenerateSampleWorld3D worlds3d/world001.vwr

# easy-3d: default (headers-only) build + tests
cmake -S ../easy-3d -B /tmp/e3d-build -DEASY3D_CNA_DIR=../cna
cmake --build /tmp/e3d-build -j2
ctest --test-dir /tmp/e3d-build --output-on-failure

# easy-3d: CNA-linked build + tests (heavier — builds CNA/SHARP_RUNTIME/EasyGL too)
cmake -S ../easy-3d -B /tmp/e3d-build-cna -DEASY3D_LINK_CNA=ON -DEASY3D_CNA_BACKEND=EASY_GL -DEASY3D_CNA_DIR=../cna
cmake --build /tmp/e3d-build-cna -j2
ctest --test-dir /tmp/e3d-build-cna --output-on-failure

# Reference: mobile-eggbert animation tables / gameplay logic (read-only)
grep -n "table_decor\|table_blupi" ../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Tables.cpp
less ../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Decor.cpp
```

No `.clang-format`/`.clang-tidy` config exists in this repo — no lint/format tooling to run.

## 8. Next smallest tasks

1. **Work through the ~168-task `mobile-eggbert-reference/` rework list (in progress, reopened
   2026-07-03 — see §4 and `plan.md` §16 for the full story).** `DOC-100`–`DOC-141` are done: the
   GIF-ghosting root-cause fix, a second fix for translucent content vanishing (`DOC-105`), the
   real `BlockTypes::tileUV` gap/pitch engine bug (found via `DOC-101`, see §3 and `plan.md`'s
   `S3D-2`), all 12 tile animations, and 29 of 84 Blupi actions (also landed a reusable
   tool, `mobile-eggbert-reference/tools/extract-blupi-action.py`, reading mobile-eggbert's
   `table_blupi` live rather than copying it — see `plan.md`'s `DOC-113`). Full per-task detail is
   in `plan.md` §16, not repeated here. Next: `DOC-142` (`blupi-action-31-stopsuspend.gif`), then continue
   through `DOC-143`–`DOC-267` (87 more GIF regenerations, then static-icon re-verification, then
   `DOC-005`/`DOC-006` sounds/backgrounds which were never started). Read-only research against
   `../mobile-eggbert` plus local image/GIF tooling work — the `BlockTypes.hpp`/`GETileAtlas.cpp`
   engine fix already landed this session; no further galaxy-eggbert C++ code changes expected for
   the remaining GIF-regeneration tasks themselves.
2. **Chunk-radius world streaming (E3D-MIG-057, now scheduled)** — implement loading/rendering
   only the current + neighboring chunks, once real (denser, more 3D) hand-authored worlds exist.
   Natural co-requisite with face-culling below.
3. **Expand `worlds3d/world001.vwr`, or author more `.vwr` worlds** — the current sample is a
   proof-of-concept (staircase + one room). A natural next step is a more level-like, denser,
   genuinely 3D design (multiple rooms/levels, hazard tiles at various Y) — see task 1, this
   should follow the mapping doc, not precede it.
4. **Add face-culling/occlusion to `GETerrainRenderer`** — needed once worlds get denser (see
   task 2/3); not needed at the current ~2700-block scale.
5. **Render Blupi as a billboard (E3D-MIG-061..063)** — CPU-side vertex builder +CNA renderer
   adapter for `Easy3D::BillboardBatch`, then draw `blupi.png` at `GEBlupiController`'s position.
6. **Fix `ctest` discovery in the `cmake-build-debug` profile** — investigate why
   `gtest_discover_tests` doesn't find `GalaxyEggbertWorldsTests` there (works fine in a fresh
   `build/` dir). **Files:** `CMakeLists.txt`, `cmake-build-debug/` config.
   **Verification:** `ctest --test-dir cmake-build-debug -R GalaxyEggbert` reports 54 passed.
7. **Verify `GalaxyEggbertCNA`'s clean-exit path** — close the window via the window manager
   (not a forced kill) and confirm the process exits 0 with no leaked resources.
   **Files:** none expected — diagnostic verification only, possibly add an `OnExiting` log line
   to `GalaxyEggbertCnaGame` if useful. **Verification:** manual run + exit code check.
8. **Simple3D camera shake** — make `GECameraRig::StartShake()` produce visible jitter on
   death/hazard hit, matching `DecorAction::SmallShake` in mobile-eggbert. **Files:**
   `src/GalaxyEggbertSimple3D/Game/GECameraRig.cpp/hpp`; may need a new
   `Game::SetCameraPositionOffset()`-style API added to `../simple-3d` (would need discussion,
   since `simple-3d` is a sibling repo). **Verification:** trigger a death in-game; camera
   visibly shakes briefly.

## 9. Do not do yet

- No further investment in Simple3D/U3D/Nova3D beyond bug fixes on the existing reference target
  — that direction is superseded.
- No modifications to `../mobile-eggbert`, `../cna`, `../easy-3d`, or `../simple-3d` without
  explicit user approval for that specific change — none of the approvals granted so far are
  blanket authorization for further edits.
- No `.txt → .vwr` (or any) automated 2D-to-3D world converter — rejected, see §1.
- No Easy3D scope creep — no ECS, scene graph, physics, resource cache, editor, or Lua.
- No mass refactor of `include/GalaxyEggbert/Worlds/` unless a failing unit test justifies it.
- No `#ifdef GE_ENGINE_*` anywhere.
- No new gameplay mechanics not present in mobile-eggbert (no coins, coyote time, combo
  multipliers, star ratings, time bonuses).
- Committing after each finished task is now standing user instruction (2026-07-04, see §3) — one
  commit per `DOC-1xx`/task, not batched. **Pushing** to `origin/develop` is still NOT standing
  authorization — only push on explicit request each time.

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
