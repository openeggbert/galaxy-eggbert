# Mission 034 — the teleporter station and world 3's golden key

Built by `tools/worlds/world034.py` → `worlds3d/world034.vwr` (mission 34, region 19 as the
original). `--check STAGE out` writes the staged variants for `levelcheck.py`, which models
teleporters since `4563ec0`.

## Reference

`worlds/world034.txt`, played at 1:25:39–1:34:49 of the video (0/5 → 5/5 at 1:34:43, then the
key and the main hub). Blupi starts on the golden key (ObjectType 21) in front of a house,
between two teleporters (330 α and 332 δ); the key counts only with all five treasures. Four
teleporter pairs (330–333, a pillar hanging one cell above the floor; standing under it sends
Blupi to the other pillar of the same letter) join four places:
- a rock cave (284/301), the junction: teleporters back to the start and on to the toy room (β)
  and the brick ruins (χ); a sliding lift (36 ticks, 3 stop) over a gap, a temporary block (324:
  solid most of the time, passable for a moment every few seconds), the blue key (51), a
  treasure, an egg, and a long lift (180 ticks, four side by side) up into the machine hall;
- a machine hall (78–84): a fan (126, deadly at its mouth), a bulldozer (210 ticks) among mines,
  a hanging mine, two invert pickups (40), lava (68), a fish (17) in a pool before the blue
  door (336) and the treasure behind it with a moving mine, three eggs, a bird;
- a toy room (215–219, 231, striped poles): a lift (60 ticks) among mines, a lollipop (26) for
  the high jump to the treasure, an egg;
- brick ruins (261–263) with house roofs under the sky: six birds over the roofs, a drink (30)
  to pass them unseen, a lift (48 ticks) up eight to the treasure.

## 3D design

Five places built apart in the grid, joined only by the teleporters and the long lift. Raw
cells, Y up. A teleporter's pillar hangs one cell above the floor; Blupi arrives one cell +Z of
the cell under the partner pillar, still facing it (one more step forward sends him back), so
every arrival has room to turn.

| Place | Where | What |
|---|---|---|
| S, the courtyard | x 38–58, z 74–90, walk 30, before a house (facade z 73) | Spawn (48, 30, 84) facing −Z with the golden key at (48, 30, 78) ahead; α at (44, 31, 79) → machine gallery, δ at (52, 31, 79) → rock cave; conifers, a stone parapet |
| R, the rock cave | x 14–46, z 30–62, y 0–22 | Arrival hall (walk 14) with δ (from S), β (→ toy room) and χ (→ ruins) hanging from rock columns. A drop into the lower cave (walk 8): west, a chasm over lava bridged by three temporary blocks to the ledge with the blue key and a mine; the middle with an egg; east, a lava pit six wide crossed by three sliding lifts in step to the floor with the treasure; a stair of two-block steps back up to the hall. From the hall's east end the long lift (four lifts, 180 ticks) up fourteen cells into the machine hall |
| M, the machine hall | x 8–46, z 28–62, y 23–44 | The gallery (walk 34) where α arrives: two eggs, the fan in its east wall blowing −X (wind tiles before it), the treasure in the corner beside its mouth, a window down into the hall's east part (walk 28) where the long lift arrives and a bird rises and falls. A tunnel three wide to the west part: the bulldozer drives its middle lane, mines in the side lanes, one hanging a jump above. West part: lava with a walkway into it, two invert pickups on it and an egg at its end; the pool (one below the floor, banks level with the water) with the fish sweeping before the ledge of the blue door; behind the door the treasure and a moving mine riding up and down |
| T, the toy room | x 58–86, z 28–58, y 0–26 | β arrives on the star floor (walk 6); a lift three wide (60 ticks) between two mines, one hanging beside its shaft, up to the balcony (walk 12) with the lollipop; the high jump onto the shelf (walk 18) with the treasure and an egg; striped poles |
| B, the brick ruins | x 18–34, z 2–26, walk 36, under the sky | χ hangs in a brick gate in the courtyard, the drink beside it; three houses stepping up north (roofs walk 38, 40, 42), two birds sweeping each roof at Blupi's height; the tower's lift (48 ticks) in its south face up eight to the treasure on top |

Objects: 5 treasures, 5 eggs, 7 birds, 7 mines (2 hanging), 1 moving mine, bulldozer, fish,
11 lifts, drink, lollipop, 2 invert pickups, the blue key and door, the golden key.

## Checks

- `levelcheck.py` in stages (teleporters linked): the world itself reaches the rock cave's
  treasure, the blue key and the gallery's treasure (the birds, the bulldozer's tunnel and the
  toy shelf stop it); `--check timed` (bulldozer, fish, birds and moving mine left out, the
  temporary blocks solid) also reaches the ruins' treasure and the hall's west part;
  `--check power` (a stair where the lollipop's jump goes) also the toy room's treasure;
  `--check solved` (also the blue door open) all five and the golden key. From each treasure
  the way back to the golden key exists (checked from each treasure's cell).
- `VerifyShippedWorlds`.
- Live (`--mission 34`, `--spawn`, probes through `QA_LIVE_WORLDS`): the start view (key, α,
  δ, the house); all four teleporter pairs taken (the camera needs a few seconds to travel to
  a far partner); the lower cave with the temporary bridge holding Blupi, the lava pit's
  sliding lift ridden, the lava drawn as flames; the long lift ridden from the cave up into the
  machine hall (and down again); the gallery, the fan and its treasure; the tunnel; the pool
  swum to the blue door's ledge (water rules of `762d254`); the toy room's lift; the ruins'
  courtyard, roofs with birds and the tower lift to the treasure. A probe without treasures:
  touching the golden key ends the world and returns to the main hub.
