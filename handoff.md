# handoff.md — where work stands and how to continue

Replaces `NEXT.md` (obsolete since 2026-10-24, kept frozen as history; do not add to it). Update
this file after every committed task: what was done, where it stopped, what to run next. Keep it
short; the backlog with checkboxes is `plan.md`, verified current state is `CURRENT.md`.

_Last updated: 2026-10-24 (evening)._

## Active work: billboards → 3D models (`plan.md` Phase 7B, `BB3D-*`)

Approved 2026-10-24. Objects, big decor and the other Blupis get 3D models converted from the Ray
Dream scenes in `../speedyblupi-data/originals`; effects stay billboards. Background:
`assets-3d-analysis.md`.

- **Done:** the plan (`plan.md` Phase 7B); `tools/rd4_to_gltf/survey.py` (the archive survey and
  the per-sheet icon matching, formerly throwaway scripts).
- **Done (Stage 1):** `BB3D-001` cones (`primitives.cone`); `BB3D-002` flat surfaces (the
  never-visible ground plane, built as nothing); `BB3D-003` mesh forms (`geometry.mesh_form`);
  camera frame offset `ofst` (`scene.camera_centre`, part of `BB3D-006`); `BB3D-005`
  (Studio 5 files without render settings read as still frames); `BB3D-004` (free-form and
  `im` variants; `COFA`/`fogg`/`tx` left on purpose). `survey.py icons` takes every sheet's grid.
- **Done (Stage 2-4):** `BB3D-010`/`011` the object model library and its game side
  (`avatars3d/objects/`, `ObjectModelTable`, `VerifyObjectModelTable`); in game as 3D:
  treasure, egg, exit, drink, ammo, projectile (`BB3D-020`-`023`), the creature's icons
  247-251 (`BB3D-038`, partly), the mines incl. the hanging one (`BB3D-034`). Checked live
  under Xvfb with `--golden-capture` (world 999: eggs incl. flipped icons, treasure, exit).
- **Next:** `BB3D-031` bird, `BB3D-032` fish, `BB3D-033` follow, `BB3D-036` wasp, the
  creature's eyes (`BB3D-038`); trial runs go to `build-probe/rd4-objects-try` with a scratch
  objects file, so only verified objects enter `tools/rd4_to_gltf/objects.txt` (first results
  in `plan.md`). `BB3D-007` icon surveys find unknown sources (element survey running in the
  background: `build-probe/rd4-survey/element_icons.csv`).
- **Pending check:** `BB3D-008` (Blupi's library with the camera offset): the matching re-run
  was started in the background; compare `build-probe/rd4-blupi/library/clips.md` with
  the copy saved before (`clips.md` table, overcraft rows) after rebuilding the library.
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
