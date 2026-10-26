# World 112 — mission 2 of world 11 (the machine cavern)

Built by `tools/worlds/world112.py` → `worlds3d/world112.vwr` (mission 112, region 0 as the
original). Staged variant for `flightcheck.py`: `--stage 1 out.vwr` (no mines, bulldozer or
lightning).

**Reference** (`worlds/world112.txt`, video 0:34:35-0:36:27): a cavern in the blue machine panel.
From Blupi's start west past a bulldozer (4) to a row of six helicopters (13); up on the left a
gallery with three small lifts bobbing (10 ticks) between mines (2) on posts and the first
treasure with two eggs; in the middle, high up, the second treasure between two mines; east past
a column of lightning (305) and a mine a tall lift (54 ticks) up to the exit, three eggs by its
foot. Six eggs.

**3D design**: one block of panel.
- The start hall (walk 6): spawn (30, 6, 52); over it the high chamber (walk 18) with treasure 2
  between two mines, open west to the hangar's upper space.
- West the bulldozer's passage (alcoves to step into) to the hangar (walk 6, open up to walk 24):
  six helicopters, an egg.
- Along the hangar's north wall the gallery (walk 18): treasure 1 and two eggs at its west end,
  three small pits with lifts bobbing in them and mines on the posts between (fly up, or hop).
- East the lightning passage (a curtain across it under its emitters, a mine beyond) to the lift
  (2 × 2, 54 ticks, dwell 30) up nine to the exit's room (walk 15); three eggs by its foot.

**Checks**: `flightcheck.py --mine-margin 0`: the real world and stage 1 reach both treasures, six
eggs and the exit (the curtain is crossed in its off ticks: the checker jumps a one-cell curtain).
`VerifyShippedWorlds`. Live (`--mission 112`, direct view F3): the hall and the bulldozer, a
helicopter, the gallery's mines and lifts, the high chamber's treasure, the lightning passage.
Not played through: the flights, the lift.
