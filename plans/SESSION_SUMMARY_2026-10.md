# Galaxy Eggbert — summary of the completion session (2026-10-25 to 2026-10-27)

A status report of one long AI-assisted working session (several context windows, plus background
sub-agents it directed), written for an external review together with the source code. Verified
facts only; where something is approximate or open, it says so.

Note on dates: the commits of this session are dated 2026-10-25 and 2026-10-26 (system clock);
several documents written during it say 2026-10-27. The difference is documentation drift, not a
second period of work.

## 1. Brief and constraints

The owner's goal: finish Galaxy Eggbert as a faithful, fully playable, polished true-3D tile/voxel
remake of Mobile Eggbert / Speedy Blupi on CNA (an XNA 4.0 reimplementation in C++) and
cna-engine, keeping the existing architecture (voxel worlds, the `.vwr` format extended only
additively, the 3D editor, controls, camera, physics, registries, saves). Gameplay comes from the
original's C++ (`mobile-eggbert`, read-only, authoritative); models and animations from the
original Ray Dream scenes (`speedyblupi-data`). Worlds are original 3D tile designs (no 2D-to-3D
extrusion): the main hub, 12 theme hubs, 64 missions, the bonus world 199, plus the world999
showcase. Constraints: CNA absolutely read-only (defects recorded in `upstream_bugs.md`);
cna-engine changes only justified and upstream (no workarounds in Galaxy); fixes only for
reproduced bugs, with regression tests; small commits, no AI attribution, push only when asked;
shared ccache and strict build-directory rules (`../CLAUDE.md`).

## 2. What was done

### 2.1 Numbers

- 361 commits since the restore point `00ba088` (124 feat, 76 fix, 75 test, 70 docs, 12 tools);
  1718 files changed, +42 608 / −1 993 lines (most of the bulk is world data, converted models
  and playthrough scripts).
- 79 shipped worlds (`worlds3d/`), each built by a Python script (`tools/worlds/worldNNN.py`) from
  a written brief (`plans/briefs/worldNNN.md`).
- 79 autoplay scripts (`tools/autoplay/`): every mission, every world as a chain from its hub, the
  bonus world, and the whole campaign in one run.
- 110 automated tests (CTest), all passing.

### 2.2 Phases

1. **Restart on the restored `develop`** with a new plan (`plans/GALAXY_EGGBERT_COMPLETION.md`):
   controls, menus, title scene, editor slots, hint zones, world tooling (`worldkit.py`,
   `tools/worlds/kit.py`, `levelcheck.py`, `VerifyShippedWorlds`), renderer fixes (props, lifts,
   lighting, signs, door bars, sprite tiles cut out), Blupi's 3D model rebuilt from the Ray Dream
   originals, hubs working as the original's (signs, mission bars, gold bars, golden key, arrival
   points), 3D crates (solid, push along X/Z, pull), 3D dynamite.
2. **The campaign authored**: the main hub, all 12 worlds (hub + missions each) and world 199,
   partly by this session, partly by background agents it briefed and reviewed. Each world has a
   brief, a builder script, staged variants for reachability checks and a live check.
3. **Engine defects found by building and playing the worlds, fixed with tests**: D1-D59 in the
   plan's §5 (see 2.3).
4. **QA autoplay**: a deterministic script player built into the game
   (`src/GalaxyEggbert/Game/Autoplay.{hpp,cpp}`, `--autoplay`, verified by `VerifyAutoplay`),
   used to play every mission through, then every world as a chain from its hub, then the whole
   campaign from a new game to the win screen (`tools/autoplay/campaign.txt`, 755 171 ticks,
   about 3.5 hours of game time). The playthroughs found engine defects D55-D59 and world defects
   in 13, 25, 62, 66, 82, 94, 103, 104, 105, 113, 114, 121-124 and 199.
5. **Milestone F (stabilisation and completion audit) closed**: the test suite, a live sweep that
   loads every world (`tools/qa_sweep.sh`, 79/79), the campaign structure check
   (`campaigncheck.py`), the water-escape check (`watercheck.py`) and the campaign playthrough.
