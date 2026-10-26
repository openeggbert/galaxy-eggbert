# World 062 — mission 2 of world 6 (the slime labyrinth)

Built by `tools/worlds/world062.py` → `worlds3d/world062.vwr` (mission 62, region 4 as the
original). Staged variants for `levelcheck.py`: `--stage K out.vwr` (K ≥ 1 no creatures or moving
mines, the shuttle a floor; K ≥ 2 the key doors open, the flooded way as floors; K ≥ 3 the exit's
door open).

**Reference** (`worlds/world062.txt`, video 5:32:32-5:44:35): a labyrinth of slime tunnels, Blupi
starting in a blue machine structure top left, the red arrow far top right behind a door that
wants all six treasures (426). Two keys and their doors: the green key (50) opens 335, the blue
(51) opens 336. On the way a column of lightning (305 under its emitter 304, live half of each
5 s), lava crossed on crumbling bridges (364: they give way under Blupi and come back), rows of
saws (378), four big creatures, nine moving mines, spike traps (373), six lifts, a drip (404),
girders (201) and a water channel; twelve eggs.

**3D design**: an upper level (walk 21), a lower one (walk 11) and a flooded one under them.
- U1, the machine start room (x 18–28, z 26–36): lined with machine panels (faces on the walls
  only), a girder across; spawn (23, 21, 34) facing −Z.
- T1, the tunnel east (three wide, three high): a curtain of lightning across it at x 37 under its
  emitter; Blupi passes in its quiet half.
- U2, the lava gallery (x 45–61, z 22–40): lava over the floor but a rim; treasure 1 on an island
  reached on crumbling bridges from west and east; treasure 6 on a pillar in the lava, reached from
  a shuttle off the south rim (walk 22, one step up). A mine in a corner.
- U3 north: a corridor five wide with three rows of saws right across it (jump each), then a room
  with the green key and treasure 2.
- U4 east: the exit room behind the six-treasure door, a girder under its ceiling.
- Down: a hole in U2's south rim drops to the lower level; a lift (40 ticks) brings Blupi back.
- L1, the creatures' hub: a big creature (80 / 200) rests in the doorway west and walks off it; a
  drip, a mine. West of it a cross corridor (three wide) under nine moving mines in three columns
  that rise and fall in turn: Blupi passes a column while it is up and waits between columns. Then
  a room with a strip of spike traps right across it; the blue key and treasure 3 beyond it.
- L3 behind the green door: a corridor one wide along X; a creature (60 / 200) walks its east half
  and rests at both ends, the east one before treasure 4.
- Behind the blue door (L2's south side): a ledge over a pool, a step to climb back; a channel three
  high, flooded, dived east (16 cells) to a second pool whose bank (one above its water) holds
  treasure 5.

**Checks**: `levelcheck.py` stages: K=1 six of nine targets (the upper level, L1, L2); K=2 adds L3's
and the dive's treasures; K=3 the exit; every egg from K=2. The real world fails where creatures,
moving mines, the shuttle and the water stand in. `VerifyShippedWorlds`. Live (spawns in each
area): the machine room (floor faces dropped after green slime showed through their open tops),
the lightning curtain, the lava gallery's island and bridges, the saw rows, the creature at the
hub's doorway, both pools with the ledge, the door and the bank.
