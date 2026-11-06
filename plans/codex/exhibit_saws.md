# Task: the exhibit saw switches of the gallery and the showcase turn saws (Galaxy Eggbert)

Repository: /rv/data/development/github.com/openeggbert/galaxy-eggbert (develop). Read `AGENTS.md`
and `../AGENTS.md` and follow them.

## Hard rules
- Never modify anything outside this repository; CNA, cna-engine, mobile-eggbert, speedyblupi-data,
  blupi_recon, free-eggbert are read only (never read speedyblupi-data at all).
- Commit the finished task yourself (one commit, no AI attribution; see `plans/codex/README.md`);
  never push, stash or reset. Your files: the generators of world998 (`tools/worlds/gallery.py`) and world999 (find
  its generator, e.g. `tools/GenerateSampleWorld3D.cpp` or a `tools/worlds/*.py`), the generated
  `worlds3d/world998.vwr` / `world999.vwr`, and their docs if a generator writes them.
- Building: only if the world999 generator is a C++ tool, in `build-cna-opengl3/`
  (`export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv; cmake --build build-cna-opengl3 --target
  <that tool> --parallel 4`). If the build fails inside `../../libcna/cna`, report and stop. To play,
  use `build-cna-opengl3` (build it first if stale). Silent game runs; runs under
  `build-probe/exhibit-saws/`.

## The problem (owner 2026-11-06: "the saw on/off machine has no effect"; he tried it in the
gallery/showcase: "I walk up to the machine, press space and nothing happens")
The switch turns the saws (378 turning <-> 379 stopped) within 20 cells along X in its own row: same
Y and Z (`WorldRuntime::TryActivateSwitch`, Decor.cpp:7138-7147). Blupi uses a switch standing on it
or touching it at his height. The gallery's "blocks as the worlds use them" row (z 56) already does
it right (saws and switches in one floor row). But the exhibits -- every icon on its pedestal --
show the switches 384/385 alone: world998 at (14, 1, 42) and (16, 1, 42), world999 at (3, 1, 19)
(and any other 384/385 exhibit you find; check with `python3 tools/vwr_inspect.py`). Pressing them
toggles the switch and nothing else, which reads as broken. The owner's decision: put saws in the
rows of those exhibit switches.

## Do
1. In each generator, put a few saws (378, and one stopped 379 where it fits) in the same row as each
   exhibit switch (same Y and Z, within 20 cells along X), where they do not overwrite other exhibits
   and do not make the gallery's walking routes deadly (a turning saw kills; keep the visitor's
   ways free: e.g. saws on their own short pedestal row beside the switch, reachable to look at).
   Note it in the generator's legend/notes the way the z 56 row is noted.
2. Regenerate both worlds with their generators (never hand-edit the .vwr).
3. Prove it: an autoplay script under `build-probe/exhibit-saws/` (verbs: `src/GalaxyEggbert/Game/
   Autoplay.hpp`/`.cpp`, header of `tools/autoplay.sh`) walking to an exhibit switch in 998 and in 999,
   pressing Action, and taking screenshots (`AUTOPLAY_SHOTS` or the `shots` verb) before and after
   showing the saws stopped/turning; and check with the trace or a block probe that the saw blocks
   changed. Run: `AUTOPLAY_WORLDS=... AUTOPLAY_RUN=$PWD/build-probe/exhibit-saws/<n> tools/autoplay.sh
   build-cna-opengl3 <script> 998` (see autoplay.sh for how the worlds dir is picked; the working
   tree's worlds3d must be the one played).
4. `ctest --test-dir build-cna-opengl3 -j4` must still pass (some verifiers read 998/999; if one checks
   an exact cell you moved, update the verifier only if the test's intent survives, and say so) --
   run ctest only if build-cna-opengl3 is built; do not rebuild the game for it.

## Report
Files changed, the saws' cells per world, the script, the screenshot paths, ctest result.
