# handoff.md — where work stands and how to continue

Replaces `NEXT.md` (obsolete since 2026-10-24, kept frozen as history; do not add to it). Update
this file after every committed task: what was done, where it stopped, what to run next. Keep it
short; the backlog with checkboxes is `plan.md`, verified current state is `CURRENT.md`.

_Last updated: 2026-10-26 (worlds 5, 8, 10 and 11 complete, D47-D54)._

## Resume here (exact state when last written)

2026-10-25 evening: the owner reverted the earlier campaign and restarted it on the restored
`develop` (`00ba088`) with a new brief; the active plan is `plans/GALAXY_EGGBERT_COMPLETION.md`
(read it first: direction, art fidelity, defects D1-D18, milestones). Not pushed (push only
when asked).

Done since the restore (details and hashes: the plan's §5 defects D1-D23 and §13): controls,
menus, title scene, editor slots, hint zones, world tooling (`worldkit.py`, `tools/worlds/kit.py`,
`levelcheck.py`, `VerifyShippedWorlds`), renderer (props, lifts, lighting, signs, door bars), Blupi
rebuilt (whole body, Bye, StopPop/Pop), hubs as the original's (signs, doors, gold bars, golden
key, arrivals), 3D crates (solid, push X/Z, pull), 3D dynamite, D13 (vehicles run over mines),
no underwater camera crash, and **the main hub plus all of world 1** (`world001`, `world010`,
missions 011-014; briefs in `plans/briefs/`, scripts in `tools/worlds/`).

