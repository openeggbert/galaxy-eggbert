# World 121 — mission 1 of world 12 (the brick house)

Built by `tools/worlds/world121.py` → `worlds3d/world121.vwr` (mission 121, region 24 as the
original: a brick backdrop). Solved variant for `levelcheck.py`: `--solved out.vwr` (the crates
pushed to the ledges' feet, no mines).

**Reference** (`worlds/world121.txt`, video 0:06:20-0:07:53): a short first lesson in brick (261-263,
283). Blupi starts beside the exit in a little room under two hanging mines (3); round it rooms
with four treasures, three crates (12) to push, mines (2), four eggs; far off a cave of brown rock
(284, 301) with the invert (40) and mines.

**3D design**: a block of brick.
- The start room (walk 10): spawn (50, 10, 52) beside the exit (it opens with the four treasures);
  north a doorway under two mines hanging a cell above Blupi's head (walk under, never jump).
- West, the room with the ledge: treasure 1 on its floor, treasure 2 on a ledge three up: push the
  crate to its foot; an egg.
- East, a corridor with two mines (step round) to a room with treasure 3 on a ledge three up and
  its crate; an egg.
- North, through the doorway, a drop into the cellar (walk 5): treasure 4, two eggs, a crate, a
  stair back up.
- West down a long tunnel the cave of brown rock: the invert (a trap: controls turned round for a
  while), an egg, three mines.

**Checks**: `levelcheck.py`: the solved variant reaches the four treasures, five eggs and the exit;
the real world stops at the two ledges (the crates not pushed). `VerifyShippedWorlds`. Live
(`--mission 121`, direct view F3): the start room's doorway and its hanging mines, the west room's
crate, the east corridor's mines and crate, the cellar, the rock cave. Played through from the
spawn to hub 120 by `tools/autoplay/world121.txt` (both crate pushes, the cellar and its stair; the
rock cave skipped). 2026-10-27: the cellar's stair had been buried in the house's brick (the fills
added nothing to solid brick), so the cellar was a trap; it is now cut out (`levelcheck.py`'s trap
check).
