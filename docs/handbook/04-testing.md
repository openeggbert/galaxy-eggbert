# 4. Testing and verification

There are five layers, from cheap to expensive. Run the cheap ones all the time and the expensive
ones whenever you touch what they cover.

| Layer | Cost | Covers | Run |
|---|---|---|---|
| ctest | about 2 s once built | rules, data model, registries, editor logic, shipped worlds load | always |
| Python world checks | seconds | reachability, structure, water, timing envelopes | after a world change |
| Golden frames and trace | under a minute, needs a display | rendering and movement did not change | after a render or movement change |
| Autoplay playthroughs | minutes per world, about two hours for the campaign | every mission can still be finished | after a gameplay or world change |
| Live QA (`qa_live.sh`) | manual | what it looks like, UI flows | visual and editor changes |

Compilation alone is not evidence of visual parity; look at the result.

## ctest

```bash
ctest --test-dir build-cna-opengl3 -N            # list the 112 tests
ctest --test-dir build-cna-opengl3 -j4 --output-on-failure
ctest --test-dir build-cna-opengl3 -R VerifyShippedWorlds
```

Today (2026-11-07) all 112 pass. What is in the list (`CMakeLists.txt` registers them):

**`GalaxyEggbertWorldsTests`** (GoogleTest, 82 cases, one ctest entry each via `gtest_discover_tests`;
sources in `tests/GalaxyEggbert/`): `Block`, `BitPacking`, `Chunk`, chunk and world serialization
(round trips and rejection of bad files), `BlockMetadata`, `MoveObjectRecord`, `BigDecorRecord`,
`HintZoneRecord`, `HubDoors`. These run without CNA.

**`Verify*` executables** (plain `main()` that prints `PASS:`/`FAIL:` lines and returns non-zero on
failure; sources in `tools/`):

| Test | What it checks |
|---|---|
| `VerifyBlupiMovement` | `BlupiController` against `world999`: walking, jumping, gravity, collision, crates as boxes, swimming, vehicles, one-shot sound cues. Runs from the repo root |
| `VerifyInteractionSystem` | `InteractionSystem` against `world999`: lifts, crate push and pull timing against the original's tables, pickups, enemy and hazard contact, dynamite, doors. Runs from the repo root |
| `VerifyLocalization` | the texts in every language: UTF-8 drawn by text.png's glyphs (accented letters, broken sequences), every `Tr`/`TrMark`/`TrFormat` literal of `src/` and `include/` translated, no translation left over, placeholders kept, the training hints' French drawable. Takes the source directory |
| `VerifyShippedWorlds` | every `worlds3d/*.vwr` loads through the game's reader, carries the mission number of its file name, gives `WorldRuntime` its objects, and survives load, save, load unchanged |
| `VerifyBlockDefinitionsCsv` | `DumpBlockDefinitions --check tools/data/blocks.csv`: the committed CSV equals the C++ registry |
| `VerifyDefinitionRegistries` | the block and object registries: render modes, roles, capabilities, texture sheets |
| `VerifyGetObjIcon` | `GetObjIcon` frame arrays and divisors (a regression lock) |
| `VerifyObjIconAgainstReferenceDoc` | `GetObjIcon` against the tables parsed out of `mobile-eggbert-reference/08-animations.md` |
| `VerifyObjectModelTable` | `avatars3d/objects/` manifests and `icons.txt` files are well formed |
| `VerifyTileUvBounds`, `VerifyTileCoverage`, `VerifyTileFaces`, `VerifyTerrainLighting`, `VerifyTerrainAnimDivisor`, `VerifyUnverifiedRenderMapping` | pure render-support functions: atlas rectangles inside the sheet, cut-out versus opaque tiles, faces of directional tiles, light, animation divisors, a bookkeeping table of unconfirmed render mappings |
| `VerifyCameraShake`, `VerifyChaseCameraYaw` | the camera state machines |
| `VerifyAutoplay` | the autoplay script parser and its commands against a stand-in Blupi |
| `VerifyGESaveData` | `SaveData` round trip (runs in `build-cna-opengl3/verify-savedata/`) |
| `VerifyGEInputPad` | on-screen pad and menu hit-testing with synthetic mouse state |
| `VerifyGEWorldEditor`, `VerifyGEEditorPalette`, `VerifyGEEditorStorage`, `VerifyGEEditorObjects` | the editor: placement, undo, palette, slots, backups, objects. Share one object library (`GEEditorVerifySupport`) |
| `VerifyMoveObjectTypesCna`, `VerifyBigDecorParsingCna` | parse real `../mobile-eggbert/worlds/*.txt` files (need that sibling; run from the repo root) |

