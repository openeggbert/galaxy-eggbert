# Mission 093 — the pipe works (world 9, region 27)

Built by `tools/worlds/world093.py` → `worlds3d/world093.vwr` (mission 93, region 27 as the
original); `--stage K out.vwr` writes the staged variants for the checks.

## Reference

`worlds/world093.txt`, played at 2:44:20–2:50:32 of the video (`build-probe/video-evidence/w093/`;
0/2 → 2/2 at 2:48:40). Clods under the red cave crossed by blue pipelines (251–253 between
machine joints 10/11/22) on girder piers. Blupi starts in the west, jumps mines in low passages,
comes to a cistern of blue machine under four enemy helicopters (32), dives in and swims down,
along a water pipe and up into the works; pushes crates (12) to climb up to a pipeline, walks it
to a machine box with the two treasures (5), and the treasure door (422) opens with both; a lift
(80 ticks, stop 10); a second water pipe swum through; the red arrow (7) in a machine box guarded
by enemy helicopters and enemy tanks (33), passed with the drink (30, Hide), which lies in a
pocket behind two mines riding up and down (16). 2 treasures, 12 eggs, 15 mines, 3 moving mines,
11 enemy helicopters, 5 tanks, 5 crates, the drink, the lift.

## 3D design

Raw cells, Y up; a mass of clods (x 4–95, z 4–95, up to y 40).

| Part | Where | Lesson |
|---|---|---|
| S, the start | x 8–18, z 80–90, walk 20; spawn (13, 20, 86) facing −Z | Lumpy cave, an egg |
| M, the mine passage | x 12–14, z 61–79, three high | A mine to walk round, then two rows of three across it: each jumped; a niche of three eggs |
| R, the cistern room | x 6–26, z 38–60, walk 20 | The cistern (machine, rim walk 22: a jump up), water from y 15 to 21; four enemy helicopters hover over it dropping shots. At its bottom an outlet three by three runs east under the floor and rises into the hall's basin: the only way on (dive, swim along, swim up). An egg at the bottom; props on columns |
| P, the hall of pipes | x 32–78, z 20–60, walk 20, roof y 38 | Out of the basin. The plinth (machine, walk 23) is three high; five crates lie round it: push one against it, climb, jump. From the plinth a pier (walk 25) and pipeline P1 east on machine piers, a mine on it to jump, to the treasure box (machine on four piers) with the two treasures; out east along P2 (a mine, an enemy helicopter over it) to the treasure door "2" (422) in the east wall. Eggs on a shelf, mines, an enemy tank firing along Z, plants |
| L, the lift | behind the door, walk 25 → 17 | A corridor (a mine sweeping across, 20 ticks); four lifts in step down eight (80 ticks, the original's; its 10-tick dwell made 40 to step on and off in 3D) to a corridor south (a moving mine across it, a mine) |
| W2, the second swim | the well at x 86–88, z 61–63 | Its surface level with the floor; down six, a pipe three by three south and west, up into the court's east basin |
| G, the guarded court | x 40–84, z 66–90, walk 12 | The drink's pocket in the south-east corner behind a mouth two wide where two mines ride up and down three cells (30 ticks, opposite phase): step through under one. A bank of clods three high (x 51–62) cuts the court in two; the lane through it (z 78, one wide) is the only way west: two tanks drive in it firing along it at both ends of their runs (their bodies are harmless, their shots kill), three enemy helicopters over it dropping shots. West of it the exit box (machine, its doorway north, out of the lane's line of fire) with two more helicopters round it, the arrow inside |

## Checks

- Staged `flightcheck.py --mine-margin 0` (`world093.py --stage K`; levelcheck's own mine margin
  cannot jump the rows; no helicopter in this mission): 1 (the swims as stand-in lifts) the hall
  floor, not the plinth; 2 (+ a crate at the plinth) the treasures; 3 (+ door open) all but the
  court's west part; 4 (+ enemies and moving mines timed or unseen) everything (OK). D41 audit
  clean. `VerifyShippedWorlds worlds3d` 80/80.
- Live (`--mission 93`, `build-probe/world9-agent/shots/093*`): views of every part; the dive from
  the cistern's rim, along the outlet and up into the basin, out onto the hall floor with air to
  spare; a crate pushed against the plinth, climbed, the jump onto the plinth and the pier; the
  mine on P1 jumped, into the treasure box; both treasures taken (probe with them on the path):
  the door "2" opens and Blupi walks through; the lift ridden down; the dive into the well and the
  swim through the pipe up into the court's basin; the pocket's two mines riding up and down;
  without the drink Blupi dies in the lane's shots (tried twice); with the drink taken (with Action,
  Space, as the original's Decor.cpp:6053 and the lollipop) he walks the whole lane unhurt; the arrow
  touched (probe without treasures): mission won, back in the hub.
- Not verified live: a run of the whole mission in one go; the four helicopters over the cistern
  hitting a slow diver.
