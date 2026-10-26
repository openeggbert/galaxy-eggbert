# Mission 024 — the halls of mines (white palace, region 28)

Built by `tools/worlds/world024.py` → `worlds3d/world024.vwr` (mission 24, region 28).

## Reference

Mobile Eggbert's `worlds/world024.txt`, played at 3:18:00–3:27:25 of the full-game video
(`video-evidence/w024/`): Blupi starts beside the red arrow, shut in behind a red, a green and a
blue door, with a crate in the room. A diagonal staircase climbs to rooms of hanging mines
(ObjectType3) and the blue key; a cage of blue bars (51) holds lifts one above the other
(30, 30, 15/66, 60 and 18 ticks), the red key at its top; a long hall over lava has balustrades
(398) and small lifts rising and sinking (20 ticks) to cross it, mines (2), mines moving up and
down (16) and the green key at its end; a flooded part of the palace is swum through past a fish
(17) to a tower of lifts (24 and 12 ticks) with the five treasures one above the other. 5
treasures, 11 eggs, 3 keys, 3 doors, 19 lifts, 13 mines, 3 moving mines, 2 crates, 5 saws.

## 3D design

A start room (x 41–59, z 39–61, walk 11, walls six high) with a wing on each side; a fall off
the open parts is a fall to death.

| Part | Where | Lesson |
|---|---|---|
| Start room | spawn (50, 11, 59) facing −Z | the shrine before him: the red, green and blue doors in a row, the red arrow inside on its open court; an egg on a ledge three high in a corner, climbed from the room's crate pushed four cells −Z |
| North: stairwell | x 49–51, z 15–38 | eight two-block steps up to walk 27, saws on two of them, a mine on another; the top room: a corridor under two rows of hanging mines at head height (walk on, never jump: walking is safe, a jump meets them), the blue key at its end beside a mine, a mine moving up and down the room's east side |
| East: the cage | yard x 60–74, z 42–58, walk 11 | ledges one above the other on the cage's west and east sides (walk 13 … 23) and five lifts between them with the original's timings, each carrying Blupi across to the foot of the next ledge two higher; bars of 51 round the cage; the red key on the top ledge |
| West: the lava hall | x 12–40, z 46–54, lava four below | hanging mines over its entry; balustrades two wide standing in the lava, between them gaps of two with a lift rising into the first cell (step on when it is up, jump the second); a mine on one balustrade, a mine moving up and down beside a jump; the green key on the far platform |
| South: the flooded gallery | x 43–47, z 62–76 | water up to the room's floor; a curtain wall across it (z 65) to dive under (LeftShift), the fish patrolling beyond it at middle depth; out onto the tower's landing |
| The treasure tower | landing x 38–54, z 77–90, walk 11 | a white core with shelves round it: the second crate pushed under shelf 1 (walk 14), jumps to shelf 2 (16), a fast lift (24 ticks) to shelf 3 (20, a saw), a short one (12 ticks) to shelf 4 (22), a jump to shelf 5 (24); a treasure on each shelf, eggs between; a mine moving along the landing |

Keys are used up by their doors and there is one of each colour, so the wings can be done in
any order. The exit needs all five treasures (InteractionSystem: m_nbTresor ≥ m_totalTresor).

## Checks

- Staged `levelcheck.py` (`world024.py --stage K`, `build-probe/world2-agent/stagecheck.py 024
  0 1 2 3`): as built the three keys (each wing needs its lifts: without them the cage's and the
  lava hall's keys are out of reach); + crates pushed: the corner egg; + doors open: the exit;
  + the gallery swum (levelcheck cannot swim: its surface stands in): the five treasures and all
  eleven eggs.
- `VerifyShippedWorlds worlds3d`: 80/80.
- Live (`--mission 24`, `build-probe/qa-shots/world2/w024/`): the shrine's red door from the
  spawn; the stairwell climbed by jumps from the room to step 6; the corridor walked under the
  hanging mines to the blue key (alive); the cage, its bars and lifts; the lava hall's
  balustrades, lifts and hanging mines; the gallery: dived under the curtain past the fish,
  surfaced (idle, Blupi drifts up) and out onto the tower's landing; the shelves and lifts.

2026-10-27 (D41: empty-mask decor is no floor): the lava hall's balustrades (398) were its piers;
the piers are now white stone with the balustrades along their edges (walked through); every
staged check reaches what it did, Blupi stood on a pier live.