The five `easy-gl-*` tests come from the dependency.

### Adding a test

1. Write `tools/VerifyYourThing.cpp` with the `check(cond, "what")` / `ALL CHECKS PASSED` pattern
   used by `tools/VerifyCameraShake.cpp`. Keep it deterministic and free of audio and windows.
2. In `CMakeLists.txt` add an `add_executable` with the sources it links (engine-independent files
   from `src/GalaxyEggbert/Game/` plus `${GE_SHARED_SOURCES}` when it needs the world model), the
   include directories `include` and `src`, and an `add_test(NAME ... COMMAND ...)` inside
   `if(BUILD_TESTING)`. Give it `WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}` if it opens repo-relative
   paths. If it touches CNA types (audio, `Texture2D`), copy the link pattern of
   `VerifyInteractionSystem` or `VerifyGEInputPad`; editor tests use `ge_add_editor_verify`.
3. Prefer logic that stays engine-independent so that the test needs no GPU. That is why
   `BlupiController`, `InteractionSystem` and `Autoplay` are written without graphics.
4. For a unit test of the world model add a `TEST` to `tests/GalaxyEggbert/` and list the file in
   the `GalaxyEggbertWorldsTests` target.
5. A gameplay fix should come with a test that fails without it, and a regression test for a bug
   found by playing is the project's usual way to close a ledger row.

## Golden frames and trace

`tests/golden/` holds three screenshots and a movement trace of the showcase world
(`golden_frame_0060.png`, `_0120`, `_0180`, `golden_trace.txt`). They are exact byte comparisons, so
any rendering change shows up. They need a real or virtual display and are not in ctest or CI.

```bash
xvfb-run -a tools/verify_golden_frames.sh build-cna-opengl3
xvfb-run -a tools/verify_golden_trace.sh build-cna-opengl3
```

Each script runs the game with `--golden-capture` / `--golden-capture-trace` in the build directory
(20 s timeout), then `cmp`s the fresh files against the references and prints `PASS`/`FAIL`. The trace
mode additionally drives Blupi through a fixed walk and jump and records position, vertical speed,
grounded state and animation per tick, so movement and collision code is exercised.

**Re-baselining.** Do it only when the change is intended (a deliberate look or physics change),
never to silence a failure. Look at the new images beside the old ones and, for the trace, at
`diff`'s output (the script prints the first differences); then copy the fresh files over the
references and commit them on their own with the reason, as the history does (for example
`test(golden): re-baselined after the ball cubes' tops ...`):

```bash
cd build-cna-opengl3
xvfb-run -a ./GalaxyEggbert --golden-capture ; xvfb-run -a ./GalaxyEggbert --golden-capture-trace
cp golden_frame_0060.png golden_frame_0120.png golden_frame_0180.png golden_trace.txt ../tests/golden/
```

(The `cp` line is the procedure as the scripts imply it; this session did not run it.) The files were
reproducible run to run on the maintainer's machine; whether they are byte-identical on other
drivers is not verified.

## Autoplay playthroughs

A script plays a mission through, tick for tick, against the real game.

```bash
tools/autoplay.sh build-cna-opengl3 tools/autoplay/world012.txt 12            # one mission
tools/autoplay_all.sh build-cna-opengl3                                       # every mission script
tools/autoplay_all.sh build-cna-opengl3 world011 world121                     # chosen ones
PLAY_PASS_JOBS=4 tools/play_pass.sh build-cna-opengl3 build-probe/play-pass-NN   # all, with frame sheets
```

`autoplay.sh` runs the game under its own Xvfb display from `build-probe/autoplay-run/`
(`AUTOPLAY_RUN=<dir>` for another, needed when two runs overlap) with a fresh save and symlinked
assets, prints `autoplay_trace.txt` and the game's `autoplay ...` lines, and exits with the game's
code: 0 when the script passed. It is silent (`SDL_AUDIODRIVER=dummy`) unless `QA_SOUND=1`.
`AUTOPLAY_WORLDS=<dir>` substitutes the `worlds3d` directory, `AUTOPLAY_REALTIME=1` paces at 60 Hz
instead of running flat out, `AUTOPLAY_SHOTS=N` saves a frame every N ticks into the run
directory's `autoplay_shots/`. `GALAXY_RESUME_SELFTEST=<ticks>` exercises the resume feature during
a run. Extra game arguments follow the mission number (for example `--spawn x,y,z,yaw`).

