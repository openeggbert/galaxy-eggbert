# Chase-camera quality measurement

Measured 2026-11-12 on branch `localization`, source commit `044321a92a2a92d1bc5e130c8e98ecaca9fc44c0`.
Executable SHA-256: `588f4e974c9c2391434c74529f2a7bf41c7d0e409dba741c751bcf0c840fa491`.
No game code, assets, sibling repositories, or existing play-pass outputs were changed. No build or commit was made.

<!-- FINDINGS -->
**The chase camera still has measurable failures.** Across the eight complete mission routes and the longer hub route, **110,843 eligible 60 Hz camera samples** contain **131 jump events**, **480 static-terrain occlusion samples (0.433%)**, and **112 eye-inside-solid samples**. The seven-point hidden proxy fires on 283 samples. The worst jump is **4.004 blocks** while Blupi moves **0.060 blocks** (world 045, tick 16536). The longest static-terrain occlusion is **32 ticks (0.533 s)**; the longest penetration is **12 ticks (0.200 s)**. These are short but severe failures, corroborated by exact-frame screenshots. The strongest mechanisms are the final undamped wall correction, a smoothed target entering terrain, and shake applied after collision checking.

Every standard mission script and both hub routes passed. No deaths/respawns occurred. World changes and the two observed teleports are excluded from jump counts. **Coverage distinction:** every-tick camera poses were measured everywhere; every-tick rasterization was performed for 011, the short hub route, and selected failure windows. The other full routes use the independently validated read-only observer.

**Intentional shake matters:** 19 of the jump candidates fall below the same threshold after subtracting shake; 102 still exceed it, and 10 lack complete shake coverage. Thus 131 raw candidates should not be called 131 unexplained camera defects. All penetration samples have shake coverage: 67 disappear when shake is subtracted, while 45 persist. See `shake-analysis.json`.
<!-- /FINDINGS -->

## Method and interpretation

**These are real scripted game playthroughs, with the chase camera active.** The eight selected missions are 011 (outdoor platforms/lifts), 025 (keep, towers, vehicle corridor), 045 (cheese ledges, shafts and lifts), 065 (dense cave, crates, vehicles and teleporters), 082 (lifts and narrow corridors), 094 (caves, stairs, pools and pockets), 105 (helicopter cavern and low passage), and 113 (industrial gallery and platforms). The separate 001 portal route traverses the main hub and hub 010 into mission 011. A second 001 route climbs all six two-block terraces, waits through idle orbits facing four directions, and descends; it uses normal controls and no portals or cheats. Each standard script starts with a fresh private save. No cheats or altered movement commands were introduced.

### Sampling and trace alignment

A significant instrumentation limitation was found in this checkout:

