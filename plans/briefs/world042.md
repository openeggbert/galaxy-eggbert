# Mission 042 — mines and lifts under the red sky (the cheese)

Built by `tools/worlds/world042.py` → `worlds3d/world042.vwr` (mission 42, region 20 as the
original: the red sky). `--check STAGE out` writes the staged variants (below). World 4's tiles
and pieces: `tools/worlds/cheese.py`.

## Reference

`worlds/world042.txt`, played at 1:37:30–1:42:42 of the video (0/15 → 15/15). On foot, no
vehicles: Blupi starts in a pocket of cheese with mines (2) on its floor and three hanging (3)
over the jumps; treasure 1 up a step; the treasure door "1" (421) opens with it onto a stair of
six little lifts (1), each bobbing one cell (10 ticks, 10 stop, alternate ones in counter-step),
over a drop with treasure 2 below; door "2" (422) opens onto a hill with a house and a row of
trees (BigDecor 20–22, 26); a row of bumps with a mine lying between them (fourteen); treasure 3
at the bottom of a shaft climbed on five lifts in relay (42 ticks, 3 stop, each rising seven,
alternately up and down so that one's top meets the next one's bottom), the cheese round it with
walk-through secret passages (339); an avenue of trees with twelve mines to jump; a pyramid with
twelve treasures on its steps; with all fifteen the doors "15" (435) at its top open and the
way leads down to the red arrow (7). 15 treasures, 6 eggs, 32 mines, 3 hanging mines, 12 lifts.

## 3D design

Islands of cheese (246–249) floating in the red sky, joined by the route. Raw cells, Y up.

| Part | Where | What |
|---|---|---|
| A, the crater | round, radius 7.5 round (15, 85), walk 12, rim seven high | Spawn (15, 12, 90) facing −Z. A notch in the north wall with two steps (walk 13, 15): treasure 1 and an egg on the upper one. Across the notch's mouth a row of three mines (z 79) jumped; a beam over two of its three lanes with mines hanging at the top of a jump, the third lane free; another mine on the floor. Door 1 (421) at (23, 12, 85) in the east wall, a mine hanging in the short tunnel behind it |
| B, the lift stair | x 29–46, z 84–85 | Six stones, each four lifts two by two, bobbing one cell (10 ticks, 10 stop, counter-step), each a jump from the last and one higher, from the landing (walk 12) to the house island (walk 18). A lift does not stop Blupi sideways, so each stone is a cell from the last: every one is jumped onto. The basin under it (walk 6) catches a fall: treasure 2 there, steps back up to the landing |
| C, the house island | x 46–58, z 76–94, walk 18 | The house (BigDecor 26), five trees, an egg; along its east edge a ridge four high with door 2 (422) at (58, 18, 85) |
| D, the causeway | x 59–80, z 84–86, walk 18, over the sky | Five rows of three mines across it, four apart, jumped one at a time (a mine kills within 0.7 of Blupi and a jump carries him 2.4 cells: rows closer than four leave no room to land) |
| E, the tower hall | x 84–94, z 70–88, floor walk 18, up to y 55 | Treasure 3 and two eggs on the floor; an egg in a niche behind walk-through cheese (339, at walking level only, never a floor). The relay along the north wall: five lifts two deep (42 ticks, 3 stop), each rising seven above the last (walk 19 → 26 → … → 54), alternately up and down, a column apart; between each two a rest whose top is their meeting height. Blupi rides one up (the rest's column blocks him until the top), steps onto the rest and boards or jumps onto the next at its bottom; a mistimed step drops him back to the hall floor. Stepping straight from lift to lift as the 2D original does would want them to meet at the cell boundary in the 0.15 s they stand level |
| G, the avenue | x 90–94, z 39–65, walk 53 | Out of the top of the relay through a passage north, a step down: bays three long with trees (20–22), between them five necks three wide where three mines lie across the way (15 mines for the original's 12–14), an egg in a bay |
| H, the pyramid island | x 60–96, z 6–38, walk 53 | The pyramid round (78, 21), six steps one high: twelve treasures on steps 1, 3 and 5, three on each face's middle. On the top step a cap with door 15 (435) in its south wall at (78, 59, 23), a jump up from step 5; behind it a shaft down the pyramid's core into the exit hall (walk 43) with the red arrow at (78, 43, 16) |

Mines: 34 lying (the original's 32), 3 hanging; lifts: 34 cubes for the original's 12 (the
stones are four lifts each, the relay lifts two deep).

## Checks

- `levelcheck.py` cannot jump rows of mines (it keeps a cell round each); `flightcheck.py
  --mine-margin 0` (577267c: a still mine kills in its own cell only, as the game's 0.7 radius)
  in stages: the world — treasure 1 only (door 1 closed); `--check door1` — also treasure 2 in
  the basin, not the house island (the stones move: levelcheck does not jump onto a moving
  lift); `--check stones` (stones fixed at their low ends) — also the house island, door 2
  closed; `--check door2` — also the tower, the relay (lift ends with the rests between),
  the avenue and the twelve pyramid treasures, not the exit (door 15 closed); `--check solved`
  — every treasure and the exit.
- `VerifyShippedWorlds`; objects on floors, hanging mines under ceilings, nothing sunk into the
  cheese (scratch check).
- Live (`--mission 42`, `--spawn`, probes through `QA_LIVE_WORLDS`): the start view in the crater
  with the notch, its mine row and the hanging mines; a jump in the free lane over the row,
  then up the steps to treasure 1 (a jump under the beam meets the hanging mine and kills, as
  meant); the stones seen bobbing from the landing, jumped onto from the landing and from stone
  to stone; the house island with the house, trees and door 2; the causeway crossed jumping its
  five rows; through the secret cheese into the niche for its egg; lift 1 of the relay ridden
  from its step, the rest stepped onto at its top, the next lift boarded by a jump from the rest
  at four moments of a 0.6 s window, the last lift ridden into the passage and out onto the
  avenue; the avenue's first row jumped; door 435 in a probe with fifteen treasures in a row
  opens at 15/15 and lets Blupi through; in the solved variant the jump into door 15's doorway,
  the fall down the shaft into the exit hall and its arrow.
