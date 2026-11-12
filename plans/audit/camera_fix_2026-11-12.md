# Chase camera fix — review

Measured on the same routes as [the original audit](camera_quality_2026-11-12.md). No goldens were re-baselined and no commit was made.

## Before / after

Each cell is before → after. Longest occlusion is consecutive 60 Hz ticks. Definitions and static-terrain exclusions are unchanged from the original audit; hidden means all seven body landmarks blocked. A jump requires eye displacement >0.5 blocks and >4 × player displacement +0.02; it includes intentional shake. Occlusion/penetration use static opaque cube terrain inset by 0.01 block.

| Mission | Samples | Jumps | Occluded | Longest occlusion (ticks) | Hidden | Eye inside |
|---|---:|---:|---:|---:|---:|---:|
| 001 hub | 2,530 | 0 → 0 | 46 → 0 | 9 → 0 | 14 → 0 | 0 → 0 |
| 011 | 5,929 | 3 → 0 | 33 → 0 | 15 → 0 | 13 → 0 | 0 → 0 |
| 025 | 13,557 | 35 → 20 | 46 → 0 | 17 → 0 | 25 → 0 | 10 → 0 |
| 045 | 16,692 | 18 → 9 | 73 → 0 | 32 → 0 | 45 → 0 | 17 → 0 |
| 065 | 22,919 | 47 → 18 | 220 → 0 | 16 → 0 | 155 → 0 | 83 → 0 |
| 082 | 12,819 | 9 → 1 | 0 → 0 | 0 → 0 | 0 → 0 | 0 → 0 |
| 094 | 17,675 | 9 → 4 | 38 → 0 | 9 → 0 | 16 → 0 | 0 → 0 |
| 105 | 10,799 | 1 → 0 | 2 → 0 | 2 → 0 | 2 → 0 | 2 → 0 |
| 113 | 7,923 | 9 → 3 | 22 → 0 | 9 → 0 | 13 → 0 | 0 → 0 |

Totals: jumps: 131 → 55; occluded: 480 → 0; body_hidden: 283 → 0; inside: 112 → 0.

Largest jump candidate: 4.0040 → 1.7887 blocks. The new largest is world 025, tick 5144. Player discontinuities, world changes and startup exclusions are unchanged. Raw jump counts include intentional shake.

## Verification

- All eight standard `tools/autoplay.sh` replays passed. Each complete `autoplay_trace.txt` is byte-identical to its pre-fix replay (hashes in `trace-comparison.json`).
- Every-tick simulation states in the probe, including the terrace/idle hub route, are identical to the original audit. Indexed camera samples agree with their native drawn traces to <0.0001 blocks; the first unindexed startup draw in each combined capture is explicitly excluded from draw alignment.
- The build used only `build-cna-opengl3`, six compile jobs and the shared ccache. See `build.log`. CTest: **131/131 passed**, see `ctest.log`.
- All captures were silent with at most two game instances. Exact failure frames use ordinary `shots` commands; the GDB observer retains only selected PNG exports. The main routes observe every camera tick, draw every 30 ticks and additionally rasterize the selected failure windows.

Camera measurement binary SHA-256: `29e508b13636817769032937f133e955e1fdde3d5e47c527c9dc171c8e708644`. Final tested binary: `6463b07c8763df8eaee032621004402b3102761c41ea076e12fa8acd6fa845d9`. The main measurements use the first binary; the supplemental hub screenshots and full CTest use the final binary. The only subsequent change is demo snapshot serialization: the existing camera fingerprint now records the preserved simulation projection that controls reward timing. No expected recording was rewritten. The initial full CTest run passed 130/131, with five demo camera fingerprints differing; after this serialization correction all 131 tests pass. See `ctest-before-snapshot-fix.log` and `ctest.log`.

Build command: `export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv; cmake --build build-cna-opengl3 --parallel 6`. Tests: `ctest --test-dir build-cna-opengl3 -j4 --output-on-failure`. Focused verification additionally covers exact voxel intersections (including grazing rays and 2,000 randomized cases), preserving reward timing when HUD draw endpoints change, and the probe's geometry/alignment self-tests.

## Golden images and trace

