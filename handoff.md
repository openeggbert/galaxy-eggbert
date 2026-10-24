# handoff.md — where work stands and how to continue

Replaces `NEXT.md` (obsolete since 2026-10-24, kept frozen as history; do not add to it). Update
this file after every committed task: what was done, where it stopped, what to run next. Keep it
short; the backlog with checkboxes is `plan.md`, verified current state is `CURRENT.md`.

_Last updated: 2026-10-24._

## Active work: billboards → 3D models (`plan.md` Phase 7B, `BB3D-*`)

Approved 2026-10-24. Objects, big decor and the other Blupis get 3D models converted from the Ray
Dream scenes in `../speedyblupi-data/originals`; effects stay billboards. Background:
`assets-3d-analysis.md`.

- **Done:** the plan (`plan.md` Phase 7B); `tools/rd4_to_gltf/survey.py` (the archive survey and
  the per-sheet icon matching, formerly throwaway scripts).
- **Done (Stage 1):** `BB3D-001` cones (`primitives.cone`); `BB3D-002` flat surfaces (the
  never-visible ground plane, built as nothing).
- **Next:** `BB3D-003` (`vmda` mesh-form masters).
- **Survey baseline** (`build-probe/rd4-survey/scenes.csv`, 2026-10-24): 500 of 528 scenes read;
  masters not built: `COfl` 87 scenes, `vmda` 83, `ex` IndexError 18, `co` 15, `tx` 14,
  `COFA` 2, `im` ValueError 1, `fogg` 1; 28 RDS 5 files fail with AttributeError
  (`Decor/goutte` g1-7, `Decor/palais` a, `Decor/smooth` a/a1/a2, `Decor/tentacule` p01-p17).

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

- `../../libcna/cna` is a live working tree another session edits: a build can break or crash
  mid-edit; rebuild after its header changes before suspecting this repository.
- VirtualBox VM `xp` (Ray Dream Studio) is free to use but must never get network access: keep its
  NIC unattached, never NAT or bridged.
- Commit each discrete task and push to `origin/develop` right after (standing approval).
- Effects (explosions, splashes, sparkles, bursts, goo, the bridge being built) stay billboards
  (decision 2026-10-24).
- Types 32 and 33: the code's names ("blupih"/"blupit") do not match their icons; check against
  mobile-eggbert before converting (`BB3D-030`).
