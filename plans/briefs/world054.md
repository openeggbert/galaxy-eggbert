# Mission 054 — the sky tower (world 5, region 26)

Built by `tools/worlds/world054.py` → `worlds3d/world054.vwr` (mission 54, region 26 as the
original: a blue sky); `--stage K out` writes the staged variants for `levelcheck.py`.

## Reference

`worlds/world054.txt`, played at 4:53:46–4:57:57 of the video (`build-probe/world5-agent/video/w054/`;
0/0: no treasures). A climb through a station tower in the sky: in a hall at the bottom Blupi
licks a lollipop (ObjectType26: very high jumps for 15 s) and jumps up a girder shaft from stone
to stone; a relay of six lifts, each meeting the next halfway (50 ticks, stop 10), carries him up
past side rooms of mines, hanging mines (28) and sweeping mines (12); two little lifts bob a cell
(6 ticks, stop 3) under hanging mines; a slime pocket (341–363) with eggs and a wasp; a wasp's
balloon carries him up a well past a sweeping mine into a room of machines and columns (BigDecor
28–30, 66–69); a lift rising out of a gap (20 ticks); then down through slime past an enemy tank
(ObjectType33) and three enemy helicopters (ObjectType32) dropping shots, to the red arrow. 8
eggs, 9 lifts, the lollipop, 2 wasps, 1 tank, 3 helicopters, 16 mines, 28 hanging, 12 moving.

## 3D design

One tower, climbed from its foot (walk 5) to the machine room (walk 67), then down its east
side. Raw cells, Y up.

| Part | Where | Lesson |
|---|---|---|
| A, the hall | x 31–51, z 58–71, walk 5; spawn (35, 5, 69) facing −Z | Ball machines on plinths, two girder masts; the lollipop at (45, 5, 59) by the shaft's door |
| B, the shaft | x 42–48, z 50–56, open above | Four pillars spiralling round its walls, each touching the last and four higher (walk 9, 13, 17, 21), the ledge under the doorway out at walk 25: one licked jump each, none without it; an egg on P3 |
| C, the relay hall | x 42–56, z 58–68, walk 25 up to the landing at 55 | Six lifts, each a pair along Z one cell further east (50 ticks): A 24→29, B 34→29, C 34→39, D 44→39, E 44→49, F 54→49; each meets the next halfway and they wait 20 ticks there (the original's 10 is a 2D step, too short to judge in 3D). Three galleries on the north side where two meet: G1 (walk 30) two eggs behind a sweeping mine, a mine hanging over its mouth; G2 (walk 40) six mines hanging at Blupi's height, an egg behind (crouch); G3 (walk 50) a row of mines to jump, an egg. Two mines bob across the landing's way out |
| D, the bridge | x 57–62, z 62–63, walk 55 | Two pads (2 × 2 lifts) bob a cell under the line in opposite phase (6 ticks, stop 3), a mine hanging over every pad cell at Blupi's height under a low roof: crossed crouching, a jump meets the mines |
| E, the corridor and the pocket | corridor x 63–72, z 59–66, walk 55; pocket x 62–73, z 57–68, walk 47 | A hole in the corridor's floor drops into the slime pocket: three eggs, plants, a wasp under the hole whose balloon lifts Blupi back up into the corridor |
| F, the balloon well | x 73–77, z 60–64, walk 55 up to 67 | A wasp at its foot; the balloon rises twelve cells into the machine room; a mine sweeps across halfway up and pops it if mistimed |
| G, the machine room | x 66–85, z 55–69, walk 67 | Ball machines, columns, two sweeping mines; out east over a gap where three lifts rise two cells to lie flush (20 ticks) |
| H, the slime terraces | S1 x 90–98, z 57–67, walk 67; S2–S4 walk 63, 59, 55 heading south; the cave x 87–98, z 80–90, walk 51 | S1: the tank drives across it (z 65) and fires along its line; S3: an enemy helicopter dropping shots; the cave of the arrow under a slime roof with two more helicopters over the arrow, plants |

Objects: 8 eggs, 23 lifts (the original's 9, most of them pairs), the lollipop, 2 wasps (+1
balloon wasp in the well), 1 tank, 3 helicopters, 3 mines, 15 hanging, 6 moving. Fewer mines than
the original's 56 — its side rooms full of mines are here the three galleries and the pads.

## Checks

- Staged `levelcheck.py` (`build-probe/world5-agent/stagecheck.py 054 0 … 4`; 1 the lollipop's
  jumps (stand-in lifts pillar to pillar), 2 the sweeping mines and the enemies timed, 3 the pads
  and G2 crouched, 4 the balloon up the well (a stand-in lift)): nothing; G1, G3 and the shaft's
  egg; the same; the pocket's eggs and G2's; every egg and the exit.
- `VerifyShippedWorlds worlds3d`: 80/80.
- Live (`--mission 54`, probes through `QA_LIVE_WORLDS`, `build-probe/world5-agent/shots/054-*`):
  views of every part; the lollipop licked (gauge, sparkles) and the licked jumps: P1 from the
  floor and P2 from P1 (treasure markers on P2 collected, 2/6), P4 to the ledge — a first build
  with a cell of gap between pillars could not be cleared even licked, so the pillars now touch;
  the relay A→B→C→D in one run (markers at the meeting cells, 3/3); the pads crossed crouching
  under their mines, a jump there killed; the pocket's wasp balloon back up into the corridor and
  the well's balloon into the machine room; the walk down the terraces into the cave to the arrow,
  which ends the mission (enemies out); the tank and the helicopters' shots in place.