`play_pass.sh` plays every `tools/autoplay/world*.txt` (or the named worlds) in parallel lanes, writes
`results.txt`, and lays the kept frames out as contact sheets (`tools/play_pass_sheet.py`), a look at
each route as the player sees it. The last recorded full pass: 78 of 78 on 2026-11-07, on commit
`5d371662` (`handoff.md`); world 199's route has been reworked since, so replay it.

**The scripts.** `tools/autoplay/worldNNN.txt` plays mission NNN. A theme hub's script
(`world010.txt`, ...) is a chain: it `include`s its missions' scripts and walks through each sign and
back, to the golden key. `tools/autoplay/campaign.txt` plays a new game through all twelve worlds
and the bonus world (written by `python3 tools/autoplay_campaign.py`; about two hours). `probes/`
holds one-off scripts for a specific question; `spare/` holds alternate routes (run alone, not part
of the pass). A script whose first line says `BLOCKED` is skipped by `autoplay_all.sh`.

**The language** (authoritative text: the header of `src/GalaxyEggbert/Game/Autoplay.hpp`; one
command per line, `#` comments; coordinates are raw grid, a cell's centre is its integer value):

| Command | Meaning |
|---|---|
| `wait N` | no keys for N ticks (1/60 s) |
| `hold N KEY...` | hold keys: `up down left right jump action crouch lookup` |
| `type WORD` | type letters (the cheat words) |
| `face DEG` | turn on the spot (0 faces -Z, 90 faces +X) |
| `go X Z [jump D] [tol T] [max N] [until COND] [with KEY]...` | walk towards a point; optionally jump once when within D |
| `fly X Y Z ...` | like `go`, holding a height (helicopter, overcraft) |
| `until COND [max N]` | no keys until the condition holds |
| `lift X Y Z`, `crate X Y Z [tol T]` | wait until a lift platform or a crate is at that place |
| `clear X Y Z R [lane x\|z]`, `near ...` | wait until no enemy (clear) or an enemy (near) is within R: time a pass by a patrol |
| `expect COND` | record a pass or a failure |
| `log TEXT`, `trace N`, `shots N`, `end`, `include FILE` | trace and flow control |

A condition is `FIELD OP VALUE` with OP one of `= != < <= > >=`. Fields: `x y z yaw tick mtick
mission treasures eggs lives keys dynamite vehicle ground dead ghost anim exit blitz temp switches
saws crush phase` (`vehicle` is accepted by the parser but missing from the header comment;
`phase` takes a name such as `Play` or `Win`). `tick` counts from the script's start; `mtick` from
the current mission's start, so use `mtick` or a cycle field, never `tick`, in anything that can run
inside a chain. A command that runs out of ticks, or Blupi dying, ends the run as a failure.

**Writing a script** (from the project's experience):

- Write a section, run it, read the trace, repeat. `trace N` logs every tick, `shots N` saves frames.
- Jumps need a run-up and take off at an edge; jumps two up take off close to the wall.
- Pickups need Blupi within about 0.6. Dynamite needs `hold 2 action`. A door spends all the keys.
  Eggs cap at ten per mission.
- Lifts: wait on the leading cube's unique end cell and board at the start of a dwell.
- Cycles (vanishing saucers, lightning, crushers): sync on `temp`, `blitz`, `crush`.
- To tune a delay, write it as `wait {D}` in a template and run
  `python3 tools/autoplay_search.py <template> "<mission> [--spawn x,y,z,yaw]" <from> <to> <step>`;
  take the middle of the passing range so the script keeps a margin. After a timing change in the
  game, `python3 tools/autoplay_chains.py <world-number> --jobs 3` re-times synced chains
  automatically.
- If a script fails after an engine change, first check whether the mechanic changed against the
  reference; a real engine defect gets a test before the script is "fixed".

## Live QA with step files

`tools/qa_live.sh <build-dir> <out-dir> <steps-file> [game args]` runs the game under its own Xvfb
display (needs `xvfb-run`, `xdotool`, ImageMagick `import`) from `build-probe/qa-run/` and executes a
step file: `sleep S`, `shot NAME` (PNG of the window into `<out-dir>`), `key KEYSYM`, `keydown`,
`keyup`, `click X Y`, `mousedown`, `mouseup`, `move X Y`, `focus`. Environment: `QA_LIVE_RUN=<dir>`
(a separate run directory so two runs can overlap), `QA_LIVE_AVATARS=<dir>` and `QA_LIVE_WORLDS=<dir>`
(substitute asset directories, for example to look at a re-exported model or a probe world before it
replaces the committed one), `QA_SOUND=1`. Step files for the editor live in `tools/qa_steps/`
(`editor_playtest.txt`, `editor_copy_paste.txt`, ...). `tools/qa_sweep.sh <build-dir> [out-dir]` loads
every shipped world, walks ahead two seconds, takes a screenshot and writes a line per world to
`result.txt`. `tools/qa_original.sh <port-dir> <out-dir> <steps>` does the same for a *copy* of the
original game's Linux build, to see what the original does where the source leaves it open (never run
or change the original in place).

## Python checking tools

All are read-only unless stated, run from the repo root with `python3`.

| Tool | Checks |
|---|---|
| `tools/levelcheck.py worlds3d/worldNNN.vwr [--rides]` | conservative reachability from the spawn to every treasure, key, egg and exit (walk, fall, jump, lifts, swimming, teleporters); also traps (one-way drops without a way out) and, in a hub, the signs. It does not model crates, dynamite, vehicles or timing, so a world built around those reports `FAIL` unless you check a "solved" variant (below) |
| `tools/flightcheck.py worlds3d/worldNNN.vwr [--mine-margin 0]` | the same plus the helicopter |
| `tools/campaigncheck.py` | the campaign's structure: the main hub has twelve world signs, each hub its mission signs (4, 5, 4, 6, 8, 6, 5, 4, 5, 7, 5, 5), every mission exists with a spawn and one way out (red arrow, or the golden key for a theme's last mission) |
| `tools/watercheck.py [worlds...]` | every body of water has a way out for a swimming Blupi |
| `tools/audit/patrol_envelope.py` | every creature of the shipped worlds uses its type's original speed and rest; must end with `creatures off their type's original timing: 0` |
| `tools/audit/lift_envelope.py` | no lift faster than the fastest original one; must report `lifts faster than the original's fastest: 0` |
| `tools/audit/target_reach.py <play-pass-dir>` | for each of the 921 treasures, eggs, keys and exits, the evidence it is reachable (levelcheck, the playthrough's trace) |
| `tools/vwr_inspect.py [--summary] [--objects 7,21] worlds3d/*.vwr` | read-only dump: header, blocks per type and height, objects, BigDecor, hint zones |
| `tools/world_inventory.py`, `tools/asset_inventory.py [--check]`, `tools/model_parity.py` | write the tables in `plans/inventory/` |

Example of the "solved variant" idea: `python3 tools/worlds/world012.py --solved <out.vwr>` puts
the crates where the lessons end up and `levelcheck.py` on that file reports the exit reachable,
while the real world 012 reports `FAIL` (exit "not reachable"): the crates must be moved first.
Both results were reproduced on 2026-11-07. Other generators have staged variants with their own
flags (`--stage`, `--check STAGE out.vwr`); read the script's docstring.

Checks that exercise the sibling reference need `../mobile-eggbert`; the audit generators
(`tools/audit/*.py`) regenerate the ledgers' tables and have a `--check` mode that reports staleness
([07-contributing.md](07-contributing.md)).

## Which checks for which change

| You changed | Run at least |
|---|---|
| a world generator | the generator, `levelcheck.py` (and a solved variant if needed), `campaigncheck.py`, `watercheck.py`, `VerifyShippedWorlds`, `patrol_envelope.py`, `lift_envelope.py`, the world's autoplay script, a live look |
| gameplay (`BlupiController`, `InteractionSystem`, registries) | ctest, golden trace, the affected missions' autoplay, ideally a full play pass |
| rendering | ctest, golden frames, a live look at the affected worlds (`world998` shows every block and object) |
| the editor | the four editor verifiers, the relevant `tools/qa_steps/editor_*.txt` |
| `World`/record format | ctest, `tools/vwr_inspect.py` and `tools/worldkit.py` (update both), regenerate nothing blindly: the world files must still load |
| a block's class in `BlockDefinitionRegistry` | rebuild, refresh `tools/data/blocks.csv` with `build-cna-opengl3/DumpBlockDefinitions > tools/data/blocks.csv` (run from the repo root), then the world checks |
