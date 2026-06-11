# Galaxy Eggbert — Development Roadmap

Galaxy Eggbert is a faithful 3D remake of Speedy Blupi, using mobile-eggbert
(`/rv/data/development/github.com/openeggbert/mobile-eggbert`) as the primary reference.
All gameplay logic, level data, enums, sounds, and textures originate from mobile-eggbert
or the original Windows Phone game.

---

## Architecture plan — class and file structure

Inspired by mobile-eggbert's layering (`Game1` → `Decor` → `Pixmap`, `Tables`, `GameData`).
Galaxy-eggbert maps the same concerns to Urho3D 3D classes:

```
src/GalaxyEggbert/
  GalaxyEggbertApp.hpp/.cpp     — Urho3D Application subclass; wires lifecycle + event loop
  GalaxyEggbertGame.hpp/.cpp    — top-level coordinator; owns scene, world, subsystems
  Game/
    Blupi.hpp/.cpp              — Blupi character: physics, input, collision (≈ Decor.cpp §Blupi)
    Decor.hpp/.cpp              — tile/world simulation: objects, enemies, events (≈ Decor.cpp bulk)
    Tables.hpp/.cpp             — animation frame tables (port Tables.cpp verbatim)
    ObjectNode.hpp/.cpp         — Urho3D scene node for one moving object/enemy
    HUD.hpp/.cpp                — 2D overlay: lives, keys, gauge (≈ Game1.cpp HUD drawing)
    PhaseManager.hpp/.cpp       — GamePhase state machine + menu screen transitions
    SoundManager.hpp/.cpp       — wraps Urho3D audio; maps SoundChannel → WAV files
    Camera.hpp/.cpp             — 3rd-person / isometric camera logic
  World/
    (= current include/GalaxyEggbert/Worlds/ — keep engine-agnostic)
```

**Rules:**
- One class per file. No "god object" files.
- `GalaxyEggbertGame` delegates to subsystem objects; it does not contain game logic itself.
- `Decor` is the heart of gameplay (same as in mobile-eggbert).
- Headers that are used by tests stay in `include/`; all others in `src/`.

---

## Phase 2 — Shared enums and assets

Port the engine-independent definitions from mobile-eggbert verbatim.
These carry numeric IDs that must match the level file format exactly.

### 2.1 Port game enums to galaxy-eggbert

Copy/adapt from `mobile-eggbert/include/WindowsPhoneSpeedyBlupi/`:

| mobile-eggbert file | galaxy-eggbert target | Notes |
|---|---|---|
| `def/BlupiAction.hpp` | `include/GalaxyEggbert/BlupiAction.hpp` | 87 action states (Stop, March, Jump, …) |
| `decor/ObjectType.hpp` | `include/GalaxyEggbert/ObjectType.hpp` | 203 object type IDs — do not renumber |
| `decor/DecorAction.hpp` | `include/GalaxyEggbert/DecorAction.hpp` | Camera shake types |
| `def/SoundChannel.hpp` | `include/GalaxyEggbert/SoundChannel.hpp` | Channels 0–92, same WAV index mapping |
| `def/Direction.hpp` | `include/GalaxyEggbert/Direction.hpp` | 4-direction enum |
| `def/SecretPower.hpp` | `include/GalaxyEggbert/SecretPower.hpp` | Power-up state flags |
| `def/GameSpeed.hpp` | `include/GalaxyEggbert/GameSpeed.hpp` | Speed presets |
| `Def.hpp` (constants) | `include/GalaxyEggbert/GameConstants.hpp` | MAXCELX=100, MAXCELY=100, sprite dims |
| `Def::Phase` | `include/GalaxyEggbert/GamePhase.hpp` | Init, Play, Pause, Lost, Win, … |

Drop SharpRuntime dependency — use plain C++ `uint8_t` instead of `ubytecs`.
Drop Doxygen requirement — galaxy-eggbert does not require documentation on every member.

### 2.2 Copy assets from mobile-eggbert

These files can be used as-is:

- **Sounds**: `Content/sounds/sound010.wav` … `sound092.wav` → `Content/sounds/` in galaxy-eggbert
- **Sprite sheets** (for cube face textures and billboards):
  - `Content/icons4x/element.png` — collectibles, objects, effects
  - `Content/icons4x/blupi.png` — Blupi character sprite frames
  - `Content/icons4x/explo.png` — explosions
  - `Content/icons4x/object-m.png` — moving objects
- **Tile textures**: `Content/backgrounds/decor000.png` … `decor031.png` — 32 tile types as cube face textures
- **World files**: `worlds/world001.txt` … — same format, load via `GalaxyEggbert::Worlds::World::load()`

---

