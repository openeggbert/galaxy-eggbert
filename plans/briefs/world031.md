# Mission 031 — birds over the rooftops (the village)

Built by `tools/worlds/world031.py` → `worlds3d/world031.vwr` (mission 31, region 16 as the
original); `--solved` writes a variant with the treasure door open, for `levelcheck.py`.

## Reference

`worlds/world031.txt`, played at 1:12:50–1:15:42 of the video (0/3 → 3/3 at 1:15:07). World 3's
first mission, left to right: a street of houses whose roofs Blupi jumps across while birds
(ObjectType 20, a touch kills) fly short beats to and fro over them, mines on three roofs; a tall
house with a lift rising seven cells beside it (70 ticks, an egg halfway up); a cave in a stone
hill where seven mines lie round the first treasure; a treasure door ("1", tile 421) that opens
once one treasure is taken (`Decor::OpenDoorsTresor`); behind it a shaft into a vault with
treasures 2 and 3 and two eggs among plants; out of the vault the street goes on over three
gaps whose floors are mines, each crossed on a sliding lift — 60 ticks/20 stop, 30/10, 18/3 —
under a bird; a last lift (50 ticks) climbs beside a tall house among four birds to the red
arrow. 14 birds, 24 mines, 5 eggs, 3 treasures, 5 lifts. The exit counts only with all three
treasures.

## 3D design

Everything floats in the region-16 sky on stone footings; falling off is a fall to death, as
the original's gaps were. Raw cells, Y up.

| Part | Where | Lesson |
|---|---|---|
| A, the rooftop ridge, heading −Z | x 20–24; yard z 85–93 (walk 10), spawn (22, 10, 90) facing −Z | Houses across the ridge, each a jump: z 81–84 (walk 12), 77–80 (13, mine), alley z 76 (a one-cell gap), 71–75 (13, mine), 67–70 (15, mine), 63–66 (14). Six birds: high over the yard, a head above the second roof's landing row and over the alley (they meet a jump), one at Blupi's height along the third roof's east edge, one above each of the last two roofs |
| B, the tall house | x 18–26, z 53–62, roof walk 21 | The lift in its facade at x 22 rises from the last roof (walk 14) to the roof (70 ticks / 10); an egg in an alcove beside the shaft at walk 18, another on the roof; two birds over the roof |
| C, the cave | in the stone hill x 10–52, z 18–52 (grass and conifers on top, y 31) | The mouth opens from the tall house's roof (walk 21). A hall seven high: a mine at the entrance, shelf 1 (walk 23) with three mines along its edge, shelf 2 (walk 25) in the north-west corner with treasure 1 at (14, 25, 36) and a mine beside it on the side one comes from; a passage east with two mines to weave between; the treasure door, two cells of 421, at x 38 |
| D, the vault | x 39–50, z 28–44, walk 13 | Behind the door a shaft drops ten cells into it: treasures 2 and 3, two eggs, plants; the only way on is the tunnel east |
| E, the arcade, heading +X | z 34–38, roofs walk 15 | Four houses with three gaps of four cells between them; down in each gap a beam of four mines over the void; across each three lifts side by side (in step) slide from roof to roof at the original's three speeds, a bird a jump above each gap |
| F, the exit tower | yard x 88–90 (walk 15), tower x 92–97 (roof walk 20) | Two mines in the yard's corners, the lift at x 91 (50 ticks / 10) up five, the red arrow at (96, 20, 36); four birds round the top, one across the roof at Blupi's height |

Tiles: stone 35 with its variants for footings and the hill, grass 107, houses of window tiles
(186–190) with slate roofs and dormers (193–197), window facades on +Z. Tiles that render
black in 3D (arches 198, the stone corner pieces, the pillar 76) are left out.

## Checks

- `levelcheck.py`: the world — treasure 1 reachable, the vault, treasures 2–3 and the exit not
  (the door closed); `--solved` — every treasure and the exit reachable. Removing the tall
  house's lift cuts off everything past the rooftops; removing any one gap's lifts, or the exit
  tower's lift, cuts off the exit. (The alcove egg is reached from the moving lift, which the
  check does not model.)
- `VerifyShippedWorlds`.
- Live (`--mission 31 [--spawn …]`): the start view (ridge, tall house, hill); the first bird
  killed a jump onto the first roof (moved higher, over the yard's south side); the tall
  house's lift ridden past the alcove egg to the roof and the cave mouth; the cave hall, shelf
  mines and the passage; the treasure door, tested in a probe with the treasure beside it,
  opens at 1/3 and Blupi drops through the shaft into the vault; the fast gap lift ridden to and
  fro over its mines; the exit lift ridden to the tower top with the arrow and birds. The exit
  lift first had no floor under its start (Blupi fell into the sky): a stone footing was added.
