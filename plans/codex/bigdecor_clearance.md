# Task: no solid BigDecor in a way a player must take (Galaxy Eggbert)

Repository: /rv/data/development/github.com/openeggbert/galaxy-eggbert (develop). Read `AGENTS.md`,
`../AGENTS.md`, `CLAUDE.md` and `plans/codex/README.md`. Recommended: `gpt-6-astra`, effort high.

## Hard rules
- Never modify anything outside this repository; never read `../speedyblupi-data`.
- Commit when done (see `plans/codex/README.md`; no AI attribution; never push).
- Build only in `build-cna-opengl3/`; silent game runs; scratch under `build-probe/bigdecor-clearance/`.
- Change worlds only through their generators (`tools/worlds/worldNNN.py`, or the tool named in the
  world's brief `plans/briefs/worldNNN.md`), never by hand-editing a `.vwr`.

## Background
Since BD-0080 (`plans/audit/behavior_differences.md`, commit `b19db064`) the BigDecor standing in the
rooms as models are really solid in play (`kSolidBigDecorIcons`: palms, trees, plants, houses,
machines, toys, the palace pieces and the Blupi statue; shapes as their models; M3D-0006). Before, a
bug left them passable, so some were placed where a player must pass. Eleven playthroughs were
rerouted round them. Two of those reroutes squeeze through lanes no human could take:

- **world053**: the machine `BALL_MACHINES[1]` at (57, 30, 19) (`tools/worlds/world053.py` ~217)
  stands in the approach to the door "14" (east wall x 58, z 18-20); the script passes it on a lane
  0.13 cells wide with `tol 0.02` (`tools/autoplay/world053.txt` ~262-268).
- **world058**: the plant by the pillar perimeter near (60-62, 84-86): waypoints at `tol 0.05`
  (`tools/autoplay/world058.txt`, the helicopter approach and the pillar circuit).

## Do
1. For every campaign world, find solid BigDecor that narrow a way the player must or naturally takes
   to less than a comfortable width (say under 0.6 cell of free lane for Blupi's body, or blocking a
   door, a landing, a lift's stop, a ladder of ledges, a switch's front): use `tools/levelcheck.py`
   (it counts the boxes as solid), the BigDecor list of `python3 tools/vwr_inspect.py`, and the
   playthroughs' tight tolerances (`grep -n "tol 0.0" tools/autoplay/*.txt`) as leads.
2. Fix each in its generator: move the BigDecor a cell or two to where it still dresses the place
   (beside the way, against a wall, in a corner), keeping the world's look and the original identity;
   never delete one. Regenerate the world.
3. Restore comfortable routes in the playthroughs you can (undo the squeezes: 053's door approach
   straight again, 058's perimeter with normal tolerances) and run every affected playthrough and its
   hub chain to PASS; then a full play pass (78/78).
4. Record each move in the world's brief and in `plans/audit/behavior_differences.md` BD-0080's "Left"
   line (closed), with before/after screenshots of 053's door under `build-probe/bigdecor-clearance/`.

## Report
The places found, the moves, the playthroughs' PASS lines, the play pass result.
