# Mission 055 — the dark works (world 5, region 29)

Built by `tools/worlds/world055.py` → `worlds3d/world055.vwr` (mission 55, region 29 as the
original: dark machinery); `--stage K out` writes the staged variants for `levelcheck.py`.

## Reference

`worlds/world055.txt`, played at 4:58:04–5:02:41 of the video (`build-probe/world5-agent/video/w055/`;
0/7 → 7/7 at 5:02:07; a death in the lava at 5:00:39). Dark machine plates (78, 2 267 of them)
round a chain of shafts and corridors, seven treasures behind the treasure doors "2", "4" and "7"
(422, 424, 427): the start shaft, where a wasp's balloon carries Blupi up to two treasures in its
walls and into the corridor to the door "2" before he meets the mines hanging under its roof; two
more balloon shafts, past hanging mines, a block with mines under it and eggs on top, and two
mines sweeping across (30 ticks); a corridor of mines with two invert pickups (ObjectType40: the
controls turn round for a while) and two treasures, the door "4"; a shaft climbed down ledge to
ledge past mines, a treasure; three lightning curtains (304/305); lava (68) crossed on overcrafts
(ObjectType46, ten of them), a lift (40 ticks, stop 20) up to a treasure, the last treasure, the
door "7"; a pool the overcraft dislikes, and the red arrow. 7 treasures, 3 eggs, 3 wasps, 17
hanging mines, 15 mines, 3 moving mines, 2 inverts, 10 overcrafts, 1 lift.

## 3D design

Rooms of dark plates joined in a chain, stepping up north through the balloon shafts, east along
the inverts' corridor, down the ledge shaft, north through the lightning, west through the lava
hall and down into the pool hall. Raw cells, Y up.

| Part | Where | Lesson |
|---|---|---|
| A, the start shaft | x 20–26, z 68–74, walk 10, roof at 24; spawn (23, 10, 74) facing −Z | A wasp sweeps its floor (12 ticks, stop 4); its balloon rises (faster with jump held, held at a height with crouch) and drifts where Blupi walks. Two treasures in niches three high in the west and east walls four above the floor; the corridor north out of the wall at walk 18; mines hanging under the roof above it |
| The corridor | x 22–24, z 59–66, walk 18 | Three mines across it to jump; the door "2" |
| D0, L | drop x 22–24, z 55–58 to walk 4; low corridor z 45–54 | Down to the next shaft's foot |
| B1 | x 20–26, z 38–44, walk 4 up to 14 | A wasp at its foot; low, four corner ledges with mines hanging under them (rise in the middle); higher, a block in the middle with three mines under it and two eggs on top (drift to a side); the mouth north at walk 14 |
| B2 | x 20–26, z 25–31, walk 14 up to 24 | A wasp; two mines sweeping across at two heights (time the rise); mines hanging under the roof except over the mouth east at walk 24 |
| C, the inverts' corridor | x 28–55, z 27–29, walk 24 | Narrowed to one cell at x 30 and x 52, an invert lying in each narrows; nine mines staggered across the lanes; treasures 3 and 4; the door "4" |
| D, the ledge shaft | x 57–63, z 25–31, walk 24 down to 12 | From the landing inside the door a spiral of ledges round the walls, each two below the last, meeting at the corners; mines where a careless step lands; treasure 5 on the south ledge |
| E, the lightning | x 58–60, z 9–24, walk 12 | Three curtains two deep across the corridor (emitters in the roof, 2.5 s on, 2.5 s off), mines between them |
| F, the lava hall | x 30–56, z 6–14, walk 12, roof at 17 | Five overcrafts by the door; lava across the hall (x 38–48): on foot it burns him, ridden it crosses while Up holds it hovering; treasure 6 on a shelf three up in the south-east corner, reached by a lift (20 ticks a cell, stop 20); treasure 7 beyond the lava; the door "7" |
| G, the pool hall | x 8–24, z 4–16, walk 6 | Dropped into from the passage; five overcrafts on the east bank; a pool across the hall one deep, level with the floor (an overcraft that meets it is lost); an egg; the arrow on the west bank |

## Checks

- Staged `levelcheck.py` (`build-probe/world5-agent/stagecheck.py 055 0 … 7`; 1 the balloons
  (stand-in lifts), 2 the floor mines passed, 3 the door "2", 4 the door "4", 5 the lightning
  timed, 6 the lava crossed (a floor), 7 the door "7"): the two niche treasures, then +2 and the
  eggs, +treasure 5, +treasure 6, +treasure 7, then everything and the exit. (Single-cell lightning
  curtains could be jumped through in levelcheck's model: two deep now.)
- `VerifyShippedWorlds worlds3d`: 80/80.
- Live (`--mission 55`, probes through `QA_LIVE_WORLDS`, `build-probe/world5-agent/shots/055-*`):
  views of every part; the start wasp's balloon steered into the west niche (treasure 1/7; the
  first niches, two high, were missed: now three high and five wide) and up into the corridor
  mouth; the invert taken in the narrows and Up walking Blupi backwards; the lightning; the
  overcraft over the lava with Up held, to the door "7" (after the engine's overcraft fix D46:
  before it the overcraft flew freely — reported; let go over the lava it sinks and he burns);
  the shelf lift ridden and stepped off onto the shelf (1/7); the pool swum and climbed out to
  the arrow (refused at 0/7).