Baseline: all nine approved PNGs and the approved trace matched before the fix. Below are RGB pixel differences against those references; review copies are in `golden/effects-*/before` and `after`.

| Effects | Frame | Changed pixels | RGB mean absolute error (0–255) | Maximum channel difference |
|---|---|---:|---:|---:|
| 0 | 0060 | 0 / 384,000 | 0.000000 | 0 |
| 0 | 0120 | 0 / 384,000 | 0.000000 | 0 |
| 0 | 0180 | 0 / 384,000 | 0.000000 | 0 |
| medium | 0060 | 0 / 384,000 | 0.000000 | 0 |
| medium | 0120 | 0 / 384,000 | 0.000000 | 0 |
| medium | 0180 | 0 / 384,000 | 0.000000 | 0 |
| high | 0060 | 0 / 384,000 | 0.000000 | 0 |
| high | 0120 | 0 / 384,000 | 0.000000 | 0 |
| high | 0180 | 0 / 384,000 | 0.000000 | 0 |

Golden trace: byte-identical to the approved reference.

## Exact former worst cases

[Before/after contact sheet](comparison.png)

After images cover the ten largest former jumps, and start/middle/end frames of the ten longest former occlusion and penetration runs. The selected-frame inventory is `evidence-ticks.json`. Before images where retained by the original audit are copied into `evidence-before/`. The complete set of new PNGs is in `run-NNN/autoplay_shots/` (the hub in `run-001-hub`).

| Former failure | World | Tick | After screenshot |
|---|---|---:|---|
| Jump | 045 | 16536 | [PNG](run-045/autoplay_shots/0016536.png) |
| Jump | 105 | 5256 | [PNG](run-105/autoplay_shots/0005256.png) |
| Jump | 045 | 14407 | [PNG](run-045/autoplay_shots/0014407.png) |
| Jump | 025 | 3573 | [PNG](run-025/autoplay_shots/0003573.png) |
| Jump | 065 | 19251 | [PNG](run-065/autoplay_shots/0019251.png) |
| Jump | 065 | 18893 | [PNG](run-065/autoplay_shots/0018893.png) |
| Jump | 065 | 7648 | [PNG](run-065/autoplay_shots/0007648.png) |
| Jump | 065 | 11633 | [PNG](run-065/autoplay_shots/0011633.png) |
| Jump | 065 | 7217 | [PNG](run-065/autoplay_shots/0007217.png) |
| Jump | 113 | 5674 | [PNG](run-113/autoplay_shots/0005674.png) |
| Occlusion | 045 | 3173 | [PNG](run-045/autoplay_shots/0003173.png) |
| Occlusion | 025 | 9092 | [PNG](run-025/autoplay_shots/0009092.png) |
| Occlusion | 065 | 11770 | [PNG](run-065/autoplay_shots/0011770.png) |
| Occlusion | 011 | 291 | [PNG](run-011/autoplay_shots/0000291.png) |
| Occlusion | 065 | 15033 | [PNG](run-065/autoplay_shots/0015033.png) |
| Occlusion | 065 | 4365 | [PNG](run-065/autoplay_shots/0004365.png) |
| Occlusion | 045 | 3885 | [PNG](run-045/autoplay_shots/0003885.png) |
| Occlusion | 065 | 14926 | [PNG](run-065/autoplay_shots/0014926.png) |
| Occlusion | 065 | 11890 | [PNG](run-065/autoplay_shots/0011890.png) |
| Occlusion | 045 | 1151 | [PNG](run-045/autoplay_shots/0001151.png) |
| Eye inside | 065 | 14926 | [PNG](run-065/autoplay_shots/0014926.png) |
| Eye inside | 065 | 14912 | [PNG](run-065/autoplay_shots/0014912.png) |
| Eye inside | 045 | 11069 | [PNG](run-045/autoplay_shots/0011069.png) |
| Eye inside | 065 | 18896 | [PNG](run-065/autoplay_shots/0018896.png) |
| Eye inside | 045 | 16544 | [PNG](run-045/autoplay_shots/0016544.png) |
| Eye inside | 065 | 7651 | [PNG](run-065/autoplay_shots/0007651.png) |
| Eye inside | 065 | 14875 | [PNG](run-065/autoplay_shots/0014875.png) |
| Eye inside | 065 | 19254 | [PNG](run-065/autoplay_shots/0019254.png) |
| Eye inside | 045 | 14412 | [PNG](run-045/autoplay_shots/0014412.png) |
| Eye inside | 025 | 3603 | [PNG](run-025/autoplay_shots/0003603.png) |
| Hub occlusion | 001 | 2289 | [PNG](run-001-hub/autoplay_shots/0002289.png) |
| Hub occlusion | 001 | 2293 | [PNG](run-001-hub-evidence/autoplay_shots/0002293.png) |
| Hub occlusion | 001 | 2297 | [PNG](run-001-hub-evidence/autoplay_shots/0002297.png) |

