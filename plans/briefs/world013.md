# Mission 013 — helicopter, diving, skateboard (blue machine world)

Built by `tools/worlds/world013.py` → `worlds3d/world013.vwr` (mission 13, region 0); hints:
`Tables::table_training3` (11 records, gated on vehicle / no vehicle) as hint zones.

**Reference** (`worlds/world013.txt`, video 3:18–4:49): take a helicopter in the start room and fly;
leave it before the water ("it dislikes water"), plunge and swim on; take another helicopter and
fly up for the three treasures; fetch a skateboard from the top-left corner; ride it along a
corridor over two rows of mines (the skateboard runs them over) and two pools (jump them); leave
it at the red arrow. 13 helicopters, 11 mines, 6 skateboards, 3 treasures.

**3D design**: sealed machine halls in a row (x 40–56), each reachable only the way the lesson
wants:
- A, the hangar (walk 1, two helicopters): its north wall has a window high up (y 6–8, x 43–53).
- B, the pool hall: a ledge behind the window (walk 6) to land and leave the helicopter; the pool
  below (water y 1–4).
- The dive: a one-block curtain wall reaching into the water to y 3; the pools meet under it.
- C, the treasure shaft (walk 5, two helicopters, roof at y 26): three treasures at y 10, 14, 18;
  the cliff of D on its north side (top y 20).
- D, the gallery on the cliff (walk 21): west through a door the skate room with three
  skateboards; east the skate tunnel (3 wide, roofed): mines across it at x 64–65 and 80–81, water
  trenches at x 70–71 and 86–87, the red arrow at x 95.
Each tunnel and wall is open enough for the chase camera; the dive's curtain is one block thick.

**Checks**: `VerifyShippedWorlds`. `levelcheck.py` cannot model vehicles or swimming; checked live:
helicopter taken and climbing, the dive under the curtain into C (surfacing beside the
helicopters), the skateboard taken and run over the first mine row to the water.
