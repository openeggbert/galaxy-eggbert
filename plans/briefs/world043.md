# Mission 043 — the cheese warren

Built by `tools/worlds/world043.py` → `worlds3d/world043.vwr` (mission 43, region 21 as the
original: lava rock). World 4's tiles and pieces: `tools/worlds/cheese.py` (`cave`, `shaft`,
`tube`).

## Reference

`worlds/world043.txt`, played at 1:42:52–1:48:30 of the video (0/12 → 12/12, then back to the
arrow). A warren of narrow passages through cheese. Blupi starts beside the red arrow (7),
which counts only with all twelve treasures; two treasures near the start; then a long walk
down through winding passages, three mines (2) lying in narrow places to jump, more treasures,
to a cave where eight helicopters (13) stand in a row. He flies up past two mines hanging (3)
over the way, through the winding upper passages for the other treasures (the video: 5/12 at
the helicopters, then 6 … 12 in flight), finds a second row of eight helicopters, and flies back
down to the arrow beside the start. 12 treasures, 2 eggs, 16 helicopters, 3 + 2 mines.

## 3D design

One block of cheese (x 18–82, y 8–58, z 20–72) carved inside; only round windows in the start
cave look out on the lava rock. Raw cells, Y up.

| Part | Where | What |
|---|---|---|
| S, the start cave | round (30, 30), walk 40 | Spawn (30, 40, 33) facing −Z, the red arrow at (28, 40, 34) beside him; treasures 1 and 2 on ledges a jump up (west and north); a plant; windows high in the west and north walls |
| W, the walk down | tunnels five wide: east from the cave (walk 40), a drop south (37), a drop east (33), a drop north (29), a shaft down into the grotto's roof | Three cross walls with a slot one wide and five high, a mine lying in each slot (at (42, 40, 31), (47, 37, 44), (56, 33, 51)): jumped through. Pockets off the way: treasure 3 on a block in a pocket east of the first drop, treasure 4 and an egg in a pocket north of the east passage |
| G, the grotto | round (66, 35), walk 14 | Eight helicopters, treasure 5, an egg. In its roof the shaft up (radius 2.5) to chamber 1, a ring across it at y 27 leaving a hole three wide with two mines hanging under it at its edge |
| F, the flight passages | south side of the block | Chamber 1 (walk 34): treasure 6 on a shelf three high. A tube descending south-west to chamber 2 (walk 24): treasure 7 on its floor, treasure 8 in a niche high in its south wall. A chimney from chamber 2 up to the dome, treasure 9 in a side niche halfway. The dome (walk 46): the second row of eight helicopters, treasures 10 and 11 on ledges four and six high, an egg |
| R, the way back | the dome → north over the start cave | A tube from the dome's north side (five above its floor: a flight) over the start cave, treasure 12 in a pocket of its roof, then a shaft down into the start cave's roof: Blupi drops (or flies) down to the arrow |

## Checks

- `levelcheck.py`: only the start cave and the first passage (it keeps a cell round mines);
  `flightcheck.py --mine-margin 0`: OK — treasures 1–5, the eggs on the way and the exit on
  foot, treasures 6–12 with a helicopter; from the upper passage the walk drops to the arrow;
  from the dome without a helicopter the way back is closed. The block is closed to the outside
  apart from the start cave's windows (scratch check without them: closed).
- `VerifyShippedWorlds`; objects on floors, hanging mines under ceilings, nothing sunk into the
  cheese (scratch check).
- Live (`--mission 43`, `--spawn`, probe with extra helicopters through `QA_LIVE_WORLDS`): the
  start cave and the first passage with the slot's mine; the jumps through slots 1, 2 and 3;
  the drop down the shaft into the grotto with its helicopters; a grotto helicopter taken and
  flown up the shaft past the hanging mine (seen at the ring) into chamber 1 and its shelf; the
  walk off the upper passage's end, dropping down the shaft into the start cave toward the arrow.
