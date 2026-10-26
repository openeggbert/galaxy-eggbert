# World 063 — mission 3 of world 6 (the slime works)

Built by `tools/worlds/world063.py` → `worlds3d/world063.vwr` (mission 63, region 13 as the
original: the red machine wall). Staged variant for `levelcheck.py`: `--stage 1 out.vwr` (no mines
in the corridor, which the skateboard clears; the beam's saws a floor, as after the switch; the
basin shuttle a bridge; the pistons floors).

**Reference** (`worlds/world063.txt`, video 5:44:39-5:48:13): a building site inside the slime, on
the red machine wall of region 13: girders (201), riveted metal blocks (78-84), machines (87, 89,
90), grates (200). From Blupi's start a shuttle of three lifts carries him over a pit past a
floating mine (16); an elevator (72 ticks, twelve up) on a girder column; a bowl lined with metal
steps, water in it, crossed under a girder bridge by a shuttle of six lifts (130 ticks); in the
bowl a tree of metal tiers on a girder trunk with six eggs; the skateboard, then six mines in a row
on a mound, which it rides over and blows up; a tower of arms with saws (378) and mines, the
switch (384) at its foot stopping the saws of its row (`Decor::ActiveSwitch`: ±20 cells along its
row); a trap (373) before an egg; a shaft lift (96 ticks, sixteen up) with a trap under it; a
gallery of four lifts pumping one cell (6 ticks) between striped poles (235) under green knobs
(410), a trap at its end; a last lift to a girder tower, saws and mines on its arms, the red arrow
in a cage at its top. Seven eggs, no treasures.

**3D design**: a route through four parts of the slime mass, each floor two above the last on the
towers (Blupi jumps two).
- A, the start pocket (x 42–50, z 86–95, walk 11): spawn (46, 11, 90) facing −Z, the pit in front.
- B, the pit (z 77–85, three below): a shuttle two wide and three long (60 ticks, 10 dwell)
  carries Blupi north; the floating mine waits over one of its lanes, so he rides the other. A fall:
  a step and a jump back up into A.
- C, the landing, and the elevator (2 × 2, twelve up) in a metal shaft, its door on C's side, to the
  ledge E (walk 23) over the basin.
- The basin (x 32–62, z 43–58, rims walk 20, the hall to y 32): a pool (surface 13), the west side
  terraced in metal from the bank (walk 14) up to the west rim, the north and east sides sheer. The
  shuttle (2 × 3, 130 ticks, 10 dwell) crosses along Z from the south rim to the north rim under a
  girder gantry hung from the roof (y 26, clear of every jump). Beside its way the tree: four metal
  plates two apart (walk 22, 24, 26, 28) on a girder trunk standing in the pool, its lowest plate
  touching the shuttle's east lane; six eggs on the plates' rims, one at the top. Only a jump off
  the passing shuttle reaches it; a fall is a swim and the terraces back up.
- The machine yard (x 40–58, z 26–37, walk 20): machines (89, 84, 87, with 90 on top) against its
  walls, grates under its ceiling, the skateboard.
- The mine corridor east (two wide): six mines across it, three deep, too long to jump; the
  skateboard rides through them.
- The saw yard (walk 20) and the tower in a pit (walk 14, steps back up). The beam (one wide along X,
  walk 20) leads from the yard to the tower's metal core, three saws in its floor (too long to
  jump); the switch in the yard's floor at the beam's start, in its row, stops them, and a stopped
  saw is a floor. Arms two wide spiral up the core, each two above the last (north, east, south,
  west, north), saws on the far ends of two, mines on two, to the ledge L (walk 32).
- On L a nook behind a trap: the seventh egg. The shaft: three lifts in a row (96 ticks), sixteen
  up, spikes two under their low stop.
- The gallery (walk 48, two wide) west: striped poles and knobs in niches of its north wall, a
  trench two deep with four pistons (6 ticks, 3 dwell) pumping between the floor and one under it
  in turn (a fall: a jump back out at its ends), a row of traps before the exit tower.
- The exit tower: a lift (four up) beside a metal core, four arms round it each two above the last,
  a saw and a mine on the far ends of the first two, and on the last the cage (grates, a girder
  roof) with the red arrow (walk 62).

Original pieces kept apart from walking cells: the girders, grates, poles and knobs are decor
(empty collision masks, D41), on the gantry, the trunk, the masts, the cage and in niches. The
switch and the beam's saws lie in the floor, as world 2's (023, 025).

**Checks**: `levelcheck.py`: stage 1 reaches all seven eggs and the exit; the real world stops at
the mines (the skateboard is not modelled) and counts the tree's eggs only through its lenient lift
stops. `VerifyShippedWorlds`. Live (`--mission 63`, spawns in each part, direct view F3): the pit
with the shuttle and the floating mine, the elevator's door, the basin from the north rim with the
shuttle and the tree, the yard and the skateboard, the corridor's mines, the saw yard, the gallery
and its pistons, the exit tower; a run across the piston trench (jumps from piston to piston). A
probe with the beam's saws switched off (379) showed them drawn as still plates, which led to D42.
