# Galaxy Eggbert — Development Roadmap

Galaxy Eggbert is a faithful 3D remake of Speedy Blupi, using mobile-eggbert
(`/rv/data/development/github.com/openeggbert/mobile-eggbert`) as the primary reference.
All gameplay logic, level data, enums, sounds, and textures originate from mobile-eggbert
or the original Windows Phone game.

---

## Current state (as of Phase 4 partial)

**Working:**
- Hardcoded demo level: flat grass floor, border walls, scattered raised platforms
- Terrain rendered from `World` data model; tiles UV-mapped from `object-m.png`
- Blupi character: yellow ellipsoid placeholder + red "nose" direction indicator
- Arrow key controls: LEFT/RIGHT rotate Blupi, UP/DOWN move forward/back along facing
- Physics: gravity, jump (SPACE), AABB collision against voxel world
  - Derived from mobile-eggbert `Decor.cpp` (`m_blupiVitesseX/Y`, same constants, scaled to 1-unit voxels)
- `GamePhase` state machine: Init (title overlay) → Play → Pause (ESC toggles)
- Camera: smoothly follows Blupi's facing direction (auto yaw with shortest-path interpolation)
- HUD: live position and facing angle (`Text` overlay)
- 54 unit tests pass for `Worlds/` data model (engine-independent)

**Not yet done:**
- Level loading from actual `world001.txt` (currently hardcoded demo geometry)
- Sprite-based Blupi (`blupi.png` billboard)
- `Decor` port: tile simulation, enemy AI, moving objects, events
- Sound (`SoundManager`, WAV mapping)
- Full HUD: lives, keys, gauge sprite
- Game phases: Lost, Win
- Save data (`GameData`)
- Refactor `GalaxyEggbertGame` into proper subsystem classes (see Architecture plan below)

---

## Architecture plan — class and file structure

Inspired by mobile-eggbert's layering (`Game1` → `Decor` → `Pixmap`, `Tables`, `GameData`).
Galaxy-eggbert maps the same concerns to Urho3D 3D classes:

```
src/GalaxyEggbert/
  GalaxyEggbertApp.hpp/.cpp     — Urho3D Application subclass; wires lifecycle + event loop
  GalaxyEggbertGame.hpp/.cpp    — top-level coordinator; owns scene, world, subsystems
  Game/
    Blupi.hpp/.cpp              — Blupi character: physics, input, collision (≈ Decor.cpp §Blupi) ✅
    Decor.hpp/.cpp              — tile/world simulation: objects, enemies, events (≈ Decor.cpp bulk)
    Tables.hpp/.cpp             — animation frame tables (port Tables.cpp verbatim)
    ObjectNode.hpp/.cpp         — Urho3D scene node for one moving object/enemy
    HUD.hpp/.cpp                — 2D overlay: lives, keys, gauge (≈ Game1.cpp HUD drawing)
    PhaseManager.hpp/.cpp       — GamePhase state machine + menu screen transitions
    SoundManager.hpp/.cpp       — wraps Urho3D audio; maps SoundChannel → WAV files
    Camera.hpp/.cpp             — 3rd-person camera logic (currently inline in GalaxyEggbertGame)
  World/
    (= current include/GalaxyEggbert/Worlds/ — keep engine-agnostic) ✅
```

**Rules:**
- One class per file. No "god object" files.
- `GalaxyEggbertGame` delegates to subsystem objects; it does not contain game logic itself.
- `Decor` is the heart of gameplay (same as in mobile-eggbert).
- Headers used by tests stay in `include/`; all others in `src/`.

Currently `GalaxyEggbertGame` still holds camera, HUD, phase, and terrain logic inline.
The refactor into the subsystem classes above is the next major architectural task.

---

## Phase 5 — Level loading from world file

Replace hardcoded demo geometry with actual level data loaded at runtime.

