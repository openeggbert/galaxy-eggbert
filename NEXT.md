# NEXT.md — Galaxy Eggbert

## 1. Project summary

**Galaxy Eggbert** is a faithful 3D remake of **mobile-eggbert** (a C++ port of *Speedy Blupi*, a
Windows Phone XNA game from 2013). Main goal: reimplement the same game logic, levels, and assets
in 3D — perspective camera, billboard sprites, 3D-rendered tiles — without inventing new mechanics.

**Current development phase:** mid-migration between two build targets:

- `GalaxyEggbertSimple3D` (built on `simple-3d` → U3D/Urho3D) — the **only playable** target,
  feature-complete enough to play through core mechanics end-to-end (see §2).
- `GalaxyEggbertCNA` (built directly on **CNA** + **Easy3D** helper library) — the **new
  long-term target**. Currently: opens a window, copies mobile-eggbert assets next to itself,
  parses one world file, and renders real, textured, static terrain (one cube per non-air world
  cell, using `object-m.png`). No Blupi, no objects/pickups, no animated tiles, no HUD, no sound,
  no gameplay yet.

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
- Mobile-eggbert's world files are flat 2D data (Y=0 everywhere). There is **no** and will be
  **no** automated `.txt → .vwr` converter that "promotes" a 2D level into a 3D one. The existing
  approach (parse the flat 2D layout, render it with 3D tech: cubes, camera, billboards) is
  correct and stays. Genuinely 3D-designed levels are separate future hand-authored work.
- New CPU-side mesh builders and CNA renderer adapters (e.g. `Easy3D::CubeMesh`,
  `Easy3D::CubeMeshRenderer`) live inside `../easy-3d` itself, not as galaxy-eggbert-local
  adapters — decided because that work is generic 3D-batching plumbing with zero Eggbert-specific
  knowledge, matching Easy3D's own stated role. Modifying `../easy-3d` still requires explicit,
  per-change user approval; it is not blanket-authorized.

## 2. Current status

### Build status
- `GalaxyEggbertSimple3D` — **builds clean** (`cmake -S . -B build && cmake --build build -j2`).
- `GalaxyEggbertWorldsTests` — **54/54 tests pass** (`ctest --test-dir build`).
- `GalaxyEggbertCNA` — **builds clean** (opt-in: `-DGALAXY_EGGBERT_BUILD_CNA=ON`), **runs and
  renders real, textured terrain**.
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
  `../mobile-eggbert/Content/` and `worlds/` next to its binary at build time; parses
  `worlds/world001.txt` into the shared `GalaxyEggbert::Worlds::World`; maps block types to
  `object-m.png` UV rects (`GETileAtlas`); walks the loaded world and renders one textured cube
  per non-air cell (`GETerrainRenderer` + `Easy3D::CubeMeshRenderer`), textured with a real CNA
  `Texture2D` of `object-m.png`. Verified by a real run: `594 blocks, 14256 vertices, 7128
  triangles` uploaded; a 5x5 on-screen pixel sample found 13/25 sampled points showing terrain
  color with 5 distinct colors among them (confirms real texture sampling, not a placeholder).
- **`Easy3D::CubeMesh`** (`AppendCubeMesh`/`BuildCubeMesh`): turns `CubeBatch` items into
  vertex/index arrays (24 vertices + 36 indices per cube).
- **`Easy3D::CubeMeshRenderer`**: uploads that data to CNA `VertexBuffer`/`IndexBuffer` once and
  issues `DrawIndexedPrimitives` via a caller-configured `BasicEffect` — the real CNA draw path,
  matching CNA's own `examples/house3d_demo.cpp` pattern.

### What does not work yet
- `GalaxyEggbertCNA`: no Blupi rendering, no object/pickup rendering, no animated tiles
  (lava/crusher/saw/water/etc. — terrain is currently fully static), no HUD, no sound, no
  gameplay, no input beyond default window handling. `Easy3D::BillboardBatch`/`DebugDraw` have no
  vertex builder or renderer adapter at all yet (needed for Blupi/objects).
