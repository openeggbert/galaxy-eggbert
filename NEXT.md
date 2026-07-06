# NEXT.md — Galaxy Eggbert

## 1. Project summary

**Galaxy Eggbert** is a faithful 3D remake of **mobile-eggbert** (a C++ port of *Speedy Blupi*,
a Windows Phone XNA game from 2013). Main goal: reimplement the same game logic, levels, and assets
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
  base icon's identity (Lava/Fan/Crusher/Temp/Saw/Water2 groups). **Nothing from this
  identification work has been implemented in the renderer yet** — see §8 task 1, the new active
  thread.
- A real documentation bug was found and fixed during round 3: `02-tiles.md`'s "0/78 files, unused"
  claim only checked the static `Decor:`/`BigDecor:` grid, not `MoveObject:` records' own `icon=`
  field (which can reference the same `object-m.png` sheet). At least 32 of the "unused" icons are
  confirmed actually used this way (crates, water splash, breathing bubbles, a pickup animation,
  a conveyor-lift tread, bridge-build frames) — see §3 and `02-tiles.md`'s correction note.

## 2. Current status

### Build status
- `GalaxyEggbertCNA` — **last confirmed clean build+run today (2026-07-08)**, after the
  `BlockTypes.hpp`/`GenerateSampleWorld3D.cpp` texture fix (§3). Built `GenerateSampleWorld3D`,
  `VerifyBlupiMovement`, `VerifyMoveObjectTypesCna`, and `GalaxyEggbertCNA` itself from
  `build-cna/` — all succeeded. The previously-reported missing `CMakeFiles/rules.ninja` issue
  (noted 2026-07-07) did **not** reproduce this session; `cmake --build build-cna --target ...`
  worked directly with no reconfigure needed. Not root-caused, just no longer blocking.
- `GalaxyEggbertSimple3D` — **not rebuilt this session** (no source changes to this target since
  2026-07-03/04, and no reason to touch it this session). Carrying forward the prior caveat
  unverified: U3D's own prebuilt directory
  (`/rv/data/library/github.com/u3d-community/U3D/`) may still be missing a prebuilt
  `cmake-build-debug` in this environment — confirm before trusting a fresh Simple3D build.
- `GalaxyEggbertWorldsTests` — not rebuilt/re-run this session; last confirmed 54/54 via
  `ctest --test-dir build` on 2026-07-07. The `cmake-build-debug` profile's `ctest` discovery
  issue (`GalaxyEggbertWorldsTests_NOT_BUILT`) was also last checked 2026-07-07, not re-verified.
- `../easy-3d` — not touched or rebuilt this session; last confirmed 2026-07-06 (headers-only 2/2
  tests, CNA-linked 6/6 tests).

### Test status
- `VerifyBlupiMovement` — **run today, ALL CHECKS PASSED** (spawn, staircase step-up climb,
  standing on platform, wall collision, gravity/fall) against the regenerated `world001.vwr`.
  Confirms the texture-only fix in §3 didn't change collision/geometry.
- `VerifyMoveObjectTypesCna` — built today, not run this session (no MoveObject-parsing code
  changed, so not expected to differ from its last known-good state).
- A live `GalaxyEggbertCNA` run (5s, headless-terminated) confirmed: loads
  `worlds3d/world001.vwr` (2729 non-air blocks, Y range [0,13] — unchanged from before the texture
  fix), uploads the terrain mesh (65496 vertices, 32748 triangles), and a terrain-visibility sample
  shows real texture variation (4 distinct sampled colors, not a flat fallback).

### What works
- **`GalaxyEggbertSimple3D`**: unchanged this session — world loading from mobile-eggbert `.txt`,
  textured/animated 3D terrain, Blupi billboard with state machine, mobile-object billboards, crate
  push, platform patrol, hazard detection, enemy stomp, respawn invincibility, pickups, exit-gate
  logic, HUD, save/load (3 slots), 93-channel sound, 3rd-person orbit camera, 5 sky colors per
  region, camera shake on death/hazard.
- **`GalaxyEggbertCNA`**: unchanged in behavior this session (only the sample world's *textures*
  changed, not its logic) — loads `worlds3d/world001.vwr` by default, renders every Y layer as
  textured cubes (animated tiles included), tank-control Blupi (invisible collision point),
  first-person camera + 2D animation-state HUD indicator, renders `MoveObject`s as billboards.
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
- **None of the new render modes found by tile identification are implemented**: `DirectionalCube`
  (per-face texture + fallback color/transparency), `InnerPillarBox`, `InnerFlatPlate`,
  `TripleCrossBillboard` (see §6) exist only as documentation/decisions, not code. The renderer
  still draws every terrain block as plain `UniformCube`.
- Water render mode (icons 91/92/93/94/95/96) is an **open, explicitly deferred design question**
  — the user has not yet decided how water should look in 3D.
- Icon 107 (grass) needs a real top-face grass texture, which doesn't exist yet in the asset set —
  needs sourcing (license-compatible) or generating.
