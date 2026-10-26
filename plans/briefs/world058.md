# Mission 058 — the lava mine and world 5's golden key (world 5, region 21)

Built by `tools/worlds/world058.py` → `worlds3d/world058.vwr` (mission 58, region 21 as the
original: a rock cave lit by fire); `--stage K out` writes the staged variants for the checks.

## Reference

`worlds/world058.txt`, played at 5:15:10–5:21:52 of the video (`build-probe/world5-agent/video/w058/`;
0/10 → 10/10 at 5:21:12, then the golden key and the main hub). World 5's last test in a mine lit
by lava (68): Blupi starts in a room whose way on is shut by the red door (334); the red key
(ObjectType49) lies in a hall above it on stepping stones over lava, over the corridor beyond the
door, where four mines lie and three ride up and down (20 ticks); a great cave of slime islands
(341–363) follows, enemy tanks (ObjectType33) on them and in a tunnel to the first treasure, drinks
(30) to pass them unseen, the golden key (ObjectType21) in the middle, counted only with all ten
treasures; east of it a corridor of tanks, a room where five crates (12) plug a tunnel to seven
treasures, a shaft of three little lifts shifting a cell (20 ticks, stop 20) climbed with a
lollipop's high jumps (26), and on top a helicopter (13) flown — through flight barriers (214),
walls on foot that only a flyer passes — to the last two treasures on a slime pillar under enemy
helicopters (32) and a sweeping mine, and back to the golden key. 10 treasures, 7 eggs, 8 tanks,
5 drinks, 10 mines, 4 moving mines, 1 hanging mine, 2 enemy helicopters, 5 crates, 3 lifts, the
lollipop, the helicopter, the red key and door, the golden key.

## 3D design

Raw cells, Y up.

| Part | Where | Lesson |
|---|---|---|
| A, the start room | x 10–18, z 78–88, walk 20; spawn (14, 20, 86) facing −Z | The red door in the east wall at the floor (three wide, two high); two steps up in the north-east corner (walk 22, 24) and a jump into a high doorway to the lava hall; a ball machine, an egg |
| K, the lava hall | x 20–40, z 79–87, walk 25, over the corridor | A floor of lava; four stones, each a jump over one cell of it, to the pedestal with the red key |
| B, the mine corridor | x 20–41, z 82–84, walk 20 | Five mines across the lanes (jump them), then three gates one cell wide, a mine riding up and down in each (20 ticks, alternately): pass while it is up. Taken back with the key, the door opens (the key in hand, no button) |
| C, the cave | x 42–78, z 52–96, floor walk 8, open above to the fire | The rim (walk 20) where B comes in; islands floating in slime, each a jump over one cell and two down: I1 (walk 18, a drink, an egg), I2 (walk 16, a tank), GK (walk 14): the golden key under a slime arch, a tank across its north side, an enemy helicopter over it. Lava pools in the floor; a tank sweeping before the tunnel, a drink |
| T, the tank tunnel | x 29–41, z 61, walk 8, two high, one wide | Two tanks drive it and fire along it at each turn: step into an alcove (x 37, x 32) while one turns, or take the drink at the mouth; treasure 1 at its end |
| D, the corridor of tanks | x 80–96, z 59–63, walk 8 | Three tanks swing across it, firing across at each turn; two drinks; two mines |
| E, the crate room | x 82–92, z 48–57, walk 8 | Entered through a doorway one wide with a mine hanging in it (crouch under); three eggs. The tunnel north (x 89, z 42–47, one high) full of five crates: pull one out (crouch facing it, Down), walk round it, pull the next; the vault (x 86–92, z 35–40) with seven treasures |
| F, the lollipop shaft | x 94–98, z 49–53, walk 8 up to 24 | The lollipop at its foot (very high jumps for 15 s); three platforms (2 × 2) each four above the last and beside it, alternately east and west, each shifting a cell along Z (20 ticks, stop 20); the top ledge four above |
| G, the deck | x 80–98, z 55–72, walk 24, on top of the east works | The helicopter, two mines; its west edge over the cave a row of flight barriers (214): a wall on foot (no falling back into the cave), which the helicopter flies through |
| P, the slime pillar | x 60–62, z 84–86, top walk 30 | The last two treasures, a mine sweeping across between them (60 ticks), an enemy helicopter over it: flown to, and from it down to the golden key |

Object counts as the original's (the lifts as 2 × 2 platforms).

## Checks

- Staged `tools/flightcheck.py --mine-margin 0` (`build-probe/world5-agent/stagefc.py 058 0 … 5`;
  levelcheck's own mine margin cannot jump B's mines; 1 the tanks, helicopters and moving mines
  timed and the low mine crouched, 2 the red door open, 3 the crates pulled, 4 the lollipop's jumps
  (stand-in lifts), 5 the flights (stand-in lifts)): the key only; the same; the cave, T's treasure,
  the eggs and the golden key's island; + the vault's seven; + the deck and, with the helicopter,
  P's two (OK); OK on foot with stand-ins. (A first build had the lava hall's lava floor right over
  the corridor: levelcheck lets a falling Blupi through lava, whose collision mask is empty, so a
  layer of plates now lies under it.)
- `VerifyShippedWorlds worlds3d`: 80/80.
- Live (`--mission 58`, probes through `QA_LIVE_WORLDS`, `build-probe/world5-agent/shots/058-*`):
  views of every part; a probe with the key in the start room: taken (HUD), the red door's bars
  slide up and Blupi walks into the corridor; the gates' mines bobbing and killing a careless walk;
  the jump from the rim onto I1 (markers on it collected); the first crate pulled out of the tunnel
  into the room, and from inside the tunnel the next one (after the engine's crate fix D50: before
  it a crate with another behind it could not be pulled — reported); the lollipop licked and the
  four jumps up the shaft to the top ledge (marker taken); the helicopter flown off the deck through
  the flight barrier over the cave (after the engine's fix D49: 214 blocked flyers — reported); with
  no treasures, the golden key ends world 5 and Blupi arrives in the main hub.
