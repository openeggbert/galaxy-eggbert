# Mission 052 — the spiral of vaults round the well (world 5, region 13)

Built by `tools/worlds/world052.py` → `worlds3d/world052.vwr` (mission 52, region 13 as the
original); `--stage K out` writes the staged variants for `levelcheck.py`.

## Reference

`worlds/world052.txt`, played at 4:37:18–4:41:16 of the video (`build-probe/world5-agent/video/w052/`;
0/12 → 12/12 at 4:39:55). One corridor of riveted plates cut by six treasure doors that open with
2, 4, 5, 6, 7 and 10 treasures (422, 424–427, 430). Each stretch's treasures lie in a pocket
above it, reached by a small lift (20 ticks), or in a pit below it; follow stars (ObjectType96,
34 of them) sleep round them and wake when Blupi comes within about two cells, then home on him
slowly (one pixel a tick) and burst against any wall. Past the door "7" a room of eight
helicopters with three treasures only flight reaches; past the door "10" two more, a little
water (92), and a shaft Blupi flies up past more stars to the red arrow. 12 treasures, 34
stars, 8 helicopters, 2 lifts. The stars are the mission's single idea: approach slowly, wake
them one at a time, step back and let them follow into a wall.

## 3D design

A block of plates (x 40–60, z 40–60, y 10–42) round a central well (x 47–53, z 47–53, open to
the sky from walk 11). The corridor winds down round the well as a square spiral, each side
three wide and three high, four cells below the last; each door at a corner, past it a drop of
four into the next side. Raw cells, Y up.

| Part | Where | Lesson |
|---|---|---|
| S1, east, the start | x 55–57, z 46–57, walk 35; spawn (56, 35, 56) facing −Z | A lift (one cell, 40 ticks for four, waiting 30 at each end: the original's 10 is half a second, too short to step off in 3D) rises through the ceiling into the pocket with two treasures; a star sleeps under the ceiling past it; the door "2" at the north end |
| S2, north | x 46–57, z 43–45, walk 31 | Four stars in a row under the ceiling between the drop and the lift: wake them and back away; the pocket above with two treasures; the door "4" at the west end |
| S3, west | x 43–45, z 43–54, walk 27 | A pit four deep (a hole of two cells, a step inside to climb out), a treasure and two stars at its top corners; the door "5" |
| S4, south | x 43–54, z 55–57, walk 23 | A pit with a treasure and four stars along its top; the door "6" |
| S5, east again | x 55–57, z 43–57, walk 19 | A pit, a treasure, four stars a jump above it; the door "7" in the block's north face |
| Hangar | x 42–60, z 20–38, walk 11, roofed at 26 | The ledge where the door opens (walk 19) with four helicopters and a star; four more on the floor. Three treasures only a helicopter reaches, eight stars round them: on a column in the middle (walk 21), in an alcove high in the west wall (walk 17), in a pen three high on the floor |
| Door "10", the well's foot | tunnel x 49–51, z 39–46, walk 11 | The door "10" at the hangar's south wall; water across the tunnel's mouth (a helicopter that meets it is lost, so Blupi walks it); the last two treasures in the well's far corners, four stars over them |
| The well | x 47–53, z 47–53 | Fly up past two stars in its middle (along the sides they stay asleep) to the ledge along its south wall (walk 38): the arrow between two red-dot machines, four stars round it |

Object counts as the original's: 12 treasures, 34 follow stars, 8 helicopters, 2 lifts, the exit.

## Checks

- Staged `levelcheck.py` (`build-probe/world5-agent/stagecheck.py 052 0 … 6`; stage K opens the
  first K doors, stage 6 adds stand-in lifts for the flights): 2, 4, 5, 6, 7, 7, then all 12 and
  the exit — each door opens exactly its side's treasures. `tools/flightcheck.py` on a variant with
  the doors open and no stand-ins: the hangar's three treasures and the arrow reachable with a
  helicopter. (A first build leaked into the well through pipe tiles in its one-thick walls, which
  have no collision sideways: replaced by grilles.)
- `VerifyShippedWorlds worlds3d`: 80/80.
- Re-checked after the engine's lift timing fix (D43): S1's lift ridden into the pocket and stepped
  off at the top (markers taken at three delays) once its dwell was lengthened to 30 ticks.
- Live (`--mission 52`, `--spawn`, probes through `QA_LIVE_WORLDS`; `build-probe/world5-agent/shots/052-*`):
  the start view down S1 to the door "2"; S1's lift into the pocket and a treasure taken; S2's
  stars waking, homing and killing Blupi on contact; a probe with S1's treasures before the door:
  the door "2" opens at 2/12 (once Blupi kept clear of the lift's pit, which is open while the
  lift is up); out of S3's pit by the step; the hangar from the door "7" ledge (helicopters, the
  alcove's treasure, the ledge star waking); a helicopter taken at the well's foot and flown up
  along its side past the middle stars to the ledge; the arrow on the ledge ends the mission.
