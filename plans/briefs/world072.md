# World 072 — mission 2 of world 7 (the lollipop well)

Built by `tools/worlds/world072.py` → `worlds3d/world072.vwr` (mission 72, region 11 as the
original). Staged variant for `levelcheck.py`: `--stage 1 out.vwr` (stand-in lifts where each
licked jump goes, floors to the two columns).

**Reference** (`worlds/world072.txt`, video 3:41:58–3:43:04): the lollipop lesson. Everything is
yellow checkered balls (215, 1 341 of 1 380 tiles), single orange balls (217) as ledges, candy
poles (235) as decor, two big lollipops in the BigDecor layer (46, 47). Blupi starts in a little
room over a pit with the lollipop (26): down, lick it with Action (very high jumps for 15 s;
`Decor.cpp` puts it back at once, so it can be licked again), and jump up; then a tall shaft of
small ledges four rows apart: a treasure on the first, three side by side in a wider chamber
halfway up, two beside the red arrow at the top. Six treasures; the arrow counts only once all six
are taken. In the video the power runs out once on the way and he drops back for a second lick.

**3D design**: a well (inside x 37–63, z 39–63, floor walk 5) with walls three thick up to a rim at
walk 41, out of every jump's reach, candy poles standing on the rim; a tower of checkered balls
in its north-west (x 47–53, z 39–45, top walk 29) against the north wall.
- The stair of orange-ball blocks up the tower's west side, five wide, each a jump toward −Z, five
  higher than the last (the pier four over the floor): the pier (x 42–46, z 51–52, walk 9, the
  spawn (44, 9, 52) facing −Z), B1 (z 48–50, walk 14, a treasure), B2 (z 44–47, walk 19, a
  treasure: the chamber's level), B3 (z 39–43, walk 24, against the north wall, so a jump too
  many meets the wall), then one turn east onto the tower's top: the arrow at (50, 29, 40)
  between two treasures, orange balls on its corners. An ordinary jump (two) reaches none of it.
- The chamber's two other treasures on columns standing off B2: west (x 38–39, z 44–46, walk 21)
  and east against the tower (x 49–51, z 46–47, walk 21), each two higher across two cells.
- The lollipop on the floor behind the pier (44, 5, 54): stepping back off the pier he lands on
  it. Big toy lollipops, half balls and a cone with balls on the floor.
- The climb is one straight stair toward −Z because the turns cost time: with the stair turning
  round the tower (a first build) one lick ran out before the top; straight, one lick climbs from
  the floor to the tower's top with a third of the gauge left.

**Checks**: `levelcheck.py`: the real world reaches only the floor and the pier; stage 1 all six
treasures and the arrow. `VerifyShippedWorlds`: 80/80. Live (`--mission 72`, probes through
`QA_LIVE_WORLDS` with extra lollipops and treasure markers, shots
`build-probe/world7-agent/shots/72*`): each licked jump (floor→pier, pier→B1, B1→B2, B2→B3,
B3→top, B2→each column) lands; unlicked, the column hops fall to the floor; the gauge lasts
about 15 s; after it runs out the lollipop is there and a second lick works; one run from the
lollipop with Up and jump held: floor, pier, B1 (its treasure), B2 (its treasure), B3, then the
turn and the jump onto the tower's top beside the arrow.
Found while building it: from a spawn on a block's edge cell a short walk before the jump takes
him off the edge (a test artefact, not the level). The win at the arrow with all six treasures
was not played in one run.
