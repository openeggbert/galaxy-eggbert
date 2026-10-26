# World 104 — mission 4 of world 10 (the jeeps)

Built by `tools/worlds/world104.py` → `worlds3d/world104.vwr` (mission 104, region 2 as the
original). Staged variant for `levelcheck.py`: `--stage 1 out.vwr` (the mines driven over, the
hanging mines crouched under, the crates pushed into their steps, the sliding ladder as a stair).

**Reference** (`worlds/world104.txt`, video 4:09:01-4:12:46, jeeps over mines 4:10:22): a cobbled
cave crossed in jeeps (19): six wait on a ledge over a long corridor of mines (2), which a jeep sets
off one after another as it drives on; a heap of crates (12) by the start, a treasure on a ledge,
hanging mines (3) over the way down to another; a lift bobbing over lava; a shaft of lifts sliding
a cell to and fro one above the other (20 ticks, dwell 20) up to a treasure; a seventh jeep over a
second row of mines; a long winding way down with mines on it to the exit. Six treasures, three
eggs, forty-four mines, seven crates, seven lifts.

**3D design**: one mass of cobble.
- The start cave (walk 30): spawn (10, 30, 52). A ledge three up in its north-west corner,
  treasure 1: push a crate of the heap to its foot. South a passage down a step, one wide where
  two mines hang in the walking cells (crouch under them), treasure 2 at its end. The mines hang
  over level stretches with the step between them (2026-10-27: built as three steps, one at each
  mine, it was a trap: a crouching Blupi cannot jump the steps back up, standing he meets the
  mines).
- The garage east of the cave: six jeeps, two eggs. The corridor of mines (z 49–51): four blocks of
  mines two deep across it (no jump clears one, the jeep drives through setting them off), a step
  down, a block of four more in a passage two wide.
- The hall (walk 28): treasure 3 on a ledge three up (push a crate); a pit of lava, a lift bobbing
  in it between gaps of one and a pillar beyond with treasure 4 (jump when the lift is up); a mine
  hanging over the lava; an egg.
- The sliding ladder east of the hall: five lifts two apart one above the other, each sliding a
  cell (20 ticks, dwell 20), even ones west of the odd ones sliding the other way, so at the end of
  their slides two neighbours stand side by side, a jump up from one to the next with free head
  room (stacked straight above each other there was none); treasure 5 on a landing at the top.
- The lower gallery (walk 18), dropped into through a hole in the hall's floor: the seventh jeep,
  two blocks of mines, treasure 6.
- The spiral: a ramp of steps one high, two wide, winding down round a pillar from walk 18 to walk
  3, four mines on its outer lane (the jeep sets them off, a walker steps round), the exit at its
  foot. Golden forks in alcoves off the walking cells.

**Checks**: `levelcheck.py`: stage 1 reaches the six treasures, three eggs and the exit; the real
world stops at the corridor of mines. `VerifyShippedWorlds`. Live (`--mission 104`, direct view F3
at spawns in each part): the start cave's heap and ledge, the corridor of mines, the hall with the
ladder, the pit with its bobbing lift and the pillar, the lower gallery, the spiral. Played live:
a jeep taken in the garage and driven down the corridor, the mines going off ahead of it. The
ladder's first jumps led to D52 (a jump from a lift was cancelled at once), fixed; a jump from a
lift onto a ledge two high played live after it. Played through from the spawn to hub 100 by
`tools/autoplay/world104.txt` (both crate steps, the passage both ways crouching, the jeeps, the
bobbing lift's jumps, the whole ladder, the spiral on foot). 2026-10-27: the spiral's first corner
(86, 53) had no floor at the top, a fall to its foot; floored.
