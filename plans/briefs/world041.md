# Mission 041 — the helicopter caves (the cheese)

Built by `tools/worlds/world041.py` → `worlds3d/world041.vwr` (mission 41, region 6 as the
original: dark rocks). `--check STAGE out` writes the staged variants (below). World 4's tiles
and pieces: `tools/worlds/cheese.py`.

## Reference

`worlds/world041.txt`, played at 1:35:05–1:37:22 of the video (0/7 → 7/7 at 1:37:22). The first
cheese mission, and the helicopter's: Blupi starts in a cellar beside a house and a hut (BigDecor
26, 27) among plants; he pushes one crate (12) onto the other to make a stack two high, climbs
it to an island where three helicopters (13) stand, and flies up through the cheese to
treasure 1; the treasure door "1" (421) opens with it. A winding tunnel leads to a cave whose
ceilings and ledges hang with mines (3): five treasures and three eggs on its ledges, two
mines (2) lying beside treasures, nine helicopters on its floor. The door "6" (426) at its top
opens with six treasures onto a corridor over water (91/92) where three moving mines (16, 50
ticks, no stop; two in step, one in counter-step) rise and fall between a row of mines hanging
from the ceiling and the water; at its far end the seventh treasure, two eggs under a hanging
mine, helicopters, and a shaft up to the red arrow (7). 21 helicopters in all, so one is always
at hand. The original's helicopter is lost if it touches water (Decor.cpp `ByeByeHelico`; in
Galaxy since 557a694 a vehicle that meets water is wrecked).

## 3D design

Caves carved round in three masses of cheese (246–249), joined only by the route; only the
cellar opens to the outside. Raw cells, Y up.

| Part | Where | What |
|---|---|---|
| A, the cellar | x 21–43, z 66–86, walk 10, eight high under a dome | Open to the south on the dark rocks (a parapet one high). Spawn (36, 10, 80) facing −Z; the house at (40, 10, 70), the hut at (40, 10, 77), plants. The shelf along the north wall (x 22–30, z 66–69, standing level 14: four above the floor) with three helicopters. Crate A on the floor at (28, 10, 78); crate B on a bench one high along the west wall (x 22, z 71–77) under a low overhang (y 14–16): B is pushed north off the bench's end onto A at (22, 10, 70), under the shelf's west end. One crate is no stair (three up), nor is B on the bench (the overhang leaves no room for a full jump) |
| B, the chimney and the loft | chimney radius 2.3 above the shelf, y 14–28; loft x 20–36, z 57–71, walk 28 | Fly up the chimney; treasure 1 on a ledge three above the loft's floor in its west side, (23, 31, 62); door 1, two cells of 421, at (37, 28, 64) in the east wall |
| C, the tunnel | (40, 28, 64) → (44, 28, 64) → (50, 28, 58), five wide and five high | Walked; a drop of cheese from the vault with a mine hanging at head height at (42, 29, 62), to pass by the south side; the mouth opens six above the great cave's floor |
| D, the great cave | cylinder radius 12 round (60, 58), floor walk 22, walls to y 41, dome above | Nine helicopters on the floor; five ledges: south-west (top 26) treasure and egg, a mine hanging under it; south-east (30) treasure with a mine beside it; east (33) treasure and egg; north-west (35) treasure with a mine beside it; the middle ledge on a column (31) treasure and egg; mines hanging under the ledges and from the dome. Door 6 (426) at (60, 38, 45) over a landing (x 58–62, z 46–48, walk 38) in the north wall |
| E, the corridor | x 59–61, z 18–44, landings walk 38, water y 36–37 (z 22–39), air to y 44 | South landing with six helicopters; mines hanging from the ceiling along it; three gates (z 35, 30, 25) of three moving mines across the corridor rising and falling between y 38 and 43 (50 ticks, no stop; the middle gate in counter-step), so each gate is a moving barrier as the original's one-cell corridor made each mine; the water two deep under it (a fall wrecks the helicopter; a dive passes under the gates, as in the original's deep row) |
| F, the far landing and the exit | z 18–21; shaft radius 2.3 round (60, 19.5), y 45–51; chamber walk 52 | Treasure 7, three helicopters, an alcove with two eggs either side of a mine hanging at head height (crouch); the shaft up to the chamber with the red arrow at (60, 52, 16) |

The 2D corridor's three moving mines become three gates of three (nine) across the 3D
corridor's width; everything else keeps the original's counts (7 treasures, 5 eggs, 21
helicopters, 2 crates, 2 lying mines; 15 hanging mines for its 16).

Walking tunnels are five wide and five high and the cellar eight high: in a three-high tunnel
a jump took the chase camera into the ceiling (the screen showed only the background).

## Checks

- `levelcheck.py`: nothing beyond the cellar (no vehicles modelled). `flightcheck.py` (3d44172,
  helicopters) in stages: the world itself and `--check onecrate` (A under the shelf, B one push
  short) — nothing beyond the cellar; `--check stacked` — treasure 1 only (door 1 closed);
  `--check door1` — also the five cave treasures, door 6 closed; `--check door6` — everything,
  the far landing by diving under the gates (levelcheck walks the pool's floor), flight through
  the gates blocked (a flight path from the south to the far landing: none); `--check timed`
  (moving mines left out) — the far landing and the exit in flight.
- `VerifyShippedWorlds`. Objects stand on floors, hanging mines under ceilings, nothing sunk
  into the cheese, the cave and the corridor closed to the outside (scratch checks).
- Live (`--mission 41`, `--spawn`, probes through `QA_LIVE_WORLDS`): the start view (house,
  hut, crates, the opening on the dark rocks); crate A pushed west and north along the bench;
  crate B pushed off the bench onto A, the stack climbed, the shelf reached, a helicopter taken
  and flown up the chimney into the loft; treasure 1 taken on its ledge and door 1 open, the
  tunnel walked past its hanging mine; a cave helicopter flown up to the south-west ledge
  (treasure and egg); door 6 seen from its landing with the corridor beyond; hovering low over
  the corridor's water without a wreck, the gates rising and falling, the flight to the far
  landing and up the shaft into the exit chamber; with no treasures left, walking into the red
  arrow wins and returns to the hub.
