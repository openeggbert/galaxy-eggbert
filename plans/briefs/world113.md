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
- After the four lift pits, a fifth pit of lava five long (x 35-39) crossed hanging from a bar,
  one wide between walls, a mine hanging at each end of the bar in its row (x 34 and 40, y 22): the
  original's bar 28-32 over lava between its hanging mines 27 and 33 (added 2026-11-02, Wave E;
  the lift pits moved closer, x 15, 20, 25, 30, to make room). Caught from a running jump at the
  floor's edge; let go with Down short of the far end (hanging, he touches anything in the bar's
  row within 0.44) and drift onto the floor past the second mine. The original also hangs bars over
  its lift pits (its x 14-25, y 19) as a second way across them: not reproduced (the lifts are).
- A curtain of lightning; the ferry room: a crate, a pit of lava crossed on a lift (2 × 3, 80
  ticks), a mine hanging over it.
- The pool with a fish (swim across, or wait); a lift down the shaft beyond (walk 20 → 10); a
  second curtain across the low passage, two mines in it.
- The cage: the treasure on a pillar in a pit, four little bridges (lifts) sliding in to it from
  its four sides and out again (20 ticks, dwell 20: lifts here do not block Blupi sideways, so the
  original's closing cage became bridges that come and go); mines hanging a cell above them and
  lying in the pit below.
- The way back: a lift up from beside the cage to the high way (walk 28), back west over the whole
  loop: first a windy stretch (x 62-70, one wide at z 56), as the original's corridor beyond its
  lightning (47-64, 33-36): a fan (126) in its upper row at the east end blowing west, bars (138)
  in its top row over two mines lying in the lane, passed hanging (caught from a jump beyond the
  fan's mouth); then a third curtain and three mines, an egg; a hole down onto the start ledge.

**Checks**: `levelcheck.py`: stage 1 reaches the treasure, five eggs and the exit; the real world
stops at the first hanging mines. `VerifyShippedWorlds`. Live (`--mission 113`, direct view F3):
the gallery's lifts, the ferry room, the pool, the low passage's mines, the high way. Played
through from the spawn to hub 110 by `tools/autoplay/world113.txt` (every lift, the three curtains
between strikes, the swim, the cage's bridges). 2026-10-27: the shaft's lift and the lift up to the
high way stood a cell off the floors they serve (a gap to jump from the lift); both now stand
against them.
