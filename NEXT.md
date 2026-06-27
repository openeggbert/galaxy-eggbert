# NEXT.md — Galaxy Eggbert

## 1. Project summary

**Galaxy Eggbert** is a faithful 3D remake of **mobile-eggbert** (a C++ port of *Speedy Blupi*, a Windows Phone XNA game from 2013).

- **Faithful remake rule:** Only implement what exists in mobile-eggbert. No invented mechanics.
- Sole build target: `GalaxyEggbertSimple3D` — built against `simple-3d` (which wraps U3D/Urho3D).
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

- **No Android or web build** until desktop gameplay faithfully matches mobile-eggbert.
- **No Nova3D integration** — Simple3D backend switch belongs in the `simple-3d` repo. Wait until Nova3D implements the full Urho3D API.
- **No new gameplay mechanics** not present in mobile-eggbert (no coins, coyote time, combo multipliers, star ratings, time bonuses).
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
