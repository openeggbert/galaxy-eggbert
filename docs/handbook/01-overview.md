# 1. Overview, rules and repository map

## What the project is

Galaxy Eggbert is a faithful 3D remake of *Mobile Eggbert* / *Speedy Blupi* (a 2D platform
game by Daniel Roux and EPSITEC, ported to C++ as `mobile-eggbert`). The player walks Blupi
through hub worlds and missions in free 3D space (movement in X and Z, real heights), collects
treasures, eggs and keys, avoids enemies and hazards, and reaches the exit.

- The only game target is **`GalaxyEggbertCNA`** (CMake target name; the executable file is called
  `GalaxyEggbert`). It is built on **CNA**, a C++ reimplementation of the XNA 4.0 API, with two
  strict modules of **cna-engine** (`CnaEngine::Core`, `CnaEngine::Graphics`: camera, mesh builder,
  GPU meshes, texture regions) as helpers. Game code calls CNA directly; the helpers do not hide
  it.
- The campaign has 78 worlds: the main hub (`world001`), twelve theme hubs
  (`world010`, `world020`, ... `world120`), 64 missions, and the bonus world `world199`. Two more
  worlds are Galaxy's own: `world999` (a mechanics showcase) and `world998` (a gallery of every
  block, object and big decor). `title.vwr` is the scene behind the main menu.
- Worlds are `.vwr` voxel files in `worlds3d/`, authored by Python scripts (`tools/worlds/`) or in
  the game's own 3D editor.
- A retired earlier implementation (`GalaxyEggbertSimple3D`, built on a helper library called
  easy-3d) was removed from the tree on 2026-07-25. It is recoverable at git commit `4afd53e`;
  do not revive it or add another engine target.

## The rules

These are the project owner's rules, taken from `CLAUDE.md`, `AGENTS.md`, `CURRENT.md` and
`plans/GALAXY_EGGBERT_COMPLETION.md`. They are not style preferences; most defects in the project's
history came from breaking one of them.

### Fidelity

1. **Every gameplay feature must trace to confirmed Mobile Eggbert behaviour.** Do not invent a
   mechanic because it seems plausible. Where a 2D rule has no unique 3D reading, pick a reading,
   keep it small, and record it as an "intentional 3D adaptation" (`plans/audit/mechanics_3d_decisions.md`,
   rows `M3D-NNNN`).
2. **`../mobile-eggbert` is the read-only reference.** Never modify it. Its C++ source, mainly
   `Decor.cpp`, is the authority for gameplay numbers and timing. Do not copy its code, data tables,
   enum values, byte layouts or sprite-frame tables into this repository without the owner's
   approval; since 2026-10-25 a standing approval covers *small* data tables needed for fidelity
   (glyph widths, animation and timing tables), with the source cited in a comment. Larger code
   needs asking first.
3. **Reuse approved media by their existing paths** (`Content/`). Do not add new copied asset sets
   without approval.
4. **Preserve the real numeric IDs**: `Def::ObjectType`, block (icon) numbers, sound channels. They
   encode reference data and are stored in world files. Never renumber.
5. **Worlds are hand-authored true 3D designs.** The original's 2D maps give a world's identity,
   theme, mechanics, difficulty and progression and nothing else. No conversion, extrusion, tracing,
   side-view "2.5D", lane locking or row-to-row mapping. A 2.5D side-view conversion was tried and
   rejected by the owner (2026-10-25); the plan records that its branches are kept only as notes.
6. **Models and animations come from the original Ray Dream sources** whenever they can be
   recovered (see [06-assets-and-tools.md](06-assets-and-tools.md)); never a "modernised"
   redesign. Effects (explosions, sparks, glows) may stay billboards.
7. **The in-game 3D world editor is the one approved exception**: its UX may be invented, but it
   must not add gameplay behaviour. Its palette order follows the Eggbert 2 editor where the
   original specifies one.

### Repository boundaries

- Do not modify the sibling repositories (`../cna`, `../cna-engine`, `../easy-gl`,
  `../sharp-runtime`, `../meta-gl`, `../mobile-eggbert`) unless the task is explicitly scoped to
  them. A defect found there is written down in `upstream_bugs.md` as `UB-NNN`
  ([07-contributing.md](07-contributing.md)); a bug in cna-engine should be fixed in cna-engine
  rather than worked around here.
- Keep `InteractionSystem` free of controller, camera and graphics dependencies; it talks to the
  rest of the game through events and requests ([02-architecture.md](02-architecture.md)).
- Keep the engine-independent data model (`include/GalaxyEggbert/`, `src/GalaxyEggbert/Worlds/`)
  free of CNA and cna-engine includes.
- Avoid backend-specific branches in game logic.
- Reuse existing engine-independent code and its verification tests before re-deriving math or
  reference behaviour. `missing.md` records cases where that was missed.

