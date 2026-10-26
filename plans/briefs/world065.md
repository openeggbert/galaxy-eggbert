# World 065 — mission 5 of world 6 (the slime catacombs)

Built by `tools/worlds/world065.py` → `worlds3d/world065.vwr` (mission 65, region 4 as the
original). Staged variant for `levelcheck.py` and `flightcheck.py`: `--stage 1 out.vwr` (the
keys' doors and door "1" open, the saws floors, the crates and the lightning gone, no enemies in
the narrow ways, a causeway over the lake, a floor over the cavern's lava, no mines on the jeeps' run).

**Reference** (`worlds/world065.txt`, video 5:54:15-6:03:58): a maze of slime round Blupi's start
with a single treasure, and the red arrow alone in a funnel far below, reached by a fall through
a hole behind door "1" (421). The treasure lies behind the red door (334); the red key behind the
green door (335) and a row of saws whose switch is reached from above; the green key in a cage of
machine tiles behind a curtain of lightning (305 under 304), past a switch and its saws; the blue
key (51) walled in by a block of nine crates that only dynamite (four sticks at the top of a
tower of blue blocks, saws and switches) blows away. The blue door (336) leads past the big
creature, over saws on a switch's row, down into a lake with an egg in an air pocket, to a
cavern of four overcrafts (46) hovering over its lava floor; teleporters (330) join the west to the north-east, where a garage of
four jeeps on lifts drives over a row of seven mines to three eggs; a lava bridge of crumbling
tiles (364) under a moving mine guards the drop. Twelve eggs, fourteen mines, six moving mines,
six switches.

**3D design**: wings round the start hall, one storey above them, the funnel far below.
- The start hall (x 40–52, z 58–70, walk 30): spawn (48, 30, 66) facing −Z. A lift (2 × 2) in its
  north-west corner up to the west region; the red door west, the green door north; east a
  passage one wide, three saws in its floor, to door "1".
- The west wing behind the red door: a corridor one wide, the switch in its floor at the door (its
  row reaches the wing's saws and the passage's), a mine to jump, three saws; a shaft down to the
  treasure's pocket (walk 22, a crate), a stair back up.
- The key room behind the green door: a band of saws across its floor under a ceiling too low to
  jump, the red key beyond; its switch in a closet in the band's row, reached by a stair down from
  the west region.
- The west region (walk 36), over the wing and the key room: columns, two mines, a moving mine, an
  egg, teleporter 1 hanging over its floor; the tower west, the north corridor north, the blue
  door east.
- The tower (blue blocks, x 6–16): a stair of steps two apart round a core; the first and third
  steps are saws, the first stopped by a switch on a block in the region by the doorway, the third
  by a switch raised on the second; at the top (walk 53) four eggs between poles and four sticks.
- The north corridor (walk 36): two mines to weave between, two mines rising and falling through
  it from shafts, a pit crossed on a lift or hanging from bars; then the crates' front, two mines
  moving up and down at its sides, and the passage one high where nine crates wall in the blue key
  (one stick's blasts take all nine; three are spares).
- North of the front: a corridor along X, its switch at the mouth, three saws in its floor; a step
  and a landing up to the cage (walk 40), its door under the lightning (live half the time), the
  green key and an egg among machine parts.
- The blue door's way (walk 36): the big creature's corridor (it rests at both ends, by the door
  too); stepping stones over a lava trench, the switch on the second, three saws among the rest;
  steps down to the lake.
- The lake (surface 30, a low vault): an island with an egg and a saw; the cavern's west shore (walk
  30) with four overcrafts; lava over the cavern's floor from wall to wall, crossed hovering (the
  overcraft climbs only within 1.25 cells of the ground, D46); beyond it a stair up to the
  north-east.
- The north-east (walk 46): teleporter 2 by two crates and a mine; bars over a gap; the garage of
  four jeeps on grates and a lift (2 × 2, 40 ticks) down to the mine row (walk 42, one wide, seven
  mines: only a jeep passes), the niche of three eggs at its end; lava.
- Door "1" (walk 30): lava over the floor but two lanes of crumbling bridges, a moving mine on one;
  the hole on the far side drops to the funnel (walk 5): the arrow on a pedestal, two eggs.

**Checks**: stage 1: `levelcheck.py` reaches all twelve eggs, the three keys, the treasure and the
arrow; the real world stops where switches, dynamite, the jeep, swimming and the doors stand in.
`VerifyShippedWorlds`. Live (`--mission 65`, spawns in each part, direct view F3): the hall, the
wing, the pocket, the key room's band, the region, the tower inside and its top with the sticks,
the corridor and the crates, the cage with its curtain, the creature's corridor, the stones, the
lake's island, the overcrafts, the north-east and its teleporter, the bridges, the funnel. Played
live: an overcraft mounted and hovered over the cavern's lava to the far floor. Played through,
every part, by `tools/autoplay/world065.txt`: the tower's saw steps and switches, a stick's blasts
on the nine crates and the blue key, the creature's corridor, the stones, the lake, an overcraft
over the lava, a jeep on the lift over the mine row, the niche's eggs, teleporter 2, the bridges
and the funnel (ten eggs, the most taken). It found D59 (two sticks taken from the tower).