6. **Model parity (milestone C)**: a parity matrix for the 82 object models
   (`plans/inventory/model_parity.md`, silhouette IoU against every sprite icon: 75 at 0.85 or
   better, the 7 below faithful by inspection); every turning object re-matched every 5 degrees;
   per-object colour tones calibrated from the sprites (owner's choice) for 37 objects and 4
   vehicles (colour error 37.6 → 14.0 on the toned objects); Blupi's 45 animation clips match
   their icons at 0.94-0.99 mean IoU.

### 2.3 Defects fixed (plan §5, one row each with cause, fix and test)

- Rendering and assets (D1-D3, D14-D17, D31, D35, D36, D47, D48, D51): half-body poses, inward
  free-form caps, black placeholder cards, lifts drawn as boxes, flat-shaded terrain, sprite
  tiles as opaque cubes, vertex buffers over the GPU limit, mirrored signs (fixed in cna-engine).
- Controls, camera, menus (D4-D9, D12, D18, D22, D34, D38): step-up, default view, turn speed,
  menu text, camera against walls, crashes underwater, teleport arrival.
- Gameplay against the original (D13, D19-D21, D23-D30, D32, D33, D37, D39-D46, D49, D50,
  D52-D59): vehicles and mines, hub doors, crates in 3D, dynamite blast in 3D, fans and wind,
  enemies and shots along Z, followers in depth, swimming, decor collision, saws and switches,
  lift timing, hiding from followers, bars, the overcraft's hover limit, the flight barrier,
  crate rows, lightning visibility, jumping off lifts, crumbling bridges, a lift's reach and
  penetration recovery, door keys opening whole doors, dynamite pickup rules.
- D11 (models that look wrong) is the only defect not closed: it waits for the owner's list.

## 3. Evidence of the current state

- `ctest --test-dir build-cna-opengl3`: 110/110.
- `tools/qa_sweep.sh build-cna-opengl3`: 79/79 worlds load, walk, close without an error in the log.
- `tools/autoplay.sh build-cna-opengl3 tools/autoplay/campaign.txt 1`: passes, ending in the win
  screen; it includes every mission's script, so it is the full gameplay regression.
- `python3 tools/campaigncheck.py`, `python3 tools/watercheck.py worlds3d/world*.vwr`: pass.
- `plans/inventory/model_parity.md`, `plans/inventory/worlds.md`: regenerated from the shipped data.

## 4. Open items, known weaknesses and risks (honest list)

- **D11**: the owner's list of models that look wrong is pending; candidates prepared in
  `plans/inventory/d11_candidates.md`.
- **BB3D-032**: the fish's head renders ~15 % larger than in its sprites; every hypothesis tested
  and recorded in `plan.md` without finding the cause. Cosmetic.
- **Procedural textures** (palm bark, wood grain, metal noise) are flat colours: Ray Dream's noise
  algorithms are unknown, and an approximate bake measured worse against the sprite.
- **Colour tones** are fitted on offline renders whose lighting approximates the game's; they
  bring the colours measurably closer to the sprites, but were checked live only in a few places.
- **Autoplay scripts are timing-tuned**: deterministic, with margins chosen as the middle of the
  passing range, but a change to movement physics can shift them; they then need re-tuning
  (`tools/autoplay_search.py`), not necessarily a code fix.
- **Static checks are conservative**: `levelcheck.py` does not model the wasp's balloon (false
  traps reported in 052 and 054), dynamite, vehicles or keys in all cases; it under-reports
  reachability, never over-reports it.
- **3D can weaken 2D lessons**: some obstacles can be walked round in 3D (094's mine rows were
  fixed for that; 066's blupit can still be walked round). A systematic review of each mission's
  lesson against its 3D layout has not been done beyond what the playthroughs exposed.
- **Golden frame tests are stale** since the 3D model work (`tests/golden/`); live checks replace
  them for now.
- **Dependence on sibling repositories**: CNA is a live working tree edited by another session;
  versions verified together are listed in `handoff.md` §6.

## 5. Where to look in the code

- Game host and platform wiring: `src/GalaxyEggbertCNA/GalaxyEggbertGame.cpp`.
- Gameplay: `src/GalaxyEggbert/Game/` (`BlupiController`, `InteractionSystem`, `WorldRuntime`,
  `Autoplay`, registries); shared definitions `include/GalaxyEggbert/Def/`; hubs
  `include/GalaxyEggbert/HubDoors.hpp`.
- Editor: `src/GalaxyEggbert/Editor/`.
- Verifiers (the test suite): `tools/Verify*.cpp`.
- World authoring and checks: `tools/worlds/`, `tools/levelcheck.py`, `tools/campaigncheck.py`,
  `tools/watercheck.py`, `tools/worldkit.py`.
- QA: `tools/autoplay*.{sh,py}`, `tools/autoplay/`, `tools/qa_live.sh`, `tools/qa_sweep.sh`.
- Model conversion: `tools/rd4_to_gltf/` (its `README.md` records every verified Ray Dream
  convention), outputs in `avatars3d/`.
- Plans and records: `plans/GALAXY_EGGBERT_COMPLETION.md` (plan, defects, milestones), `plan.md`
  (detailed backlog), `CURRENT.md`, `handoff.md`, `upstream_bugs.md`, `plans/briefs/`.

## 6. Questions worth an independent review

1. Are the gameplay rules in `BlupiController`/`InteractionSystem` faithful to `mobile-eggbert`'s
   `Decor.cpp` where the 2D original had to be extended to 3D (lifts, crates, blasts, patrols
   along Z)?
2. Do any missions' 3D layouts let the player skip the obstacle the original mission was about?
3. Is the autoplay harness (`Autoplay.cpp`) and the way the game feeds it (`StepAutoplay`)
   isolated enough not to change normal play?
4. Are there code paths in the engine that the playthroughs never exercise (e.g. editor-made
   worlds, rarely used objects)?
