# Mission 045 — the cheese mountain under the red curtains

Built by `tools/worlds/world045.py` → `worlds3d/world045.vwr` (mission 45, region 3 as the
original: red curtains). `--check STAGE out` writes the staged variants (below). World 4's tiles
and pieces: `tools/worlds/cheese.py`.

## Reference

`worlds/world045.txt`, played at 1:55:59–2:05:06 of the video (0/11 → 11/11), the longest
mission of the world. On foot first: beside a house and a tree a lift up to treasure 1 and
eggs; three lifts sliding over lava; three little lifts popping up a cell under hanging mines
(3) — 5 ticks up and 22 down, the middle one 6 and 6 — then the treasure door "1" (421); four
lifts at four speeds (50, 70, 40, 20 ticks; spans 5, 7, 4, 2) to hop across over a pit of mines;
three lifts in step (40 ticks) carrying Blupi over a gap where the bulldozer (4, 180 ticks)
drives; seven helicopters (13) beside a treasure between two mines and an invert (40). Then
flying: a machine cage (10, with 22 gratings, 55/56 props, a 76 pillar) with a treasure inside
and crushers (317) before it and before the red arrow (7); a cellar of lava pools (68) with six
treasures and nine helicopters; a pool (91/92) with eggs at its bottom (the helicopter is lost
in water); walk-through cheese (339) to an attic with the crate (12) and two treasures and to a
pocket of eggs. 11 treasures, 17 eggs, 21 helicopters, 18 lifts, 10 mines, 4 hanging, 7 inverts.

## 3D design

The on-foot half runs east from a plateau under the curtains into the mountain; the flying half
is the mountain's core, a cavern whose walls hold the other places. Raw cells, Y up.

| Part | Where | What |
|---|---|---|
| S, the plateau | x 8–32, z 50–92, walk 20, open to the curtains | Spawn (22, 20, 80); the house, trees, a plant. A ledge six high in the west corner with treasure 1 and three eggs, its lift (50 ticks) jumped onto from the floor. A lava moat (z 63–69) crossed on a raft of lifts in step, three wide and two long, at the original's speed (60 ticks for six cells) and waiting a second and a half at each bank; two eggs beyond; an invert |
| P, the gallery | x 33–44, z 55–57, walk 20 | A strip of lava; over it a line of stones one wide and, between them, three pads (lifts sinking a cell below the line and popping back flush with it, 5/22 and 6/6 ticks, a second's rest at the top); a mine hangs in each pad's column at Blupi's height: crossed crouching. Door 1 (421) at (45, 20, 56) |
| H, the hop hall | x 46–62, z 52–60, pit floor walk 10, ledges walk 20 | The four lifts at the original's ticks and spans as a chain of lift columns with rests between them at their meeting heights: L1 down from the entry ledge to rest 1 (walk 15), L2 up to rest 2 (22), L3 down to rest 3 (18), L4 up to the exit ledge. Mines on the pit floor, a stair back up to the entry ledge, an egg on the exit ledge |
| B, the ferry and the road | x 63–87, z 55–57, walk 20 | Three lifts in step (40 ticks, two seconds' wait at the ledge) over a pit seven long and four deep (a stair in it back up after a fall); a mine hanging over it. The road beyond: the bulldozer drives its middle lane (180 ticks); at two pinch points mines lie in both side lanes, passed in its lane while it is away; an invert |
| G, the hangar | x 86–96, z 46–64, walk 20 | Seven helicopters; along its north side a mine, an invert, treasure 2, a mine; a machine (BigDecor 29); its north side open onto the balcony over the core |
| K, the core | x 56–92, z 10–40, floor walk 8, up to y 56 | The balcony under the hangar's opening (walk 20) |
| R, the cellar | the core's floor | Two lava pools, six treasures (one behind a row of crushers), nine helicopters, two eggs, a mine, an invert, machines (BigDecor 28, 30); in the east wall walk-through cheese in front of a pocket with three eggs |
| C, the cage | on a column in the middle, walk 44 | Machine walls three high, open above (the camera looks in), gratings (22) in the west and east walls; on each landing a row of crushers a cell short of the grating; treasure 3 inside with a pillar and props |
| X, the exit | the core's west wall, walk 36 | A landing with a helicopter and an invert; the corridor west with a row of crushers, the red arrow at its end |
| W, the pool | the core's north wall, walk 26 | A pool two deep with three eggs at its bottom (a dive; the banks one above the water), four helicopters |
| A, the attic | high in the west wall, walk 52 | Behind walk-through cheese: treasures 4 and 5 (one on a ledge three high), three eggs, the crate to push under the ledge |

3D decisions: the pads never rise above the line of stones and the hop lifts have rest columns
between them (a lift does not stop Blupi sideways, and stepping straight from lift to lift needs
them level at the cell boundary); the ferry's gap is a pit with a way back, not a death; the moat's raft and the ferry wait
longer at their landings than the original's half second, which in 3D is a blind step. The
original's eleven treasures, seventeen eggs, twenty-one helicopters and eighteen lifts are kept;
ten mines lying (four of them the road's pinch points), four hanging, six inverts.

## Checks

- `flightcheck.py --mine-margin 0` in stages: the world itself and `--check door1` reach only
  the plateau, the gallery and the hop hall's entry (levelcheck does not jump onto lifts, and the
  pads' mines and the bulldozer are in the way); `--check timed` (the bulldozer and the pads'
  mines left out, a step beside the plateau lift): OK — treasures 1, 2 and the cellar's six on
  foot (the balcony drops into the core), the cage, the attic's two and the exit with a
  helicopter; `--check solved` (also the crate under its ledge): OK. From every treasure the way
  back to the arrow exists with a helicopter (scratch check).
- `VerifyShippedWorlds`; objects on floors, hanging mines under ceilings, nothing sunk into the
  cheese (scratch check).
- Live (`--mission 45`, `--spawn`, all lift checks with the authored lift timing of 3b0a861):
  the start view under the curtains; the moat crossed on its raft; the pads crossed crouching under their
  mines (walking, a pad lifts Blupi into its mine and he dies, as meant); the hop hall's chain
  ridden from the entry ledge down L1, onto rest 1, up L2 and on to the exit ledge; the ferry
  ridden to the road (boarded in its wait at the ledge; a step while it is away drops into the
  pit); a hangar helicopter flown out over the balcony and down into the core; the cage walked
  into past its crushers (Blupi squashed, then through the grating) and its treasure taken; the
  exit corridor's crushers; the egg pool dived into; the attic's walk-through wall.

2026-10-27 (`tools/watercheck.py`, world 7's agent's measurement of the weak jump out of the water): the grotto's pool's water now reaches its banks' top (it was a block below and trapped a Blupi who dived for the eggs); climbed out live, the staged checks unchanged.
