# World 075 — mission 5 of world 7 (the golden key of the toys)

Built by `tools/worlds/world075.py` → `worlds3d/world075.vwr` (mission 75, region 15 as the
original). Staged variants (checked with `flightcheck.py --mine-margin 0`): `--stage 1 out.vwr` (the
skateboards' mine fields gone, the bars taken away), `--stage 2` (the fields gone, the bars in
place).

**Reference** (`worlds/world075.txt`, video 3:48:22–3:54:27): the last mission of world 7 ends at
the golden key (21, the secret exit), which counts only once all seven treasures are taken. Two
masses of toys: green marble balls (218) above, orange balls (217) below; grey grates (200/201) as
pillars, the flight barrier (214) framing a shaft, water, lava, bars (138). Blupi starts at the
foot of a hill with mines on its steps and a hanging mine; two shuttles (60 ticks) meet over a gap;
five skateboards (24) by a row of mines with three eggs beyond; a raft of three lifts in step
(80/20) over lava; a lava strip crossed on three small shuttles (40/20) with a mine on each cell
between; a long fall down a shaft; four skateboards at its foot, humps covered in mines; seven
treasures in a funnel with two moving mines (16) sliding over it, an egg; a relay of five lifts
(30/10) climbing a column, each meeting the next; bars over a gap; four skateboards; a corridor of
mines in groups between pillars, ridden to the golden key. 61 mines, 13 lifts, 13 skateboards,
4 eggs.

**3D design**: a plateau of marble walkways (walk 40, walls three high, open to the sky), a shaft,
a basin hall of orange balls (walk 10), a relay, a west tube (walk 25).
- A, the hill (x 10–16, z 26–46): the foot (walk 34, spawn (13, 34, 44) facing −Z), steps of two
  (walk 36, 38) to the top (40). Mines on each step's landing row in some lanes (a jump there meets
  one, other lanes clear); over step 2's takeoff row hanging mines under a beam: walk under them,
  jump from the lanes beside.
- B, the canal (z 14–25): a gap of twelve over water (top cell y 37, a bank at its south end level
  with it, a step up to the deck: a fall is no trap); two shuttles three wide meet in its middle
  (60 ticks, 40 dwell): ride S1, step onto S2, ride it to the north bank.
- C, the yard (x 10–26, z 6–12) with five skateboards; the mine corridor east (z 9–11): mines two
  deep across it (no jump clears them; the skateboard runs them over); past them the eggs' ledge
  (walk 42) in an alcove, the walls round it raised so its top is no step onto the walls.
- D, the pond (x 46–54): a raft of three lifts in step carries him eight cells over lava (80 ticks).
- E, the alley (x 57–76): three pits of lava three long, a small shuttle over each (40 ticks),
  alternating lanes; a mine on the middle lane of the cell after each pit: step off beside it.
- F, the shaft (x 77–79, z 9–11): thirty down into the basin, walled to the plateau's walls.
- G, the basin (x 72–88, z 9–62, walk 10, roofed): four skateboards at the shaft's foot; the course
  south (x 78–82): two mine fields two deep, a hump one high after each (hop it).
- H, the funnel room: a square funnel four deep (rings walk 9, 8, 7, bottom 6), the seven treasures
  on its rings and bottom; two moving mines sliding along ring 1's north and south rows (40 ticks);
  an egg on a ledge two high in the south-east corner; grates on high plinths.
- I, the relay (x 46–47, z 60–61), from the way west (walk 10): A1 rises, B1 comes down to meet it,
  B1 goes back up to A2, A2 meets B2, B2 back up to A3, A3 up to the west level (walk 25); 30 ticks,
  40 dwell (the original's 10 is too short to step across in 3D). A fall goes back to the bottom.
- J, the west tube (walk 25): a gap of five over lava under rods (202) along X: walk off and hang
  across on foot; the skate shed (x 22–29, z 57–64) with four skateboards; the key corridor north
  (x 24–26, z 21–56): four mine fields two deep between striped pilasters; the key's room under
  striped pillars, the golden key at (25, 25, 18).

**Checks**: `flightcheck.py --mine-margin 0`: the real world stops at the mine corridor (two deep);
stage 1 reaches all seven treasures and four eggs and stops at the bars; stage 2 also the golden
key. `levelcheck.py` (cells beside mines avoided) stops on the hill. `watercheck.py`: every body of
water has a way out. `VerifyShippedWorlds`: 80/80. Live (`--mission 75`, probes through
`QA_LIVE_WORLDS` with markers and long-dwell lifts, shots `build-probe/world7-agent/shots/75*`):
every part's view; the hill climbed in clear lanes, a jump in a mined lane and one under a hanging
mine killed; S1 to S2 to the north bank; the yard's mines run over on a skateboard; the raft, the
three alley shuttles (alive beside each mine); the fall down the shaft; the course on a skateboard
(fields run over, humps hopped); the funnel walked through, five treasures past the sliding mines,
and a later start caught by one; the relay A1 to A3 at the landing's level; the rods across the
gap; the key corridor ridden on a skateboard to the golden key, which ends the mission and returns
Blupi to the main hub (in a probe without treasures, so the key counted at once). Not played in one
run; the relay and the canal at the authored timing not played.
