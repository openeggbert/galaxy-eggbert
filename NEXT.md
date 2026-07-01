# NEXT.md — Galaxy Eggbert

## 0. Current direction (read this first)

**Current active direction:** Direct CNA + Easy3D migration — skeleton target implemented.

**Current working implementation:** `GalaxyEggbertSimple3D` — unchanged, remains the buildable
reference target. Sections 1–10 below describe it and stay accurate/current for that target.

**`GalaxyEggbertCNA` skeleton status: builds and runs (2026-07-01).** See "§0a. GalaxyEggbertCNA
skeleton status" immediately below for build commands, verification, and what it does and does
not do yet.

**Next recommended implementation task:** finalize and implement the asset path strategy for
Mobile Eggbert assets/worlds (build-time copy, per `plan.md` E3D-MIG-013) — without modifying
`../mobile-eggbert`. See `plan.md`, Phase 3 ("Asset path and Mobile Eggbert reuse").

- `easy3d.md` — the migration analysis document (current Simple3D/Easy3D/mobile-eggbert state,
  target architecture, reuse strategy, risks, open questions).
- `plan.md` — section "Direct CNA + Easy3D Migration" has the full task list (`E3D-MIG-*`).

**Key unresolved decisions** (see `easy3d.md` §12 for full detail; tracked as `plan.md` tasks):
- Asset strategy for mobile-eggbert assets — build-time copy recommended for the first
  implementation, sibling-path runtime read considered as an optional later convenience
  (`plan.md` E3D-MIG-013). **Decided, not yet implemented — this is the next task.**
- Target/source-tree name — decided: `GalaxyEggbertCNA` / `src/GalaxyEggbertCNA/` / build option
  `GALAXY_EGGBERT_BUILD_CNA` (`plan.md` E3D-MIG-014). **Implemented.**
- Whether mobile-eggbert should later gain a read-only library target for `Tables`/`Def`/
  `GameData`/`ObjectType`/`SoundChannel` (requires explicit user approval as a separate
  mobile-eggbert-side task), versus staying asset+reference-only (`plan.md` E3D-MIG-015).
- Where Easy3D's CPU-side vertex builders and CNA render adapters should be implemented — inside
  `../easy-3d` itself, or as an adapter local to `GalaxyEggbertCNA` (`easy3d.md` §7.3/§12 Q5).

**Do not start the CNA/Easy3D gameplay/world/asset port beyond the asset-strategy task above until
that task is done and reviewed.** No content loading, world loading, terrain rendering, or Blupi
rendering exists in `GalaxyEggbertCNA` yet — see §0a.

## 0a. GalaxyEggbertCNA skeleton status

**Implemented 2026-07-01.** New source tree `src/GalaxyEggbertCNA/` (`main.cpp`,
`GalaxyEggbertCnaGame.hpp/.cpp`, namespace `GalaxyEggbert::CNA`). New CMake option
`GALAXY_EGGBERT_BUILD_CNA` (default `OFF`) in the root `CMakeLists.txt`, adding
`add_subdirectory(../cna)` then `add_subdirectory(../easy-3d)` (in that order, so Easy3D
auto-detects and links the parent-provided `CNA` target instead of building its own copy) and
defining an `add_executable(GalaxyEggbertCNA ...)` linking `CNA` and `easy3d::easy3d`.

What it does: opens a CNA/SDL window titled "Galaxy Eggbert (CNA)", constructs an
`Easy3D::Camera3D` and calls `GetViewMatrix()` once (proves Easy3D headers compile and its math
links against CNA from this target — not used for rendering), clears the screen to a solid color
every frame (cornflower blue, `0.392, 0.584, 0.929`), and runs/exits via CNA's standard
`Game::Run()` loop. No content loading, no textures, no sounds, no world files, no terrain, no
Blupi, no input handling beyond what `Game::Run()` provides by default.

