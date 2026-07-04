# Galaxy Eggbert — Comprehensive Feature Plan

3D faithful remake of **mobile-eggbert** (C++ port of *Speedy Blupi*, Windows Phone XNA 2013).

**Faithful remake rule:** Implement only what exists in mobile-eggbert.
3D-specific adaptations (camera, billboard sprites, blob shadow, auto step-up) are allowed.

**Inspiration rule:** Be inspired by Decor.cpp logic; do not port it verbatim.

**Rendering:** Simple3D API → U3D/Urho3D. Animations via billboard sprites from same PNGs as mobile-eggbert.

> **Direction update (2026-07-01):** The Simple3D → U3D/Nova3D engine direction described above
> and throughout sections 1–14 below is **superseded** as the long-term target. The new target
> direction is **direct CNA + Easy3D**, with mobile-eggbert used read-only as reference/asset
> source. See `easy3d.md` for the full analysis and the **"Direct CNA + Easy3D Migration"**
> section below for the task list. Sections 1–14 remain valid as historical/current reference for
> `GalaxyEggbertSimple3D`, which is **not** being deleted — it stays as the working faithful-remake
> reference until the CNA/Easy3D target reaches parity.

**Mission numbering:** 1=intro hub, 10=world1 hub, 11-19=world1 levels, 20=world2 hub, 21-29=world2 levels, … (78 worlds total in mobile-eggbert).

Legend: `[x]` done · `[ ]` todo · `[~]` partial / revision needed

---

## Direct CNA + Easy3D Migration

**Superseding direction (2026-07-01).** Full analysis: `easy3d.md`. This section is the task
list; `easy3d.md` has the reasoning, evidence, and open questions behind each task.

Target architecture: `Galaxy Eggbert → CNA directly`, with `Easy3D` used beside CNA for small
reusable helpers (cameras, texture atlas, billboard/cube batching, debug draw). Easy3D does not
hide CNA. `mobile-eggbert` is read-only reference/asset source — no code or data is copied from
it without explicit user approval, task by task.

The old Simple3D/U3D/Nova3D direction (sections 1–14 above) is superseded but **not discarded**:
`GalaxyEggbertSimple3D` stays intact and buildable as historical reference and as the current
faithful-remake gameplay checklist until the new target reaches parity.

Legend: `[x]` done · `[ ]` todo · `[~]` partial · `[?]` requires user decision before starting

### Phase 0 — Documentation and direction lock

- [x] E3D-MIG-000 — Create `easy3d.md` analysis document.
- [x] E3D-MIG-001 — Mark Simple3D/Nova3D direction as superseded by Direct CNA + Easy3D (this section + note above).
- [x] E3D-MIG-002 — Document Mobile Eggbert as read-only for Galaxy migration.
- [x] E3D-MIG-003 — Document that Easy3D is a CNA helper and must not hide CNA.
- [x] E3D-MIG-004 — Document no Lua in first migration phase.
- [x] E3D-MIG-005 — Reconciled: `README.md`, `CLAUDE.md`, and `NEXT.md` now consistently describe `GalaxyEggbertSimple3D` as current/working and `GalaxyEggbertCNA` as planned/not-yet-implemented (documentation cleanup pass, 2026-07-01). `README.md`'s CNA build instructions were not rolled back — they were relabeled as "planned" and retargeted to `GalaxyEggbertCNA` instead of implying `GalaxyEggbert` builds today.

### Next implementation batch — CNA target skeleton

**Done (2026-07-01).** This was the immediate next implementation task after the
documentation-cleanup pass (tracked in `NEXT.md` §0/§0a for full build commands and verification
results). It was a focused pull of the skeleton-only items already listed in Phase 2 below
(`E3D-MIG-020..026`), called out here separately so the very next actionable step was unambiguous.
No gameplay, no asset pipeline, no renderer beyond the minimum — just a window that clears to a
solid color.

- [x] E3D-MIG-NEXT-001 — Add `GALAXY_EGGBERT_BUILD_CNA` option, default OFF.
- [x] E3D-MIG-NEXT-002 — Add `src/GalaxyEggbertCNA/` skeleton.
- [x] E3D-MIG-NEXT-003 — Add `GalaxyEggbertCNA` executable target linking CNA and easy3d.
- [x] E3D-MIG-NEXT-004 — Minimal CNA game/window/clear-color loop.
- [x] E3D-MIG-NEXT-005 — Verify `GalaxyEggbertSimple3D` still builds unchanged.
- [x] E3D-MIG-NEXT-006 — Verify `GalaxyEggbertWorldsTests` still pass.

### Phase 1 — Repository integration investigation

- [x] E3D-MIG-010 — Inspect CNA CMake target and include/link requirements (via mobile-eggbert's usage and Easy3D's `EASY3D_CNA_DIR`/`EASY3D_LINK_CNA` options).
- [x] E3D-MIG-011 — Inspect Easy3D CMake target and include/link requirements (target `easy3d`, alias `easy3d::easy3d`, auto-detects parent `CNA` target).
- [x] E3D-MIG-012 — Inspect whether Mobile Eggbert exposes a reusable library target — confirmed **no**, only `add_executable(WindowsPhoneSpeedyBlupi ...)`.
- [~] E3D-MIG-013 — Asset path strategy **decided, not yet implemented**: first implementation will use a **build-time copy** from `../mobile-eggbert/Content` and `../mobile-eggbert/worlds` into the Galaxy Eggbert build/runtime output (reproducible builds, no dependency on a sibling checkout existing at runtime). A later, optional convenience mode may add sibling-path runtime read for local development. Symlinks were considered and rejected as fragile on Windows/CI. **The copy mechanism itself (CMake custom command, etc.) is future work — not implemented in this task; see Phase 3.**
- [x] E3D-MIG-014 — Decided: source tree `src/GalaxyEggbertCNA/`, target `GalaxyEggbertCNA`, build option `GALAXY_EGGBERT_BUILD_CNA` (default OFF). Recorded consistently in `easy3d.md` §9, `README.md`, `CLAUDE.md`, and `NEXT.md`.
- [ ] E3D-MIG-015 — `[?]` Decide whether to ask mobile-eggbert maintainers (i.e. request user approval) for a future `add_library()` target covering `Tables`/`Def`/`GameData`/`ObjectType`/`SoundChannel` only (`easy3d.md` §12 Q2).

### Phase 2 — New target skeleton

**Done (2026-07-01).** Full verification and build commands recorded in `NEXT.md` §0a.

- [x] E3D-MIG-020 — Add new CMake option (default OFF initially) to build the new target, e.g. `GALAXY_EGGBERT_BUILD_CNA`.
- [x] E3D-MIG-021 — `add_subdirectory(../cna)` then `add_subdirectory(../easy-3d)` from galaxy-eggbert's `CMakeLists.txt`. (Confirmed this order matters: Easy3D auto-detects and links the parent-provided `CNA` target instead of building its own copy.)
- [x] E3D-MIG-022 — Create `src/GalaxyEggbertCNA/` tree (skeleton: `main.cpp`, `GalaxyEggbertCnaGame.hpp/.cpp`, namespace `GalaxyEggbert::CNA`).
- [x] E3D-MIG-023 — Add `add_executable(GalaxyEggbertCNA ...)` linking `CNA` and `easy3d` (via the `easy3d::easy3d` alias).
- [x] E3D-MIG-024 — Minimal CNA `Game` subclass: open a window, run the loop, clear to a solid color. No gameplay. Verified by a 3-second smoke run (window created, EasyGL/OpenGL ES 3.2 backend initialized, no crash).
- [x] E3D-MIG-025 — Confirm `GalaxyEggbertSimple3D` target and `GALAXY_EGGBERT_BUILD_SIMPLE3D` option still build unaffected. Confirmed: default `build/` config (option untouched/OFF) builds `GalaxyEggbertSimple3D` clean, source unchanged.
- [x] E3D-MIG-026 — Confirm `GalaxyEggbertWorldsTests` still builds and 54/54 tests pass unaffected. Confirmed via `ctest --test-dir build --output-on-failure`.

### Phase 3 — Asset path and Mobile Eggbert reuse

**Done (2026-07-01).** Full verification recorded in `NEXT.md` §0b. mobile-eggbert was only read
from, never modified.

- [x] E3D-MIG-030 — Implemented the asset path strategy decided in E3D-MIG-013: `GalaxyEggbertCNA`'s
  `CMakeLists.txt` block now copies `../mobile-eggbert/Content/` and `../mobile-eggbert/worlds/`
  next to the built binary via a `POST_BUILD` `copy_directory` command (new `MOBILE_EGGBERT_HOME`
  cache variable, defaults to `../mobile-eggbert`, mirroring the existing `CNA_HOME`/`EASY3D_HOME`
  pattern). The copy destination is inside the git-ignored `build-cna/` output directory — nothing
  from mobile-eggbert is committed into this repository. Missing source directories produce a
  `WARNING`, not a hard failure (the code skeleton still builds without assets present).
- [x] E3D-MIG-031 — Reuse `Content/icons/blupi.png`. Copied wholesale with the rest of `Content/icons/` (verified present).
- [x] E3D-MIG-032 — Reuse `Content/icons/object-m.png`. Copied wholesale (verified present).
- [x] E3D-MIG-033 — Reuse `Content/icons/element.png`. Copied wholesale (verified present).
- [x] E3D-MIG-034 — Reuse `Content/icons/pad.png`. Copied wholesale (verified present).
- [x] E3D-MIG-035 — Reuse `Content/icons/jauge.png`. Copied wholesale (verified present).
- [x] E3D-MIG-036 — Reuse `Content/icons/explo.png`. Copied wholesale (verified present).
- [x] E3D-MIG-037 — Reuse `Content/sounds/soundNNN.wav` (93 files). Verified: 93/93 present after build.
- [x] E3D-MIG-038 — Reuse `worlds/worldXXX.txt` (78 files). Verified: 78/78 present after build, `world001.txt` byte-identical to mobile-eggbert's copy (`diff -q`).
- [x] E3D-MIG-039 — Cross-checked galaxy-eggbert's `include/GalaxyEggbert/def/ObjectType.hpp` and
  `SoundChannel.hpp` against mobile-eggbert's `decor/ObjectType.hpp` and `def/SoundChannel.hpp`.
  **Result: exact ID parity, no mismatches.** `ObjectType`: both sides declare the identical set of
  204 numeric IDs (0–203, with the same internal gaps). `SoundChannel`: both sides declare 0–92
  identically. Verified programmatically (extracted and diffed the full numeric ID sets, not just
  spot-checked) — not just visual inspection. **No action needed; nothing was changed in either
  repository.**

Note: only `Content/` and `worlds/` are made available on disk next to `GalaxyEggbertCNA`.
Nothing in `GalaxyEggbertCNA` code loads, parses, or renders any of these files yet — that starts
in Phase 4 (world loading) and Phase 5+ (terrain/Blupi rendering).

### Phase 4 — World loading

**Done (2026-07-01).** Full verification recorded in `NEXT.md` §0c.

