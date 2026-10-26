# World 111 — mission 1 of world 11 (the hangar shaft)

Built by `tools/worlds/world111.py` → `worlds3d/world111.vwr` (mission 111, region 0 as the
original). Staged variant for `flightcheck.py`: `--stage 1 out.vwr` (no hanging mines, mines or
bulldozer).

**Reference** (`worlds/world111.txt`, video 0:31:38-0:34:31): a climb through the blue machine
panel (10 and its parts 1-14). From Blupi's start a passage over two pits crossed on lifts (30
ticks) under eight hanging mines (3), three small lifts bobbing (10 ticks) with mines (2) on the
posts between them, a bulldozer (4) pushing to and fro (90 ticks); a hangar of thirteen
helicopters (13) at the foot of a tall shaft, two eggs by its pool; four treasures on ledges up
the shaft (flown up to), the invert (40) on the way; at the top a corridor of four mines and the
exit's room with four eggs.

**3D design**: the same climb, inside one block of panel.
- The start cave (walk 6): spawn (8, 6, 50).
- The passage of pits: two pits crossed on lifts (3 wide, 30-tick dwells) under mines hanging a
  cell above Blupi's head (harmless walking, deadly jumping); three small pits, a lift bobbing in
  each, mines on the posts between them (step round in the side lanes).
- The bulldozer's corridor: a bulldozer pushing along it, alcoves in its walls to step into.
- The hangar (walk 6): nine helicopters in a row, four more by a pool with two eggs; the shaft
  open above it to walk 34, treasures on four ledges up its walls (walk 12, 18, 24, 30), the
  invert on a fifth (a trap: controls turned round for a while).
- The top: a corridor of four mines (fly over or walk round them), the exit's room, four eggs.

**Checks**: `flightcheck.py --mine-margin 0`: stage 1 reaches the four treasures, six eggs and the
exit (on foot and in flight); the real world stops at the passage (its checker counts the cells
under hanging mines as unsafe). `VerifyShippedWorlds`. Live (`--mission 111`, direct view F3): the
passage's lifts under their mines, the bulldozer's corridor, the hangar's pool, the shaft from
above, the exit's room. Played through from the spawn to hub 110 by `tools/autoplay/world111.txt`
(the pits' lifts, the posts jumped, the bulldozer passed from an alcove, a helicopter to the four
ledges and through the corridor weaving round its mines).