- Load `worlds/world001.txt` via `World::load()` (already implemented, tests passing)
- Iterate chunks → blocks → spawn one `StaticModel` (Box.mdl) per non-air block
- Apply UV-mapped tile texture from `object-m.png` to each block (mapping in `BlockTypes.hpp`)
- Spawn Blupi at the level's start position (read from world metadata or fixed offset)
- `worlds/` is already copied next to the executable by CMake post-build step

---

## Phase 6 — Architecture refactor

Extract subsystems out of `GalaxyEggbertGame` into their own classes (see Architecture plan).
Priority order:

1. `Camera.hpp/.cpp` — isolate 3rd-person camera orbit and smooth follow logic
2. `HUD.hpp/.cpp` — move Urho3D UI elements (Text, Sprite) here
3. `PhaseManager.hpp/.cpp` — move `GamePhase` state machine and overlay transitions
4. `SoundManager.hpp/.cpp` — stub class, wire WAV loading; no gameplay needed yet
5. `Decor.hpp/.cpp` — begin porting tile simulation from mobile-eggbert `Decor.cpp`

---

## Phase 7 — Sprite-based Blupi

Replace the yellow ellipsoid placeholder with a sprite billboard.

- Billboard quad node parented to `Blupi::node_`
- UV offset into `blupi.png` sprite sheet selects the correct frame
- `BlupiAction` enum (already copied from mobile-eggbert) selects animation row
- `Tables` (port from mobile-eggbert `Tables.cpp`) provide per-action frame indices
- Frame advance every N game ticks, same rate as mobile-eggbert
- `Blupi` class is designed to swap the visual without touching physics

Future (not now): replace billboard with `AnimatedModel` + bones (Blender → Urho3D exporter).

---

## Phase 8 — Decor port (tile simulation + AI)

Port `mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Decor.cpp` to galaxy-eggbert.
This is the heart of the game: tile events, enemy AI, moving objects, collectibles.

- `MoveObjectStepLine` — patrol, follow, projectile movement
- Tile activation / destruction sequences
- Each `ObjectType` spawns one `ObjectNode` (billboard) in the Urho3D scene
- `ObjectType::ObjectType7` (level exit) triggers win sequence

---

## Phase 9 — Game phases and full HUD

- **Lost**: death screen → restart
- **Win**: level complete → next level
- HUD: Urho3D UI `Text` + `Sprite` for lives counter, treasure count, level name
- Same gauge sprite (`jauge.png`) as mobile-eggbert, rendered as 2D overlay

---

## Phase 10 — Sound

- Load `Content/sounds/sound010.wav` … via Urho3D `SoundSource` + `ResourceCache`
- `SoundChannel` enum maps channel index → WAV file index (same as mobile-eggbert)
- Multiple channels play simultaneously (Urho3D supports multiple `SoundSource` components)
- Motor loop sounds (helicopter, jeep) use continuous play; stop on vehicle exit

---

## Phase 11 — Save data

- Port `GameData` save format from mobile-eggbert verbatim (flat byte array, 3 gamer slots)
- Store in user data directory via Urho3D `FileSystem::GetUserDocumentsDir()`
- Compatible with mobile-eggbert save files

---

## Platform support for U3D engine

### Windows (MinGW cross-compile)

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

### Android

Status: **Not yet implemented for U3D.**

U3D uses its own Gradle + CMake Android integration.
Steps: build U3D AAR → set `BUILD_STAGING_DIR` → remove `FATAL_ERROR` guard for ANDROID in CMakeLists.txt.

### Web (Emscripten)

Status: **Not yet implemented for U3D.**

Steps: Emscripten U3D build (`emcmake cmake`) → add `--preload-file Data/ CoreData/` → remove `FATAL_ERROR` guard.

---

## Nova3D backend

When Nova3D implements the full Urho3D API (same headers, same namespace, same scene graph):

1. Set `GALAXY_EGGBERT_ENGINE=NOVA3D` in CMake.
2. Verify all Urho3D API calls compile and behave identically.
3. Expected C++ source change: zero lines (no `#ifdef` guards exist).