* `AUTOPLAY_REALTIME=1` changes pacing, **not** autoplay's draw suppression: `src/GalaxyEggbertCNA/GalaxyEggbertGameUpdate.cpp:250-258` still suppresses 29 of 30 draws unless a shot is pending.
* The interpolation trace's `step` is always zero in autoplay. It increments only in the interpolation branch (`GalaxyEggbertGameUpdate.cpp:131-145`); autoplay disables interpolation (`GalaxyEggbertGame.cpp:452-456`). Treating it as an autoplay tick, or treating normal trace lines as consecutive 60 Hz frames, would produce invalid results.
* Mission 011 and the hub route were rendered **every tick**, using existing `AUTOPLAY_SHOTS=1`. A GDB hook records each screenshot filename's tick and the byte offset of the already-flushed `interp.txt`. It bypasses only unselected diagnostic `Texture2D::SaveAsPng` exports; the game still calculates the camera, renders the scene, and reads the backbuffer. Periodic/selected PNGs are exported normally. The first frame has no screenshot index and is excluded.
* For the seven longer missions, a **read-only** GDB breakpoint records the eye and look target passed by `UpdateTickCamera` to `Camera::SetLookAt` at every simulation tick. A tick without a camera update holds the last eye. These are **60 Hz camera-update samples**, not a claim that every sample was rasterized. The ordinary draw trace, one frame per 30 ticks, independently checks the sampled eyes. Exact failure windows are subsequently rendered every tick with existing `shots N` commands.
* This sampling optimization was validated by replaying 011 and the hub route: **6,550 aligned frames** agree with the every-frame draw capture to within **0.00008377 blocks** (the draw trace rounds to four decimals), with exactly matching player coordinates and world IDs. See `observer-validation.json`. Evidence replays are checked against their original trajectories as well.
* `StepAutoplay` logs the state **before** update t (`GalaxyEggbertGameMissions.cpp:95-102`), whereas screenshot filename t and the observed camera pose describe the state after update t−1. Thus the report's tick t joins screenshot t / camera tick t with autoplay position t, not t−1. Only `UpdateTickCamera` calls are observed: `ResetMissionInput` runs before `StepAutoplay` at a world start and must not be assigned to the preceding drawn frame.
* Mission runs are unthrottled for capture efficiency; the longer hub route uses `AUTOPLAY_REALTIME=1`. In both modes, the simulation and camera use the game's fixed **1/60-second dt** (`GalaxyEggbertGameUpdate.cpp:317`). Durations below are game time, not capture wall time. They do not measure desktop frame pacing or performance. Interpolation above 60 Hz is outside this test.

All processes were silent (`SDL_AUDIODRIVER=dummy`), on private Xvfb displays, at the ordinary 800×480 backbuffer and default effects. At most two instances belonging to this probe ran concurrently. Existing play-pass jobs were left alone. `LP_NUM_THREADS=2` limits software-renderer workers in the later runs. An abandoned slow pilot is retained separately and is not counted.

### Coordinates, terrain and metrics

All positions below are **raw world/grid coordinates** `(X,Y,Z)`, in blocks; X/Z from the render-eye trace have 50 added (`WorldRuntime.hpp:90-91`). A terrain voxel centered at integer `(x,y,z)` occupies center ±0.5 on each axis. Player positions are logged to 0.01 blocks; eyes to 0.0001 in the draw trace and full float precision in the observer.

The analyzer reuses `tools/vwr_inspect.py` for `.vwr` decoding and `tools/data/blocks.csv`, verified against the existing `DumpBlockDefinitions` executable. The camera's own solid predicate is non-air, non-water, nonzero collision mask (`GameHelpers.hpp:596-610`). Primary geometric counts are restricted further to **static opaque cube terrain**, excluding modeled/non-cubic pieces, animated terrain, teleporters, springs, opening door families, crumbling bridges, and ragged brick edges. Their runtime shape/state cannot be recovered accurately from the on-disk world alone. In particular, counting an already-open door from the original `.vwr` as a wall would manufacture failures. The report also provides unfiltered camera-mask candidate counts, explicitly not confirmed visual failures.

* **Jump:** consecutive same-world camera poses with `eye_delta > 0.5` and `eye_delta > 4 × Blupi_delta + 0.02` blocks. Half a block is one eighth of the ordinary four-block arm in a single 60 Hz frame (30 blocks/s); the ratio separates camera-only motion from normal falls/vehicle motion, and 0.02 guards against rounded player coordinates. This flags abrupt motion, not automatically a defect: intentional screen shake can contribute. Exclusions are world changes, the first two ticks of a mission/world start, nonconsecutive samples, and player discontinuities over two blocks (the recorded teleports). All standard scripts fail on death, and all passed; no respawn is concealed by a passed run. Excluded discontinuities are retained in `analysis.json`.
* **Occluded:** an exact segment from the eye to the logged Blupi position intersects static cube terrain. A 3D DDA enumerates candidate voxels; slab intersection tests the actual segment rather than sampling every 0.1 block. Each cube is inset by 0.01 block to reject grazing/rounding ambiguities. The longest run counts consecutive eligible ticks, resetting at a gap/world/run boundary. Eye-in-solid cases also count as occluded; the columns overlap.
* **Seven-point hidden proxy:** the above point plus points at Y−0.35 and Y+0.25, and four lateral points ±0.25 in X/Z, are all blocked. These cover body/feet/head landmarks consistent with the collision dimensions (`BlupiController.hpp:83-93`, rendered feet at Y−0.5 in `GalaxyEggbertGame.cpp:401,448`). This is a stronger visibility warning, **not** a pixel-perfect proof that the entire animated silhouette is invisible. Vehicle poses, particles, relief geometry, frustum cropping, and model occluders are not reproduced by the voxel test. Selected screenshots supply visual corroboration.
* **Eye inside solid:** the eye is more than 0.01 block inside all six faces of a static solid cube. This is an actual coordinate penetration, not a near-plane-only intersection. Near-plane clipping can still occur without this test firing.

