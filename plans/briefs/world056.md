# Mission 056 — the valley of the tanks (world 5, region 1)

Built by `tools/worlds/world056.py` → `worlds3d/world056.vwr` (mission 56, region 1 as the
original: the red sky); `--stage 1 out` writes the variant without the enemies for `levelcheck.py`.

## Reference

`worlds/world056.txt`, played at 5:02:44–5:08:23 of the video (`build-probe/world5-agent/video/w056/`;
0/12 → 12/12 at 5:08:12, "enemy blue tanks"). Green slime (341–363) and riveted plates under the
red sky; nine enemy tanks (ObjectType33) patrol short beats and fire along their lines at each
turn, two enemy helicopters (ObjectType32) drop shots. From a slime cave with a drink (30) a lift
climbs ten cells (100 ticks) up a girder column to a metal level; two lifts side by side slide
five cells (30 ticks, stop 3) over a floor of spike traps (373) with a tank under them; then a V of
girder steps, six treasures down one side and six up the other, tanks guarding the steps of one
side, the red arrow at the bottom between two pillars. 12 treasures, 18 eggs, 9 tanks, 2
helicopters, 3 lifts, the drink.

## 3D design

Raw cells, Y up.

| Part | Where | Lesson |
|---|---|---|
| S, the slime cave | x 10–34, z 60–78, walk 10, walls five above the deck; spawn on the west shelf (12, 12, 76) | The drink beside the start (20 s unseen and untouchable); three tanks firing along their beats, two helicopters dropping shots; a pit of three eggs; slime mounds |
| The lift | (31, 9, 61) beside a girder column | Ten cells up to the deck (100 ticks, the original's; it waits 30 at each end — its 10 is half a second) |
| M, the metal deck | x 26–33, z 50–58, walk 20 | Two eggs |
| The spike lane | x 34–40, z 50–52 | Spike traps in its floor two below the deck, a tank driving along it under the platform; the sliding platform (2 × 3 lifts in step) five cells east and back (30 ticks, stop 20) |
| The islands | east platform x 41–46; I1 x 47–52 (walk 20); I2 x 53–58 (walk 22) with spikes and a tank; I3 x 60–68 (walk 20) with a tank | Eggs on each |
| V, the valley | z 64–72 between slime walls; east rim x 70–74, west rim x 35–38 (walk 20) | Seven steps down each side, each one lower (walk 19 … 13), the bottom (x 53–55, walk 12) with the arrow between two columns; a treasure on each of the six lower steps of each side; three tanks drive across the west slope's steps (W3, W5, W7) and fire along them; eggs on the rims and the steps |

## Checks

- `levelcheck.py`: every treasure, egg and the exit reachable, with the enemies (the check
  avoids their beats) and without (`--stage 1`); removing the long lift or the sliding platform
  makes the exit unreachable (each is on the only route).
- `VerifyShippedWorlds worlds3d`: 80/80.
- Live (`--mission 56`, `build-probe/world5-agent/shots/056-*`): views of the cave, the deck, the
  lane, the islands, the rim and the valley; the long lift ridden and stepped off onto the deck
  (once its dwell was lengthened); the sliding platform boarded from the deck, ridden over the
  spikes and left at the east end; the walk down the east slope and up the first west steps; the
  arrow at the bottom ends the mission (probe without treasures).
