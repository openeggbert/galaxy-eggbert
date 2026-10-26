# World 101 — mission 1 of world 10 (the dynamite caves)

Built by `tools/worlds/world101.py` → `worlds3d/world101.vwr` (mission 101, region 2 as the
original: a dark cobbled sky). World 10's pieces: `tools/worlds/cobbles.py`. Solved variant for
`levelcheck.py`: `--solved out.vwr` (every crate wall blown, the two step crates pushed into place).

**Reference** (`worlds/world101.txt`, video 3:54:30-3:58:48): a cave in grey cobbled rock (35,
156, edge pieces) with golden forks (77) about it. Forty-four crates (12): loose ones pushed into
steps, walls of them across the passages, a heap of them in a basin, a three-by-three plug sealing
the way to the treasure caves; seven sticks of dynamite (55): four on a high gallery, one in a
side cave, two by the heap. Six treasures in the caves on the right, two eggs, the exit at the top.

**3D design**: the caves run round a hall inside one mass of cobble (walk 6 unless noted).
- The hall (x 44–56, z 46–58): spawn (50, 6, 56) facing −Z. Its north wall is the gallery's
  front, three high: push one of the two loose crates to its foot and jump up from it.
- The gallery (x 44–56, z 40–45, walk 9): four sticks, treasure 2 at its east end.
- West from the gallery a tunnel with a wall of six crates under a cobbled lintel; the cave beyond
  (walk 9): treasure 1, an egg, a stick.
- East from the hall a tunnel plugged by nine crates; two sticks beyond the plug; the pit room,
  its basin two under (drop in, jump out: no soft lock if the heap is blown), a heap of crates in
  it to climb over.
- The treasure caves: a corridor north and south. South, a cave open on it (treasure 3); east, a
  cave behind a wall of crates (treasure 4); north, another behind a wall (treasure 5, an egg); in
  its west wall a ledge three up (treasure 6): push the third loose crate to its foot. At its north
  end a stair of cobble (walk 8, 10, 12) to the exit.
- Every wall faces X, as the blast reaches about three cells along X and one along Y and Z; the
  crates of a wall are linked, so one blast takes it all. Golden forks stand in one-cell alcoves
  cut into the walls, off the walking cells (D41).

**Checks**: `levelcheck.py`: the solved variant reaches the six treasures, both eggs and the exit;
the real world stops in the hall (the gallery needs the pushed crate). `VerifyShippedWorlds`. Live
(`--mission 101`, direct view F3 at spawns in each part): the hall's crates, the gallery's sticks,
the west wall, the plug, the pit's heap, the corridor's walls and ledge crate, the stair. Played
live: the crate pushed to the gallery's foot, the jumps onto it and the gallery; a stick carried
into the west tunnel, put down, and the wall gone after the blast (the stick's flight to the HUD
on pickup takes a few seconds before it can be put down). Not played through: the whole mission.