These results quantify the sampled routes and establish failures; they do not establish an exhaustive campaign-wide failure rate. The hub measurement covers the terraces and idle orbit, not every sign niche or the locked gold-bar tunnel. The independent ray checks passed six geometric fixtures, 500 randomized comparisons against exhaustive cell enumeration, and run-boundary checks (`self-test.txt`).

<!-- RESULTS -->
## Numbers by mission

Counts below exclude the first two start/load ticks. `Hidden` means all seven body landmarks blocked. Durations assume the fixed 60 Hz game clock. Occlusion includes eye-inside samples.

| World / route | Samples | Jumps | Occluded | Longest occlusion (ticks / seconds) | Hidden | Eye inside | Longest inside (ticks) |
|---|---:|---:|---:|---:|---:|---:|---:|
| 001 terrace/idle hub route | 2,530 | 0 | 46 | 9 / 0.150 | 14 | 0 | 0 |
| 011 | 5,929 | 3 | 33 | 15 / 0.250 | 13 | 0 | 0 |
| 025 | 13,557 | 35 | 46 | 17 / 0.283 | 25 | 10 | 3 |
| 045 | 16,692 | 18 | 73 | 32 / 0.533 | 45 | 17 | 7 |
| 065 | 22,919 | 47 | 220 | 16 / 0.267 | 155 | 83 | 12 |
| 082 | 12,819 | 9 | 0 | 0 / 0.000 | 0 | 0 | 0 |
| 094 | 17,675 | 9 | 38 | 9 / 0.150 | 16 | 0 | 0 |
| 105 | 10,799 | 1 | 2 | 2 / 0.033 | 2 | 2 | 2 |
| 113 | 7,923 | 9 | 22 | 9 / 0.150 | 13 | 0 | 0 |

Primary total: 110,843 samples (1847.38 seconds of game time). The raw candidate test using every on-disk camera-solid voxel produces 1311 occlusion and 263 inside candidates. Those higher counts include non-cubic models and obsolete closed-door cells; they are not additional confirmed failures.

### Additional route coverage

The original 001→010→011 portal route and each mission's return-to-hub tail were retained separately:

| Capture | World | Eligible samples | Jumps / occluded / inside |
|---|---:|---:|---:|
| run-001 | 001 | 135 | 0 / 0 / 0 |
| run-001 | 010 | 360 | 0 / 0 / 0 |
| run-011 | 010 | 119 | 0 / 0 / 0 |
| run-025 | 001 | 119 | 0 / 0 / 0 |
| run-045 | 040 | 119 | 0 / 0 / 0 |
| run-065 | 060 | 59 | 0 / 0 / 0 |
| run-082 | 080 | 59 | 0 / 0 / 0 |
| run-094 | 090 | 59 | 0 / 0 / 0 |
| run-105 | 100 | 59 | 0 / 0 / 0 |
| run-113 | 110 | 59 | 0 / 0 / 0 |

### Threshold sensitivity and sampling checks

Across all eligible main captures (including the additional hub tails), the same four-times-player rule gives 524 events above 0.25 blocks, 131 events above 0.5 blocks, 57 events above 1.0 blocks. Eye displacement quantiles are p50=0.0417, p95=0.1095, p99=0.2001, p99.9=0.5386 blocks.

