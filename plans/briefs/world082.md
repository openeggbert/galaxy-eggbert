# World 082 — mission 2 of world 8 (the mine shaft works)

Built by `tools/worlds/world082.py` → `worlds3d/world082.vwr` (mission 82, region 11 as the
original). Staged variants for `levelcheck.py`: `--stage K out.vwr` (K treasures taken: door "2"
open from K ≥ 2; from K ≥ 1 no moving or hanging mines, the shuttles as floors over their runs).

**Reference** (`worlds/world082.txt`, video 6:11:27-6:14:27): a machine of panels and pipes under a
dark sky. From Blupi's start in a room walled with pipe, a lift (60 ticks) up and a corridor of
four small shuttles (20 ticks) to treasure 1; a shaft where two lifts (70 ticks) meet midway
through a swarm of twenty moving mines (16) to treasure 2; four lifts meeting in pairs across the
sky (50 ticks) to door "2" (422); a gallery of six treasures, each under a hanging mine (3),
shuttles over lava (30 ticks), moving mines bobbing in its floor; a tower of three lifts (40
ticks) flanked by hanging mines and mines to the red arrow. Eight treasures, one egg, thirteen
hanging mines, four mines.

**3D design**: the climb runs south to north.
- The start room (x 10–20, z 70–80, walk 10): spawn (12, 10, 78) facing −Z, pipe along its walls;
  a lift (2 × 2) in its north-east corner up to the corridor.
- The shuttle corridor (walk 20, two wide, along X): four gaps across it, each bridged by a pair
  of shuttles in step; treasure 1 at its east end.
- The shaft (x 48–56, z 66–74, walk 20 to 40): one lift rises from its floor, another sinks from
  the top ledge, they meet level side by side at walk 31 (Blupi steps across); twenty moving mines
  drift round their way on short beats. Treasure 2 on the top ledge; the way out east through the
  wall.
- The relay in the open sky (walk 40, z 66–67): two pairs of lifts each meeting side by side
  midway (the first five cells, the second four, the same ticks), a rest between the pairs, a
  landing at door "2".
- The gallery (x 80–88, z 29–64, walk 40, along −Z): door "2" at its south end, an egg by it; a
  lava pit across it crossed on two shuttles along Z with a rest between them; six niches in its
  west wall, a treasure in each under a hanging mine (walk in, never jump); three pits across the
  floor, two mines bobbing up out of each; a last gap and a mine before the tower.
- The tower (walk 40 to 58): three lifts stacked in one column beside a shelf of the north wall two
  above each top (the upper shelves float), hanging mines over the shelves' ends, a mine on two.
- The arrow's room (walk 58): a gap two wide across it bridged by a shuttle, a mine beyond, the
  red arrow.

**Checks**: `levelcheck.py` stage 2 reaches all eight treasures, the egg and the arrow; stage 1
and the real world stop at door "2". `VerifyShippedWorlds`. Live (`--mission 82`,
spawns in each part, direct view F3): the start room, the corridor's shuttles, the relay over the
sky, the gallery's lava and shuttles, the niches under their mines, the bobbing mines, the tower,
the arrow's room. Played through by `tools/autoplay/world082.txt` (all eight treasures,
the egg, the arrow).

2026-10-27: the lifts' dwells raised from the original's 3-20 ticks to 30-40 (a step from lift to
lift or ledge in 3D needs a second or two), and the relay's lifts made two cells along X (one cell
was too small a target to stop on: walking across, Blupi went over it into the sky). Played live:
riding A out of the wall and stepping onto B where they wait side by side, B carrying him to the
rest.

The arrow room's shuttle runs within its gap, z 17 to 16 (its ends had lain in the floor's cells
at z 18 and 15: Blupi stood on the floor there and the shuttle slid away from under him; found
by the playthrough).
