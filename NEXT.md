# NEXT.md — Galaxy Eggbert

## 1. Project summary

**Galaxy Eggbert** is a faithful 3D remake of **mobile-eggbert** (a C++ port of *Speedy Blupi*, a Windows Phone XNA game from 2013).

- **Faithful remake rule:** Only implement what exists in mobile-eggbert. No invented mechanics.
- Sole build target: `GalaxyEggbertSimple3D` — built against `simple-3d` (which wraps U3D/Urho3D).
- The legacy Urho3D-direct target `GalaxyEggbert` was removed in S3D-9.
- **Backend layering:** `galaxy-eggbert game code → Simple3D API → U3D (Urho3D fork)`.
  Nova3D can replace U3D in the future by changing only Simple3D's cmake linkage — zero changes in galaxy-eggbert.
- World format: identical to mobile-eggbert `.txt` files (`worlds/world001.txt` … `world005.txt`).
- Tile sprites: same PNGs as mobile-eggbert (`Content/icons/object-m.png`, `blupi.png`, `element.png`).
- Reference: `/rv/data/development/github.com/openeggbert/mobile-eggbert` (Decor.cpp, Tables.cpp).

---

## 2. Current status

### Builds
- `cmake-build-debug/GalaxyEggbertSimple3D` — **builds clean** (CLion default profile).
- `cmake-build-simple3d/GalaxyEggbertSimple3D` — **builds clean**.
- `cmake-build-debug/GalaxyEggbertWorldsTests` — **54/54 tests pass** (run directly; ctest discovery broken).

### What works
- World loading from mobile-eggbert `.txt` format (5 worlds).
- 3D terrain rendered as cubes with tile textures from `object-m.png`.
- Animated tiles: lava (8-frame), crusher (10-frame), saw (6-frame), spike (16-frame), water1/2 (6-frame) — exact frame tables from `mobile-eggbert Tables.cpp`.
- Blupi billboard sprite from `blupi.png` with state machine (Stop/March/Jump/Air/Down/Up) and exact frame tables from `table_blupi`.
- Mobile object billboards from `element.png` with per-type animation (keys, treasure, shield, egg, drink, exit, enemies, platforms).
- Tile hazard detection: lava/spike/saw instant kill; crusher kills only in phases 5-9 of 10-frame animation cycle.
- Enemy stomp: upward `BounceUp()` impulse on kill, score +25.
- Respawn invincibility: 2 s after death with 10 Hz sprite flash; hazard/enemy-hit detection skipped.
- Pickup system: treasure counting, exit gate opens when all collected, bonus life on gate open.
- Key pickups (red/green/blue), shield pickup (5 s timer), egg/drink (+1 life).
- HUD: lives, world, treasure count, key icons, shield timer, level timer, score, game speed indicator, controls hint.
- Save/load: 3 gamer slots (lives, world, best score) + sound settings, persisted via `Simple3D::SaveData`.
- Sound: 93 channels, per-channel volume (from `tableVolumePitch`), correct WAV paths.
- Camera: 3rd-person orbit following Blupi via `GECameraRig`.
- Physics: `CharacterController` + gravity + jump + glide (RShift).
- Game phases: Init (slot select) → Play → Pause → Win → Lost → Settings.
- 5 sky colours (one per world region) via `SetClearColor`.
- Resource loading: `Content/` registered via `Game::AddResourceDir("Content")` — icons and sounds load correctly.
- `GALAXY_EGGBERT_BUILD_SIMPLE3D` defaults to `ON` — CLion default profile works without extra flags.

### What does not work yet
- Camera shake (`GECameraRig::StartShake` is a no-op — Simple3D has no shake API).
- ~~Sound loop support~~ — implemented; `GESound::Play(loop=true)` now loops via `sound->SetLooped(loop)` in Simple3D.
- "EXIT OPEN!" popup text (no on-screen notification when exit unlocks).
- Per-type enemy AI: all enemies use the same linear patrol regardless of `ObjectType`.
- ObjectType12 (crate) push mechanic — crate renders but cannot be pushed.
- Ventilator/fan tiles (icons 126-137) not in `BlockTypes`; not animated.
- Sky fog/zone colour per world region (only clear colour changes, no fog).
- Android and web builds (untested after S3D-9).

---

## 3. Recent changes

| Commit | Change |
|--------|--------|
| (pending) | feat: sound loop support — `GESound::Play(loop=true)` + `Game::PlaySound(loop)` in Simple3D |
| `869dceb` | fix: `AddResourceDir("Content")` in `Start()` — fixes white/blank tile textures |
| `c129475` | fix: `GALAXY_EGGBERT_BUILD_SIMPLE3D` default changed to `ON` |
| `89dd87a` | feat: tile animation (exact tables from Tables.cpp), respawn invincibility (2 s flash), stomp `BounceUp()` |
| `af6434f` | feat: `BlupiAction` state machine, tile hazard detection from World data, 6 fps `animPhase_` counter |
| `da6232c` | feat: S3D-9 — removed legacy Urho3D target; `src/GalaxyEggbert/Game/` deleted |
| simple-3d `9cd38f7` | feat: `Game::AddResourceDir()` added to Simple3D public API |

---

## 4. Current blocker / main problem

**No single hard blocker** — game runs and is playable end-to-end.

