# World 122 — mission 2 of world 12 (the brick warren)

Built by `tools/worlds/world122.py` → `worlds3d/world122.vwr` (mission 122, region 24 as the
original). Solved variant for `levelcheck.py`: `--solved out.vwr` (no mines, moving mines or
bulldozer, the crates pushed into their pockets).

**Reference** (`worlds/world122.txt`, video 0:07:56-0:15:45): a warren of short tunnels in brick
round a shaft: ten treasures in pockets of the tunnels, sixteen mines (2) along them, six moving
mines (16) bobbing in shafts, five crates (12) plugging the ways, a bulldozer (4), the drink (30),
two inverts (40), twelve eggs; the exit by Blupi's start.

**3D design**: a block of brick.
- The central shaft (x 46–54, z 46–54): spawn (50, 6, 51) by the exit; a stair two up a step along
  its west and north walls from walk 6 to 22; passages out to the galleries: south from the bottom
  (walk 6), north from the middle (walk 14), east from the top (walk 22).
- Three galleries, each a loop of tunnels two wide and three high round the shaft (x and z 36–64):
  pockets off them with the treasures (three, four and three), four mines each (step round in the
  other lane), two moving mines bobbing through each loop, a crate plugging the north tunnels of
  the lower two (push it into the pocket beside it), four eggs each.
- The drink in a pocket of the top gallery; the bulldozer patrolling the middle gallery's south
  tunnel; two inverts in pockets of the bottom one.

**Checks**: `levelcheck.py`: the solved variant reaches the ten treasures, twelve eggs and the exit;
the real world stops at the first mines and crates. `VerifyShippedWorlds`. Live (`--mission 122`,
direct view F3): the shaft and its stair, the bottom gallery's crate and mines, a treasure's
pocket, the middle gallery, the top gallery. Played through from the spawn to hub 120 by
`tools/autoplay/world122.txt` (the ten treasures, all three loops; the drink and inverts left).
2026-10-27: the stair's steps stood one lower than designed (walk 7 ... 21, a jump up at each
landing); now 8 ... 22, level with the galleries' passages.