| Capture | Camera samples before startup exclusions | Native drawn trace lines | Observer-to-draw check |
|---|---:|---:|---|
| run-001 | 499 | 500 | Every tick drawn; first unindexed frame omitted |
| run-011 | 6051 | 6052 | Every tick drawn; first unindexed frame omitted |
| run-025 | 13680 | 456 | 0.00007969 blocks max error; all match |
| run-045 | 16815 | 560 | 0.00008037 blocks max error; all match |
| run-065 | 22982 | 766 | 0.00008314 blocks max error; all match |
| run-082 | 12882 | 429 | 0.00007944 blocks max error; all match |
| run-094 | 17738 | 591 | 0.00008503 blocks max error; all match |
| run-105 | 10862 | 362 | 0.00008192 blocks max error; all match |
| run-113 | 7986 | 266 | 0.00007820 blocks max error; all match |
| run-001-hub | 2532 | 84 | 0.00007362 blocks max error; all match |

### Ten largest jumps

Positions are before → after the indicated tick. Geometry describes the final target/eye cell, or terrain intersecting the old eye's new view. The raw displacement includes any intentional shake.

| Rank | World / tick | Eye Δ | Blupi Δ | Eye before → after | Blupi before → after | Geometry / exact image |
|---:|---|---:|---:|---|---|---|
| 1 | 045 / 16536 | 4.0040 | 0.0600 | (57.123, 37.449, 23.253) → (53.255, 36.443, 23.010) | (52.830, 36.000, 23.010) → (52.770, 36.000, 23.010) | No static-cube intersection at the final pose [PNG](../../build-probe/camera/run-045-evidence/autoplay_shots/0016536.png) |
| 2 | 105 / 5256 | 3.9816 | 0.0632 | (52.101, 32.449, 65.344) → (48.452, 31.319, 66.465) | (48.260, 30.300, 66.580) → (48.200, 30.300, 66.600) | cell (48, 31, 66), tile 35 [PNG](../../build-probe/camera/run-105-evidence/autoplay_shots/0005256.png) |
| 3 | 045 / 14407 | 3.8833 | 0.0500 | (58.510, 54.117, 14.904) → (54.990, 53.455, 16.405) | (54.540, 52.710, 16.500) → (54.490, 52.710, 16.500) | cell (55, 54, 16), tile 246 [PNG](../../build-probe/camera/run-045-evidence/autoplay_shots/0014407.png) |
| 4 | 025 / 3573 | 3.3996 | 0.0800 | (46.656, 12.023, 61.998) → (49.959, 11.218, 61.997) | (50.390, 11.000, 62.000) → (50.470, 11.000, 62.000) | No static-cube intersection at the final pose [PNG](../../build-probe/camera/run-025-evidence/autoplay_shots/0003573.png) |
| 5 | 065 / 19251 | 3.0757 | 0.0361 | (39.659, 30.319, 50.926) → (41.668, 30.525, 53.246) | (41.870, 30.000, 53.470) → (41.890, 30.000, 53.500) | cell (42, 31, 53), tile 345 [PNG](../../build-probe/camera/run-065-evidence/autoplay_shots/0019251.png) |
| 6 | 065 / 18893 | 3.0751 | 0.0412 | (38.699, 30.320, 53.848) → (38.228, 30.525, 50.816) | (38.200, 30.000, 50.520) → (38.190, 30.000, 50.480) | cell (38, 31, 51), tile 341 [PNG](../../build-probe/camera/run-065-evidence/autoplay_shots/0018893.png) |
| 7 | 065 / 7648 | 3.0742 | 0.0400 | (69.897, 36.320, 26.985) → (66.830, 36.525, 26.982) | (66.530, 36.000, 26.990) → (66.490, 36.000, 26.990) | cell (67, 37, 27), tile 345 [PNG](../../build-probe/camera/run-065-evidence/autoplay_shots/0007648.png) |
| 8 | 065 / 11633 | 3.0678 | 0.0400 | (75.841, 36.910, 43.996) → (78.563, 35.493, 43.996) | (78.810, 35.000, 44.000) → (78.850, 35.000, 44.000) | cell (79, 36, 44), tile 346 [PNG](../../build-probe/camera/run-065-evidence/autoplay_shots/0011633.png) |
| 9 | 065 / 7217 | 3.0669 | 0.0412 | (63.654, 37.411, 28.455) → (66.557, 36.485, 28.109) | (66.810, 36.000, 28.020) → (66.850, 36.000, 28.010) | No static-cube intersection at the final pose [PNG](../../build-probe/camera/run-065-evidence/autoplay_shots/0007217.png) |
| 10 | 113 / 5674 | 3.0386 | 0.0500 | (72.838, 28.920, 55.997) → (69.824, 28.536, 55.997) | (69.530, 28.000, 56.000) → (69.480, 28.000, 56.000) | cell (70, 29, 56), tile 126 (model/state-dependent candidate) [PNG](../../build-probe/camera/run-113-evidence/autoplay_shots/0005674.png) |