2026-10-26: sprite tiles (doors, treasure doors, pillars, girders, bars, plate decor) render cut
out, cobwebs blended (`73c85a6`, D31, `TileCoverage.hpp`); Blupi's Win/Turn/Vertigo/Recede clips
installed (`3415d3f`). `qa_live.sh` takes `QA_LIVE_WORLDS=<dir>` for probe worlds. Later on
2026-10-26: swimming as the original's (D32), the helicopter's glue drop and no self-hit (D33),
Blupi's reactions JumpAie/Ouf* with clips, the camera over obstacles and after teleports
(D18/D34), flat-colour faces (D35), fan faces (D36). World 3 is done (030-034, background
session) and world 2 (020-025); worlds 4 and 5 are being authored by two background sessions.
World agents' reports name the engine problems they met; each was fixed with a test (D32-D37).
Later still: the wind pushes Blupi (D39), 347 is plain slime (D40); world 6 by this session:
hub 060, missions 061-066 (`b083d7b`, `5162e0e`, `955008a`, `237c771`, `5353607`, `74b3ded`):
world 6 is complete. World 4 is complete too (its agent, through 046). A switched-off saw
(379) now stands still and is harmless (D42, `0cf3ce2`); lifts keep their ticks and dwell (D43,
`3b0a861`; the pre-D43 worlds' lift meetings re-checked, 011 and 061 lengthened, `89b12c0`); follow stars ignore a hidden
Blupi (D44, `d115903`); a jump from under a bar catches it (D45, `7c49172`); the overcraft (46,
not a balloon: `worldkit` OVERCRAFT) hovers within 1.25 cells of the ground (D46, `4cdd48b`). Open: D41 (empty-mask decor is a floor to the
vertical test; worlds keep girders, poles, grates out of walking cells). `--spawn` yaw: 0 faces
-Z, 90 +X; the chase camera orbits to his side after 3 s idle, so F3 (direct view) shows the
facing in scripted shots.
World 8 by this session, complete: hub 080 and missions 081-084 (`6441258`, `7705ef6`,
`5b2eb86`, `ef92476`, `0414a90`; 084's chain of lifts to the golden key played live). World 5 is
complete (its agent: hub 050, missions 051-058, `8e06def`..`92c64a7`). Closed machine cubes hide the
faces between them (D47, `be9daab`: 083 crashed on load with a vertex buffer over the GPU's
limit; a mesh over that limit still throws instead of being split). Signs and flat plates stand
upright and read unmirrored from both sides (D48, `5fbc237`, cna-engine's UB-005 worked round).
The helicopter and the overcraft fly through the flight barrier 214 (D49, `d9980d9`; the
original hides 214 in play, a question for the owner). Of a row of crates Blupi pulls the one
touching him (D50, `26dbadd`). The lightning 305 shows only while it strikes (D51, `15bc7a9`).
Worlds 7 and 9 are being authored by two background sessions. World 10 by this session, complete:
hub 100 and missions 101-107 (`b8fb27e`, `f0a8333`, `ce2917b`, `c01c06a`, `a8636e8`, `035f254`,
`1f42f11`, `dde3c6a`). World 11 by this session, complete: hub 110 and missions 111-115 (`34745ee`,
`f017378`, `e17e780`, `f30ea6a`, `0e27a76` and 115's). A lift holds Blupi while a foot is on its
edge (D54, `fc230a3`: walking off at the cell's boundary he used to drop past the ledge). Blupi jumps off lifts (D52, `6ed7942`); crumbling bridges hold him as
the original's table does (D53, `7bb9415`); a terrain over a buffer's limit is split (D47's rest,
`7b96caf`); the editor's dead speed field is gone (`2a91ca6`). Then world 12 and 199.
Unconfirmed (world 2's agent): Action seemed ignored for ~6 s after a mission loads; not
reproduced (a helicopter mount at 4.5 s works).

Next: milestone D, worlds 2-12 and 199 — hub then missions per world, each from the original's
map, hints, objects and the video, authored with the kit, checked with `levelcheck.py` (a
`--solved` variant where crates or dynamite gate the route) and live. Expect 2D-era mechanics
like D21/D23 to surface; fix them in the engine with tests before relying on them.

Live checks: `tools/qa_live.sh build-cna-opengl3 <out> <steps> [--mission N --spawn x,y,z,yaw]`
runs from `build-probe/qa-run/` so the owner's `build-cna-opengl3/savedata.txt` and
`customworlds/` are never touched. The owner plays from `build-cna-opengl3/`.

## Active work: billboards → 3D models (`plan.md` Phase 7B, `BB3D-*`)

Approved 2026-10-24. Objects, big decor and the other Blupis get 3D models converted from the Ray
Dream scenes in `../speedyblupi-data/originals`; effects stay billboards. Background:
`assets-3d-analysis.md`.

- **State:** Phase 7B is complete apart from the open items. In game as 3D models
  (`avatars3d/objects/`, built from `tools/rd4_to_gltf/objects.txt`): every pickup but the charge and the shield, every
  enemy, the decoys (200-203), the tentacle and all big decor the original worlds use (palms,
  trees, grass, houses, machines, toys, pillars, plant, column, fountain, statue; BigDecor 203
  is the object channel's marine animation, now drawn as such). Effects, the charge and the
  shield stay billboards. Per-object results and open points: `plan.md` Phase 7B.
- **Converter conventions found today** (all in `tools/rd4_to_gltf/README.md` with evidence):
  cones; the flat surface is the unseen ground; mesh forms; Studio 5 `xyzS`; camera frame
  `ofst`; right-handed free-form sections (blupi.dxf confirms); continuous sweep frames;
  textures on cubes (4 x 3 cross), cylinders/spheres/cones (meridian fraction) and free-form
  domains; explo.png icon squares and per-icon sprite offsets; flips about the sprite's centre
  line; objects' model space upright in the scene.
- **Open (no blocker):** the fish's head is ~15 % large (`BB3D-032`, jaw analysis in the plan);
  the grass blades are wider than the sprites' (`BB3D-050`, 0.65-0.69); procedural textures
  (palm bark, metal noise, wood grain) are flat colours.
- **Waiting for the user:** (1) the charge (type 31) and the shield stay billboards because
  their sprites' sparks are effects; a model plus a sparks-only billboard derived from the
  sprites is possible but is a new derived asset set (`BB3D-025`, `BB3D-039`). (2) BigDecor
  models stand at the original's size (a 128/144-pixel icon = 2/2.25 blocks, as
  mobile-eggbert's Pixmap draws it 1:1), where the billboards were 1 block; say if the old size
  is wanted (`GalaxyEggbertGame.cpp`, the BigDecor loop of the object model pass).
- **Found and fixed on the way:** the opened door's slide showed the treasure's icon
  (`BB3D-042`); BigDecor icon 203 was drawn as a missing explo.png icon; parts of a mix shader
  with one colour were a red placeholder (now their colour: metal grey, the jeep's orange);
  non-ASCII part names crashed the load (UB-004).
- **Done on request (2026-10-24):** the chase camera leaves Blupi's back after 3 s standing
  still (`ChaseCameraYaw::kIdleDelaySeconds`, was 0.8 s).
- **Survey** (`build-probe/rd4-survey/scenes.csv`, after `BB3D-004`): all 528 scenes read; the
  only masters not built are `tx` (14, menus), `COFA` (2, terrain tiles), `fogg` (1, the cloud).

Regenerate the survey (about 25 s for scenes, much longer for icons):

```bash
python3 tools/rd4_to_gltf/survey.py scenes --originals ../speedyblupi-data/originals --out build-probe/rd4-survey
python3 tools/rd4_to_gltf/survey.py icons --originals ../speedyblupi-data/originals \
    --sheet ../mobile-eggbert/Content/icons/element.png --out build-probe/rd4-survey
```

## Build, test, run

Environment for every build (global rules in `../CLAUDE.md`): shared ccache, at most `-j2` in
this repository, only the existing build directories (`build-cna-opengl3/` is the native one in
use; `build-probe/` for converter output; never `/tmp` or the scratchpad).

```bash
export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv
cmake --build build-cna-opengl3 --target GalaxyEggbertCNA -j2
ctest --test-dir build-cna-opengl3 --output-on-failure -j2
cd build-cna-opengl3 && ./GalaxyEggbertCNA          # assets are relative to the build dir
cd build-cna-opengl3 && timeout 80 xvfb-run -a ./GalaxyEggbertCNA   # headless live run
```

Reconfigure only when the build is broken or the configuration changed:

```bash
cmake -S . -B build-cna-opengl3 -DGALAXY_EGGBERT_BUILD_CNA=ON -DCNA_GRAPHICS_RENDERER=OPENGL33 \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache
```

Golden frames and behavioural trace (need a display; not in default CTest):
`xvfb-run -a tools/verify_golden_frames.sh build-cna-opengl3`,
`xvfb-run -a tools/verify_golden_trace.sh build-cna-opengl3`. WebGL 2: `build-cna-webgl2/`
(Emscripten toolchain, `-DCNA_GRAPHICS_RENDERER=WEBGL2 -DBUILD_TESTING=OFF`).

## The real Blupi and the converter

- Converter and every verified Ray Dream convention: `tools/rd4_to_gltf/README.md`.
- Blupi's asset, contents and regeneration commands: `avatars3d/blupi/README.md`. The `.glb` files
  are intermediates in `build-probe/rd4-blupi/`; the game loads CNA `SkinnedModelEXT`
  directories.
- Python converters need `numpy` and `Pillow`; renders for checking go to `build-probe/` or the
  scratchpad (images only, never builds).

## Sibling repositories (last verified together)

| Repo | Verified commit | Date |
|---|---|---|
| `../../libcna/cna-engine` | `d86c62da` | 2026-10-20 |
| `~/deps/cna-baseline` (cna-engine's CNA compatibility baseline) | `900d0db3b` | 2026-10-20 |
| `../../libcna/easy-gl` | `f4d6afaf` (v0.1.1) | 2026-10-20 |
| `~/deps/sharp-runtime-baseline` | `f2aecfef` | 2026-10-20 |
| `../../libcna/meta-gl` | `dd3b3005` (v0.4.1) | 2026-10-20 |

`../mobile-eggbert` is test data (worlds read by two verifiers) and the behavioural reference;
read-only, never modified. Do not modify any sibling repository unless the user scopes it in; a
bug found in one goes to `upstream_bugs.md` (English, `UB-NNN`).

## Gotchas

- `tests/golden/` frames are stale since before Phase 7B (the whole frame differs; CURRENT.md
  says why); `--golden-capture` still works as a live check, its output just won't byte-match.

- `../../libcna/cna` is a live working tree another session edits: a build can break or crash
  mid-edit; rebuild after its header changes before suspecting this repository.
- VirtualBox VM `xp` (Ray Dream Studio) is free to use but must never get network access: keep its
  NIC unattached, never NAT or bridged.
- Commit each discrete task; push only when the owner asks (the standing push approval is
  suspended since 2026-10-25).
- Effects (explosions, splashes, sparkles, bursts, goo, the bridge being built) stay billboards
  (decision 2026-10-24).
- Types 32 and 33: the code's names ("blupih"/"blupit") do not match their icons; check against
  mobile-eggbert before converting (`BB3D-030`).
