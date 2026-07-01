# NEXT.md — Galaxy Eggbert

## 1. Project summary

**Galaxy Eggbert** is a faithful 3D remake of **mobile-eggbert** (a C++ port of *Speedy Blupi*, a
Windows Phone XNA game from 2013). Main goal: reimplement the same game logic, levels, and assets
in 3D — perspective camera, billboard sprites, 3D-rendered tiles — without inventing new mechanics.

**Current development phase:** mid-migration between two build targets:

- `GalaxyEggbertSimple3D` (built on `simple-3d` → U3D/Urho3D) — the **only playable** target,
  feature-complete enough to play through core mechanics end-to-end (see §2).
- `GalaxyEggbertCNA` (built directly on **CNA** + **Easy3D** helper library) — the **new
  long-term target**, currently a skeleton: opens a window, copies mobile-eggbert assets next to
  itself, parses one world file. No rendering, no gameplay yet.

**Important architectural decisions (all recorded in `plan.md`/`easy3d.md`/`CLAUDE.md`):**

- Direct CNA + Easy3D supersedes the old Simple3D → U3D → Nova3D direction as the long-term
  target. `GalaxyEggbertSimple3D` is **not being deleted** — it stays as the working reference
  until the CNA/Easy3D path reaches feature parity with it.
- Easy3D is a small helper library beside CNA (cameras, texture atlas, batching) — it must not
  hide CNA; game code may call CNA directly.
- `../mobile-eggbert` is **read-only**. Its assets (PNGs, sounds, world files) are freely reused;
  its code/data (tables, enum values, `Decor.cpp` logic) are reference-only — copying/adapting
  requires explicit user approval first.
- Mobile-eggbert's world files are flat 2D data (Y=0 everywhere). There is **no** and will be
  **no** automated `.txt → .vwr` converter that "promotes" a 2D level into a 3D one — that
  produces empty, unplayable geometry. The existing approach (parse the flat 2D layout, render it
  with 3D tech: cubes, camera, billboards) is correct and stays. Genuinely 3D-designed levels are
  separate future hand-authored work, not derived from mobile-eggbert.

## 2. Current status

### Build status
- `GalaxyEggbertSimple3D` — **builds clean** (`cmake -S . -B build && cmake --build build -j2`).
- `GalaxyEggbertWorldsTests` — **54/54 tests pass** (`ctest --test-dir build`).
- `GalaxyEggbertCNA` — **builds clean** (opt-in: `-DGALAXY_EGGBERT_BUILD_CNA=ON`), **runs**
  (opens an SDL/EasyGL window, clears to a solid color, prints a world-load diagnostic to stdout,
  exits without crashing under a forced kill).

### What works
- **`GalaxyEggbertSimple3D`** (full details unchanged from before this migration): world loading
  from mobile-eggbert `.txt`, textured/animated 3D terrain, Blupi billboard with state machine,
  mobile-object billboards, crate push, platform patrol, hazard detection, enemy stomp, respawn
  invincibility, pickups (treasure/keys/shield/egg/drink), exit-gate logic, HUD, save/load (3
  slots), 93-channel sound, 3rd-person orbit camera, 5 sky colors per region.
- **`GalaxyEggbertCNA`** (new, minimal): CMake option `GALAXY_EGGBERT_BUILD_CNA` (default OFF)
  builds it alongside/instead of Simple3D; links `CNA` + `easy3d::easy3d`; copies
  `../mobile-eggbert/Content/` and `worlds/` next to its binary at build time; parses
  `worlds/world001.txt` into the shared `GalaxyEggbert::Worlds::World` at startup (parse only).

### What does not work yet
- `GalaxyEggbertCNA`: no terrain/Blupi/object rendering, no HUD, no sound, no gameplay, no input
  beyond default window handling. Easy3D itself currently has **no renderer** at all (its
  `BillboardBatch`/`CubeBatch` are CPU-side item queues with zero GPU draw calls — confirmed by
  grep, see `easy3d.md` §7.3) — this blocks the next phase, see §4.
- `GalaxyEggbertSimple3D`: camera shake is a no-op; no per-zone fog; Android/Web builds untested
  since the last engine change.

## 3. Recent changes

| Commit | Change |
|---|---|
| `3293a3d` | docs: reject mobile-eggbert 2D→3D world auto-conversion idea (`plan.md`) |
| `0f01490` | feat: mobile-eggbert asset build-time copy + world-file parsing (`.txt` → `World`) for `GalaxyEggbertCNA`; `ObjectType`/`SoundChannel` ID parity confirmed identical to mobile-eggbert; `GE_SHARED_SOURCES` hoisted to top-level CMake scope |
| `c6b6456` | feat: `GalaxyEggbertCNA` skeleton target — new `GALAXY_EGGBERT_BUILD_CNA` option, links CNA + Easy3D, opens window, clears screen |
| `f579824` | docs: reconcile `README.md`/`CLAUDE.md`/`NEXT.md` around the new CNA+Easy3D direction |
| `ebea834` | docs: add `easy3d.md` migration analysis + `plan.md` "Direct CNA + Easy3D Migration" section |
| `ac1d8d1` | feat (Simple3D): crate push (ObjectType12) + fix vertical platform patrol (3D distance) |