### Ten longest static-terrain occlusion runs

Runs are ranked by consecutive samples (ties retain capture order). Positions and the intersected cell are from the representative middle tick. All first/middle/last positions are in `analysis.json`; all samples are in `frames.csv`.

| Rank | World / tick range | Ticks / seconds | Representative tick | Eye | Blupi | Geometry / exact image |
|---:|---|---:|---:|---|---|---|
| 1 | 045 / 3157–3188 | 32 / 0.533 | 3173 | (47.655, 21.880, 56.555) | (48.880, 17.600, 56.500) | cell (48, 19, 57), tile 246 [PNG](../../build-probe/camera/run-045-evidence/autoplay_shots/0003173.png) |
| 2 | 025 / 9084–9100 | 17 / 0.283 | 9092 | (76.793, 12.640, 61.225) | (76.980, 7.750, 59.750) | cell (77, 10, 61), tile 396 [PNG](../../build-probe/camera/run-025-evidence/autoplay_shots/0009092.png) |
| 3 | 065 / 11762–11777 | 16 / 0.267 | 11770 | (79.446, 34.490, 43.432) | (79.500, 31.000, 42.180) | cell (79, 32, 43), tile 347 [PNG](../../build-probe/camera/run-065-evidence/autoplay_shots/0011770.png) |
| 4 | 011 / 284–298 | 15 / 0.250 | 291 | (65.272, 13.608, 54.734) | (64.170, 10.000, 53.360) | cell (65, 11, 54), tile 4  |
| 5 | 065 / 15026–15039 | 14 / 0.233 | 15033 | (70.464, 42.939, 12.056) | (68.970, 40.000, 12.190) | cell (70, 41, 12), tile 362  |
| 6 | 065 / 4359–4371 | 13 / 0.217 | 4365 | (11.888, 53.352, 41.700) | (9.740, 50.000, 41.600) | cell (11, 51, 42), tile 4  |
| 7 | 045 / 3879–3890 | 12 / 0.200 | 3885 | (51.661, 24.069, 55.925) | (52.880, 19.270, 56.500) | cell (52, 21, 56), tile 249  |
| 8 | 065 / 14920–14931 | 12 / 0.200 | 14926 | (74.583, 43.570, 12.133) | (71.590, 42.000, 11.900) | cell (75, 44, 12), tile 341 [PNG](../../build-probe/camera/run-065-evidence/autoplay_shots/0014926.png) |
| 9 | 065 / 11885–11895 | 11 / 0.183 | 11890 | (79.640, 32.620, 42.694) | (79.460, 30.280, 39.890) | cell (80, 32, 43), tile 362  |
| 10 | 045 / 1146–1155 | 10 / 0.167 | 1151 | (9.408, 27.334, 83.035) | (9.950, 24.080, 80.810) | cell (10, 25, 82), tile 248  |

### Ten longest eye-inside-solid runs

