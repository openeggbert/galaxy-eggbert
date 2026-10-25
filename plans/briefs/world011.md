# Mission 011 — the first lesson (blue machine world)

Built by `tools/worlds/world011.py` → `worlds3d/world011.vwr` (mission 11, background region 0).
Validated by `tools/levelcheck.py` and the `VerifyShippedWorlds` CTest.

## Reference

Mobile Eggbert's `worlds/world011.txt`, played at 0:28–1:46 of the full-game video
(`plans/evidence/video-timeline.csv`): a corridor that teaches, left to right, move → jump onto a
pillar for treasure 1 → jump two mine pits → a water channel → an elevator taken quietly → an
elevator jumped onto → hanging mines (no jumping) → three moving platforms (slow, faster, long then
jump) → the upper path with two eggs over the mines → two lifts that meet at the top → treasure 2 →
the red arrow. 2 treasures, 2 eggs, the exit. Hints: `Tables::table_training1`, 22 records
(`TrainingHints.cpp`), placed here as hint zones.

## 3D design

A square spiral of 5-wide machine walkways (3-wide lane between parapets, walking level y 10) round a
central machine tower, in the original's order. Falling off a walkway is a fall to death, as the
original's mine pits were. Raw cells, Y up.

| Part | Where | Lesson (hint record) |
|---|---|---|
| Start | x 61–67, z 57–62; spawn (64, 10, 60) facing −Z | Move (0) |
| East walkway, heading −Z | x 62–66, z 38–56 | Treasure 1 on a 2-high pillar at (65, 12, 54): jump (1); mine pits z 51, 47 (2, 3); water z 42–43 (4) |
| North walkway, heading −X | z 32–36, x 38–60 | Elevator 1 at x 58 up a 3-high wall (6); trough, elevator 2 at x 53 jumped onto and off onto wall 2 (7); gantry x 44–48 with nine hanging mines 2 above the lane (9); mine pit x 41 |
| West walkway, heading +Z | x 32–36, z 38–62 | Three chasms crossed on 3-wide platforms: 40 ticks (12, 13), 24 ticks / 3 stop "faster" (14), 60 ticks stopping one cell short — jump (15, 16) |
| South walkway, heading +X | z 64–68, x 38–52 | Fork (17): the lower lane runs into a 1-high tunnel of six mines; the upper path (2 up) runs over it with the two eggs (18) |
| Paired lifts | x 50–51 over the moat z 55–60 | Lift A rises from the path, lift B from the terrace; both meet at the top: step across (19) |
| Tower | terrace x 43–57, z 40–54 (walk 12); tower x 46–54, z 43–51, roof walk 18 | Treasure 2 behind the tower at (50, 12, 41) (20); elevator up the east face; the red arrow on the roof (21) |

Timings are the original's (`stepAdvance`/`timeStop` ticks). Records 5, 8, 10 and 11 have no text
and are not placed. Tiles: the machine panel 10 with the opaque machine cubes 2–14 mixed in, monitors
(1), small machine parts (53–60) on the corner parapets and the tower roof.

## Checks

- `levelcheck.py`: both treasures, both eggs and the exit reachable; removing any elevator or any
  group of platform lifts makes the exit unreachable (each lesson is on the only route).
- Live (`tools/qa_live.sh … --mission 11 --spawn x,y,z,yawDegrees`): every hint appears in its
  section.