## Repository map

Directories that are generated or machine-local are marked.

| Path | What lives there |
|---|---|
| `src/GalaxyEggbertCNA/` | the thin application host: `main.cpp`, `GalaxyEggbertGame.{hpp,cpp}` (the XNA `Game` subclass: phases, input, the per-tick update, drawing), `AndroidAssets.{hpp,cpp}` |
| `src/GalaxyEggbert/Game/` | the game implementation, namespace `GalaxyEggbert::Game`: `BlupiController`, `InteractionSystem`, `WorldRuntime`, `TerrainRenderer`, `Hud`, `InputPad`, `Sound`, `SaveData`, `ResumeLog`, `Autoplay`, the object registry and many render helpers |
| `src/GalaxyEggbert/Editor/` | the in-game 3D editor, namespace `GalaxyEggbert::Editor` |
| `src/GalaxyEggbert/Worlds/` | the voxel container: `World`, `Chunk`, bit packing, binary I/O (engine-independent) |
| `src/GalaxyEggbert/*.cpp` | record codecs and tables: `MoveObjectRecord`, `BigDecorRecord`, `HintZoneRecord`, `HubDoors`, `BlockDefinitionRegistry` |
| `include/GalaxyEggbert/` | public engine-independent headers: `Worlds/`, `Def/` (the original's enums), `BlockTypes.hpp`, `BlockDefinitionRegistry.hpp`, records |
| `tools/` | verification executables (`Verify*.cpp`), the sample-world generator, Python world and asset tools, shell QA scripts; subdirectories below |
| `tools/worlds/` | one generator per campaign world plus shared kits ([05-worlds.md](05-worlds.md)) |
| `tools/audit/` | generators for the ledgers in `plans/audit/` |
| `tools/autoplay/` | scripted playthroughs, one per world, plus `probes/` and `spare/` |
| `tools/qa_steps/` | step files for `tools/qa_live.sh` (the editor checks) |
| `tools/rd4_to_gltf/`, `tools/terrain_relief/`, `tools/a3d/`, `tools/bolt/`, `tools/glue/`, `tools/music/` | asset converters ([06-assets-and-tools.md](06-assets-and-tools.md)) |
| `tools/data/blocks.csv` | block classes exported from the C++ registry for the Python tools (kept in sync by a ctest) |
| `tests/GalaxyEggbert/` | GoogleTest unit tests of the world model and records |
| `tests/golden/` | approved golden frames and trace ([04-testing.md](04-testing.md)) |
| `worlds3d/` | the shipped `.vwr` worlds (81 files: 78 campaign worlds, 998, 999, `title.vwr`) |
| `worlds/` | five old text-format maps (`world001.txt`-`world005.txt`); the game does not read them (not verified for every tool) |
| `Content/` | runtime 2D assets: `backgrounds/`, `icons/` (sprite sheets), `sounds/`, `music/` (rendered by `tools/music/render_music.py`), `Content.mgcb` |
| `avatars3d/` | runtime 3D models in CNA's `SkinnedModelEXT` format: `blupi/`, `objects/`, `terrain/`, `blupi_placeholder/` (fallback) |
| `textures3d/` | Galaxy's own 3D-specific textures (`grass_top.png`) |
| `plans/` | plans, per-world briefs (`briefs/`), the fidelity-audit ledgers (`audit/`), evidence images (`evidence/`), generated inventories (`inventory/`), old task briefs (`codex/`) |
| `mobile-eggbert-reference/` | researched notes on the original's behaviour (tiles, objects, sounds, animations, mechanics) |
| `android/` | Gradle project for the APK (`app/`, `gradlew`, `local.properties.template`) |
| `cmake/` | `CopyDirectoryIfDifferent.cmake`, `toolchains/mingw-w64.cmake`, `web/` (Emscripten HTML shells) |
| `packaging/` | Linux runtime-bundle launcher template and README |
| `web/` | the project's static website (deployed by `.github/workflows/pages.yml`); not the game's web build |
| `.github/workflows/`, `.github/actions/` | `cna-ci.yml` (build and test on pushes to `develop` and `master`), `release.yml` (a version tag builds and publishes the release), `pages.yml` (website); `actions/checkout-siblings/` holds the siblings' pins |
| `docs/handbook/` | this handbook |
| `build-cna-opengl3/`, `build-cna-webgl2/`, `build/`, `build-probe/` | build trees and scratch output; git-ignored, machine-local |
| root documents | `CURRENT.md`, `handoff.md`, `plan.md`, `NEXT.md` (frozen), `missing.md`, `REMAKE-ANALYSIS.md`, `renderers*.md`, `easy3d.md` (historical migration analysis), `upstream_bugs.md`, `World Format.md`, `ANDROID.md`, `WINDOWS.md` |

## Glossary

**Mission** - a numbered world. 1 is the main hub; N0 (10, 20, ... 120) is theme hub N; N1...
are that theme's missions; 199 is the bonus world. A world's file is `worlds3d/worldNNN.vwr`
(`MissionWorldPath` in `GalaxyEggbertGame.cpp`), and its mission number is stored in the header.

**Hub** - a world whose signs open other worlds. The main hub carries twelve world signs; each
theme hub carries one sign per mission, with blue bars (tiles 182/183) that open when the previous
mission is won (`src/GalaxyEggbert/HubDoors.cpp`). Each theme's last mission ends with the golden
key (`ObjectType` 21).

**Block / block icon number** - a voxel's type. The value equals the **icon index into
`Content/icons/object-m.png`**, the original's tile sheet (64 px tiles on a 65 px pitch, 20
columns, icons 0-440; 0 is air). A stored block is a 16-bit value: 12 bits of type and 4 bits of
metadata. Named constants are in `include/GalaxyEggbert/BlockTypes.hpp`; what each number *means*
(collision, hazard, render mode, animation) is in `BlockDefinitionRegistry`.

