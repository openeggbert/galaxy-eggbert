# Galaxy Eggbert — Claude Code Guidelines

## Project Overview

**Galaxy Eggbert** is a faithful 3D remake of **mobile-eggbert** (itself a faithful C++ port of the original *Speedy Blupi*, a Windows Phone XNA game from 2013).
It is implemented in C++ using the **Urho3D API** with **U3D** (`u3d-community/U3D`) as the reference backend.
Nova3D (the user's own Urho3D fork) will replace U3D once it fully implements the same API.

**Faithful remake rule:** Galaxy Eggbert must only implement gameplay features that exist in mobile-eggbert. Do not invent new mechanics (time bonuses, star ratings, coyote time, combo multipliers, etc.) unless they are present in mobile-eggbert. The 3D dimension allows natural adaptations (camera, step-up traversal, shadow, billboard sprites) but the game logic must mirror mobile-eggbert.

Engine backend is selected at CMake configure time:
```
cmake -S . -B cmake-build-u3d -DGALAXY_EGGBERT_ENGINE=U3D     # default
cmake -S . -B build-nova3d    -DGALAXY_EGGBERT_ENGINE=NOVA3D   # future
```

## Relationship to mobile-eggbert

**mobile-eggbert** (`/rv/data/development/github.com/openeggbert/mobile-eggbert`) is the primary reference and inspiration.
It is a C++ port of the original Windows Phone Speedy Blupi, ported path: C# (ILSpy decompile) → MonoGame → C++ with CNA (XNA-compatible SDL3 framework).

### What to reuse or use as direct inspiration from mobile-eggbert

| Category | mobile-eggbert source | Reuse plan for galaxy-eggbert |
|---|---|---|
| Game enums | `include/WindowsPhoneSpeedyBlupi/def/` | Copy/adapt: `BlupiAction`, `Direction`, `SecretPower`, `GameSpeed`, `KeyPressFlags` |
| Object types | `decor/ObjectType.hpp` | Copy verbatim — numeric IDs must match level file format |
| Decor actions | `decor/DecorAction.hpp` | Copy verbatim (camera shake types) |
| Sound channels | `def/SoundChannel.hpp` | Copy — 92 sound effects, same index mapping |
| Sprite sheets | `Content/icons/` and `Content/icons4x/` | Use as texture source for cube faces: `blupi.png`, `element.png`, `explo.png`, `object-m.png`, `button.png` |
| Level backgrounds | `Content/backgrounds/decor000.png` … | Use as skybox or level-specific background |
| Sounds | `Content/sounds/sound010.wav` … | Copy directly — same WAV files, same indices |
| World files | `worlds/world001.txt` … | Same format — galaxy-eggbert reads these worlds |
| Game phases | `Def::Phase` enum | Adapt for 3D (same logical phases: Init, Play, Pause, Lost, Win, …) |
| Level constants | `Def::MAXCELX=100`, `MAXCELY=100` | Same grid dimensions — level data is 2D, rendered in 3D |
| Gameplay logic | `Decor.cpp` / `Decor.hpp` | Port to 3D: same state machine, same physics rules, same AI |
| Save data | `GameData.cpp` | Reuse byte-level save format for compatibility |
| Tables | `Tables.cpp` (animation tables, movement tables) | Port directly — same frame indices |

### What differs in galaxy-eggbert (3D-specific)

- **Renderer**: Urho3D scene graph (`Scene`, `Node`, `StaticModel`, `Camera`) instead of 2D sprite batching
- **Tile representation**: Each 2D decor tile → one 3D cube/mesh node with the tile's sprite as a texture on its top face
- **Blupi character**: 3D mesh/model instead of 2D sprite sheet (initially use Box.mdl placeholder until a real mesh exists)
- **Camera**: 3rd-person perspective or isometric, orbiting the player; not a fixed 2D scrolling viewport
- **Sprite sheets as textures**: `decor*.png` tile sprites are applied as textures to cube faces; explosion/effect sprites can use billboard nodes

## Source layout

```
include/GalaxyEggbert/
  Worlds/          — data model: Block, Chunk, World (engine-agnostic, tested)
src/GalaxyEggbert/
  GEEngine.hpp     — includes <Urho3D/Urho3DAll.h>
  GalaxyEggbertApp.hpp / .cpp   — Urho3D Application subclass
  Game/
    GalaxyEggbertGame.hpp / .cpp — main game logic (scene, camera, terrain, HUD)
tests/
  GalaxyEggbert/Worlds/         — unit tests (54 tests, engine-independent)
```

## Engine rules

- **No `#ifdef GE_ENGINE_*` in C++ source code.** Galaxy-eggbert targets pure Urho3D API.
  If Nova3D doesn't compile it, Nova3D must fix itself.
- Entry point is always `URHO3D_DEFINE_APPLICATION_MAIN(GalaxyEggbertApp)`.
- Update loop is always via `SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(...))`.
- Include always `<Urho3D/Urho3DAll.h>` via `GEEngine.hpp`.

## Build

U3D must be pre-built:
```
/rv/data/library/github.com/u3d-community/U3D/cmake-build-debug/lib/libUrho3D.a
```

Default build (U3D, Linux):
```bash
cmake --build cmake-build-u3d --target GalaxyEggbert -j2
./cmake-build-u3d/GalaxyEggbert
```

Use `-j2` maximum to protect RAM (32 GB limit; crashes occurred with more parallel jobs + multiple sessions).

## Code rules

- No comments explaining *what* code does — only *why* when non-obvious.
- No abstractions beyond what the task requires.
- No `#ifdef` guards for engine differences.
- Prefer editing existing files over creating new ones.
- Public headers (used by tests) go in `include/`; private implementation headers go in `src/`.
