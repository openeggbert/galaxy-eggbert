# World 061 — mission 1 of world 6 (the slime chasm)

Built by `tools/worlds/world061.py` → `worlds3d/world061.vwr` (mission 61, region 31). Staged
variants for `levelcheck.py`: `--stage K out.vwr` (K treasures taken: the doors needing at most
K open, the lifts the next treasures need as floors, at K ≥ 5 no creatures).

**Reference** (`worlds/world061.txt`, video 5:22:13-5:32:28): a cave of green slime round Blupi's
start; the red arrow beside him, counting only once all 7 treasures are taken. Treasure doors
(421 + n − 1 opens with n treasures; here 421, 422, 424, 426 and two 427) gate the ways round
it. Sixteen lifts: three shuttles stacked a jump apart and moving in step; a chain of eight across
the top, some moving across and up at once; a tall vertical lift. Two big creatures (54) walk 4-5 s
and rest 10 s at each end: harmless walking, deadly resting. Lava pits crossed on machine blocks
(10), a small pool, five mines, a moving mine, fourteen eggs. The slime tiles marked 347 in the map
are plain slime (only 373 is the spike trap): a fall costs the climb back, not a life.

**3D design**: rooms of slime round the start chamber, shells two thick.
- S, the start chamber (x 43–57, z 43–57, walk 15): spawn (50, 15, 54) facing −Z, the red arrow at
  (50, 15, 45) in view; an egg cache in its north-east corner behind a seven-treasure door.
- West, through a corridor: the shaft A (x 25–36, z 42–58), three shuttle pairs along Z (z 45–55,
  24 ticks, 3 dwell, all in step) at walk 17, 19 and 21, each one column further west, then the
  ledge at walk 23 with treasure 1. Each step up is a two-block jump onto the neighbouring column.
- Out of the ledge's north end behind door "1" (421): the chasm P (x 24–76, z 21–40). From the
  landing G (walk 23) two shuttles meet in the middle (18 ticks, opposite phases) to island I1
  (treasure 2); a lift moving across X and Z at once to island I2 (treasure 3, a mine in its far
  corner); a lift rising as it goes to the east ledge E (walk 25). The chasm's floor is twelve
  blocks down; a stair of two-block steps beside G leads back up.
- On E the tower lift (36 ticks) up to the tower room (walk 33): treasure 4.
- E's south doorway behind door "4" (424): a ledge over the lava hall H (walk 15), two lava trenches
  across it (one a jump wide, one two wide crossed on machine blocks), treasure 5, two mines.
  H opens into S behind door "5" (425).
- S's south door "5" (425): a stair of two-block steps down to the creatures' halls (walk 9). From
  the landing a corridor one wide runs west; creature 1 (80 ticks, 200 rest) walks it and rests at
  both ends, blocking the doorway and the chamber of treasure 6 while it rests; Blupi passes it while
  it walks. The chamber opens south into the lava room (machine blocks to step on, a pool, a moving
  mine), whose far side leads east along creature 2's corridor (100 ticks, 200 rest) to treasure 7.
- Then back up the stairs to the arrow.

**Checks**: `levelcheck.py` on the stages: K=0 treasure 1 only; K=1 treasures 1-4; K=4 adds 5;
K=5 all seven; K=7 every egg (the cache). The real world fails where lifts and creatures stand in
(levelcheck does not ride chained lifts). `VerifyShippedWorlds`. Live (`--mission 61` and spawns in
each room): the start view with the arrow; the shuttles moving in the shaft; the chasm from G with
its islands and lifts; the lava hall with its bridge; the creature corridor from the landing.
Not scripted live: a full climb of the shuttle ladder and a ride along the lift chain (timing).
