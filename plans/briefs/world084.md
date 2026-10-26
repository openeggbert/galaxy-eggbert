# World 084 — mission 4 of world 8 (the golden key of the works)

Built by `tools/worlds/world084.py` → `worlds3d/world084.vwr` (mission 84, region 22 as the
original: a striped sky). Staged variant for `levelcheck.py`: `--stage 1 out.vwr` (no mines,
moving or hanging mines, birds or lightning; a floor under each run of bars).

**Reference** (`worlds/world084.txt`, video 6:18:50-6:23:51): the last of world 8 ends not at the
red arrow but at the golden key (21), out on floating lifts. A machine of panel (10) and pipe
(250-259) under a striped sky. From Blupi's start a pipe walkway with two mines under hanging mines
(3); lifts over pits of lava; two mines bobbing (16) by a lift down; a corridor with a mine, a
column of lightning (305, its emitter 304 above) and a hanging mine, warning panels (48); bars over
a pit where two mines sweep; a hall with a shuttle over water, a second column of lightning between
girders (201) and lifts meeting in pairs over lava, each pair a step higher; a fall down the east
side; a fan (126) whose wind (110) blows over a shuttle across water and bars over lava; six birds
(20) over lava; rows of mines (2) on a pipe walkway; hanging mines and mines by a lift moving across
and up at once, then a chain of lifts in the open out to the key; a pool with three eggs under it.
Fifteen eggs, no treasure; twenty-three mines, five hanging mines, four moving mines, sixteen lifts.

**3D design**: the works wind round the sky, each part turning off the last.
- The catwalk (x 9-11, z 56-94, walk 60, north): spawn (10, 60, 92) facing −Z. Pipe round the
  deck's flanks and pipe rails along it. Two rows of mines, each under hanging mines a cell above
  the free lanes (walk past, never jump); two pits of lava five long, each crossed on a lift three
  wide; a one-wide bridge where two mines bob two cells up and down in turn (the cell between them
  is safe); the lift down (2 × 2, twelve cells) at its north end.
- The corridor (z 55-57, walk 48, east), inside a block of panel: two mines, warning panels, a
  curtain of lightning across it (deadly, and drawn, only in the flickering half of its 5 s cycle:
  D51), a hanging mine, an egg; a pit six long across it, lava at its foot and two mines sweeping
  it, crossed hanging from rods (202, along X; 138's flat plate cannot be seen end on); a hole down
  its shaft into the hall.
- The hall (x 36-44, walk 36, north): a shuttle three wide over a pool level with the floor (fall
  in and climb out); a pipe floor with eggs on pedestals; a curtain of lightning across the whole
  hall; a field of lava crossed on two pairs of lifts meeting midway with a rest between (2.5 s
  dwells: two cells to step); the last lift carries Blupi out east through the wall to the ledge
  two higher (walk 38).
- The wind hall (z 3-9, walk 24, east): a drop from the ledge's far end through the roof; a fan in
  the west wall (129, blowing east here, the way he goes) over a shuttle across a pool and over rods
  across a trench of lava; the east ledge, a mine, a stair down south.
- The birds (walk 18, south): a causeway winding over a tray of lava in the open, six birds across
  it (two a cell above the walk: harmless to a walker, deadly to a jump), an egg on a stand a jump
  off it.
- The mines (z 43-45, walk 18, west): a pipe walkway, six rows of mines across it, each row leaving
  a different lane free (weave through), eggs between.
- The chain (walk 18 → 26, out south-east): the platform with two mines and a hanging mine; a lift
  moving out and up at once, a lift along X, a lift up, a lift along Z, all four in step, each
  meeting the next at a dwell; the golden key on its stand. Under the chain a pool two deep
  catches a fall (its water level with its rim; three eggs in it, a stair from it back up to the platform).

**Checks**: stage 1: `levelcheck.py` reaches all fifteen eggs and the golden key; the real world
stops at the first row of mines (levelcheck keeps a cell off mines). `VerifyShippedWorlds`. Live
(`--mission 84`, direct view F3 at spawns in each part): the catwalk's mines, pits and bridge, the
corridor's lightning, the hall's pool, curtain and lava lifts, the drop, the wind hall, the birds,
the rows of mines, the chain over the pool. Played live: walking onto the corridor's rods, hanging
across the pit and stepping off; the hall's curtain flickering, then gone (D51); the whole chain from the platform's
first lift — out and up, across onto the second, onto the third, up, onto the fourth, onto the
stand — and the golden key, after which the main hub loads. Played through in one run by
`tools/autoplay/world084.txt`: both curtains walked through in their off phase, the hall's lift
pairs, the wind hall's shuttle and rods, the birds let pass at each crossing, the rows of mines,
the chain to the key. Ten eggs are taken (at ten an egg is not taken, the original's
MAX_EGG_COUNT); the egg on its stand by the birds is left (its jump rises into the bird a cell
above the walk).

Since 2026-10-27 every pool's water is level with its banks' top blocks (the weak jump out of the
water clears no more, as the original's -12 px/tick: the three pools were a block below their
banks and trapped a Blupi who fell in).
