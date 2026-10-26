# World 066 — mission 6 of world 6 (the slime winds)

Built by `tools/worlds/world066.py` → `worlds3d/world066.vwr` (mission 66, region 31 as the
original). Staged variant for `levelcheck.py`: `--stage 1 out.vwr` (no mines, enemies or lightning
in the narrow ways, the trench filled, floors under the bars and the rods).

**Reference** (`worlds/world066.txt`, video 6:04:02-6:07:37): the last mission of world 6 ends not
at the red arrow but at the golden key (21, the secret exit); three eggs, no treasure. A strip of
slime passages: Blupi starts between two mines with an egg behind the western one; a curtain of
lightning (305 under 304), three big creatures (54) in the corridor, one taking a single step; a
blupit (33), a bulldozer (4), a lift over a gap, a trench of five mines and a crate crossed hanging
from bars (138); an upper corridor with traps (373), a still and a moving mine (16), an egg under
drips (404), teleporter 1 (330) at its west end; two wind tunnels (fans 126/129, wind 110/114), the
lower blowing east, the upper west, mines in both and a bulldozer in the upper; far east, over a
lava pit, thin rods (202) Blupi hangs from past a mine rising through them and a bird (20), then a
trap and the golden key; teleporter 2 beside the rods.

**3D design**: two storeys over each other, the tunnels over each other east of them, the east
cave.
- The start corridor (x 16–51, z 60, walk 20, one wide, three high): an egg, a mine, spawn (19, 20,
  60) facing −Z, a mine; the lightning curtain at x 24; the big creatures (100/200 x 27–32, 40/200
  x 35–36, 80/200 x 39–43; the corridor is high enough to jump the one that barely moves); a trap.
- The middle storey (walk 26), up a stair from the corridor's east end: B1 with the blupit shooting
  along it; west the bulldozer's corridor (one wide, jump it); the gap crossed on a lift (18 ticks);
  the trench of five mines with the crate at its end, the bars over it (a jump catches them, D45).
- The upper corridor (walk 32), up a stair from the trench room: traps to jump, a still mine and a
  moving one, the egg at its east end under two drips, teleporter 1 over its west end.
- The lower wind tunnel (walk 20), entered from the start corridor beside its fan, blows Blupi east
  over three mines (jump them in the wind) to an egg; on east two saws in its floor and a pole, a
  stair up.
- The corridor of lava cells (walk 26) under knobs; a side way to the upper wind tunnel, which blows
  west over three mines and its bulldozer back to the middle storey.
- The east cave (walk 32), up a stair: teleporter 2 on its west ledge; the lava pit from wall to
  wall; the rods across it to hang from, a mine rising and falling through their lane, a bird
  flying over; the far ledge, a trap, a pole, the golden key.

**Checks**: stage 1: `levelcheck.py` reaches the three eggs and the golden key; the real world
stops at Blupi's start between two mines (levelcheck keeps off the cells next to them;
`flightcheck.py --mine-margin 0` jumps them and stops at the lightning). `VerifyShippedWorlds`.
Live (`--mission 66`, spawns in each part, direct view F3): the start between the mines, the
lightning, the creatures, B1, the bulldozer, the trench with its bars, the upper corridor, both
tunnels, the lava cells, the east cave with teleporter 2 and the rods. Played live: hanging across
the rods to the far ledge and the golden key (with the rising mine and the bird left out of a
probe; with them in, a crossing must be timed).
