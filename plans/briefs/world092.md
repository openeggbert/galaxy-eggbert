# Mission 092 — the lollipop dell and the saw works (world 9, region 27)

Built by `tools/worlds/world092.py` → `worlds3d/world092.vwr` (mission 92, region 27 as the
original); `--stage K out.vwr` writes the staged variants for the checks.

## Reference

`worlds/world092.txt`, played at 2:36:22–2:44:17 of the video (`build-probe/video-evidence/w092/`;
0/1 → 1/1 at 2:43:53). Blupi starts in the grey stone village (houses, trees, a pond with a fish)
walled in at the top of the clods; a lollipop (26) gives the very high jumps that get him out.
Then crumbling bridges (364) over lava (68) with two mines hanging over one of them, a great cave
where ten birds (20) fly among plants, mines and a moving mine; down in blue machines saws (378)
in the floor, each row stopped by its switch (384), a box with two sticks of dynamite (55)
guarded by an enemy helicopter (32) and an enemy tank (33), and the one treasure in a machine
cage behind a block of crates (12), the red arrow beside it. 1 treasure, 9 eggs, 10 birds, 6
mines, 2 hanging mines, 1 moving mine, 10 crates, 2 dynamite, 1 helicopter, 1 tank, 1 fish, the
lollipop.

## 3D design

Raw cells, Y up; a mass of clods (x 4–94, z 26–97, up to y 40), every open part walled up to y 40.

| Part | Where | Lesson |
|---|---|---|
| A, the dell | x 8–32, z 66–92, walk 20, stone floor; spawn (14, 20, 86) facing −Z | Two houses, the pond with its fish, trees, two eggs. Walls twenty high. In the north-east corner a step four high (walk 24), out of a plain jump's reach; the lollipop at its foot (30, 20, 70): licked, one jump onto it; then three steps of two (walk 26, 28, 30) up to the canyon |
| B, the lava canyon | x 26–58, z 46–56, walk 32 | The lava five below; two crumbling bridges two wide (z 50–51, x 34–40 and 43–49) with an island of clods between them (two eggs). They hold Blupi while he walks on (16 ticks a cell) and give way behind him: walk on, never stop. Two mines hang a cell over the second bridge: walk under them, a jump meets them. A mine on the east landing, props on columns |
| C, the birds' crater | x 58–90, z 28–60, walk 32 | Lumpy walls, low mounds (two with eggs), plants; ten birds crossing at and above Blupi's height in every direction, a moving mine, two mines |
| D, the stair | x 67–88, z 62–64 | Out of the crater's south-east corner, eleven steps of two down west to walk 12 |
| E, the machine hall | x 40–66, z 66–84, walk 12 | A lava pit across its middle (x 51–60). Catwalk 1 (z 71) from the east side: its switch (60, 11, 71), three saws, then a landing (x 55–56, z 70–79); catwalk 2 (z 78) from the landing's switch (55, 11, 78) over three saws to the west side. Each switch stops only its own row; three saws cannot be jumped; the pit is four wide where there is no catwalk. An egg, a mine, machines along the walls |
| F, the dynamite box | x 43–47, z 69–73, open south | The two sticks; an enemy helicopter crossing its mouth, a tank driving before it, a mine |
| G, the cage | machine, x 42–50, z 87–93 | Its door (x 45–47) filled by a block of crates three by three with one more before it; a trench inside stops the block being pushed in. One stick at the block (walked up against it), then away: the treasure and an egg. The arrow's alcove opens off its west side (36, 12, 90) |

## Checks

- Staged `levelcheck.py` (`world092.py --stage K`): as built the dell only (588 cells); 1 (the
  licked jump, a stand-in lift) canyon, crater and the hall's east side, not the box; 2 (+ saws
  stopped) the box; 3 (+ crates blasted) everything, the treasure and the arrow (OK); 4 OK. D41
  audit clean. `VerifyShippedWorlds worlds3d` 80/80.
- Live (`--mission 92`, `build-probe/world9-agent/shots/092*`): the dell; the lollipop licked
  (Space, gauge) and the licked jump onto the step, the steps climbed; the canyon walked across from the west landing to the crater under both hanging mines,
  every bridge cell giving way behind him (first build: he dropped through after two to four cells,
  the engine kept a cell only 7 ticks; reported, fixed as D53, re-checked live); standing still on
  a bridge drops him into the lava; the crater with its birds; the hall: without the switch Blupi
  dies on catwalk 1's saws, with the switch pressed (Space) he walks over them; a stick put down
  at the crate block, walked away: the whole block went, Blupi walked into the cage; the treasure
  taken and the arrow touched: mission won, back in the hub with bar 3 open.
- Not verified live: the enemy helicopter's and the tank's fire at the box, the birds' and the
  moving mine's timing, catwalk 2 (the same switch and saws as catwalk 1).
