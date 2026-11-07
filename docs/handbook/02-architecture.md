# 2. Architecture

The code is layered so that most of the game's rules can be tested without a window. The layers,
from the bottom:

```
CNA (XNA 4.0 API, windowing, audio, GPU)     ../../libcna/cna   (sibling repository)
cna-engine (Camera, GpuMesh, TextureAtlas)   sibling repository
        |
GalaxyEggbertCNA host  src/GalaxyEggbertCNA/        phases, input, tick order, drawing
        |
Game + Editor          src/GalaxyEggbert/Game/, Editor/   rules, renderers, UI
        |
Engine-independent     include/GalaxyEggbert/, src/GalaxyEggbert/Worlds/, *.cpp beside them
model                  World, Chunk, records, registries, Def enums
```

"Engine-independent" is enforced for `include/GalaxyEggbert/` and `src/GalaxyEggbert/Worlds/`
(no CNA or cna-engine include; checked by grep on 2026-11-07, the only hit is a comment). Many files
in `src/GalaxyEggbert/Game/` are engine-independent too, and that is what makes the cheap tests
possible (below).

## The game host: `src/GalaxyEggbertCNA/`

`main.cpp` creates one `GalaxyEggbertGame` (a `Microsoft::Xna::Framework::Game`), parses the
command-line flags and calls `Run()`. Flags:

| Flag | Effect |
|---|---|
| `--mission N` | skip the menus and start mission N in Play |
| `--spawn x,y,z[,yawDeg]` | with `--mission`: place Blupi at that raw-grid position (yaw 0 faces -Z, 90 faces +X) |
| `--autoplay <script>` | play the mission by an autoplay script, write `autoplay_trace.txt`, exit 0 when it passed |
| `--golden-capture` | deterministic screenshots of world 999 for `tests/golden/` |
| `--golden-capture-trace` | deterministic movement trace for `tests/golden/golden_trace.txt` |

`GalaxyEggbertGame.cpp` is very large (about 8 400 lines). Its structure:

- `LoadContent()` loads sprite sheets, sounds, the 3D model libraries (`avatars3d/`), creates the
  `Sound`, `Hud`, `InputPad`, `SaveData` and `WorldEditor` objects.
- `Update()` runs once per fixed 1/60 s step. It handles resume-replay, autoplay, one-time
  start-up modes, then `CaptureInput()` and `UpdateTick()`.
- `UpdateTick()` is a long function: first the per-phase input handling (menus, setup, pause,
  editor, win/lost screens), then, in the Play phase, the simulation order below.
- `Draw()` calls `DrawFrame()`, which draws by phase. Several helpers are in the same file
  (`LoadMission`, `SetPhase`, `ResolvePendingVoyage`, `ApplyCheat`, vehicle and death handling).

`LoadMission(n)` reads `worlds3d/worldNNN.vwr` into `WorldRuntime`, rebuilds the world's
presentation (meshes, models), runs the hub-door logic (`HubDoors`), places Blupi (`PlaceBlupi`,
`BlupiController::SettleOnGround`) and starts recording the resume log.

Platform specifics live here only: Android asset copying (`AndroidAssets.cpp`), Emscripten storage
paths, desktop executable hashing for the resume log.

## The engine-independent model

- `Worlds::World` (`src/GalaxyEggbert/Worlds/World.cpp`) is a 100 x 100 x 100 voxel grid in
  10 x 10 x 10 `Chunk`s. A block is a `uint16_t` (12-bit type, 4-bit metadata). Chunks hold a local
  palette with adaptive bit packing and optional sparse per-block "extra metadata". The world also
  keeps a block revision counter (`blockRevision()`) and a bounded change journal
  (`blockChangesSince()`, 256 entries) that the renderer uses to patch instead of rebuilding.
- Sparse metadata types, each with its own codec: 1 MoveObject (`MoveObjectRecord`), 2 plate turn,
  3 BigDecor (`BigDecorRecord`), 4 hint zone (`HintZoneRecord`), 5 object turn
  (`Game/PlateRotationMetadata.hpp`). Details in `World Format.md`.
- `Def/*.hpp` mirrors the original's enums (`ObjectType`, `BlupiAction`, `SoundChannel`,
  `SpriteChannel`, `GamePhase`, ...). Their numbers must not change.
- `HubDoors` implements the hub door and sign logic.

### The two registries (single sources of truth)

Per-type facts live in exactly two tables. Rendering, collision, editor validation, animation and
interaction code **consume** them; they must not keep parallel lists.

- **`BlockDefinitionRegistry`** (`include/GalaxyEggbert/BlockDefinitionRegistry.hpp`,
  `src/GalaxyEggbert/BlockDefinitionRegistry.cpp`): one `BlockDefinition` per block icon 0-440:
  texture icon, render mode (`UniformCube`, `DirectionalCube`, `InnerPillarBox`, `ThinBar`,
  `InnerFlatPlate`, `TripleCrossBillboard`, `GroundAnchoredPlate`), collision mask, animation
  frames and divisor, and role flags (water, spring, temporary, bridge, switch, hazard kind, fan
  direction, door key, teleporter index, world-sign index, `decor` = passable scenery).
  `GetBlockDefinition(type)` is the accessor. The registry also holds `kSolidIn3DDecor` and
  `kSolidBigDecorIcons`. `tools/DumpBlockDefinitions.cpp` exports it to `tools/data/blocks.csv`,
  which the Python world checkers read; a ctest fails when the CSV is stale.