The most visible quality gap is **all enemies use identical linear patrol AI** regardless of type.
In mobile-eggbert, each enemy type has distinct behaviour (bird hovers at fixed Y, fish swims vertically, bulldozer charges on contact, blupit mirrors Blupi). Current `GEDecorSystem::Update()` moves all enemies identically, which makes levels feel wrong.

Secondary gap: **sound loops not implemented** — music/ambient channels that should loop play once and stop.

---

## 5. Known bugs and limitations

| Status | Issue |
|--------|-------|
| confirmed | Camera shake is a no-op (`GECameraRig::StartShake` does nothing) |
| fixed | Sound loop flag — implemented; looping sounds now loop correctly |
| confirmed | All enemies share one patrol AI regardless of `ObjectType` |
| confirmed | ObjectType12 (crate) cannot be pushed — renders statically |
| confirmed | "EXIT OPEN!" event has no on-screen text notification |
| confirmed | `ctest` does not discover `GalaxyEggbertWorldsTests` in cmake-build-debug (binary runs fine manually) |
| incomplete | Ventilator/fan tiles (icons 126-137) not defined in `BlockTypes`, not animated |
| incomplete | Sky fog/zone colour per world region — only `SetClearColor` changes |
| unknown | Web (Emscripten) build untested after S3D-9 |
| unknown | Android build untested |
| needs verification | Stomp bounce height `kJumpSpeed * 0.65f` — matches mobile-eggbert feel? |

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
                                  animation from GetObjIcon()
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
- **Faithful remake:** check `Decor.cpp` in mobile-eggbert before implementing any new gameplay behaviour.
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

Ordered by impact / faithfulness to mobile-eggbert:

### Task 1 — Per-type enemy AI (inspired by `MoveObjectStepIcon` in Decor.cpp)
**Goal:** Bird (type 20) hovers at fixed Y and patrols horizontally; fish (type 17) patrols vertically; bulldozer (type 4) uses charge/turn; blupit (type 33) mirrors Blupi X direction.
**Files:** `src/GalaxyEggbertSimple3D/Game/GEDecorSystem.cpp`
**Reference:** `mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Decor.cpp` — `MoveObjectStepIcon()`.
**Verify:** Load world001.txt; enemies visually behave differently from each other.

### Task 2 — "EXIT OPEN!" HUD popup
**Goal:** When all treasures collected, show timed text "EXIT OPEN!" for ~3 s.
**Files:** `src/GalaxyEggbertSimple3D/Game/GEHud.cpp/hpp`, `GalaxyEggbertSimpleGame.cpp`
**Verify:** Collect all treasures; popup appears and disappears after a few seconds.

### Task 3 — ObjectType12 crate push mechanic
**Goal:** Walking into a crate pushes it one tile horizontally (inspired by Decor.cpp crate logic).
**Files:** `src/GalaxyEggbertSimple3D/Game/GEDecorSystem.cpp`
**Reference:** `mobile-eggbert Decor.cpp` — ObjectType12 handling in `MoveObjectStepIcon`.
**Verify:** Walk into a crate; it slides one tile in the push direction.

### Task 4 — Ventilator/fan tile animation
**Goal:** Icons 126-137 (fan up/down/left/right) animate using `table_decor_ventillog/d/h/b` (3 frames each).
**Files:** `include/GalaxyEggbert/BlockTypes.hpp` (add Vent* constants + `tileAnimBase` ranges), `src/GalaxyEggbertSimple3D/Game/GETerrainRenderer.cpp` (add kAnimVent* tables).
**Reference:** `mobile-eggbert Tables.cpp` — `table_decor_ventillog[3]` = {126,127,128}.
**Verify:** Load a level with fan tiles; they animate at 6 fps.

### Task 5 — Camera shake
**Goal:** `GECameraRig::StartShake()` produces visible camera jitter for ~0.3 s on death/hit.
**Files:** `src/GalaxyEggbertSimple3D/Game/GECameraRig.cpp/hpp`; possibly add `Game::OffsetCamera()` or position-jitter to Simple3D.
**Verify:** Trigger a death; camera shakes briefly.

### Task 6 — Fix ctest discovery
**Goal:** `ctest --test-dir cmake-build-debug` discovers and runs the 54 world tests.
**Files:** `CMakeLists.txt` — investigate `gtest_discover_tests` issue in the debug profile.
**Verify:** `ctest --test-dir cmake-build-debug -R GalaxyEggbert` reports 54 passed.

---

## 9. Do not do yet

- **No broad refactor** of `GEDecorSystem` until per-type AI is implemented (structure will change with it).
- **No Android or web build** until desktop gameplay faithfully matches mobile-eggbert.
- **No Nova3D integration** — Simple3D backend switch belongs in the `simple-3d` repo. Wait until Nova3D implements the full Urho3D API.
- **No new gameplay mechanics** not present in mobile-eggbert (no coyote time, combo multipliers, star ratings, time bonuses).
- **No 3D character model** — billboard Blupi is correct for now; a real mesh requires asset work outside this repo.
- **Do not touch `src/GalaxyEggbert/Worlds/`** unless fixing a data model bug confirmed by a failing unit test.
- **Do not add `#ifdef GE_ENGINE_*`** anywhere — backend differences belong in Simple3D only.
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
