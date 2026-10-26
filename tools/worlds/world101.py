#!/usr/bin/env python3
"""Mission 101, the dynamite caves (worlds3d/world101.vwr): brief plans/briefs/world101.md.

Mobile Eggbert's level 101 is a cave in grey cobbled rock where walls and heaps of crates (44)
stand in the way and seven sticks of dynamite lie about: crates pushed into a step up to a
gallery of four sticks, walls of crates blown open to reach the caves of six treasures, a heap
of crates to climb over, two more sticks by it, golden forks about the caves, the exit at the
top. Here the caves run round a hall in 3D: the hall and its gallery, the west cave behind a
wall, the east tunnel's plug of crates, the pit with its heap, the treasure caves with two more
walls and a ledge reached from a pushed crate, the stair up to the exit.

    python3 tools/worlds/world101.py                 # writes worlds3d/world101.vwr
    python3 tools/worlds/world101.py --solved out    # every wall blown, the crates pushed into
                                                     # their steps (for levelcheck)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from cobbles import carve, dress, fork_alcove, rock  # noqa: E402
from kit import O, World  # noqa: E402
from village import STONE  # noqa: E402

FLOOR = 5                   # the hall's floor: Blupi walks at 6
WALK = FLOOR + 1
HIGH = WALK + 3             # the gallery, the west tunnel and cave: walk 9


def main(argv) -> int:
    solved = len(argv) > 2 and argv[1] == "--solved"
    out = Path(argv[2]) if solved else Path(__file__).resolve().parents[2] / "worlds3d" / "world101.vwr"
    rng = random.Random(101)
    w = World(mission=101, sky=2)

    def crate(x, y, z):
        w.place(O.CRATE, x, y, z)

    def crate_wall(cells):
        """A wall of crates (x, y, z) Blupi blows open; gone in the solved stage."""
        if not solved:
            for x, y, z in cells:
                crate(x, y, z)

    rock(w, 18, 0, 28, 95, 16, 70)

    # ---- the hall (walk 6) and its gallery (walk 9) ----
    # Spawn at the hall's south end. Its north wall is the gallery's front, three high: push one
    # of the two crates to its foot and jump up from it. Four sticks along the gallery, treasure 2
    # at its east end.
    carve(w, 44, WALK, 46, 56, WALK + 4, 58)
    carve(w, 44, HIGH, 40, 56, HIGH + 2, 45)
    w.spawn = (50, WALK, 56)
    if solved:
        crate(48, WALK, 46)
    else:
        crate(48, WALK, 50)
    crate(53, WALK, 52)
    for x in (45, 47, 49, 51):
        w.place(O.DYNAMITE, x, HIGH, 41)
    w.place(O.TREASURE, 55, HIGH, 41)
    fork_alcove(w, 44, WALK, 54, (-1, 0))
    fork_alcove(w, 56, WALK, 48, (1, 0))

    # ---- the west tunnel and cave (walk 9) ----
    # From the gallery's west end a tunnel west, a wall of crates two high across it under a
    # cobbled lintel; the cave beyond: treasure 1, an egg, a stick.
    carve(w, 32, HIGH, 41, 43, HIGH + 2, 43)
    w.fill(38, HIGH + 2, 41, 38, HIGH + 2, 43, STONE)
    crate_wall([(38, y, z) for y in (HIGH, HIGH + 1) for z in (41, 42, 43)])
    carve(w, 22, HIGH, 36, 31, HIGH + 3, 48)
    w.place(O.TREASURE, 24, HIGH, 38)
    w.place(O.EGG, 29, HIGH, 46)
    w.place(O.DYNAMITE, 26, HIGH, 45)
    fork_alcove(w, 22, HIGH, 44, (-1, 0))

    # ---- the east tunnel, its plug, the pit and the heap (walk 6) ----
    # East from the hall a tunnel plugged by nine crates; two sticks beyond the plug; the pit, its
    # floor two under (drop in, jump out), a heap of crates in it to climb over.
    carve(w, 57, WALK, 50, 66, WALK + 2, 52)
    crate_wall([(62, y, z) for y in (WALK, WALK + 1, WALK + 2) for z in (50, 51, 52)])
    for z in (50, 52):
        w.place(O.DYNAMITE, 65, WALK, z)
    carve(w, 67, WALK, 46, 77, WALK + 4, 56)
    carve(w, 68, WALK - 2, 47, 76, WALK - 1, 55)
    for x, z, height in ((71, 51, 2), (72, 51, 2), (73, 51, 2), (70, 51, 1), (74, 51, 1),
                         (71, 50, 1), (72, 50, 1), (73, 50, 1), (71, 52, 1), (72, 52, 1), (73, 52, 1)):
        for k in range(height):
            crate(x, WALK - 2 + k, z)

    # ---- the treasure caves (walk 6) ----
    # From the pit east a corridor running north and south. South: a cave open on it, treasure 3.
    # East: a cave behind a wall of crates, treasure 4. North: a cave behind another, treasure 5
    # and an egg. In the corridor's west wall a ledge three up, treasure 6 on it: push the crate
    # to its foot. At the corridor's north end the stair up to the exit (walk 12).
    carve(w, 78, WALK, 49, 80, WALK + 2, 53)
    carve(w, 81, WALK, 36, 83, WALK + 5, 66)
    carve(w, 84, WALK, 60, 90, WALK + 3, 66)
    w.place(O.TREASURE, 88, WALK, 64)
    carve(w, 84, WALK, 51, 84, WALK + 1, 53)
    crate_wall([(84, y, z) for y in (WALK, WALK + 1) for z in (51, 52, 53)])
    carve(w, 85, WALK, 49, 91, WALK + 3, 55)
    w.place(O.TREASURE, 89, WALK, 52)
    carve(w, 84, WALK, 38, 84, WALK + 1, 40)
    crate_wall([(84, y, z) for y in (WALK, WALK + 1) for z in (38, 39, 40)])
    carve(w, 85, WALK, 36, 91, WALK + 3, 42)
    w.place(O.TREASURE, 90, WALK, 37)
    w.place(O.EGG, 87, WALK, 41)
    carve(w, 79, HIGH, 57, 80, HIGH + 2, 59)                             # the ledge
    w.place(O.TREASURE, 79, HIGH, 58)
    crate(81, WALK, 58) if solved else crate(82, WALK, 55)
    for k, z in enumerate((35, 34, 33)):                                # the stair: 8, 10, 12
        carve(w, 81, WALK, z, 83, WALK + 8, z)
        w.fill(81, FLOOR, z, 83, WALK + 1 + 2 * k, z, STONE)
    carve(w, 81, WALK + 6, 30, 83, WALK + 8, 32)
    w.place(O.EXIT, 82, WALK + 6, 31)
    fork_alcove(w, 81, WALK, 46, (-1, 0))
    fork_alcove(w, 83, WALK, 62, (1, 0))

    dress(w, 18, 0, 28, 95, 16, 70, rng)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