- `GalaxyEggbertSimple3D`: camera shake is a no-op; no per-zone fog; Android/Web builds untested
  since the last engine change.

## 3. Recent changes

Most recent first. None of this is committed yet in either repository (see §4/§9).

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

Files added this migration and not yet committed: `easy3d.md`; `src/GalaxyEggbertCNA/` (`main.cpp`,
`GalaxyEggbertCnaGame.hpp/.cpp`, `Game/GEWorldRuntime.hpp/.cpp`, `Game/GETileAtlas.hpp/.cpp`,
`Game/GETerrainRenderer.hpp/.cpp`); in `../easy-3d`: `include/Easy3D/CubeMesh.hpp`,
`include/Easy3D/CubeMeshRenderer.hpp`, `src/CubeMesh.cpp`, `src/CubeMeshRenderer.cpp`,
`tests/test_cube_mesh.cpp`, `tests/test_cube_mesh_renderer.cpp`, plus doc/CMake edits.
`src/GalaxyEggbertSimple3D/` was not touched. `../mobile-eggbert`, `../cna`, and `../simple-3d`
remain untouched/read-only. `../easy-3d` was modified twice this migration, **each time with
explicit user approval** — any further edits there need their own approval too.

## 4. Current blocker / main problem

**No blocker.** Real, textured, non-animated terrain renders end-to-end from the actual loaded
world file — `GEWorldRuntime` → `GETerrainRenderer` → `Easy3D::CubeMesh`/`CubeMeshRenderer` +
`Texture2D` → visible, textured pixels on screen, verified by geometry counts and by on-screen
color sampling. The remaining gaps (animated tiles, Blupi, objects, HUD, sound) are new
capabilities to build, not bugs to fix.

The one open practical note: **nothing described in this document is committed** in either
`galaxy-eggbert` or `../easy-3d`. A future session should confirm with the user before committing,
and should re-check `git status`/`git log` in both repos first, since this environment has shown
other concurrent sessions landing changes in sibling repos (e.g. the `../cna` fix above).

## 5. Known bugs and limitations

