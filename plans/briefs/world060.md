# World 060 — the hub of world 6 (the slime grotto)

Built by `tools/worlds/world060.py` → `worlds3d/world060.vwr` (mission 60, region 31 as the
original). World 6's tiles and pieces: `tools/worlds/slime.py`.

**Reference** (`worlds/world060.txt`, video 5:22:09): a shelf of green slime inside the slime,
under slime vaults and columns, the region-31 green cave behind. Six alcoves in a row along the
shelf: mission 1's sign (174) open beside Blupi's start, missions 2-6 (175-179) each behind its
blue bar (182). Everything is one autotiled set, 341-363; measured over the original's worlds
060-066, 341-346 sit inside the mass, 362 on tops, 363/348 on undersides, 351 on one-block slabs,
349/350 on the sides and 352-361 on thin pieces and corners; 347, also on tops, is the world's
hazard (an animated slime that kills like spikes).

**3D design**: a grotto (x 30–70, z 30–66, walking level 5) under a slime vault eight blocks up,
its walls three thick on the west, north and east; the south side open onto the green sky over a
lip three high (too high to jump), the vault's edge hanging lower there.
- The six pods cut into the walls in a horseshoe, in the original's order from the start: 1 in
  the west wall at (32, 5, 58), open, beside the start; 2 in the west wall at (31, 5, 47); 3, 4,
  5 in the north wall at x 40, 50, 60 (z 31); 6 in the east wall at (69, 5, 44). Pods 2-6 are two
  deep, the bar in the wall's face, so there is no way round it; Blupi arrives outside the bar
  last opened. A slime arch (three wide, three high) stands in front of each pod.
- In the middle a mound of slime (walk 6 and 7) with a column from it up into the vault; columns
  hang from the vault elsewhere, two with stumps rising toward them.
- Skinning: in 3D a cube shows its tile on every side, so only the vertical roles carry over: 362
  on every top, inside tiles elsewhere. The undersides' and slab's clear edge bits fall under the
  renderer's cutout threshold and would draw black, so they are not used.
- Spawn (36, 5, 62) facing −Z, with pod 1 to his left.

**Checks**: `levelcheck.py` (all six signs reachable, bars counted open); `VerifyShippedWorlds`.
Live (`--mission 60`, and spawns facing the north wall and pod 1): the start view, the pods with
their bars and arches; the vault's edge clean after the underside tiles were dropped.