- Icon 200 (`Platform`) is used as a solid floor in `GenerateSampleWorld3D.cpp`, but round-1 Q&A
  found it's actually a passable grate graphic, not a real platform surface — same class of bug
  as `StoneA`/`StoneB`, not yet fixed (see §5/§8).
- `src/GalaxyEggbertSimple3D/Game/GEWorldRuntime.cpp`'s small hardcoded fallback/default world
  (lines ~184-195) still uses `Ground`/`StoneA`/`StoneB` the same wrong way `GenerateSampleWorld3D`
  did before today's fix — not yet fixed, separate file/target.
- Simple3D: no per-zone fog; Android/Web builds untested since the last engine change.

## 3. Recent changes

Most recent first. Full history: `git log`.

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

**The active thread has shifted from identification to implementation.** Tile identification (the
multi-session "active thread" through 2026-07-06/07) is now complete (§1). The next real piece of
work is **implementing the render modes the identification work converged on** —
`DirectionalCube`, `InnerPillarBox`, `InnerFlatPlate`, `TripleCrossBillboard` — none of which exist
in `Easy3D::CubeMesh`/`GETerrainRenderer` yet; the renderer still draws every terrain block as
plain `UniformCube`. This is a real, nontrivial CNA/Easy3D coding task (not a blocker, just
unstarted) — see §8 task 1.

## 5. Known bugs and limitations

| Status | Issue |
|---|---|
| confirmed, needs fix | Icon 200 (`Platform`) is used as a solid floor in `GenerateSampleWorld3D.cpp`, but is actually a passable grate graphic (round-1 Q&A) — same class of bug `StoneA`/`StoneB` just got fixed for. |
| confirmed, needs fix | `src/GalaxyEggbertSimple3D/Game/GEWorldRuntime.cpp`'s hardcoded fallback/default world (~line 184-195) still uses `Ground`/`StoneA`/`StoneB` for terrain the same wrong way `GenerateSampleWorld3D.cpp` did before today. |
| open design question | Water tile render mode (icons 91/92/93/94/95/96) — user has explicitly deferred deciding this. |
| incomplete | Icon 107 (grass) needs a real top-face grass texture — none exists yet; needs a license-compatible source or generation. |
| incomplete | None of `DirectionalCube`/`InnerPillarBox`/`InnerFlatPlate`/`TripleCrossBillboard` are implemented in `Easy3D`/`GETerrainRenderer` — identification-only so far. |
| incomplete | `GalaxyEggbertCNA`: no Blupi/object-behavior rendering beyond billboards, no HUD, no sound, no gameplay logic (expected at this phase). No `BigDecor` rendering (parsed only). No platform-lift/crate `UniformCube` object path. |
| unverified this session | `GalaxyEggbertWorldsTests` 54/54 pass and the `cmake-build-debug` `ctest` discovery issue — both last checked 2026-07-07, not re-run today. |
| unverified this session | Simple3D build (U3D prebuilt directory possibly missing) — not touched today. |
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

src/GalaxyEggbertSimple3D/   — full playable game (Simple3D/U3D). Not touched this session except
                                (not yet) the fallback-world texture bug noted in §5.

src/GalaxyEggbertCNA/        — GalaxyEggbertCnaGame owns GEWorldRuntime, GETileAtlas,
                                GETerrainRenderer (World, all Y layers → static + animated
                                CubeMeshRenderer-backed meshes — still plain UniformCube only),
                                GEBlupiController, GEObjectIcons, an Easy3D::Camera3D
                                (first-person).

tools/GenerateSampleWorld3D.cpp — builds worlds3d/world001.vwr. Fixed today to use RockPile/
                                BrickWall instead of StoneA/StoneB. Still uses BlockTypes::Platform
                                (icon 200) for the raised floor — known-wrong, not yet fixed (§5).

mobile-eggbert-reference/    — questionnaire-unidentified-tiles.md (round 1, 34 icons, DONE),
                                questionnaire-all-remaining-tiles.md (round 2, 280 icons, DONE),
                                questionnaire-unused-tiles.md (round 3, 97 icons, DONE) — together
                                account for all 441 object-m.png icons (see §1). 02-tiles.md is the
                                canonical catalog; its "Unused/unnamed icons" section now carries a
                                correction note about the MoveObject icon= finding (§3).

../easy-3d/                  — companion library beside CNA. Standing permission to modify.
                                CubeMesh/CubeMeshRenderer (terrain, UniformCube only today),
                                BillboardMesh/BillboardMeshRenderer (objects). New render modes
                                from §1/§8 (DirectionalCube, InnerPillarBox, InnerFlatPlate,
                                TripleCrossBillboard) are NOT implemented here yet.
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

