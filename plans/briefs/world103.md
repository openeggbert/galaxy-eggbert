# World 103 — mission 3 of world 10 (the lava labyrinth)

Built by `tools/worlds/world103.py` → `worlds3d/world103.vwr` (mission 103, region 21 as the
original: cobble lit by lava). Staged variant for `levelcheck.py`: `--stage 1 out.vwr` (floors
over the water, for the swims levelcheck cannot model; the curtain cut).

**Reference** (`worlds/world103.txt`, video 4:03:15-4:07:42): a labyrinth in grey cobble lit by
lava. From Blupi's start two lifts meeting over a trench of lava (30 ticks, dwell 10), a third
over another (60, 20); flooded passages with seven fish (17) to dive and swim through; a shaft
climbed on seven lifts in two columns (18 ticks, dwell 3), each meeting the next at its top or its
foot; a hall at the top where six lifts each carry him across a pool of lava from pillar to pillar
(12 ticks, dwell 3) to the one treasure; a drop to the exit. Nine eggs.

**3D design**: one mass of cobble.
- The gallery (z 80–86, walk 6, east): spawn (6, 6, 83). The two lifts (2 × 3) meet over the first
  trench, dwelling 30 ticks where they meet; a side pit flooded with two fish and an egg at its
  foot; a third lift over the second trench.
- The flooded dip (x 53–66): the floor falls away into water level with it; a curtain of stone
  hangs into the water across the middle: dive under it; three fish, two eggs on the bottom.
- The lift shaft (x 80–87, walk 6 → 27): seven lifts (2 × 2) in two side-by-side columns, all in
  step: one rises, the next comes down to meet it at its top and carries Blupi up as it goes back,
  meeting the third at its foot (30-tick dwells, the original's 3 too short to step across in 3D).
- The hall of pools (walk 27, west): six pools of lava between pillars of cobble, a lift on each
  starting by its east pillar and carrying him two cells to its west pillar (30-tick dwells), eggs
  on two pillars; a pool two fish swim in, the treasure beyond it.
- The exit's chamber (walk 15) under the hall's west end, reached only by the drop beside the
  treasure; an egg in it.

**Checks**: `levelcheck.py`: stage 1 reaches the treasure, the exit and every egg but the three on
the water's bottoms; the real world reaches those three (a fall into the water lands on them) and
stops at the dip (it cannot swim). `VerifyShippedWorlds`. Live (`--mission 103`, direct view F3 at
spawns in each part): the trenches' lifts, the dip and its curtain, the shaft's lifts, the pools'
lifts and the eggs on the pillars, the pool and the treasure by the drop. Not played through: the
swims, the shaft's relay (the same in-step relay as 084's chain, played there), the pools' lifts.