Files added this migration: `easy3d.md`; `src/GalaxyEggbertCNA/` (`main.cpp`,
`GalaxyEggbertCnaGame.hpp/.cpp`, `Game/GEWorldRuntime.hpp/.cpp`). No `src/GalaxyEggbertSimple3D/`
files were touched. No sibling repository was modified (mobile-eggbert, cna, easy-3d, simple-3d
are all read-only for this work).

## 4. Current blocker / main problem

**Not a build/test failure — an open design decision blocking Phase 5.**

- **Symptom:** Easy3D (`../easy-3d`) has cameras and a texture atlas, but its `BillboardBatch` and
  `CubeBatch` are pure CPU-side item queues — there is no code anywhere (in Easy3D or in
  galaxy-eggbert) that turns queued items into vertex data or issues a CNA `GraphicsDevice` draw
  call. This isn't a bug; Easy3D's own roadmap describes this as not-yet-built.
- **Failing command:** none — nothing crashes or errors; the capability simply doesn't exist yet.
- **Affected modules:** `../easy-3d` (`BillboardBatch`, `CubeBatch`), and whatever new
  `GalaxyEggbertCNA` terrain code Phase 5 would add.
- **What's blocking progress:** `plan.md` `E3D-MIG-050` asks where the CPU-side vertex builders
  and CNA render-call adapters should live — inside `../easy-3d` itself, or as an adapter local to
  `GalaxyEggbertCNA`. This has not been decided yet, and writing terrain-rendering code before
  deciding risks building it in the wrong place.
- **What's already been tried:** nothing — this is queued as the next task (§8, task 1).

## 5. Known bugs and limitations

| Status | Issue |
|---|---|
| confirmed | Simple3D: `GECameraRig::StartShake()` is a no-op (Simple3D has no camera-offset API) |
| confirmed, environment-specific | `ctest` does not discover `GalaxyEggbertWorldsTests` when configured in the pre-existing `cmake-build-debug` CLion profile (binary runs fine manually). **Not reproduced** in a fresh `build/` directory this session — `ctest --test-dir build` correctly finds and runs all 54 tests there. Likely a stale/IDE-specific config issue in `cmake-build-debug`, not a general CMake problem. |
| incomplete | Simple3D: no per-zone fog, only `SetClearColor` per sky region |
| incomplete | `GalaxyEggbertCNA`: no terrain/Blupi/object rendering, no HUD, no sound, no gameplay (expected at this phase, not a bug) |
| unknown | Simple3D Android/Web builds untested since the last engine change |
| unknown | `GalaxyEggbertCNA`'s clean-exit-on-window-close path was not separately exercised — only a forced external `timeout`/kill was tested during verification |
| needs verification | Simple3D: stomp bounce height (`kJumpSpeed * 0.65f`) — does it match mobile-eggbert's feel? |
| needs verification | Simple3D: crate push floor-support check only tested at y=0; stacked crates (y=1) untested |
| risky assumption | `GalaxyEggbertCNA`'s world loader uses a relative path (`"worlds/world001.txt"`) — only works if the binary is run from its own build directory; fails silently to an empty world (with a stderr warning) otherwise |

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

