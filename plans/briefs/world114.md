# World 114 — mission 4 of world 11 (the four keys' works)

Built by `tools/worlds/world114.py` → `worlds3d/world114.vwr` (mission 114, region 18 as the
original). Staged variants: `--stage K out.vwr` (K ≥ 1 no mines, moving or hanging mines, follow
stars, fish or enemy; K ≥ 2 the red door open, 3 the green, 4 the blue), for `levelcheck.py` and
`flightcheck.py`.

**Reference** (`worlds/world114.txt`, video 0:45:18-0:59:08, the longest of world 11): a great
maze of blue machine panel shut by coloured doors (334 red, 335 green, 336 blue) and their keys
(49, 50, 51): fourteen treasures, sixteen eggs; stairs swept by thirty-one moving mines (16); a
row of eight skateboards (24) by mines (2) and two inverts (40); pools with nine fish (17) and
hanging mines (3); sleeping follow stars (96) by treasures, an enemy Blupi (32) firing down, five
helicopters (13), fourteen lifts; the exit high up.

The original's tiles hold four more lessons the first 3D design left out (found 2026-11-02 by
`tools/audit/lesson_inventory.py`): it opens on a maze of bars (138) over lava (its x 3-20,
y 2-12); a room whose floor is four fans blowing up (132) under two mines sweeping across it (16;
its x 25-34, y 3-8); rooms of wind blowing sideways (129: its x 20-27, y 15-19, and the top
corridor, y 1); four columns of lightning (305; its x 31, 33, 42, 55).

**3D design**: a hall with four ways out, inside one mass of panel.
- The hall (walk 20): spawn (50, 20, 52); the red door west, the green north, the blue east (three
  wide, two high); south an open way down; two eggs.
- South, the terraces (walk 20 → 8): four terraces three below each other and the floor; on each,
  mines sweeping along it in three lanes at different paces (thirty in all: cross between them);
  treasures 1 and 2 at terrace ends, three eggs, the red key on the floor; a lift (2 × 2, 120 ticks)
  from the floor back up.
- West, behind the red door, the skate track: eight skateboards at its head, a block of nine mines
  three deep across it (the skateboard rides through), treasures 3–5 along it, the green key at its
  end, two inverts in alcoves off it, two eggs.
- North, behind the green door, the pool: level with the floor round it, nine fish, treasures 6–9
  on its bottom (dive for them); a chain of four islands across it, each reached on a ferry (2 wide,
  60 ticks, dwelling 40); mines hanging over the islands' edges; three eggs; the blue key on the
  last island.
- On the ways between (added 2026-11-02, Wave E): south, a pit of lava fills the way (z 60-64) and
  a bar runs over it along Z (x 50, two over the floor): crossed hanging, both ways; west, between
  the red door and the track, a passage one wide and six high (x 31-39, z 50) over three fans in
  its floor blowing up (x 38, 35, 32): each jumped (its mouth kills), the updraught throwing the
  jump up to ~4.8 over the floor, where a moving mine sweeps the passage under its ceiling (x 31-39,
  five over the floor, out of a plain jump's reach); a column of lightning across the north way
  (z 35) and across the east way (x 65). The sideways wind rooms are not reproduced (open).
- East, behind the blue door, the vault: treasures 10–14 on its floor, each by a sleeping follow
  star; an enemy Blupi patrolling a ledge along its north wall firing down; five helicopters; the
  exit on a high platform (walk 34), reached by helicopter; eggs on the platform and the ledge.

**Checks**: `flightcheck.py --mine-margin 0`: stage 4 reaches the fourteen treasures, sixteen eggs,
the keys and the exit; `levelcheck.py` stage 1 stops at the three doors. `VerifyShippedWorlds`.
Live (`--mission 114`, direct view F3): the hall's doors, the terraces' moving mines, the skate
track's mines and treasures, the pool and its islands, the vault, the exit's platform; the bar over
the lava, the fan passage (`plans/evidence/world114-lessons.png`). Played
through from the spawn to hub 110 by `tools/autoplay/world114.txt` (the bar over the lava both
ways, the fans jumped as the mine turns at the passage's far end, the lightning in its quiet half,
the terraces' lanes crossed
at their middles, the skateboard through the mines, the four ferries, four dives, the vault's
stars left asleep, a helicopter to the exit). 2026-10-27: the way from the terraces' floor back
north to the first terrace had no floor, so the red key's floor was a trap; it has one now.
