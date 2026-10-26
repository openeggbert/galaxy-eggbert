# World 125 — mission 5 of world 12 (the golden key's cave and world 12's end)

Built by `tools/worlds/world125.py` → `worlds3d/world125.vwr` (mission 125, region 2 as the
original). Staged variant for `flightcheck.py`: `--stage 1 out.vwr` (no mines, moving or hanging
mines, lightning, bird or bulldozer).

**Reference** (`worlds/world125.txt`, video 0:29:05-0:31:27, golden key → main hub): no treasure.
Blupi takes a helicopter (13; twenty-two of them) through a dark cave of brick walls and pillars
to the golden key (21), which hangs in a ring of moving mines (16) bobbing round it; on the way
rows of mines (2, twenty-one) on the ledges, columns of lightning (305) over a corridor of hanging
mines (3), a bird (20), a pool with a fish (17), a bulldozer (4), crates (12), four eggs.

**3D design**: a dark hall walled in brick (x 8–92, z 14–86, its floor walk 8).
- The start ledge on the west wall (walk 20): spawn (12, 20, 50), six helicopters, an egg, two
  mines.
- The corridor of lightning: a tube of brick across the hall (walk 20, three high), mines hanging
  in its roof, three curtains of lightning, a mine on its floor.
- The pool on a block of brick: a fish, an egg, two helicopters.
- Ledges round the hall's east end at walk 22 to 32: rows of mines along them, crates, a bulldozer
  patrolling one, a bird crossing the hall, a helicopter on each.
- The golden key on a pillar in the far east (walk 30), eight mines bobbing round it at different
  paces; a lift beside it from the east ledge. Two helicopters on the hall's floor for a fall.

**Checks**: `flightcheck.py --mine-margin 0`: the real world and stage 1 reach the golden key and
the four eggs. `VerifyShippedWorlds`. Live (`--mission 125`, direct view F3): the start ledge's
helicopters, the corridor of lightning and its hanging mines, the pool, a ledge's mines, the
pillar, the hall's floor. Not played through: the flight to the key.