**Build commands used:**
```bash
# Default build — unaffected by the new option (GALAXY_EGGBERT_BUILD_CNA defaults OFF):
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure

# New CNA target:
cmake -S . -B build-cna -DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF
cmake --build build-cna --target GalaxyEggbertCNA -j2
```

**Results (verified 2026-07-01, this sandbox):**
- `GalaxyEggbertCNA` — **builds successfully.** CNA configured with its own default backend on
  this platform (EASYGL on Linux, via sibling `../easy-gl`); no backend flags were forced. Linked
  `CNA` via the GNU/Clang linker-group workaround (`-Wl,--start-group CNA
  cna_backend_graphics_easygl -Wl,--end-group SHARP_RUNTIME`), mirroring how CNA's own
  `examples/demo_2d` links itself — a plain `target_link_libraries(... CNA)` is expected to fail
  to link on GNU/Clang/non-Windows due to circular symbol references between CNA and its backend
  static library. `easy3d::easy3d` linked cleanly via the alias.
- **Ran successfully** for a 3-second smoke test (`timeout 3s ./build-cna/GalaxyEggbertCNA`,
  `DISPLAY=:0` reachable in this sandbox): SDL window created, `EasyGLGraphicsBackend initialized
  with OpenGL OpenGL ES 3.2 Mesa 25.0.7-2`, ran the clear loop with no crash, no leftover process
  after being killed. The window-close → clean-exit path (via CNA's own `Game::Run()` event
  polling) was not separately exercised — only a forced external kill was tested.
- `GalaxyEggbertSimple3D` — **unaffected.** Built clean in the default `build/` configuration
  (`GALAXY_EGGBERT_BUILD_CNA` untouched/OFF), binary present, unchanged source.
- `GalaxyEggbertWorldsTests` — **unaffected. 54/54 tests pass** (`ctest --test-dir build`).
- No errors were hit requiring CNA/easy-3d/sharp-runtime/easy-gl fixes — the build succeeded
  end-to-end with only the linker-group workaround noted above, which is standard practice
  already used by CNA's own examples, not a bug.

**Not done / explicitly out of scope for this task:** no assets loaded, no worlds loaded, no
terrain/Blupi rendering, no gameplay, no Lua, no renderer abstraction beyond the minimum shown
above. See `plan.md` Phase 3 onward for what comes next.

---

## 1. Project summary

**Galaxy Eggbert** is a faithful 3D remake of **mobile-eggbert** (a C++ port of *Speedy Blupi*, a Windows Phone XNA game from 2013).

- **Faithful remake rule:** Only implement what exists in mobile-eggbert. No invented mechanics.
- Current build target: `GalaxyEggbertSimple3D` — built against `simple-3d` (which wraps U3D/Urho3D). This is a reference/historical target going forward, not the long-term direction — see §0.
- **Backend layering (current target only):** `galaxy-eggbert game code → Simple3D API → U3D (Urho3D fork)`.
  This Simple3D/U3D/Nova3D layering is superseded as the long-term direction by direct CNA + Easy3D (§0); it remains accurate for how `GalaxyEggbertSimple3D` itself works today.
- World format: identical to mobile-eggbert `.txt` files (`worlds/world001.txt` … `world005.txt`).
- Tile sprites: same PNGs as mobile-eggbert (`Content/icons/object-m.png`, `blupi.png`, `element.png`).
- Reference: `/rv/data/development/github.com/openeggbert/mobile-eggbert` (Decor.cpp, Tables.cpp) — read-only, no changes without explicit user approval.

---

## 2. Current status

### Builds
- `cmake-build-debug/GalaxyEggbertSimple3D` — **builds clean** (CLion default profile).
- `cmake-build-simple3d/GalaxyEggbertSimple3D` — **builds clean**.
- `cmake-build-debug/GalaxyEggbertWorldsTests` — **54/54 tests pass** (run directly; ctest discovery broken).

