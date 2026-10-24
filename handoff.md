# handoff.md — where work stands and how to continue

Replaces `NEXT.md` (obsolete since 2026-10-24, kept frozen as history; do not add to it). Update
this file after every committed task: what was done, where it stopped, what to run next. Keep it
short; the backlog with checkboxes is `plan.md`, verified current state is `CURRENT.md`.

_Last updated: 2026-10-24 (late evening)._

## Resume here (exact state when last written)

1. **Object library re-export pending.** Commit `cc8c391` changed the converter (free-forms
   drop rings/points their neighbours reproduce; meshes split at 65 535 vertices, which had
   stopped the export at the palms). A full rebuild of `build-probe/rd4-objects` was started
   (log `build-probe/rd4-objects/build.log`). `avatars3d/objects/` in the worktree is a
   *partial, failed* export from before the fix: do not commit it as is. When the rebuild has
   finished (every object in `tools/rd4_to_gltf/objects.txt` has a line `... icons, IoU ...` in
   the log; if the process is gone and the log is incomplete, re-run the command from
   `avatars3d/objects/README.md`), run the export there, then build, `ctest`, commit
   `avatars3d/objects` together with the big decor/grass entries of `objects.txt`, push.
   Compare IoU with `report.md` rows (palms 0.755-0.784, grass 0.65-0.69, others 0.87-1.0).
2. **Blupi (`BB3D-008`):** a library built before `cc8c391` (exit 0) matched as well as or
   better than every earlier one (jeep 0.949 -> 0.970, tank 0.893 -> 0.929, MarchOver 0.739 ->
   0.940, Clear4 0.843 -> 0.983; its `clips.md`/`clips.txt` are kept in the session scratchpad
   only). It is being rebuilt once more with `cc8c391` so the vehicles shrink 2-3 x (jeep
   100 122 triangles); log `build-probe/rd4-blupi/library/build.log`, `clips.md` is written
   last. Next: check `clips.md` against the numbers above, export with
   `avatars3d/blupi/README.md`'s commands, build, test, commit. If the rebuild was lost,
   re-run the `build_clip_library.py` command from that README (about an hour).

## Active work: billboards → 3D models (`plan.md` Phase 7B, `BB3D-*`)

Approved 2026-10-24. Objects, big decor and the other Blupis get 3D models converted from the Ray
Dream scenes in `../speedyblupi-data/originals`; effects stay billboards. Background:
`assets-3d-analysis.md`.

- **State:** Phase 7B is nearly complete. In game as 3D models (`avatars3d/objects/`, built
  from `tools/rd4_to_gltf/objects.txt`): every pickup but the charge and the shield, every
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
- **In progress `BB3D-008`:** Blupi's clip library rebuilt with the current converter (the
  matching re-run gave 145 confident / 61 probable / 25 unmatched icons, was 121 / 76 / 34; a
  library with `ofst` and `xyzS` gave StopOver 0.764 -> 0.983, MarchOver 0.739 -> 0.940,
  Clear4 0.843 -> 0.982). When `build-probe/rd4-blupi/library/blupi.glb` is rebuilt: compare
  `clips.md` with the scratchpad copies, check his face, export with the commands in
  `avatars3d/blupi/README.md`, build, test, commit. If interrupted: re-run those commands.
- **Open (no blocker):** the fish's head is ~15 % large (`BB3D-032`); the grass blades are wider
  than the sprites' (`BB3D-050`); procedural textures (metal noise, wood grain) are flat
  colours.
- **Waiting for the user:** (1) the charge (type 31) and the shield stay billboards because
  their sprites' sparks are effects; a model plus a sparks-only billboard derived from the
  sprites is possible but is a new derived asset set (`BB3D-025`, `BB3D-039`). (2) BigDecor
  models stand at the original's size (a 128/144-pixel icon = 2/2.25 blocks, as
  mobile-eggbert's Pixmap draws it 1:1), where the billboards were 1 block; say if the old size
  is wanted (`GalaxyEggbertGame.cpp`, the BigDecor loop of the object model pass).
- **Not checked live:** BigDecor models (no generated world has BigDecor; only editor worlds
  do). The draw path is shared with the objects, which were checked live under Xvfb.
- **Found and fixed on the way:** the opened door's slide showed the treasure's icon
  (`BB3D-042`); BigDecor icon 203 was drawn as a missing explo.png icon.
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
- Commit each discrete task and push to `origin/develop` right after (standing approval).
- Effects (explosions, splashes, sparkles, bursts, goo, the bridge being built) stay billboards
  (decision 2026-10-24).
- Types 32 and 33: the code's names ("blupih"/"blupit") do not match their icons; check against
  mobile-eggbert before converting (`BB3D-030`).
