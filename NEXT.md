# Galaxy Eggbert — Development Roadmap

Galaxy Eggbert is a faithful 3D remake of Speedy Blupi, using mobile-eggbert
(`/rv/data/development/github.com/openeggbert/mobile-eggbert`) as the primary reference.
All gameplay logic, level data, enums, sounds, and textures originate from mobile-eggbert
or the original Windows Phone game.

---

## Current state (as of Phase 20)

**Working:**
- World loaded from `worlds/world001.vwr` at runtime; demo world saved on first run
- Terrain rendered from `World` data model; tiles UV-mapped from `object-m.png`
- Blupi: sprite billboard from `blupi.png` (10-col × 34-row sheet, 60×60 tiles)
  - Full animation state machine: Stop, March, Turn, Jump, Air
  - Frame tables ported from mobile-eggbert `Tables.cpp`
  - Physics: gravity, jump, AABB voxel collision (derived from `Decor.cpp`)
  - Arrow key controls: LEFT/RIGHT rotate, UP/DOWN move along facing
- `Decor` object pool (up to 50 objects), all animated via `GetIcon()` phase tables:
  - ObjectType2/3 (patrol enemies A/B), ObjectType16 (spider)
  - ObjectType5 (treasure), ObjectType6 (egg), ObjectType7 (exit)
  - ObjectType49/50/51 (red/green/blue keys) — key collection tracked, shown in HUD
  - ObjectType25 (shield orb) — grants 5 s invincibility, shown in HUD as countdown
  - Tile hazards: `Lava` (icon 68), `Spike` (icon 373), `Crusher` (icon 317) block types
    — kill Blupi on contact unless shield is active; shield bypasses enemy hits too
- `GamePhase` state machine: Init → Play → Pause / Win / Lost
  - Init, Pause, Win, Lost phases each show a fullscreen background overlay
- Camera: smooth 3rd-person orbit; RMB pitch, scroll zoom, auto-yaw follow
- HUD: lives, treasures, keys, shield timer, `jauge.png` gauge sprite (bottom-left)
- `SoundManager`: loads all 93 WAV files; per-channel volume from `tableVolumePitch`
  - Wired: jump (ch1), collect/treasure (ch10), key (ch11), shield (ch42), death (ch8), exit (ch57)
- `GameData`: 640-byte save file, binary-compatible with mobile-eggbert
  - Stores lives, last world reached, door states (3 gamer slots)
  - Loaded at startup, written on win/lost/quit/reset
- Level progression: `LoadWorld(N)` loads `worlds/worldNNN.vwr`; `AdvanceToNextWorld()` transitions
  win → next world without full reset; wraps at world 5; terrain cleared via `terrainRoot_` node
  - Per-world difficulty: +1 Crusher tile and +1 patrol enemy per world level
  - HUD shows current world number: `"World N | Lives: N  Treasures: N  Keys: N"`
- Gamer-select screen at Init phase: keys 1/2/3 choose one of 3 save slots; shows
  lives/world/doors per slot; text rendered on overlay via `PhaseManager::SetOverlayText()`
- Settings screen (`GamePhase::MainSetup` / `PlaySetup`, `backgrounds/setup.png`):
  sound on/off toggle (S key), persisted to save, applied at startup; ESC returns to caller
  - Accessible via S key from Init (main menu) or Pause
  - `SoundManager::SetEnabled(bool)`: mutes all channels; `Play()` is no-op when disabled
- 54 unit tests pass for `Worlds/` data model (engine-independent)
- Windows build: `GalaxyEggbert.exe` (PE32+, x86-64) via MinGW-w64 cross-compile
  - `build-windows/` configured with `cmake/toolchains/mingw-w64.cmake` + U3D Windows build
  - Statically linked: `-static-libgcc -static-libstdc++`; Windows system libs include `iphlpapi`
- HUD sprite icons: life icons (Blupi head, `blupi.png` icon 48) and key icons (`element.png` icon 215)
  replace "Lives: N / Keys: N" text; both shown as `BorderImage` sprites inside/near the gauge