| Status | Issue |
|---|---|
| confirmed | Simple3D: `GECameraRig::StartShake()` is a no-op (Simple3D has no camera-offset API) |
| confirmed, environment-specific | `ctest` does not discover `GalaxyEggbertWorldsTests` when configured in the pre-existing `cmake-build-debug` CLion profile (binary runs fine manually). Not reproduced in a fresh `build/` directory — `ctest --test-dir build` correctly finds and runs all 54 tests there. Likely a stale/IDE-specific config issue in `cmake-build-debug`, not a general CMake problem. |
| incomplete | Simple3D: no per-zone fog, only `SetClearColor` per sky region |
| incomplete | `GalaxyEggbertCNA`: no Blupi/object rendering, no animated tiles, no HUD, no sound, no gameplay (expected at this phase, not a bug) |
| unknown | Simple3D Android/Web builds untested since the last engine change |
| unknown | `GalaxyEggbertCNA`'s clean-exit-on-window-close path was not separately exercised — only a forced external `timeout`/kill was tested during verification |
| needs verification | Simple3D: stomp bounce height (`kJumpSpeed * 0.65f`) — does it match mobile-eggbert's feel? |
| needs verification | Simple3D: crate push floor-support check only tested at y=0; stacked crates (y=1) untested |
| risky assumption | `GalaxyEggbertCNA`'s world loader uses a relative path (`"worlds/world001.txt"`, `"Content/icons/object-m.png"`) — only works if the binary is run from its own build directory; fails silently (world) or presumably throws (texture) otherwise |
| risky assumption | `GETerrainRenderer` rebuilds nothing after construction — fine for the current static-only phase, but animated tiles (§8 task 1) will need either a rebuild-per-frame path or a shader-side animation approach; not yet decided |

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
                                owns Game/GEWorldRuntime (mobile-eggbert .txt parser),
                                Game/GETileAtlas (block type → UV rect), Game/GETerrainRenderer
                                (World → one CubeMeshRenderer-backed mesh for all non-air blocks),
                                an Easy3D::Camera3D (aimed at the terrain's block centroid), a CNA
                                Texture2D of object-m.png, and a BasicEffect bound to it.

../easy-3d/                  — companion library beside CNA (not touched by default; two
                                approved edits this migration). Camera3D/OrbitCamera/FollowCamera,
                                TextureAtlas, BillboardBatch/CubeBatch/DebugDraw (CPU-side item
                                queues), CubeMesh (vertex/index builder), CubeMeshRenderer (CNA
                                draw-call adapter). Billboard/DebugDraw builders+adapters: not
                                started.
```

### Data flow (both targets read the same file format)
```
worlds/worldXXX.txt (mobile-eggbert format, header + Decor: grid [+ MoveObject: lines])
  Simple3D: → GEWorldRuntime::LoadFromMobileEggbertFile() → World + MobileObjSpec list + blupiSpawn
            → GETerrainRenderer / GEDecorSystem / GEBlupiController (renders everything)
  CNA:      → GEWorldRuntime::LoadFromMobileEggbertFile() → World (MoveObject: lines still ignored
              — that's a later phase) → GETerrainRenderer → Easy3D::CubeBatch/CubeMesh/
              CubeMeshRenderer + Texture2D(object-m.png) (renders static, textured terrain; no
              Blupi/object rendering yet).
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
#   "GalaxyEggbertCNA: loaded worlds/world001.txt — spawn tile (12, 92), sky region 0, 594 non-air blocks."
#   GETileAtlas UV diagnostics (Ground/Lava/Wall)
#   "GalaxyEggbertCNA: terrain texture loaded — 1301x1431 px."
#   "GalaxyEggbertCNA: terrain mesh uploaded — 594 blocks, 14256 vertices, 7128 triangles."
#   "GalaxyEggbertCNA: terrain visibility check — N/25 sampled screen points show non-background (terrain) color, M distinct color(s) among them..."
# A window opens showing textured cube terrain (object-m.png tiles) from an angled overhead view.

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

1. **Add animated-tile support** (lava/crusher/saw/spike/water/fan/marine/temp) — using
   mobile-eggbert's frame tables and the Simple3D port's existing behavior as reference (do not
   copy code/data without approval). This is the main remaining terrain gap.
   **Files:** `src/GalaxyEggbertCNA/Game/GETerrainRenderer.hpp/.cpp` (currently build-once; will
   need either a per-frame partial rebuild for animated-tile groups, or a different mechanism —
   worth a short design pass before implementing, since it's not yet decided how).
   **Verification:** run `GalaxyEggbertCNA`; lava/water tiles visibly animate over time.
2. **Fix `ctest` discovery in the `cmake-build-debug` profile** — investigate why
   `gtest_discover_tests` doesn't find `GalaxyEggbertWorldsTests` there (works fine in a fresh
   `build/` dir). **Files:** `CMakeLists.txt`, `cmake-build-debug/` config.
   **Verification:** `ctest --test-dir cmake-build-debug -R GalaxyEggbert` reports 54 passed.
3. **Verify `GalaxyEggbertCNA`'s clean-exit path** — close the window via the window manager
   (not a forced kill) and confirm the process exits 0 with no leaked resources.
   **Files:** none expected — diagnostic verification only, possibly add an `OnExiting` log line
   to `GalaxyEggbertCnaGame` if useful. **Verification:** manual run + exit code check.
4. **Simple3D camera shake** — make `GECameraRig::StartShake()` produce visible jitter on
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
- No committing anything without asking first — the user has not requested a commit for this
  migration's work yet (see §4).

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