**Tile** - used loosely for a block, or for its picture in the sheet.

**MoveObject** - a moving or interactive object of the original (pickups, enemies, lifts, crates,
vehicles, effects): the original's `MoveObject:` lines. Stored per world as sparse metadata (type
1): an object type byte, a start and end position, a speed and four patrol-timing floats, and an
optional visual icon. Types are `Def::ObjectType` (`include/GalaxyEggbert/Def/ObjectType.hpp`).
Runtime form: `MobileObjSpec` in `WorldRuntime.hpp`.

**BigDecor** - a large scenery picture of the original (palms, trees, houses, machines, toys,
slime, palace pieces; icons from `Content/icons/explo.png`) placed as a record (metadata type 3)
rather than a voxel. Galaxy draws most as 3D models, and 31 icons are solid
(`kSolidBigDecorIcons`), so a world must not put one in a route the player has to take.

**Hint zone** - a box in a world (metadata type 4) that selects one of the mission's training
hints, replacing the original's 2D map rectangles.

**`.vwr`** - Galaxy's native world file (`VWR1` header, `VCH1` chunks), a 100 x 100 x 100 voxel
grid in 10 x 10 x 10 chunks. See `World Format.md` and [05-worlds.md](05-worlds.md).

**Raw grid coordinates** - X, Y, Z in 0-99, Y up, as `.vwr` files, the editor, the scripts and
`--spawn` use. A block at height y makes y + 1 the walking level. At run time the game shifts X and
Z by the world centre (`WorldRuntime::kWorldCenterX/Z`); scripts and tools stay in raw coordinates.

**Tick** - the game simulation runs on a fixed 1/60 s step (`UpdateTick`). The original runs at
20 Hz; creature, lift and animation timings are stored in 20 Hz ticks and converted.

**Phase** - the game's screen state, `Def::GamePhase` (Init, Play, Pause, Win, Lost, MainSetup,
PlaySetup, Resume, Editor, ...).

**CNA** - the engine under the game (repository `libcna/cna`): XNA 4.0's namespaces
(`Microsoft::Xna::Framework`...) reimplemented in C++, with several graphics back ends selected by
the CMake cache variable `CNA_GRAPHICS_RENDERER`.

**cna-engine** - helper modules over CNA (`CnaEngine::Core`, `CnaEngine::Graphics`) restricted to XNA
4.0 types. It replaced the former helper library easy-3d on 2026-10-20.

**XNA types** - `Vector3`, `Matrix`, `Texture2D`, `GraphicsDevice`, `BasicEffect`, `Keys`... as CNA
provides them. The engine-independent model uses plain floats instead, so it can be tested without
a graphics context.

**Ray Dream scene (`.rd4`, `.rds`)** - the 3D scene files the original's sprites were rendered
from. They live in the sibling data repository `speedyblupi-data` and are converted to the
game's models by `tools/rd4_to_gltf/`.

**Ledger** - a numbered table of findings in `plans/audit/`: `OR-` (owner reports), `BD-`
(behaviour differences), `M3D-` (3D adaptations), `ED-` (editor parity), `LV-` (world lessons),
`MR-` (manual-review items for the owner), `Q-` (owner questions). See
[07-contributing.md](07-contributing.md).

**Autoplay** - the scripted-playthrough harness: a text script feeds key presses to the running
game and checks conditions ([04-testing.md](04-testing.md)).

**Golden frames / trace** - committed reference screenshots and a movement trace the game must
reproduce byte for byte ([04-testing.md](04-testing.md)).