- HUD treasure total: shows "Treasures: N/total" — total counted by `Decor::GetTotalTreasures()`
- HUD world names: `WorldName(N)` lookup — "Grassland", "Forest", "Ice Caves", "Lava Fields", "Space Station"
- Real levels: `LoadMobileEggbertTerrain()` parses mobile-eggbert `.txt` world files
  - `worlds/world00N.txt` (from mobile-eggbert world01N.txt) loaded automatically when present
  - Terrain built from 100×100 Decor grid; tile IDs mapped via `BlockTypes::fromMobileIconId()`
  - MoveObjects parsed: types 2,3,4,5,6,7,16,20,25,49,50,51 placed as Decor objects
  - Patrol enemies with posStart==posEnd get default ±2 tile X patrol range
  - 64px tile size for all position conversions (pixel → tile = px/64)
  - `blupiPos=` header parsed → stored as `blupiSpawn_`; Blupi spawns at correct world position
  - Fall-through-floor respawn uses `spawn_` (no longer hardcoded to origin)
- ObjectType4 (bulldozer) and ObjectType20 (bird) added as patrol enemies
- `ObjectNode` tile dimensions fixed: 60×60 px tiles, 10 cols (was wrong 64×9)

**Architecture (subsystem classes):**
```
src/GalaxyEggbert/Game/
  Blupi.hpp/.cpp          — physics, animation, sprite billboard       ✅
  Camera.hpp/.cpp         — 3rd-person orbit camera                    ✅
  Decor.hpp/.cpp          — object pool, patrol movement, collision     ✅ (partial)
  GameData.hpp/.cpp       — 640-byte save format (mobile-eggbert compat)✅
  HUD.hpp/.cpp            — lives, keys, shield, gauge sprite           ✅ (partial)
  ObjectNode.hpp/.cpp     — single object billboard in Urho3D scene     ✅
  PhaseManager.hpp/.cpp   — GamePhase state + overlay transitions       ✅
  SoundManager.hpp/.cpp   — 93-channel WAV audio with volume table      ✅
  Tables.hpp/.cpp         — animation frame tables (from Tables.cpp)    ✅
```

**Not yet done:**
- Vehicles and advanced object types (helicopter, jeep, skateboard, bulldozer — player-mode state machine)
- Animated 3D model for Blupi (currently billboard placeholder)
- Android and Web (Emscripten) platform builds

---

## Architecture plan — class and file structure

Inspired by mobile-eggbert's layering (`Game1` → `Decor` → `Pixmap`, `Tables`, `GameData`).
Galaxy-eggbert maps the same concerns to Urho3D 3D classes:

```
src/GalaxyEggbert/
  GalaxyEggbertApp.hpp/.cpp     — Urho3D Application subclass; wires lifecycle + event loop
  GalaxyEggbertGame.hpp/.cpp    — top-level coordinator; owns scene, world, subsystems
  Game/
    Blupi.hpp/.cpp              — Blupi character: physics, input, collision      ✅
    Camera.hpp/.cpp             — 3rd-person camera logic                         ✅
    Decor.hpp/.cpp              — object pool, enemies, events                    ✅ (partial)
    GameData.hpp/.cpp           — save data persistence                           ✅
    HUD.hpp/.cpp                — 2D overlay: lives, keys, shield, gauge          ✅ (partial)
    ObjectNode.hpp/.cpp         — Urho3D scene node for one moving object/enemy   ✅
    PhaseManager.hpp/.cpp       — GamePhase state machine + overlay transitions   ✅
    SoundManager.hpp/.cpp       — wraps Urho3D audio; maps SoundChannel → WAV     ✅
    Tables.hpp/.cpp             — animation frame tables (port Tables.cpp)        ✅
  World/
    (= current include/GalaxyEggbert/Worlds/ — keep engine-agnostic)             ✅
```

---

## Phase 15 — Windows build (MinGW cross-compile)

Status: **DONE** — `build-windows/GalaxyEggbert.exe` (PE32+, x86-64, 22 MB).