Runs are ranked by consecutive samples (ties retain capture order). Positions and the intersected cell are from the representative middle tick. All first/middle/last positions are in `analysis.json`; all samples are in `frames.csv`.

| Rank | World / tick range | Ticks / seconds | Representative tick | Eye | Blupi | Geometry / exact image |
|---:|---|---:|---:|---|---|---|
| 1 | 065 / 14920–14931 | 12 / 0.200 | 14926 | (74.583, 43.570, 12.133) | (71.590, 42.000, 11.900) | cell (75, 44, 12), tile 341 [PNG](../../build-probe/camera/run-065-evidence/autoplay_shots/0014926.png) |
| 2 | 065 / 14908–14916 | 9 / 0.150 | 14912 | (74.728, 43.523, 12.130) | (71.590, 42.000, 11.900) | cell (75, 44, 12), tile 341 [PNG](../../build-probe/camera/run-065-evidence/autoplay_shots/0014912.png) |
| 3 | 045 / 11066–11072 | 7 / 0.117 | 11069 | (65.956, 8.684, 24.428) | (65.980, 8.000, 24.580) | cell (66, 9, 24), tile 248 [PNG](../../build-probe/camera/run-045-evidence/autoplay_shots/0011069.png) |
| 4 | 065 / 18893–18899 | 7 / 0.117 | 18896 | (38.259, 30.655, 50.643) | (38.180, 30.000, 50.350) | cell (38, 31, 51), tile 341 [PNG](../../build-probe/camera/run-065-evidence/autoplay_shots/0018896.png) |
| 5 | 045 / 16541–16546 | 6 / 0.100 | 16544 | (52.659, 36.780, 23.005) | (52.270, 36.000, 23.010) | cell (53, 37, 23), tile 247 [PNG](../../build-probe/camera/run-045-evidence/autoplay_shots/0016544.png) |
| 6 | 065 / 7648–7653 | 6 / 0.100 | 7651 | (66.628, 36.655, 27.019) | (66.360, 36.000, 26.990) | cell (67, 37, 27), tile 345 [PNG](../../build-probe/camera/run-065-evidence/autoplay_shots/0007651.png) |
| 7 | 065 / 14872–14877 | 6 / 0.100 | 14875 | (75.376, 43.570, 12.147) | (71.750, 42.000, 11.910) | cell (75, 44, 12), tile 341  |
| 8 | 065 / 19251–19256 | 6 / 0.100 | 19254 | (41.746, 30.655, 53.399) | (41.980, 30.000, 53.600) | cell (42, 31, 53), tile 345 [PNG](../../build-probe/camera/run-065-evidence/autoplay_shots/0019254.png) |
| 9 | 045 / 14410–14413 | 4 / 0.067 | 14412 | (54.588, 53.590, 16.511) | (54.230, 52.700, 16.680) | cell (55, 54, 17), tile 248 [PNG](../../build-probe/camera/run-045-evidence/autoplay_shots/0014412.png) |
| 10 | 025 / 3602–3604 | 3 / 0.050 | 3603 | (52.100, 11.704, 61.971) | (52.810, 11.000, 62.000) | cell (52, 12, 62), tile 396 [PNG](../../build-probe/camera/run-025-evidence/autoplay_shots/0003603.png) |

### Excluded teleport jumps

These large camera moves follow actual discontinuities in Blupi's position and are not camera-jump failures:

| World / tick | Eye Δ | Blupi Δ | Blupi before → after |
|---|---:|---:|---|
| 065 / 17555 | 63.972 | 58.924 | (70.000, 46.000, 10.470) → (20.000, 36.000, 40.000) |
| 094 / 6550 | 38.583 | 39.717 | (81.020, 22.000, 51.490) → (60.000, 10.000, 20.000) |

Of the 112 measured static-terrain penetration samples in all main captures, 43 also have the look target inside static terrain. On 364 occluded samples the final eye-to-target ray is clear of static cubes while the eye-to-Blupi ray is blocked. These are direct geometric evidence for mechanisms 2 and 3 below.
<!-- /RESULTS -->

