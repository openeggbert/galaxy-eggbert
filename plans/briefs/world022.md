# Mission 022 — the brick cellars and the green key (white palace)

Built by `tools/worlds/world022.py` → `worlds3d/world022.vwr` (mission 22, region 25).

## Reference

Mobile Eggbert's `worlds/world022.txt`, played at 3:05:31–3:12:47 of the full-game video
(`video-evidence/w022/`): from a balcony over a lava pit Blupi collects three treasures past a
water channel one deep, one of them high up by pushing a crate under it; he goes down into the
brick halls of crates and brick lifts (30 ticks), one of them carrying him sideways, and a pool
two deep with a treasure at its bottom; a shaft of four lifts one above the other (30, 40, 40
and 80 ticks) has three treasures on its side ledges; the bars marked 4 (424) and 7 (427) open
the way up to the library of bookshelves, whose eighth treasure lies between saws; four bars
marked 8 (428) keep the green key (ObjectType50), and the key opens the green doors (335) of the
red arrow's shrine. 8 treasures, 8 eggs, 15 crates, 8 lifts, 11 saws; no hints.

## 3D design

Three parts climbing round each other, with a fall off the roof terrace a fall to death:

| Part | Where | Lesson |
|---|---|---|
| A. Roof terrace | x 37–51, z 56–78, walk 11, parapets; spawn (44, 11, 76) facing −Z | a lava trench (z 73) to jump; treasure 1; a water channel one deep (z 67–68); treasure 2; a plinth three high (x 43–45, z 58–60) with treasure 3, climbed from the crate pushed two cells −Z to its foot; its north end drops into the court |
| B. Brick court | x 29–61, z 23–55, walk 1, walls 14 high | five crates on the landing, one pushed to the foot of the south gallery (walk 4); a lava trench (z 34–38) crossed on a lift three wide (30 ticks); the north gallery with the pool (two deep, treasure 4 at its bottom), two saws, and a brick lift (30 ticks) up to the ledge (walk 7) and the bar marked 4 |
| C. Lift tower | yard x 63–77, z 18–33, walk 7, open all round | four lifts between white piers, one above the other: walk 7→10 (30 ticks), 10→14 (40), 14→18 (40), 18→26 (80); treasures 5 and 6 on side ledges one jump from the piers, treasure 7 on the top landing; a crate before the bar marked 7, pushed aside |
| D. Upper hall | x 30–74, z 4–16, walk 26, walls to y 33 | columns, the ball machines and the statue (BigDecor); a passage three wide with saws across its floor; the library: a wall of bookcases parts its aisles, treasure 8 in the north aisle beyond a row of saws; at its west end the shrine of the green key behind the bar marked 8 and the shrine of the red arrow behind the green door |

The original's water depths are kept: the channel one deep (the surface tile in the floor
row), the pool two deep with its treasure in the lower cell. Doors are crossed along X or Z;
every lift has the original's timing.

## Checks

- Staged `levelcheck.py` (`world022.py --stage K`, `build-probe/world2-agent/stagecheck.py 022
  0 1 2 3 4 5 6`): treasures reachable 2 (as built), 3 (terrace crate at the plinth), 4 (court
  crate at the gallery), 7 (bar 4 open), 8 and all eggs (crate moved, bar 7 open), the key
  (bar 8 open), the exit (green door open). Without the lifts the tower is unreachable.
- `VerifyShippedWorlds worlds3d`: 80/80.
- Live (`--mission 22`, `build-probe/qa-shots/world2/w022/`): the lava trench jumped, the
  channel crossed (one deep: Blupi walks out), the crate pushed to the plinth and the plinth
  climbed from it (treasure 3, 0/8 → 1/8); the court crate pushed to the gallery and the gallery
  climbed; Blupi carried across the lava trench on the lift; the lift tower's piers and lifts;
  the top landing's crate and bar 7; the saw passage, the library aisles, the green door.
- Open (engine): Blupi cannot climb out of water two deep — at the surface he sinks back
  (reported 2026-10-26); until that is fixed the pool's treasure cannot be brought out.
