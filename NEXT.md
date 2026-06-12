# Galaxy Eggbert — Development Roadmap

Galaxy Eggbert is a faithful 3D remake of Speedy Blupi, using mobile-eggbert
(`/rv/data/development/github.com/openeggbert/mobile-eggbert`) as the primary reference.
All gameplay logic, level data, enums, sounds, and textures originate from mobile-eggbert
or the original Windows Phone game.

---

## Current state (as of Phase 11)

**Working:**
- World loaded from `worlds/world001.vwr` at runtime; demo world saved on first run
- Terrain rendered from `World` data model; tiles UV-mapped from `object-m.png`
- Blupi: sprite billboard from `blupi.png` (10-col × 34-row sheet, 60×60 tiles)
  - Full animation state machine: Stop, March, Turn, Jump, Air
  - Frame tables ported from mobile-eggbert `Tables.cpp`
  - Physics: gravity, jump, AABB voxel collision (derived from `Decor.cpp`)
  - Arrow key controls: LEFT/RIGHT rotate, UP/DOWN move along facing
- `Decor` object pool (up to 50 objects):
  - ObjectType2 (enemy, patrols), ObjectType5 (treasure), ObjectType6 (egg), ObjectType7 (exit)
  - Sprite billboard from `element.png`, animated via `GetIcon()` phase tables
  - Collision: collect removes object, exit triggers Win, enemy triggers respawn/Lost
- `GamePhase` state machine: Init → Play → Pause / Win / Lost
  - Init, Pause, Win, Lost phases each show a fullscreen background overlay
- Camera: smooth 3rd-person orbit; RMB pitch, scroll zoom, auto-yaw follow
- HUD: lives counter, treasure count, `jauge.png` gauge sprite (bottom-left)
- `SoundManager`: loads all 93 WAV files; per-channel volume from `tableVolumePitch`
  - Wired: jump (ch1), collect (ch10), death (ch8), exit (ch57)
- `GameData`: 640-byte save file, binary-compatible with mobile-eggbert
  - Stores lives, last world reached, door states (3 gamer slots)
  - Loaded at startup, written on win/lost/quit/reset
- 54 unit tests pass for `Worlds/` data model (engine-independent)

**Architecture (subsystem classes):**
```
src/GalaxyEggbert/Game/
  Blupi.hpp/.cpp          — physics, animation, sprite billboard       ✅
  Camera.hpp/.cpp         — 3rd-person orbit camera                    ✅
  Decor.hpp/.cpp          — object pool, patrol movement, collision     ✅
  GameData.hpp/.cpp       — 640-byte save format (mobile-eggbert compat)✅
  HUD.hpp/.cpp            — lives, treasures, gauge sprite              ✅
  ObjectNode.hpp/.cpp     — single object billboard in Urho3D scene     ✅
  PhaseManager.hpp/.cpp   — GamePhase state + overlay transitions       ✅
  SoundManager.hpp/.cpp   — 93-channel WAV audio with volume table      ✅
  Tables.hpp/.cpp         — animation frame tables (from Tables.cpp)    ✅
```

**Not yet done:**
- More `ObjectType` variants (vehicles, hazards, traps, power-ups)
- Tile-based events: lava, spikes, moving platforms, crushers
- Multiple level files and progression (world001 → world002 → …)
- Gamer-select screen (3 save slots) and settings screen
- Full HUD: life icons (blupi head sprites), key count, level name
- Animated 3D model for Blupi (currently billboard placeholder)
- Windows, Android, Web platform builds

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
    HUD.hpp/.cpp                — 2D overlay: lives, keys, gauge                  ✅ (partial)
    ObjectNode.hpp/.cpp         — Urho3D scene node for one moving object/enemy   ✅
    PhaseManager.hpp/.cpp       — GamePhase state machine + overlay transitions   ✅
    SoundManager.hpp/.cpp       — wraps Urho3D audio; maps SoundChannel → WAV     ✅
    Tables.hpp/.cpp             — animation frame tables (port Tables.cpp)        ✅
  World/
    (= current include/GalaxyEggbert/Worlds/ — keep engine-agnostic)             ✅
```

---

## Phase 12 — Extended Decor: more object types and tile hazards

Port more of `mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Decor.cpp`:

- **More enemy types**: ObjectType1 (spider), ObjectType3 (fireball), ObjectType4 (rolling rock)
- **Collectibles**: ObjectType8 (key), ObjectType9 (bomb), ObjectType25 (shield)
- **Tile hazards**: lava tiles (`IsLave`), spike traps (`IsPiege`), crusher (`IsEcraseur`)
  — detect via block type, trigger the same Blupi-kill logic
- **Key counter**: HUD shows collected keys; locked doors require matching key count
- **`PlaceObjectFromWorld()`**: read object placement from world file metadata
  (mobile-eggbert stores objects in the same `.txt` grid; galaxy-eggbert can store them
  in the `.vwr` format's metadata section once it is extended)

---

## Phase 13 — Level progression

- Load `worlds/world001.vwr`, `world002.vwr`, … based on `GameData::GetLastWorld()`
- On Win: `++currentWorld_` → `CreateTerrain()` for the next world file
- Level-select screen shown at Init phase (list of unlocked worlds)
- HUD shows current level number
- Each world file stores its own layout; ship at least `world001.vwr` … `world005.vwr`

---

## Phase 14 — Gamer select and settings screen

- Gamer select: show three slots with `GetGamerInfo()` stats (lives, main doors, secondary doors)
- Settings screen (`backgrounds/setup.png` overlay):
  - Sound on/off toggle → `GameData::SetSounds()` + `SoundManager` enable/disable
  - (Future: accelerometer sensitivity, jump button side)
- `selectedGamer` stored in `GameData`; switching slot reloads lives + world

---

## Phase 15 — Windows build (MinGW cross-compile)

Status: **CMake ready, needs U3D Windows build.**

1. Build U3D for Windows:
   ```bash
   cmake -S /rv/data/library/github.com/u3d-community/U3D \
         -B /rv/data/library/github.com/u3d-community/U3D/build-windows \
         -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchains/MinGW.cmake
   ninja -C /rv/data/library/github.com/u3d-community/U3D/build-windows -j2 Urho3D
   ```
2. Configure galaxy-eggbert:
   ```bash
   cmake -S . -B build-windows \
         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64.cmake \
         -DGALAXY_EGGBERT_ENGINE=U3D \
         -DU3D_HOME=/rv/data/library/github.com/u3d-community/U3D/build-windows
   ```

---

## Android

Status: **Not yet implemented for U3D.**

U3D uses its own Gradle + CMake Android integration.
Steps: build U3D AAR → set `BUILD_STAGING_DIR` → remove `FATAL_ERROR` guard for ANDROID in CMakeLists.txt.

---

## Web (Emscripten)

Status: **Not yet implemented for U3D.**

Steps: Emscripten U3D build (`emcmake cmake`) → add `--preload-file Data/ CoreData/` → remove `FATAL_ERROR` guard.

---

## Nova3D backend

When Nova3D implements the full Urho3D API (same headers, same namespace, same scene graph):

1. Set `GALAXY_EGGBERT_ENGINE=NOVA3D` in CMake.
2. Verify all Urho3D API calls compile and behave identically.
3. Expected C++ source change: zero lines (no `#ifdef` guards exist).