- [x] E3D-MIG-040 — Confirmed `GalaxyEggbert::Worlds` (the engine-agnostic `include/GalaxyEggbert/Worlds/` + `src/GalaxyEggbert/Worlds/` tree) links into `GalaxyEggbertCNA` unchanged. `GE_SHARED_SOURCES` was hoisted from inside the `GALAXY_EGGBERT_BUILD_SIMPLE3D` block to top-level `CMakeLists.txt` scope so both targets reuse the same source list; `GalaxyEggbertSimple3D` re-verified unaffected after the move (54/54 `GalaxyEggbertWorldsTests` still pass).
- [x] E3D-MIG-041 — `GalaxyEggbertCnaGame::LoadContent()` loads `worlds/world001.txt` end-to-end (parse only — nothing renders it). Verified by running the binary: `spawn tile (12, 92), sky region 0, 594 non-air blocks`.
- [x] E3D-MIG-042 — Cross-checked parsed grid dimensions/tile codes. **Caveat, stated honestly:** this was cross-checked against an independent from-scratch Python re-implementation of the parser (spawn tile, sky region, raw grid cell count, and the `BlockTypes::kPassable` filtering), not a live re-run of the `GalaxyEggbertSimple3D` binary — modifying Simple3D to add diagnostic output was out of scope (hard rule: do not modify `src/GalaxyEggbertSimple3D/`), and Simple3D's existing `GEWorldRuntime::LoadFromMobileEggbertFile()` uses the identical shared `World`/`BlockTypes` code already, so a second C++ run would not be a truly independent check. The Python script mechanically extracts the `kPassable[441]` table from `BlockTypes.hpp` (not hand-transcribed) and reimplements the header + grid parsing with different control flow, run against the real `build-cna/worlds/world001.txt`. Result: **exact match** — spawn tile `(12, 92)`, sky region `0`, `594` non-air blocks (out of `612` raw non-zero Decor cells before transparency filtering) on both sides.
- [x] E3D-MIG-043 — Minimal `GEWorldRuntime` for CNA: `src/GalaxyEggbertCNA/Game/GEWorldRuntime.hpp/.cpp`, namespace `GalaxyEggbert::CNA`. Modeled on (header/region parsing shape, `Decor:`/`MoveObject:` line handling, `BlockTypes::fromMobileIconId` + `World::setBlock` usage) but not copied from `GESimple3D::GEWorldRuntime` — independently written, smaller (no `MobileObjSpec`/object parsing, that's Phase 7; no Simple3D types).

### Phase 5 — Easy3D terrain path

- [x] E3D-MIG-050 — Decided (2026-07-02): both the CPU-side vertex-builder gap (Easy3D's own
  Phase 3) and the CNA renderer-adapter gap (Phase 4) get filled **inside `../easy-3d` itself**,
  not as a `GalaxyEggbertCNA`-local adapter. Matches easy-3d's own `docs/ROADMAP.md` Phase 3/4 and
  `NEXT.md` §8 item 8, which already scope this in detail (proposed files
  `include/Easy3D/BillboardMesh.hpp`, `CubeMesh.hpp`). Reasoning: turning a `BillboardItem`/
  `CubeItem`/`DebugLine`/`DebugBox` into vertex/index arrays, and turning those into CNA
  `GraphicsDevice` draw calls, is generic 3D-batching plumbing with zero Eggbert-specific knowledge
  (no `ObjectType`, no tile IDs, no gameplay) — exactly the "boring, testable... billboard/cube
  batching, texture atlas, debug drawing" helper role `easy-3d/docs/ARCHITECTURE.md` assigns to
  Easy3D, and reusable beyond this one game. Everything Eggbert-specific (which UV rect maps to
  which `ObjectType`/tile ID, which world cell gets a `CubeItem`, animated-tile frame selection)
  stays in `GalaxyEggbertCNA`, which only calls Easy3D's new vertex-builder/adapter functions with
  that data — mirroring the existing `TextureAtlas` split (Easy3D stores UV rects by name; Galaxy
  Eggbert decides what name means what tile). **Caveat:** this records *where the code should
  live*, not a green light to start editing `../easy-3d` — implementing E3D-MIG-051/052 modifies a
  sibling repo, which `NEXT.md`'s "Do not do yet" list requires explicit user approval for before
  any code is written there.
- [x] E3D-MIG-051 — Done (2026-07-02, user approved). Implemented `Easy3D::CubeVertex` +
  `AppendCubeMesh`/`BuildCubeMesh` **inside `../easy-3d`**
  (`include/Easy3D/CubeMesh.hpp`/`src/CubeMesh.cpp`) per E3D-MIG-050: turns `CubeBatch` items into
  24 vertices + 36 indices per cube (one UV per face — 4 vertices per face rather than 8 shared
  corners, since a shared corner can't hold 3 different per-face UVs), CCW-wound as seen from
  outside. New CNA-link-gated `tests/test_cube_mesh.cpp` in easy-3d. Verified: easy-3d default
  build 2/2 tests, CNA-linked build 5/5 tests; `GalaxyEggbertCNA` still builds clean against the
  updated `easy3d`; `GalaxyEggbertWorldsTests` still 54/54 (unaffected, unrelated tree). Full
  detail recorded in `../easy-3d/NEXT.md` §3/§8 item 8.
- [x] E3D-MIG-052 — Done (2026-07-02, user approved). CNA draw-path decision: real, working,
  tested indexed-3D drawing already exists in CNA (`VertexBuffer`+`IndexBuffer`+`BasicEffect`+
  `GraphicsDevice::DrawIndexedPrimitives`), proven by CNA's own `examples/house3d_demo.cpp` — build
  GPU buffers once, `SetVertexBuffer`/`Indices`/`DrawIndexedPrimitives` per frame inside a
  `pass.Apply()` loop. `SpriteBatch` is 2D-only, not usable. New `Easy3D::CubeMeshRenderer`
  (`include/Easy3D/CubeMeshRenderer.hpp` + `src/CubeMeshRenderer.cpp` in `../easy-3d`): constructor
  uploads a `CubeMesh`'s `CubeVertex`/index arrays to GPU (converting `CubeVertex{Position,Uv}` to
  CNA's `VertexPositionTexture{Position,TextureCoordinate}` — an exact 1:1 field match), 32-bit
  `IndexBuffer` (matches `CubeMesh`'s `uint32_t` indices); `Draw(GraphicsDevice&, BasicEffect&)`
  issues the indexed draw call, following `house3d_demo.cpp`'s exact pattern. The caller owns and
  configures the `BasicEffect` (World/View/Projection/Texture) — the adapter knows nothing about
  tiles, gameplay, or cameras, matching E3D-MIG-050's split. Found and fixed a real bug while
  wiring this up: `../easy-3d`'s "headers-only" default build (no CNA linked) didn't have
  `../sharp-runtime/include` on its path, so anything pulling `GraphicsDevice.hpp`/
  `BasicEffect.hpp` (which need `Color.hpp` → `SharpRuntime/SharpRuntimeHelper.hpp`) failed to
  compile there — unlike the shallow `Vector2`/`Vector3`/`Matrix` headers `Camera3D`/`CubeBatch`
  already used. Added a `EASY3D_SHARP_RUNTIME_DIR` cache variable (mirrors the existing
  `EASY3D_CNA_DIR` pattern) to `../easy-3d/CMakeLists.txt`, fixing the default build without
  affecting the CNA-linked path (which already gets it transitively through the `CNA` target).
  New `tests/test_cube_mesh_renderer.cpp` in `../easy-3d` — **compile-check only, always** (not
  gated by `EASY3D_CNA_LINKED` like the other tests), since `CubeMeshRenderer`'s constructor needs
  a live `GraphicsDevice&`, which only exists once a real CNA `Game` has opened a window — not
  something a plain `main()` can produce. **Genuine runtime verification instead came from the real
  target:** wired a temporary debug cube (one hardcoded `CubeItem`, `GETileAtlas`'s Ground UV) into
  `GalaxyEggbertCnaGame` (galaxy-eggbert), ran the actual windowed binary, and read back the
  center-screen pixel via `GraphicsDevice::GetBackBufferData` (mirroring CNA's own
  `easygl_vertex_formats_test.cpp` pattern) — printed `debug cube center pixel RGBA = (255, 255,
  255, 255)`, clearly distinct from the sky-blue clear color, confirming a real `DrawIndexedPrimitives`
  call actually rasterized pixels. Verified: `../easy-3d` default build (2/2 tests, plus the new
  compile-check) and CNA-linked build (5/5 tests, unaffected) both green;
  `GalaxyEggbertCNA` rebuilt clean and the debug-cube pixel readback above confirms it end-to-end.
  This debug cube is temporary scaffolding (labeled as such in code) — E3D-MIG-054 replaces it with
  real per-world-cell terrain.
- [x] E3D-MIG-053 — Done (2026-07-02). New `GalaxyEggbert::CNA::GETileAtlas`
  (`src/GalaxyEggbertCNA/Game/GETileAtlas.hpp/.cpp`): builds an `Easy3D::TextureAtlas` sized to
  `BlockTypes::kSheetW`/`kSheetH` and registers the full `object-m.png` grid via `AddGrid` (rows
  computed as `ceil(kSheetH / kTileSize)`); `GetTileUv(blockType)` returns the UV rect for a block
  type, exploiting that `AddGrid`'s row-major frame numbering is mathematically identical to
  `BlockTypes`' own `icon = row * kSheetCols + col` convention, so frame index == block type
  directly (no separate mapping table). Wired into `GalaxyEggbertCnaGame::LoadContent()`
  (prints UV for Ground/Lava/Wall to stdout, data-only — nothing queues a `CubeBatch` item yet).
  **Verification note:** `GalaxyEggbertCNA`'s full CNA-linked build is currently broken by an
  unrelated, external issue — `../sharp-runtime`'s `IAsyncResult` interface was extended
  (uncommitted change in that repo, not mine) and `../cna`'s `StorageDevice` hasn't been updated to
  match, so the whole `CNA` static library fails to compile. This is outside this task's scope
  (neither `../cna` nor `../sharp-runtime` is approved for modification here) and appears to be a
  concurrent, in-progress change by another session. Because `GETileAtlas` has zero functional
  dependency on CNA (only touches CNA-free `Easy3D::TextureAtlas` + header-only `BlockTypes`),
  it was verified independently instead: a standalone compile+link (bypassing the broken parts of
  CNA entirely) confirmed `GetTileUv()` for Air/negative/out-of-range → all-zero, named tile types
  match independently recomputed UVs, row-wrap and distinct-icon behavior correct. Separately,
  `-fsyntax-only` confirmed `GalaxyEggbertCnaGame.cpp`'s actual CNA/easy-3d usage compiles cleanly
  against the real headers. **Update (2026-07-02, later same day):** `../cna` shipped
  `e1939bc` ("fix(StorageDevice): implement IAsyncResult's new AsyncState/AsyncWaitHandle"),
  resolving the external breakage. Rebuilt `GalaxyEggbertCNA` clean and ran it: printed UV values
  for Ground/Lava/Wall match the standalone independent verification exactly, and the world-load
  line (spawn tile (12, 92), sky region 0, 594 non-air blocks) is unregressed. Full build+run
  re-verification is done — no longer owed.
- [x] E3D-MIG-054 — Done (2026-07-02). New `GalaxyEggbert::CNA::GETerrainRenderer`
  (`src/GalaxyEggbertCNA/Game/GETerrainRenderer.hpp/.cpp`): walks the full 100x100 `World` grid,
  queues one `Easy3D::CubeBatch` item (1x1x1, world position `(x - kWorldCenterX, 0, z -
  kWorldCenterZ)`, UV from `GETileAtlas`) per non-air block, builds it through
  `Easy3D::BuildCubeMesh`, and owns an `Easy3D::CubeMeshRenderer` for it — a static, one-shot
  upload (no per-frame rebuild, matching `house3d_demo.cpp`'s own pattern and this phase's
  "non-animated" scope). Also tracks the block-position centroid (`CentroidX()`/`CentroidZ()`),
  used to aim the camera reliably (the spawn tile itself is usually the open-air cell Blupi stands
  in, not a solid block, so it's not a reliable look-at target). Replaced the temporary
  `E3D-MIG-052` debug cube in `GalaxyEggbertCnaGame` entirely — camera now targets the terrain
  centroid from a high angled overhead position; `Draw()` also enables depth testing
  (`SetDepthTestEnabled(true)`, needed now that many cubes can occlude each other, matching
  `house3d_demo.cpp`). **Verified:** rebuilt `GalaxyEggbertCNA` clean and ran it —
  `terrain mesh uploaded — 594 blocks, 14256 vertices, 7128 triangles` (594×24=14256,
  594×12=7128, exact match to the already-verified non-air-block count); a 5x5 grid pixel-sampling
  check (more robust than a single center pixel, which can miss terrain if it falls between blocks)
  found `13/25 sampled screen points show non-background (terrain) color` — genuine, non-vacuous
  confirmation that real per-block terrain, not a placeholder, is on screen. **Follow-up, done same
  day (2026-07-02):** loaded `object-m.png` directly via CNA's own
  `Texture2D(assetName, GraphicsDevice&)` constructor (against the already build-time-copied
  `Content/icons/object-m.png`) and bound it to `terrainEffect_`
  (`setTextureEnabledProperty(true)`/`setTextureProperty(&terrainTexture_)`) — **not** via
  mobile-eggbert's `Pixmap` class, which the user initially suggested reusing; declined because
  (a) mobile-eggbert has no CMake library target to link at all today, and (b) `Pixmap` is
  confirmed 2D-`SpriteBatch`-coupled in `easy3d.md` §5.2, not usable for this 3D `BasicEffect`
  path — CNA's native `Texture2D` already does exactly what's needed with no mobile-eggbert
  dependency. Verified: texture loads at `1301x1431 px` (matches `BlockTypes::kSheetW`/`kSheetH`
  exactly); re-ran the pixel-sampling check — `13/25` terrain-covered points still, now with
  **5 distinct colors** among them (proof the texture is genuinely sampled per-pixel, not a flat
  fallback).
- [x] E3D-MIG-055 — Done (2026-07-03). Added animated-tile support to `GalaxyEggbertCNA`
  (lava/crusher/saw/spike/water1/water2/fan×4/marine/temp) by porting the frame tables and
  `animIcon`/`isAnimated` logic 1:1 from `GalaxyEggbertSimple3D`'s already-shipped
  `GETerrainRenderer.cpp` (same galaxy-eggbert repo; not a fresh mobile-eggbert transcription —
  those tables were already approved/committed for Simple3D). Design (user-approved): periodic
  rebuild of just the animated subset, not shader-side UV animation — `GETerrainRenderer` now
  splits blocks into a static `CubeMeshRenderer` (built once, unchanged) and an animated subset
  tracked as `(x, z, animBase)`, rebuilt into a second `CubeMeshRenderer` only when
  `GEWorldRuntime::GetAnimPhase()` changes (`GETerrainRenderer::Update()`). Added
  `GEWorldRuntime::Update(dt)`/`GetAnimPhase()` to CNA's `GEWorldRuntime`, mirroring Simple3D's 6
  fps `kAnimPeriod` exactly. `GalaxyEggbertCnaGame` gained an `Update(GameTime&)` override driving
  both. Verified two ways: (1) `worlds/world001.txt` has 0 animated tiles — ran 8s (~48 rebuild
  cycles at 6 fps) with no crash, proving the empty-animated-set path is safe; (2) swapped in
  mobile-eggbert's `world024.txt` (137 animated blocks) as a build-artifact-only substitution (not
  committed, restored after) — ran 8s with no crash, `1887 blocks (137 animated), 45288 vertices`
  uploaded, terrain visibility check still found textured pixels on screen. Guarded the one new
  edge case (all animated tiles hidden in the same phase, e.g. Temp's 2 blank frames of 20) by
  skipping `CubeMeshRenderer` construction instead of building a zero-size GPU buffer. Files:
  `src/GalaxyEggbertCNA/Game/GETerrainRenderer.hpp/.cpp`,
  `src/GalaxyEggbertCNA/Game/GEWorldRuntime.hpp/.cpp`,
  `src/GalaxyEggbertCNA/GalaxyEggbertCnaGame.hpp/.cpp`. No `../easy-3d`/`../mobile-eggbert` changes
  needed.
- [x] E3D-MIG-058 — Done (2026-07-03). User flagged that `GalaxyEggbertCNA` loading
  mobile-eggbert's flat 2D `.txt` files is not the intended end-state for galaxy-eggbert's world
  data — mobile-eggbert 2D layouts are reference/inspiration only; galaxy-eggbert needs at least
  one genuinely 3D, hand-authored `.vwr` world. Full pipeline now done, in two parts:
  1. **World generation.** Added `tools/GenerateSampleWorld3D.cpp` (new CMake target
     `GenerateSampleWorld3D`, engine-agnostic — only depends on `include/GalaxyEggbert/Worlds/`):
     builds a small structure with real Y variation (ground floor, a 10-step solid ascending
     staircase, a raised platform, a walled room with a doorway, two pillars) using the existing
     tested `World::setBlock()`/`saveToFile()` API, round-trip-verified via `World::loadFromFile()`.
     Ran it once and committed the output: `worlds3d/world001.vwr` — 2749 non-air blocks, Y range
     [0, 13].
  2. **Wiring, per user decisions (asked via `AskUserQuestion` and answered):** kept
     `GEWorldRuntime::LoadFromMobileEggbertFile` in the code as a secondary/reference path (not
     removed); added `GEWorldRuntime::LoadFromVwrFile()` (thin wrapper over
     `Worlds::World::loadFromFile()`; `.vwr` carries no spawn/sky-region header, so those reset to
     0) and made `GalaxyEggbertCnaGame::LoadContent()` call it by default instead of the
     mobile-eggbert `.txt` loader. Fixed the real prerequisite gap found while investigating:
     `GETerrainRenderer` (CNA) previously only ever read `world.getBlock(x, 0, z)` — a single Y
     layer — so it could not render a multi-Y world at all; it now walks every Y layer
     (`0..blocksPerAxis()-1`). Added a `CentroidY()` (alongside the existing X/Z) and updated the
     camera framing in `GalaxyEggbertCnaGame` to target the full 3D centroid instead of assuming
     Y=0. Added `worlds3d/` to the CMake asset-copy step for `GalaxyEggbertCNA`. **Verified with a
     real run:** `loaded worlds3d/world001.vwr — 2749 non-air blocks, Y range [0, 13]` →
     `terrain mesh uploaded — 2749 blocks (0 animated), 65976 vertices, 32988 triangles` →
     terrain visibility check found `21/25` sampled points showing textured terrain color with 10
     distinct colors; ran 8s with no crash. Known follow-up (not blocking, noted for later): no
     face-culling/occlusion — fine at this structure's size (2749 blocks) but will matter for
     denser/taller hand-authored worlds. Files: `src/GalaxyEggbertCNA/Game/GEWorldRuntime.hpp/.cpp`,
     `src/GalaxyEggbertCNA/Game/GETerrainRenderer.hpp/.cpp`,
     `src/GalaxyEggbertCNA/GalaxyEggbertCnaGame.hpp/.cpp`, `CMakeLists.txt`,
     `tools/GenerateSampleWorld3D.cpp`, `worlds3d/world001.vwr`.
- [ ] E3D-MIG-056 — Do not add MeshCraft or any mesh-import path.
- [ ] E3D-MIG-057 — **Scheduled** (status changed 2026-07-03, user override of the original "not
  scheduled" call). Chunk-radius world loading/streaming — load/render only the current +
  neighboring chunks instead of the whole `World` at once. Original 2026-07-03 reasoning for
  deferring it: `world001`-derived worlds only had ~594–2749 blocks, trivial for any GPU, and
  mobile-eggbert itself has no chunk-radius concept. **User override, same day:** that reasoning
  was based on the current placeholder sample world only — galaxy-eggbert's real hand-authored 3D
  worlds are intended to be much denser and more genuinely three-dimensional than any mobile-eggbert
  2D level (Blupi moving through real volume, not just a mostly-flat plane with a staircase), so
  chunk-radius streaming will be needed once real worlds are built, not deferred until a problem
  appears. Not yet implemented — scheduled means "on the list to actually do," not done. Natural
  prerequisite/co-requisite: `GETerrainRenderer`'s face-culling/occlusion gap (noted under
  E3D-MIG-058) — a denser world needs both.
- [x] E3D-MIG-059 — Done (2026-07-03). Fixed a real bug found while writing
  `mobile-eggbert-2d-reference.md` §2.3: neither `GEWorldRuntime` (Simple3D nor CNA) recognized the
  `BigDecor:` section of mobile-eggbert level files — its 100 rows were silently skipped because
  the row counter had already reached 100 from the main `Decor:` grid. Refactored both
  `GEWorldRuntime::LoadFromMobileEggbertFile` implementations
  (`src/GalaxyEggbertSimple3D/Game/GEWorldRuntime.hpp/.cpp`,
  `src/GalaxyEggbertCNA/Game/GEWorldRuntime.hpp/.cpp`) to track section state explicitly
  (`None`/`Decor`/`BigDecor`) instead of one shared row counter, and added a `GetBigDecor()`
  accessor (flat row-major `vector<uint16_t>`, same icon→block-type conversion as the main grid).
  Deliberately **parses and stores only** — does not render it anywhere, since how to represent
  `BigDecor` in 3D is still an open question (`mobile-eggbert-2d-reference.md` §9), not something to
  invent here. Verified with a new tool, `tools/VerifyBigDecorParsing.cpp` (added to `CMakeLists.txt`
  inside the `GALAXY_EGGBERT_BUILD_SIMPLE3D` block): confirmed `world013.txt` now parses 14 non-air
  `BigDecor` cells (previously 0) and `world001.txt` parses 2 (matches the doc's "effectively
  empty" note); also confirmed no regression in the main grid (`world001.txt` still parses exactly
  594 non-air blocks, matching the value established earlier this session). `GalaxyEggbertCNA`
  rebuilt clean; `GalaxyEggbertWorldsTests` still 54/54.

### Phase 6 — Blupi first version

- [x] E3D-MIG-060 — Done (2026-07-03). Added `GalaxyEggbert::CNA::GEBlupiController`
  (`src/GalaxyEggbertCNA/Game/GEBlupiController.hpp/.cpp`): invisible, collision-only movement —
  arrow keys move (world-axis-aligned), Space jumps, grid-based collision against `Worlds::World`
  with step-up traversal (climbs up to 1 block, matching `CLAUDE.md`'s allowed 3D adaptations) and
  gravity/landing. Constants (`kMoveSpeed=5.5`, `kJumpSpeed=12`, `kGravity=25`, `kFallLimit=-10`)
  match Simple3D's already-approved `GEBlupiController` values. Deliberately engine-agnostic (only
  depends on `GalaxyEggbert::Worlds::World`, no CNA types) so it is independently testable — added
  `tools/VerifyBlupiMovement.cpp` (new CMake tool target, no CNA link needed either) which scripts
  input against the real `worlds3d/world001.vwr` and checks: spawns grounded, climbs the staircase
  via step-up (Y 1→11), is blocked by the room's west wall (doesn't clip through), and falls under
  real gravity from height and lands. All checks passed. This surfaced and fixed a real bug in
  `tools/GenerateSampleWorld3D.cpp`: the platform floor fill included x=20, double-stacking a block
  on top of the staircase's own last step and creating an unclimbable 2-block cliff — narrowed the
  fill range to x=5..19; regenerated `worlds3d/world001.vwr` (2729 blocks now, was 2749) and
  re-verified. Wired into `GalaxyEggbertCnaGame`: spawns Blupi at world (0,1,0) (the ground floor);
  `Update()` polls `Keyboard::GetState()` (arrows + Space) and calls `Step()`; the camera now
  follows Blupi's live position every frame instead of a fixed terrain-centroid shot. Verified with
  a real run (no live keypresses in headless CI, so Blupi stays at spawn, but confirms the full
  pipeline runs with no crash): `terrain visibility check — 25/25` (close-up follow-cam fills the
  screen with floor, as expected at spawn). No sprite/visual yet (`E3D-MIG-061..063`), no animation
  state machine (`E3D-MIG-064`).
- [ ] E3D-MIG-061 — CPU-side vertex builder for `Easy3D::BillboardBatch` items.
- [ ] E3D-MIG-062 — CNA renderer adapter for billboard-batch vertex data.
- [ ] E3D-MIG-063 — Render Blupi as a 2D billboard using `blupi.png`/`blupi1.png` frames — no 3D model required.
- [ ] E3D-MIG-064 — Port Blupi's state machine (Stop/March/Jump/Air/Down/Up) using `table_blupi` frame indices as reference (copy/adapt requires approval per `easy3d.md` §5.4/§5.7 — confirm scope before transcribing table data).
- [ ] E3D-MIG-065 — (Optional, later) 3D Blupi model.
- [ ] E3D-MIG-066 — (Optional, later) Camera mode switching.

### Phase 7 — Object/decor first version

- [ ] E3D-MIG-070 — Render pickups/enemies as billboards from `element.png`.
- [ ] E3D-MIG-071 — Reuse `ObjectType` IDs (pending E3D-MIG-039 parity check) for object identification.
- [ ] E3D-MIG-072 — Use mobile-eggbert's `Decor.cpp` as canonical behavior reference for per-object-type movement/collision — reference only, no direct linking or copying (`easy3d.md` §5.2, §6.4).
- [ ] E3D-MIG-073 — Keep gameplay faithful — no invented mechanics; cross-check every behavior against mobile-eggbert before implementing.
- [ ] E3D-MIG-074 — `[?]` Before implementing any object behavior that seems to require exposing internal `Decor` state, stop and discuss with the user (`easy3d.md` §5.2).

### Phase 8 — Sound

- [ ] E3D-MIG-080 — Thin CNA `SoundEffect`/`SoundEffectInstance` wrapper for `GalaxyEggbertCNA`, informed by but not copied from mobile-eggbert's `Sound`/`ISound`.
- [ ] E3D-MIG-081 — Reuse `sound*.wav` files directly.
- [ ] E3D-MIG-082 — Reuse/cross-check `SoundChannel` indices (pending E3D-MIG-039).
- [ ] E3D-MIG-083 — 93-channel playback parity with the Simple3D port's existing sound system (per-channel volume, loop support).

### Phase 9 — HUD

- [ ] E3D-MIG-090 — Start minimal: lives, world, treasure count only.
- [ ] E3D-MIG-091 — Reuse mobile-eggbert HUD assets (`jauge.png`, `pad.png`, `text.png`, `button.png`).
- [ ] E3D-MIG-092 — Use CNA `SpriteBatch` (or an Easy3D HUD helper only if one already exists — do not build a new one speculatively).
- [ ] E3D-MIG-093 — Avoid building a full UI framework in Easy3D or Galaxy Eggbert.

### Phase 10 — Gameplay parity

- [ ] E3D-MIG-100 — Port pickups (treasure/keys/shield/egg/drink) — behavior reference: mobile-eggbert `Decor.cpp` + existing Simple3D port.
- [ ] E3D-MIG-101 — Port hazard/kill detection (lava/spike/saw/crusher).
- [ ] E3D-MIG-102 — Port enemy stomp + score.
- [ ] E3D-MIG-103 — Port respawn invincibility.
- [ ] E3D-MIG-104 — Port exit-gate logic.
- [ ] E3D-MIG-105 — Port crate push (ObjectType12) and platform patrol, matching the Simple3D port's already-fixed behavior.
- [ ] E3D-MIG-106 — Save/load — decide scope per `easy3d.md` §12 Q7 (byte-compatible with mobile-eggbert `GameData`, or fresh format).
- [ ] E3D-MIG-107 — No new gameplay mechanics — every item on this list must trace back to confirmed mobile-eggbert behavior.

### Phase 11 — Retire Simple3D path (later)

- [ ] E3D-MIG-110 — Do not delete `GalaxyEggbertSimple3D` now.
- [ ] E3D-MIG-111 — Retire it only after `GalaxyEggbertCNA` reaches playable parity with sections 1–14 above.
- [ ] E3D-MIG-112 — Keep it as historical reference until then; re-evaluate with the user before any deletion.

### Phase 12 — Optional future

- [ ] E3D-MIG-120 — Optional 3D Blupi model.
- [ ] E3D-MIG-121 — Optional camera mode switching.
- [ ] E3D-MIG-122 — Optional Lua discussion (undecided, out of scope for first migration — coordinate timing with `../easy-3d`'s own open Lua question).
- [ ] E3D-MIG-123 — Optional Easy3D renderer polish, once the minimal adapters from Phase 5/6 exist.
- [ ] E3D-MIG-124 — None of the above are part of the first playable migration.

**Decided (2026-07-01, rejected — do not revisit without explicit user request):** no
`mobile-eggbert .txt → galaxy-eggbert .vwr` auto-converter tool. Mobile-eggbert levels are flat
(Y=0); mechanically expanding that into `.vwr`'s native 3D format produces a mostly-empty,
unplayable shape — "a curiosity, not something to play." The existing pattern (parse `.txt` live
at runtime into a flat `World`, render with 3D tech — Phase 4/5) stays as the faithful-remake
content source. Genuinely 3D-designed worlds (real verticality) are separate future work,
hand-authored by the user or Claude — not derived from mobile-eggbert data by any tool.

---

## 1. Engine & Build

- [x] BUILD-001 — CMake target `GalaxyEggbertSimple3D` builds on Linux (U3D backend)
- [x] BUILD-002 — `GALAXY_EGGBERT_BUILD_SIMPLE3D` defaults to `ON`
- [x] BUILD-003 — Web build (Emscripten / WebAssembly) — `GalaxyEggbertSimple3D.html`
- [ ] BUILD-004 — Android build (blocked: U3D has no Android support; revisit with Nova3D)
- [ ] BUILD-005 — Windows cross-compile (MinGW-w64) verified after S3D-9
- [ ] BUILD-006 — Nova3D backend switch: `cmake -S . -B build-nova3d -DSIMPLE3D_ENGINE=NOVA3D`
- [x] BUILD-007 — `GalaxyEggbertWorldsTests` (54 unit tests) builds and all pass
- [ ] BUILD-008 — `ctest --test-dir cmake-build-debug` discovers and runs 54 world tests
- [ ] BUILD-009 — CI: automated build on push (GitHub Actions, Linux + Web targets)
- [ ] BUILD-010 — Package installer / distributable (Linux AppImage or .tar.gz with bundled assets)

---

## 2. Menu & Screens (PRIORITY)

Menu screens use the same PNG backgrounds as mobile-eggbert (`Content/backgrounds/*.png`, `Content/icons/*.png`).
Each screen = full-screen `UI::Image` background + Simple3D `UI::Button` / `UI::Label` overlays.

### 2.1 Phase: First / Wait (loading screen)

- [ ] MENU-001 — Render `wait.png` as full-screen `UI::Image` during boot loading phase
- [ ] MENU-002 — Display animated loading gauge (`jauge.png`, yellow fill) at bottom-centre, same position as mobile-eggbert (196, 426 in 640×480 space)
- [ ] MENU-003 — Gauge fills from 0→100% as resources load (replicate `DrawWaitProgress` logic)
- [ ] MENU-004 — Transition from Wait → Init after loading completes (≥1 s minimum)
- [ ] MENU-005 — Hide wait gauge if resuming a saved game (ContinueMission path)

### 2.2 Phase: Init (main menu / gamer select)

- [x] MENU-006 — Render `init.png` as full-screen background
- [x] MENU-007 — Render `speedyblupi.png` (title logo) sliding in from top on enter, ease-out quadratic over 1 s
- [x] MENU-008 — Render `blupiyoupie.png` (Blupi character art) scaling in from centre (0.5→1.0 with fade-in) over 1 s
- [x] MENU-009 — Three gamer-slot buttons (A / B / C): render from `pad.png` (cell 140×140), correct screen positions, selected slot highlighted with alternate icon
- [ ] MENU-010 — Each gamer slot shows: name ("Gamer A/B/C"), lives count, main doors opened, secondary doors opened (text next to button, 0.7 scale)
- [ ] MENU-011 — "PLAY" button (`InitPlay` glyph) with label below
- [ ] MENU-012 — "SETUP" button (`InitSetup` glyph) with label to the right
- [ ] MENU-013 — "RANKING" button (`InitRanking` glyph) — shown only when ranking mode active
- [ ] MENU-014 — Semi-transparent panel behind left gamer slots (pad.png icon 15, opacity 0.3)
- [ ] MENU-015 — Semi-transparent panel behind right action buttons (pad.png icon 15, opacity 0.3)
- [ ] MENU-016 — Keyboard Back / Escape → exit game (from Init phase)
- [ ] MENU-017 — Animated fade-out when transitioning from Init → Play (speedyblupi.png slides out, blupiyoupie.png zooms out)
- [ ] MENU-018 — Animated fade-out when transitioning from Init → MainSetup (speedyblupi.png slides right, gear appears)
- [x] MENU-019 — Gamer selection persisted (GameData byte 2)
- [x] MENU-020 — Gamer slot info (lives, lastWorld, doors) read from GameData

### 2.3 Phase: Play (active gameplay)

- [ ] MENU-021 — Hide all menu UI elements during Play phase
- [ ] MENU-022 — "PAUSE" button (`PlayPause` glyph) visible during Play — top-right corner icon from `button.png`
- [ ] MENU-023 — On-screen directional pad (`pad.png` icons 0, 1) for touch/gamepad emulation
- [ ] MENU-024 — On-screen "JUMP" button (`PlayJump`) visible during Play
- [ ] MENU-025 — On-screen "ACTION" button (`PlayAction`) visible during Play
- [ ] MENU-026 — On-screen "DOWN" button (`PlayDown`) visible during Play (when applicable)
- [x] MENU-027 — Keyboard: Back/Escape during Play → Pause phase

### 2.4 Phase: Pause

- [x] MENU-028 — Render `pause.png` as full-screen background
- [ ] MENU-029 — Render `blupiyoupie.png` scaling/rotating in (same animation as Init but centred at 418,190)
- [ ] MENU-030 — "MENU" button (`PauseMenu`) with label below
- [ ] MENU-031 — "BACK" button (`PauseBack`) — shown only when mission ≠ 1
- [ ] MENU-032 — "SETUP" button (`PauseSetup`) with label below
- [ ] MENU-033 — "RESTART" button (`PauseRestart`) — shown only when mission ≠ 1 AND mission % 10 ≠ 0
- [ ] MENU-034 — "CONTINUE" button (`PauseContinue`) with label below
- [ ] MENU-035 — PauseBack goes to previous hub world (MissionBack logic: if mission%10==0 → Init, else mission/10*10)
- [ ] MENU-036 — PauseRestart restarts current mission
- [ ] MENU-037 — PauseContinue resumes play without reloading
- [ ] MENU-038 — Animated fade-out from Pause → Play (blupiyoupie.png zooms out)
- [ ] MENU-039 — Animated slide-out when Pause → PlaySetup (blupiyoupie.png slides right)

### 2.5 Phase: Resume (saved game continue prompt)

- [ ] MENU-040 — Render `pause.png` background (same as Pause)
- [ ] MENU-041 — Render `blupiyoupie.png` with rotation spring animation
- [ ] MENU-042 — "MENU" button (`ResumeMenu`) → Init
- [ ] MENU-043 — "CONTINUE" button (`ResumeContinue`) → ContinueMission()
- [ ] MENU-044 — Resume phase triggers when app reactivates with a saved mid-game state
- [ ] MENU-045 — Keyboard Back during Resume → Init

### 2.6 Phase: Win

- [x] MENU-046 — Render `win.png` as full-screen background
- [ ] MENU-047 — Render `blupiyoupie.png` with pulsating scale (sin wave animation, amplitude 1.0±0.5)
- [ ] MENU-048 — "RETURN" button (`WinLostReturn`) → Init
- [ ] MENU-049 — Display mission elapsed time in text overlay
- [ ] MENU-050 — Display score in text overlay
- [ ] MENU-051 — Display "NEW RECORD!" text if score exceeds saved high score
- [ ] MENU-052 — Auto-advance to next level after N seconds (optional: like mobile-eggbert)

### 2.7 Phase: Lost (game over)

- [x] MENU-053 — Render `lost.png` as full-screen background
- [ ] MENU-054 — Render `blupiyoupie.png` with spin animation (6× rotation, quadratic ease-in, same as mobile-eggbert)
- [ ] MENU-055 — "RETURN" button (`WinLostReturn`) → Init
- [ ] MENU-056 — Display lives remaining and score
- [ ] MENU-057 — If 0 lives: "GAME OVER" text; if lives remain: "TRY AGAIN" hint

### 2.8 Phase: MainSetup / PlaySetup (settings)

- [x] MENU-058 — Render `setup.png` as full-screen background
- [ ] MENU-059 — Render `speedyblupi.png` sliding in from left (ease-out quadratic)
- [ ] MENU-060 — Render two rotating `gear.png` icons (one CW, one CCW, varying speeds)
- [ ] MENU-061 — "SOUNDS" toggle button (`SetupSounds`) — shows ON/OFF state
- [ ] MENU-062 — "JUMP" mode toggle (`SetupJump`) — left/right jump direction
- [ ] MENU-063 — "ZOOM" toggle (`SetupZoom`) — auto-zoom on/off
- [ ] MENU-064 — "ACCEL" toggle (`SetupAccel`) — accelerometer on/off
- [ ] MENU-065 — "RESET Gamer X" button (`SetupReset`) — with gamer letter in text
- [ ] MENU-066 — "RETURN" button (`SetupReturn`) → Init (from MainSetup) or Play (from PlaySetup)
- [ ] MENU-067 — All toggles persist to GameData immediately on press
- [ ] MENU-068 — Animated slide-in/out of settings panel (matching mobile-eggbert timing)
- [ ] MENU-069 — Keyboard Back during Setup → Init

### 2.9 Phase: Ranking

- [ ] MENU-070 — Render `pause.png` background (same as Pause/Resume)
- [ ] MENU-071 — Display high-score table for all 3 gamer slots (name, score, doors opened)
- [ ] MENU-072 — "BACK" button (`RankingContinue`) → Init
- [ ] MENU-073 — Highlight current gamer row

### 2.10 Phase: Trial (purchase prompt — low priority for open-source port)

- [ ] MENU-074 — Render `trial.png` background
- [ ] MENU-075 — Display trial text lines (TX_TRIAL1..6)
- [ ] MENU-076 — "BUY" button (`TrialBuy`) — no-op or skip in open-source build
- [ ] MENU-077 — "CANCEL" button (`TrialCancel`) → Init
- [ ] MENU-078 — Trial mode guard: if mission > 20 and mission % 10 > 1 → Trial (replicate mobile-eggbert trial logic)

### 2.11 Level Intro / Mission Title

- [ ] MENU-079 — Level intro title card: world name text, 3 s duration (fade-in 0.5s, hold 2s, fade-out 0.5s)
- [ ] MENU-080 — Training level hint bar: show tutorial text from `table_training1..4` based on Blupi position
- [ ] MENU-081 — Training hint rendered as overlay bar (pad.png icon 15 background, text centred)
- [ ] MENU-082 — Training hint auto-scales down if text is too wide (min 0.5×)

### 2.12 Button Font & Text Rendering

- [ ] MENU-083 — Render button labels using `text.png` font sheet (32×32 per glyph)
- [ ] MENU-084 — `Text::DrawText` equivalent: render text string using glyph atlas
- [ ] MENU-085 — `Text::DrawTextCenter` equivalent: centre-aligned text rendering
- [ ] MENU-086 — Text scaling (0.45×, 0.7×, 1.0×) used for different label sizes
- [ ] MENU-087 — Localised strings (MyResource strings): port key TX_ constants for button labels

### 2.13 Phase Transitions & Animations

- [ ] MENU-088 — Fade-out animation between animated phases (20-frame linear fade, Config::ScaleTime(20))
- [ ] MENU-089 — `fadeOutPhase` deferred transition: start animation, complete transition after 20 frames
- [ ] MENU-090 — `missionToStart1/2` two-stage mission loading pipeline (background swap before Start)
- [ ] MENU-091 — Phase time counter reset on each phase entry

### 2.14 Cheat Menu (hidden)

- [ ] MENU-092 — Cheat gesture recognition: sequence of 6 button glyphs (Cheat11..Cheat32) unlocks cheat menu
- [ ] MENU-093 — Cheat menu overlay: 9 cheat action buttons (Cheat1..Cheat9)
- [ ] MENU-094 — Cheat 1: OpenDoors (open all doors in current level)
- [ ] MENU-095 — Cheat 2: SuperBlupi (invincibility + all abilities)
- [ ] MENU-096 — Cheat 3: ShowSecret (reveal secret exits)
- [ ] MENU-097 — Cheat 4: LayEgg (spawn eggs)
- [ ] MENU-098 — Cheat 5: Reset gamer progress
- [ ] MENU-099 — Cheat 6: Simulate trial mode toggle
- [ ] MENU-100 — Cheat 7: CleanAll (remove all mobile objects)
- [ ] MENU-101 — Cheat 8: AllTreasure (collect all treasures)
- [ ] MENU-102 — Cheat 9: EndGoal (win current level immediately)

---

## 3. HUD (Heads-Up Display)

- [x] HUD-001 — Life icons: Blupi head sprite (icon 48 from `blupi.png`) × nbVies, bottom-left row
- [~] HUD-002 — Life icons cap at 5 visible; overflow shown as "+N" text — revision: verify exact mobile-eggbert layout
- [x] HUD-003 — Treasure counter "N/total" text, bottom-centre panel
- [x] HUD-004 — Panel background behind treasure counter (pad.png icon 15, opacity 0.6)
- [x] HUD-005 — Key icon — red key (element.png icon 215) shown when Key1 held
- [x] HUD-006 — Key icon — green key (element.png icon 222) shown when Key2 held
- [x] HUD-007 — Key icon — blue key (element.png icon 229) shown when Key3 held
- [x] HUD-008 — Shield timer gauge (jauge.png yellow fill) — visible when shield active
- [x] HUD-009 — Score display (text label, top-right area)
- [x] HUD-010 — World name + elapsed level timer
- [x] HUD-011 — Game speed indicator label (SLOW / NORMAL / FAST)
- [x] HUD-012 — Gauge sprite (jauge.png) bottom-left area
- [x] HUD-013 — Hit flash: red full-screen overlay panel, 0.4 s fade on damage
- [x] HUD-014 — Camera shake on hit (wired but shake amount is no-op — see CAM-006)
- [ ] HUD-015 — Bullet counter: element.png icon 176 × m_blupiBullet, small row near bottom-right
- [ ] HUD-016 — Dynamite count: element.png icon 252 shown when m_blupiDynamite > 0
- [ ] HUD-017 — Perso (persona) counter: button.png icon 108 + "= N" text when m_blupiPerso > 0
- [ ] HUD-018 — Second gauge (jauge.png red fill): used for charge level (m_blupiLevel in charge mode)
- [ ] HUD-019 — Yellow gauge: shield timer ticks down from 100 (shown while shield active, hidden when expired)
- [ ] HUD-020 — "EXIT OPEN!" popup text (3 s timed, big centred text) when all treasures collected
- [ ] HUD-021 — Controls hint bar fades after 8 s (re-show on new level)
- [ ] HUD-022 — Pause button icon visible during Play phase (top area)
- [ ] HUD-023 — HUD hidden during non-Play phases (Init, Pause, Win, Lost, Setup)
- [ ] HUD-024 — Training hint overlay at screen top (missions 11-14 only)
- [ ] HUD-025 — Score popup: "+N" floating text rises and fades over 1 s at collection position *(3D visual)*
- [ ] HUD-026 — "EXIT OPEN!" text: centre screen, large font, 3 s duration

---

## 4. Blupi Character

### 4.1 Physics & Movement

- [x] BLUPI-001 — Gravity applied every frame (m_blupiVitesseY increases downward)
- [x] BLUPI-002 — Jump: upward velocity on Left Ctrl press; air flag set
- [x] BLUPI-003 — Walk left/right: arrow keys set horizontal speed
- [x] BLUPI-004 — Crouch: Left Shift sets Down state
- [x] BLUPI-005 — Look up / glide: Right Shift in air → reduced gravity, capped fall speed
- [x] BLUPI-006 — Auto step-up: 1-tile ledges climbed automatically *(3D adaptation)*
- [x] BLUPI-007 — AABB tile collision via CharacterController
- [x] BLUPI-008 — Respawn at blupiStart on death
- [x] BLUPI-009 — Respawn invincibility: 2 s grace period after death
- [x] BLUPI-010 — Flash during invincibility (10 Hz sprite show/hide)
- [x] BLUPI-011 — Blob shadow (scans downward, scales with height) *(3D adaptation)*
- [ ] BLUPI-012 — Sub-pixel accumulator (m_blupiSubPixelX/Y) — prevents drift at high FPS
- [ ] BLUPI-013 — BlupiBloque: directional collision query (can I move here?)
- [ ] BLUPI-014 — BlupiAdjust: push Blupi out of penetrated tiles after movement
- [ ] BLUPI-015 — SCROLL_SPEED = 8 px/tick camera scroll toward Blupi
- [ ] BLUPI-016 — m_blupiLevel: charge level gauge for vehicles/actions
- [ ] BLUPI-017 — m_blupiTimeNoAsc: timer preventing lift re-entry after a dismount
- [ ] BLUPI-018 — m_blupiTimeMockery: timer for enemy mockery animation
- [ ] BLUPI-019 — m_blupiTimeOuf: relief animation timer (Ouf variants)
- [ ] BLUPI-020 — m_blupiFifoPos[10]: history of last 10 positions (used for teleporter exit placement)
- [ ] BLUPI-021 — Blupi "front" flag (m_blupiFront): determines draw order vs objects

### 4.2 BlupiAction State Machine (87 states)

- [x] BLUPI-022 — None (uninitialised)
- [x] BLUPI-023 — Stop (idle standing)
- [x] BLUPI-024 — March (walking)
- [x] BLUPI-025 — Turn (turning around)
- [x] BLUPI-026 — Jump (jumping)
- [x] BLUPI-027 — Air (airborne / falling)
- [x] BLUPI-028 — Down (crouch)
- [x] BLUPI-029 — Up (look-up / glide)
- [ ] BLUPI-030 — Vertigo (hanging on ledge in fear — ACTION_VERTIGO)
- [ ] BLUPI-031 — Recede (moving backward — ACTION_RECEDE)
- [ ] BLUPI-032 — Advance (moving forward — ACTION_ADVANCE)
- [ ] BLUPI-033 — Clear1..Clear8 (clearing animations — used for special level events)
- [ ] BLUPI-034 — Set (placing object — ACTION_SET)
- [ ] BLUPI-035 — Win (level-win celebration animation)
- [x] BLUPI-036 — Push (pushing a crate — ACTION_PUSH) — state exists but logic not wired
- [ ] BLUPI-037 — StopHelico (helicopter hover)
- [ ] BLUPI-038 — MarchHelico (helicopter fly forward)
- [ ] BLUPI-039 — TurnHelico (helicopter turn)
- [ ] BLUPI-040 — StopNage (treading water)
- [ ] BLUPI-041 — MarchNage (swimming forward)
- [ ] BLUPI-042 — TurnNage (turning while swimming)
- [ ] BLUPI-043 — StopSurf (surfboard idle)
- [ ] BLUPI-044 — MarchSurf (surfing forward)
- [ ] BLUPI-045 — TurnSurf (turning on surfboard)
- [ ] BLUPI-046 — Drown (drowning in deep water)
- [ ] BLUPI-047 — StopJeep / MarchJeep / TurnJeep (jeep vehicle states)
- [ ] BLUPI-048 — StopPop / Pop (pop-star costume idle/dance)
- [ ] BLUPI-049 — Bye (farewell exit animation)
- [ ] BLUPI-050 — StopSuspend / MarchSuspend / TurnSuspend / JumpSuspend (rope hanging)
- [ ] BLUPI-051 — Hide (hiding in object)
- [ ] BLUPI-052 — JumpAie (hurt jump on hazard contact)
- [ ] BLUPI-053 — StopSkate / MarchSkate / TurnSkate / JumpSkate / AirSkate (skateboard)
- [ ] BLUPI-054 — TakeSkate (picking up skateboard)
- [ ] BLUPI-055 — DeposeSkate (putting down skateboard)
- [ ] BLUPI-056 — Ouf1a / Ouf1b / Ouf2 / Ouf3 / Ouf4 / Ouf5 (relief animations)
- [ ] BLUPI-057 — Sucette (collecting lollipop/suction-cup power-up)
- [ ] BLUPI-058 — StopTank / MarchTank / TurnTank / FireTank (tank vehicle)
- [ ] BLUPI-059 — Glu (stuck in glue)
- [ ] BLUPI-060 — Drink (drinking power-up animation)
- [ ] BLUPI-061 — Charge (being charged at by enemy)
- [ ] BLUPI-062 — Electro (electrocuted by electric field)
- [ ] BLUPI-063 — HelicoGlu (helicopter stuck in glue)
- [ ] BLUPI-064 — TurnAir (turning while airborne)
- [ ] BLUPI-065 — StopMarch (decelerating from walk to stop)
- [ ] BLUPI-066 — StopJump / StopJumph (jump landing, high-jump landing)
- [ ] BLUPI-067 — Mockery / Mockeryi / Mockeryp (enemy mocking Blupi)
- [ ] BLUPI-068 — Balloon (balloon flight mode)
- [ ] BLUPI-069 — StopOver / MarchOver / TurnOver (flat/squashed mode)
- [ ] BLUPI-070 — Recedeq / Advanceq (quick backward/forward movement)
- [ ] BLUPI-071 — StopEcrase / MarchEcrase (crushed under object)
- [ ] BLUPI-072 — Teleporte (teleporting animation)
- [ ] BLUPI-073 — Switch (activating a switch)
- [ ] BLUPI-074 — Non (refusing / head-shake animation)
- [ ] BLUPI-075 — SlowdownSkate (skateboard braking)
- [ ] BLUPI-076 — TakeDynamite / PutDynamite (dynamite pickup/place)

### 4.3 Blupi Sprite Animation

- [x] BLUPI-077 — Frame tables from `table_blupi` (2911 entries)
- [x] BLUPI-078 — Billboard sprite from `blupi.png` (60×60 px cells)
- [x] BLUPI-079 — Direction flipping: mirror sprite when moving right (table_mirror)
- [ ] BLUPI-080 — All 87 BlupiAction frames resolved from table_blupi via action+phase+dir lookup
- [ ] BLUPI-081 — `blupi1.png` alternate skin channel (Blupi1_11/12/13 variants for ObjectType200-203)
- [x] BLUPI-082 — Shield tint: cyan/blue sprite overlay when m_blupiShield active
- [x] BLUPI-083 — Shield blink at < 1.5 s remaining (blink 10 Hz)

### 4.4 Vehicle Modes (each = new movement model + sprite sheet section)

- [ ] BLUPI-084 — Helicopter mode (m_blupiHelico): 8-direction flight, no gravity, propeller sound loop (ch16/ch18)
- [ ] BLUPI-085 — Helicopter boarding: touch ObjectType13 → sets m_blupiHelico, removes object
- [ ] BLUPI-086 — Helicopter dismount: press Down → drops Blupi, reverts to normal mode
- [ ] BLUPI-087 — Helicopter destroyed by creature (ObjectType54): ByeByeHelico debris effect
- [ ] BLUPI-088 — Jeep mode (m_blupiJeep): horizontal drive, jump, carry Blupi
- [ ] BLUPI-089 — Jeep boarding: touch ObjectType19 → sets m_blupiJeep
- [ ] BLUPI-090 — Jeep motor sound loop (ch29/ch31, high/low pitch via m_blupiMotorHigh)
- [ ] BLUPI-091 — Tank mode (m_blupiTank): drive + fire projectiles (ch FireTank)
- [ ] BLUPI-092 — Tank boarding: touch ObjectType28 → sets m_blupiTank
- [ ] BLUPI-093 — Tank fire sound (ch 53) on FireTank animation frames
- [ ] BLUPI-094 — Skateboard mode (m_blupiSkate): faster horizontal, higher jump
- [ ] BLUPI-095 — Skateboard pickup: touch ObjectType24 → TakeSkate animation → sets m_blupiSkate
- [ ] BLUPI-096 — Balloon mode (m_blupiOver/m_blupiBalloon): float up/down, limited horizontal
- [ ] BLUPI-097 — Balloon pickup: touch ObjectType46 → sets m_blupiOver
- [ ] BLUPI-098 — Swimming (m_blupiNage): entered when Blupi falls into water (`table_vitesse_nage`)
- [ ] BLUPI-099 — Surfing (m_blupiSurf): surfboard on water surface (`table_vitesse_surf`)
- [ ] BLUPI-100 — Vent (fan) propulsion (m_blupiVent): blown by ventilator tile
- [ ] BLUPI-101 — Suspend (rope hang): entered when Blupi grabs a rope tile
- [ ] BLUPI-102 — Motor sound crossfade: one-shot start/stop sounds + looped motor sound
- [ ] BLUPI-103 — m_blupiMotorHigh: pitch variant selection (fast vs slow motor)

### 4.5 Blupi Special States & Power-ups

- [x] BLUPI-104 — Shield (m_blupiShield): 5 s invincibility from ObjectType25; bypasses all hazards
- [ ] BLUPI-105 — Shield timer (m_blupiTimeShield): counts down 0→100 ticks; gauge shows progress
- [ ] BLUPI-106 — Shield trail sparkle (ObjectType57 spawned while shield active)
- [ ] BLUPI-107 — SuperBlupi (m_bSuperBlupi): cheat mode, full invincibility + all powers
- [ ] BLUPI-108 — Cloud mode (m_blupiCloud): from ObjectType31, floats through blocks for N ticks
- [ ] BLUPI-109 — Invert mode (m_blupiInvert): from ObjectType40, inverted controls for 100 ticks
- [ ] BLUPI-110 — Invert start/stop particle burst (ObjectType41/42 in 4 directions)
- [ ] BLUPI-111 — Ghost mode (m_blupiGhost): cheat, passes through walls, no interactions
- [ ] BLUPI-112 — Hide mode (m_blupiHide): concealed in object
- [ ] BLUPI-113 — Sucette/suction-cup (m_blupiPower): from ObjectType26, walk up walls
- [ ] BLUPI-114 — Dynamite (m_blupiDynamite): from ObjectType55; TakeDynamite / PutDynamite actions
- [ ] BLUPI-115 — Bullet count (m_blupiBullet): from ObjectType29; FireTank expends bullets
- [ ] BLUPI-116 — Ecrase mode (m_blupiEcrase): crushed flat under object (StopEcrase/MarchEcrase)
- [ ] BLUPI-117 — m_blupiPerso: persona counter (shown in HUD as button icon 108 + count)

### 4.6 Blupi Death & Respawn

- [x] BLUPI-118 — Death: BlupiDead() triggers explosion effect, resets lives-1, respawn
- [x] BLUPI-119 — Death freeze: 1 s input lock before respawn
- [ ] BLUPI-120 — Drown death: different animation (ACTION_DROWN) in deep water
- [ ] BLUPI-121 — Electro death: ACTION_ELECTRO animation + electric shake
- [ ] BLUPI-122 — Glu death: Blupi stuck (ACTION_GLU) for several frames then die
- [ ] BLUPI-123 — Charge death: enemy charge-hit animation (ACTION_CHARGE)
- [ ] BLUPI-124 — Ouf recovery: after close call, play one of Ouf1a..Ouf5 animations
- [ ] BLUPI-125 — Mockery: enemies mock Blupi (ACTION_MOCKERY/i/p) for m_blupiTimeMockery ticks
- [x] BLUPI-126 — Stomp kill: velY < -1.0 on contact with enemy → BounceUp() + kill enemy
- [x] BLUPI-127 — BounceUp: upward impulse kJumpSpeed × 0.65

### 4.7 Blupi Sounds

- [x] BLUPI-128 — Jump sound: ch1 on jump
- [x] BLUPI-129 — Footstep sound: ch3 per march stride (surface-dependent via SoundEnviron)
- [x] BLUPI-130 — Landing sound: ch4 on ground contact
- [x] BLUPI-131 — Stomp kill sound: ch5
- [x] BLUPI-132 — Death sound: ch8
- [ ] BLUPI-133 — Surface-specific footstep: SoundEnviron maps ch3/ch4 to ch78-91 based on tile type
- [ ] BLUPI-134 — Walk in water sound: ch36 (shallow water ambient)
- [ ] BLUPI-135 — Swim bubble sound: ch37
- [ ] BLUPI-136 — Glide sound: ch41
- [ ] BLUPI-137 — Teleport in/out sounds: ch9 / ch12
- [ ] BLUPI-138 — Shield-on sound: ch50 when shield activated
- [x] BLUPI-139 — Shield-off sound: ch44 when shield expires
- [ ] BLUPI-140 — Dynamite pickup/place sounds: ch52
- [ ] BLUPI-141 — Tank fire sound: ch53
- [ ] BLUPI-142 — Switch activate: ch76/ch77 (off/on)
- [ ] BLUPI-143 — Rope suspend sounds: ch47 (attach), ch65 (detach)
- [ ] BLUPI-144 — Drink sound: ch58 on Drink action
- [ ] BLUPI-145 — Key pickup sound: ch11
- [x] BLUPI-146 — Life / egg pickup sound: ch42
- [ ] BLUPI-147 — Sucette pickup sound: ch62
- [ ] BLUPI-148 — Balloon motor sounds: ch28/ch30 (start/stop), ch29/ch31 (loop low/high)
- [ ] BLUPI-149 — Electro sounds: ch38 (long arc) / ch90 (spark)
- [ ] BLUPI-150 — Glu splash sounds: ch51
- [ ] BLUPI-151 — Water splash sounds: ch23 (small plouf), ch64 (tiplouf), ch24 (blup bubble)
- [ ] BLUPI-152 — Secret exit found sound: ch21
- [ ] BLUPI-153 — Door open sound: ch7

---

## 5. Tile Types & Terrain

### 5.1 World Loading

- [x] TILE-001 — Load world from mobile-eggbert `.txt` format (100×100 grid)
- [x] TILE-002 — 100×100 decor grid rendered as 3D cubes (1 cube per occupied tile)
- [x] TILE-003 — Tile textures from `object-m.png` (20 cols, 64×64 px; icon ID = block type)
- [x] TILE-004 — Correct tile passability via `table_decor_quart` (decorative → Air)
- [x] TILE-005 — 5 worlds (Grassland, Forest, Ice Caves, Lava Fields, Space Station)
- [x] TILE-006 — Level progression: win → next world, wraps at world 5
- [x] TILE-007 — Terrain depth fill (cliff edges extrude 3 dark fill blocks downward) *(3D)*
- [x] TILE-008 — Sky dome per world (`backgrounds/decorNNN.png`)
- [x] TILE-009 — Per-world sky colour (ambient + fog)
- [x] TILE-010 — `blupiPos=` header parsed → Blupi spawn position
- [x] TILE-011 — `region=` header parsed → background texture selection
- [x] TILE-012 — `music=` header parsed → ambient music track

### 5.2 Animated Tiles

- [x] TILE-013 — animPhase_ counter: 6 fps tick counter in GEWorldRuntime
- [x] TILE-014 — Lava tiles (icon 68, 8-frame: {68,69,70,71,72,71,70,69}) — kills on contact
- [x] TILE-015 — Crusher tiles (10-frame: {317..323...}) — kills in frames 5-9
- [x] TILE-016 — Saw tiles (6-frame: {378..383}) — kills on contact
- [x] TILE-017 — Spike tiles (16-frame: table_decor_piege1) — kills on contact
- [x] TILE-018 — Water1 tiles (6-frame: {92..95,94,93}) — animated decoration
- [x] TILE-019 — Water2 tiles (6-frame: {91,96..98,97,96}) — animated decoration
- [ ] TILE-020 — Ventilator/fan Up tiles (icons 126-128, 3-frame: table_decor_ventillog)
- [ ] TILE-021 — Ventilator/fan Down tiles (icons 129-131, 3-frame: table_decor_ventillod)
- [ ] TILE-022 — Ventilator/fan Right tiles (icons 132-134, 3-frame: table_decor_ventilloh)
- [ ] TILE-023 — Ventilator/fan Left tiles (icons 135-137, 3-frame: table_decor_ventillob)
- [ ] TILE-024 — Water drip tiles (icons: table_decor_goutte, 48-frame)
- [ ] TILE-025 — Temperature tile animation (table_decor_temp, 20-frame)
- [ ] TILE-026 — Marine tile (icon 203: table_marine, 11 frames, Object channel)
- [ ] TILE-027 — GETerrainRenderer::Update called every frame with animPhase for all animated tiles

### 5.3 Interactive / Hazard Tiles

- [x] TILE-028 — Lava (icon 68-72): kill Blupi on contact (IsLave)
- [x] TILE-029 — Spike (icon 373/347): kill Blupi on contact (IsPiege)
- [x] TILE-030 — Crusher (icon 317-323): kill only when fully extended (IsEcraseur, phase 5-9)
- [x] TILE-031 — Saw (icon 378-383): kill Blupi on contact (IsScie)
- [ ] TILE-032 — Water drip (IsGoutte): triggers glu/slow effect when hit
- [ ] TILE-033 — Blitz/lightning tile (IsBlitz): electric instant death
- [ ] TILE-034 — Spring/ressort tile (IsRessort): launch Blupi upward
- [ ] TILE-035 — Temp tile (IsTemp): brief passability change (bridge-like)
- [ ] TILE-036 — Door tile (IsDoor): locked door, opened by matching key (DoorKeyFlags)
- [ ] TILE-037 — Teleporter tile (IsTeleporte / SearchTeleporte): pair of tiles, teleport Blupi
- [ ] TILE-038 — Switch tile (IsSwitch / ActiveSwitch): toggles state of linked door/bridge
- [ ] TILE-039 — Bridge tile (IsBridge): builds a bridge (ObjectType52 animation)
- [ ] TILE-040 — Ventilator tile (IsVentillo): blows Blupi in direction when standing in fan stream
- [ ] TILE-041 — Normal jump tile (IsNormalJump): forces a jump when stepped on
- [ ] TILE-042 — Water surface (IsSurfWater): enter surf mode
- [ ] TILE-043 — Deep water (IsDeepWater): enter swim/drown mode
- [ ] TILE-044 — Out-of-water exit (IsOutWater): exit swim mode when reaching dry tile
- [ ] TILE-045 — Barre / barrier tile (GetTypeBarre): blocks certain vehicle types

### 5.4 Tile Adaptation (visual smoothing)

- [ ] TILE-046 — `table_adapt_decor` (144 entries): smooth corner blending based on neighbour mask
- [ ] TILE-047 — `table_adapt_fromage` (32 entries): cheese tile corner blending
- [ ] TILE-048 — `table_decor_quart` (7056 entries): full tile replacement lookup by neighbour mask

### 5.5 Background & Sky

- [x] TILE-049 — Background sky PNG per region (`decor000.png`..`decor031.png`, not all consecutive)
- [x] TILE-050 — 5 sky colour palettes (ambient + fog per world region)
- [ ] TILE-051 — Per-zone fog colour changes mid-level (region changes between areas)
- [ ] TILE-052 — Lightning tile visual effect (icon 66-68 drawn 13 px higher, ch69 sound)

---

## 6. Enemy AI

All enemies use billboard sprites from `element.png` (64×64 px cells).

### 6.1 Common Enemy Behaviour

- [x] ENEMY-001 — Patrol movement: oscillate between posStart and posEnd at constant speed
- [x] ENEMY-002 — Stationary enemies get ±2 tile default patrol range
- [x] ENEMY-003 — Directional sprites: flipX when moving right (`table_mirror`)
- [x] ENEMY-004 — Stomp kills all enemy types on velY < -1.0 contact
- [x] ENEMY-005 — Enemy respawns at posStart after 5 s (kill timer)
- [x] ENEMY-006 — Blob shadow under enemies *(3D adaptation)*
- [x] ENEMY-007 — Y-proximity check: aerial enemies don't hit ground-level Blupi
- [ ] ENEMY-008 — `MoveObjectStepLine`: advance/recede speed + end-dwell timer logic
- [ ] ENEMY-009 — `MoveObjectStepIcon`: per-type animation phase counter update

### 6.2 Per-Type Enemy Implementation

- [x] ENEMY-010 — ObjectType2: patrol enemy A (table_robot_left/right, icons 12-20 in element.png)
- [x] ENEMY-011 — ObjectType3: patrol enemy B (icons 48-56 in element.png)
- [x] ENEMY-012 — ObjectType4: bulldozer (table_bulldozer_left/right, turn2l/r)
- [ ] ENEMY-013 — ObjectType4: bulldozer charge behaviour on Blupi contact (distinct from simple patrol)
- [x] ENEMY-014 — ObjectType16: spider (icons 69-77, vertical oscillation hang↔drop)
- [x] ENEMY-015 — ObjectType17: fish (table_poisson_left/right, patrol in water)
- [ ] ENEMY-016 — ObjectType17: turn animation (table_poisson_turn2l/r, 48 frames each)
- [x] ENEMY-017 — ObjectType20: bird (table_oiseau_left/right, aerial Y=3.0 patrol)
- [ ] ENEMY-018 — ObjectType20: turn animation (table_oiseau_turn2l/r, 10 frames each)
- [x] ENEMY-019 — ObjectType33: blupit (table_blupit_left/right)
- [ ] ENEMY-020 — ObjectType33: blupit fires ObjectType23 projectile at phase 3 and phase 21 during turn
- [ ] ENEMY-021 — ObjectType32: blupih (table_blupih_left/right, turn2l/r)
- [ ] ENEMY-022 — ObjectType32: blupih fires ObjectType23 projectile during turn animation
- [ ] ENEMY-023 — ObjectType44: wasp/bee (table_guepe_left/right, 6-frame, fast patrol)
- [ ] ENEMY-024 — ObjectType44: turn animation (table_guepe_turn2l/r, 5 frames)
- [ ] ENEMY-025 — ObjectType54: creature (table_creature_left/right, 8-frame, slow patrol)
- [ ] ENEMY-026 — ObjectType54: long turn animation (table_creature_turn2, 152 frames)
- [ ] ENEMY-027 — ObjectType54: destroys Blupi's helicopter on contact (ByeByeHelico triggered)
- [ ] ENEMY-028 — ObjectType18: additional patrol enemy variant (sprite/behaviour TBD from Decor.cpp)
- [ ] ENEMY-029 — ObjectType96: follow enemy 1 (table_follow1, 26 frames) — chases Blupi
- [ ] ENEMY-030 — ObjectType97: follow enemy 2 (table_follow2, 5 frames) — tracks exact position
- [ ] ENEMY-031 — ObjectType96/97: MoveObjectFollow() logic — path toward Blupi

### 6.3 Projectiles

- [ ] ENEMY-032 — ObjectType23: fired projectile (icon 176 from element.png) — spawned by blupih/blupit
- [ ] ENEMY-033 — Projectile travels toward Blupi position, expires after 55 frames
- [ ] ENEMY-034 — Projectile hit detection: damage Blupi if shield inactive
- [ ] ENEMY-035 — Projectile sound (ch27 on fire?)

### 6.4 Enemy Sounds

- [ ] ENEMY-036 — Stomp kill sound: ch5 (already wired)
- [ ] ENEMY-037 — Bulldozer turn sound (ch33)
- [ ] ENEMY-038 — Enemy destruction sound varies by type
- [ ] ENEMY-039 — Wasp/bee movement sound (ch72/ch73)
- [ ] ENEMY-040 — Creature movement sound

---

## 7. Pickups & Objects

### 7.1 Static Collectibles

- [x] PICKUP-001 — ObjectType5: treasure (icons 0-11, element.png) — +10 score, required for exit
- [x] PICKUP-002 — ObjectType6: egg (icons 21-28) — +1 life (cap 9), ch42 sound
- [x] PICKUP-003 — ObjectType7: exit goal (icons 29-36) — triggers Win when all treasures collected
- [x] PICKUP-004 — ObjectType49: red key (table_cle1, 12 frames) — sets Key1 flag, +50 score, ch11
- [x] PICKUP-005 — ObjectType50: green key (table_cle2, 12 frames) — sets Key2 flag, +50 score
- [x] PICKUP-006 — ObjectType51: blue key (table_cle3, 12 frames) — sets Key3 flag, +50 score
- [x] PICKUP-007 — ObjectType25: shield orb (table_shield, 16 frames) — 5 s invincibility, ch50
- [x] PICKUP-008 — ObjectType30: drink (icon 178) — +1 life (cap 9), ch42 sound
- [ ] PICKUP-009 — ObjectType21: secret exit (table_cle, 12 frames) — sets m_bFoundCle, triggers Win
- [ ] PICKUP-010 — ObjectType31: cloud power-up (table_charge, 6 frames, Object channel) — m_blupiCloud 100 ticks
- [ ] PICKUP-011 — ObjectType40: invert power-up (table_invert, 20 frames) — m_blupiInvert 100 ticks + particle burst
- [ ] PICKUP-012 — ObjectType26: suction-cup (table_power, 8 frames) — ACTION_Sucette, wall climbing
- [ ] PICKUP-013 — ObjectType29: bullet ammo (icon 177) — +10 bullets to m_blupiBullet
- [ ] PICKUP-014 — ObjectType55: dynamite (icon 252) — ACTION_TakeDynamite
- [ ] PICKUP-015 — ObjectType13: helicopter (icon 68 in element.png) — sets m_blupiHelico
- [ ] PICKUP-016 — ObjectType19: jeep (icon 89) — sets m_blupiJeep
- [ ] PICKUP-017 — ObjectType28: tank (icon 167) — sets m_blupiTank
- [ ] PICKUP-018 — ObjectType24: skateboard (table_skate, 34 frames) — ACTION_TakeSkate
- [ ] PICKUP-019 — ObjectType46: balloon (icon 208) — sets m_blupiOver / m_blupiBalloon

### 7.2 Platform Lifts

- [x] PICKUP-020 — ObjectType1: platform lift (moves posStart↔posEnd, carries Blupi)
- [ ] PICKUP-021 — ObjectType47: platform lift rightward carry (+2 px/frame horizontal to Blupi when riding)
- [ ] PICKUP-022 — ObjectType48: platform lift leftward carry (-2 px/frame horizontal)
- [ ] PICKUP-023 — AscenseurDetect: detect lift below Blupi within height threshold
- [ ] PICKUP-024 — AscenseurVertigo: Blupi hangs on edge of platform (Vertigo state)
- [ ] PICKUP-025 — AscenseurShift: shift Blupi with moving platform
- [ ] PICKUP-026 — AscenseurSynchro: synchronise multiple lifts
- [ ] PICKUP-027 — m_blupiTimeNoAsc: cooldown preventing immediate re-entry

### 7.3 Crates (ObjectType12)

- [~] PICKUP-028 — ObjectType12: crate renders (static billboard) — revision: push mechanic needed
- [ ] PICKUP-029 — Crate push: walking into a crate → push 1 tile horizontally (ACTION_PUSH)
- [ ] PICKUP-030 — Crate stops when hitting a wall or another crate
- [ ] PICKUP-031 — Crates can be stacked (UpdateCaisse / SearchLinkCaisse)
- [ ] PICKUP-032 — m_rankCaisse / m_nbRankCaisse: array of crate object indices
- [ ] PICKUP-033 — TestPushCaisse: check if push is valid (clear path)
- [ ] PICKUP-034 — CaisseInFront: detect crate directly in front of Blupi
- [ ] PICKUP-035 — SmallShake on crate land / impact

### 7.4 Doors & Keys

- [ ] PICKUP-036 — DoorKeyFlags: 3-bit flag (Key1 / Key2 / Key3)
- [ ] PICKUP-037 — Door tile (IsDoor): opens when Blupi touches and holds matching key
- [ ] PICKUP-038 — InitializeDoors: restore door states from GameData on level load
- [ ] PICKUP-039 — MemorizeDoors: save door states to GameData on level exit
- [ ] PICKUP-040 — Door open animation: ObjectType22 (3-phase animation, self-removes)
- [ ] PICKUP-041 — Door open sound: ch7

### 7.5 Visual Effects (transient objects)

- [ ] PICKUP-042 — ObjectType8: primary explosion (table_explo1, explo.png Explosion channel)
- [ ] PICKUP-043 — ObjectType9: small explosion (table_explo2, 20 frames)
- [ ] PICKUP-044 — ObjectType10: tertiary explosion (table_explo3, 20 frames)
- [ ] PICKUP-045 — ObjectType11: fan shockwave (table_explo4, 9 frames) — triggers BigShake
- [ ] PICKUP-046 — ObjectType36: pollution puff (table_pollution, 8 frames, 16-tick lifetime)
- [ ] PICKUP-047 — ObjectType37: clear effect (table_clear, 70 frames)
- [ ] PICKUP-048 — ObjectType38: electric arc (table_electro, 90 frames, starts Blupi1_12 channel)
- [ ] PICKUP-049 — ObjectType39: treasure sparkle (table_tresortrack, 11 frames) — spawned on pickup
- [ ] PICKUP-050 — ObjectType41/42: invert start/stop particles (table_invertstart/stop, 8 frames × 4 dirs)
- [ ] PICKUP-051 — ObjectType53: tentacle hazard (table_tentacule, 45 frames, 90-tick lifetime)
- [ ] PICKUP-052 — ObjectType57: shield trail (table_shieldtrack, 20 frames)
- [ ] PICKUP-053 — ObjectType58: shield disappear (20 frames)
- [ ] PICKUP-054 — ObjectType27: magic track sparkle (table_magictrack, 24 frames)
- [ ] PICKUP-055 — ObjectType90: electric spark (table_explo5, 12 frames) — triggers ElectricShake
- [ ] PICKUP-056 — ObjectType91: small flash (table_explo6, 6 frames)
- [ ] PICKUP-057 — ObjectType92: long energy arc (table_explo7, 128 frames)
- [ ] PICKUP-058 — ObjectType93: tiny flash (table_explo8, 5 frames)
- [ ] PICKUP-059 — ObjectType98/99/100: water splashes (table_sploutch1/2/3, 10/13/18 frames)
- [ ] PICKUP-060 — ObjectType14: water plouf (table_plouf, 7 frames, Object channel)
- [ ] PICKUP-061 — ObjectType15: water bubble (table_blup, 20 frames) — rises to waypoint
- [ ] PICKUP-062 — ObjectType34: goo particle (table_glu, 25-frame looping element) — sticks to geometry
- [ ] PICKUP-063 — ObjectType35: small plouf (table_tiplouf, 3 frames)

### 7.6 Special Level Objects

- [ ] PICKUP-064 — ObjectType52: bridge construction (table_bridge, 157 frames) — also updates static decor
- [ ] PICKUP-065 — ObjectType56: dynamite fuse (table_dynamitef, 100 frames) — triggers DynamiteStart() at phases 50-69
- [ ] PICKUP-066 — DynamiteStart: blast clears tiles in all 4 directions (radius-based)
- [ ] PICKUP-067 — ObjectType200-203: Blupi avatar skins (icons 257-262 on respective channels)
- [ ] PICKUP-068 — ObjectType200: costume select pickup → triggers player-select voyage when touched
- [ ] PICKUP-069 — ObjectType201-203: damage Blupi on contact if shield/hide/SuperBlupi inactive

### 7.7 Pickup Sounds

- [x] PICKUP-070 — Treasure collect: ch10 (always restarts)
- [x] PICKUP-071 — Key pickup: ch11
- [x] PICKUP-072 — Life pickup (egg/drink): ch42
- [x] PICKUP-073 — Shield pickup: ch50 (already wired in GESound)
- [x] PICKUP-074 — Win/exit sound: ch57
- [ ] PICKUP-075 — Door open: ch7
- [ ] PICKUP-076 — Switch activate (on): ch77; switch deactivate: ch76
- [ ] PICKUP-077 — Explosion sounds: ch10 (collect), ch39 (key sparkle), ch40 (explosion)
- [ ] PICKUP-078 — Water plouf: ch23
- [ ] PICKUP-079 — Water bubble: ch24
- [ ] PICKUP-080 — Water small plouf: ch64
- [ ] PICKUP-081 — Glu/glue sound: ch51
- [ ] PICKUP-082 — Dynamite fuse sounds: ch52 (placement / explosions)
- [ ] PICKUP-083 — Secret exit pickup: ch21
- [ ] PICKUP-084 — Bridge construction sound: ch20
- [ ] PICKUP-085 — Balloon pickup sound: ch46
- [ ] PICKUP-086 — Shield trail sound: ch48
- [ ] PICKUP-087 — Shield loop sound: ch49 (looped while shield active)

---

## 8. Score & Progression

- [x] SCORE-001 — +10 score per treasure collected
- [x] SCORE-002 — +25 score per enemy stomped
- [x] SCORE-003 — +50 score per key collected
- [ ] SCORE-004 — +50 score per egg collected (mobile-eggbert: ch42 + life + score)
- [ ] SCORE-005 — +50 score per drink collected
- [x] SCORE-006 — +100 bonus when all treasures collected (all-treasures bonus)
- [x] SCORE-007 — High score per gamer slot persisted
- [x] SCORE-008 — Level elapsed timer displayed in HUD
- [x] SCORE-009 — Game speed selector: G key cycles Slow(0.6×) → Normal(1.0×) → Fast(1.5×)
- [ ] SCORE-010 — GameSpeed::Faster and GameSpeed::Fastest modes (from mobile-eggbert enum)
- [ ] SCORE-011 — Slow game speed: alternate-frame skip (`slow_frame` toggle in game loop)
- [ ] SCORE-012 — Win screen: display total score, elapsed time, new-record indicator
- [ ] SCORE-013 — Mission numbering: world hub (X0) → levels (X1-X5) → next hub ((X+1)0)
- [ ] SCORE-014 — 78 world files (world001.txt … world055.txt + hubs) supported
- [ ] SCORE-015 — IsTerminated: -1=lost, -2=win, ≥1=advance to mission N
- [ ] SCORE-016 — Mission advance: m_term = m_mission/10*10 + next_level_in_world
- [ ] SCORE-017 — Hub mission (mission % 10 == 0): no treasure counter in HUD
- [ ] SCORE-018 — Training missions (11-14): show tutorial hint overlay
- [ ] SCORE-019 — MemorizeGamerProgress: save lives and doors after each win/loss
- [ ] SCORE-020 — LastWorld: updated when completing a hub (mission divisible by 10)

---

## 9. Sound System

- [x] SOUND-001 — 93 WAV files (`sounds/sound000.wav`..`sound092.wav`) loaded
- [x] SOUND-002 — Per-channel volume from tableVolumePitch (GESound)
- [x] SOUND-003 — Sound on/off toggle (persisted in GameData byte 3)
- [x] SOUND-004 — Sound loop support: `loop=true` flag passes to `Game::PlaySound(loop)` in Simple3D
- [ ] SOUND-005 — Positional (panned) audio: volume/balance based on screen X position (SoundEnviron)
- [ ] SOUND-006 — SoundEnviron: maps ch3/ch4 footstep to tile-surface variant (ch78-91)
- [ ] SOUND-007 — Vehicle motor loop: ch16/ch18 (helicopter high/low), ch29/ch31 (jeep/tank/over)
- [ ] SOUND-008 — Motor sound crossfade: start sound (ch15/ch28) + stop sound (ch17/ch30)
- [ ] SOUND-009 — PosSound: update panned position of active motor loop each frame
- [ ] SOUND-010 — Ambient sound: all 72 gameplay channels wired to correct game events (see full list)

### Complete Sound Channel Wire-up (0=reserved, 1-92=game SFX)

- [x] SOUND-011 — ch1: jump
- [ ] SOUND-012 — ch2: unknown (research needed)
- [x] SOUND-013 — ch3: footstep (surface-dependent)
- [x] SOUND-014 — ch4: landing
- [x] SOUND-015 — ch5: stomp kill
- [ ] SOUND-016 — ch6: unknown
- [ ] SOUND-017 — ch7: door open
- [x] SOUND-018 — ch8: death / hit
- [ ] SOUND-019 — ch9: teleport in
- [x] SOUND-020 — ch10: collect (always restarts)
- [x] SOUND-021 — ch11: key pickup
- [ ] SOUND-022 — ch12: teleport out
- [ ] SOUND-023 — ch13: bridge build phase 1
- [ ] SOUND-024 — ch14: bridge build phase 2
- [ ] SOUND-025 — ch15: helicopter motor start
- [ ] SOUND-026 — ch16: helicopter motor high (loop)
- [ ] SOUND-027 — ch17: helicopter motor stop
- [ ] SOUND-028 — ch18: helicopter motor low (loop)
- [ ] SOUND-029 — ch19: teleport (alternate)
- [ ] SOUND-030 — ch20: bridge completed
- [ ] SOUND-031 — ch21: secret exit found
- [ ] SOUND-032 — ch22: unknown
- [ ] SOUND-033 — ch23: water plouf
- [ ] SOUND-034 — ch24: water bubble rise
- [ ] SOUND-035 — ch25: unknown
- [ ] SOUND-036 — ch26: unknown
- [ ] SOUND-037 — ch27: projectile fired
- [ ] SOUND-038 — ch28: jeep/tank start
- [ ] SOUND-039 — ch29: jeep/tank motor high (loop)
- [ ] SOUND-040 — ch30: jeep/tank stop
- [ ] SOUND-041 — ch31: jeep/tank motor low (loop)
- [ ] SOUND-042 — ch32: unknown
- [ ] SOUND-043 — ch33: bulldozer turn
- [ ] SOUND-044 — ch34: unknown
- [ ] SOUND-045 — ch35: unknown
- [ ] SOUND-046 — ch36: water walk ambient
- [ ] SOUND-047 — ch37: swim bubble
- [ ] SOUND-048 — ch38: electric arc (long)
- [ ] SOUND-049 — ch39: key sparkle effect
- [ ] SOUND-050 — ch40: explosion
- [x] SOUND-051 — ch41: glide
- [x] SOUND-052 — ch42: life/egg/drink pickup
- [ ] SOUND-053 — ch43: unknown
- [x] SOUND-054 — ch44: shield off
- [ ] SOUND-055 — ch45: unknown
- [ ] SOUND-056 — ch46: balloon mode sound
- [ ] SOUND-057 — ch47: suspend attach
- [ ] SOUND-058 — ch48: shield sparkle
- [ ] SOUND-059 — ch49: shield loop (looped while active)
- [ ] SOUND-060 — ch50: shield pickup
- [ ] SOUND-061 — ch51: glu/glue splash
- [ ] SOUND-062 — ch52: dynamite / impact
- [ ] SOUND-063 — ch53: tank fire
- [ ] SOUND-064 — ch54: long explosion (creature death?)
- [ ] SOUND-065 — ch55: unknown
- [ ] SOUND-066 — ch56: unknown
- [x] SOUND-067 — ch57: exit open / win
- [ ] SOUND-068 — ch58: drink pickup
- [ ] SOUND-069 — ch59: unknown
- [ ] SOUND-070 — ch60: pickup/collect (variant)
- [ ] SOUND-071 — ch61: unknown
- [ ] SOUND-072 — ch62: sucette / suction-cup
- [ ] SOUND-073 — ch63: unknown
- [ ] SOUND-074 — ch64: small water plouf
- [ ] SOUND-075 — ch65: suspend detach / rope release
- [ ] SOUND-076 — ch66: unknown
- [ ] SOUND-077 — ch67: unknown
- [ ] SOUND-078 — ch68: unknown
- [ ] SOUND-079 — ch69: lightning strike
- [ ] SOUND-080 — ch70: unknown
- [ ] SOUND-081 — ch71: unknown
- [ ] SOUND-082 — ch72: wasp approach
- [ ] SOUND-083 — ch73: wasp attack
- [ ] SOUND-084 — ch74: teleport in (Blupi arrival)
- [ ] SOUND-085 — ch75: teleport out (Blupi exit)
- [ ] SOUND-086 — ch76: switch deactivate
- [ ] SOUND-087 — ch77: switch activate
- [ ] SOUND-088 — ch78-91: surface-specific footstep/landing variants (mapped by SoundEnviron)
- [ ] SOUND-089 — ch92: follow-enemy sound
- [ ] SOUND-090 — Sound enable/disable respects enabled_ flag (all channels silenced when off)

---

## 10. Camera *(3D-specific)*

- [x] CAM-001 — 3rd-person orbit following Blupi (GECameraRig)
- [x] CAM-002 — RMB pitch control
- [x] CAM-003 — Scroll-wheel zoom (smooth lerp)
- [x] CAM-004 — Pitch auto-reset to 20° when RMB released
- [x] CAM-005 — Wall collision (DDA ray march from Blupi to desired camera position)
- [x] CAM-006 — FOV 65°
- [ ] CAM-007 — Camera shake: SmallShake (minor impacts: crate land, small explosions)
- [ ] CAM-008 — Camera shake: BigShake (fan-blade hit, large explosion) — triggered by ObjectType11
- [ ] CAM-009 — Camera shake: ElectricShake (ObjectType90 electric spark contact)
- [ ] CAM-010 — Camera shake: table_decor_action per-frame (dx, dy) offsets × 3 multiplier
- [ ] CAM-011 — Camera shake: fixed N-frame duration, self-clears to None after last frame
- [ ] CAM-012 — GECameraRig::StartShake(DecorAction) implementation in Simple3D
- [ ] CAM-013 — HotSpot zoom: MoveHotSpot() eases camera zoom toward target
- [ ] CAM-014 — HotSpot target: m_hotSpotFinalZoom/X/Y interpolated over N frames
- [ ] CAM-015 — HotSpot: triggered on special events (secret exit found, level end zoom)
- [ ] CAM-016 — SCROLL_MARGX = 80 px / SCROLL_MARGY = 40 px viewport scroll margins
- [ ] CAM-017 — Smooth scroll: camera eases toward Blupi at SCROLL_SPEED = 8 px/tick

---

## 11. Save Data

- [x] SAVE-001 — GameData: 640-byte flat binary format, binary-compatible with mobile-eggbert
- [x] SAVE-002 — Global header (10 bytes): version, selectedGamer, sounds, jumpRight, autoZoom, accelActive
- [x] SAVE-003 — 3 gamer slots × 210 bytes: lives (byte 0), lastWorld (byte 1), doors[200] (bytes 10-209)
- [x] SAVE-004 — Auto-save on win / lost / quit / reset / gamer-select
- [x] SAVE-005 — `Simple3D::SaveData` used for persistence
- [ ] SAVE-006 — doors[0..179]: secondary door states (180 secondary doors)
- [ ] SAVE-007 — doors[180..199]: main door states (20 main doors / hub worlds)
- [ ] SAVE-008 — GetGamerInfo: return lives, mainDoors, secondaryDoors per gamer slot
- [ ] SAVE-009 — CurrentWrite / CurrentRead: mid-game save/load (on app deactivate/activate)
- [ ] SAVE-010 — CurrentDelete: remove mid-game save (on OnExiting or normal level exit)
- [ ] SAVE-011 — Accelerometer sensitivity setting (byte 7, 0-100 → 0.0-1.0)
- [ ] SAVE-012 — JumpRight setting (byte 4) — jump direction preference
- [ ] SAVE-013 — AutoZoom setting (byte 5)
- [ ] SAVE-014 — Ranking mode persisted when isRankingMode is active

---

## 12. Visual Polish *(3D-specific and faithful to mobile-eggbert)*

- [x] VISUAL-001 — Blob shadow under Blupi (scales with height, disabled in helicopter/balloon)
- [x] VISUAL-002 — Blob shadow under enemies (disabled for birds)
- [x] VISUAL-003 — Pickup bobbing: sine-wave Y offset on collectibles
- [x] VISUAL-004 — Score popups: rising "+N" text, 1 s fade at collection position
- [x] VISUAL-005 — Respawn flash: Blupi billboard blinks at 10 Hz for 2 s after respawn
- [x] VISUAL-006 — Shield tint: cyan sprite when m_blupiShield active
- [x] VISUAL-007 — Shield blink at < 1.5 s remaining
- [ ] VISUAL-008 — Explosion billboard effects: ObjectType8-11 from `explo.png` (128×128 px, Explosion channel)
- [ ] VISUAL-009 — Water splash billboard effects: ObjectType98-100 from `explo.png`
- [ ] VISUAL-010 — Electric arc: ObjectType92 long arc from `explo.png` (128 frames)
- [ ] VISUAL-011 — Shield sparkle loop: ObjectType57 trail behind Blupi while shielded
- [ ] VISUAL-012 — Treasure sparkle: ObjectType39 on each treasure pickup
- [ ] VISUAL-013 — Pollution puff: ObjectType36 on environmental triggers
- [ ] VISUAL-014 — Invert power-up particles: ObjectType41 (4-direction burst on pickup)
- [ ] VISUAL-015 — Invert expire particles: ObjectType42 (4-direction burst on expiry)
- [ ] VISUAL-016 — Goo particle: ObjectType34 sticks to geometry (element.png, 25 frames)
- [ ] VISUAL-017 — Magic track sparkle: ObjectType27 trail effect
- [ ] VISUAL-018 — Helicopter debris: ByeByeHelico float-based debris pool when helico destroyed
- [ ] VISUAL-019 — Bridge construction animation: ObjectType52 (157 frames) modifies static decor
- [ ] VISUAL-020 — Dynamite fuse animation: ObjectType56 (100 frames) with blast events at phases 50-69
- [ ] VISUAL-021 — Tentacle hazard animation: ObjectType53 (45 frames, explo.png)
- [ ] VISUAL-022 — Sky gradient per world region (SetSkyGradient with zenith/horizon colours)
- [ ] VISUAL-023 — Per-world fog (SetFogEnabled + SetFogColor + SetFogRange per region)
- [ ] VISUAL-024 — Lightning visual: tiles 66-68 draw 13 px higher; ch69 sound
- [ ] VISUAL-025 — "EXIT OPEN!" text pop-up with sparkle effect when exit unlocks

---

## 13. Simple3D Migration Milestones

- [x] S3D-1 — Port skeleton: app entry, world loading, placeholder terrain, Blupi CharacterController, basic HUD, camera, sound, CMake target
- [x] S3D-2 — Terrain visual fidelity: tile atlas UV per block type. **Bug found + fixed
  (2026-07-04, while regenerating `mobile-eggbert-reference/tile-anim-lava.gif` for `DOC-101`):**
  `BlockTypes::tileUV()` (`include/GalaxyEggbert/BlockTypes.hpp`, shared by `GalaxyEggbertSimple3D`
  and `GalaxyEggbertCNA`) assumed a flat, contiguous 64px grid in `object-m.png`. The real sheet
  (confirmed from mobile-eggbert's own `Pixmap::GetSrcRectangle`, `PixmapChannel::Object` case:
  `srcGap=1`) is packed on a **65px pitch** (64px icon + 1px gap) with a 1px leading margin —
  `pixelX = 1 + col*65`, not `col*64`. The missing gap caused a cumulative 1px/column/row drift:
  by icon 68 (Lava, col 8 row 3) the sampled box was already off by (9px, 4px), bleeding in ~4px
  of the icon-above's content (a blue warning-sign icon) and cutting a visible seam partway
  through the tile — this is what the user spotted as "a thick blue line and the lava looking cut
  ~20% from the left" in a `GalaxyEggbertCNA` screenshot. Fixed by adding `BlockTypes::kSheetGap=1`
  and using it in `tileUV()`'s pixel-offset math, and passing the same gap as
  `startX/startY/spacingX/spacingY` to `Easy3D::TextureAtlas::AddGrid` in
  `src/GalaxyEggbertCNA/Game/GETileAtlas.cpp` (which already supported gap/offset parameters —
  no `../easy-3d` change needed); also corrected `GETileAtlas`'s row count from the old
  ceil(1431/64)=23 to the pitch-aware floor((1431-1)/65)=22 (the highest named `BlockTypes` icon
  constant is 413, well within the 22-row range, so nothing used the extra bogus row). **Verified**:
  both `GalaxyEggbertSimple3D` and `GalaxyEggbertCNA` rebuild clean; `GalaxyEggbertWorldsTests`
  still 54/54; a real `GalaxyEggbertCNA` run logs UV values matching the corrected formula by hand
  (`Lava (68) UV = (0.400461, 0.136967)-(0.449654, 0.181691)`, i.e. pixel origin ≈(521,196), matching
  `1+8*65=521, 1+3*65=196`) and its built-in terrain-visibility check still shows real, multi-color
  textured terrain (25/25 sampled points, 7 distinct colors) — no crash, no flat-fallback regression.
  Independently, cropping the same corrected pixel math for the `tile-anim-lava.gif` doc frames
  produced clean images with no blue bleed/seam (see `DOC-101`).
- [x] S3D-3 — Blupi billboard animation from `blupi.png`
- [x] S3D-4 — Decor object visuals: enemy + pickup billboard sprites from `element.png`
- [x] S3D-5 — HUD images: gauge sprite, life icons, key icons, hit flash panel
- [x] S3D-6 — Phase/menu port: Init gamer select with per-slot data, Settings screen, SaveData
- [x] S3D-7 — Sound channel parity: 93 channels, per-channel volume, no-restart policy
- [x] S3D-8 — Web build verified (Emscripten)
- [x] S3D-9 — Remove legacy Urho3D direct target; src/GalaxyEggbert/Game/ deleted
- [ ] S3D-10 — Full menu system (all phases with background PNGs and correct transitions)
- [ ] S3D-11 — Vehicle modes: helicopter, jeep, tank (boarding + physics + motor sounds)
- [ ] S3D-12 — Skateboard mode: faster movement + jump + table_skate animation
- [ ] S3D-13 — Swimming / surfing modes
- [ ] S3D-14 — Full enemy AI (all ObjectTypes with correct turn animations and projectiles)
- [ ] S3D-15 — Explosion / effect billboard system (explo.png objects)
- [ ] S3D-16 — Dynamite mechanic (pickup + place + fuse + blast)
- [ ] S3D-17 — All 78 world files playable end-to-end
- [ ] S3D-18 — Camera shake implementation
- [ ] S3D-19 — Crate push mechanic
- [ ] S3D-20 — Positional audio (panned sound by screen X position)
- [ ] S3D-21 — Nova3D backend switch (`-DSIMPLE3D_ENGINE=NOVA3D`)
- [ ] S3D-22 — Android build

---

## 14. Tests & Quality

- [x] TEST-001 — GalaxyEggbertWorldsTests: 54 unit tests (BlockTests, BitPackingTests, ChunkTests, WorldTests, BlockMetadataTest)
- [ ] TEST-002 — ctest discovery in cmake-build-debug (gtest_discover_tests fix)
- [ ] TEST-003 — Test: all 78 world files parse without error
- [ ] TEST-004 — Test: BlockTypes::tileUV returns valid UV for all known icon IDs
- [ ] TEST-005 — Test: GEWorldRuntime::LoadFromMobileEggbertFile round-trip
- [ ] TEST-006 — Test: GameData read/write round-trip (640-byte format)
- [ ] TEST-007 — Test: animPhase_ matches mobile-eggbert table indices at known times

---

## 15. Documentation — mobile-eggbert 2D reference (complete)

User feedback 2026-07-03: the first pass at `mobile-eggbert-2d-reference.md` (single file, 461
lines) was a representative sample, not a complete catalog — "všechny animace, všechny typy bloků"
(all animations, all block types). Restructured into `mobile-eggbert-reference/` (multi-file, one
concern per file, `images/` subfolder) and tracked as explicit, individually completable tasks
below. Each file's own top-of-file status note tracks its completion state — check there for
current progress, not just this list.

- [x] DOC-001 — Done (2026-07-03). Split the single `mobile-eggbert-2d-reference.md` into
  `mobile-eggbert-reference/00-overview.md` through `09-open-questions.md` (10 files) +
  `mobile-eggbert-reference/images/` (moved via `git mv`, history preserved). Verified
  programmatically: all 85 image references across all files resolve to real files, and all 85
  actual image files are referenced somewhere (no orphans, no broken links).
- [ ] DOC-002 — **Reopened 2026-07-03 (GIF ghosting bug + full audit, see §16).** All 441 tile icons
  are still individually correct static crops (single-frame images aren't affected by the GIF bug),
  but this is being re-verified anyway (`DOC-230`) given how many sprite-channel bugs have surfaced
  elsewhere. History below kept as-is.
  ~~Done (2026-07-03). All 441 addressable icons (0–440) accounted for in `02-tiles.md`:~~
  313 with a full 64×64 crop (every named/behavioral icon plus every icon confirmed used in at
  least one of the 78 real level files), remaining 128 unused/unnamed icons listed compactly by
  range (passability + mechanical alpha-based visual signal, not fabricated names). Built via a
  script (parses `BlockTypes::kPassable[441]` programmatically, batch-crops all icons with
  ImageMagick, scans real `Decor:`/`BigDecor:` grid sections across all 78 world files for genuine
  usage counts — not hand-typed). Real findings: (1) icon 440 has no real pixel data — the sheet
  only has 22 full 65px-stride rows (1430 of 1431px), so real icons are 0–439; (2) animated
  sub-frame icons (e.g. `Crusher` 318–323, `Saw` 379–383) are *never* placed directly in level
  files — only the base/first frame is, confirming `BlockTypes::tileAnimBase()`'s design assumption
  independently; (3) three named icons (`Water2`=96, `Spring`=211, `SwitchOff`=385) have 0/78 usage
  in the shipped level set (double-checked with a raw grep to rule out a parsing bug) — still
  functionally real, just never authored into any shipped level. Verified: all 372 image
  references across the whole `mobile-eggbert-reference/` tree resolve to real files, zero
  orphaned images, spot-checked crops for real pixel dimensions/content. Old per-name image files
  superseded by `images/tile-full-NNN.png` (single source of truth) except the 3 door crops still
  referenced by `06-doors.md`. **Follow-up fix (same day, coordinator's own verification pass):**
  found the "128 unused/unnamed" compact-range table used coarse, loosely-worded ranges (e.g.
  "166–205") that numerically overlapped icons already documented above (79 icons double-counted,
  e.g. `Wall`=183 sat inside that range) — total coverage (441/441) was never actually wrong, but
  the range boundaries were imprecise. Regenerated the 128-icon set as exact non-overlapping ranges
  (independently re-cropped, re-measured, re-merged only on consecutive-and-same-category runs);
  verified programmatically: 313 + 128 = 441, zero overlap, zero gaps.
- [ ] DOC-003 — **Reopened 2026-07-03 (GIF ghosting bug + full audit, see §16).** The 0–203
  classification itself is unaffected (that's a text/grep result, not an image); the static icon
  crops (single-frame, not GIF) are also not subject to the ghosting bug, but are being re-verified
  anyway (`DOC-231`) since 11 sprite-channel bugs have already turned up among them. History below
  kept as-is.
  Done (2026-07-03). Every `ObjectType` ID 0–203 classified into exactly one of 4
  categories, verified programmatically (script partitioned all 204 IDs, asserted no overlap/gap):
  **A** (29 IDs) — real `Decor.cpp` logic AND placed in ≥1 of the 78 shipped levels; **B** (41 IDs)
  — real logic confirmed (via direct grep of `Decor::MoveObjectStepIcon`, lines ~8192–9057, plus
  the rest of `Decor.cpp`/`Tables.cpp`) but never level-placed (mostly dynamically-spawned effects:
  explosions, splashes, door-open animation, etc.); **C** (1 ID, `95`) — appears only as a
  range-boundary literal in comparisons, never a direct `type==` check, left ambiguous rather than
  guessed; **D** (133 IDs) — zero references anywhere in `Decor.cpp`/`Tables.cpp`, genuinely
  vestigial (`43`, `45`, `59-89`, `94`, `101-199`). Cropped icons for the 12 types added by the
  earlier `MoveObject` fix (bringing total cropped from 18 to 30). **Real bug found, not fixed
  (documented in `03-objects.md`, tracked as `DOC-007` below)**: `GEDecorSystem.cpp` assumes every
  `ObjectType` draws via `element.png`, but real level data's `channel=` field shows types `1`,
  `12`, `47` actually use `PixmapChannel::Object` (`object-m.png`), and types `32`/`33` (`blupih`/
  `blupit`) use `PixmapChannel::Blupi1_11/12/13` (`blupi1.png`) — confirmed by grepping real
  `MoveObject:` lines across all 78 world files, not assumed. `47`'s `table_chenille` icon values
  (311–316) are additionally out-of-bounds for `element.png` (which only holds icons 0–289) —
  reproduced the ImageMagick crop error directly. `33` is one of the *original* 18 "confirmed"
  types, meaning this bug predates today's session. Files: `mobile-eggbert-reference/03-objects.md`,
  `00-overview.md`, 12 new + 1 corrected image crop in `images/`.
  **Follow-up (same day)**: user pointed out that skipping icon crops for Category B (41 IDs, real
  behavior but never level-placed) as "lower value" was exactly the kind of silent scope cut they'd
  already flagged — cropped 37 of 41 (the other 4 — `0`, `18`, `22`, `58` — genuinely have no
  icon-assignment logic anywhere in `Decor.cpp`, documented as such rather than guessed). Found 6
  more sprite-channel corrections along the way: `14`/`15`/`31`/`35`/`48`/`52` use `object-m.png`
  (not `element.png`), and `38` switches from `blupi1.png` (first 30 ticks) to `element.png`.
  Verified: all 440 image references in `mobile-eggbert-reference/` resolve, zero orphans.
- [ ] DOC-007 — Fix the `ObjectType` sprite-channel bug found by `DOC-003`: `GEDecorSystem.cpp`
  (Simple3D) hardcodes `element.png` for every object type; types `1`/`12`/`47` need
  `object-m.png`, types `32`/`33` need `blupi1.png` (variant selected by the level file's own
  `channel=` field — not a fixed choice per type, per real data showing `32`/`33` use different
  `Blupi1_1{1,2,3}` variants in different levels). Likely fix: read `.channel` from the parsed
  `MoveObjectSpec` (already captured from the level file — see `01-world-file-format.md`) instead
  of assuming `Element` unconditionally in `GEDecorSystem`. Needs its own verification (visual
  check or a scripted crop-and-compare against the correct sheet) before considering it done.
- [ ] DOC-004 — **Reopened a second time, 2026-07-03 (GIF ghosting bug, see §16.1/§16.2).** The user
  found by visual inspection that the GIFs themselves are corrupted: each frame accumulates the
  previous frame's opaque pixels instead of clearing (e.g. frame 2 shows frame 1 ghosted into its
  background). Confirmed via alpha-channel analysis on coalesced frames: `blupi-action-02-march.gif`
  mean alpha rises monotonically and plateaus (80.0→85→86.2→86.3→86.6→86.7) instead of fluctuating
  with the real walk-cycle silhouette; same pattern confirmed in `object-anim-type05-treasure.gif`
  (133.7→...→153.6, plateaus) and `explosion-anim-explo1.gif` (9.3→...→161.9, plateaus). Tile
  animations (`tile-anim-temp.gif`) show NO such pattern (alpha correctly oscillates and hits exactly
  0 on the two real blank frames) — but since tile frames are fully opaque, this test can't rule out
  a shared root cause being invisible there; do not assume tiles are exempt without dedicated
  verification (`DOC-101`–`DOC-112`). All 129 animated GIFs need regeneration with fixed tooling —
  see §16 for the full task breakdown. Nothing in this "done" entry below is trustworthy for the
  actual `.gif` files; the frame-data research (which icons, which order, which sheet) is still
  correct and doesn't need redoing, only the final GIF assembly step.
  **Previous "done" note (2026-07-03), after being reopened once same day.** User caught a real
  completeness gap in the first "done" pass (kept below for history), then this was fixed by parsing
  `Tables::table_blupi[2911]` (`../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Tables.cpp`) directly
  and programmatically — a flat record list `{actionId, frameCount, holdLimit, icon_0..icon_N}`
  terminated by `actionId==0`, the same structure `Decor::BlupiSearchIcon()` (`Decor.cpp` ~line 2390)
  consumes at runtime. Result: **84 of the 87 real `BlupiAction` values now have a documented,
  GIF-illustrated animation** in `08-animations.md` §2, grouped into 23 categories (Core movement,
  Helicopter/Jeep/Tank/Skate/Surf/Nage modes, hazard contact, relief "Ouf" animations, Clear/erasing
  variants, etc.). 3 actions (`Set`=12, `Recedeq`=70, `Advanceq`=71) have no direct `table_blupi`
  record — noted honestly as "no record found", not guessed (mobile-eggbert's own doc comment
  suggests a stage-1 action-remapping step may redirect these before the table lookup runs; not
  traced in this pass). Channel selection (`blupi.png` vs `element.png`) and the `-1` "invisible
  frame" convention (same as `Temp` tile) were both confirmed directly from `BlupiSearchIcon()`, not
  assumed. **Independent verification**: the parsed data reproduces `GEBlupiController.cpp`'s
  existing `March`/`Jump`/`Air`/`SwimIdle`/`SwimMove` arrays byte-for-byte, confirming both the new
  parser and the original partial port are correct — the gap was purely that 82 other real actions
  were outside that port's scope. Whole-tree image-integrity check: 519 image references, 519 real
  files, zero orphans, zero broken links. Frame counts spot-checked with `identify` (Stop=330,
  March=6, Teleporte=128, all exact). **Explicitly out of scope for this fix, not done**: the 40
  `ObjectType` icon crops from `DOC-003`'s Category B that the user separately approved — still
  pending.
  ---
  Original (reopened) entry, kept for history:
  the Blupi section only covers the **8 `BlupiState` values `GEBlupiController.cpp` (galaxy-eggbert's
  own Simple3D port) implements** (Stop/March/Jump/Air/Down/Up/SwimIdle/SwimMove — 5 animated + 3
  static), not mobile-eggbert's real `BlupiAction` enum
  (`../mobile-eggbert/include/WindowsPhoneSpeedyBlupi/def/BlupiAction.hpp`), which has **87 real
  states** (`None`=0 + 87 actions 1–87): Stop/March/Turn/Jump/Air/Down/Up/Vertigo/Recede/Advance/
  Win/Push, plus full Stop/March/Turn triads per mode (Helico, Nage/swim, Surf, Jeep, Tank, Skate,
  Over/flattened, Ecrase/crushed), Drown, Glu, Electro, Charge, Teleporte, TakeDynamite/PutDynamite,
  8 `Clear1`–`8` variants, `Ouf1a`–`Ouf5` relief animations, `Mockery`/`Mockeryi`/`Mockeryp`, and
  more — see the file for the full list. **Root cause**: both the generating fork and the
  coordinator's own verification treated galaxy-eggbert's already-ported subset as if it were
  mobile-eggbert's complete state machine, instead of checking the real upstream enum directly —
  the same class of mistake as `DOC-003`'s sprite-channel bug (trusting a partial port as ground
  truth) but in the completeness-checking process itself, not just the data. Verification in this
  pass checked internal consistency (do frame/image counts add up) but not external completeness
  (does the *scope* match the real source) — a gap in the verification method itself, worth
  remembering for future `DOC-*` passes.
  Previous (incomplete) summary, kept for history: expanded `08-animations.md` from 31 to 71 animated sequences: 8
  explosion tables (`table_explo1`–`8`, transcribed from `Tables.cpp`, `-1` frames rendered
  transparent same as `Temp`'s vanish frames), a 10-frame door slide-up composite (positional, not
  a frame-cycle), and 10 GIFs for 9 of the 12 newly-supported `ObjectType`s (`19`/`46`/`55` are
  static icons, no animation; `96` has dormant+awake states = 2 GIFs). Applied `DOC-003`'s
  sprite-sheet correction: `32` (blupih) cropped from `blupi1.png`, `47` (chenille track) from
  `object-m.png`, not `element.png` — cross-checked the other 7 new types' real `channel=` field
  across all 78 world files and confirmed `element.png` (channel=10) is correct for those. Ran a
  full `Tables.cpp` sweep (grep for every `table_*` identifier) and categorized all ~100 found:
  covered / deferred-with-reason (mostly directional-variant tables for enemies whose one direction
  is already shown, and effects for `ObjectType`s confirmed vestigial-in-real-levels by `DOC-003`) /
  out-of-scope (non-visual parameter tables like `table_decor_quart`, `table_vitesse_*`,
  `table_training*`) — nothing silently dropped. **Real finding, not fixed here**: object animations
  (`GEDecorSystem::Update()`) have no throttled timer (unlike tiles/Blupi, which do) — `st.animPhase`
  increments every `Update()` call with no rate cap found in `GalaxyEggbertSimple3D`, so the "6 fps ÷
  divisor" duration figures for the original 14 object animations (and any new ones added this pass)
  are unconfirmed and likely off by ~10x if the real frame rate is closer to 60 fps; flagged
  prominently in `08-animations.md` rather than left as an uncorrected assumption. Verified: all 403
  image references across `mobile-eggbert-reference/` resolve to real files (zero orphans/broken
  links), all 19 new GIFs' frame counts checked with `identify` against the source table lengths
  (all exact matches).
- [ ] DOC-005 — Complete sound catalog. Currently a pointer only (`07-sounds.md`). List all 93
  `SoundChannel` entries with their actual in-game trigger/purpose (several are already known
  incidentally — e.g. channel 33 = door opening, per `06-doors.md` — collect these plus grep
  `Decor.cpp` for the rest).
- [ ] DOC-006 — Complete backgrounds catalog. Currently 3 of 38 background images thumbnailed
  (`05-backgrounds.md`), and the `region=` → filename selection code was not located. Find that
  mapping code (likely in higher-level UI/game-state code, not `Decor.cpp` itself — search for
  `PixmapChannel::Background` load sites), document it, and thumbnail the remaining 35 backgrounds.

## 16. Documentation rework (2026-07-03) — GIF ghosting bug + full completeness audit

**Nothing in `mobile-eggbert-reference/` is being treated as actually finished right now.** The user
inspected `08-animations.md`'s GIFs directly and found a real rendering bug: each frame keeps the
previous frame's opaque pixels instead of clearing before drawing the next one — visually, frame 2
shows frame 1 ghosted into its background, frame 3 shows frames 1+2, etc. Confirmed programmatically
(coalesce each GIF, measure mean alpha per frame): `blupi-action-02-march.gif` goes
80.0→85→86.2→86.3→86.6→86.7 and plateaus — a real walk cycle's alpha should fluctuate with the leg
positions, not monotonically converge. Same pattern in `object-anim-type05-treasure.gif`
(133.7→...→153.6, plateaus) and `explosion-anim-explo1.gif` (9.3→...→161.9, plateaus). Tile
animations (`tile-anim-temp.gif`) do NOT show this pattern — alpha correctly oscillates and hits
exactly 0 on the genuinely-blank frames — but since tile content is fully opaque, this test cannot
rule out the same root cause being invisible there, so tiles are not assumed exempt.

The user also asked, separately, whether `DOC-002`/`DOC-003`'s "complete" claims can be trusted at
all given this — see those entries above, now marked reopened: their *text/data* content (icon
classifications, frame tables, channel corrections) is not affected by a GIF-specific bug and
doesn't need redoing, but every generated *image file* is being re-verified rather than assumed
correct, given how many distinct bugs (channel mismatches, the Blupi partial-port gap, now this)
have turned up in this same effort so far.

**Per the user's explicit instruction**: this is broken into ~170 small, single-purpose tasks so the
rework can actually be done properly and incrementally, not batched into a few large opaque passes
the way the original `DOC-002`–`DOC-004` work was (which is part of how these gaps went undetected —
large multi-hundred-tool-call passes are hard to spot-check as thoroughly as many small ones).
`DOC-100` (the root-cause fix) blocks every GIF-regeneration task below it; nothing else in this
section has a hard ordering dependency, but doing the static-asset re-verification (§16.3) before or
alongside the GIF work is reasonable since neither blocks the other.

### 16.1 Root cause
- [x] DOC-100 — Diagnosed and fixed the GIF-assembly ghosting/disposal bug. **Root cause:** the
  prior workflow assembled frames with plain `convert -delay D -loop 0 frame*.png out.gif`, which
  leaves every frame's GIF disposal method as `Undefined` (`identify -format "%D"` on the existing
  broken GIFs confirms this). With disposal `Undefined`/`None`, a GIF decoder composites each new
  frame on top of whatever is still on the canvas from the previous frame instead of clearing to
  background first — so any frame with transparent/semi-transparent pixels lets prior opaque
  pixels bleed through, and per-frame mean alpha climbs (then plateaus once compounded pixels
  saturate at alpha=255), matching the originally-reported symptom exactly. **Fix:** add
  `-dispose Background` so each frame is cleared to a transparent canvas before the next is drawn.
  **Verified** with a synthetic repro (6 semi-transparent frames, same shape as a real sprite
  crop): the broken path reproduced the reported pattern (mean alpha 17.6→21.3→24.9→28.5→32.1→35.7,
  monotonic climb); the fixed path (`-dispose Background`) gave a flat 17.6 on every coalesced
  frame, i.e. each frame now matches its real source data with zero accumulation. Landed as a
  reusable wrapper, `mobile-eggbert-reference/tools/make-gif.sh` (`make-gif.sh <delay> <out.gif>
  <frame1.png> [frame2.png ...]`), so `DOC-101`–`DOC-267` use the fixed tool instead of ad hoc
  `convert` invocations (the ad hoc-command pattern is how the bug was introduced in the first
  place). Real sprite-sheet GIFs (`blupi-action-*`, `tile-anim-*`, etc.) are not regenerated by
  this task — that is `DOC-101` onward; this task only fixes and verifies the tool itself.

### 16.2 Regenerate animated GIFs with the fixed tooling (129 tasks, one per sequence)

Tile animations (12) — no ghosting symptom observed via alpha testing, but content is fully
opaque so the test can't rule out a shared root cause being invisible there; regenerate
defensively with the fixed tooling and re-verify rather than assuming these are exempt.

- [x] DOC-101 — Regenerated + verified `tile-anim-lava.gif` (animated tile: Lava). Frames are the 8
  real `kAnimLava` icons (`GETerrainRenderer.cpp`: `{68,69,70,71,72,71,70,69}`), cropped directly
  from `../mobile-eggbert/Content/icons/object-m.png` at `64x64`, assembled with the fixed
  `mobile-eggbert-reference/tools/make-gif.sh` at 17-centisecond delay (167 ms, matches
  `08-animations.md`'s existing table). **First crop attempt used the naive `col*64,row*64` pixel
  math and turned up a second, independent bug**: a thick blue bar bled in at the top of every
  frame and a vertical seam cut across the flame ~20% from the left (user caught this from a
  screenshot). Root cause + fix is the `S3D-2` gap/pitch bug above — `object-m.png` is packed on a
  65px pitch (64px + 1px gap), not flat 64px. **Final crop uses the corrected
  `1 + col*65, 1 + row*65` pixel math** (same formula now codified in `BlockTypes::tileUV()`) —
  clean frames, no bleed, no seam. **Verified**: coalesced-frame alpha-mean tracks the corrected
  per-frame source crops almost exactly (e.g. frame 4/icon 72, the one visually distinct frame:
  99.17 coalesced vs 99.00 source-crop; mirrored frames 1↔7, 2↔6, 3↔5 match each other within
  noise) — no monotonic climb, i.e. the DOC-100 ghosting fix holds on the corrected crops too. The
  **old** committed GIF, re-coalesced for comparison, was uniformly ~8-10 alpha points higher
  across all 8 frames than the (naive-crop) true source at the time — confirms the ghosting bug was
  present here too, just smaller in magnitude than the Blupi-sprite cases because lava tile content
  is mostly opaque (little transparent area to accumulate through), matching the
  `08-animations.md` caveat that this test "can't rule out the same root cause being invisible" on
  opaque tile content.
- [x] DOC-102 — Regenerated + verified `tile-anim-spike.gif` (animated tile: Spike). Frames are the
  16 real `kAnimSpike` icons (`GETerrainRenderer.cpp`:
  `{374,374,373,347,373,374,374,374,373,347,347,373,374,374,374,374}` — 3 distinct icons: 374
  extended, 373 mid, 347 retracted), cropped with the corrected `1 + col*65, 1 + row*65` pixel math
  (`S3D-2`) — this set stress-tested the gap fix at high row indices (row 17-18 of 22; the old
  buggy 64px-flat math would have been off by ~17-18px here), assembled with
  `mobile-eggbert-reference/tools/make-gif.sh`. **Verified**: coalesced-frame alpha-mean
  (252.88 for icon 374/373, 252.63 for icon 347) tracks the source crops (253.285/253.082) almost
  exactly with no monotonic climb; visually matches the old committed GIF's green-spike look (icon
  mapping confirmed correct), just without any bleed/seam.
- [x] DOC-103 — Regenerated + verified `tile-anim-crusher.gif` (animated tile: Crusher). Frames are
  the 10 real `kAnimCrusher` icons (`GETerrainRenderer.cpp`: `{317,317,318,319,320,321,322,323,
  323,323}`), cropped with the corrected `1+col*65,1+row*65` pixel math (`S3D-2`) and assembled
  with `make-gif.sh`. Stress-tested two more edge cases: icon 319 sits in the last column (col 19,
  x=1236, right edge at x=1300 — fits within `kSheetW`=1301 with exactly 1px to spare) and icon 320
  wraps to column 0 of the next row, both landing cleanly with no cropping error. **Verified**:
  coalesced-frame alpha-mean tracks the source crops closely (108.2→99.2→89.8→78.2→69.7→61.3→52.5,
  matching the real per-icon values 108.9→98.6→89.3→78.8→70.3→60.7→52.0) — this is a genuine
  content decrease (piston retracting, more transparent background revealed each frame), distinct
  in shape from the ghosting bug's spurious monotonic *increase*-then-plateau.
- [x] DOC-104 — Regenerated + verified `tile-anim-saw.gif` (animated tile: Saw). Frames are the 6
  real `kAnimSaw` icons (`GETerrainRenderer.cpp`: `{378,379,380,381,382,383}` — a rotating blade),
  cropped with the corrected `1+col*65,1+row*65` pixel math (`S3D-2`) and assembled with
  `make-gif.sh`. Another row-wrap edge case (icon 379 col 19 row 18 → icon 380 col 0 row 19), row
  19 of 22 — all clean. **Verified**: coalesced-frame alpha-mean (75.8/74.9/74.8/75.4/74.6/75.1)
  tracks the source crops (75.75/74.85/74.55/75.33/74.67/74.79) closely, roughly flat as expected
  for a symmetric rotating shape — no accumulation.
- [x] DOC-105 — Regenerated + verified `tile-anim-water1.gif` (animated tile: Water1). Found and
  fixed a **second, independent tooling bug** while doing this task: Water1's source icons (92-95)
  are genuinely translucent (alpha 0-127, never above ~50%, matching a see-through water surface).
  GIF only supports binary transparency, so ImageMagick's `convert` must collapse each pixel to
  fully transparent or fully opaque; for this near-uniform low-alpha content its automatic
  per-image heuristic collapsed the **entire frame** to a single fully-transparent color
  (`colors=1`) — confirmed the **already-committed** `tile-anim-water1.gif` had this exact defect
  too (pre-existing, not introduced this session). A near-identical tile, Water2 (similar ~100/255
  alpha), did *not* collapse — confirms the heuristic is inconsistent, not a clean/predictable
  threshold. **User decision (2026-07-04):** since GIF cannot represent partial alpha at all here
  regardless of approach, show the sprite's real saturated color (fully opaque wherever alpha>0)
  rather than pre-blending against an arbitrarily-chosen backdrop color (rejected alternative —
  the blended look would only be "correct" for one specific, made-up background color, not
  whatever the doc page actually renders against). Landed in `make-gif.sh`: each frame now gets
  `-channel A -threshold 1% +channel` before assembly. **Verified no regression on already-shipped
  GIFs**: re-ran `tile-anim-lava.gif`'s frames through the updated script — coalesced alpha-mean
  shifted by only ~2-3/255 (edge anti-aliasing very slightly harder), visually unchanged; the
  already-committed lava/spike/crusher/saw GIFs were left as-is (they never exhibited the
  collapse-to-one-color symptom, so redoing them isn't necessary). **Water1 verified**: `colors`
  per frame went from `1` (broken) to `89`-`100` (real content), coalesced alpha-mean ~241-246/255
  (near-fully-opaque, matching the forced-opaque intent), visually a clean, always-visible water
  surface with no black/blue bleed.
- [x] DOC-106 — Regenerated + verified `tile-anim-water2.gif` (animated tile: Water2). Frames are
  the 6 real `kAnimWater2` icons (`GETerrainRenderer.cpp`: `{91,96,97,98,97,96}`), cropped with the
  corrected pixel math and assembled with the (now translucency-aware, `DOC-105`) `make-gif.sh`.
  **Initially looked like a regression** — coalesced-frame alpha-mean came back as 0 despite
  `colors=42-48` — but this was a false alarm in the *test methodology*, not the image: unlike
  Water1, Water2's source crop has **no fully-transparent region at all** (alpha uniformly
  100-104/255 across the entire 64x64 cell — a full-bleed underwater fill, not a wave-shaped
  surface silhouette), so after the `DOC-105` opacity fix every pixel legitimately becomes fully
  opaque with no transparency left to report; when ImageMagick then writes that as a plain
  (alpha-free) indexed PNG for the coalesce-test, `%[fx:mean.a]` reports 0 for images with no
  alpha channel at all, which is where the false "0" came from. Confirmed by direct pixel query on
  the real output GIF (solid color at both center and corner, as expected for a full-tile fill) and
  a visual comparison to the previously-committed GIF (same solid light-blue look, no regression).
- [x] DOC-107 — Regenerated + verified `tile-anim-temp.gif` (animated tile: Temp). Frames are the
  20 real `kAnimTemp` values (`GETerrainRenderer.cpp`:
  `{328,328,327,327,326,326,325,325,324,324,325,325,326,326,327,329,328,328,-1,-1}`), 18 real
  icons cropped with the corrected pixel math + the 2 trailing `-1` sentinels rendered as genuinely
  blank 64×64 transparent frames (`convert -size 64x64 xc:none`, not cropped from the sheet —
  matches this file's own pre-existing documented convention that `-1` = a real invisible frame,
  not a skip), assembled with `make-gif.sh`. **Verified**: coalesced-frame alpha-mean traces a
  clean pulse (47.8→84.0→112.1→130.1→141.9→…→47.8→**0, 0**) matching the source icons' real alpha
  progression (46.5→79.6→…→137.4→…→46.5) with the two invisible frames correctly landing at exactly
  0 — no ghosting, no bleed on any of the 6 distinct sheet positions used.
- [x] DOC-108 — Regenerated + verified `tile-anim-marine.gif` (animated tile: Marine). Frames are
  the 11 real `kAnimMarine` icons (`GETerrainRenderer.cpp`: `{203,204,205,206,207,208,207,206,205,
  204,203}` — a seaweed/plant sway), cropped with the corrected pixel math and assembled with
  `make-gif.sh`. **Verified**: coalesced-frame alpha-mean is a clean mirrored pattern
  (63.1→61.1→59.5→58.6→58.3→59.5→58.3→58.6→59.5→61.1→63.1) matching the real per-icon values
  (53.4→52.8→50.9→49.9→50.1→51.9→50.1→49.9→50.9→52.8→53.4) — no ghosting, no bleed.
- [x] DOC-109 — Regenerated + verified `tile-anim-fanleft.gif` (animated tile: FanLeft). Frames are
  the 3 real icons (`GETerrainRenderer.cpp`: `FanLeft = 126 + (phase % 3)` → 126,127,128), cropped
  with the corrected pixel math and assembled with `make-gif.sh`. **Verified**: coalesced-frame
  alpha-mean (143.1/143.4/139.7) tracks the source crops (136.4/136.5/133.2) closely — no
  ghosting.
- [x] DOC-110 — Regenerated + verified `tile-anim-fanright.gif` (animated tile: FanRight). Frames
  are the 3 real icons (`GETerrainRenderer.cpp`: `FanRight = 129 + (phase % 3)` → 129,130,131),
  cropped with the corrected pixel math and assembled with `make-gif.sh`. **Verified**:
  coalesced-frame alpha-mean (144.4/144.4/140.8) tracks the source crops (137.9/138.2/134.8)
  closely — no ghosting.
- [x] DOC-111 — Regenerated + verified `tile-anim-fanup.gif` (animated tile: FanUp). Frames are
  the 3 real icons (`GETerrainRenderer.cpp`: `FanUp = 132 + (phase % 3)` → 132,133,134), cropped
  with the corrected pixel math and assembled with `make-gif.sh`. **Verified**: coalesced-frame
  alpha-mean (141.8/141.8/138.3) tracks the source crops (134.2/134.4/131.0) closely — no
  ghosting.
- [x] DOC-112 — Regenerated + verified `tile-anim-fandown.gif` (animated tile: FanDown, last of
  the 12 tile animations). Frames are the 3 real icons (`GETerrainRenderer.cpp`:
  `FanDown = 135 + (phase % 3)` → 135,136,137), cropped with the corrected pixel math and
  assembled with `make-gif.sh`. **Verified**: coalesced-frame alpha-mean (145.9/145.9/142.3)
  tracks the source crops (139.7/139.9/136.6) closely — no ghosting. All 12 tile animations
  (`DOC-101`-`DOC-112`) are now regenerated and verified; `DOC-113` onward moves to the 87 real
  Blupi action animations.

Blupi actions (84) — **confirmed ghosting**, all need regeneration.

- [x] DOC-113 — Regenerated + verified `blupi-action-01-stop.gif` (`BlupiAction::Stop`=1). First
  of the 84-Blupi-action batch — landed a new reusable tool,
  `mobile-eggbert-reference/tools/extract-blupi-action.py`, which reads
  `../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Tables.cpp` **live** at run time and parses
  `table_blupi`'s record format (`{actionId, frameCount, holdLimit, icons...}`, confirmed directly
  from `Decor::BlupiSearchIcon()`'s loop, `Decor.cpp` ~line 2393) — it does not copy/transcribe the
  table into galaxy-eggbert, same "read live, don't copy" pattern as `make-gif.sh` reading sprite
  sheets (per `CLAUDE.md`'s mobile-eggbert reuse rules). Action 1 has **330 real frames**
  (frameCount=330, holdLimit=0 — matches the already-committed GIF's frame count and 08-animations.md's
  documented table exactly, confirming both the parser and the pre-existing doc entry are correct),
  mostly icon 0 (Blupi's neutral standing pose on `blupi.png`, 60×60 cells, no gap — confirmed via
  mobile-eggbert's own `Pixmap::GetSrcRectangle`, `PixmapChannel::Blupi`: `srcGap=0`, so no
  gap-math needed here unlike `object-m.png`) interspersed with blink/idle-gesture icons (23, 133,
  135-138). Cropped all 330 frames, assembled with `make-gif.sh` at delay 13 (125 ms, matches the
  doc's confirmed 8fps Blupi tick). **Verified**: coalesced-frame alpha-mean spot-checked across
  the full cycle (frames 0,1,5,50,100,150,200,250,300,329) stays in a tight, non-monotonic
  ~79-86/255 range — no ghosting even across this much longer sequence.
- [x] DOC-114 — Regenerated + verified `blupi-action-02-march.gif` (`BlupiAction::March`=2). This
  is the exact GIF that first surfaced the `DOC-100` ghosting bug (originally reported climbing
  80.0→85→86.2→86.3→86.6→86.7 then plateauing). 6 real frames (icons 5-10, via
  `extract-blupi-action.py`), cropped from `blupi.png` (60×60, no gap) and assembled with
  `make-gif.sh`. **Verified**: coalesced-frame alpha-mean now fluctuates
  (85.9→81.8→83.7→85.4→81.9→83.9) matching the real per-icon values (80.5→76.9→78.7→80.2→76.8→78.5)
  — no monotonic climb, confirming `DOC-100`'s fix resolves the original bug report.
- [x] DOC-115 — Regenerated + verified `blupi-action-03-turn.gif` (`BlupiAction::Turn`=3). 6 real
  frames (icons `1,1,2,2,3,3`, via `extract-blupi-action.py`), cropped from `blupi.png` and
  assembled with `make-gif.sh`. **Verified**: coalesced-frame alpha-mean (86.3/86.3/87.1/87.1/
  86.1/86.1) exactly matches the paired-icon pattern of the source crops (79.7/79.7/78.6/78.6/
  79.3/79.3) — no accumulation.
- [x] DOC-116 — Regenerated + verified `blupi-action-04-jump.gif` (`BlupiAction::Jump`=4). 3 real
  frames (icons 17,18,19), cropped from `blupi.png` and assembled with `make-gif.sh`.
  **Verified**: coalesced-frame alpha-mean (76.8/79.6/80.1) tracks the source crops
  (72.0/74.5/75.1) closely — no ghosting.
- [x] DOC-117 — Regenerated + verified `blupi-action-05-air.gif` (`BlupiAction::Air`=5). 5 real
  frames (icons 169,26,170,170,27; `holdLimit=4` — a one-shot hold on the last frame in real
  gameplay, per `BlupiSearchIcon()`'s clamp logic, but shown as a looping 5-frame cycle here,
  matching this doc's existing convention of showing every real frame an action can produce, same
  as other already-cataloged holdLimit actions). Cropped from `blupi.png`, assembled with
  `make-gif.sh`. **Verified**: coalesced-frame alpha-mean (79.3/79.1/80.0/80.0/81.8) tracks the
  source crops (74.1/74.0/74.7/74.7/76.0) closely — no ghosting.
- [x] DOC-118 — Regenerated + verified `blupi-action-06-down.gif` (`BlupiAction::Down`=6). 3 real
  frames (icons 33,34,35), cropped from `blupi.png` and assembled with `make-gif.sh`.
  **Verified**: coalesced-frame alpha-mean (84.3/82.8/80.4) tracks the source crops
  (79.3/78.5/76.0) closely — no ghosting.
- [x] DOC-119 — Regenerated + verified `blupi-action-07-up.gif` (`BlupiAction::Up`=7). Single real
  frame (icon 44, `frameCount=1`), cropped from `blupi.png` and assembled with `make-gif.sh`
  (single-frame GIF, no ghosting possible by construction). **Verified**: coalesced alpha-mean
  (85.3) matches the source crop (80.0) — visually a clean "looking up" pose.
- [x] DOC-120 — Regenerated + verified `blupi-action-08-vertigo.gif` (`BlupiAction::Vertigo`=8).
  8 real frames (icons 0,169,26,170,27,171,28,172), cropped from `blupi.png` and assembled with
  `make-gif.sh`. **Verified**: coalesced-frame alpha-mean tracks the source crops closely
  (e.g. 79.9→80.0, 82.8→76.3) — no ghosting.
- [x] DOC-121 — Regenerated + verified `blupi-action-09-recede.gif` (`BlupiAction::Recede`=9). 6
  real frames (icons 5-10 — same icon set as `March`, direction is a separate flip flag, not a
  different icon set), cropped from `blupi.png` and assembled with `make-gif.sh`. **Verified**:
  coalesced-frame alpha-mean tracks the source crops closely — no ghosting.
- [x] DOC-122 — Regenerated + verified `blupi-action-10-advance.gif` (`BlupiAction::Advance`=10).
  6 real frames (icons 5-10, same icon set as `March`/`Recede`), cropped from `blupi.png` and
  assembled with `make-gif.sh`. **Verified**: coalesced-frame alpha-mean matches the same pattern
  as `March`/`Recede` — no ghosting.
- [x] DOC-123 — Regenerated + verified `blupi-action-11-clear1.gif` (`BlupiAction::Clear1`=11).
  70 real frames (icons 40-47, a "smoke puff" disappear effect), **first action in this batch using
  `element.png`** (not `blupi.png` — per the confirmed rule: only `Clear1`/`Clear2`/`Clear3`/`Glu`/
  `Electro` use `element.png`), cropped and assembled with `make-gif.sh`. **Verified**:
  coalesced-frame alpha-mean spot-checked across the cycle (77.3→77.3→69.6→21.4→37.4→40.9→40.9) is
  a real decreasing-then-settling dissipation pattern, not a ghosting climb-then-plateau — no bug.
- [x] DOC-124 — Regenerated + verified `blupi-action-13-win.gif` (`BlupiAction::Win`=13). 6 real
  frames (icons 41,41,42,42,43,43), cropped from `blupi.png` and assembled with `make-gif.sh`.
  **Verified**: coalesced-frame alpha-mean (84.6/84.6/86.4/86.4/87.6/87.6) matches the source
  crops' paired pattern with a gentle real trend, not a ghosting climb — no bug.
- [x] DOC-125 — Regenerated + verified `blupi-action-14-push.gif` (`BlupiAction::Push`=14). 6 real
  frames (icons 49-54), cropped from `blupi.png` and assembled with `make-gif.sh`. **Verified**:
  coalesced-frame alpha-mean fluctuates (82.1/80.6/79.9/82.8/80.3/81.0) matching the real per-icon
  values — no ghosting.
- [x] DOC-126 — Regenerated + verified `blupi-action-15-stophelico.gif` (`BlupiAction::StopHelico`=15).
  Single real frame (icon 61, `frameCount=1`), cropped from `blupi.png` and assembled with
  `make-gif.sh` (no ghosting possible with a single frame). **Verified**: coalesced alpha-mean
  matches the source crop.
- [x] DOC-127 — Regenerated + verified `blupi-action-16-marchhelico.gif` (`BlupiAction::MarchHelico`=16).
  8 real frames (icons 61,62,63,62,61,64,65,64), cropped from `blupi.png` and assembled with
  `make-gif.sh`. **Verified**: coalesced-frame alpha-mean is a clean mirrored pattern matching the
  source crops — no ghosting.
- [x] DOC-128 — Regenerated + verified `blupi-action-17-turnhelico.gif` (`BlupiAction::TurnHelico`=17).
  10 real frames (icons 71,71,72,72,73,73,74,74,75,75), cropped from `blupi.png` and assembled with
  `make-gif.sh`. **Verified**: coalesced-frame alpha-mean is an exact paired pattern matching the
  source crops — no ghosting.
- [x] DOC-129 — Regenerated + verified `blupi-action-18-stopnage.gif` (`BlupiAction::StopNage`=18).
  10 real frames (icons 76×6, 77×4), cropped from `blupi.png` and assembled with `make-gif.sh`.
  **Verified**: coalesced-frame alpha-mean is flat per icon (77.9 then 79.2) matching the source
  crops exactly — no ghosting.
- [x] DOC-130 — Regenerated + verified `blupi-action-19-marchnage.gif` (`BlupiAction::MarchNage`=19).
  14 real frames (icons 76-81 and 39, each paired), cropped from `blupi.png` and assembled with
  `make-gif.sh`. **Verified**: coalesced-frame alpha-mean is a clean paired pattern matching the
  source crops — no ghosting.
- [x] DOC-131 — Regenerated + verified `blupi-action-20-turnnage.gif` (`BlupiAction::TurnNage`=20).
  10 real frames (icons 88-92, each paired), cropped from `blupi.png` and assembled with
  `make-gif.sh`. **Verified**: coalesced-frame alpha-mean is a clean paired pattern matching the
  source crops — no ghosting.
- [x] DOC-132 — Regenerated + verified `blupi-action-21-stopsurf.gif` (`BlupiAction::StopSurf`=21).
  12 real frames (icons 93-98, each paired), cropped from `blupi.png` and assembled with
  `make-gif.sh`. **Verified**: coalesced-frame alpha-mean is a clean paired, gently-progressing
  pattern matching the source crops — no ghosting.
- [x] DOC-133 — Regenerated + verified `blupi-action-22-marchsurf.gif` (`BlupiAction::MarchSurf`=22).
  12 real frames (icons 93-98, same icon set as `StopSurf`), cropped from `blupi.png` and assembled
  with `make-gif.sh`. **Verified**: coalesced-frame alpha-mean matches `StopSurf`'s pattern exactly
  (same source icons) — no ghosting.
- [x] DOC-134 — Regenerated + verified `blupi-action-23-turnsurf.gif` (`BlupiAction::TurnSurf`=23).
  10 real frames (icons 105-109, each paired), cropped from `blupi.png` and assembled with
  `make-gif.sh`. **Verified**: coalesced-frame alpha-mean is a clean paired pattern matching the
  source crops — no ghosting.
- [x] DOC-135 — Regenerated + verified `blupi-action-24-drown.gif` (`BlupiAction::Drown`=24). 90
  real frames (icons in range 76-98, via `extract-blupi-action.py`), cropped from `blupi.png` and
  assembled with `make-gif.sh`. **Verified**: coalesced-frame alpha-mean spot-checked across the
  full cycle stays in a tight ~77.7-79.3/255 range — no ghosting even across this long sequence.
- [x] DOC-136 — Regenerated + verified `blupi-action-25-stopjeep.gif` (`BlupiAction::StopJeep`=25).
  8 real frames (icons 111,111,110,110,111,111,112,112), cropped from `blupi.png` and assembled
  with `make-gif.sh`. **Verified**: coalesced-frame alpha-mean is a clean paired pattern matching
  the source crops — no ghosting.
- [x] DOC-137 — Regenerated + verified `blupi-action-26-marchjeep.gif` (`BlupiAction::MarchJeep`=26).
  8 real frames (icons 111,111,110,110,111,111,112,112, same icon set as `StopJeep`), cropped from
  `blupi.png` and assembled with `make-gif.sh`. **Verified**: coalesced-frame alpha-mean matches
  `StopJeep`'s pattern exactly — no ghosting.
- [x] DOC-138 — Regenerated + verified `blupi-action-27-turnjeep.gif` (`BlupiAction::TurnJeep`=27).
  7 real frames (icons 112-118), cropped from `blupi.png` and assembled with `make-gif.sh`.
  **Verified**: coalesced-frame alpha-mean fluctuates matching the source crops — no ghosting.
- [x] DOC-139 — Regenerated + verified `blupi-action-28-stoppop.gif` (`BlupiAction::StopPop`=28).
  6 real frames (icons 122×3, 125×3), cropped from `blupi.png` and assembled with `make-gif.sh`.
  **Verified**: coalesced-frame alpha-mean is flat per icon matching the source crops — no
  ghosting.
- [x] DOC-140 — Regenerated + verified `blupi-action-29-pop.gif` (`BlupiAction::Pop`=29). 6 real
  frames (icons 126,125,124,123,122,121 — same set as `StopPop` but descending, one per frame),
  cropped from `blupi.png` and assembled with `make-gif.sh`. **Verified**: coalesced-frame
  alpha-mean fluctuates matching the source crops — no ghosting.
- [x] DOC-141 — Regenerated + verified `blupi-action-30-bye.gif` (`BlupiAction::Bye`=30). 12 real
  frames (icons 25,29,46,47,46,29 each paired), cropped from `blupi.png` and assembled with
  `make-gif.sh`. **Verified**: coalesced-frame alpha-mean is a clean paired pattern matching the
  source crops — no ghosting.
- [x] DOC-142 — Regenerated + verified `blupi-action-31-stopsuspend.gif` (`BlupiAction::StopSuspend`=31).
  328 real frames (icons in range 143-164, via `extract-blupi-action.py`), cropped from `blupi.png`
  and assembled with `make-gif.sh`. **Verified**: coalesced-frame alpha-mean spot-checked across
  the full cycle stays in a tight ~84.6-91.6/255 range — no ghosting even across this long
  sequence.
- [x] DOC-143 — Regenerated + verified `blupi-action-32-marchsuspend.gif` (`BlupiAction::MarchSuspend`=32).
  12 real frames (icons 143-146, each paired, in a rock-back-and-forth pattern), cropped from
  `blupi.png` and assembled with `make-gif.sh`. **Verified**: coalesced-frame alpha-mean is a
  clean paired pattern matching the source crops — no ghosting.
- [x] DOC-144 — Regenerated + verified `blupi-action-33-turnsuspend.gif` (`BlupiAction::TurnSuspend`=33).
  10 real frames (icons 151-155, each paired), cropped from `blupi.png` and assembled with
  `make-gif.sh`. **Verified**: coalesced-frame alpha-mean is a clean paired pattern matching the
  source crops — no ghosting.
- [x] DOC-145 — Regenerated + verified `blupi-action-34-jumpsuspend.gif` (`BlupiAction::JumpSuspend`=34).
  10 real frames (icons 157×2, 165×4, 166×4), cropped from `blupi.png` and assembled with
  `make-gif.sh`. **Verified**: coalesced-frame alpha-mean is flat per icon matching the source
  crops exactly — no ghosting.
- [x] DOC-146 — Regenerated + verified `blupi-action-35-hide.gif` (`BlupiAction::Hide`=35). 9 real
  frames (icons 276-284), cropped from `blupi.png` and assembled with `make-gif.sh`.
  **Verified**: coalesced-frame alpha-mean fluctuates matching the source crops — no ghosting.
- [x] DOC-147 — Regenerated + verified `blupi-action-36-jumpaie.gif` (`BlupiAction::JumpAie`=36).
  32 real frames (icons 135,177-180 mirrored, 135), cropped from `blupi.png` and assembled with
  `make-gif.sh`. **Verified**: coalesced-frame alpha-mean spot-checked across the cycle stays in a
  tight ~79.2-86.0/255 range — no ghosting.
- [x] DOC-148 — Regenerated + verified `blupi-action-37-stopskate.gif` (`BlupiAction::StopSkate`=37).
  140 real frames (icons in range 182-211, via `extract-blupi-action.py`), cropped from `blupi.png`
  and assembled with `make-gif.sh`. **Verified**: coalesced-frame alpha-mean spot-checked across
  the full cycle stays in a tight ~93.2-99.6/255 range — no ghosting even across this long
  sequence.
- [x] DOC-149 — Regenerated + verified `blupi-action-38-marchskate.gif` (`BlupiAction::MarchSkate`=38).
  96 real frames (icons from `blupi.png`, via `extract-blupi-action.py`), cropped and assembled
  with `make-gif.sh`. **Verified**: coalesced-frame alpha-mean spot-checked across the cycle stays
  flat/tight — no ghosting.
- [x] DOC-150 — Regenerated + verified `blupi-action-39-turnskate.gif` (`BlupiAction::TurnSkate`=39).
  14 real frames (icons 194-200, each paired), cropped from `blupi.png` and assembled with
  `make-gif.sh`. **Verified**: coalesced-frame alpha-mean is a clean paired pattern matching the
  source crops — no ghosting.
- [x] DOC-151 — Regenerated + verified `blupi-action-40-jumpskate.gif` (`BlupiAction::JumpSkate`=40).
  3 real frames (icons 210,211,212), cropped from `blupi.png` and assembled with `make-gif.sh`.
  **Verified**: coalesced-frame alpha-mean fluctuates matching the source crops — no ghosting.
- [x] DOC-152 — Regenerated + verified `blupi-action-41-airskate.gif` (`BlupiAction::AirSkate`=41).
  8 real frames (icons 213,214,215 mirrored), cropped from `blupi.png` and assembled with
  `make-gif.sh`. **Verified**: coalesced-frame alpha-mean is a clean paired pattern matching the
  source crops — no ghosting.
- [x] DOC-153 — Regenerated + verified `blupi-action-42-takeskate.gif` (`BlupiAction::TakeSkate`=42).
  20 real frames (icons 17,18,19,1,215,214,213,212,211,210, each paired), cropped from `blupi.png`
  and assembled with `make-gif.sh`. **Verified**: coalesced-frame alpha-mean is a clean paired
  pattern matching the source crops — no ghosting.
- [ ] DOC-154 — Regenerate + verify `blupi-action-43-deposeskate.gif` (`BlupiAction::DeposeSkate`=43).
- [ ] DOC-155 — Regenerate + verify `blupi-action-44-ouf1a.gif` (`BlupiAction::Ouf1a`=44).
- [ ] DOC-156 — Regenerate + verify `blupi-action-45-ouf1b.gif` (`BlupiAction::Ouf1b`=45).
- [ ] DOC-157 — Regenerate + verify `blupi-action-46-ouf2.gif` (`BlupiAction::Ouf2`=46).
- [ ] DOC-158 — Regenerate + verify `blupi-action-47-ouf3.gif` (`BlupiAction::Ouf3`=47).
- [ ] DOC-159 — Regenerate + verify `blupi-action-48-ouf4.gif` (`BlupiAction::Ouf4`=48).
- [ ] DOC-160 — Regenerate + verify `blupi-action-49-sucette.gif` (`BlupiAction::Sucette`=49).
- [ ] DOC-161 — Regenerate + verify `blupi-action-50-stoptank.gif` (`BlupiAction::StopTank`=50).
- [ ] DOC-162 — Regenerate + verify `blupi-action-51-marchtank.gif` (`BlupiAction::MarchTank`=51).
- [ ] DOC-163 — Regenerate + verify `blupi-action-52-turntank.gif` (`BlupiAction::TurnTank`=52).
- [ ] DOC-164 — Regenerate + verify `blupi-action-53-firetank.gif` (`BlupiAction::FireTank`=53).
- [ ] DOC-165 — Regenerate + verify `blupi-action-54-glu.gif` (`BlupiAction::Glu`=54).
- [ ] DOC-166 — Regenerate + verify `blupi-action-55-drink.gif` (`BlupiAction::Drink`=55).
- [ ] DOC-167 — Regenerate + verify `blupi-action-56-charge.gif` (`BlupiAction::Charge`=56).
- [ ] DOC-168 — Regenerate + verify `blupi-action-57-electro.gif` (`BlupiAction::Electro`=57).
- [ ] DOC-169 — Regenerate + verify `blupi-action-58-helicoglu.gif` (`BlupiAction::HelicoGlu`=58).
- [ ] DOC-170 — Regenerate + verify `blupi-action-59-turnair.gif` (`BlupiAction::TurnAir`=59).
- [ ] DOC-171 — Regenerate + verify `blupi-action-60-stopmarch.gif` (`BlupiAction::StopMarch`=60).
- [ ] DOC-172 — Regenerate + verify `blupi-action-61-stopjump.gif` (`BlupiAction::StopJump`=61).
- [ ] DOC-173 — Regenerate + verify `blupi-action-62-stopjumph.gif` (`BlupiAction::StopJumph`=62).
- [ ] DOC-174 — Regenerate + verify `blupi-action-63-mockery.gif` (`BlupiAction::Mockery`=63).
- [ ] DOC-175 — Regenerate + verify `blupi-action-64-mockeryi.gif` (`BlupiAction::Mockeryi`=64).
- [ ] DOC-176 — Regenerate + verify `blupi-action-65-ouf5.gif` (`BlupiAction::Ouf5`=65).
- [ ] DOC-177 — Regenerate + verify `blupi-action-66-balloon.gif` (`BlupiAction::Balloon`=66).
- [ ] DOC-178 — Regenerate + verify `blupi-action-67-stopover.gif` (`BlupiAction::StopOver`=67).
- [ ] DOC-179 — Regenerate + verify `blupi-action-68-marchover.gif` (`BlupiAction::MarchOver`=68).
- [ ] DOC-180 — Regenerate + verify `blupi-action-69-turnover.gif` (`BlupiAction::TurnOver`=69).
- [ ] DOC-181 — Regenerate + verify `blupi-action-72-stopecrase.gif` (`BlupiAction::StopEcrase`=72).
- [ ] DOC-182 — Regenerate + verify `blupi-action-73-marchecrase.gif` (`BlupiAction::MarchEcrase`=73).
- [ ] DOC-183 — Regenerate + verify `blupi-action-74-teleporte.gif` (`BlupiAction::Teleporte`=74).
- [ ] DOC-184 — Regenerate + verify `blupi-action-75-clear2.gif` (`BlupiAction::Clear2`=75).
- [ ] DOC-185 — Regenerate + verify `blupi-action-76-clear3.gif` (`BlupiAction::Clear3`=76).
- [ ] DOC-186 — Regenerate + verify `blupi-action-77-clear4.gif` (`BlupiAction::Clear4`=77).
- [ ] DOC-187 — Regenerate + verify `blupi-action-78-clear5.gif` (`BlupiAction::Clear5`=78).
- [ ] DOC-188 — Regenerate + verify `blupi-action-79-clear6.gif` (`BlupiAction::Clear6`=79).
- [ ] DOC-189 — Regenerate + verify `blupi-action-80-clear7.gif` (`BlupiAction::Clear7`=80).
- [ ] DOC-190 — Regenerate + verify `blupi-action-81-clear8.gif` (`BlupiAction::Clear8`=81).
- [ ] DOC-191 — Regenerate + verify `blupi-action-82-switch.gif` (`BlupiAction::Switch`=82).
- [ ] DOC-192 — Regenerate + verify `blupi-action-83-mockeryp.gif` (`BlupiAction::Mockeryp`=83).
- [ ] DOC-193 — Regenerate + verify `blupi-action-84-non.gif` (`BlupiAction::Non`=84).
- [ ] DOC-194 — Regenerate + verify `blupi-action-85-slowdownskate.gif` (`BlupiAction::SlowdownSkate`=85).
- [ ] DOC-195 — Regenerate + verify `blupi-action-86-takedynamite.gif` (`BlupiAction::TakeDynamite`=86).
- [ ] DOC-196 — Regenerate + verify `blupi-action-87-putdynamite.gif` (`BlupiAction::PutDynamite`=87).

Object/pickup/enemy animations (24) — **confirmed ghosting** (spot-checked via `treasure`),
all need regeneration.

- [ ] DOC-197 — Regenerate + verify `object-anim-type02-patrolA.gif`.
- [ ] DOC-198 — Regenerate + verify `object-anim-type03-patrolB.gif`.
- [ ] DOC-199 — Regenerate + verify `object-anim-type04-bulldozer.gif`.
- [ ] DOC-200 — Regenerate + verify `object-anim-type05-treasure.gif`.
- [ ] DOC-201 — Regenerate + verify `object-anim-type06-egg.gif`.
- [ ] DOC-202 — Regenerate + verify `object-anim-type07-exit.gif`.
- [ ] DOC-203 — Regenerate + verify `object-anim-type16-spider.gif`.
- [ ] DOC-204 — Regenerate + verify `object-anim-type17-fish.gif`.
- [ ] DOC-205 — Regenerate + verify `object-anim-type20-bird.gif`.
- [ ] DOC-206 — Regenerate + verify `object-anim-type21-secretexit.gif`.
- [ ] DOC-207 — Regenerate + verify `object-anim-type24-skateboard.gif`.
- [ ] DOC-208 — Regenerate + verify `object-anim-type25-shield.gif`.
- [ ] DOC-209 — Regenerate + verify `object-anim-type26-suctioncup.gif`.
- [ ] DOC-210 — Regenerate + verify `object-anim-type32-blupih.gif`.
- [ ] DOC-211 — Regenerate + verify `object-anim-type33-blupit.gif`.
- [ ] DOC-212 — Regenerate + verify `object-anim-type40-mirrorinvert.gif`.
- [ ] DOC-213 — Regenerate + verify `object-anim-type44-wasp.gif`.
- [ ] DOC-214 — Regenerate + verify `object-anim-type47-chenille.gif`.
- [ ] DOC-215 — Regenerate + verify `object-anim-type49-key1.gif`.
- [ ] DOC-216 — Regenerate + verify `object-anim-type50-key2.gif`.
- [ ] DOC-217 — Regenerate + verify `object-anim-type51-key3.gif`.
- [ ] DOC-218 — Regenerate + verify `object-anim-type54-largecreature.gif`.
- [ ] DOC-219 — Regenerate + verify `object-anim-type96-follower-awake.gif`.
- [ ] DOC-220 — Regenerate + verify `object-anim-type96-follower-dormant.gif`.

Explosions (8) — **confirmed ghosting** (spot-checked via `explo1`), all need regeneration.

- [ ] DOC-221 — Regenerate + verify `explosion-anim-explo1.gif` (`table_explo1`, real per-icon size from `table_explo_size`).
- [ ] DOC-222 — Regenerate + verify `explosion-anim-explo2.gif` (`table_explo2`, real per-icon size from `table_explo_size`).
- [ ] DOC-223 — Regenerate + verify `explosion-anim-explo3.gif` (`table_explo3`, real per-icon size from `table_explo_size`).
- [ ] DOC-224 — Regenerate + verify `explosion-anim-explo4.gif` (`table_explo4`, real per-icon size from `table_explo_size`).
- [ ] DOC-225 — Regenerate + verify `explosion-anim-explo5.gif` (`table_explo5`, real per-icon size from `table_explo_size`).
- [ ] DOC-226 — Regenerate + verify `explosion-anim-explo6.gif` (`table_explo6`, real per-icon size from `table_explo_size`).
- [ ] DOC-227 — Regenerate + verify `explosion-anim-explo7.gif` (`table_explo7`, real per-icon size from `table_explo_size`).
- [ ] DOC-228 — Regenerate + verify `explosion-anim-explo8.gif` (`table_explo8`, real per-icon size from `table_explo_size`).

Door (1) — same DOC-004 pass as explosions, assume affected until verified.

- [ ] DOC-229 — Regenerate + verify `door-slide.gif` (10-frame positional composite).
### 16.3 Static (non-animated) asset re-verification

Not confirmed broken (single-frame images have no compositing/disposal to go wrong), but
given the repeated sprite-channel bugs already found this session (11 `ObjectType`s drawing
from the wrong sheet), these deserve a real re-check, not an assumption they're fine.

- [ ] DOC-230 — Re-verify all 313 `tile-full-*.png` crops: correct grid math, correct source pixels, no off-by-one against the 65px-stride formula (spot-check at minimum the first/last icon of each row boundary).
- [ ] DOC-231 — Re-verify all 67 `object-type*.png` static icons' sprite-sheet channel against `Decor.cpp`'s real `channel=`/`BlupiSearchIcon()`-equivalent logic per ID — do not assume the 11 corrections already found are the only ones; check every single one.
- [ ] DOC-232 — Re-verify the 4 `blupi-icon*.png` representative frames (§4.2 of `03-objects.md`) are still accurate now that the full `table_blupi` parse exists — pick more meaningful representative frames if the originals (icons 0/1/5/10) don't actually represent real named states well.
- [ ] DOC-233 — Re-verify the 3 `bg-decor*.png` background thumbnails render correctly and are the correct real files (not off-by-one in the `decorNNN` numbering).
- [ ] DOC-234 — Re-verify the 3 door crops (`tile-334/335/336-Door*.png`) are pixel-correct against the current (fixed) tile-grid formula.

### 16.4 DOC-005 — Complete sound catalog (93 channels), broken into batches

- [ ] DOC-235 — Research and document `SoundChannel` 0-9 (or however mobile-eggbert's own channel numbering starts) — real in-game trigger/purpose per channel, grep `Decor.cpp` for `SoundChannel` usage.
- [ ] DOC-236 — Research and document `SoundChannel` 10-19 — real in-game trigger/purpose per channel, grep `Decor.cpp` for `SoundChannel` usage.
- [ ] DOC-237 — Research and document `SoundChannel` 20-29 — real in-game trigger/purpose per channel, grep `Decor.cpp` for `SoundChannel` usage.
- [ ] DOC-238 — Research and document `SoundChannel` 30-39 — real in-game trigger/purpose per channel, grep `Decor.cpp` for `SoundChannel` usage.
- [ ] DOC-239 — Research and document `SoundChannel` 40-49 — real in-game trigger/purpose per channel, grep `Decor.cpp` for `SoundChannel` usage.
- [ ] DOC-240 — Research and document `SoundChannel` 50-59 — real in-game trigger/purpose per channel, grep `Decor.cpp` for `SoundChannel` usage.
- [ ] DOC-241 — Research and document `SoundChannel` 60-69 — real in-game trigger/purpose per channel, grep `Decor.cpp` for `SoundChannel` usage.
- [ ] DOC-242 — Research and document `SoundChannel` 70-79 — real in-game trigger/purpose per channel, grep `Decor.cpp` for `SoundChannel` usage.
- [ ] DOC-243 — Research and document `SoundChannel` 80-89 — real in-game trigger/purpose per channel, grep `Decor.cpp` for `SoundChannel` usage.
- [ ] DOC-244 — Research and document `SoundChannel` 90-92 (final batch) — real in-game trigger/purpose per channel.
- [ ] DOC-245 — Cross-check the completed per-channel catalog against `include/GalaxyEggbert/def/SoundChannel.hpp`'s existing names/comments for consistency; flag and resolve any mismatch.
- [ ] DOC-246 — Verify all 93 `.wav` files in `../mobile-eggbert/Content/sounds/` are accounted for 1:1 against the 93 documented channels (no gaps, no extras).

### 16.5 DOC-006 — Complete backgrounds catalog (38 images) + resolve region= mapping

- [ ] DOC-247 — Locate the real `region=` → `decorNNN.png` selection code (search UI/game-state code beyond `Decor.cpp` for `PixmapChannel::Background` load sites) — this is the single most important unresolved fact in this file.
- [ ] DOC-248 — Once the mapping is found, verify it against every real level file's actual `region=` value (all 78 files) to confirm the mapping is exhaustive and correct.
- [ ] DOC-249 — Thumbnail + verify background images `decor000`-`decor003` (skip any confirmed-missing IDs in that range).
- [ ] DOC-250 — Thumbnail + verify background images `decor004`-`decor007` (skip any confirmed-missing IDs in that range).
- [ ] DOC-251 — Thumbnail + verify background images `decor008`-`decor011` (skip any confirmed-missing IDs in that range).
- [ ] DOC-252 — Thumbnail + verify background images `decor012`-`decor015` (skip any confirmed-missing IDs in that range).
- [ ] DOC-253 — Thumbnail + verify background images `decor016`-`decor019` (skip any confirmed-missing IDs in that range).
- [ ] DOC-254 — Thumbnail + verify background images `decor020`-`decor023` (skip any confirmed-missing IDs in that range).
- [ ] DOC-255 — Thumbnail + verify background images `decor024`-`decor027` (skip any confirmed-missing IDs in that range).
- [ ] DOC-256 — Thumbnail + verify background images `decor028`-`decor031` (skip any confirmed-missing IDs in that range).
- [ ] DOC-257 — Document `blupiyoupie.png`/`gear.png` (title/settings-screen backgrounds) separately from level backgrounds, with their real trigger context.

### 16.6 Markdown file completeness re-review (one real read-through per file)

- [ ] DOC-258 — Full read-through of `00-overview.md` after all the above tasks land: confirm every image reference is current (not pointing at a since-regenerated/renamed file), confirm the status note at the top matches true state, confirm no stale claims survived from before this rework.
- [ ] DOC-259 — Full read-through of `01-world-file-format.md` after all the above tasks land: confirm every image reference is current (not pointing at a since-regenerated/renamed file), confirm the status note at the top matches true state, confirm no stale claims survived from before this rework.
- [ ] DOC-260 — Full read-through of `02-tiles.md` after all the above tasks land: confirm every image reference is current (not pointing at a since-regenerated/renamed file), confirm the status note at the top matches true state, confirm no stale claims survived from before this rework.
- [ ] DOC-261 — Full read-through of `03-objects.md` after all the above tasks land: confirm every image reference is current (not pointing at a since-regenerated/renamed file), confirm the status note at the top matches true state, confirm no stale claims survived from before this rework.
- [ ] DOC-262 — Full read-through of `04-enemy-behavior.md` after all the above tasks land: confirm every image reference is current (not pointing at a since-regenerated/renamed file), confirm the status note at the top matches true state, confirm no stale claims survived from before this rework.
- [ ] DOC-263 — Full read-through of `05-backgrounds.md` after all the above tasks land: confirm every image reference is current (not pointing at a since-regenerated/renamed file), confirm the status note at the top matches true state, confirm no stale claims survived from before this rework.
- [ ] DOC-264 — Full read-through of `06-doors.md` after all the above tasks land: confirm every image reference is current (not pointing at a since-regenerated/renamed file), confirm the status note at the top matches true state, confirm no stale claims survived from before this rework.
- [ ] DOC-265 — Full read-through of `07-sounds.md` after all the above tasks land: confirm every image reference is current (not pointing at a since-regenerated/renamed file), confirm the status note at the top matches true state, confirm no stale claims survived from before this rework.
- [ ] DOC-266 — Full read-through of `08-animations.md` after all the above tasks land: confirm every image reference is current (not pointing at a since-regenerated/renamed file), confirm the status note at the top matches true state, confirm no stale claims survived from before this rework.
- [ ] DOC-267 — Full read-through of `09-open-questions.md` after all the above tasks land: confirm every image reference is current (not pointing at a since-regenerated/renamed file), confirm the status note at the top matches true state, confirm no stale claims survived from before this rework.

---

*Total tasks: ~820 (~650 gameplay/engine + ~168 documentation-rework tasks in §16, `DOC-100`–`DOC-267`). Sections by size: Sound (80), Blupi (150), Menu (102), Pickups (90), Tiles (52), Enemy (40), HUD (26), Score (20), Camera (17), Save (14), Visual (25), Build (10), Tests (7), S3D milestones (22), Documentation rework (168).*
