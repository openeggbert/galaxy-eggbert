# Mission 095 — the three doors and world 9's golden key (world 9, region 2)

Built by `tools/worlds/world095.py` → `worlds3d/world095.vwr` (mission 95, region 2 as the
original: dark stone, the only world-9 mission not in the red cave); `--stage K out.vwr` writes
the staged variants for the checks.

## Reference

`worlds/world095.txt`, played at 2:54:04–3:03:06 of the video (`build-probe/video-evidence/w095/`;
0/1 → 1/1 at 3:03:06, then the golden key and the main hub). Blupi starts in a little chamber and
drops down a shaft onto a block of crates (12) with a stick of dynamite (55) at each end of the
gallery round it; the blast opens the way down to teleporter alpha (330). Its partner stands in a
grassy cave with trees, the hub of a chain of keys and doors: the green key (50) beyond crumbling
bridges (364) over lava (68); the green door (335) to the red key (49); the red door (334) to a
store of dynamite; a block of crates walling in the blue key (51); the blue door (336) to the way
on: a lift, a second plug of crates with its sticks by cobwebs (401/402), a pool swum through past
three fish (17), a lift over lava (18 ticks, stop 3), a cluster of moving mines (16), and the last
cave with the treasure (5), the invert (40) behind it and the golden key (ObjectType21) on a ball
(217), which ends world 9 once the treasure is taken. 1 treasure, 4 eggs, 62 crates, 14 dynamite,
3 keys and doors, 3 fish, 2 lifts, 3 moving mines, 1 mine, the invert, the golden key.

## 3D design

Raw cells, Y up. The start and its gallery are a mass of clods floating high (x 6–36, z 8–32,
up to y 52); everything else is a second mass below (x 4–98, z 38–98, up to y 24).

| Part | Where | Lesson |
|---|---|---|
| S, the start | x 14–20, z 22–26, walk 46; spawn (17, 46, 25) facing −Z | A girder frame over the shaft's mouth in front of him; the shaft (x 16–18, z 19–21) twelve down |
| G, the gallery | x 10–26, z 15–25, walk 34 | Blupi lands on the crate pit (3 × 3 × 3 crates, their top flush with the floor). A stick at each end of the gallery: one put down on the crates, then away along Z (along X the blasts reach further) and the whole linked block goes; the pit's floor (walk 31) and a tunnel east to teleporter alpha (30, 32, 20) |
| R, the grove | x 16–30, z 76–90, walk 10, grass, trees | Alpha's partner by its north side: Blupi arrives at (20, 10, 81) facing +Z. Four wings |
| E, the green key | x 31–50, z 82–84 | Crumbling bridges over a lava pit (x 36–43): walk on, never stop; the green key and an egg at the end |
| N, the red key | behind the green door (23, 10, 75) | The red key in a small room |
| W, the dynamite store | behind the red door (15, 10, 83) | Five sticks along its west wall, a mine |
| B, the blue key | south, x 20–26, z 93–96 | A doorway three wide filled by a 3 × 2 × 2 block of crates, a trench one deep in front of it so it can neither be pulled out nor walked against: a stick put down in the trench, a jump out and away; the blue key and an egg |
| U, the way on | behind the blue door (23, 10, 68) | Four lifts in step up four (40 ticks, the original's ten a cell, its dwell made 40) to a corridor (walk 14) |
| C, the second plug | x 26–44, z 57–59 | An alcove (x 33–35) under cobwebs with three sticks; a plug of crates 2 × 3 × 3 beyond it: a stick at the plug, the alcove is the shelter |
| P, the pool | x 45–62, z 54–62 | Water flush with the banks, seven deep; a wall across its middle down to two below the surface: dive under it; three fish at three depths, water weed |
| F, the lava ferry | x 63–73 | Four lifts in step carry Blupi six cells east over lava (36 ticks, the original's six a cell; its dwell of 3 made 40) |
| M, the mines' passage | x 74–85, z 57–59 | One danger at a time with a clear pocket between: a mine sweeping across (x 75), a gate one wide with a mine bobbing up and down in it (x 79), a second sweeper (x 83); all 20 ticks |
| K, the last cave | x 86–94, z 40–66, walk 14 | Forks and trestles on posts; the golden key on the ball (217) on a rock (walk 18, a step at 16): jump onto the ball to take it; the treasure in a niche of the east wall (96, 14, 61) with the invert behind it (97) |

## Checks

- Staged `flightcheck.py --mine-margin 0` (`world095.py --stage K`): 0 the start and the gallery
  only (222 cells); 1 (gallery blasted) + the teleporter, the grove, the green key; 2 (green and red
  doors) + the red key and the store; 3 (blue key's block and door) + the blue key, the way on to
  the plug; 4 (plug) + the pool; 5 (swim) + the ferry; 6 (mines timed) everything, the treasure
  and the golden key (OK). D41 audit clean. `VerifyShippedWorlds` 80/80.
- Live (`--mission 95`, `build-probe/world9-agent/shots/095*`): views of every part; a stick put
  down on the crate pit, Blupi walked off along Z: the whole block went, he dropped into the pit
  and walked the tunnel; teleported from alpha to the grove (arrival facing +Z); across the
  bridges to the green key and back (they give way behind him); each of the three doors opened by
  walking into it with its key (probe keys); a stick in the trench, the block blasted, the blue key
  taken; the second plug blasted from beside it; the swim under the pool's wall past the fish and
  out onto the far bank; the ferry ridden over the lava and stepped off in its dwell; the mines'
  passage: walked through blind he dies at a sweeper or the gate (several runs), timed (waiting in
  the pockets) he reaches the last cave; the jump onto the ball takes the golden key (probe without
  the treasure): world 9 ends, Blupi arrives in the main hub, world 9 marked complete (door 189).
- Not verified live: the whole mission in one run; the invert's 5 s of reversed controls.
