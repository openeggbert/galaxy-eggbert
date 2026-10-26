# Mission 053 — the foundry of stars (world 5, region 1)

Built by `tools/worlds/world053.py` → `worlds3d/world053.vwr` (mission 53, region 1 as the
original); `--stage K out` writes the staged variants for `levelcheck.py`.

## Reference

`worlds/world053.txt`, played at 4:41:34–4:53:41 of the video (`build-probe/world5-agent/video/w053/`;
0/21 → 21/21 at 4:51:28), world 5's long mission. Riveted plates over lava (68), girders (201):
a hall of girder towers where follow stars (96) sleep, a small lift step (10 ticks); two mines
bobbing in a corridor (20 ticks); a lava pool crossed on two lifts that meet in the middle (24
ticks, stop 3) under sleeping stars; a lift (18 ticks) over a pit of seven wasps (44) under six
hanging mines; a shaft climbed in the balloon a wasp's touch gives, past hanging mines that pop
it; a vault of thirteen treasures, each guarded by stars; the treasure door "13" (433) onto a
corridor of eleven stars with the drink (30) beside it — unseen, Blupi passes and they sleep on —
to the fourteenth treasure; the door "14" (434) onto a crusher (317), a lava slope, gates of mines
bobbing in pairs (20 ticks), three crushers before a row of seven treasures under two columns of
stars, a lift up seven cells (42 ticks) to the last corridor and the red arrow in a pit of stars.
21 treasures, 17 eggs, 67 stars, 10 wasps, 9 hanging mines, 8 moving mines, 3 mines, 5 lifts,
the drink. The lesson of the mission: the stars wake within about two cells and home slowly —
lure them into walls, or slip past them unseen.

## 3D design

Raw cells, Y up; the route runs north from the start, up into the vault, then east and south.

| Part | Where | Lesson |
|---|---|---|
| A, start hall | x 21–31, z 82–93, walk 11; spawn (26, 11, 91) facing −Z | Four girder masts with a star asleep at the foot of each, four ball machines (BigDecor 28/30); a lift (30 ticks) up three to the ledge of the north door; two eggs |
| B, the gates | corridor x 25–27, z 67–80, walk 14 | Two gates one cell wide, a mine riding up and down in each (20 ticks, opposite phase) |
| C, the lava crossing | pool x 18–34, z 51–61 | Lift A from the south bank four cells north, lift B from the north bank five cells south, in the same 36 ticks (stop 12: the original's 24/3 asks for a 2D step in 0.15 s), so they meet side by side: step from A to B; five stars asleep over the lane; eggs on the north bank |
| D, the wasp pit | pit x 18–34, z 40–45, walk 8, six below the banks | Seven wasps sweep its floor (a touch: a balloon, and back up); the ferry (two lifts, 30 ticks) across under a girder gantry with six mines hanging two above it; steps out of the pit at its west end |
| E, the balloon shaft | x 23–27, z 25–29, walk 14 up to the vault (walk 26) | Three wasps at its foot and halfway up: the balloon rises (faster with jump held) and drifts where Blupi walks, about ten seconds; two hanging mines in corners, a block in the middle with a mine on it and one hanging under it — any of them pops the balloon |
| F, the vault | x 15–57, z 5–33, walk 26, the shaft coming up through its floor | A dais of three tiers (walk 28, 30, 32) with seven treasures and ten stars; four alcoves (two west, two south) each with a treasure and a star at its mouth; the east gallery (walk 30) with two treasures, four eggs and two stars |
| G, the door "13" | x 53–55, z 4 | The corridor of stars (x 15–57, z 1–3, walk 30): the drink at its east end, four curtains of stars across all three lanes at Blupi's height; drunk, he walks through them in about 13 of the drink's 20 seconds to the fourteenth treasure; seen, the first curtain wakes and kills him |
| H, the door "14" | x 58, z 18–20 | The crusher passage (x 59–69), narrowed to one cell at x 64 over a crusher: squashed, no jump for a while; the lava slope (x 70–74, z 18–42): five terraces heading south, each two below the last, a strip of lava across each, two cells past the drop; its foot (walk 20) with three gates of two mines bobbing in opposite phase |
| Treasure hall | x 60–82, z 58–66, walk 20 | Crushers two deep in its doorway (no jump clears them); seven treasures in a row under two columns of four stars; the lift at its east end seven cells up (42 ticks, stop 3) to the ledge with two eggs |
| The cage of the arrow | corridor x 85–87, z 68–80 (two mines, three stars); antechamber x 79–87, z 81–83; cage x 78–86, z 85–91 | The arrow in the middle of the cage among twenty stars; the doorway one cell wide: wake the nearest from it, step aside, and they burst on its jambs; four eggs in the corners |

Object counts as the original's: 21 treasures, 17 eggs, 67 stars, 10 wasps, 9 hanging mines, 8
moving mines, 3 mines, the drink; 8 lifts (the original's 5, pairs two wide).

## Checks

- Staged `levelcheck.py` (`build-probe/world5-agent/stagecheck.py 053 0 … 7`; 1 the bobbing mines
  timed, 2 the balloon ridden (a stand-in lift), 3 the door "13", 4 the corridor crossed unseen,
  5 the door "14", 6 the crushers waited out, 7 every star lured away): eggs only, the start's
  eggs, the vault's open treasures, +the corridor's treasure, +the hall's seven, then all 21 and
  the exit. (The check avoids every cell next to a star, so the guarded treasures count only at
  stage 7.)
- `VerifyShippedWorlds worlds3d`: 80/80.
- Live (`--mission 53`, probes through `QA_LIVE_WORLDS`, `build-probe/world5-agent/shots/053-*`):
  the start hall, the gates, the lava pool, the ferry and the pit; the lava pair meeting and
  Blupi stepping from A onto B and riding to the far bank (after the engine's lift timing fix
  D43; before it the two lifts never met — reported); the balloon from a wasp at the shaft's foot
  rising to the vault; the vault and its dais; the drink corridor: without the drink the first
  curtain kills, with it Blupi walks through all four (after the follow-star fix D44; before it
  the stars woke and swarmed a hidden Blupi — reported); the crusher squashing him; one jump over
  a lava strip on the slope; the gates walked; the hall's lift ridden; stars bursting on the
  cage's jambs; the arrow (probe without treasures) ends the mission. Found along the way: the
  original 3 × 3 lava strips one cell past the drop left no runway (moved one cell further);
  stars two above the floor never reached Blupi (moved to his height); a pit two deep for the
  drink trapped him (removed).
