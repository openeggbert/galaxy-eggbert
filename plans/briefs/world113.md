# World 113 — mission 3 of world 11 (the lightning loop)

Built by `tools/worlds/world113.py` → `worlds3d/world113.vwr` (mission 113, region 26 as the
original: a pale blue sky). Staged variant for `levelcheck.py`: `--stage 1 out.vwr` (no mines,
hanging mines or lightning; a floor over the pool).

**Reference** (`worlds/world113.txt`, video 0:38:07-0:41:02): a run through the blue machine panel
and back under a pale blue sky. Blupi starts beside the exit; east a gallery of small quick lifts
(6-12 ticks) over lava under hanging mines (3), columns of lightning (305) one after another, a
long lift (80 ticks) over a pit of lava by a crate, a pool with a fish (17), lifts down; at the
far end the one treasure in a cage of four tiny lifts (5 ticks) opening and closing round it,
mines hanging above and lying below; then the long way back to the exit. Five eggs, twelve mines
(2), twelve lifts.

**3D design**: a loop inside one block of panel.
- The start ledge (walk 20): spawn (9, 20, 50) beside the exit (it needs the treasure).
- The gallery east: four pits of lava, a small lift in each carrying Blupi across (10-12 ticks,
  dwelling 20 rather than 3), mines hanging a cell above his head over the last two.
- A curtain of lightning; the ferry room: a crate, a pit of lava crossed on a lift (2 × 3, 80
  ticks), a mine hanging over it.
- The pool with a fish (swim across, or wait); a lift down the shaft beyond (walk 20 → 10); a
  second curtain across the low passage, two mines in it.
- The cage: the treasure on a pillar in a pit, four little bridges (lifts) sliding in to it from
  its four sides and out again (20 ticks, dwell 20: lifts here do not block Blupi sideways, so the
  original's closing cage became bridges that come and go); mines hanging a cell above them and
  lying in the pit below.
- The way back: a lift up from beside the cage to the high way (walk 28), back west over the whole
  loop, a third curtain and three mines in it, an egg; a hole down onto the start ledge.

**Checks**: `levelcheck.py`: stage 1 reaches the treasure, five eggs and the exit; the real world
stops at the first hanging mines. `VerifyShippedWorlds`. Live (`--mission 113`, direct view F3):
the gallery's lifts, the ferry room, the pool, the low passage's mines, the high way. Not played
through: the cage's bridges, the lightning crossings, the swim.
