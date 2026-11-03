# Wave F — release audit at pinned revisions (2026-11-03)

_The fidelity audit's last wave (`../GALAXY_EGGBERT_FIDELITY_AUDIT.md` §3): what was built and
checked, at which revisions, and what still stands between this tree and a release. Written from the
evidence named in each row; nothing here is claimed from compilation alone._

## 1. Pinned revisions

| Repository | Revision | State |
|---|---|---|
| galaxy-eggbert (`develop`) | the commit that adds this file (parent `69376154`) | the Android port in `c5dcdc5`, `c48ee08`, `37ca3b3` |
| `../../libcna/cna` | `5a754159d` | live tree of another session, read only; since the last full regression (`090c998a1`) only its C API packaging changed (`modules/c-api/cmake`, `tools/build`): nothing the game compiles (the desktop rebuild after it recompiled only Galaxy's `main.cpp`) |
| `../../libcna/cna-engine` | `2a93b7e43` | clean |
| `../../libcna/easy-gl` | `b582bd85f` | clean (v0.1.1 pin) |
| `../../libcna/meta-gl` | `795bcdcfe` | clean |
| `../../libcna/sharp-runtime` | `638c7e3c9` | clean |
| References (read only) | mobile-eggbert `f96f02f`, free-eggbert `74457dd`, blupi_recon `61c96b3`, speedyblupi-data `7f5cbff` | unchanged by this campaign |

## 2. Builds

| Target | Directory | Result | Evidence |
|---|---|---|---|
| Linux desktop (OpenGL 3.3, Release) | `build-cna-opengl3/` | builds; `GalaxyEggbert` | 2026-11-03 rebuild after the Android entry point |
| Web (WebGL 2, Emscripten) | `build-cna-webgl2/` | builds; `GalaxyEggbert.{html,js,wasm,data}`; loads in headless Chrome (menu drawn, 120 models, 93/93 sounds) | `handoff.md` 2026-11-03, built before the Android commits, which change nothing Emscripten compiles but `main.cpp`'s include of CNA's `Entrypoint.hpp` (empty there); the music is served beside the page and fetched per track since `789ceee` (OR-0014) |
| Android (x86_64 debug APK) | `android/app/build/`, `android/app/.cxx/` | builds and runs to the hub; no 3D models (UB-007), see §4 | OR-0011 |

## 3. Tests and regression

| Check | Result | Where |
|---|---|---|
| CTest (112 tests: verifiers of movement, interaction, editor, palette, models, worlds) | 112/112 | `ctest --test-dir build-cna-opengl3`, rerun at the pins above after the Android commits |
| Campaign playthroughs (every mission played to its exit by script) | 78/78 | rerun at the pins above: `build-probe/wave-e/lanes_log.sh 4 build-probe/wave-e/release-f.txt` (1,603 s, four lanes) |
| World 199's spare enclaves | win | `tools/autoplay/spare/world199.txt` |
| Campaign structure (`tools/campaigncheck.py`), water exits (`tools/watercheck.py`) | the main hub, 12 hubs, 64 missions and 199 complete; every body of water in the 80 worlds has a way out | rerun at the pins above |
| Creature timing (`tools/audit/patrol_envelope.py`) | 0 outside the original's envelope | rerun at the pins above |

## 4. Android (OR-0011)

| Step | Result | Evidence |
|---|---|---|
| Build (`./gradlew assembleDebug -Pandroid.injected.build.abi=x86_64`, NDK r29, API 24) | 102 MB APK: `libmain.so` (exports `SDL_main`), `libSDL3.so`, `libSDL3_mixer.so`, 2,562 assets and `assets.txt` | `c5dcdc5` |
| Install in the API 35 x86_64 emulator (`-read-only`, no KVM: software emulation) | installs (`install -t`: a single-ABI build is test-only) | |
| First start | copies the 2,562 assets into internal storage, draws the title scene | `plans/evidence/android-emulator-loading.png` |
| Loading | stalled in CNA's APK walk per missing `.xnb` (UB-006); with the absolute content root it completes: 120 object models, 93/93 sounds, terrain, background | `c48ee08`; logcat (tag `GalaxyEggbert`) |
| Main menu | drawn (players, setup, editor, play) | `plans/evidence/android-emulator-menu.png` |
| Play (a held touch) | the main hub with the on-screen pad and lives | `plans/evidence/android-emulator-hub.png` |
| 3D models | **not drawn**: CNA's skinned shader exceeds 256 vertex uniform vectors (UB-007) | logcat `skinned (vertex-lit) link failed` |
| Play, touch feel, tilt | not judged: without KVM a frame takes seconds (Q-009) | |

Verdict for Android: the port builds and runs end to end; it is not releasable until CNA fixes
UB-007 (or the target devices report more than 256 vertex uniform vectors) and a person has
played it on a device.

## 5. Ledgers

| Ledger | Rows | Not settled |
|---|---|---|
| `gameplay_use_cases.csv` | 538 | 0 (BD-0053, the follower star's wake, fixed after the audit: the original's box, 052/053 re-laid, 78/78) |
| `world_lessons.csv` | 80 | 0 (55 match, 21 fixed, 2 adaptations, 2 not applicable); no human play pass (MR-0004) |
| `behavior_differences.md` | 57 | none open; BD-0031 fixed, its live listening waits for a run with sound |
| `editor_parity.md` | all ED rows | 0 unreviewed |
| `asset_sources.csv` | 703 | 185 BLOCKED: the 56 `.max` (MR-0001, no licensed 3D Studio MAX) and the files waiting on them or on Q-004 |
| `render_representation.csv` | 564 | 10 UNREVIEWED (D11's candidates: the owner's own list), 283 BLOCKED on Q-004 (terrain tiles as models) and Q-001/Q-002 (grass, lightning) |

## 6. What stands between this tree and a release

Owner decisions (`owner_questions.md`, `manual_review.md`), none of which the code can settle:

- Q-001 grass and Q-002 the lightning bolt in 3D (task #10); Q-003 the clay bridge's surface;
  Q-004 the terrain tiles as Ray Dream models; Q-005 the electrode's sparks; Q-006 the glue
  death's strands; Q-008 world 105's lava floor;
  Q-009 KVM or a phone for the Android test.
- D11: the owner's list of models that look wrong (`plans/inventory/d11_candidates.md`).
- MR-0001: the `.max` sources need a licensed 3D Studio MAX.
- MR-0004: a human play pass (feel of crates, lifts, editor, looks); every machine check passes.

Open engineering items:

- OR-0017: Blupi's collision is a point; the original's 36 px box (`or0017-body-width.patch`)
  fails 55 of 78 playthroughs until they are re-planned world by world. His model is kept out of
  walls (render only).
- OR-0011: Android draws no 3D models until CNA fixes UB-007; UB-006 is worked around; no
  device or KVM here to play it (Q-009).

Verdict: the tree builds and passes every automated check at the revisions above; it is not a
release until the owner's decisions above are taken and a human has played it.
