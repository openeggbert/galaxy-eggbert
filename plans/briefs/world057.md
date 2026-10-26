# Mission 057 — the slime tree (world 5, region 10)

Built by `tools/worlds/world057.py` → `worlds3d/world057.vwr` (mission 57, region 10 as the
original: the dark blue hexagons); `--stage K out` writes the staged variants for `levelcheck.py`.

## Reference

`worlds/world057.txt`, played at 5:08:28–5:14:58 of the video (`build-probe/world5-agent/video/w057/`;
0/3 → 3/3 at 5:13:35). A station of nineteen enemy helicopters (ObjectType32: each hovers a cell
to and fro, 8 ticks, stop 25, and drops a shot straight down at every turn) and three enemy tanks
(ObjectType33): from the start a gallery under seven helicopters in a row; two lifts stacked in a
shaft (60 and 40 ticks) past three more, eggs on the ledge between; a corridor with a tank; a hall
with two treasures on a pillar of slime under two helicopters, a lift either side; the treasure
door "2" (422); a corridor with a tank to a ferry of six lifts over water, each pair meeting the
next (100 ticks, stop 20), a tank on the far shelf; up to the drink (30); a gallery above the last
room, where the third treasure and the red arrow stand on a slime tree under seven helicopters,
with a lollipop (26) for the jump onto it. 3 treasures, 10 eggs, 19 helicopters, 3 tanks, 10 lifts,
the drink, the lollipop.

## 3D design

Raw cells, Y up; the route runs north, up, west, then back east above.

| Part | Where | Lesson |
|---|---|---|
| S, the start | x 73–81, z 78–86, walk 10; spawn (77, 10, 84) facing −Z | Ball machines |
| G, the gallery | x 76–78, z 62–77, walk 10, roof at 17 | Seven helicopters over the middle lane at y 14, each swinging a cell across: their shots fall on every lane in turn; walk between them |
| E, the lift shaft | x 73–83, z 50–60 | L1 rises six (60 ticks) to the ledge (walk 16) with four eggs and a helicopter over each end, L2 four (40 ticks) to the top platform (walk 20); a helicopter in the shaft beside L1 |
| C2, the corridor west | x 62–72, z 50–52, walk 20 | A tank swinging across it, firing across at each turn |
| H, the hall | x 46–60, z 44–56, floor walk 14 | From the east balcony (walk 20) a jump across one cell onto the slime pillar (top walk 20): treasures 1 and 2, two helicopters over it; the west ledge; two lifts from the floor (60 ticks) up to the balcony and the ledge after a fall; the door "2" in the west wall at the floor |
| W, the corridor | x 34–44, z 54–56, walk 14 | A tank swinging across it, girder posts |
| F, the ferry | x 2–33, z 48–59 | The water pit wall to wall three below the shelves (a bank at its east end to climb out); three pairs of lifts (2 × 3) all 100 ticks: A from the east shelf west, B east to meet it, C west to the far shelf from where B starts; they wait 40 ticks where they meet (the original's 20). A tank on the far shelf firing along the lanes; two eggs |
| The stair, D | stair x 3–5, z 40–45 (six steps of two, walk 14 → 26); D x 2–10, z 30–37, walk 26 | The drink and an egg |
| The gallery east | x 11–47, z 32–34, walk 26 | Girder colonnade; at its end a ledge high in the tree room, then the drop |
| X, the tree | x 46–60, z 28–42, floor walk 14 | A slime pillar (x 51–55, z 34–38) four above the floor: treasure 3 and the arrow on it between slime bumps, seven helicopters over it; the lollipop in an alcove in the east wall with two eggs: licked, the jump clears four |

Objects as the original's: 3 treasures, 10 eggs, 19 helicopters, 3 tanks, the drink, the
lollipop; 22 lifts (the original's 10, as pairs and platforms).

## Checks

- Staged `levelcheck.py` (`build-probe/world5-agent/stagecheck.py 057 0 … 3`; 1 the helicopters
  and tanks passed, 2 the door "2" open, 3 the lollipop's jump (a stand-in lift)): the ledge's
  eggs; the hall's treasures; the ferry, the drink and the tree room's eggs (levelcheck links the
  ferry's lifts where their ends touch); everything and the exit. Without the ferry the far shelf
  is unreachable (a first build had floor round the pit's sides: the pit is now wall to wall).
- `VerifyShippedWorlds worlds3d`: 80/80.
- Live (`--mission 57`, probes through `QA_LIVE_WORLDS`, `build-probe/world5-agent/shots/057-*`):
  views of every part; the gallery's shots falling, a careless run killed; the jump from the
  balcony onto the hall's pillar and treasure 1 taken (1/3); the ferry step by step: boarding A
  at the east shelf, A meeting B and the step across, B and C side by side at their starts and
  the step onto C, C to the far shelf and the step off (after the dwell was lengthened; the
  scripted runs drift over a 14-second cycle, so the steps were checked one at a time); the
  lollipop licked and the jump onto the tree; the arrow on the tree ends the mission (probe
  without treasures).

2026-10-27 (`tools/watercheck.py`, world 7's agent's measurement of the weak jump out of the water): the ferry pit's bank is now level with the water (it stood a block above and trapped a Blupi who fell in); climbed out live, the staged checks unchanged.