## Likely causes in the camera code

1. **The last wall correction can snap immediately, bypassing the damping.** The raw eye/target are damped with `1−exp(−8 dt)` at `src/GalaxyEggbertCNA/GalaxyEggbertGameUpdate.cpp:3086-3106`, but the final ray check replaces the smoothed eye directly at `:3112-3124`. This explains abrupt inward movements when a previously clear arm encounters a wall edge. Elevation is selected in discrete 0.15-radian steps (`src/GalaxyEggbert/Game/ChaseCameraYaw.hpp:84-130`), and the emergency side search chooses among discrete yaws/elevations when room falls below one block (`GalaxyEggbertGameUpdate.cpp:3036-3063`). These discontinuities can change the desired eye suddenly; the final projection is not itself damped.
2. **The final safety check protects a lagging look target, not the actual body.** Elevation selection evaluates a ray toward the feet at `GalaxyEggbertGameUpdate.cpp:2998-3016`, but this is for the raw candidate. The final corrected eye is checked only against `cameraTargetSmoothed_` (`:3114-3120`). Horizontal target lag is unconstrained; only vertical lag has a 0.35-block cap (`:3097-3106`). After a drop, a ray to the elevated/lagged target can clear a ledge while the ray to Blupi is blocked. This is directly measurable where `target_ray_hits` is empty but the body ray hits terrain.
3. **A smoothed target inside a wall makes the final check ineffective.** Only the raw target is lowered out of a ceiling (`GalaxyEggbertGameUpdate.cpp:2951-2954`). The smoothed target is never independently made non-solid. If the final ray begins in a wall, `RaymarchWallDistance` returns zero, but `:3123` still puts the eye 0.05 blocks from that target. Both can remain inside the same block. The raw-arm fallback has the same minimum-distance policy at `:3065-3069`. Several measured eye-inside runs exhibit an eye–target separation of exactly 0.05 blocks with both points inside the same terrain cell.
4. **Shake is added after all collision checks.** `GalaxyEggbertGameUpdate.cpp:3138-3154` adds `(shakeX/64, −shakeY/64, 0)` to eye and target with no subsequent collision validation. The exact 025 replay records shake and demonstrates that subtracting it moves some penetrating eyes back outside the wall. This is distinct from the smoothed-target failure, which also occurs with zero shake.
5. **The ray marcher is approximate and assumes a usable origin.** `src/GalaxyEggbertCNA/GameHelpers.hpp:617-629` tests at 0.1-block intervals, not at the origin, and backs off by a 0.15-block face margin. Thin/grazing intersections can fall between samples. This is a possible additional mechanism, but the observed large snaps and the strongest penetration cases are already explained by items 1–4; this report does not attribute every event to sampling alone.

The yaw controller (`src/GalaxyEggbert/Game/ChaseCameraYaw.cpp`) smoothly follows yaw and changes idle/viewer-facing sides. Its motion alone is not evidence of a defect. The demonstrated failures occur in the host's geometry/smoothing/collision composition above.

## Exact-frame visual evidence and replay checks

All ten jump rows above link to an exact PNG. Five targeted script-prefix replays (025, 045, 065, 105, 113) reproduced **56,783 camera-update samples with zero eye or player-coordinate difference**. Their **2,169 indexed drawn frames** agree with the original eye positions within 0.000080 blocks; their observed shake values agree exactly with the native trace. `evidence-validation.json` and `evidence-check.txt` retain the per-replay counts. These repeats are not included again in the main totals.

