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
- East, behind the blue door, the vault: treasures 10–14 on its floor, each by a sleeping follow
  star; an enemy Blupi patrolling a ledge along its north wall firing down; five helicopters; the
  exit on a high platform (walk 34), reached by helicopter; eggs on the platform and the ledge.

**Checks**: `flightcheck.py --mine-margin 0`: stage 4 reaches the fourteen treasures, sixteen eggs,
the keys and the exit; `levelcheck.py` stage 1 stops at the three doors. `VerifyShippedWorlds`.
Live (`--mission 114`, direct view F3): the hall's doors, the terraces' moving mines, the skate
track's mines and treasures, the pool and its islands, the vault, the exit's platform. Not played
through: the mission (fourteen minutes in the original), the swims, the flights, the follow stars.