- **`ObjectDefinitionRegistry`** (`src/GalaxyEggbert/Game/ObjectDefinitionRegistry.{hpp,cpp}`): one
  `ObjectDefinition` per `Def::ObjectType` 0-203: render mode and texture sheet, placement and
  semantic kind (lift, enemy, pickup, projectile, ...), and declarative capability flags
  (`patrolMotion`, `genericContactHazard`, `balloonPoppableHazard`, `standardPickupTouch`,
  `dynamiteDestructible`, `levelExit`, patrol ticks per cell and rest ticks). Use
  `GetObjectDefinition(type)` and `ResolveObjectVisual(type, phase)`.
  `Game/ObjectIcons.cpp` (`GetObjIcon`) maps (type, animation phase) to a sprite icon.

If you add a block or object behaviour, change the registry entry first and let the consumers
follow. `VerifyDefinitionRegistries`, `VerifyGetObjIcon` and `VerifyBlockDefinitionsCsv` guard them.

## How a frame flows (Play phase)

Per 1/60 s step, in the order of `GalaxyEggbertGame::UpdateTick()`:

1. **Input.** `CaptureInput()` snapshots keyboard, mouse, touches and gamepad into one record (the
   resume log stores exactly this record). `InputPad` handles the on-screen pad, the menus and the
   typed cheat words; the keyboard layout is: arrows to turn and walk, Left Ctrl jump, Space action,
   Left Shift crouch, Right Shift look up, Right Ctrl back up, Esc pause, F3 first/third person,
   F5/F6 game speed, Tab slow motion, F12 cheat menu, F11 full screen.
2. **World clock.** `WorldRuntime::Update(dt)` advances the animation phase (20 Hz reference ticks)
   and the per-object phases; `TerrainRenderer::Update` rebuilds the small animated meshes when the
   phase changes.
3. **Blupi.** `BlupiController::Step(...)` is the movement and collision model: yaw turn at 135
   degrees/s, 2.5 cells/s walking, the original's jump speeds, gravity, swimming, ledges, vehicles,
   crouch, hanging on bars, and an animation state (`AnimState`) that selects his clip. It is
   engine-independent: it sees only the `World`, `SolidBox`es and plain floats.
4. **Interactions.** `InteractionSystem::Update(...)` runs lift and creature patrols, pickups, enemy
   and hazard contact, projectiles, dynamite, doors, teleporters and so on, working on the
   `MobileObjSpec` list in `WorldRuntime` and Blupi's position. `UpdateCratePush` handles crates. It
   returns **events** (`EventsThisFrame()`: died, balloon touched, pickup, voyage requests...) and
   the host applies the state changes that need the controller, camera or HUD. This is how the
   invariant "`InteractionSystem` has no controller, camera or graphics dependency" is kept
   (`InteractionSystem.hpp` includes only `Sound.hpp`, `WorldRuntime.hpp` and `Def/`).
5. **World changes.** Tile reactions (switches, bridges, doors) change blocks through the `World`;
   `RefreshTerrainMeshIfChanged()` uses the change journal to patch the terrain, falling back to a
   full rebuild for unknown history.
6. **Camera and presentation state.** The chase camera (`ChaseCameraYaw`, `CameraShake`, the
   cna-engine `Camera`) follows Blupi; the HUD, sounds and effects are updated.
7. **Draw** (`DrawFrame`): clear, sky background (`Content/backgrounds/decorNNN.png` selected by the
   world's sky region), `TerrainRenderer::DrawOpaque` and `DrawGrass`, objects and Blupi (3D models
   from `avatars3d/` where a model exists, otherwise billboards cut from the sprite sheets), the
   transparent terrain pass, then 2D overlays: `Hud`, `InputPad::DrawPlay`, the cheat overlay.

Menus and screens (`Init`, `Wait`, `Pause`, `Win`, `Lost`, setup, resume) are phases handled by the
same functions with `InputPad` doing hit-testing in the original's 640 x 480 layout space.

### Determinism

The tick is fixed and everything in the Play path is deterministic, which is what the golden
files and autoplay scripts depend on. `AUTOPLAY_REALTIME=1` only changes pacing, not results. Do
not introduce wall-clock time, unseeded randomness or frame-rate dependence into the simulation.
The resume feature (`ResumeLog`) relies on it too: an interrupted mission is saved as its start
plus the recorded input stream and replayed on continue, and it refuses a replay if the world
bytes or the build changed (`plans/audit/behavior_differences.md`, BD-0077).

## Rendering pipeline in outline