## Phase 3 — Level rendering from World data

Replace the hardcoded procedural terrain in `GalaxyEggbertGame::CreateTerrain()` with actual level data.

### 3.1 World loader integration

- Load `worlds/world001.txt` via `World::load()` (already implemented, 54 tests passing)
- Iterate chunks → blocks → spawn one `StaticModel` (Box.mdl) per non-air block
- Apply the correct `decor*.png` tile texture to the top face of each cube
- Use chunk dirty flags to batch spawning; do not spawn one node per frame

### 3.2 Tile-to-texture mapping

- `Block::type` maps to `decor{NNN}.png` (same mapping as in `Decor.cpp` of mobile-eggbert)
- Load tile textures into Urho3D `ResourceCache` from `Content/decor/`
- Each cube's top face uses the tile sprite; sides use a neutral stone texture

### 3.3 Camera

- Start from current 3rd-person orbit camera
- Follow the player position once Blupi exists; until then, orbit the level centre
- Isometric projection option: `Camera::SetOrthographic(true)`

---

## Phase 4 — Blupi player character ✅ (in progress)

### 4.0 Current state (Phase 4a — done)

- `src/GalaxyEggbert/Game/Blupi.hpp/.cpp` — dedicated Blupi class (separate from game coordinator)
- Yellow ellipsoid (Sphere.mdl scaled) as placeholder
- WASD movement (camera-relative), Space=jump
- Physics: gravity, jump impulse, AABB collision against voxel world
  - Derived from mobile-eggbert `Decor.cpp` (m_blupiVitesseX/Y), scaled from 64-px tile units to 1-unit 3D coords
- `GamePhase` state machine: Init (title screen) → Play → Pause (ESC)

### 4.1 Sprite-based representation

- Billboard quad with UV offset into `blupi.png` sprite sheet
- `BlupiAction` enum selects animation row; `Tables` (ported from mobile-eggbert) select frame
- Frame advance every N game ticks, same rate as mobile-eggbert

### 4.2 3D model with bones (future)

- Replace billboard with a real Urho3D `AnimatedModel` + `AnimationController`
- Skeleton bones for body, head, arms, legs (Blender → Urho3D exporter)
- Animations: idle, walk, jump, fall — driven by `BlupiAction` state
- Current class `Blupi` is designed to swap the model without changing physics/logic

---

## Phase 5 — Input and gameplay logic

Port the gameplay state machine from `mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Decor.cpp`.
Decor.cpp is the heart of the game: tile simulation, Blupi physics, enemy AI, collision.

### 5.1 Input

- WASD / arrow keys → `Direction` (left/right movement, jump)
- Map touch pad buttons from mobile-eggbert to keyboard/gamepad equivalents
- Urho3D `Input` subsystem replaces CNA `InputPad`

### 5.2 Blupi physics (port from Decor.cpp)

- Gravity, jump arc, landing — same float/int arithmetic as original
- Walk animation state machine: Stop → March → Turn, same transitions
- Vehicle modes: helicopter, jeep, skateboard, tank, balloon, surfboard — port each

### 5.3 Collision

- 2D tile-grid collision from Decor.cpp maps directly: Blupi's (x, y) in tile coordinates
- The 3D world is the 2D tile grid extruded; collision stays 2D in the XZ plane
- Block height (Y axis) is visual only; gameplay collision uses the 2D grid

### 5.4 Enemy AI and objects

- Port `MoveObjectStepLine` (patrol, follow, projectile) from Decor.cpp
- Each `ObjectType` spawns one `StaticModel` billboard node in the Urho3D scene
- `ObjectType::ObjectType7` (level exit) triggers win sequence

---

## Phase 6 — Game phases and HUD

Port `Def::Phase` state machine from `Game1.cpp`:

- **Init**: level/gamer select screen (adapt 2D menu to 3D UI overlay)
- **Play**: active gameplay
- **Pause**: pause overlay
- **Lost**: death screen → restart
- **Win**: level complete → next level

HUD: Urho3D UI `Text` + `Sprite` for lives counter, treasure count, level name.
Same gauge sprite (`jauge.png`) as mobile-eggbert, rendered as a 2D overlay.

---

## Phase 7 — Sound

- Load `Content/sounds/sound010.wav` … via Urho3D `SoundSource` + `ResourceCache`
- `SoundChannel` enum maps channel index → WAV file index (same as mobile-eggbert)
- `SoundChannel::SoundChannel10` → `sound010.wav`, etc.
- Multiple channels play simultaneously (Urho3D supports multiple `SoundSource` components)
- Motor loop sounds (helicopter, jeep) use continuous play; stop on vehicle exit

---

## Phase 8 — Save data

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
