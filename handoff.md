# handoff.md — where work stands and how to continue

Replaces `NEXT.md` (obsolete since 2026-10-24, kept frozen as history; do not add to it). Update
this file after every committed task: what was done, where it stopped, what to run next. Keep it
short; the backlog with checkboxes is `plan.md`, verified current state is `CURRENT.md`.

_Last updated: 2026-10-27 (the campaign complete: every world authored; D48 fixed in cna-engine; the saw
death's half body wound consistently)._

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
not a balloon: `worldkit` OVERCRAFT) hovers within 1.25 cells of the ground (D46, `4cdd48b`). Empty-mask decor is
passable both ways, as the original's (D41, done 2026-10-27; only 024 stood on it, rebuilt). `--spawn` yaw: 0 faces
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
`7b96caf`); the editor's dead speed field is gone (`2a91ca6`). World 12 by this session, complete: hub 120 and missions 121-125 (`52b36e9`, `d8240bb`, `85b71cb`,
`f4276da`, `dce9f47` and 125's). World 199, the grand tour, by this session (its red arrow shows the win screen live). World 9 is complete (its agent: hub 090, missions 091-095, `0813bec`..`1684214`). The weak jump
out of the water clears only a bank level with the water's top cell (the guide said one above):
`tools/watercheck.py` finds water without a way out; 045, 057 and 084 had such pools, fixed
(`53f7de1`, `7450970`). World 7 is complete (its agent: hub 070, missions 071-075, `75999ce`..`81a1f97`). The campaign is
complete: `tools/campaigncheck.py` (the main hub, 12 hubs, 64 missions and 199) and
`tools/watercheck.py` pass. D48 is now fixed in cna-engine (`2a93b7e`, owner's request: no
cna-engine workarounds in Galaxy), Galaxy's interim plate code removed (`ae7b0a8`). The flight
barrier 214 is drawn only by the ShowSecret cheat and in the editor (owner, `aa50281`).
Next: milestone F, the completion audit (playthroughs of the other missions); D11 waits for the owner's list of models that look wrong;
D41 is done: decor is neither a floor nor a ceiling (only 024 needed a change).
The saw death's half body no longer shows through its cut (`0cff927`: the converter winds Ray Dream's
boolean masters consistently, `geometry.orient_consistently`; only its two parts re-exported, every
other clip pixel-identical; checked live in a probe of 065).
Mission 011 is played through in one run (`81b51f0`): `GalaxyEggbertCNA --mission N --autoplay
<script>` plays by a script (`Autoplay.hpp` documents its commands; `VerifyAutoplay`), and
`tools/autoplay.sh build-cna-opengl3 tools/autoplay/world011.txt 11` runs it under Xvfb in 21 s
with a fresh save, printing the trace; exit 0 = passed. Write a mission's script a section at a
time, rerunning; `trace N` logs every tick. A lift holds Blupi while his feet strip touches it,
as the original's (`MoveObjectStepLine`): on a lift with a 3-tick dwell ride at its front edge.
Later on 2026-10-27: playthroughs of worlds 1, 2 and 12 (12 also as a chain from hub 120 through
all five missions to the main hub: `tools/autoplay/world120.txt`, a hub's script is its world's
chain) and the first of worlds 3-5 (background agents: worlds 3-4, 5-6). `tools/autoplay_all.sh
build-cna-opengl3` replays them all (run it after any gameplay change). Autoplay commands added on
the way: crate, clear/near (time patrols), `with KEY` (swim, dive), `tol 0 ... until`, include, the
fields exit and blitz; the run draws one frame in 30. Found and fixed: D55 (penetration recovery
tested his feet and sank him through floors, `1b8c144`), and world defects in 013, 025, 121, 122,
123 (crates cannot stack on a flat floor: one crate's climb), 124 (wind at the feet's cell).
`levelcheck.py` swims and reports traps (candidates still open in 052, 054, 062).
Unconfirmed (world 2's agent): Action seemed ignored for ~6 s after a mission loads; not
reproduced (a helicopter mount at 4.5 s works).
Then: every mission of worlds 1-4 and 7-12 and each of those worlds as one chain from its hub
(`worldN0.txt`: the hub's signs in order, each mission's script included, back past the bar it
opened, the golden key to the main hub), world 199 to the win screen (each key spent on one
door: machine, library, cheese, the finale's outer door, village, inner door). Found and fixed:
D56 (a lift carried him sideways off a ledge), D57 (a lift going down pulled him through a ledge,
107), D58 (a key opened one cell of a door, 107), 082's arrow-room shuttle (its ends in the
floor's cells: levelcheck now reports a level lift with an end inside a block), 199's crushers
(floating two above the floor, now in it). Autoplay fields: `temp` (saucers), `crush`
(crushers), `mtick` (ticks since the mission started; a mission timed by the script's `tick`
lost its timing in a chain, 032-034 moved to it). Eggs stop at ten (the original's
MAX_EGG_COUNT): a script expects at most ten. Worlds 5-6 (agent C: 051-058, 061-066, the fix of
062's first pool, `dc0b6e1`) and their chains 050/060 followed; 094's mine rows now bar a corridor
to T2 (in the open room they could be walked round). The whole campaign plays in one run:
`tools/autoplay/campaign.txt` (from `tools/autoplay_campaign.py`, which climbs the main hub's
tiers at x 50 between the signs; the summit's tunnel along z 44 past the gold bars to 199):
`tools/autoplay.sh build-cna-opengl3 tools/autoplay/campaign.txt 1`, 755171 ticks, a couple of
hours of wall time; it includes every mission's script, so it is the full replay. 065 and 066
are then played in every part (two agents: 065's tower, crates, lake, overcraft and jeep; 066's
middle and upper storeys, which found a pit under its stair to B1, fixed `c2e4861`), which found
D59: walking past sticks of dynamite took two, any stick on contact; now, as the original, one
stick per press of Action while none is carried or flying (`c4a539f`; the seven scripts that
take sticks press Action). levelcheck reports traps in 052 and 054 that the wasp's balloon
leaves; 066's blupit can be walked round (B1 wider than its line of fire, left). Open: D11.
Milestone F is done (the suite 110/110; `tools/qa_sweep.sh` loads all 79 worlds live, 79/79;
campaigncheck, watercheck; the campaign in one run). Milestone C: every turning object (bird,
fish, follow star, egg, exit arrow, bulldozer) is now matched to its icons every 5 degrees
(`tools/rd4_to_gltf/objects.txt`; only the icon-to-pose tables change, the meshes stay), and the
parity matrix is `plans/inventory/model_parity.md` (`tools/model_parity.py`): 75 of 82 models
match every icon at 0.85 or better, the 7 below are faithful by their compare sheets. To re-export
one object: `build_object_library.py ... --only NAME --out build-probe/rd4-objects-probe`, then
`export_cna_avatar.py` into `avatars3d/objects/<dir>/` and copy its `icons.txt` (the full
`export_objects.py` would remove every directory its library lacks). Objects are toned to their
sprites (owner's choice): `tone=A,B` in `objects.txt` (37 objects), fitted by
`tools/rd4_to_gltf/calibrate_tone.py <library> --objects tools/rd4_to_gltf/objects.txt` from a
build's compare sheets; colour error 37.6 -> 14.0 on those models. A new object: build it
without a tone, run the calibration, add the suggested `tone=`, build again and compare. Open:
D11 (the owner's list), BB3D-032 (the fish's head), the flat procedural textures (a bake was
tried and did not help), the vehicles' tone (Blupi's clip library, not toned).

Milestone D (every world) is done; a change to a world is checked with `levelcheck.py` (a `--solved`
variant where crates or dynamite gate the route), `campaigncheck.py`, `watercheck.py` and live.
Mechanics a world needs that misbehave are fixed in the engine with a test first (D21-D54).

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
