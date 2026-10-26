# World 071 — mission 1 of world 7 (the jeep and the toy tower)

Built by `tools/worlds/world071.py` → `worlds3d/world071.vwr` (mission 71, region 12 as the
original). Staged variants for `levelcheck.py`: `--stage K out.vwr` (1: the cleft's mines gone,
the bars taken away; 2: the mines gone, floors under the bars; 3: the mines gone, the trench
floored, the basin's bars taken away).

**Reference** (`worlds/world071.txt`, video 3:36:41–3:41:55): the first toy mission, no treasure,
eight eggs. Blupi starts by a row of six mines (2) between two warning signs (48); the jeep (19)
waits at the top of a zigzag shaft of green marble balls (218), its ledges with mines and hanging
mines (3): he climbs to it on foot, drives it down over the mines and over the row (the jeep runs
mines over), and leaves it where a lift (1) in a frame of blue pipes (49–56) crosses a gap. On foot:
a ramp up among moving mines (16) hovering in place, an egg (6); pole tops (230 caps on 236 poles)
to jump from one to the next under balls hung with mines; bars (138) over lava (68); a garden of
every toy with two mines and an egg; a long fall down a shaft between striped poles to a forest of
poles; lifts carrying him sideways over gaps, then up a lattice of star platforms (230) on striped
poles with lifts (rising and shuttling, 10–40 ticks, dwell 10), bars over lava, eggs, the red arrow
at its top. Tiles by count: 218, 236, 222, 230, 221 and the other toys. 22 mines, 7 hanging,
14 moving, 8 eggs, 16 lifts, the jeep.

**3D design**: an island court with its tower, a cleft through a toy cliff, the ramp hall and the
pole hall west to east, a lava trench into the garden, the lower hall under the garden.
- A, the court (x 4–30, z 66–96, walk 30), floating in the sky, a kerb of striped tiles; spawn
  (16, 30, 71) facing −Z, the cleft ahead. The tower of balls (core x 10–16, z 78–84, top walk 44)
  with a stair of six ledges round its east, south and west faces (walk 32 … 42, a jump of two
  each); mines on the ledges' outer cells, hanging mines under beams over the cells where a jump
  starts (walk under, jump elsewhere). The jeep on the top; driven off the north edge it drops
  onto the court facing the cleft.
- B, the cleft (x 15–17, z 52–65), three wide through a toy cliff, warning signs at both ends:
  its floor holds fifteen mines five deep, too long to jump. Only the jeep crosses.
- C, the chasm (z 47–51, open below): three lifts in step (40 ticks, 40 dwell) carry him over; a
  gantry of blue pipes six above them (out of reach). An egg on the landing.
- D, the ramp hall (x 14–18, z 26–46): four steps of two up to walk 38; moving mines hovering
  over each step's edge in two or three of its five lanes (a jump in a mined lane meets them,
  a different lane each step); an egg at the top.
- E, the pole hall (x 22–46, z 26–36, walk 38), roofed: fifty caps on striped stems over a pit,
  a cell apart; twelve caps under balls hung on candy poles from the roof, a hanging mine under
  each (a hop onto or off them meets the mine). Steps out of the pit back to the entry ledge.
- F, the lava trench (x 47–53, z 30–32): the corridor's floor gone over seven cells, lava three
  below; bars across at the walking level: walking off the edge he hangs and moves along to the
  garden.
- G, the garden (x 56–78, z 18–42, walk 38) of every toy: two mounds (walk 40, 42; an egg on the
  higher), kept three cells from its kerb (from a mound a jump reached the kerb top, and its end
  dropped onto the lattice: found by levelcheck, fixed); toy decor (cones with balls, a ball on a
  pole); the shaft (x 70–72, z 36–38) down through its floor, lined with striped poles, a mine
  each side.
- H, the lower hall (x 40–82, z 10–46, walk 6), its floor of striped tiles: a trench of lava cut
  across it (x 58–67) with an island, crossed on two pairs of lifts (30 ticks, 40 dwell). In its
  west the lattice of star platforms on striped posts: V1 (two lifts) up to P1 (walk 10); jumps to
  P2 (12) and P3 (14); bars over a lava basin to P4 (an egg each side); a shuttle pair H1 over a
  gap to P5; V2 up to P6 (17, three eggs); V3 up to P7 (20); H2 over a strip of lava to P8; two
  jumps to the red arrow (45, 24, 12). The basin has no rim (a rim level with P3 was a way round
  the bars).

Candy poles 235/236 are kept out of reach: their empty collision mask makes them ground to the
engine's vertical test (D41), so the poles Blupi touches are the solid striped tiles 221–223.

**Checks**: `levelcheck.py`: the real world stops at the cleft (its mines); stage 1 reaches the
chasm's and the ramp's eggs and stops at the lava trench; stage 3 also the garden's egg and P3's,
not P4 and on; stage 2 everything: eight eggs and the arrow. `flightcheck.py --mine-margin 0`: the
same as the real world. `VerifyShippedWorlds`: 80/80. Live (`--mission 71`, spawns in each part,
F3; shots `build-probe/world7-agent/shots/71*`): every part's view; the climb's jumps (L2→L3,
L5→L6, L6→top) and a jump under a hanging mine (killed); the jeep through the cleft's mines (they
explode under it) to the chasm, and off the tower top onto the court; the chasm lift ridden over
(stepping on at its near dwell); the ramp: jumps in clear lanes land on the next step, in a mined
lane they die; three hops along a clear row of caps, a hop onto a mined cap killed; the trench
crossed hanging from the bars into the garden; the fall down the shaft to the lower hall; both
trench lift pairs ridden; V1, H1, V2, V3, H2 ridden; the jumps P1→P2→P3; the basin's bars to P4;
the climb to the arrow and the win (back in the hub).