### What works
- World loading from mobile-eggbert `.txt` format.
- 3D terrain rendered as cubes with tile textures from `object-m.png` using `Techniques/DiffUnlit.xml` (no hard-edge lighting seam).
- Animated tiles: lava (8-frame), crusher (10-frame), saw (6-frame), spike (16-frame), water1/2 (6-frame), fan (3-frame per direction), marine (11-frame), temp (20-frame) — exact frame tables from `mobile-eggbert Tables.cpp`.
- Blupi billboard sprite from `blupi.png` with state machine (Stop/March/Jump/Air/Down/Up) and exact frame tables from `table_blupi`.
- Mobile object billboards from `element.png` with per-type animation (keys, treasure, shield, egg, drink, exit, enemies, platforms).
- ObjectType12 crate: renders with sprite (element.png icon 32) and can be pushed one tile horizontally; floor support check prevents pushing off cliffs; 0.4 s cooldown.
- Vertical platform (type 16): patrol uses 3D distance (X+Y+Z) so it now correctly moves up/down.
- Tile hazard detection: lava/spike/saw instant kill; crusher kills only in phases 5-9.
- Enemy stomp: `BounceUp()` impulse on kill, score +25.
- Respawn invincibility: 2 s after death with 10 Hz sprite flash.
- Pickup system: treasure counting, exit gate opens when all collected (+1 life bonus).
- Key pickups (red/green/blue), shield pickup (5 s timer), egg/drink (+1 life).
- "EXIT OPEN!" HUD popup: green text, 3 s (0.3 s fade-in / hold / 0.5 s fade-out).
- HUD: lives, world, treasure count, key icons, shield timer, level timer, score, game speed, controls hint.
- Save/load: 3 gamer slots (lives, world, best score) + sound settings via `Simple3D::SaveData`.
- Sound: 93 channels, per-channel volume (from `tableVolumePitch`), correct WAV paths, loop support.
- Camera: 3rd-person orbit following Blupi via `GECameraRig`.
- Physics: `CharacterController` + gravity + jump + glide (RShift).
- Game phases: Init (slot select) → Play → Pause → Win → Lost → Settings.
- 5 sky colours (one per world region) via `SetClearColor`.
- Resource loading: `Content/` registered via `Game::AddResourceDir("Content")`.

### What does not work yet
- Camera shake: `GECameraRig::StartShake` is a no-op — Simple3D has no camera-offset API.
- Sky fog/zone colour per world region — only clear colour changes, no distance fog.
- `ctest` does not discover `GalaxyEggbertWorldsTests` (binary runs fine manually).
- Android and web builds untested after S3D-9.

---

## 3. Recent changes

| Commit | Change |
|--------|--------|
| `ac1d8d1` | feat: crate push (ObjectType12) + fix vertical platform patrol (3D dist) |
| `396295a` | docs: strengthen faithful remake rule in CLAUDE.md (forbid coins etc.) |
| `a3cd1df` | fix: half-pixel UV inset in tileUV(); raise decor entity Y to 1.05 |
| `dd35277` | fix: revert to single Box entity per tile |
| `4f50e0e` | feat: TILE-039/042/064 + VISUAL-011/012 + controls + camera |
| simple-3d `f5ae0a9` | fix: use DiffUnlit technique in SetTileTexture to eliminate tile seams |

---

## 4. Current blocker / main problem

**No single hard blocker** — game runs and is playable end-to-end.

Most visible remaining quality gap: **camera shake is a no-op**. Deaths and hazard hits have no visual impact. Simple3D currently exposes no camera-offset API, so adding shake requires either extending Simple3D's `Game` class with an `OffsetCamera()` call, or applying a position-jitter directly to the camera node inside `GECameraRig` (if the node is accessible).

---

## 5. Known bugs and limitations

