# Task: the "ghost" cheat as Mobile Eggbert's: nothing touches him, he walks and stands see-through (Galaxy Eggbert)

Repository: /rv/data/development/github.com/openeggbert/galaxy-eggbert (develop). Read `AGENTS.md`
and `../AGENTS.md` and follow them.

## Hard rules
- Never modify anything outside this repository; CNA, cna-engine, mobile-eggbert, speedyblupi-data,
  blupi_recon, free-eggbert are read only (never read speedyblupi-data at all).
- Commit the finished task yourself (one commit, no AI attribution; see `plans/codex/README.md`);
  never push, stash or reset. Keep any unrelated uncommitted changes you find.
- Build only in `build-cna-opengl3/`: `export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv;
  cmake --build build-cna-opengl3 --parallel 4`; `ctest --test-dir build-cna-opengl3 -j4` (all pass).
  Silent game runs; runs under `build-probe/ghost/`. If the build fails inside `../../libcna/cna`
  (another session's live tree), report and stop.

## The owner's reports (2026-11-06)
1. "I turned on ghost Blupi; he should not interact with his surroundings, but the lightning killed him."
2. "In ghost mode (the cheat) Blupi should not have the falling animation but look see-through like a ghost."

## The original (`../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Decor.cpp`, all under `#ifdef MODERN`)
- `BlupiStep()` (~2733): `if (m_blupiGhost) { BlupiGhostStep(); return; }` -- none of the step's
  checks run: no hazard (lightning, lava, saws, spikes, drip, crusher, water...), no tile reaction.
- `BlupiGhostStep()` (~2640-2700): free movement; `m_blupiAction` = March when moving, Stop when not;
  `BlupiSearchIcon()`. Read it whole (also how it treats facing and the vertical).
- `BlupiElectro` (~9611), `MoveObjectFollow` (~9648), `MoveObjectDetect` (~9691: returns -1, bNear
  false), `MoveChargeDetect` (~9805), `MovePersoDetect` (~9837): all bail out for the ghost -- no object
  touches him (no enemy, mine, bomb, pickup, door, exit, lift ride...).
- Drawing (~820-826 and ~1158-1165): the ghost is drawn at opacity 0.75 (`kGhostBlupiOpacity`).
- `CheatAction` Ghost (~2044-2075): on/off rules (Galaxy's `BlupiController::ToggleGhost` already ports them).

## Galaxy now
`BlupiController` has `m_ghost` / `IsGhost()` and a free-flight branch in `Step()` (~1496), but the
game's hazard checks (`src/GalaxyEggbertCNA/GalaxyEggbertGame.cpp` ~4880-5000: lava, spikes, drip,
lightning ~4950, saws, crusher, water...) use `IsInvincible()`, which does not include the ghost; the
`InteractionSystem` object contacts may not check it either; and his animation/draw ignores it.

## Do
1. Make every check the original skips for the ghost skip in Galaxy too: all hazards and deaths, all
   MoveObject contacts and pickups (InteractionSystem; also follow/home-in, electro, charge, perso
   detection), switches/fans/teleporters/doors/exits/wind/conveyors/lifts acting on him. Prefer one
   clear gate (e.g. a `BlupiController` query used at the top of the step's checks) over scattering,
   and cite the original's lines. Do not invent: if the original lets something still act on a ghost,
   keep it (check each).
2. Animation: the ghost uses the original's March while moving, Stop while not (whatever clip mapping
   Galaxy uses for March/Stop with the 3D model and the sprite fallback); never Fall/Jump/etc.
3. Draw him at 0.75 opacity (the 3D model and any fallback), as the original; correct alpha blending
   and depth so he reads as see-through without breaking other draws (look at how Hide's 0.3 opacity
   is drawn in Galaxy, if it is, and reuse).
4. Tests: a pure-logic verifier (the closest `tools/Verify*.cpp`, e.g. `VerifyBlupiMovement` or
   `VerifyInteractionSystem`): a ghost over an active lightning/lava cell does not die; a ghost
   touching a mine/enemy is not hurt; his anim state is March moving and Stop idle. All ctest pass.
5. Live: an autoplay probe under `build-probe/ghost/` (the cheat word "ghost" typed: see how the
   autoplay types cheats, `src/GalaxyEggbert/Game/Autoplay.*`, or `tools/qa_live.sh` steps) in a world
   with lightning (grep `worlds3d` via `tools/vwr_inspect.py` for icon 305), flying through the active
   lightning and surviving; screenshots of the see-through ghost walking and standing; report paths.
6. Golden frames/trace must still pass (`xvfb-run -a tools/verify_golden_frames.sh build-cna-opengl3`,
   `xvfb-run -a tools/verify_golden_trace.sh build-cna-opengl3`).

## Report
Files changed, each original guard and where Galaxy now honours it, the animation and draw changes,
test lines, screenshot paths, anything left (e.g. something the original still lets act on a ghost).
