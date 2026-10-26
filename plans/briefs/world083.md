# World 083 — mission 3 of world 8 (the grey works)

Built by `tools/worlds/world083.py` → `worlds3d/world083.vwr` (mission 83, region 15 as the
original: a grey sky). Staged variants for `levelcheck.py`: `--stage K out.vwr` (K treasures taken:
door "1" open from K ≥ 1, door "3" from K ≥ 3; from K ≥ 1 no mines or moving mines, the shuttles
and the saucers of the arrow's hall as floors).

**Reference** (`worlds/world083.txt`, video 6:14:32-6:18:46): a machine of panels under a grey sky,
three treasures. From Blupi's start a shuttle (50 ticks) over a pool and a climb to treasure 1;
door "1" (421); a cavern of platforms joined by Temp saucers (324) over pools of water and lava; a
long lift (90 ticks); rooms with moving mines (16), two mines, bars (138) and treasure 2; a lift
moving across and up at once (50 ticks) beside mines rising and falling in shafts; door "3" (423)
by the pocket of treasure 3; a bridge of saucers over lava, an egg, a stair of saucers up to the red
arrow; warning panels (48).

**3D design**: the machine round a cavern, inside one block of panel.
- The start hall (x 10–30, z 60–72, walk 30): spawn (11, 30, 67) facing −Z; a pool flush with the
  floor across it, a shuttle (2 × 2) over it; north of the west ledge a stair of steps two apart to
  the ledge of treasure 1 (walk 40), a warning panel at its foot; door "1" in the east wall.
- The cavern (x 32–66, z 56–80, walk 14): a ledge behind the door; platforms at walk 22, 20, 18, 16
  east across it joined by chains of saucers two wide; its floor panel with pools of lava and water,
  a stair back up at its west end.
- The lift column at its east end (2 × 2, fifteen up) to the block of rooms (walk 31): a mine rising
  and falling in a shaft across the way, another sweeping a room, treasure 2, two mines, bars over a
  lava pit.
- The east: a lift moving across and up at once beside two shafts of rising and falling mines, to a
  gallery (walk 36) where door "3" closes the way north; under the gallery the pocket of treasure 3,
  a drop and a stair back.
- The arrow's hall over a deep pit (its floor walk 20, a stair back up to the gallery): a bridge on a
  keel, two lanes of saucers over lava with lava beside them and rests of panel, an egg at its north
  end; a stair of saucers two at a time up to the arrow's ledge (walk 50).

**Checks**: `levelcheck.py` stage 3 reaches the three treasures, the egg and the arrow (and from the
pit's floor back to the gallery); stage 1 stops at door "3". `VerifyShippedWorlds`. Live
(`--mission 83`, spawns in each part, direct view F3): the hall and its pool, the climb, door "1",
the cavern's platforms and saucers, the rooms with their mines and treasure 2, door "3", the saucer
stair. Played through by `tools/autoplay/world083.txt` (the three treasures, the egg, the
arrow): round the pool by the hall's north rim (the shuttle is not needed), treasure 3 by the floor
of the east shafts south of their mines, the lift moving across and up at once boarded from
beside its start, each run over saucers timed to the field `temp`. The first build of this world crashed the game on load (the terrain's vertex buffer passed
the graphics limit), which led to D47: closed machine cubes now hide the faces between them.
