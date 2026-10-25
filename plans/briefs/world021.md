# Mission 021 — the palace garden (white palace)

Built by `tools/worlds/world021.py` → `worlds3d/world021.vwr` (mission 21, region 25).

## Reference

Mobile Eggbert's `worlds/world021.txt`, played at 3:03:31–3:05:28 of the full-game video
(`video-evidence/w021/`): world 2's first lesson, treasure doors. On the grass before the
palace stand three stepped white towers with balconies; the treasures lie in an arch at the
first tower's foot, on its balconies beside saws (378), on the second tower behind a golden bar
marked 2 (422, opens once Blupi has two treasures), on a slim column reached by the mission's
one lift (40 ticks, stop 10) and at the third tower's foot behind the bar marked 5 (425); in
the palace he climbs a tall shaft to the high hall for the sixth, and the bar marked 6 (426)
shuts the room of the red arrow. 6 treasures, 4 eggs, 5 saws, 1 lift; no hints.

## 3D design

A walled garden on grass (x 22–78, z 36–82, walk 1) with white paths, palms, plants and
fountains (BigDecor), and the palace across its north (x 24–76, z 12–35). Saws are floor cells:
the blade stands up out of the floor, and stepping on it kills.

| Part | Where | Lesson |
|---|---|---|
| Start | spawn (44, 1, 77) facing −Z | the garden, tower 1 ahead left |
| Tower 1 | x 29–39, z 61–71: tiers walk 3, 5, 7 | treasure 1 in the middle arch of its foot (34, 1, 71); treasure 2 on its top (34, 7, 66) over a balcony with four saws, one in the middle of each side |
| Tower 2 | x 57–67, z 55–65: tiers walk 3, 5 | treasure 3 in a kiosk on the second tier (63, 5, 60) behind the bar marked 2, three saws in the floor before its door; an egg on the first tier |
| Tower 3 and the column | x 29–39, z 40–50: tiers walk 3, 5 | treasure 4 on its top between four saws (jump onto it); the lift (38, 4→8, 45) from the top's east edge to the slim column (39, z 45–46, top walk 9): treasure 5 on one cell, a saw on the other |
| Gatehouse | x 46–54, z 36–40, open on top, walls 4 high | its door from the west (46, 1, 38) the bar marked 5; an egg inside; through the facade into the court |
| Court | x 26–74, z 14–32, open | a fountain with an egg; a staircase of two-block steps up its east side (saws on two steps) to the wall tops (walk 10) and the north battlements: fountains with two eggs, the north-east tower (walk 12) with treasure 6 and the statue |
| The arrow's room | x 26–35, roofed (walk 1–4) | entered from the court through the bar marked 6 (36, 1, 23); the red arrow on a plinth between palms |

The palace walls stand ten high and the arrow's room is roofed, so the battlements and wall
tops (walk 10) lead nowhere but to the sixth treasure. Tiles: white stone 396 with trims and
windows (391, on the +Z faces), arches 400 as blind arches, grass 107; saws 378; treasure doors
422, 425, 426.

## Checks

- `levelcheck.py` on the world: the treasures of towers 1 and 3 and the column reachable, tower
  2's, the palace's and the exit not. Staged (`world021.py --stage N`, the doors up to N open,
  `build-probe/world2-agent/stagecheck.py 021 0 2 5 6`): 4 treasures with no door open, 5 with
  the 2 open, 6 and all eggs with the 5 open, the exit with the 6 open. Without the lift the
  column's treasure is unreachable.
- `VerifyShippedWorlds worlds3d`: 80/80.
- Live (`--mission 21`, `build-probe/qa-shots/world2/w021/`): the garden from the spawn; treasure
  1 picked up in the arch (0/6 → 1/6); the bars 2, 5 and 6 in their doorways; tower 3's treasure
  between its saws; the lift rising and falling beside the column; the court's staircase, the
  battlements with a fountain and the north-east tower; the arrow between the palms.
