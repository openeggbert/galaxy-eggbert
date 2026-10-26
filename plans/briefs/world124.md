# World 124 — mission 4 of world 12 (the cisterns)

Built by `tools/worlds/world124.py` → `worlds3d/world124.vwr` (mission 124, region 27 as the
original). Staged variants for `levelcheck.py`: `--stage K out.vwr` (K treasures taken: the doors
"3", "9" and "17" open as they count; K ≥ 1 no mines, moving or hanging mines, or fish).

**Reference** (`worlds/world124.txt`, video 0:20:19-0:27:50, teleport 0:28:48): brick cisterns full
of water: tanks where eighteen fish (17) swim, treasures on their bottoms and in pockets of their
walls; three pairs of teleporters (330-332); mines (2), hanging mines (3), mines bobbing (16), five
lifts, two crates (12), a fan (129) and its wind (114); treasure doors (423 "3" to 437 "17").
Seventeen treasures, seven eggs.

**3D design**: a block of brick, the cisterns in a row from west to east (walk 20).
- The start passage: spawn (6, 20, 50) by the exit; a mine hanging a cell above the way, a mine,
  an egg; door "17" north of the exit to a pocket with an egg.
- Tank 1 (three deep, its water level with the floor round it): three fish, three treasures on its
  bottom, one on its rim; teleporter 330 at its far end (its partner by its own entrance); door "3".
- Tank 2 (five deep): five fish, four treasures on the bottom, one on the rim, a ferry of two lifts
  across it (90 ticks, dwelling 40), two eggs; teleporter 331; door "9".
- The wind tunnel: a fan in its west end blowing east, its wind carrying Blupi over a pit where a
  mine bobs.
- Tank 3 (seven deep): ten fish, seven treasures on its bottom, one on its rim, two crates, two
  eggs; teleporter 332 back to the start passage.
- A fall into a tank lands Blupi on its bottom: swim up and climb out at its rim.

**Checks**: `levelcheck.py`: stage 17 reaches the seventeen treasures, seven eggs and the exit
(falls reach the bottoms; it cannot swim, the game does); stage 9 stops at door "17", stage 2 at
door "3"; the real world stops at the first mines. `VerifyShippedWorlds`. Live (`--mission 124`,
direct view F3): the start passage, tank 1 and its fish, tank 2's ferry, the fan tunnel, tank 3,
a teleporter's pillar. Not played through: the swims, the teleporters, the wind.