| Status | Issue |
|--------|-------|
| confirmed | Camera shake is a no-op (`GECameraRig::StartShake` does nothing) |
| confirmed | `ctest` does not discover `GalaxyEggbertWorldsTests` in cmake-build-debug (binary runs fine manually) |
| incomplete | Sky fog/zone colour per world region — only `SetClearColor` changes |
| unknown | Web (Emscripten) build untested after S3D-9 |
| unknown | Android build untested |
| needs verification | Stomp bounce height `kJumpSpeed * 0.65f` — matches mobile-eggbert feel? |
| needs verification | Crate push floor-support check uses world tile at y=0; stacked crates (y=1) not tested |

---

## 6. Architecture notes

### Main modules

```
GalaxyEggbertSimpleGame        — main game class (game phases, level lifecycle)
  GEWorldRuntime               — world data: loads .txt, holds World voxel grid, MobileObjSpec list,
                                  6 fps animPhase_ counter
  GETerrainRenderer            — spawns Box entities from World; Update(animPhase) refreshes animated tile UVs
  GEBlupiController            — Blupi entity: CharacterController, BlupiState machine, billboard sprite
  GEDecorSystem                — mobile objects: pickups (trigger sphere), enemies/platforms (patrol),
                                  crate push, animation from GetObjIcon()
  GEHud                        — HUD overlay (Simple3D UI labels + panels)
  GECameraRig                  — 3rd-person camera following Blupi
  GESound                      — sound channel wrapper (93 channels, per-channel volume)

include/GalaxyEggbert/Worlds/  — engine-independent voxel grid (100×100×10), tested by GalaxyEggbertWorldsTests
include/GalaxyEggbert/BlockTypes.hpp — tile type constants; block type = icon index in object-m.png
```

### Data flow

```
worlds/worldXXX.txt
  → GEWorldRuntime::LoadFromMobileEggbertFile()
      → World (blocks) + mobileObjects_ (MobileObjSpec list) + blupiSpawn_
  → GETerrainRenderer::Build()   — one Box entity per block; animTiles_ subset for UV updates
  → GEDecorSystem::Build()       — one entity per MobileObjSpec; trigger spheres for pickups
  → GEBlupiController::Respawn() — positions Blupi at blupiSpawn_
```

### Important invariants

- **Block type = icon index** in `object-m.png` (20 cols, 64×64 px tiles). No separate mapping.
- **World coordinates:** world grid is 100×100; Blupi's 3D position uses offset `kWCX=50, kWCZ=50` to centre the grid at origin.
- **animPhase_** in `GEWorldRuntime` ticks at 6 fps; used for crusher kill-phase check AND `GETerrainRenderer::Update()`.
- **No `#ifdef GE_ENGINE_*`** anywhere. Engine differences belong in Simple3D, not in galaxy-eggbert.
- **Faithful remake:** check `Decor.cpp` in mobile-eggbert before implementing any new gameplay behaviour. Mobile-eggbert enemies have NO per-type AI — all patrol linearly between `posStart` and `posEnd`; only ObjectType97 (homing bomb) tracks Blupi.
- **RAM:** build with `-j2` maximum (32 GB RAM constraint; crashes with more parallel jobs).

### API boundaries that must remain stable

- `GEWorldRuntime::kWCX / kWCZ = 50` — changing breaks tile coordinate conversion everywhere.
- `BlockTypes::fromMobileIconId()` — maps mobile-eggbert icon IDs to block types; must match world file format.
- `GEDecorSystem::GetObjIcon(ObjectType, phase)` — ported from Decor.cpp; do not change without cross-referencing mobile-eggbert.

---

## 7. Useful commands

