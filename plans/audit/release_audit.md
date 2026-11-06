# Wave F — release audit at pinned revisions (2026-11-06)

_The fidelity audit's last wave (`../GALAXY_EGGBERT_FIDELITY_AUDIT.md` §3): what was built and
checked, at which revisions, and what still stands between this tree and a release. Written from the
evidence named in each row; nothing here is claimed from compilation alone. The first audit
(2026-11-03, at galaxy-eggbert `69376154`) is in this file's git history._

## 1. Pinned revisions

| Repository | Revision | State |
|---|---|---|
| galaxy-eggbert (`develop`) | the commit that adds this version (parent `a0523ecd`) | clean before it |
| `../../libcna/cna` | `be933add6` (2026-11-06) | live tree of another session, read only, 69 files changed in its working tree: the builds compile what was there when they ran (memory `project_cna_live_working_tree`) |
| `../../libcna/cna-engine` | `6737a1cf0` | clean |
| `../../libcna/easy-gl` | `b582bd85f` | clean (v0.1.1 pin) |
| `../../libcna/meta-gl` | `795bcdcfe` | clean |
| `../../libcna/sharp-runtime` | `c793ef3e8` | clean |
| References (read only) | mobile-eggbert `f96f02f25`, free-eggbert `74457dd94`, blupi_recon `61c96b37e`, speedyblupi-data `7f5cbffb9` | unchanged by this campaign (the untracked files in blupi_recon and speedyblupi-data date from May 2026, before it) |

## 2. Builds

| Target | Directory | Result | Evidence |
|---|---|---|---|
| Linux desktop (OpenGL 3.3, Release) | `build-cna-opengl3/` | builds; `GalaxyEggbert` | rebuilt 2026-11-06 at the pins |
| Web (WebGL 2, Emscripten) | `build-cna-webgl2/` | builds; loads in headless Chrome (121 object models, 93/93 sounds, the menu); the editor's export downloads a world and its import takes one from the file picker | rebuilt 2026-11-06; `plans/evidence/editor-export-import-web.jpg` |
| Android (x86_64 debug APK) | `android/` | builds at the pins (`./gradlew assembleDebug -Pandroid.injected.build.abi=x86_64`, 1.5 min with ccache); in the API 35 emulator (software, `-accel off`) it loads the 121 object models and Blupi and **draws them in 3D** on the title scene: CNA's UB-007 fix (`22739f89b`) seen working; not played (Q-009) | `plans/evidence/android-emulator-models-ub007.jpg`; OR-0011; `upstream_bugs.md` |

## 3. Tests and regression

| Check | Result | Where |
|---|---|---|
| CTest (112 tests: movement, interaction, editor, storage, palette, models, worlds) | 112/112 | `ctest --test-dir build-cna-opengl3` |
| Golden frames | match the approved references | `xvfb-run -a tools/verify_golden_frames.sh build-cna-opengl3` |
| Campaign playthroughs (every mission played to its exit by script, each world as one chain) | **78/78** | play pass 31: `PLAY_PASS_JOBS=4 tools/play_pass.sh build-probe/pass-bin build-probe/play-pass-31` |
| Every egg, key, treasure and exit has evidence it can be reached | 921 targets, 0 unknown (levelcheck 223, taken in a playthrough 575, from the run's cells 94, behind an earned door 4, timed 12, staged 13) | `tools/audit/target_reach.py build-probe/play-pass-31`; `target_reach.md` |
| Campaign structure, water exits | the main hub, 12 hubs, 64 missions and 199 complete; every body of water in the 80 worlds has a way out | `tools/campaigncheck.py`, `tools/watercheck.py` |
| Creature and lift timing against the original's | 0 creatures off their type's timing; 0 lifts faster than the original's fastest (424 lifts in 52 worlds) | `tools/audit/patrol_envelope.py`, `tools/audit/lift_envelope.py` |
| The sounds' pitch per channel (BD-0031), measured | see §4 | `tools/audit/sound_pitch.py` |

## 4. The sounds' pitch, measured (BD-0031)

The original plays each channel at its own pitch (`tableVolumePitch`, most effects 0.1-0.5 octave
up, the crate efforts 38/39 and channels 1-2 a whole octave). Measured without playing anything
aloud: a playthrough run in real time with SDL's disk audio driver
(`AUTOPLAY_REALTIME=1 QA_SOUND=1 SDL_AUDIODRIVER=disk SDL_AUDIO_DRIVER=disk
SDL_AUDIO_DISK_OUTPUT_FILE=<file> tools/autoplay.sh ...`), the recording searched for every sound
file resampled to each pitch from 0 to 1 octave (normalised cross-correlation).

| Recording | Channels heard at their table pitch (ncc 0.96-1.00) | Heard at another pitch |
|---|---|---|
| 012 (55 s, crates) | 1, 38, 39 at 1.0; 3, 4, 14, 80 at 0.2 | none |
| 045 (285 s, lifts, helicopter, bulldozer, crushers) | 1 at 1.0; 3, 11, 14, 37, 65, 88 at 0.2; 12 at 0.1; 16, 18 at 0.0 | none |

14 distinct channels, every pitch value the table uses for them. Channel 2's file resembles channel
1's an octave apart and is found only where 1 plays: not a sound of its own. BD-0031 is
FIXED_AND_VERIFIED; whether it sounds right is the owner's ear.

## 5. Ledgers

| Ledger | Rows | Not settled |
|---|---|---|
| `gameplay_use_cases.csv` | 538 | 0 (208 match, 98 fixed and verified, 51 3D adaptations, 181 not applicable) |
| `world_lessons.csv` | 80 | 0 (55 match, 22 fixed, 1 adaptation, 2 not applicable) |
| `behavior_differences.md` | 74 | 0 open (BD-0001..0074; BD-0031 verified by measurement, §4) |
| `editor_parity.md` | 13 (ED-0001..0013) | 0 |
| `mechanics_3d_decisions.md` | M3D-0001..0006 | decided |
| `asset_sources.csv` | 703 | 0 (288 used and verified, 10 adaptations, 405 not applicable: among them the 56 `.max`, MR-0001) |
| `render_representation.csv` | 565 | 0 (388 fixed and verified, 146 adaptations, 31 not applicable) |
| `owner_reports.md` | OR-0001..0072 | answered or fixed; OR-0046 (the 3ds Max offer) answered, the copy not used |
| `owner_questions.md` | Q-000..Q-014 | all answered; nothing waits on the owner (MR-0009) |

## 6. What stands between this tree and a release

Settled by the owner since the first audit: Q-001..Q-008, Q-010..Q-014 built as decided; D11
closed with the models named (MR-0009); the playability audit by the automated playthroughs and
the target reach audit, without a human play pass (MR-0007). Still open, none of which the code
can settle:

- MR-0001: the 56 `.max` sources need a licensed 3D Studio MAX (the offered copy is not used);
  the shields stay billboards and Blupi comes from the Ray Dream scenes.
- Q-009 / OR-0011: Android stays at "builds and starts" by the owner's choice; no device or KVM
  here to play it. UB-007 (no 3D models) is fixed in CNA (`22739f89b`) and seen working on the APK
  2026-11-06; UB-006 (slow content lookups, worked around in the game) reported to the cna session
  2026-11-06.
- A person's ear and eye: the sounds were measured, not listened to; looks were judged on
  screenshots.

Verdict: the desktop and web builds pass every automated check at the revisions above, and every
ledger is settled. A release of the desktop and web builds waits only on the owner's word; Android
draws its 3D models now and waits on a device (or KVM) to be played.
