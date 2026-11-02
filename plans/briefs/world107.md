# World 107 — mission 7 of world 10 (the tower and world 10's golden key)

Built by `tools/worlds/world107.py` → `worlds3d/world107.vwr` (mission 107, region 2 as the
original). Staged variants for `levelcheck.py`: `--stage K out.vwr` (K ≥ 1 a stair beside the
lollipop's wall, no mines or hanging mines; K ≥ 2 the red door open).

**Reference** (`worlds/world107.txt`, video 4:28:00-4:31:44, golden key → main hub): one tall
shaft of cobble. Blupi starts at its foot beside the golden key (21) and the one treasure behind
the red door (334); the red key (49) is at the very top. Up the shaft: a lift moving across and up
at once (50 ticks), a lollipop (26) for the high jump past a wasp (44), four mines bobbing (16), a
row of six mines hanging across the way (crouch), a lift of six cells round a pillar carrying him
far up (108 ticks), ledges with mines (2), hanging mines (3) and eggs, the red key and two eggs at
the top; then down again, the red door, the treasure, the golden key. Six eggs.

**3D design**: the tower rises north of the foot's chamber, inside one mass of cobble.
- The chamber (walk 4): spawn (44, 4, 52) by the golden key at its west end; the red door in its
  east wall, the treasure behind it. A lift (2 × 2) moving across and up at once from its floor to
  the first ledge (walk 10) in its north-west corner.
- The ledge: the lollipop at its west end (stand on it, Action: fifteen seconds of very high
  jumps), a wasp crossing it at Blupi's height; north of it the tower's floor stands four higher,
  a jump only the lollipop's power makes (head room above the ledge for it).
- Added 2026-11-02 (Wave E, `lesson_inventory.md`): a band of wind across the high jump, three fans
  stacked in the head room's east end (52, walk 11-13, z 42) blowing west over the ledge's north
  row, the original's three fans across its shaft just over the lollipop (its y 75-77); the high
  jump rises through it and drifts west (it showed BD-0030: wind overhead had cut the jump short).
- The tower's floor (walk 14): four mines bobbing between walk 14 and 16; a wall across the tower
  with a low passage through it two high, six mines hanging in its walking cells in all three lanes
  (crouch under them); beyond it the great lift: six cells round a column of cobble, carrying him
  up to walk 32 (108 ticks, dwelling 30).
- The stair of ledges (walk 32 → 56): a landing, then twelve ledges two up each, three wide,
  winding round the column twice with a gap of one between each and the next; mines on two of
  them (step round), mines hanging over the gaps, four eggs.
- Two of the stair's ledges (walk 46 and 54) are vanishing saucers (324), as
  the original's pairs of saucers up its shaft (its y 36-44): gone 8 ticks in every 80, all at
  once; each taken from the solid ledge before it just after they come back and left straight on
  within the cycle (added 2026-11-02; the ledges where the next hop needs lining up stay solid).
- The top room (walk 58): the red key, two eggs, a chute straight down to the chamber.

**Checks**: `levelcheck.py`: stage 2 reaches the treasure, the six eggs, the red key and the golden
key; stage 1 stops at the red door; the real world stops at the lollipop's wall. `VerifyShippedWorlds`.
Live (`--mission 107`, direct view F3 at spawns in each part): the golden key, the red door, the
ledge, the low passage's mines, the column, the top room. Played live on a probe of the same wall:
the lollipop taken (standing on it, Action: the power gauge shows) and the jump onto a wall four
high (a first try one cell off the lollipop never took it). Not played through: the mission's own
ledge jump, the great lift, the stair, the chute.