1. **Fix icon 200 (`Platform`) misuse in `GenerateSampleWorld3D.cpp`, and the analogous
   `Ground`/`StoneA`/`StoneB` bug in Simple3D's `GEWorldRuntime.cpp` fallback world.** Round-1 Q&A
   already found icon 200 is a passable grate, not a real platform surface. Pick (or confirm) a
   genuine solid-floor material, update both files, regenerate `world001.vwr`, re-run
   `VerifyBlupiMovement`. **Files:** `tools/GenerateSampleWorld3D.cpp`,
   `src/GalaxyEggbertSimple3D/Game/GEWorldRuntime.cpp`, `include/GalaxyEggbert/BlockTypes.hpp`.
   **Verification:** `./build-cna/VerifyBlupiMovement` all-pass; rebuild+run `GalaxyEggbertSimple3D`
   if touching its fallback world.
2. **Design + implement the new render modes** (`DirectionalCube`, `InnerPillarBox`,
   `InnerFlatPlate`, `TripleCrossBillboard`) that the round 2/3 identification converged on. Start
   with `DirectionalCube` (most common) — needs an `Easy3D::CubeMesh`/`CubeItem` per-face texture +
   fallback-color/transparency extension. **Files:** `../easy-3d/include/Easy3D/CubeMesh.*`,
   `src/GalaxyEggbertCNA/Game/GETerrainRenderer.*`/`GETileAtlas.*`. **Verification:** new unit/
   compile-check tests mirroring the existing `CubeMesh`/`BillboardMesh` ones, plus a live
   screenshot showing at least one `DirectionalCube` tile rendering correctly.
3. **Decide the water render mode** (icons 91/92/93/94/95/96) — currently an explicitly open
   question. Needs a design decision from the user (special flat surface? animated UniformCube
   like today? something else?), then implementation. **Files:** whatever §8 task 2 introduces for
   special-surface tiles, `GETerrainRenderer`. **Verification:** live screenshot of water rendering
   as intended.
4. **Source or generate a grass-top texture for icon 107.** No such asset exists yet. **Files:**
   likely a new file under `mobile-eggbert-reference/` or a texture-atlas addition in
   `GETileAtlas`. **Verification:** visual check once task 2's `DirectionalCube` mode can consume
   a custom top-face texture.
5. **`BigDecor` billboard rendering for CNA** — `GEWorldRuntime` doesn't parse `BigDecor:` for CNA
   at all yet (Simple3D already does). Recommended render mode: `Billboard`. **Files:**
   `src/GalaxyEggbertCNA/Game/GEWorldRuntime.*`, `GalaxyEggbertCnaGame.cpp`. **Verification:** a
   tool mirroring `VerifyMoveObjectTypesCna.cpp` against a real level with known `BigDecor:` cells.
6. **Platform-lift/crate `UniformCube` object path for CNA** — reuse existing terrain
   `CubeMesh`/`CubeMeshRenderer` machinery for the two approved "objects are cubes, not billboards"
   exceptions. **Files:** `src/GalaxyEggbertCNA/GalaxyEggbertCnaGame.cpp`.
7. **Re-verify `GalaxyEggbertWorldsTests` (54/54) and the `cmake-build-debug` ctest-discovery
   issue** — both last checked 2026-07-07, not re-run this session. **Verification:**
   `ctest --test-dir build --output-on-failure` and `ctest --test-dir cmake-build-debug`.
8. **Add face-culling/occlusion to `GETerrainRenderer`** — needed once worlds get denser; not
   needed at the current ~2700-block scale.
9. **Verify `GalaxyEggbertCNA`'s clean-exit path** — close the window via the window manager (not
   a forced kill/timeout) and confirm the process exits 0 with no leaked resources.

## 9. Do not do yet

- No further investment in Simple3D/U3D/Nova3D beyond bug fixes on the existing playable target.
- No deleting/removing any `GalaxyEggbertSimple3D` code without an explicit removal task.
- No modifications to `../mobile-eggbert`, `../cna`, or `../simple-3d` without explicit user
  approval for that specific change. `../easy-3d` has standing permission (still scoped to small,
  generic 3D-batching helpers).
- No `.txt → .vwr` (or any) automated 2D-to-3D world converter — rejected.
- No Easy3D scope creep — no ECS, scene graph, physics, resource cache, editor, or Lua.
- No mass refactor of `include/GalaxyEggbert/Worlds/` unless a failing unit test justifies it.
- No `#ifdef GE_ENGINE_*` anywhere.
- No new gameplay mechanics not present in mobile-eggbert.
- **No implementing any render mode from §8 task 2 speculatively without re-checking the exact
  per-icon answer recorded in `questionnaire-all-remaining-tiles.md`/`questionnaire-unused-tiles.md`
  first** — that's the whole point of having done direct user Q&A instead of agent guessing.
- No re-running the full 97-icon or 280-icon questionnaires again — both are done; only the small
  number of explicitly-open items (water, icon 107 grass texture, icon 200) need further decisions.
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
open questions in §5 (water render mode, grass texture), which need the user's actual decision,
not a guess.
After the change, run the verification command listed for that task.
Update NEXT.md when done: move the completed task into section 3 (Recent changes), remove it
from section 8, and add whatever new next-smallest task naturally follows.
```
