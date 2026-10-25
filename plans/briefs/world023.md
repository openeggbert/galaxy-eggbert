# Mission 023 — the tower of keys (white palace, region 28)

Built by `tools/worlds/world023.py` → `worlds3d/world023.vwr` (mission 23, region 28).

## Reference

Mobile Eggbert's `worlds/world023.txt`, played at 3:12:51–3:17:56 of the full-game video
(`video-evidence/w023/`): the lesson of keys and switches. From a white room two lifts (40
ticks) go, one down into a rock cave (284/301) where the red, green and blue keys lie, the blue
one high up beside the cave's one crate; one up to a vault behind a blue door holding a second
red and green key; red and green doors in a row lead on; a lift carries Blupi over a row of saws;
ball blocks (215-220) on pillars (76) climb to rooms of machine panels (10) and shelves where
switches (384) stop the saws of their row; two more blue keys open two blue doors; at the top
the red arrow stands between two basins. 7 keys (red 2, green 2, blue 3), 7 doors, 4 lifts,
1 crate, 2 treasures, 5 eggs, 11 saws, 4 switches. A key opens one door and is used up
(Decor.cpp ~5619; Galaxy clears the colour's count), so no two keys of one colour may be held
at once.

## 3D design

A white tower climbed floor by floor (walk 4 → 25), each step behind a door whose key was
just found; a fall off its open parts is a fall to death.

| Part | Where | Lesson |
|---|---|---|
| Hall | x 40–56, z 54–66, walk 11, walls seven high; spawn (48, 11, 64) | two lifts: down to the cave (70 ticks, 10 a cell as the original's) and up to the landing (40 ticks) |
| Cave | rock, x 36–60, z 52–70, walk 4 | the red and green keys on its floor; the blue key and treasure 1 on a ledge three high, climbed from the crate pushed +X to its foot; rock columns, an egg |
| Corridor | walk 15, x 33–46, z 48–50, open on top | the red door, the green door (the cave's keys), the blue door of the vault; in the vault the second red and green key |
| Saw pit | walkway x 33–35 north of the vault | a pit of saws five long crossed on three lifts side by side (40 ticks) |
| Ball blocks | behind the red door (the vault's red key) | a spiral of 2×2 ball blocks on pillars, each two above the last (walk 17–25) |
| Machine terrace | x 29–45, z 17–27, walk 25, behind the green door | west: a catwalk one wide along X, five saws, its switch at its start, the first blue key at its end; east: the toy room, a passage one wide between shelves, its switch in the doorway, five saws, treasure 2 |
| North | behind the first blue door | a landing over the void, a bridge one wide along X with four saws and its switch to the island of the second blue key, a bridge north to the second blue door |
| Arrow's room | x 31–43, z 9–10, open on top | the red arrow between two basins (BigDecor fountains) with eggs |

Each switch sits in its own row (height and Z) with its saws along X, since a switch reaches
the saws of its row only, 20 cells either way (Decor::ActiveSwitch's 2D window); the rows are
five and four saws long, too long to jump. Three switches, where the original has four.

## Checks

- Staged reachability (`world023.py --stage K`, `build-probe/world2-agent/stagecheck.py 023
  0 1 2 3 4 5 6`, with the jump-arc fix of `build-probe/world2-agent/levelcheck2.py`): as built
  the cave's red and green keys; + crate at the ledge: the blue key and treasure 1; + corridor
  doors open: the vault's keys; + red door: the ball blocks; + green door: the terrace; +
  switches pressed: the first blue key and treasure 2; + blue doors open: the second blue key,
  the eggs and the exit. Without the lifts the cave and everything above the hall are out of
  reach.
- `VerifyShippedWorlds worlds3d`: 80/80.
- Live (`--mission 23`, `build-probe/qa-shots/world2/w023/`): the hall, the cave, the corridor's
  red door, the saw pit and its lifts, the ball blocks, the terrace, the catwalk and the toy
  room, the north bridges, the arrow's room; a red key opening the corridor's red door (a probe
  copy with a key before it); the catwalk's switch pressed and the five saws walked over to
  the blue key.
