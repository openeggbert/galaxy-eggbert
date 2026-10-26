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
  high ledge; treasures 1–4 on ledges round its walls at walk 26, 30, 34 and 38; two pits of lava.
- East, the skate run: five skateboards at its head, two blocks of mines two deep across it (the
  skateboard rides through), treasures 5–7 at its end.
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
