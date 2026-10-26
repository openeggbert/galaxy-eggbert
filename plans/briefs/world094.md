# Mission 094 — the red door and the teleporter of crates (world 9, region 27)

Built by `tools/worlds/world094.py` → `worlds3d/world094.vwr` (mission 94, region 27 as the
original); `--stage K out.vwr` writes the staged variants for the checks.

## Reference

`worlds/world094.txt`, played at 2:50:35–2:53:59 of the video (`build-probe/video-evidence/w094/`;
0/4 → 4/4 at 2:53:44). Caves of clods with plants wind up to a ridge where yellow forks and
trestles (77, 199) stand by pools of lava (68); crumbling bridges (364) over lava, two mines
hanging over them (3); the red door (334, a red post) between a trestle and a fork, its key
(ObjectType49) below it, fetched and brought back; behind the door a teleporter (330) hangs too
high to stand under, and two crates (12) — one on the floor, one on a ledge, the second pushed
off onto the first — make the step; the teleporter leads down to caverns with the four treasures
(5), mines (2), a bulldozer (4) and the red arrow. 4 treasures, 2 eggs, 4 mines, 2 hanging mines,
2 crates, the key, 1 bulldozer, two teleporters.

## 3D design

Raw cells, Y up; a mass of clods (x 4–95, z 4–95, up to y 40).

| Part | Where | Lesson |
|---|---|---|
| A, the start cave | x 8–20, z 76–88, walk 14; spawn (13, 14, 84) facing −Z | Plants; three steps two high north up to the terrace |
| F, the forks terrace | x 10–40, z 46–62, walk 20 | A floor of lava with a causeway three wide across it (z 53–55), two one-cell gaps of lava in it to jump; beyond three cells of lava on each side shelves with the forks and the trestles and plants (out of reach, D41); an egg |
| B, the bridges | x 41–58, z 53–54, over lava five below | Two crumbling bridges two wide with a rest pillar between them (an egg); two mines hang a cell over the second: walk under them, a jump meets them; the bridges give way behind Blupi (D53) |
| D, the door ledge | x 59–68, z 44–56, walk 20 | The red door in the east wall (69, 20, 50), two high, a trestle and a fork on posts beside it. Its south edge drops ten into the pools cave |
| K, the pools cave | x 56–74, z 57–72, walk 10 | Pools of lava to jump; the key on an island in a ring of lava, an egg, plants; the way back a stair of two-high steps along its south side (walk 12 … 20), a shelf and a strip north onto the ledge's corner |
| T, the teleporter room | x 70–86, z 44–58, walk 20 | A ledge one high along its east side; the teleporter hangs three over the floor cell beside the ledge (81, 51). A crate on the floor (76, 51) is pushed east under it; the crate on the ledge (82, 21, 51) is pushed west off the ledge onto it; the stack's top is level with a lump of clods beside it (81, 52), climbed from the ledge, and Blupi walks onto the stack under the teleporter (in Galaxy a jump into the cell under a pillar meets the pillar) |
| C, the treasure caverns | x 30–90, z 8–40, walk 10 | The partner hangs one over the floor (60, 11, 20); arrival at (60, 10, 21). Banks of clods make three rooms: west, steps up to T1; middle, two rows of three mines to jump before T2, T4 on a pillar reached by three steps, a mine; east, the bulldozer drives a trench one wide and three deep (x 84, z 18–38) with T3 at its end and two alcoves to step into while it passes. The arrow in an alcove of the north side (60, 10, 6); plants, an egg |

## Checks

- Staged `flightcheck.py --mine-margin 0` (`world094.py --stage K`): 0 up to the door and the key
  (743 cells); 1 (door open) + the teleporter room (990); 2 (+ the crate stack) the caverns, all
  but T3 (the bulldozer's trench is a danger cell to levelcheck); 3 (+ bulldozer timed) OK. From
  the key the ledge is reachable again (`reachfrom.py`). D41 audit clean. `VerifyShippedWorlds`
  80/80.
- Live (`--mission 94`, `build-probe/world9-agent/shots/094*`): views of every part; the floor
  crate pushed five cells to the ledge's foot under the teleporter; the ledge crate pushed off onto
  it (a two-high stack); the lump climbed and the stack walked onto: teleported to the caverns
  beside the partner (a first build had Blupi jump from the ledge onto the stack: the jump met the
  pillar and he fell back every time, hence the lump); the key taken on the ledge (probe) and the
  red door opened by walking into it, through into the teleporter room; the stair climbed from the
  pools cave; the bulldozer kills Blupi in its trench and passes him in an alcove; the arrow
  touched (probe without treasures): mission won, back in the hub at sign 5.
  A gap of lava on the causeway jumped (taking off in the last cell before it; a take-off a cell
  early lands in the lava, as the original's).
- Not verified live: the whole mission in one run; the bridges' hanging mines in this world (the
  same pieces were walked in 092).
