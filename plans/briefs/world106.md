# World 106 — mission 6 of world 10 (the treasure wells)

Built by `tools/worlds/world106.py` → `worlds3d/world106.vwr` (mission 106, region 3 as the
original). Staged variant for `levelcheck.py`: `--stage 1 out.vwr` (no mines, hanging mines,
bulldozers or enemy Blupis).

**Reference** (`worlds/world106.txt`, video 4:20:05-4:27:48): sixteen treasures, twelve of them
stacked four by four in three narrow wells over Blupi's start (the exit beside him), each well
topped by a teleporter (330-333): he walks out through the caves round the start to another
teleporter, comes out over a well and drops through its four treasures back to the start. The
caves: a stair down with eggs and hanging mines (3); bulldozers (4) and lifts over lava; enemy
Blupis (32 firing down, 33 firing across) and mines (2); a block in the middle with two treasures
between mines reached by lifts; a high room with two more and two bulldozers; a passage walled
with the flight barrier (214) round three eggs. Seven eggs.

**3D design**: one mass of cobble.
- The atrium (x 40–60, z 40–60, walk 10): spawn (50, 10, 49) by the exit; a door in each wall; in
  its roof three wells (2 × 2) up to small rooms (walk 30), a teleporter's pillar in each: arriving
  there Blupi comes out over the well's mouth and drops through its four treasures into the atrium.
- West: a cave stepping down a cell at a time (walk 10 → 4), eggs on its steps, two mines hanging
  in its walking cells; at its foot the teleporter to the west well (330).
- East: the bulldozers' hall: two bulldozers pushing to and fro, a trench of lava crossed on two
  lifts meeting midway (30-tick dwells), a passage walled with the flight barrier round three eggs;
  the teleporter to the east well (331).
- South: the enemy Blupis' cave: a wall across it with a doorway, one enemy patrolling its top
  firing down, another walking firing across, two mines, a trench of lava crossed on a short lift
  (the original's bobbing lift made a ferry its ends meeting the floor); the teleporter to the
  south well (332).
- North: a hall with a block four high: two lifts up its sides, two treasures on top between two
  mines, the teleporter (333) to the high room (walk 36): two treasures, two bulldozers, a passage
  to a hole down into the hall.

**Checks**: `levelcheck.py`: stage 1 and the real world reach the four treasures off the wells,
the seven eggs, the exit and all three wells' arrival cells over their mouths (levelcheck counts
only where Blupi stands, so the twelve treasures he falls through show as not reached).
`VerifyShippedWorlds`. Live (`--mission 106`): from the west teleporter (8, 4, 49) left standing,
Blupi came out over the west well and fell through it: 4/16, standing in the atrium. Direct view
F3 at spawns in each part: the atrium and its exit, the stair's hanging mine, the bulldozer and
lift, the firing enemy on the wall, the north block, the high room. Not played through: the other
zones' walks.
