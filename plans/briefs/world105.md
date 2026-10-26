# World 105 — mission 5 of world 10 (the helicopter cavern)

Built by `tools/worlds/world105.py` → `worlds3d/world105.vwr` (mission 105, region 2 as the
original). Staged variant: `--stage 1 out.vwr` (no lightning, bulldozer, wasp, mine or moving
mines), for `flightcheck.py`.

**Reference** (`worlds/world105.txt`, video 4:13:43-4:19:45): a great cobbled cavern flown through
in helicopters (13; twenty-four parked about it), its roof hung with mines (3, thirty-two), eight
birds (20) crossing it, treasures and eggs on little ledges round its walls and on blocks in its
middle, the exit in a hut of cobble on one of them, a lava floor; at its east side a passage
under more hanging mines past columns of lightning (305) into a chamber with a bulldozer (4) and
the last treasure; lifts, a wasp (44), two moving mines (16), a mine. Seven treasures, eleven eggs.

**3D design**: a bottomless hall of cobble (x 10–70, z 20–80, roof at 41; a fall is the end: a
lava floor of that size would be rebuilt every tick).
- The start ledge on the west wall (walk 16): spawn (12, 16, 50), four helicopters, an egg; a ferry
  of two lifts (2 × 2) meeting midway (80 ticks, dwell 40) to the hut's island.
- Islands of cobble at many heights round and across the hall: treasures 1–6, eggs, spare
  helicopters (twenty-three in all), a mine; the middle island's hut with the exit inside, its door
  on the west.
- Ledges round the walls with eggs and helicopters.
- Two rows of eleven mines along the roof (z 36 and z 64): fly under them; eight birds patrolling
  between the islands at different heights; two mines bobbing near the high islands.
- The east passage (walk 20, three wide, three high) off a ledge on the east wall: mines hanging at
  Blupi's head height in its roof (walk in, never jump), two curtains of lightning across it under
  their emitters; the chamber beyond: treasure 7, a bulldozer pushing to and fro, a wasp, two eggs,
  two helicopters.

**Checks**: `flightcheck.py --mine-margin 0`: the real world and stage 1 reach the seven
treasures, eleven eggs and the exit (on foot, the ferry, flights; flights keep off the birds'
patrols and the mines). `VerifyShippedWorlds`. Live (`--mission 105`, direct view F3): the start
ledge, the ferry and the hut's island, a parked helicopter, an island's treasure, the east passage
with its mines and emitters, the chamber's treasure. Played through from the spawn to hub 100
by `tools/autoplay/world105.txt` (a helicopter to the six islands at cruise height between the
birds, the east passage crouching between strikes, the chamber past the wasp, the hut). 2026-10-27:
the east ledge stopped a cell short of the passage's mouth (x 70, the hall's last column), a gap
into the bottomless hall; it now reaches it.