```bash
# Configure (CLion default profile or manual):
cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug

# Build game:
cmake --build cmake-build-debug --target GalaxyEggbertSimple3D -j2

# Run game:
./cmake-build-debug/GalaxyEggbertSimple3D

# Build and run unit tests:
cmake --build cmake-build-debug --target GalaxyEggbertWorldsTests -j2
./cmake-build-debug/GalaxyEggbertWorldsTests

# Build with explicit Simple3D profile:
cmake -S . -B cmake-build-simple3d -DGALAXY_EGGBERT_BUILD_SIMPLE3D=ON
cmake --build cmake-build-simple3d --target GalaxyEggbertSimple3D -j2
./cmake-build-simple3d/GalaxyEggbertSimple3D

# Reference: mobile-eggbert animation tables
grep -n "table_decor\|table_blupi" /rv/data/development/github.com/openeggbert/mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Tables.cpp

# Reference: mobile-eggbert gameplay logic
less /rv/data/development/github.com/openeggbert/mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Decor.cpp
```

---

## 8. Next smallest tasks

**The primary next task for the repository overall is the `GalaxyEggbertCNA` skeleton described
in §0** — not the Simple3D tasks below. The tasks in this section are secondary
maintenance/polish items for the `GalaxyEggbertSimple3D` reference target; they remain valid to
pick up, but do not represent the project's forward direction.

Ordered by impact / faithfulness to mobile-eggbert:

### Task 1 — Camera shake
**Goal:** `GECameraRig::StartShake()` produces visible camera jitter for ~0.3 s on death/hazard hit, matching `DecorAction::SmallShake` in mobile-eggbert.
**Files:** `src/GalaxyEggbertSimple3D/Game/GECameraRig.cpp/hpp`; possibly add `Game::SetCameraPositionOffset()` or similar to `simple-3d/src/Simple3D/Game.cpp`.
**Reference:** `mobile-eggbert Decor.cpp` — `m_decorAction = DecorAction::SmallShake`.
**Verify:** Trigger a death; camera shakes briefly.

### Task 2 — Fix ctest discovery
**Goal:** `ctest --test-dir cmake-build-debug` discovers and runs the 54 world tests.
**Files:** `CMakeLists.txt` — investigate `gtest_discover_tests` issue in the debug profile.
**Verify:** `ctest --test-dir cmake-build-debug -R GalaxyEggbert` reports 54 passed.

---

## 9. Do not do yet

- **No CNA/Easy3D gameplay port** before the `GalaxyEggbertCNA` skeleton target exists and builds
  cleanly (§0).
- **No mobile-eggbert modifications** without explicit user approval — it is read-only for this
  migration (§0, `easy3d.md` §5.1).
- **No further investment in the Simple3D/U3D/Nova3D direction** beyond keeping
  `GalaxyEggbertSimple3D` working — it is superseded as the long-term target (§0).
- **No Android or web build** until desktop gameplay faithfully matches mobile-eggbert.
- **No Nova3D integration** — that direction is superseded; do not pursue it further.
- **No new gameplay mechanics** not present in mobile-eggbert (no coins, coyote time, combo multipliers, star ratings, time bonuses).
- **No 3D character model** — billboard Blupi is correct for now; a real mesh requires asset work outside this repo.
- **Do not touch `src/GalaxyEggbert/Worlds/`** unless fixing a data model bug confirmed by a failing unit test.
- **Do not add `#ifdef GE_ENGINE_*`** anywhere — backend differences belong in Simple3D (for the current target) only.
- **No sky/fog overhaul** until Simple3D exposes per-zone fog API (it currently does not).

---

## 10. Resume prompt

```
Read NEXT.md first. Then inspect only the files listed under the first task in section 8.
Do not refactor unrelated code.
Make one small, concrete improvement — implement the task goal as described.
Cross-reference mobile-eggbert source at:
  /rv/data/development/github.com/openeggbert/mobile-eggbert/
before implementing any gameplay behaviour.
After the change, run:
  cmake --build cmake-build-debug --target GalaxyEggbertSimple3D -j2
  ./cmake-build-debug/GalaxyEggbertWorldsTests
Verify the build is clean and all 54 tests pass.
Update NEXT.md: move the completed task to section 3 (Recent changes) and remove it from section 8.
```
