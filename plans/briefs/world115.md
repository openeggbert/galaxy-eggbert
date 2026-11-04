# World 115 — mission 5 of world 11 (the works' last round and world 11's golden key)

Built by `tools/worlds/world115.py` → `worlds3d/world115.vwr` (mission 115, region 22 as the
original). Staged variant for `flightcheck.py`: `--stage 1 out.vwr` (no mines or lightning, the
crates pushed into their steps).

**Reference** (`worlds/world115.txt`, video 0:59:15-1:06:15, golden key → main hub): Blupi starts
beside the golden key (21), which waits until the eleven treasures are found. About the blue
machine panel: eighteen helicopters (13) to fly up to treasures on high ledges, eleven skateboards
(24) to ride over rows of mines (2, eighteen), a row of six crates (12) to push into steps,
columns of lightning (305), pits of lava, a fish (17) in a pool, five eggs.

**3D design**: a hall with four wings, inside one mass of panel.
- The hall (walk 20): spawn (50, 20, 52); the golden key in an alcove off its south side.
- North, the helicopters' gallery (open up to walk 42): eight helicopters on its floor, five on a
  high ledge; an egg on a ledge of its walls at walk 26; two pits of lava. (Its treasures were on
  ledges at walk 30, 34 and 38 until 2026-11-04: they lie in the winds now, as five of the
  original's eleven do.)
- East of the gallery (added 2026-11-04), the stream, the original's row 17 (x 20-29: a fan at the
  corridor's west end blowing east, treasures (24, 17) and (26, 17) in the stream, an updraught
  column at its east end from a fan four rows under it): a chamber open to the gallery (x 67-71, z
  15-19, walk 20-28), a hole up into the stream (x 68-81, z 16-18, walk 30, three high; its fans in
  the west end, x 67), treasures 2 and 3 on its floor (74, 17) and (77, 17); the stream carries a
  helicopter east over both and drops it into the updraught shaft (x 82-83, its fans at y 25), which
  lifts it to a landing (walk 40) and a corridor back west to the gallery's top.
- West out of the gallery's wall (added 2026-11-02, Wave E): the draught tunnel, three wide and five
  high (x 13-33, z 16-18, walk 28), reached only by helicopter; across it four curtains of wind by
  turns, updraughts from fans in its floor (x 30, 22) and downdraughts from fans in its ceiling
  (x 26, 18), as the original's cave of drafts (its x 14-20, y 23-27, flown through by helicopter,
  ~12 fans in the world); treasure 1 in the chamber at its end (8, 28, 17; its old ledge holds an
  egg). Each curtain moves a helicopter crossing at full speed about 1.25 cells: entered too high,
  an updraught pins it under the ceiling and the next ceiling fan's mouth kills; too low in a
  downdraught it is grounded and rolls onto the next floor fan's mouth.
- South-east, the wind wing (added 2026-11-04), the original's sideways wind: east out of the hall
  (z 57-58) and up a step (walk 22) onto the start of a corridor one wide and one high (z 66, walk
  20) with two fans back to back in its middle (x 78 blowing west, x 80 east), a mine in each
  stream (x 73, 83), as the original's row 11 (x 7-24: fans 13 and 15, mines 9 and 19). Over the
  corridor blocks with one-cell gaps (the original's single blocks stand 5 apart and are met from
  above; a jump spans ~2.3 cells, so here they are walked and jumped): a gap blows the fallen onto
  a mine (x 74, 81) or to a wall (x 71, 84); treasure 3 on the block over
  the east mine (83, 22, 66), as the original's (19, 9) over its mine. From the end (x 85-87) south
  to a hole over a second stream (z 74, its fan at x 87 blowing west): treasure 4 under the hole a
  cell from the fan, as the original's (39, 11) from its fan (41, 11), reached only from above
  (walking up the stream loses ground: 9 px a tick against his 8); the stream blows him west into
  the crates' room (x 64, z 73-75).
- East, the skate run: five skateboards at its head, two blocks of mines two deep across it (the
  skateboard rides through), treasures 4 and 5 at its end.
- South, the crates' wall: a row of six crates, a wall three high and over it a second wall three
  higher: push crates to the walls' feet (one, then two stacked on the first wall) to climb to the
  high shelf with treasures 8 and 9 (a helicopter flown in reaches it too).
- West: a curtain of lightning, a pool with a fish, five skateboards, a block of mines two deep, a
  second curtain, treasures 10 and 11; a spare skateboard.

**Checks**: `flightcheck.py --mine-margin 0`: stage 1 and the real world reach the eleven treasures,
five eggs and the golden key (the real world reaches the high shelf by helicopter).
`VerifyShippedWorlds`. Live (`--mission 115`, direct view F3): the hall, the gallery, the skate
run's mines, the crates, the pool, the west run. Played through from the spawn to the main hub by
`tools/autoplay/world115.txt`, by helicopter (the gallery's ledges, over the skate run's mines,
onto the high shelf, the west wing past its curtains). The crates' steps reach only the first
wall: crates never stack on a flat floor, so the high shelf is the helicopter's (the stage
variant's two stacked crates are for levelcheck only).
