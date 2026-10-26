# Mission 046 — the cheese moon and world 4's golden key

Built by `tools/worlds/world046.py` → `worlds3d/world046.vwr` (mission 46, region 9 as the
original: the cheese moon). `--check STAGE out` writes the staged variants (below). World 4's
tiles and pieces: `tools/worlds/cheese.py`.

## Reference

`worlds/world046.txt`, played at 2:05:45–2:31:08 of the video (no treasures, 0/0; about nine lives
lost; the golden key, then the main hub). One long climb from the bottom left to the top right of
a great slope of cheese among blue machine girders (10) under the cheese moon. Blupi starts below
beside a pool (91/92); the only helicopter (13) fetches the red key (49) from a machine tower, the
red door (334) opens; up the slope mines (2) lie on its steps and moving mines (16, 20–40 ticks)
sweep across it; a chain of six lifts in step (1, 80 ticks, 20 stop) carries Blupi up over a chasm,
each handing him on to the next; a cage of toy balls (215) with candy poles holds the lollipops
(26) whose high jump reaches a machine deck with the green key (50) and the green door (335); two
drinks (30) make him invisible to the moving mines of the gallery beyond; a crate (12) climbs to
the blue key (51); the blue door (336) opens on the last lift, and the golden key (21) waits on
the summit. 21 mines, 7 moving mines, 8 eggs, 11 lifts, 3 lollipops, 2 drinks, 1 helicopter,
crates, the three keys and doors.

## 3D design

A cheese mountain on the moon: the start court at its south-west foot, the stair up its south
face, the chasm and its lift chain at the south-east corner, the plateau and the deck on its top,
the summit massif in the middle of the plateau with the gallery, the crate cave and the last lift
inside it. Raw cells, Y up.

| Part | Where | What |
|---|---|---|
| A, the court | x 8–24, z 78–94, walk 10, walled and roofed | Spawn (16, 10, 90); the helicopter; a machine tower twelve high with the red key on top (flown to); an egg. The red door (334) at (24, 10, 80) in the east wall |
| B, the pool room | x 25–33, z 77–83 | A pool three deep across it, a curtain wall down into it: Blupi dives under (the helicopter cannot pass: it is wrecked in water, and the curtain closes the air above); the way out east |
| C, the stair | x 35–76, z 72–74, walk 11 → 31 | Twenty-one steps two deep and one high above the foot path (walk 10, z 75–80) with machine girders along it. Four rows of mines in two of the three lanes of a step, the free lane changing from row to row (a weave); two moving mines sweeping across steps 6 and 14 (30 ticks); step 10 a gap with lifts bobbing two cells (20 ticks, 20 stop); a fall lands on the foot path. Two eggs |
| D, the chasm | x 77–84, z 46–80; landing walk 31, chasm floor walk 27 | The chain of six lifts (80 ticks, two wide and two deep) climbing north from the landing to the terrace (walk 31 → 35 → 39 → 42 → 45 → 48 → 51), alternately up and down, a rest column between each two at their meeting height; each waits two seconds at its ends. Holding forward from the right moment Blupi is carried up; a mistimed step drops him to the chasm floor, steps lead back to the landing |
| E, the plateau and the deck | the mountain's top, walk 51; deck x 61–84, z 30–34, walk 57 | The cage of toy balls (215) three high with candy poles (236), two lollipops inside, a third outside; two mines. The deck, six above the plateau: only the lollipop's high jump reaches it; the green key, an egg, pipes (53–60), a machine (BigDecor 31); the green door (335) at (60, 57, 32) in the summit massif |
| F, inside the summit | the massif x 30–60, z 30–62, up to y 70 | The drinks hall (walk 57) behind the green door: a lava strip to jump, the two drinks, three eggs, two mines. The gallery west and south, three wide, climbing in two-deep steps to walk 63, four moving mines sweeping across it (20–40 ticks): with a drink's twenty seconds Blupi walks through them. The crate cave (walk 63): the blue key on a ledge three high, the crate to push under it, an egg, a mine. The blue door (336) at (40, 63, 57), a chamber a cell deep before the last lift (48 ticks, two seconds' wait at the bottom: jumped onto) up to the summit |
| G, the summit | walk 71, under the moon | Craters, three mines and a moving mine round the golden key at (45, 71, 45) |

3D decisions: the original's chain hands Blupi from lift to lift in mid-air, some moving across;
in 3D the six lifts climb in relay with rests between them (as 042's relay) and wait two seconds
(the original's one-second wait is too short to cross a cell); the stair's mine rows leave a free
lane (a full row on a two-deep step left almost no landing room); the last lift has a cell before
it so Blupi does not walk under it while it is up (it comes down round him harmlessly, checked
live). Objects: 16 mines, 7 moving mines, 8 eggs, 34 lift cubes (the chain's lifts are four cubes
each), 3 lollipops, 2 drinks, 1 helicopter, 1 crate, the three keys and doors, the golden key.

## Checks

- `flightcheck.py --mine-margin 0` in stages: the world itself — only the court and the red key
  (with the helicopter); `--check red` (red door open; the dive modelled for levelcheck by a
  raised pool floor) — also the stair's foot, the moving mines stop it; `--check timed` (moving
  mines left out) — also the stair, the chain and the plateau, not the deck; `--check power` (a
  stair where the lollipop's high jump goes) — also the deck and the green key; `--check green`
  — also the drinks hall, the gallery and the crate cave, not the blue key; `--check solved`
  (crate under the ledge, blue door open, a step at the last lift) — the blue key and the golden
  key: OK. From the green key, the blue key and the plateau the way on to the golden key exists
  (scratch check).
- `VerifyShippedWorlds`; objects on floors, nothing sunk into the cheese (scratch check); the
  court is closed but for the red door, the pool room opens only on the foot path, the summit
  interior only through the green door and the last lift's shaft (scratch checks).
- Live (`--mission 46`, `--spawn`, probes through `QA_LIVE_WORLDS`, authored lift timing of
  3b0a861): the start view (tower, helicopter); the helicopter flown up to the red key on the
  tower; with a probe red key the red door opened, the pool dived under its curtain and the bank
  climbed; the stair hopped past the first mine row in its free lane (and a blind run dying on
  the moving mine); the whole lift chain ridden holding forward to the terrace (and a mistimed
  start dropping onto the chasm floor); a lollipop taken and the high jump onto the deck; the
  gallery's moving mines walked through with a drink and fatal without; a crate climbed to the
  blue key, the blue door opened with it; the last lift ridden to the summit; walking to the
  golden key ends the world and loads the main hub.
