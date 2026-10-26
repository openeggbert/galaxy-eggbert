# Mission 091 — the climb out of the clods (world 9, region 27)

Built by `tools/worlds/world091.py` → `worlds3d/world091.vwr` (mission 91, region 27 as the
original); `--stage K out.vwr` writes the staged variants for the checks. World 9's pieces:
`tools/worlds/clods.py` (houses from `village.py`).

## Reference

`worlds/world091.txt`, played at 2:31:19–2:36:18 of the video (`build-probe/video-evidence/w091/`;
0/0: no treasures, the red arrow is the goal). World 9 opens at the foot of a mountain of clay
clods: Blupi starts in a little grove (grass, trees) beside a blue machine where two sticks of
dynamite (55) lie; a block of nine crates (12) plugs the way on and the blast opens it. Then the
long climb: caves with yellow forks and trestles (77, 199) and an enemy helicopter (32), a machine
house of water tanks (91/92, water weed) swum through, mines and moving mines (2, 16), an enemy
tank (33), a lift (90 ticks, stop 10), a second enemy helicopter, and at the top the grey stone
hills under the red sky with birds (20) over them, a pond with a fish (17), a village of houses
and the arrow. 12 eggs, 9 crates, 2 dynamite, 3 mines, 2 moving mines, 2 enemy helicopters, 1
tank, 8 birds, 1 fish, 1 lift.

## 3D design

Raw cells, Y up. A mountain of clods (x 6–82, z 36–92, up to y 33) with a slab under the hills
north of it (z 4–35), its top two layers the village's stone.

| Part | Where | Lesson |
|---|---|---|
| A, the grove | x 10–24, z 74–86, walk 10, grass; spawn (17, 10, 82) facing −Z | The blue machine on its west wall, two sticks beside it (taken by walking over them); trees, plants. The tunnel north (x 16–18, three high) plugged by a block of 18 crates (3 × 3, two deep, z 70–71), a trench one deep behind it so the block cannot be pushed through: one stick at the plug, then away. (In 3D a patient player can also pull the columns out one by one and push them aside; the stick is the way the level shows.) |
| B, the cave of forks | x 10–30, z 50–63, walk 10, a shelf at 12 in the east | Lumpy walls and ceiling, a column; an enemy helicopter crossing at y 13 (80 ticks, the original's pace), a mine, two eggs, forks and a trestle on thin columns out of reach |
| P, the passage | z 54–56, x 31–36 | Rising a block twice to the tank house's door (walk 14) |
| C, the tank house | machine, x 37–55, z 46–62 | Two tanks (surface y 13) between a west and an east rim (walk 14), a dividing wall three thick up to the roof; under it a channel three high and five wide: dive (LeftShift) from the west tank, under the wall, swim up (Ctrl) in the east tank and jump out onto the east rim. An egg at the west tank's bottom, water weed |
| D, the corridor and the lift | x 56–66, z 51–55, walk 14 | A moving mine sweeping across it (40 ticks), an egg; at its end four lifts in step (x 67–68, z 52–53), their cubes in a pit flush with the floor, up nine cells (90 ticks, the original's; its 10-tick dwell made 40 to step on and off in 3D) through a shaft into the upper cave's floor |
| E, the upper cave | x 48–72, z 34–54, walk 23, roof y 30 | Lumpy, two columns; the second enemy helicopter (y 26), an enemy tank on guard firing along X, a mine; a shelf (walk 25) with three eggs; a trestle and a fork on columns |
| K, the crack | x 50–54, z 24–33 | Steps two deep, each two higher (walk 25 … 33), up onto the hills |
| F, the stone hills | x 31–81, z 5–34, walk 34, walled three high | The west hill in two tiers (walk 36, 38) with two eggs and two birds over it; mounds, two more birds sweeping across the way north-east, a mine; the pond (x 58–64, z 12–18) with a fish leaping; three houses along the north wall (fronts south) and the red arrow in front of the middle one (70, 34, 13); trees, plants |

Plates and props (77, 199) stand only where Blupi never walks or jumps (D41, `clods.audit()` clean):
on columns four high, the fringes (285/302) under ceilings five or more above any floor.

## Checks

- Staged `levelcheck.py` (`world091.py --stage K`): as built the grove only (187 cells); 1 (plug
  blasted) up to the west tank (708); 2 (+ swim, a stand-in lift up the east tank) everything,
  the arrow included (levelcheck jumps the moving mine's line); 3 (+ timing) OK. D41 audit clean.
- `VerifyShippedWorlds worlds3d`: 80/80.
- Live (`--mission 91`, shots in `build-probe/world9-agent/shots/091*`): the start view (grove,
  machine, sticks, the plug); a stick taken by walking over it, Blupi turned his back to the plug,
  put it down (Space) and walked away: the whole block went and he lived (putting it down facing
  the plug and turning after the 1.3 s put animation leaves too little time — he died twice that
  way); the swim: dive from the west rim under the wall, swim up and out onto the east rim in
  about nine seconds, air to spare (the channel was two high at first: Blupi stuck under the wall;
  three high works, as in worlds 2 and 5; from the surface he gets out by jumping, swimming
  straight at a rim level with the water's top cell does not lift him out); the lift: stepped on
  at the top in the upper cave's floor, ridden down to the corridor and back up, and stepped off
  in the 2 s dwell; the crack climbed in five jumps onto the hills; the hills with the
  pond, its fish, a bird and the village; the arrow touched: mission won, Blupi arrives in the hub
  outside bar 2.
- Not verified live: the enemy helicopters' and the tank's fire, the moving mine's and the birds'
  timing (engine behaviour checked in earlier worlds).