Build commands (for reference):
```bash
# U3D Windows library (already built at build-windows/lib/libUrho3D.a):
make -C /rv/data/library/github.com/u3d-community/U3D/build-windows -j2 Urho3D

# Galaxy Eggbert Windows executable:
cmake -S . -B build-windows \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64.cmake \
      -DGALAXY_EGGBERT_ENGINE=U3D \
      -DU3D_HOME=/rv/data/library/github.com/u3d-community/U3D/build-windows \
      -DBUILD_TESTING=OFF
cmake --build build-windows --target GalaxyEggbert -j2
```

---

## Phase 20 — Correct Blupi spawn from real world files

Status: **DONE**

- `LoadMobileEggbertTerrain` uses 64px tile size (matches mobile-eggbert pixel coordinates)
- `blupiPos=` header parsed → `blupiSpawn_` stored in `GalaxyEggbertGame`
- `EnterPhase(Play)`: calls `blupi_->SetSpawnPoint(blupiSpawn_)` then `Respawn()`
- Fall-through-floor respawn in `Blupi::Update` uses `spawn_` field (fixed hardcoded origin)
- Blupi now starts at correct tile position in all 5 real world files

---

## Phase 19 — New enemy types + ObjectNode sprite fix

Status: **DONE**

- `ObjectNode` sprite UV fixed: 60×60 px tiles, 10 cols (was erroneously 64 px/9 cols)
- `ObjectType4` (bulldozer): patrol enemy using `table_bulldozer_left` icons 65–67
- `ObjectType20` (bird): patrol enemy using `table_oiseau_left` icons 98–105
- Both damage Blupi on contact (same as ObjectType2/3)
- `LoadMobileEggbertTerrain` now parses types 4 and 20; stationary patrol enemies
  get a default ±2 tile X patrol range

---

## Phase 18 — Real mobile-eggbert world loading

Status: **DONE** — `LoadMobileEggbertTerrain()` in GalaxyEggbertGame.cpp.

- `worlds/world00N.txt` files (from mobile-eggbert world01N.txt) shipped in the repo
- `LoadWorld()` tries .vwr first, then .txt, then generates demo world
- `BlockTypes::fromMobileIconId()` maps mobile-eggbert tile IDs → galaxy-eggbert block types
- MoveObject parser handles types 2,3,5,6,7,16,25,49,50,51; speed from `stepAdvance`
- Blupi always spawns at 3D (0,y,0); world is centered on the `blupiPos` from the file

---

## Phase 17 — HUD treasure total + world names

Status: **DONE** — `Decor::GetTotalTreasures()` + `WorldName()` lookup in HUD.

- `Decor::PlaceObject` counts ObjectType5 placements into `totalTreasures_`
- `HUD::ShowPlay` now shows `"Treasures: N/total"` and `"World N: <Name>"`
- World name table: 1→Grassland, 2→Forest, 3→Ice Caves, 4→Lava Fields, 5→Space Station

---

## Phase 16 — HUD sprite icons (life icons + key icons)

Status: **DONE** — `HUD.cpp` rewritten with `BorderImage` sprite icons.

- Life icons: `blupi.png` icon 48 (col 8, row 4 of 60×60 sheet) — up to 5 Blupi heads
- Key icons: `element.png` icon 215 (col 5, row 21) — one icon per collected key (max 3)
- Text simplified: removed position/facing debug; shows `World N | Treasures: N [SHIELD Xs]`
- `ShowPlay` signature drops `pos`/`facingYaw` (no longer needed for display)

---

## Phase 19 — Android build (U3D)

Status: **Not yet implemented for U3D.**

U3D uses its own Gradle + CMake Android integration.
Steps: build U3D AAR → set `BUILD_STAGING_DIR` → remove `FATAL_ERROR` guard for ANDROID in CMakeLists.txt.

---

## Phase 20 — Web build (Emscripten)

Status: **Not yet implemented for U3D.**

Steps: Emscripten U3D build (`emcmake cmake`) → add `--preload-file Data/ CoreData/` → remove `FATAL_ERROR` guard.

---

## Nova3D backend

When Nova3D implements the full Urho3D API (same headers, same namespace, same scene graph):

1. Set `GALAXY_EGGBERT_ENGINE=NOVA3D` in CMake.
2. Verify all Urho3D API calls compile and behave identically.
3. Expected C++ source change: zero lines (no `#ifdef` guards exist).