- **Tile atlas.** `TileAtlas` maps block icons to UV rectangles in `object-m.png` (a padded copy
  with 16 px borders is used for mipmapping). `BlockTypes::tileUV` is the pure function behind it.
- **Terrain meshes.** `TerrainRenderer` builds static meshes from the world: opaque cubes, a
  cut-out mesh (sprite tiles with hard edges, per `TileCoverage`), an alpha-blended mesh, the grass
  overlay, and small animated and water meshes rebuilt when the animation phase advances. Render
  modes other than cubes have helpers (`DirectionalCubeTiles`, `InnerPillarBoxTiles`,
  `ThinBarTiles`, `InnerFlatPlateTiles`). Lighting is one directional light plus ambient
  (`TerrainLighting.hpp`).
- **Models in the terrain.** `BlockModels.hpp` lists the tiles drawn as original 3D models
  (fans, vanishing saucers, crushers, drips, pillars, switches, signs, bridges...).
  `TerrainRelief` lays each terrain tile's Ray Dream relief on exposed faces near the camera target
  (`avatars3d/terrain/`).
- **Object models.** `ObjectModelTable` reads each `avatars3d/objects/<name>/icons.txt`: which sprite
  icon is drawn as which pose of which model. An icon not listed stays a billboard. Models are CNA
  `SkinnedModelEXT` directories drawn with `SkinnedEffect`. Blupi (`avatars3d/blupi/`) has one clip per
  `BlupiController::AnimState`; if the directory is absent a fox placeholder
  (`avatars3d/blupi_placeholder/`) is used.
- **Sprites and 2D.** `QuadBatch`, `Hud` and `InputPad` draw in the original's 640 x 480 space,
  scaled to the window (menus fill the side bars with their images). Text uses `Content/icons/text.png`
  with the original proportional widths.
- **Sound.** `Sound` plays the 93 channels of `Content/sounds/` with the original's per-channel
  volume and pitch table, positioned from Blupi's place.

## The editor: `src/GalaxyEggbert/Editor/`

`WorldEditor` owns one editing session: free camera (W/A/S/D, Space and Left Ctrl for height, mouse
look), ray-cast placement (`VoxelRaycast`, aiming at the mouse cursor or touch point), the palette
(`EditorPalette*`, `PaletteCategories`; ten groups in Eggbert 2's order), object and BigDecor
editing, undo/redo over one command history (`EditCommandStack`), box fill (`BoxRegion`), copy and
paste (`Clipboard`), variant cycling (`EditorVariants.hpp`), auto-tiling (`AutoTile.hpp`), music,
sky region and spawn point, and a save/play-test loop. `CustomWorldStorage` keeps ten slots per
gamer, three gamers, as `customworlds/gamerG/custom_00N.vwr` relative to the working directory
(`/save/customworlds` on the web), with backups, export and import, and thumbnails. Slot 1 is
seeded from the showcase world and a further slot from the gallery. `EditorBrowserScreen` is the
slot list, drawn over the title scene. The editor uses the same registries, so what it can place and
validate follows the tables above. `VerifyGEWorldEditor`, `VerifyGEEditorPalette`,
`VerifyGEEditorStorage` and `VerifyGEEditorObjects` drive it without a window.

## The autoplay harness

`src/GalaxyEggbert/Game/Autoplay.{hpp,cpp}` parses a script and, each tick, returns the keys to
hold given the game's state; `GalaxyEggbertGame::StepAutoplay()` feeds them in instead of the
keyboard and writes `autoplay_trace.txt`. It is a QA tool, not a player feature, and is
engine-independent so `VerifyAutoplay` tests it. Script syntax and usage are in
[04-testing.md](04-testing.md).

## Pitfalls worth knowing

- **Never edit a generated file by hand.** `avatars3d/**`, `tools/data/blocks.csv`,
  `BoltPaths.hpp`, `GlueStrands.hpp`, `LavaSurfaceData.hpp`, `plans/inventory/*.md` and parts of
  `plans/audit/` are generated; edit the generator ([06-assets-and-tools.md](06-assets-and-tools.md)).
- **Changing a registry or the world format has wide effects**: world files, the Python checkers
  (`tools/vwr_inspect.py` and `tools/worldkit.py` hand-implement the reader and writer), golden
  files, autoplay timing. Run the full set in [04-testing.md](04-testing.md).
- **Crates, lifts and creatures are timed in 20 Hz ticks** copied from the original; a world that
  uses them is tuned to those numbers, and so are the autoplay scripts. A small gameplay change can
  break a playthrough's timing without any compile error.
- **Block 214 (flight barrier) is invisible in play** by design; many tiles with empty collision
  masks are passable scenery (`BlockDefinition::decor`).
- **Comments in `InteractionSystem.hpp` and `GalaxyEggbertGame.hpp` are long historical notes**;
  some describe states that no longer exist (for example a single-world save). Trust the code and
  the ledgers over them.
- **Code comments cite `plan.md` ids and dates** (`E3D-MIG-...`, `BD-...`, `OR-...`); the ledgers in
  `plans/audit/` and `git log` explain them ([07-contributing.md](07-contributing.md)).