Two hub frames (2293 and 2297) were not individually drawn by the realtime main capture. `run-001-hub-evidence` repeats the same movement script without realtime pacing and saves those exact frames. Its every-tick player states and camera poses match the main hub capture exactly, and the native draw trace independently matches the observed poses; see `hub-evidence-validation.json`.


## What changed

The camera uses the existing exact voxel traversal, with the same block registry predicate. Its safety anchor is Blupi's current body, independent of the damped look target. A 0.3-second motion forecast constrains the desired and lagging arms before damping, with a bounded fast pull-in. At a newly grazed corner it first tries a nearby clear rise or sideways view; otherwise the current obstruction imposes an immediate safety limit. Shake is translated as before in open space and scaled only when its requested offset would obstruct the body ray. Interpolated draws check the interpolated body as well.

The existing ChaseCameraYaw, desired distance/elevation, rise search, auto-zoom and shake tables are unchanged. Pickup reward timing and farewell facing retain the pre-fix projection in a separate, non-rendered calculation. HUD flights use the visible projection only for their drawn endpoints. Demo snapshots retain the simulation projection as their camera fingerprint, so camera improvements do not alter established deterministic recordings. This preserves the old simulation clock and replay states without putting controller, camera or graphics dependencies into InteractionSystem. No gameplay rules changed.

## Scope and limitations

The zero counts refer to the audit's static opaque cube terrain on these routes. Dynamic doors, modeled pieces and the near-plane-only silhouette are not proven by the on-disk cube proxy. Exact screenshots provide a visual check of the selected worst cases. In particular, 045/3173 now shows Blupi above the formerly occluding ledge; 045/16544 is outside solid terrain but foreground modeled mines still fill much of the tight view. Zero static-cube proxy counts do not mean every animated silhouette is unobstructed. The final aim is also clipped toward the body when the damped look point would aim into a wall. The >60 fps interpolation guard is implemented but is not exercised by the 60 Hz autoplay probe. Raw shake offsets in the observer are requested offsets; subtracting them from the constrained eye is not an exact unshaken pose when shake was clipped. Earlier tuning attempts are summarized in `iteration-1` and `iteration-2`; `iteration-3` records the camera-only change that exposed the old reward-timing dependency. `iteration-4/every-tick-validation.json` proves that the final aim correction left every eye position and simulation state unchanged. Redundant intermediate captures and executable hard links were removed.

## Changed files

- `CURRENT.md`
- `src/GalaxyEggbert/Game/ChaseCameraYaw.hpp`
- `src/GalaxyEggbert/Game/InteractionSystem.hpp`
- `src/GalaxyEggbertCNA/GalaxyEggbertGame.cpp`
- `src/GalaxyEggbertCNA/GalaxyEggbertGame.hpp`
- `src/GalaxyEggbertCNA/GalaxyEggbertGameDemo.cpp`
- `src/GalaxyEggbertCNA/GalaxyEggbertGameDraw.cpp`
- `src/GalaxyEggbertCNA/GalaxyEggbertGameMissions.cpp`
- `src/GalaxyEggbertCNA/GalaxyEggbertGameNet.cpp`
- `src/GalaxyEggbertCNA/GalaxyEggbertGameUpdate.cpp`
- `src/GalaxyEggbertCNA/GameHelpers.hpp`
- `tools/VerifyChaseCameraYaw.cpp`
- `tools/VerifyInteractionSystem.cpp`
- `tools/camera_probe.py`
- `tools/verify_golden_frames.sh`
