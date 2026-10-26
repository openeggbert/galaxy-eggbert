# Mission 044 — the labyrinth of twenty-two treasures

Built by `tools/worlds/world044.py` → `worlds3d/world044.vwr` (mission 44, region 2 as the
original: cobbles). `--check lifts out` writes the staged variant for `levelcheck.py` (below).
World 4's tiles and pieces: `tools/worlds/cheese.py` (`cave`, `shaft`, `tube`, `hang`,
`low_mine`).

## Reference

`worlds/world044.txt`, played at 1:48:59–1:55:25 of the video (0/22 → 22/22, a long search,
several minutes at 21/22). A labyrinth of caves in cheese under the region-2 cobbles: Blupi
starts in the middle beside the red arrow (7), four helicopters (13) round it; the twenty-two
treasures (5) lie in chambers at many heights all round; thirteen little lifts (1, two to four
cells, 20–40 ticks, 10 stop) join the levels; fourteen mines hang (3) from the passages'
ceilings and six lie (2) in the way; ten more helicopters stand here and there; an invert
pickup (40) swaps left and right for a while; an egg (6). The arrow counts only with all
twenty-two.

## 3D design

Fourteen caves (`cave`: upright walls under a low dome) in one block of cheese (x 12–88,
y 6–37, z 12–89), at walking levels 12 to 29, joined by tunnels three wide at the floor and
four high, by lifts in shafts (both ways: ridden up, or a drop down the shaft while the lift
is up) and by drops (one way). Raw cells, Y up.

| Chamber | Centre, walk | Ways | Contents |
|---|---|---|---|
| A, the hub | (50, 50), 20 | west tunnel to B (a mine hanging low on a drop of cheese: crouch), lift north to E, east drop to G, south tunnel to J | Spawn (49, 20, 53); the red arrow (50, 20, 50) with four helicopters round it; plants |
| B | (33, 46), 20 | A, lift down to C, tunnel from L | 1 treasure |
| C | (22, 31), 16 | lifts up to B and to D | 2 treasures, a helicopter, a mine under its dome |
| D | (36, 21), 20 | lifts from C, up to E | 2 treasures, the egg, a hanging mine |
| E | (56, 28), 24 | lifts from D and from A, up to M; drop east to F (a low mine on the way) | 2 treasures, a helicopter, two hanging mines |
| M, the top room | (51, 46), 29, over the hub | lift from E (50 ticks) | 2 treasures, a helicopter, a hanging mine |
| F | (75, 24), 20 | drop from E, lift down to G | a treasure on a ledge three high with its own lift, a helicopter |
| G | (76, 45), 16 | drop from A, lift up to F, drop to H | 2 treasures (one on a ledge four high with its own lift), a helicopter, a hanging mine |
| H | (70, 65), 12 | drop from G, lift up to I | 2 treasures (one on a ledge with its lift), a helicopter, a hanging mine |
| I | (75, 80), 16 | lifts from H and up to J | 2 treasures, a hanging mine |
| J | (55, 77), 20 | tunnel from A (a row of three mines across it to jump), lifts from I and up to K | 2 treasures, a helicopter, the invert pickup on the way to K's lift, two hanging mines |
| K | (36, 74), 24 | lift from J, drop to L | 2 treasures (one on a ledge with its lift), a helicopter, a hanging mine |
| L | (21, 59), 20 | drop from K, tunnel to B (a low mine, then a row of three mines across it), drop to N | 1 treasure, a helicopter |
| N, the deep pocket | (20, 80), 12 | drop from L only | 1 treasure, a helicopter to fly back up |

Lifts: nine between chambers and four to ledges, thirteen as the original's. In 3D each rests on
the floor (a jump up onto it) and rises until flush with the upper floor, waiting two seconds at
the bottom and half a second at the top: a lift parked in a pit flush with the floor trapped
Blupi under it when he stepped in while it was away (checked live, then changed). The lower
tunnel ends in a straight bay that stops at the lift: with the tunnel's rounded end beyond it,
Blupi walked off the rising lift into the pocket under the upper floor (checked live, fixed).
Objects: 22 treasures, 1 egg, 14 helicopters, 13 lifts, 6 mines lying, 14 hanging (3 low on
drops of cheese, 11 under domes and ledges), the invert, the arrow.

## Checks

- `levelcheck.py`: the world itself reaches little (it does not jump onto a lift resting on the
  floor); `--check lifts` (a block under each lift's low end): OK, every treasure and the exit.
  `flightcheck.py --mine-margin 0`: OK on both (the world itself with helicopters). From the
  centre of every chamber the way back to the arrow exists: on foot from all but N, with N's
  helicopter from N (scratch check). The block is closed to the outside.
- `VerifyShippedWorlds`; objects on floors, hanging mines under ceilings, nothing sunk into the
  cheese (scratch check).
- Live (`--mission 44`, `--spawn`): the start view (arrow, helicopters round it, tunnels); the
  A→E lift boarded with a jump at four moments of its cycle (three rode up into E's tunnel);
  the low mine west of the hub passed crouching, and walking into it kills; the east drop into
  G; G's ledge lift jumped onto, ridden and the ledge with its treasure stepped onto; N's
  helicopter taken and flown up the drop shaft into L.