src/GalaxyEggbertCNA/        — new, minimal. GalaxyEggbertCnaGame (Microsoft::Xna::Framework::Game
                                subclass) owns Game/GEWorldRuntime (mobile-eggbert .txt parser,
                                independent of Simple3D's version) and an Easy3D::Camera3D
                                (currently just proves the link works, not used for rendering).
```

### Data flow (current, both targets read the same file format)
```
worlds/worldXXX.txt (mobile-eggbert format, header + Decor: grid [+ MoveObject: lines])
  Simple3D: → GEWorldRuntime::LoadFromMobileEggbertFile() → World + MobileObjSpec list + blupiSpawn
            → GETerrainRenderer / GEDecorSystem / GEBlupiController (renders everything)
  CNA:      → GEWorldRuntime::LoadFromMobileEggbertFile() → World only (MoveObject: lines ignored
              for now — that's Phase 7). Nothing consumes the parsed World yet — parse only.
```

### Important invariants
- Block type = icon index in `object-m.png` (20 cols, 64×64 px tiles) — no separate mapping table.
- World grid is 100×100; both `GEWorldRuntime` implementations independently define a
  `kWCX`/`kWCZ` (Simple3D) or `kWorldCenterX`/`kWorldCenterZ` (CNA) = 50 offset to center the grid.
- `ObjectType` (204 IDs) and `SoundChannel` (93 IDs) in `include/GalaxyEggbert/def/` are confirmed
  numerically identical to mobile-eggbert's versions (verified 2026-07-01) — do not renumber.
- No `#ifdef GE_ENGINE_*` anywhere — engine differences belong inside Simple3D (for that target)
  or are simply separate code in `src/GalaxyEggbertCNA/` (for the new target).
- Build with `-j2` maximum (32 GB RAM constraint; crashes observed with more parallel jobs).

### Boundaries that must remain stable
- `BlockTypes::fromMobileIconId()` — must match the mobile-eggbert world file format exactly.
- `ObjectType`/`SoundChannel` numeric values — stored in level files, must never be renumbered.
- `GEDecorSystem::GetObjIcon(ObjectType, phase)` (Simple3D) — do not change without
  cross-referencing mobile-eggbert's `Decor.cpp`.
- `src/GalaxyEggbertSimple3D/` must not be mutated into the CNA implementation — new CNA/Easy3D
  code goes in `src/GalaxyEggbertCNA/` only.
- mobile-eggbert stays read-only; no code/data copied from it without explicit user approval.

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
# Expect on stdout: "GalaxyEggbertCNA: loaded worlds/world001.txt — spawn tile (12, 92), sky region 0, 594 non-air blocks."

# Reference: mobile-eggbert animation tables / gameplay logic (read-only)
grep -n "table_decor\|table_blupi" ../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Tables.cpp
less ../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Decor.cpp
```

No `.clang-format`/`.clang-tidy` config exists in this repo — no lint/format tooling to run.

## 8. Next smallest tasks

1. **Decide `E3D-MIG-050`** — where should Easy3D's CPU-side vertex builders and CNA
   `GraphicsDevice` draw-call adapters live: inside `../easy-3d` itself, or as an adapter local to
   `GalaxyEggbertCNA`? This is a decision, not code — but it blocks everything in Phase 5.
   **Files:** `easy3d.md` §7.3, `plan.md` `E3D-MIG-050`. **Verification:** decision recorded in
   `plan.md` with reasoning.
2. **First Easy3D terrain vertex builder** (once task 1 is decided) — implement a CPU-side
   function that turns `Easy3D::CubeBatch::Items()` into vertex/index data for one static (not yet
   animated) cube. **Files:** depends on task 1's decision (`../easy-3d/src/CubeBatch.cpp` or new
   `src/GalaxyEggbertCNA/Game/...`). **Verification:** new unit/compile-check test asserting
   expected vertex count for a known `CubeBatch` content; `GalaxyEggbertCNA` still builds.
3. **Fix `ctest` discovery in the `cmake-build-debug` profile** — investigate why
   `gtest_discover_tests` doesn't find `GalaxyEggbertWorldsTests` there (works fine in a fresh
   `build/` dir). **Files:** `CMakeLists.txt`, `cmake-build-debug/` config.
   **Verification:** `ctest --test-dir cmake-build-debug -R GalaxyEggbert` reports 54 passed.
4. **Verify `GalaxyEggbertCNA`'s clean-exit path** — close the window via the window manager
   (not a forced kill) and confirm the process exits 0 with no leaked resources.
   **Files:** none expected — diagnostic verification only, possibly add an `OnExiting` log line
   to `GalaxyEggbertCnaGame` if useful. **Verification:** manual run + exit code check.
5. **Simple3D camera shake** — make `GECameraRig::StartShake()` produce visible jitter on
   death/hazard hit, matching `DecorAction::SmallShake` in mobile-eggbert. **Files:**
   `src/GalaxyEggbertSimple3D/Game/GECameraRig.cpp/hpp`; may need a new `Game::SetCameraPositionOffset()`-style
   API added to `../simple-3d` (would need discussion, since `simple-3d` is a sibling repo).
   **Verification:** trigger a death in-game; camera visibly shakes briefly.

## 9. Do not do yet

- No further investment in Simple3D/U3D/Nova3D beyond bug fixes on the existing reference target
  — that direction is superseded.
- No modifications to `../mobile-eggbert`, `../cna`, `../easy-3d`, or `../simple-3d` without
  explicit user approval — all read-only.
- No `.txt → .vwr` (or any) automated 2D-to-3D world converter — rejected, see §1.
- No terrain/Blupi/object rendering or gameplay in `GalaxyEggbertCNA` until task 1 (§8) is decided
  — writing rendering code before that risks putting it in the wrong place.
- No Easy3D scope creep — no ECS, scene graph, physics, resource cache, editor, or Lua.
- No mass refactor of `include/GalaxyEggbert/Worlds/` unless a failing unit test justifies it.
- No `#ifdef GE_ENGINE_*` anywhere.
- No new gameplay mechanics not present in mobile-eggbert (no coins, coyote time, combo
  multipliers, star ratings, time bonuses).

## 10. Resume prompt

```
Read NEXT.md first. Then inspect only the files needed for the first task in section 8.
Do not refactor unrelated code and do not expand scope beyond that one task.
Make one small, verified improvement — implement the task goal as described, nothing more.
Cross-reference mobile-eggbert source at ../mobile-eggbert (read-only) before implementing any
gameplay or world-format behavior.
After the change, run the verification command listed for that task.
Update NEXT.md when done: move the completed task into section 3 (Recent changes), remove it
from section 8, and add whatever new next-smallest task naturally follows.
```