* **045, 16535→16536:** the arm collapses to 0.050 blocks in one tick. Before, Blupi and the helicopter are visible beneath the cheese ceiling; afterward the helicopter fills the frame from inside. This is the 4.004-block jump. [Before](../../build-probe/camera/run-045-evidence/autoplay_shots/0016535.png), [after](../../build-probe/camera/run-045-evidence/autoplay_shots/0016536.png).
* **105, 5256:** the 3.982-block jump leaves both eye and target inside cobble cell (48,31,66), with a 0.050-block arm and zero shake. The image looks through the surrounding geometry into the cavern; the controlled Blupi/vehicle is no longer usefully framed. [Exact frame](../../build-probe/camera/run-105-evidence/autoplay_shots/0005256.png).
* **045, 3173:** a broad cheese ledge fills the foreground and conceals Blupi during the longest, 32-tick occlusion run. The eye-to-smoothed-target ray is clear while the body ray crosses cell (48,19,57). [Exact frame](../../build-probe/camera/run-045-evidence/autoplay_shots/0003173.png).
* **065, 14926:** the eye is in slime terrain during the longest penetration run. The screenshot shows a cut-through ceiling and Blupi's head crowded against the bottom/HUD. This illustrates why voxel occlusion and complete silhouette invisibility are reported separately: parts of the character can remain visible even when the tested body points are blocked. [Exact frame](../../build-probe/camera/run-065-evidence/autoplay_shots/0014926.png).
* **025, 3609:** the eye (52.4054,12.2584,61.9965) is in wall cell (52,12,62). Shake is (−12,−12) pixels; subtracting its world offset gives (52.5929,12.0709,61.9965), outside that wall. This directly demonstrates the unchecked post-collision shake. [Exact frame](../../build-probe/camera/run-025-evidence/autoplay_shots/0003609.png). At [tick 3603](../../build-probe/camera/run-025-evidence/autoplay_shots/0003603.png), nearby explosions additionally dominate the clipped view.
* **113, 5674:** the eye and target enter the full-cell collision volume of fan tile 126, the arm collapsing to 0.050 blocks with zero shake. The screenshot crowds Blupi against the bottom of a very tight corridor. This is a confirmed jump but is **not** counted as static-cube eye penetration: the fan is a model, not an opaque full voxel. [Exact frame](../../build-probe/camera/run-113-evidence/autoplay_shots/0005674.png).

World 045, consecutive frames 16535 and 16536:

![Before the 4.004-block jump](../../build-probe/camera/run-045-evidence/autoplay_shots/0016535.png)

![After the 4.004-block jump](../../build-probe/camera/run-045-evidence/autoplay_shots/0016536.png)


## Reproduction and retained artifacts

`tools/camera_probe.py` and `tools/camera_probe_gdb.py` contain the capture and analysis code with usage/header comments. The observer is specific to the Linux x86-64 SysV ABI and this symbol-bearing binary; its register/layout reads must be revalidated for another ABI/build. It never writes camera/game state. The optional export bypass returns only from a void diagnostic PNG-export function.

```bash
python3 tools/camera_probe.py self-test
# Fresh destinations are required; existing captures are never overwritten.
python3 tools/camera_probe.py run 011 001
python3 tools/camera_probe.py run 025 045 065 082 094 105 113 --tick-observer
python3 tools/camera_probe.py run 001 --label=-hub --tick-observer --realtime --script tools/autoplay/camera-hub.txt
python3 tools/camera_probe.py analyze
python3 tools/camera_probe.py validate-evidence  # retained exact-shot replays
python3 tools/camera_probe.py report-tables
```

The existing captures are sufficient to rerun `self-test`, `analyze`, `validate-evidence`, and `report-tables`. Each `run-NNN/metadata.json` records the executable/world/script hashes and exact command; `input.txt`, `autoplay_trace.txt`, `interp.txt`, `draw_ticks.csv`, optional `camera_ticks.csv`/`shake_ticks.csv`, and `game.log` preserve raw evidence. `frames.csv` is the per-sample result; `analysis.json` holds summaries, exclusions, and every detected jump/occlusion/penetration run, not only the top ten. `run-*-evidence/` and `evidence-*.txt` contain the exact-shot replays. `run-*-observer/` contains independent validation replays. Only the main `run-NNN` directories and `run-001-hub` enter aggregate counts; repeats and the pilot are excluded. The longer hub script is retained as `tools/autoplay/camera-hub.txt`.
